#ifndef MOVEPICKER_H
#define MOVEPICKER_H

#include <array>
#include <cstddef>

#include "chesslib.hpp"

// Material values used by SEE and move ordering. These are ordering values,
// not the engine's UCI centipawn scale.
inline constexpr int PIECE_VALUES[6] = {
    208,    // Pawn
    781,    // Knight
    825,    // Bishop
    1276,   // Rook
    2538,   // Queen
    20000   // King (used only by SEE)
};

// Static Exchange Evaluation. `see_ge` provides the threshold interface used
// by move ordering; special moves follow Stockfish's inexpensive convention.
int see(const chess::Board& board, const chess::Move& move) noexcept;

bool see_ge(const chess::Board& board,
            const chess::Move& move,
            int threshold) noexcept;

class MovePicker {
public:
    struct MoveEntry {
        chess::Move move = chess::Move::NO_MOVE;
        int score = 0;
    };

    enum class Stage {
        TT_MOVE,
        PREPARE_TACTICALS,
        GOOD_TACTICALS,
        PREPARE_QUIETS,
        GOOD_QUIETS,
        BAD_CAPTURES,
        BAD_QUIETS,
        PREPARE_EVASIONS,
        EVASIONS,
        DONE
    };

    MovePicker(const chess::Board& board,
               const chess::Move* moves,
               std::size_t count,
               chess::Move tt_move = chess::Move::NO_MOVE,
               const chess::Move* killer_moves = nullptr,
               const int (*history)[64] = nullptr,
               const int (*capture_history)[64][6] = nullptr,
               const int (*continuation_history)[64][6][64] = nullptr,
               const int (*low_ply_history)[64] = nullptr,
               const chess::PieceType* previous_pieces = nullptr,
               const chess::Square* previous_squares = nullptr,
               int ply = 0,
               int depth = 1) noexcept;

    MovePicker(const MovePicker&) = delete;
    MovePicker& operator=(const MovePicker&) = delete;

    chess::Move next_move() noexcept;

    [[nodiscard]] int last_see() const noexcept { return last_see_; }
    [[nodiscard]] bool last_see_valid() const noexcept { return last_see_valid_; }

private:
    [[nodiscard]] bool is_tactical(const chess::Move& move) const noexcept;
    [[nodiscard]] int tactical_score(const chess::Move& move) const noexcept;
    [[nodiscard]] int quiet_score(const chess::Move& move) const noexcept;
    [[nodiscard]] int evasion_score(const chess::Move& move) const noexcept;

    void prepare_tacticals() noexcept;
    void prepare_quiets() noexcept;
    void prepare_evasions() noexcept;
    void build_threat_masks() noexcept;
    bool take_move(chess::Move move) noexcept;

    const chess::Board& board_;
    std::array<MoveEntry, 256> moves_{};
    std::size_t size_ = 0;

    chess::Move tt_move_ = chess::Move::NO_MOVE;
    chess::Move killer_moves_[2] = {
        chess::Move::NO_MOVE,
        chess::Move::NO_MOVE
    };

    const int (*history_)[64] = nullptr;
    const int (*capture_history_)[64][6] = nullptr;
    const int (*continuation_history_)[64][6][64] = nullptr;
    const int (*low_ply_history_)[64] = nullptr;
    const chess::PieceType* previous_pieces_ = nullptr;
    const chess::Square* previous_squares_ = nullptr;

    int ply_ = 0;
    int depth_ = 1;
    bool in_check_ = false;
    bool threats_built_ = false;
    std::array<chess::Bitboard, 6> threat_by_lesser_{};

    // `moves_` is partitioned lazily: tactical moves first, quiet moves next.
    // Bad captures are moved to the beginning of the same buffer and emitted
    // after good quiet moves, avoiding a second MAX_MOVES-sized stack array.
    std::size_t tactical_count_ = 0;
    std::size_t tactical_cursor_ = 0;
    std::size_t quiet_begin_ = 0;
    std::size_t quiet_cursor_ = 0;
    std::size_t bad_capture_count_ = 0;
    std::size_t bad_capture_cursor_ = 0;
    std::size_t evasion_cursor_ = 0;

    Stage stage_ = Stage::TT_MOVE;
    int last_see_ = 0;
    bool last_see_valid_ = false;
};

#endif // MOVEPICKER_H
