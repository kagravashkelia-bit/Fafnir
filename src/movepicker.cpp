#include "movepicker.h"

#include <algorithm>

namespace {

chess::PieceType least_valuable_attacker(const chess::Board& board,
                                         chess::Square square,
                                         chess::Color side,
                                         chess::Bitboard& occupied,
                                         chess::Square& attacker_square) noexcept {
    const chess::Bitboard pawns =
        board.pieces(chess::PieceType::PAWN, side) & occupied;
    const chess::Bitboard pawn_attackers =
        chess::attacks::pawn(~side, square) & pawns;
    if (pawn_attackers) {
        attacker_square = static_cast<chess::Square>(pawn_attackers.lsb());
        return chess::PieceType::PAWN;
    }

    const chess::Bitboard knights =
        board.pieces(chess::PieceType::KNIGHT, side) & occupied;
    const chess::Bitboard knight_attackers =
        chess::attacks::knight(square) & knights;
    if (knight_attackers) {
        attacker_square = static_cast<chess::Square>(knight_attackers.lsb());
        return chess::PieceType::KNIGHT;
    }

    const chess::Bitboard bishops =
        board.pieces(chess::PieceType::BISHOP, side) & occupied;
    const chess::Bitboard rooks =
        board.pieces(chess::PieceType::ROOK, side) & occupied;
    const chess::Bitboard queens =
        board.pieces(chess::PieceType::QUEEN, side) & occupied;

    chess::Bitboard bishop_attacks = 0;
    if (bishops | queens) {
        bishop_attacks = chess::attacks::bishop(square, occupied);
        const chess::Bitboard attackers = bishop_attacks & bishops;
        if (attackers) {
            attacker_square = static_cast<chess::Square>(attackers.lsb());
            return chess::PieceType::BISHOP;
        }
    }

    chess::Bitboard rook_attacks = 0;
    if (rooks | queens) {
        rook_attacks = chess::attacks::rook(square, occupied);
        const chess::Bitboard attackers = rook_attacks & rooks;
        if (attackers) {
            attacker_square = static_cast<chess::Square>(attackers.lsb());
            return chess::PieceType::ROOK;
        }
    }

    const chess::Bitboard queen_attackers =
        (bishop_attacks | rook_attacks) & queens;
    if (queen_attackers) {
        attacker_square = static_cast<chess::Square>(queen_attackers.lsb());
        return chess::PieceType::QUEEN;
    }

    const chess::Bitboard kings =
        board.pieces(chess::PieceType::KING, side) & occupied;
    const chess::Bitboard king_attackers =
        chess::attacks::king(square) & kings;
    if (king_attackers) {
        attacker_square = static_cast<chess::Square>(king_attackers.lsb());
        return chess::PieceType::KING;
    }

    return chess::PieceType::NONE;
}

}  // namespace

int see(const chess::Board& board, const chess::Move& move) noexcept {
    const chess::Square from = move.from();
    const chess::Square to = move.to();
    const chess::Piece victim = board.at(to);

    int gain[32];
    int depth = 0;
    chess::PieceType attacker_type = board.at(from).type();
    chess::Color side = board.sideToMove();

    gain[depth] =
        move.typeOf() == chess::Move::ENPASSANT
            ? PIECE_VALUES[static_cast<int>(chess::PieceType::PAWN)]
            : victim != chess::Piece::NONE
                  ? PIECE_VALUES[static_cast<int>(victim.type())]
                  : 0;
    if (move.typeOf() == chess::Move::PROMOTION) {
        gain[depth] +=
            PIECE_VALUES[static_cast<int>(move.promotionType())] -
            PIECE_VALUES[static_cast<int>(chess::PieceType::PAWN)];
        attacker_type = move.promotionType();
    }

    chess::Bitboard occupied = board.occ();
    occupied.clear(from.index());
    if (move.typeOf() == chess::Move::ENPASSANT) {
        const int captured_pawn_square =
            static_cast<int>(to.index()) +
            (side == chess::Color::WHITE ? -8 : 8);
        occupied.clear(captured_pawn_square);
    }

    chess::Square attacker_square = from;
    int current_piece_value = PIECE_VALUES[static_cast<int>(attacker_type)];

    while (true) {
        ++depth;
        side = ~side;
        gain[depth] = current_piece_value - gain[depth - 1];
        if (std::max(-gain[depth - 1], gain[depth]) < 0)
            break;

        const chess::PieceType next_attacker =
            least_valuable_attacker(board, to, side, occupied, attacker_square);
        if (next_attacker == chess::PieceType::NONE)
            break;

        occupied.clear(attacker_square.index());
        current_piece_value =
            PIECE_VALUES[static_cast<int>(next_attacker)];
    }

    while (--depth > 0)
        gain[depth - 1] = -std::max(-gain[depth - 1], gain[depth]);

    return gain[0];
}

bool see_ge(const chess::Board& board,
            const chess::Move& move,
            int threshold) noexcept {
    return see(board, move) >= threshold;
}

MovePicker::MovePicker(const chess::Board& board,
                       const chess::Move* moves,
                       std::size_t count,
                       chess::Move tt_move,
                       const chess::Move* killer_moves,
                       const int (*history)[64],
                       const int (*capture_history)[64][6],
                       const int (*continuation_history)[64][6][64],
                       const int (*low_ply_history)[64],
                       const chess::PieceType* previous_pieces,
                       const chess::Square* previous_squares,
                       int ply) noexcept
    : board_(board),
      size_(std::min(count, moves_.size())),
      tt_move_(tt_move),
      history_(history),
      capture_history_(capture_history),
      continuation_history_(continuation_history),
      low_ply_history_(low_ply_history),
      previous_pieces_(previous_pieces),
      previous_squares_(previous_squares),
      ply_(ply),
      in_check_(board.inCheck()) {
    bool has_quiet_moves = false;
    for (std::size_t i = 0; i < size_; ++i) {
        moves_[i].move = moves[i];
        has_quiet_moves = has_quiet_moves || !is_tactical(moves[i]);
    }

    if (has_quiet_moves) {
        const chess::Color enemy = ~board_.sideToMove();
        chess::Bitboard pawn_attacks = 0;
        chess::Bitboard pawns = board_.pieces(chess::PieceType::PAWN, enemy);
        while (pawns) {
            const chess::Square square = pawns.pop();
            pawn_attacks |= chess::attacks::pawn(enemy, square);
        }

        chess::Bitboard knight_attacks = 0;
        chess::Bitboard knights = board_.pieces(chess::PieceType::KNIGHT, enemy);
        while (knights) {
            const chess::Square square = knights.pop();
            knight_attacks |= chess::attacks::knight(square);
        }

        chess::Bitboard bishop_attacks = 0;
        chess::Bitboard bishops = board_.pieces(chess::PieceType::BISHOP, enemy);
        while (bishops) {
            const chess::Square square = bishops.pop();
            bishop_attacks |= chess::attacks::bishop(square, board_.occ());
        }

        chess::Bitboard rook_attacks = 0;
        chess::Bitboard rooks = board_.pieces(chess::PieceType::ROOK, enemy);
        while (rooks) {
            const chess::Square square = rooks.pop();
            rook_attacks |= chess::attacks::rook(square, board_.occ());
        }

        threat_by_lesser_[static_cast<int>(chess::PieceType::PAWN)] = 0;
        threat_by_lesser_[static_cast<int>(chess::PieceType::KNIGHT)] =
            pawn_attacks;
        threat_by_lesser_[static_cast<int>(chess::PieceType::BISHOP)] =
            pawn_attacks;
        threat_by_lesser_[static_cast<int>(chess::PieceType::ROOK)] =
            pawn_attacks | knight_attacks | bishop_attacks;
        threat_by_lesser_[static_cast<int>(chess::PieceType::QUEEN)] =
            pawn_attacks | knight_attacks | bishop_attacks | rook_attacks;
        threat_by_lesser_[static_cast<int>(chess::PieceType::KING)] = 0;
    }

    if (killer_moves) {
        killer_moves_[0] = killer_moves[0];
        killer_moves_[1] = killer_moves[1];
    }
}

bool MovePicker::is_tactical(const chess::Move& move) const noexcept {
    return board_.isCapture(move) ||
           move.typeOf() == chess::Move::PROMOTION;
}

int MovePicker::tactical_score(const chess::Move& move) const noexcept {
    const chess::PieceType attacker_type = board_.at(move.from()).type();
    const chess::Piece victim = board_.at(move.to());
    const int victim_value =
        move.typeOf() == chess::Move::ENPASSANT
            ? PIECE_VALUES[static_cast<int>(chess::PieceType::PAWN)]
            : victim == chess::Piece::NONE
                  ? 0
                  : PIECE_VALUES[static_cast<int>(victim.type())];
    const chess::PieceType victim_type =
        move.typeOf() == chess::Move::ENPASSANT
            ? chess::PieceType::PAWN
            : victim == chess::Piece::NONE
                  ? chess::PieceType::NONE
                  : victim.type();
    const int history =
        capture_history_ && board_.isCapture(move)
            ? capture_history_[static_cast<int>(attacker_type)]
                             [move.to().index()]
                             [static_cast<int>(victim_type)]
            : 0;
    const int promotion_gain =
        move.typeOf() == chess::Move::PROMOTION
            ? PIECE_VALUES[static_cast<int>(move.promotionType())] -
                  PIECE_VALUES[static_cast<int>(chess::PieceType::PAWN)]
            : 0;

    if (move.typeOf() == chess::Move::PROMOTION)
        return 1000000 + promotion_gain + victim_value * 7 + history;

    return victim_value * 7 + history;
}

int MovePicker::quiet_score(const chess::Move& move) const noexcept {
    int score = history_
        ? 2 * history_[move.from().index()][move.to().index()]
        : 0;

    if (continuation_history_ && previous_pieces_ && previous_squares_) {
        constexpr int CONTINUATION_PLY_OFFSETS[] = {1, 2, 3, 5};
        const int current_piece = static_cast<int>(board_.at(move.from()).type());
        for (const int offset : CONTINUATION_PLY_OFFSETS) {
            const int previous_ply = ply_ - offset;
            if (previous_ply < 0)
                continue;
            const int previous_piece =
                static_cast<int>(previous_pieces_[previous_ply]);
            if (previous_piece >= 6)
                continue;
            const int previous_to =
                previous_squares_[previous_ply].index();
            score += continuation_history_[previous_piece][previous_to]
                                          [current_piece][move.to().index()];
        }
    }

    if (low_ply_history_ && ply_ < 4)
        score += 8 * low_ply_history_[move.from().index()][move.to().index()]
               / (1 + ply_);

    const int piece_type =
        static_cast<int>(board_.at(move.from()).type());
    const chess::Bitboard from = chess::Bitboard::fromSquare(move.from());
    const chess::Bitboard to = chess::Bitboard::fromSquare(move.to());
    const int threat_difference =
        20 * (static_cast<int>(bool(threat_by_lesser_[piece_type] & from)) -
              static_cast<int>(bool(threat_by_lesser_[piece_type] & to)));
    score += PIECE_VALUES[piece_type] * threat_difference;

    if (move == killer_moves_[0])
        score += 20000;
    else if (move == killer_moves_[1])
        score += 19000;

    if (board_.givesCheck(move) != chess::CheckType::NO_CHECK &&
        see_ge(board_, move, -75))
        score += 16384;

    return std::clamp(score, -60000, 60000);
}

int MovePicker::evasion_score(const chess::Move& move) const noexcept {
    if (board_.isCapture(move)) {
        const chess::Piece victim = board_.at(move.to());
        const int victim_value =
            move.typeOf() == chess::Move::ENPASSANT
                ? PIECE_VALUES[static_cast<int>(chess::PieceType::PAWN)]
                : victim == chess::Piece::NONE
                      ? 0
                      : PIECE_VALUES[static_cast<int>(victim.type())];
        return (1 << 28) + victim_value;
    }

    int score = history_
        ? history_[move.from().index()][move.to().index()]
        : 0;

    if (continuation_history_ && previous_pieces_ && previous_squares_ &&
        ply_ > 0) {
        const int previous_piece =
            static_cast<int>(previous_pieces_[ply_ - 1]);
        if (previous_piece < 6) {
            score += continuation_history_[previous_piece]
                [previous_squares_[ply_ - 1].index()]
                [static_cast<int>(board_.at(move.from()).type())]
                [move.to().index()];
        }
    }

    return score;
}

MovePicker::MoveEntry MovePicker::remove_move(std::size_t index) noexcept {
    MoveEntry selected = moves_[index];
    moves_[index] = moves_[--size_];
    return selected;
}

bool MovePicker::take_best(MoveEntry* entries,
                           std::size_t& count,
                           bool tactical,
                           MoveEntry& selected) noexcept {
    std::size_t best_index = count;

    for (std::size_t i = 0; i < count; ++i) {
        MoveEntry& entry = entries[i];
        if (tactical != is_tactical(entry.move))
            continue;

        if (!entry.score_valid) {
            entry.score = tactical ? tactical_score(entry.move)
                                   : quiet_score(entry.move);
            entry.score_valid = true;
        }

        if (best_index == count || entry.score > entries[best_index].score)
            best_index = i;
    }

    if (best_index == count)
        return false;

    selected = entries[best_index];
    entries[best_index] = entries[--count];
    return true;
}

bool MovePicker::take_best_quiet(MoveEntry& selected, bool good) noexcept {
    constexpr int GOOD_QUIET_THRESHOLD = -14000;
    std::size_t best_index = size_;

    for (std::size_t i = 0; i < size_; ++i) {
        MoveEntry& entry = moves_[i];
        if (is_tactical(entry.move))
            continue;

        if (!entry.score_valid) {
            entry.score = quiet_score(entry.move);
            entry.score_valid = true;
        }

        if (good
                ? entry.score <= GOOD_QUIET_THRESHOLD
                : entry.score > GOOD_QUIET_THRESHOLD)
            continue;

        if (best_index == size_ || entry.score > moves_[best_index].score)
            best_index = i;
    }

    if (best_index == size_)
        return false;

    selected = remove_move(best_index);
    return true;
}

bool MovePicker::take_best_evasion(MoveEntry& selected) noexcept {
    std::size_t best_index = size_;

    for (std::size_t i = 0; i < size_; ++i) {
        MoveEntry& entry = moves_[i];
        if (!entry.score_valid) {
            entry.score = evasion_score(entry.move);
            entry.score_valid = true;
        }
        if (best_index == size_ || entry.score > moves_[best_index].score)
            best_index = i;
    }

    if (best_index == size_)
        return false;

    selected = remove_move(best_index);
    return true;
}

bool MovePicker::take_move(chess::Move move) noexcept {
    if (move == chess::Move::NO_MOVE)
        return false;

    for (std::size_t i = 0; i < size_; ++i) {
        if (moves_[i].move == move) {
            (void) remove_move(i);
            return true;
        }
    }
    return false;
}

chess::Move MovePicker::next_move() noexcept {
    last_see_ = 0;
    last_see_valid_ = false;

    while (stage_ != Stage::DONE) {
        switch (stage_) {
        case Stage::TT_MOVE:
            stage_ = in_check_ ? Stage::EVASIONS : Stage::GOOD_TACTICALS;
            if (take_move(tt_move_)) {
                if (board_.isCapture(tt_move_)) {
                    last_see_ = see(board_, tt_move_);
                    last_see_valid_ = true;
                }
                return tt_move_;
            }
            break;

        case Stage::EVASIONS: {
            MoveEntry selected;
            if (take_best_evasion(selected)) {
                if (board_.isCapture(selected.move)) {
                    last_see_ = see(board_, selected.move);
                    last_see_valid_ = true;
                }
                return selected.move;
            }
            stage_ = Stage::DONE;
            break;
        }

        case Stage::GOOD_TACTICALS: {
            MoveEntry selected;
            if (!take_best(moves_.data(), size_, true, selected)) {
                stage_ = Stage::QUIETS;
                break;
            }

            const chess::Move move = selected.move;
            if (board_.isCapture(move)) {
                last_see_ = see(board_, move);
                last_see_valid_ = true;
                const int see_threshold = -selected.score / 18;
                if (last_see_ < see_threshold &&
                    move.typeOf() != chess::Move::PROMOTION) {
                    selected.score = tactical_score(move);
                    selected.score_valid = true;
                    selected.see_score = last_see_;
                    selected.see_valid = true;
                    bad_captures_[bad_capture_count_++] = selected;
                    break;
                }
            }
            return move;
        }

        case Stage::QUIETS: {
            MoveEntry selected;
            if (take_best_quiet(selected, true))
                return selected.move;
            stage_ = Stage::BAD_CAPTURES;
            break;
        }

        case Stage::BAD_CAPTURES: {
            MoveEntry selected;
            if (!take_best(bad_captures_.data(), bad_capture_count_,
                           true, selected)) {
                stage_ = Stage::BAD_QUIETS;
                break;
            }
            last_see_ = selected.see_score;
            last_see_valid_ = selected.see_valid;
            return selected.move;
        }

        case Stage::BAD_QUIETS: {
            MoveEntry selected;
            if (take_best_quiet(selected, false))
                return selected.move;
            stage_ = Stage::DONE;
            break;
        }

        case Stage::DONE:
            break;
        }
    }

    return chess::Move::NO_MOVE;
}