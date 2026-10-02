#pragma once

#include <array>
#include <cstdint>
#include <cmath>

#include "chesslib.hpp"
#include "probe.h"

#undef PAWN
#undef KNIGHT
#undef BISHOP
#undef ROOK
#undef QUEEN
#undef KING

// ============================================================================
// CONTEXTO NNUE POR HILO
// ============================================================================

struct alignas(64) RubiEvalContext {
    chess::Board position{};
    std::array<chess::Move, 512> move_stack{};

    // Máximo 32 piezas en una posición legal.
    alignas(64) int pieces[32]{};
    alignas(64) int squares[32]{};

    int move_count = 0;
    bool synchronized = false;

    RubiEvalContext() = default;
    ~RubiEvalContext() = default;
};

inline thread_local RubiEvalContext* tl_rubi_context = nullptr;

// ============================================================================
// OBTENER CONTEXTO
// ============================================================================

[[gnu::always_inline]]
inline RubiEvalContext& rubi_context() noexcept {
    if (!tl_rubi_context) [[unlikely]] {
        tl_rubi_context = new RubiEvalContext();
    }

    return *tl_rubi_context;
}

// ============================================================================
// SINCRONIZACIÓN
// ============================================================================

inline bool rubi_sync(const chess::Board& board) noexcept {
    RubiEvalContext& context = rubi_context();

    context.position = board;
    context.move_count = 0;
    context.synchronized = true;

    return true;
}

// ============================================================================
// PUSH / POP
// ============================================================================

[[gnu::always_inline]]
inline bool rubi_push(const chess::Move& move) noexcept {
    RubiEvalContext& context = rubi_context();

    if (!context.synchronized ||
        context.move_count >= 512) [[unlikely]] {

        context.synchronized = false;
        return false;
    }

    context.position.makeMove(move);
    context.move_stack[context.move_count++] = move;

    return true;
}

[[gnu::always_inline]]
inline void rubi_pop() noexcept {
    RubiEvalContext& context = rubi_context();

    if (context.synchronized &&
        context.move_count > 0) [[likely]] {

        --context.move_count;

        context.position.unmakeMove(
            context.move_stack[context.move_count]
        );
    }
    else {
        context.synchronized = false;
    }
}

// ============================================================================
// EXTRACCIÓN DE PIEZAS PARA STOCKFISH NNUE
// ============================================================================

template <int StockfishCode>
[[gnu::always_inline]]
inline void extract_pieces(
    chess::Bitboard bb,
    int* __restrict pieces,
    int* __restrict squares,
    int& count) noexcept
{
    while (bb) {
        const chess::Square sq = bb.lsb();
        const int idx = sq.index();

        pieces[count] = StockfishCode;
        squares[count] = idx;

        ++count;

        bb.clear(idx);
    }
}

// ============================================================================
// EVALUACIÓN NNUE
// ============================================================================
//
// IMPORTANTE:
//
// - evaluate.hpp NO decide small net / big net.
// - Esa decisión pertenece al Stockfish NNUE Probe.
// - El probe recibe pieces[], squares[], pieceAmount, side y rule50.
// - raw_eval() devuelve:
//
//      psqt
//      positional
//      nnue
//
// - Aquí solamente hacemos el escalado final que FlashBit necesita.
// ============================================================================

[[gnu::always_inline]]
inline int evaluate(const chess::Board& board) noexcept {

    RubiEvalContext& context = rubi_context();

    if (!context.synchronized) [[unlikely]] {
        if (!rubi_sync(board)) {
            return 0;
        }
    }

    int* __restrict p_pieces = context.pieces;
    int* __restrict p_squares = context.squares;

    int pieceAmount = 0;

    // ------------------------------------------------------------------------
    // BLANCAS
    // ------------------------------------------------------------------------

    extract_pieces<1>(
        board.pieces(
            chess::PieceType::PAWN,
            chess::Color::WHITE
        ),
        p_pieces,
        p_squares,
        pieceAmount
    );

    extract_pieces<2>(
        board.pieces(
            chess::PieceType::KNIGHT,
            chess::Color::WHITE
        ),
        p_pieces,
        p_squares,
        pieceAmount
    );

    extract_pieces<3>(
        board.pieces(
            chess::PieceType::BISHOP,
            chess::Color::WHITE
        ),
        p_pieces,
        p_squares,
        pieceAmount
    );

    extract_pieces<4>(
        board.pieces(
            chess::PieceType::ROOK,
            chess::Color::WHITE
        ),
        p_pieces,
        p_squares,
        pieceAmount
    );

    extract_pieces<5>(
        board.pieces(
            chess::PieceType::QUEEN,
            chess::Color::WHITE
        ),
        p_pieces,
        p_squares,
        pieceAmount
    );

    extract_pieces<6>(
        board.pieces(
            chess::PieceType::KING,
            chess::Color::WHITE
        ),
        p_pieces,
        p_squares,
        pieceAmount
    );

    // ------------------------------------------------------------------------
    // NEGRAS
    // ------------------------------------------------------------------------

    extract_pieces<9>(
        board.pieces(
            chess::PieceType::PAWN,
            chess::Color::BLACK
        ),
        p_pieces,
        p_squares,
        pieceAmount
    );

    extract_pieces<10>(
        board.pieces(
            chess::PieceType::KNIGHT,
            chess::Color::BLACK
        ),
        p_pieces,
        p_squares,
        pieceAmount
    );

    extract_pieces<11>(
        board.pieces(
            chess::PieceType::BISHOP,
            chess::Color::BLACK
        ),
        p_pieces,
        p_squares,
        pieceAmount
    );

    extract_pieces<12>(
        board.pieces(
            chess::PieceType::ROOK,
            chess::Color::BLACK
        ),
        p_pieces,
        p_squares,
        pieceAmount
    );

    extract_pieces<13>(
        board.pieces(
            chess::PieceType::QUEEN,
            chess::Color::BLACK
        ),
        p_pieces,
        p_squares,
        pieceAmount
    );

    extract_pieces<14>(
        board.pieces(
            chess::PieceType::KING,
            chess::Color::BLACK
        ),
        p_pieces,
        p_squares,
        pieceAmount
    );

    // ------------------------------------------------------------------------
    // INFORMACIÓN DE POSICIÓN
    // ------------------------------------------------------------------------

    const bool is_white =
        board.sideToMove() == chess::Color::WHITE;

    const int half_moves =
        static_cast<int>(board.halfMoveClock());

    // ------------------------------------------------------------------------
    // STOCKFISH NNUE PROBE
    // ------------------------------------------------------------------------
    //
    // El Probe se encarga internamente de toda la evaluación pesada.
    //
    // No hacemos aquí:
    //   - selección small/big net
    //   - construcción de acumuladores
    //   - evaluación de la red
    //   - cálculo de PSQT de la red
    //
    // raw_eval() simplemente nos devuelve los componentes ya calculados.
    // ------------------------------------------------------------------------

    const auto raw =
        Stockfish::Probe::raw_eval(
            p_pieces,
            p_squares,
            pieceAmount,
            is_white,
            half_moves
        );

    // ------------------------------------------------------------------------
    // COMPLEJIDAD NNUE
    // ------------------------------------------------------------------------

    const int nnueComplexity =
        std::abs(raw.psqt - raw.positional);

    // ------------------------------------------------------------------------
    // EVALUACIÓN NNUE
    // ------------------------------------------------------------------------
    //
    // raw.nnue ya viene calculado por el Probe.
    //
    // Aplicamos únicamente el dampening por complejidad.
    // ------------------------------------------------------------------------

    int nnue = raw.nnue;

    nnue -=
        nnue * nnueComplexity / 18000;

    // ------------------------------------------------------------------------
    // MATERIAL TOTAL
    //
    // Misma escala materialista utilizada por esta implementación.
    // ------------------------------------------------------------------------

    const int pawnCount =
        static_cast<int>(
            (
                board.pieces(
                    chess::PieceType::PAWN,
                    chess::Color::WHITE
                )
                |
                board.pieces(
                    chess::PieceType::PAWN,
                    chess::Color::BLACK
                )
            ).count()
        );

    const int knightCount =
        static_cast<int>(
            (
                board.pieces(
                    chess::PieceType::KNIGHT,
                    chess::Color::WHITE
                )
                |
                board.pieces(
                    chess::PieceType::KNIGHT,
                    chess::Color::BLACK
                )
            ).count()
        );

    const int bishopCount =
        static_cast<int>(
            (
                board.pieces(
                    chess::PieceType::BISHOP,
                    chess::Color::WHITE
                )
                |
                board.pieces(
                    chess::PieceType::BISHOP,
                    chess::Color::BLACK
                )
            ).count()
        );

    const int rookCount =
        static_cast<int>(
            (
                board.pieces(
                    chess::PieceType::ROOK,
                    chess::Color::WHITE
                )
                |
                board.pieces(
                    chess::PieceType::ROOK,
                    chess::Color::BLACK
                )
            ).count()
        );

    const int queenCount =
        static_cast<int>(
            (
                board.pieces(
                    chess::PieceType::QUEEN,
                    chess::Color::WHITE
                )
                |
                board.pieces(
                    chess::PieceType::QUEEN,
                    chess::Color::BLACK
                )
            ).count()
        );

    const int nonPawnMaterial =
          781 * knightCount
        + 825 * bishopCount
        + 1276 * rookCount
        + 2538 * queenCount;

    const int material =
        535 * pawnCount
        + nonPawnMaterial;

    // ------------------------------------------------------------------------
    // ESCALADO FINAL
    // ------------------------------------------------------------------------

    int v =
        nnue * (77777 + material) / 77777;

    // ------------------------------------------------------------------------
    // RULE 50
    // ------------------------------------------------------------------------

    v -=
        v * half_moves / 212;

    return v;
}