#pragma once

#include <array>
#include <cstdint>

#include "chesslib.hpp"
#include "nnue_probe.h"

#undef PAWN
#undef KNIGHT
#undef BISHOP
#undef ROOK
#undef QUEEN
#undef KING

struct alignas(64) FafnirEvalContext {
    chess::Board position{};
    std::array<chess::Move, 512> move_stack{};
    int move_count = 0;
    bool synchronized = false;
};

inline thread_local FafnirEvalContext* tl_fafnir_context = nullptr;

[[gnu::always_inline]]
inline FafnirEvalContext& fafnir_context() {
    if (!tl_fafnir_context) [[unlikely]]
        tl_fafnir_context = new FafnirEvalContext();
    return *tl_fafnir_context;
}

inline bool fafnir_sync(const chess::Board& board) {
    FafnirEvalContext& context = fafnir_context();
    context.synchronized = false;
    Stockfish::Probe::sync(board.getFen().c_str());
    context.position = board;
    context.move_count = 0;
    context.synchronized = true;
    return true;
}

[[gnu::always_inline]]
inline bool fafnir_push(const chess::Move& move) {
    FafnirEvalContext& context = fafnir_context();
    if (!context.synchronized || context.move_count >= 512) [[unlikely]] {
        context.synchronized = false;
        return false;
    }

    if (!Stockfish::Probe::push(context.position, move)) [[unlikely]] {
        context.synchronized = false;
        return false;
    }

    context.position.makeMove(move);
    context.move_stack[context.move_count++] = move;
    return true;
}

[[gnu::always_inline]]
inline void fafnir_pop() noexcept {
    FafnirEvalContext& context = fafnir_context();
    if (context.synchronized && context.move_count > 0) [[likely]] {
        Stockfish::Probe::pop();
        context.position.unmakeMove(context.move_stack[--context.move_count]);
    } else {
        context.synchronized = false;
    }
}

[[gnu::always_inline]]
inline int evaluate(const chess::Board& board, int optimism = 0) {
    FafnirEvalContext& context = fafnir_context();
    if (!context.synchronized || context.position.hash() != board.hash()) [[unlikely]]
        fafnir_sync(board);
    return Stockfish::Probe::eval_current(optimism).value;
}
