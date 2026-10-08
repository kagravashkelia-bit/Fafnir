#ifndef MOVEPICKER_H
#define MOVEPICKER_H

#include <array>
#include <cstddef>

#include "chesslib.hpp"

// Valores base de piezas para SEE y heurística MVV-LVA
constexpr int PIECE_VALUES[6] = {
    208,   // Pawn
    781,   // Knight
    825,   // Bishop
    1276,  // Rook
    2538,  // Queen
    20000  // King
};

// Evaluación Estática de Intercambios (SEE)
int see(const chess::Board& board, const chess::Move& move) noexcept;

bool see_ge(const chess::Board& board,
            const chess::Move& move,
            int threshold) noexcept;

class MovePicker {
public:
    struct MoveEntry {
        chess::Move move = chess::Move::NO_MOVE;
        int score = 0;
        bool score_valid = false;
        int see_score = 0;
        bool see_valid = false;
    };

    enum class Stage {
        TT_MOVE,
        EVASIONS,
        GOOD_TACTICALS,
        QUIETS,
        BAD_CAPTURES,
        BAD_QUIETS,
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
               int ply = 0) noexcept;

    chess::Move next_move() noexcept;

    [[nodiscard]] int last_see() const noexcept { return last_see_; }
    [[nodiscard]] bool last_see_valid() const noexcept { return last_see_valid_; }

private:
    [[nodiscard]] bool is_tactical(const chess::Move& move) const noexcept;
    [[nodiscard]] int tactical_score(const chess::Move& move) const noexcept;
    [[nodiscard]] int quiet_score(const chess::Move& move) const noexcept;
    [[nodiscard]] int evasion_score(const chess::Move& move) const noexcept;

    MoveEntry remove_move(std::size_t index) noexcept;
    bool take_best(MoveEntry* entries,
                   std::size_t& count,
                   bool tactical,
                   MoveEntry& selected) noexcept;
    bool take_best_quiet(MoveEntry& selected, bool good) noexcept;
    bool take_best_evasion(MoveEntry& selected) noexcept;
    bool take_move(chess::Move move) noexcept;

    const chess::Board& board_;
    
    // El número máximo teórico de jugadas en una posición de ajedrez es 218
    std::array<MoveEntry, 256> moves_{};
    std::size_t size_ = 0;

    chess::Move tt_move_ = chess::Move::NO_MOVE;
    chess::Move killer_moves_[2] = {chess::Move::NO_MOVE, chess::Move::NO_MOVE};
    const int (*history_)[64] = nullptr;
    const int (*capture_history_)[64][6] = nullptr;
    const int (*continuation_history_)[64][6][64] = nullptr;
    const int (*low_ply_history_)[64] = nullptr;
    const chess::PieceType* previous_pieces_ = nullptr;
    const chess::Square* previous_squares_ = nullptr;
    int ply_ = 0;
    bool in_check_ = false;
    std::array<chess::Bitboard, 6> threat_by_lesser_{};

    std::array<MoveEntry, 256> bad_captures_{};
    std::size_t bad_capture_count_ = 0;

    Stage stage_ = Stage::TT_MOVE;

    int last_see_ = 0;
    bool last_see_valid_ = false;
};

#endif // MOVEPICKER_H