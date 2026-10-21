#include "sfdev_probe.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "chesslib.hpp"
#include "sfdev/src/attacks.h"
#include "sfdev/src/position.h"
#include "sfdev/src/evaluate.h"
#include "sfdev/src/nnue/network.h"
#include "sfdev/src/nnue/nnue_accumulator.h"
#include "sfdev/src/nnue/nnue_misc.h"

#if defined(__GNUC__) || defined(__clang__)
#define PROBE_INLINE inline __attribute__((always_inline))
#define PROBE_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define PROBE_INLINE inline
#define PROBE_UNLIKELY(x) (x)
#endif

// Poner a 1 para verificar con Position::legal() cada jugada empujada (mas lento).
// La busqueda de FlashBit solo empuja jugadas legales y ademas comprueba el hash
// del contexto antes de cada push, por lo que por defecto NO hace falta.
#ifndef FB_PROBE_VALIDATE_MOVES
#define FB_PROBE_VALIDATE_MOVES 0
#endif

namespace Stockfish::Probe {
namespace {

using Stockfish::Eval::NNUE::AccumulatorCaches;
using Stockfish::Eval::NNUE::AccumulatorStack;
using Stockfish::Eval::NNUE::EvalFile;
using Stockfish::Eval::NNUE::Network;

std::once_flag initialize_once;
EvalFile       eval_file;
std::unique_ptr<Network> network;
const Network* g_network = nullptr;       // puntero crudo: camino rapido sin call_once

struct alignas(Stockfish::Eval::NNUE::CacheLineSize) ThreadContext {
    alignas(Stockfish::Eval::NNUE::CacheLineSize) AccumulatorStack accumulators;
    alignas(Stockfish::Eval::NNUE::CacheLineSize) AccumulatorCaches caches;
    Position position;
    std::array<StateInfo, AccumulatorStack::MaxSize> states{};
    std::array<Stockfish::Move, AccumulatorStack::MaxSize> moves{};
    std::array<bool, AccumulatorStack::MaxSize> is_null{};
    std::size_t move_count = 0;
    bool synchronized = false;

    explicit ThreadContext(const Network& loaded_network) : caches(loaded_network) {}
};

// Puntero trivial (sin guard de TLS) + propietario aparte
thread_local ThreadContext* tl_ctx = nullptr;
thread_local std::unique_ptr<ThreadContext> tl_owner;

PROBE_INLINE const Network& get_network() {
    if (PROBE_UNLIKELY(!g_network)) {
        init();
        if (!g_network) throw std::runtime_error("SFNNv17 network has not been initialized");
    }
    return *g_network;
}

[[gnu::noinline]] ThreadContext& create_context() {
    const Network& net = get_network();
    tl_owner = std::make_unique<ThreadContext>(net);
    tl_ctx = tl_owner.get();
    return *tl_ctx;
}

PROBE_INLINE ThreadContext& context() {
    if (PROBE_UNLIKELY(!tl_ctx)) return create_context();
    return *tl_ctx;
}

void set_position(Position& position, const std::string& fen, StateInfo& state) {
    init();
    if (const auto error = position.set(fen, false, &state)) throw std::invalid_argument(error->what());
}

std::string fen_from_pieces(const int pieces[], const int squares[], int piece_amount, bool white_to_move,
                            int rule50) {
    if (!pieces || !squares || piece_amount < 2 || piece_amount > 32)
        throw std::invalid_argument("Invalid NNUE piece list");

    // Stockfish piece encoding: white pieces 1..6, black pieces 9..14.
    constexpr char piece_chars[] = ".PNBRQK..pnbrqk";
    std::array<char, 64> board{};

    for (int i = 0; i < piece_amount; ++i) {
        const int piece = pieces[i];
        const int square = squares[i];
        if (piece <= 0 || piece >= static_cast<int>(sizeof(piece_chars) - 1) || piece_chars[piece] == '.' ||
            square < 0 || square >= static_cast<int>(board.size()) || board[square] != '\0')
            throw std::invalid_argument("Invalid piece or square in NNUE position");
        board[square] = piece_chars[piece];
    }

    std::string fen;
    for (int rank = 7; rank >= 0; --rank) {
        if (rank != 7) fen += '/';
        int empty_squares = 0;
        for (int file = 0; file < 8; ++file) {
            const char piece = board[rank * 8 + file];
            if (piece == '\0') { ++empty_squares; continue; }
            if (empty_squares != 0) { fen += static_cast<char>('0' + empty_squares); empty_squares = 0; }
            fen += piece;
        }
        if (empty_squares != 0) fen += static_cast<char>('0' + empty_squares);
    }

    // A piece-only input cannot recover castling rights or an en-passant square.
    fen += white_to_move ? " w - - " : " b - - ";
    fen += std::to_string(std::max(0, rule50));
    fen += " 1";
    return fen;
}

PROBE_INLINE Stockfish::Move translate_move(const chess::Move& move) {
    const auto from = static_cast<Stockfish::Square>(move.from().index());
    auto to = static_cast<Stockfish::Square>(move.to().index());

    switch (move.typeOf()) {
    case chess::Move::PROMOTION: {
        Stockfish::PieceType promotion = Stockfish::KNIGHT;
        switch (move.promotionType().internal()) {
        case chess::PieceType::BISHOP: promotion = Stockfish::BISHOP; break;
        case chess::PieceType::ROOK:   promotion = Stockfish::ROOK; break;
        case chess::PieceType::QUEEN:  promotion = Stockfish::QUEEN; break;
        default: break;
        }
        return Stockfish::Move::make<Stockfish::PROMOTION>(from, to, promotion);
    }
    case chess::Move::ENPASSANT:
        return Stockfish::Move::make<Stockfish::EN_PASSANT>(from, to);
    case chess::Move::CASTLING: {
        // Stockfish encodes castling as king-square -> rook-square.
        const int rank = static_cast<int>(move.from().rank());
        const int rook_file = static_cast<int>(move.to().file()) > static_cast<int>(move.from().file()) ? 7 : 0;
        to = static_cast<Stockfish::Square>(rank * 8 + rook_file);
        return Stockfish::Move::make<Stockfish::CASTLING>(from, to);
    }
    default:
        return Stockfish::Move::make<Stockfish::NORMAL>(from, to);
    }
}

// ---------------------------------------------------------------------------
//  Salida de la red: SF (con PSQT) devuelve pair<psqt, positional>; una red
//  unificada devuelve un escalar. Se admiten ambas sin tocar la formula.
// ---------------------------------------------------------------------------
template<class T, class = void>
struct IsPairLike : std::false_type {};
template<class T>
struct IsPairLike<T, std::void_t<decltype(std::get<0>(std::declval<T>())), decltype(std::get<1>(std::declval<T>()))>>
    : std::true_type {};

template<class T>
PROBE_INLINE std::pair<int, int> split_network_output(const T& out) {
    if constexpr (IsPairLike<T>::value)
        return {static_cast<int>(std::get<0>(out)), static_cast<int>(std::get<1>(out))};
    else
        return {static_cast<int>(out), static_cast<int>(out)};   // sin PSQT: complejidad = 0
}

// ---------------------------------------------------------------------------
//  Eval::evaluate de Stockfish (evaluate.cpp), parte posterior a la red
// ---------------------------------------------------------------------------
PROBE_INLINE Evaluation finish_eval(int psqt, int positional, int optimism, int pawn_count_total,
                                    int non_pawn_material_total, int rule50) {
    Stockfish::Value nnue = (125 * psqt + 131 * positional) / 128;
    const Stockfish::Value raw_nnue = nnue;

    // Blend optimism and eval with nnue complexity
    const int nnueComplexity = std::abs(psqt - positional);
    optimism += optimism * nnueComplexity / 468;
    nnue -= nnue * nnueComplexity / 18000;

    const int material = 535 * pawn_count_total + non_pawn_material_total;
    int v = (nnue * (77777 + material) + optimism * (7777 + material)) / 77777;

    // Damp down the evaluation linearly when shuffling
    v -= v * rule50 / 212;

    // Guarantee evaluation does not hit the tablebase range
    v = std::clamp(v, Stockfish::VALUE_TB_LOSS_IN_MAX_PLY + 1, Stockfish::VALUE_TB_WIN_IN_MAX_PLY - 1);
    return {raw_nnue, v};
}

PROBE_INLINE Evaluation evaluate_position(Position& position, AccumulatorStack& accumulators,
                                          AccumulatorCaches& caches, int optimism) {
    // No lanza si se pide eval en jaque: la busqueda de SF tampoco evalua en jaque.
    const auto out = get_network().evaluate(position, accumulators, caches);
    const auto [psqt, positional] = split_network_output(out);
    return finish_eval(psqt, positional, optimism, position.count<Stockfish::PAWN>(),
                       position.non_pawn_material(), position.rule50_count());
}

// ---------------------------------------------------------------------------
//  Null move: se prueban las firmas habituales de Position::do_null_move.
//  Si ninguna compila, push_null() devuelve false (el motor hace resync).
// ---------------------------------------------------------------------------
template<int N> struct Rank : Rank<N - 1> {};
template<> struct Rank<0> {};

#ifdef FB_PROBE_NULL_TT   // p. ej. -DFB_PROBE_NULL_TT=my_dummy_tt() si tu firma exige una TT por referencia
template<class P>
auto do_null_impl(P& p, StateInfo& st, Rank<3>) -> decltype(p.do_null_move(st, FB_PROBE_NULL_TT), bool()) {
    p.do_null_move(st, FB_PROBE_NULL_TT);
    return true;
}
#endif
template<class P>
auto do_null_impl(P& p, StateInfo& st, Rank<2>) -> decltype(p.do_null_move(st), bool()) {
    p.do_null_move(st);
    return true;
}
template<class P>
auto do_null_impl(P& p, StateInfo& st, Rank<1>) -> decltype(p.do_null_move(st, nullptr), bool()) {
    p.do_null_move(st, nullptr);
    return true;
}
template<class P>
bool do_null_impl(P&, StateInfo&, Rank<0>) {
    return false;
}

}  // namespace

void init() {
    std::call_once(initialize_once, [] {
        Stockfish::Attacks::init();
        Position::init();

        network = std::make_unique<Network>();
        network->load({}, {}, eval_file);

        if (eval_file.current != std::filesystem::path(eval_file.defaultName))
            throw std::runtime_error("Failed to load the embedded SFNNv17 network");

        g_network = network.get();
    });
}

void sync(const char* fen) {
    if (!fen) throw std::invalid_argument("NNUE FEN must not be null");

    init();
    ThreadContext& ctx = context();

    ctx.synchronized = false;
    ctx.move_count = 0;

    if (const auto error = ctx.position.set(fen, false, &ctx.states[0])) throw std::invalid_argument(error->what());

    ctx.accumulators.reset();
    ctx.synchronized = true;
}

bool push(const chess::Board&, const chess::Move& move) {
    ThreadContext& ctx = context();

    if (PROBE_UNLIKELY(!ctx.synchronized || ctx.move_count + 2 >= ctx.states.size())) return false;

    const Stockfish::Move sf_move = translate_move(move);
#if FB_PROBE_VALIDATE_MOVES
    if (!ctx.position.legal(sf_move)) return false;
#endif

    auto& dirties = ctx.accumulators.push();

    ctx.position.do_move(sf_move, ctx.states[ctx.move_count + 1], ctx.position.gives_check(sf_move), dirties, nullptr,
                         nullptr);

    ctx.moves[ctx.move_count]   = sf_move;
    ctx.is_null[ctx.move_count] = false;
    ++ctx.move_count;
    return true;
}

bool push_null(const chess::Board&) {
    ThreadContext& ctx = context();

    if (PROBE_UNLIKELY(!ctx.synchronized || ctx.move_count + 2 >= ctx.states.size())) return false;

    // Como en SF: el null move NO apila acumuladores (las caracteristicas no cambian)
    if (!do_null_impl(ctx.position, ctx.states[ctx.move_count + 1], Rank<3>{})) return false;

    ctx.moves[ctx.move_count]   = Stockfish::Move::null();
    ctx.is_null[ctx.move_count] = true;
    ++ctx.move_count;
    return true;
}

void pop() noexcept {
    ThreadContext* ctx = tl_ctx;
    if (!ctx || !ctx->synchronized || ctx->move_count == 0) return;

    --ctx->move_count;
    if (ctx->is_null[ctx->move_count]) {
        ctx->position.undo_null_move();
    } else {
        ctx->position.undo_move(ctx->moves[ctx->move_count]);
        ctx->accumulators.pop();
    }
}

Evaluation eval_current(int optimism) {
    ThreadContext& ctx = context();
    if (PROBE_UNLIKELY(!ctx.synchronized)) throw std::runtime_error("SFNNv17 NNUE position is not synchronized");
    return evaluate_position(ctx.position, ctx.accumulators, ctx.caches, optimism);
}

int eval(const char* fen, int optimism) { return eval_detailed(fen, optimism).value; }

Evaluation eval_detailed(const char* fen, int optimism) {
    if (!fen) throw std::invalid_argument("NNUE FEN must not be null");

    init();
    StateInfo state{};
    Position position;
    set_position(position, fen, state);

    ThreadContext& ctx = context();
    alignas(Stockfish::Eval::NNUE::CacheLineSize) AccumulatorStack accumulators;
    return evaluate_position(position, accumulators, ctx.caches, optimism);
}

RawEval raw_eval(const char* fen) {
    if (!fen) throw std::invalid_argument("NNUE FEN must not be null");

    init();
    StateInfo state{};
    Position position;
    set_position(position, fen, state);

    ThreadContext& ctx = context();
    alignas(Stockfish::Eval::NNUE::CacheLineSize) AccumulatorStack accumulators;
    const auto out = get_network().evaluate(position, accumulators, ctx.caches);
    const auto [psqt, positional] = split_network_output(out);
    return {(125 * psqt + 131 * positional) / 128};
}

int eval(const int pieces[], const int squares[], int piece_amount, bool side, int rule50, int optimism) {
    return eval_detailed(pieces, squares, piece_amount, side, rule50, optimism).value;
}

Evaluation eval_detailed(const int pieces[], const int squares[], int piece_amount, bool side, int rule50,
                         int optimism) {
    init();
    StateInfo state{};
    const std::string fen = fen_from_pieces(pieces, squares, piece_amount, side, rule50);

    Position position;
    set_position(position, fen, state);

    ThreadContext& ctx = context();
    alignas(Stockfish::Eval::NNUE::CacheLineSize) AccumulatorStack accumulators;
    return evaluate_position(position, accumulators, ctx.caches, optimism);
}

RawEval raw_eval(const int pieces[], const int squares[], int piece_amount, bool side, int rule50) {
    init();
    StateInfo state{};
    const std::string fen = fen_from_pieces(pieces, squares, piece_amount, side, rule50);

    Position position;
    set_position(position, fen, state);

    ThreadContext& ctx = context();
    alignas(Stockfish::Eval::NNUE::CacheLineSize) AccumulatorStack accumulators;
    const auto out = get_network().evaluate(position, accumulators, ctx.caches);
    const auto [psqt, positional] = split_network_output(out);
    return {(125 * psqt + 131 * positional) / 128};
}

}  // namespace Stockfish::Probe