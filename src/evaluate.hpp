#pragma once

#include <array>

#include "chesslib.hpp"
#include "probe.h"

#undef PAWN
#undef KNIGHT
#undef BISHOP
#undef ROOK
#undef QUEEN
#undef KING

struct RubiEvalContext {
    chess::Board position{};
    std::array<chess::Move, 512> move_stack{};
    int move_count = 0;
    bool synchronized = false;

    RubiEvalContext() = default;
    ~RubiEvalContext() = default;
};

inline RubiEvalContext& rubi_context() {
    thread_local RubiEvalContext context;
    return context;
}

inline thread_local int nnue_search_depth = 0;

inline void set_nnue_search_depth(int depth) {
    nnue_search_depth = depth;
}

inline bool rubi_sync(const chess::Board& board) {
    RubiEvalContext& context = rubi_context();
    context.position = board;
    context.move_count = 0;
    context.synchronized = true;
    return true;
}

inline bool rubi_push(const chess::Move& move) {
    RubiEvalContext& context = rubi_context();
    if (!context.synchronized || context.move_count >= static_cast<int>(context.move_stack.size())) {
        context.synchronized = false;
        return false;
    }

    context.position.makeMove(move);
    context.move_stack[context.move_count++] = move;
    return true;
}

inline void rubi_pop() {
    RubiEvalContext& context = rubi_context();
    if (context.synchronized && context.move_count > 0) {
        context.move_count--;
        context.position.unmakeMove(context.move_stack[context.move_count]);
    } else {
        context.synchronized = false;
    }
}

inline int evaluate(const chess::Board& board) {
    RubiEvalContext& context = rubi_context();
    if (!context.synchronized) {
        if (!rubi_sync(board)) {
            return 0;
        }
    }
    int piece_board[64] = {};
    constexpr int stockfish_piece_codes[2][6] = {
        {1, 2, 3, 4, 5, 6},
        {9, 10, 11, 12, 13, 14}
    };

    for (int color = 0; color < 2; ++color) {
        const chess::Color side = static_cast<chess::Color>(color);
        for (int type = 0; type < 6; ++type) {
            const auto piece_type = static_cast<chess::PieceType::underlying>(type);
            chess::Bitboard pieces = board.pieces(chess::PieceType(piece_type), side);
            while (pieces) {
                const chess::Square square = pieces.lsb();
                piece_board[square.index()] = stockfish_piece_codes[color][type];
                pieces.clear(square.index());
            }
        }
    }

    try {
        const bool use_small_net = nnue_search_depth > 0 && nnue_search_depth <= 5;
        return Stockfish::Probe::eval(piece_board, board.sideToMove() == chess::Color::WHITE,
                                      static_cast<int>(board.halfMoveClock()), use_small_net);
    } catch (...) {
        context.synchronized = false;
        return 0;
    }
}