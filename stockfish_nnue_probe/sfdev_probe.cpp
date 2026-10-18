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

#include "chesslib.hpp"
#include "sfdev/src/attacks.h"
#include "sfdev/src/position.h"
#include "sfdev/src/evaluate.h"
#include "sfdev/src/nnue/network.h"
#include "sfdev/src/nnue/nnue_accumulator.h"
#include "sfdev/src/nnue/nnue_misc.h"

namespace Stockfish::Probe {
namespace {

using Stockfish::Eval::NNUE::AccumulatorCaches;
using Stockfish::Eval::NNUE::AccumulatorStack;
using Stockfish::Eval::NNUE::EvalFile;
using Stockfish::Eval::NNUE::Network;

std::once_flag initialize_once;
EvalFile eval_file;
std::unique_ptr<Network> network;

struct alignas(Stockfish::Eval::NNUE::CacheLineSize) ThreadContext {
    alignas(Stockfish::Eval::NNUE::CacheLineSize) AccumulatorStack accumulators;
    alignas(Stockfish::Eval::NNUE::CacheLineSize) AccumulatorCaches caches;
    Position position;
    std::array<StateInfo, AccumulatorStack::MaxSize> states{};
    std::array<Stockfish::Move, AccumulatorStack::MaxSize> moves{};
    std::size_t move_count = 0;
    bool synchronized = false;

    explicit ThreadContext(const Network& loaded_network) : caches(loaded_network) {}
};

thread_local std::unique_ptr<ThreadContext> thread_context;

const Network& get_network() {
    // Make every public entry point safe even if the caller forgot init().
    init();

    if (!network)
        throw std::runtime_error("SFNNv17 network has not been initialized");

    return *network;
}

ThreadContext& context() {
    const Network& loaded_network = get_network();

    if (!thread_context)
        thread_context = std::make_unique<ThreadContext>(loaded_network);

    return *thread_context;
}

void set_position(Position& position, const std::string& fen, StateInfo& state) {
    // Position::set() relies on Stockfish's attack and position tables.
    init();

    if (const auto error = position.set(fen, false, &state))
        throw std::invalid_argument(error->what());
}

std::string fen_from_pieces(const int pieces[], const int squares[],
                            int piece_amount, bool white_to_move, int rule50) {
    if (!pieces || !squares || piece_amount < 2 || piece_amount > 32)
        throw std::invalid_argument("Invalid NNUE piece list");

    // Stockfish piece encoding: white pieces 1..6, black pieces 9..14.
    constexpr char piece_chars[] = ".PNBRQK..pnbrqk";
    std::array<char, 64> board{};

    for (int i = 0; i < piece_amount; ++i) {
        const int piece = pieces[i];
        const int square = squares[i];

        if (piece <= 0 || piece >= static_cast<int>(sizeof(piece_chars) - 1)
            || piece_chars[piece] == '.'
            || square < 0 || square >= static_cast<int>(board.size())
            || board[square] != '\0')
            throw std::invalid_argument("Invalid piece or square in NNUE position");

        board[square] = piece_chars[piece];
    }

    std::string fen;
    for (int rank = 7; rank >= 0; --rank) {
        if (rank != 7)
            fen += '/';

        int empty_squares = 0;
        for (int file = 0; file < 8; ++file) {
            const char piece = board[rank * 8 + file];

            if (piece == '\0') {
                ++empty_squares;
                continue;
            }

            if (empty_squares != 0) {
                fen += static_cast<char>('0' + empty_squares);
                empty_squares = 0;
            }

            fen += piece;
        }

        if (empty_squares != 0)
            fen += static_cast<char>('0' + empty_squares);
    }

    // A piece-only input cannot recover castling rights or an en-passant square.
    fen += white_to_move ? " w - - " : " b - - ";
    fen += std::to_string(std::max(0, rule50));
    fen += " 1";
    return fen;
}

Stockfish::Move translate_move(const chess::Move& move) {
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
        const int rook_file = static_cast<int>(move.to().file())
                                    > static_cast<int>(move.from().file())
                                  ? 7
                                  : 0;
        to = static_cast<Stockfish::Square>(rank * 8 + rook_file);
        return Stockfish::Move::make<Stockfish::CASTLING>(from, to);
    }

    default:
        return Stockfish::Move::make<Stockfish::NORMAL>(from, to);
    }
}

Evaluation evaluate_position(Position& position, AccumulatorStack& accumulators,
                             AccumulatorCaches& caches, int optimism) {
    // Do not throw if the caller asks for a score while in check: this probe can
    // be called from an external engine search, and an exception here would abort
    // the whole engine. Stockfish's normal search avoids static evaluation in check;
    // for strict Stockfish-search parity, the caller should do the same. The network
    // and scalar formula below are still computed for callers that explicitly ask.
    const Stockfish::Value nnue =
        get_network().evaluate(position, accumulators, caches);

    const Stockfish::Color side = position.side_to_move();
    const Stockfish::Value simple_eval =
        Stockfish::PawnValue
            * (position.count<Stockfish::PAWN>(side)
               - position.count<Stockfish::PAWN>(~side))
        + position.non_pawn_material(side) - position.non_pawn_material(~side);

    // Correlation between material and NNUE, matching the scalar-eval Stockfish code.
    const int simple_normalized =
        (simple_eval * 1024) / (std::abs(simple_eval) + 1024);
    const int nnue_normalized =
        (nnue * 1024) / (std::abs(nnue) + 1024);
    const int alignment = (simple_normalized * nnue_normalized) / 512;

    int value = nnue + (nnue * alignment) / 65536
                      + (optimism * alignment) / 16384;

    // Use 64-bit arithmetic before multiplication to avoid signed-int overflow.
    const int material =
        521 * position.count<Stockfish::PAWN>() + position.non_pawn_material();
    value += static_cast<int>(static_cast<std::int64_t>(material) * value / 90649);

    // Stockfish rule-50 damping and tablebase score guard.
    value -= value * position.rule50_count() / 189;
    value = std::clamp(value,
                       Stockfish::VALUE_TB_LOSS_IN_MAX_PLY + 1,
                       Stockfish::VALUE_TB_WIN_IN_MAX_PLY - 1);

    return {nnue, value};
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
    });
}

void sync(const char* fen) {
    if (!fen)
        throw std::invalid_argument("NNUE FEN must not be null");

    init();
    ThreadContext& ctx = context();

    ctx.synchronized = false;
    ctx.move_count = 0;

    if (const auto error = ctx.position.set(fen, false, &ctx.states[0]))
        throw std::invalid_argument(error->what());

    ctx.accumulators.reset();
    ctx.synchronized = true;
}

bool push(const chess::Board&, const chess::Move& move) {
    ThreadContext& ctx = context();

    if (!ctx.synchronized || ctx.move_count + 1 >= ctx.states.size())
        return false;

    // Reject illegal/stale moves before touching the move or accumulator stacks.
    const Stockfish::Move sf_move = translate_move(move);
    if (!ctx.position.legal(sf_move))
        return false;

    auto& dirties = ctx.accumulators.push();

    ctx.position.do_move(sf_move,
                         ctx.states[ctx.move_count + 1],
                         ctx.position.gives_check(sf_move),
                         dirties,
                         nullptr,
                         nullptr);

    ctx.moves[ctx.move_count++] = sf_move;
    return true;
}

void pop() noexcept {
    if (!thread_context || !thread_context->synchronized
        || thread_context->move_count == 0)
        return;

    ThreadContext& ctx = *thread_context;
    ctx.position.undo_move(ctx.moves[--ctx.move_count]);
    ctx.accumulators.pop();
}

Evaluation eval_current(int optimism) {
    ThreadContext& ctx = context();

    if (!ctx.synchronized)
        throw std::runtime_error("SFNNv17 NNUE position is not synchronized");

    return evaluate_position(ctx.position, ctx.accumulators, ctx.caches, optimism);
}

int eval(const char* fen, int optimism) {
    return eval_detailed(fen, optimism).value;
}

Evaluation eval_detailed(const char* fen, int optimism) {
    if (!fen)
        throw std::invalid_argument("NNUE FEN must not be null");

    init();
    StateInfo state{};
    Position position;
    set_position(position, fen, state);

    ThreadContext& ctx = context();
    alignas(Stockfish::Eval::NNUE::CacheLineSize) AccumulatorStack accumulators;
    return evaluate_position(position, accumulators, ctx.caches, optimism);
}

RawEval raw_eval(const char* fen) {
    if (!fen)
        throw std::invalid_argument("NNUE FEN must not be null");

    init();
    StateInfo state{};
    Position position;
    set_position(position, fen, state);

    ThreadContext& ctx = context();
    alignas(Stockfish::Eval::NNUE::CacheLineSize) AccumulatorStack accumulators;
    return {get_network().evaluate(position, accumulators, ctx.caches)};
}

int eval(const int pieces[], const int squares[], int piece_amount,
         bool side, int rule50, int optimism) {
    return eval_detailed(pieces, squares, piece_amount, side, rule50, optimism).value;
}

Evaluation eval_detailed(const int pieces[], const int squares[], int piece_amount,
                         bool side, int rule50, int optimism) {
    init();
    StateInfo state{};
    const std::string fen =
        fen_from_pieces(pieces, squares, piece_amount, side, rule50);

    Position position;
    set_position(position, fen, state);

    ThreadContext& ctx = context();
    alignas(Stockfish::Eval::NNUE::CacheLineSize) AccumulatorStack accumulators;
    return evaluate_position(position, accumulators, ctx.caches, optimism);
}

RawEval raw_eval(const int pieces[], const int squares[], int piece_amount,
                 bool side, int rule50) {
    init();
    StateInfo state{};
    const std::string fen =
        fen_from_pieces(pieces, squares, piece_amount, side, rule50);

    Position position;
    set_position(position, fen, state);

    ThreadContext& ctx = context();
    alignas(Stockfish::Eval::NNUE::CacheLineSize) AccumulatorStack accumulators;
    return {get_network().evaluate(position, accumulators, ctx.caches)};
}

}  // namespace Stockfish::Probe
