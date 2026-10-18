#include "movepicker.h"

#include <algorithm>
#include <limits>

namespace {

constexpr int GOOD_QUIET_THRESHOLD = -14000;
constexpr int EVASION_CAPTURE_BONUS = 1 << 28;
constexpr int QUIET_SORT_LIMIT_PER_PLY = -3560;

[[nodiscard]] constexpr bool valid_piece_type(chess::PieceType type) noexcept {
    const int value = static_cast<int>(type);
    return value >= static_cast<int>(chess::PieceType::PAWN)
        && value <= static_cast<int>(chess::PieceType::KING);
}

[[nodiscard]] constexpr int piece_value(chess::PieceType type) noexcept {
    return valid_piece_type(type)
        ? PIECE_VALUES[static_cast<int>(type)]
        : 0;
}

[[nodiscard]] int captured_piece_value(const chess::Board& board,
                                       const chess::Move& move) noexcept {
    if (move.typeOf() == chess::Move::ENPASSANT)
        return PIECE_VALUES[static_cast<int>(chess::PieceType::PAWN)];

    const chess::Piece victim = board.at(move.to());
    return victim == chess::Piece::NONE ? 0 : piece_value(victim.type());
}

[[nodiscard]] chess::PieceType captured_piece_type(
    const chess::Board& board,
    const chess::Move& move) noexcept {
    if (move.typeOf() == chess::Move::ENPASSANT)
        return chess::PieceType::PAWN;

    const chess::Piece victim = board.at(move.to());
    return victim == chess::Piece::NONE
        ? chess::PieceType::NONE
        : victim.type();
}

// Finds the least valuable pseudo-legal attacker of `target` for `side`.
// Occupancy is updated during SEE, so slider x-rays are recalculated naturally.
chess::PieceType least_valuable_attacker(const chess::Board& board,
                                         chess::Square target,
                                         chess::Color side,
                                         chess::Bitboard& occupied,
                                         chess::Square& attacker_square) noexcept {
    const chess::Bitboard pawns =
        board.pieces(chess::PieceType::PAWN, side) & occupied;
    const chess::Bitboard pawn_attackers =
        chess::attacks::pawn(~side, target) & pawns;
    if (pawn_attackers) {
        attacker_square = static_cast<chess::Square>(pawn_attackers.lsb());
        return chess::PieceType::PAWN;
    }

    const chess::Bitboard knights =
        board.pieces(chess::PieceType::KNIGHT, side) & occupied;
    const chess::Bitboard knight_attackers =
        chess::attacks::knight(target) & knights;
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
        bishop_attacks = chess::attacks::bishop(target, occupied);
        const chess::Bitboard attackers = bishop_attacks & bishops;
        if (attackers) {
            attacker_square = static_cast<chess::Square>(attackers.lsb());
            return chess::PieceType::BISHOP;
        }
    }

    chess::Bitboard rook_attacks = 0;
    if (rooks | queens) {
        rook_attacks = chess::attacks::rook(target, occupied);
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
        chess::attacks::king(target) & kings;
    if (king_attackers) {
        attacker_square = static_cast<chess::Square>(king_attackers.lsb());
        return chess::PieceType::KING;
    }

    return chess::PieceType::NONE;
}

// Stockfish-style partial insertion sort: entries scoring below `limit` remain
// in an unspecified order; entries at or above it are moved to the front and
// sorted descending. This is useful for quiets, where the tail is only needed
// after the higher-quality moves have been tried.
void partial_insertion_sort(MovePicker::MoveEntry* begin,
                            MovePicker::MoveEntry* end,
                            int limit) noexcept {
    if (begin == end)
        return;

    MovePicker::MoveEntry* sorted_end = begin;
    for (MovePicker::MoveEntry* p = begin + 1; p < end; ++p) {
        if (p->score < limit)
            continue;

        const MovePicker::MoveEntry selected = *p;
        *p = *++sorted_end;

        MovePicker::MoveEntry* q = sorted_end;
        while (q != begin && (q - 1)->score < selected.score) {
            *q = *(q - 1);
            --q;
        }
        *q = selected;
    }
}

} // namespace

int see(const chess::Board& board, const chess::Move& move) noexcept {
    const chess::Square from = move.from();
    const chess::Square to = move.to();
    const chess::Piece moving_piece = board.at(from);

    if (moving_piece == chess::Piece::NONE)
        return 0;

    int gain[33] = {};
    int depth = 0;

    chess::PieceType current_type = moving_piece.type();
    const chess::Color moving_side = board.sideToMove();

    gain[0] = captured_piece_value(board, move);
    if (move.typeOf() == chess::Move::PROMOTION) {
        gain[0] += piece_value(move.promotionType())
                 - PIECE_VALUES[static_cast<int>(chess::PieceType::PAWN)];
        current_type = move.promotionType();
    }

    chess::Bitboard occupied = board.occ();
    occupied.clear(from.index());

    if (move.typeOf() == chess::Move::ENPASSANT) {
        const int captured_pawn_square =
            static_cast<int>(to.index())
            + (moving_side == chess::Color::WHITE ? -8 : 8);
        if (captured_pawn_square >= 0 && captured_pawn_square < 64)
            occupied.clear(captured_pawn_square);
    }

    int current_piece_value = piece_value(current_type);
    chess::Color side = moving_side;

    while (true) {
        side = ~side;
        if (depth >= 31)
            break;

        ++depth;
        gain[depth] = current_piece_value - gain[depth - 1];

        // Standard swap-list pruning: if the side to move can already decline
        // the exchange, deeper recaptures cannot change the result.
        if (std::max(-gain[depth - 1], gain[depth]) < 0)
            break;

        chess::Square attacker_square = from;
        const chess::PieceType next_type =
            least_valuable_attacker(board, to, side, occupied, attacker_square);
        if (next_type == chess::PieceType::NONE)
            break;

        // A king cannot capture onto a square defended by the other side. The
        // board API does not expose Stockfish's cached pin/blocker bitboards, so
        // this check is deliberately conservative and remains an SEE heuristic.
        if (next_type == chess::PieceType::KING) {
            chess::Bitboard after_king_move = occupied;
            after_king_move.clear(attacker_square.index());
            chess::Square defender_square = attacker_square;
            if (least_valuable_attacker(board, to, ~side, after_king_move,
                                        defender_square) != chess::PieceType::NONE)
                break;
        }

        occupied.clear(attacker_square.index());
        current_piece_value = piece_value(next_type);

        // A pawn recapturing on the last rank promotes. Use a queen as the
        // default promotion for the exchange sequence.
        const int rank = static_cast<int>(to.index()) / 8;
        const bool promotes = next_type == chess::PieceType::PAWN
            && ((side == chess::Color::WHITE && rank == 7)
                || (side == chess::Color::BLACK && rank == 0));
        if (promotes) {
            gain[depth] += PIECE_VALUES[static_cast<int>(chess::PieceType::QUEEN)]
                         - PIECE_VALUES[static_cast<int>(chess::PieceType::PAWN)];
            current_piece_value = PIECE_VALUES[static_cast<int>(chess::PieceType::QUEEN)];
        }
    }

    while (--depth > 0)
        gain[depth - 1] = -std::max(-gain[depth - 1], gain[depth]);

    return gain[0];
}

bool see_ge(const chess::Board& board,
            const chess::Move& move,
            int threshold) noexcept {
    // Recent Stockfish uses this cheap convention for special moves; normal
    // moves use the full exchange calculation. The caller can use `see()` when
    // it also needs the exchange score itself.
    if (move.typeOf() != chess::Move::NORMAL)
        return 0 >= threshold;

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
                       int ply,
                       int depth) noexcept
    : board_(board),
      size_(moves ? std::min(count, moves_.size()) : 0),
      tt_move_(tt_move),
      history_(history),
      capture_history_(capture_history),
      continuation_history_(continuation_history),
      low_ply_history_(low_ply_history),
      previous_pieces_(previous_pieces),
      previous_squares_(previous_squares),
      ply_(std::max(ply, 0)),
      depth_(std::max(depth, 1)),
      in_check_(board.inCheck()) {
    for (std::size_t i = 0; i < size_; ++i)
        moves_[i].move = moves[i];

    if (killer_moves) {
        killer_moves_[0] = killer_moves[0];
        killer_moves_[1] = killer_moves[1];
    }
}

bool MovePicker::is_tactical(const chess::Move& move) const noexcept {
    return board_.isCapture(move)
        || move.typeOf() == chess::Move::PROMOTION;
}

int MovePicker::tactical_score(const chess::Move& move) const noexcept {
    const chess::Piece moving_piece = board_.at(move.from());
    const chess::PieceType attacker_type =
        moving_piece == chess::Piece::NONE
            ? chess::PieceType::NONE
            : moving_piece.type();
    const chess::PieceType victim_type = captured_piece_type(board_, move);
    const int victim_value = captured_piece_value(board_, move);

    int history_value = 0;
    if (capture_history_ && board_.isCapture(move)
        && valid_piece_type(attacker_type)
        && valid_piece_type(victim_type)) {
        history_value = capture_history_[static_cast<int>(attacker_type)]
                                        [move.to().index()]
                                        [static_cast<int>(victim_type)];
    }

    int promotion_gain = 0;
    if (move.typeOf() == chess::Move::PROMOTION) {
        promotion_gain = piece_value(move.promotionType())
                       - PIECE_VALUES[static_cast<int>(chess::PieceType::PAWN)];
    }

    // MVV plus capture history, with a moderate promotion bonus. Avoid the
    // previous 1,000,000 sentinel: it distorted the relative ordering of all
    // promotions versus captures.
    return 7 * victim_value + history_value + 4 * promotion_gain;
}

int MovePicker::quiet_score(const chess::Move& move) const noexcept {
    const int from = move.from().index();
    const int to = move.to().index();
    const chess::Piece moving_piece = board_.at(move.from());
    if (moving_piece == chess::Piece::NONE)
        return std::numeric_limits<int>::min() / 2;

    const int current_piece = static_cast<int>(moving_piece.type());
    int score = history_
        ? 2 * history_[from][to]
        : 0;

    // Continuation history uses the same offsets as the current Stockfish
    // picker: previous plies 1, 2, 3, 4 and 6.
    if (continuation_history_ && previous_pieces_ && previous_squares_) {
        constexpr int CONTINUATION_PLY_OFFSETS[] = {1, 2, 3, 4, 6};
        for (const int offset : CONTINUATION_PLY_OFFSETS) {
            const int previous_ply = ply_ - offset;
            if (previous_ply < 0)
                continue;

            const int previous_piece =
                static_cast<int>(previous_pieces_[previous_ply]);
            if (previous_piece < 0 || previous_piece >= 6)
                continue;

            const int previous_to = previous_squares_[previous_ply].index();
            score += continuation_history_[previous_piece][previous_to]
                                           [current_piece][to];
        }
    }

    // Keep FlashBit's current low-ply history layout [from][to] so this remains
    // drop-in compatible with the existing search/history allocation.
    if (low_ply_history_ && ply_ < 4) {
        score += 8 * low_ply_history_[from][to] / (1 + ply_);
    }

    const chess::Bitboard from_bb = chess::Bitboard::fromSquare(move.from());
    const chess::Bitboard to_bb = chess::Bitboard::fromSquare(move.to());
    const chess::Bitboard threat_mask =
        threat_by_lesser_[current_piece];
    const int threat_delta = 20 * (
        static_cast<int>(bool(threat_mask & from_bb))
        - static_cast<int>(bool(threat_mask & to_bb)));
    score += piece_value(moving_piece.type()) * threat_delta;

    if (move == killer_moves_[0])
        score += 20000;
    else if (move == killer_moves_[1])
        score += 19000;

    if (board_.givesCheck(move) != chess::CheckType::NO_CHECK
        && see_ge(board_, move, -75)) {
        score += 16384;
    }

    return score;
}

int MovePicker::evasion_score(const chess::Move& move) const noexcept {
    if (board_.isCapture(move))
        return EVASION_CAPTURE_BONUS + captured_piece_value(board_, move);

    const int from = move.from().index();
    const int to = move.to().index();
    const chess::Piece moving_piece = board_.at(move.from());
    if (moving_piece == chess::Piece::NONE)
        return std::numeric_limits<int>::min() / 2;

    int score = history_ ? history_[from][to] : 0;

    if (continuation_history_ && previous_pieces_ && previous_squares_ && ply_ > 0) {
        const int previous_piece = static_cast<int>(previous_pieces_[ply_ - 1]);
        if (previous_piece >= 0 && previous_piece < 6) {
            score += continuation_history_[previous_piece]
                                           [previous_squares_[ply_ - 1].index()]
                                           [static_cast<int>(moving_piece.type())]
                                           [to];
        }
    }

    if (move.typeOf() == chess::Move::PROMOTION) {
        score += piece_value(move.promotionType())
               - PIECE_VALUES[static_cast<int>(chess::PieceType::PAWN)];
    }

    return score;
}

void MovePicker::build_threat_masks() noexcept {
    threats_built_ = true;
    threat_by_lesser_.fill(0);

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
    threat_by_lesser_[static_cast<int>(chess::PieceType::KNIGHT)] = pawn_attacks;
    threat_by_lesser_[static_cast<int>(chess::PieceType::BISHOP)] = pawn_attacks;
    threat_by_lesser_[static_cast<int>(chess::PieceType::ROOK)] =
        pawn_attacks | knight_attacks | bishop_attacks;
    threat_by_lesser_[static_cast<int>(chess::PieceType::QUEEN)] =
        pawn_attacks | knight_attacks | bishop_attacks | rook_attacks;
    threat_by_lesser_[static_cast<int>(chess::PieceType::KING)] = 0;
}

void MovePicker::prepare_tacticals() noexcept {
    auto begin = moves_.begin();
    auto end = begin + static_cast<std::ptrdiff_t>(size_);
    const auto middle = std::partition(begin, end, [this](const MoveEntry& entry) {
        return is_tactical(entry.move);
    });

    tactical_count_ = static_cast<std::size_t>(middle - begin);
    quiet_begin_ = tactical_count_;
    tactical_cursor_ = 0;
    bad_capture_count_ = 0;
    bad_capture_cursor_ = 0;

    for (std::size_t i = 0; i < tactical_count_; ++i)
        moves_[i].score = tactical_score(moves_[i].move);

    std::sort(moves_.begin(), moves_.begin() + static_cast<std::ptrdiff_t>(tactical_count_),
              [](const MoveEntry& lhs, const MoveEntry& rhs) {
                  return lhs.score > rhs.score;
              });
}

void MovePicker::prepare_quiets() noexcept {
    quiet_cursor_ = quiet_begin_;
    if (quiet_begin_ >= size_)
        return;

    if (!threats_built_)
        build_threat_masks();

    for (std::size_t i = quiet_begin_; i < size_; ++i)
        moves_[i].score = quiet_score(moves_[i].move);

    const int limit = QUIET_SORT_LIMIT_PER_PLY * depth_;
    partial_insertion_sort(moves_.data() + quiet_begin_,
                           moves_.data() + size_,
                           limit);
}

void MovePicker::prepare_evasions() noexcept {
    for (std::size_t i = 0; i < size_; ++i)
        moves_[i].score = evasion_score(moves_[i].move);

    std::sort(moves_.begin(), moves_.begin() + static_cast<std::ptrdiff_t>(size_),
              [](const MoveEntry& lhs, const MoveEntry& rhs) {
                  return lhs.score > rhs.score;
              });
    evasion_cursor_ = 0;
}

bool MovePicker::take_move(chess::Move move) noexcept {
    if (move == chess::Move::NO_MOVE)
        return false;

    for (std::size_t i = 0; i < size_; ++i) {
        if (moves_[i].move != move)
            continue;

        --size_;
        if (i != size_)
            moves_[i] = moves_[size_];
        return true;
    }
    return false;
}

chess::Move MovePicker::next_move() noexcept {
    last_see_ = 0;
    last_see_valid_ = false;

    while (stage_ != Stage::DONE) {
        switch (stage_) {
        case Stage::TT_MOVE:
            stage_ = in_check_ ? Stage::PREPARE_EVASIONS
                               : Stage::PREPARE_TACTICALS;
            if (take_move(tt_move_)) {
                if (board_.isCapture(tt_move_)) {
                    last_see_ = see(board_, tt_move_);
                    last_see_valid_ = true;
                }
                return tt_move_;
            }
            break;

        case Stage::PREPARE_TACTICALS:
            prepare_tacticals();
            stage_ = Stage::GOOD_TACTICALS;
            break;

        case Stage::GOOD_TACTICALS:
            while (tactical_cursor_ < tactical_count_) {
                const std::size_t selected_index = tactical_cursor_++;
                const chess::Move move = moves_[selected_index].move;
                const int score = moves_[selected_index].score;
                const int see_value = see(board_, move);
                const int threshold = -score / 18;
                const bool see_ok = move.typeOf() == chess::Move::NORMAL
                    ? see_value >= threshold
                    : threshold <= 0;

                if (!see_ok && move.typeOf() != chess::Move::PROMOTION) {
                    // Collect bad captures into the already-processed prefix,
                    // as in Stockfish's single-buffer MovePicker.
                    std::swap(moves_[bad_capture_count_], moves_[selected_index]);
                    ++bad_capture_count_;
                    continue;
                }

                if (board_.isCapture(move)) {
                    last_see_ = see_value;
                    last_see_valid_ = true;
                }
                return move;
            }

            stage_ = Stage::PREPARE_QUIETS;
            break;

        case Stage::PREPARE_QUIETS:
            prepare_quiets();
            stage_ = Stage::GOOD_QUIETS;
            break;

        case Stage::GOOD_QUIETS:
            while (quiet_cursor_ < size_) {
                const MoveEntry& entry = moves_[quiet_cursor_++];
                if (entry.score > GOOD_QUIET_THRESHOLD)
                    return entry.move;
            }
            bad_capture_cursor_ = 0;
            stage_ = Stage::BAD_CAPTURES;
            break;

        case Stage::BAD_CAPTURES:
            if (bad_capture_cursor_ < bad_capture_count_) {
                const chess::Move move = moves_[bad_capture_cursor_++].move;
                last_see_ = see(board_, move);
                last_see_valid_ = true;
                return move;
            }
            quiet_cursor_ = quiet_begin_;
            stage_ = Stage::BAD_QUIETS;
            break;

        case Stage::BAD_QUIETS:
            while (quiet_cursor_ < size_) {
                const MoveEntry& entry = moves_[quiet_cursor_++];
                if (entry.score <= GOOD_QUIET_THRESHOLD)
                    return entry.move;
            }
            stage_ = Stage::DONE;
            break;

        case Stage::PREPARE_EVASIONS:
            prepare_evasions();
            stage_ = Stage::EVASIONS;
            break;

        case Stage::EVASIONS:
            if (evasion_cursor_ < size_) {
                const chess::Move move = moves_[evasion_cursor_++].move;
                if (board_.isCapture(move)) {
                    last_see_ = see(board_, move);
                    last_see_valid_ = true;
                }
                return move;
            }
            stage_ = Stage::DONE;
            break;

        case Stage::DONE:
            break;
        }
    }

    return chess::Move::NO_MOVE;
}
