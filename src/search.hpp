#pragma once
#include <limits>
#include <vector>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <stdexcept>
#include <cmath>
#include <mutex>
#include <thread>
#include "pst.hpp"
#include "chesslib.hpp"
#include "evaluate.hpp"
#include "movepicker.h"

// ============================================================================
// 0. CONTROL DE TIEMPO Y LONGJUMP (Zero-Cost Overhead)
// ============================================================================
// pos_history movido a ThreadWorker
constexpr int NO_EVAL = 9999999; // Valor centinela fuera del rango normal de evaluación

constexpr int MAX_PLY = 128;

struct ThreadWorker {
    int id = 0;
    uint64_t nodos_busqueda = 0;
    std::vector<uint64_t> pos_history;
    chess::Move killer_moves[MAX_PLY][2] = {};
    int cont_history[2][6][64][6][64] = {};
    int capture_history[2][6][64][6] = {};
    int low_ply_history[4][2][64][64] = {};
    chess::PieceType moved_piece_stack[MAX_PLY] = {};
    chess::Square    moved_to_stack[MAX_PLY]    = {};
    int history_table[2][64][64] = {};
    int static_eval_stack[MAX_PLY] = {};
    int optimism = 0;
    chess::Move pv_table[MAX_PLY][MAX_PLY] = {};
    int pv_length[MAX_PLY] = {};
    int sel_depth = 0;
};

inline bool is_repetition(const ThreadWorker& worker, const chess::Board& board) {
    const int size = static_cast<int>(worker.pos_history.size());

    // No hay posiciones anteriores comparables.
    if (size < 3)
        return false;

    const int clock = std::min(
        board.halfMoveClock(),
        static_cast<unsigned>(size)
    );

    // La posición actual está en size - 1.
    // Una repetición debe estar 2 plies atrás, no 1.
    const int start = size - 3;
    const int end = std::max(0, size - 1 - clock);

    for (int i = start; i >= end; i -= 2) {
        if (worker.pos_history[i] == board.hash())
            return true;
    }

    return false;
}

extern std::atomic<bool> stop_search;
struct SearchAbortedException : public std::exception {};

inline std::chrono::high_resolution_clock::time_point search_start_time;
inline int search_max_time_ms = 0;
inline bool search_is_infinite = false;
inline bool search_is_pondering = false;
inline bool search_uses_ponder = false;
inline std::mutex search_time_mutex;
inline std::atomic<bool> ponderhit_requested{false};
inline uint64_t nodos_totales_busqueda = 0;
inline int search_aggressivity = 0;

inline int apply_aggressivity_to_optimism(int optimism) {
    const int baseline = std::clamp(optimism, -512, 512);
    return baseline * (100 + std::clamp(search_aggressivity, 0, 100)) / 100;
}

inline void process_ponderhit_request() {
    if (!ponderhit_requested.exchange(false, std::memory_order_relaxed))
        return;

    std::lock_guard<std::mutex> lock(search_time_mutex);
    if (search_is_pondering) {
        search_start_time = std::chrono::high_resolution_clock::now();
        search_is_pondering = false;
    }
}

inline bool is_search_pondering() {
    std::lock_guard<std::mutex> lock(search_time_mutex);
    return search_is_pondering;
}

inline std::chrono::high_resolution_clock::time_point search_start_time_snapshot() {
    std::lock_guard<std::mutex> lock(search_time_mutex);
    return search_start_time;
}

inline void init_search_time(int max_time_ms,
                             bool infinite = false,
                             bool pondering = false) {
    const auto start_time = std::chrono::high_resolution_clock::now();
    {
        std::lock_guard<std::mutex> lock(search_time_mutex);
        search_start_time = start_time;
        search_max_time_ms = max_time_ms;
        search_is_infinite = infinite;
        search_is_pondering = pondering;
        search_uses_ponder = pondering;
    }
    nodos_totales_busqueda = 0;
    process_ponderhit_request();
}

inline void check_time_and_longjump(const ThreadWorker& worker) {
    if (stop_search.load(std::memory_order_relaxed)) {
        throw SearchAbortedException();
    }
    
    if ((worker.nodos_busqueda & 4095) == 0) {
        if (search_uses_ponder) {
            process_ponderhit_request();

            std::lock_guard<std::mutex> lock(search_time_mutex);
            if (!search_is_infinite && !search_is_pondering
                && search_max_time_ms > 0) {
                auto now = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - search_start_time
                ).count();
                if (elapsed >= search_max_time_ms)
                    throw SearchAbortedException();
            }
        } else if (!search_is_infinite && search_max_time_ms > 0) {
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - search_start_time).count();
            if (elapsed >= search_max_time_ms)
                throw SearchAbortedException();
        }
    }
}

inline void wait_for_ponderhit() {
    while (!stop_search.load(std::memory_order_relaxed)) {
        process_ponderhit_request();
        if (!is_search_pondering())
            return;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// ============================================================================
// 1. TABLA DE TRANSPOSICIÓN (TT) Y TABLAS HISTÓRICAS
// ============================================================================
enum TTFlag : uint8_t {
    TT_EXACT = 0,
    TT_ALPHA = 1,
    TT_BETA = 2
};

constexpr int MATE_SCORE = 29000;
constexpr int DRAW_PENALTY = 20000;
constexpr int WINNING_EVAL = 150;

inline int draw_score(const chess::Board& board, int optimism) {
    const int eval = evaluate(board, optimism);

    constexpr int DRAW_THRESHOLD = 150;
    constexpr int MAX_DRAW_CONTEMPT = 160;

    if (eval > DRAW_THRESHOLD) {
        const int margin = eval - DRAW_THRESHOLD;
        const int penalty = std::clamp(32 + margin / 3,
                                       32,
                                       MAX_DRAW_CONTEMPT);
        return -penalty;
    }

    if (eval < -DRAW_THRESHOLD) {
        const int margin = -eval - DRAW_THRESHOLD;
        const int bonus = std::clamp(32 + margin / 3,
                                     32,
                                     MAX_DRAW_CONTEMPT);
        return bonus;
    }

    return 0;
}

struct TTEntry {
    uint64_t key = 0;
    int depth = -1;
    int score = 0;
    int static_eval = NO_EVAL; // INICIALIZAR CON NO_EVAL
    TTFlag flag = TT_ALPHA;
    chess::Move best_move = chess::Move::NO_MOVE;
};

inline size_t TT_SIZE = 1048576;
inline size_t TT_MASK = TT_SIZE - 1;
inline std::vector<TTEntry> TT(TT_SIZE);

inline void resize_tt(size_t size_mb) {
    size_t target_bytes = size_mb * 1024 * 1024;
    size_t num_entries = target_bytes / sizeof(TTEntry);
    size_t new_size = 1;
    while ((new_size << 1) <= num_entries)
        new_size <<= 1;

    std::vector<TTEntry> new_tt(new_size);
    TT.swap(new_tt);
    TT_SIZE = new_size;
    TT_MASK = new_size - 1;
}

inline void clear_tt() { std::fill(TT.begin(), TT.end(), TTEntry{}); }

inline int score_to_tt(int score, int ply) {
    if (score >= MATE_SCORE - 1000)  return score + ply;
    if (score <= -MATE_SCORE + 1000) return score - ply;
    return score;
}

inline int score_from_tt(int score, int ply) {
    if (score >= MATE_SCORE - 1000)  return score - ply;
    if (score <= -MATE_SCORE + 1000) return score + ply;
    return score;
}

constexpr int NormalizeToPawnValue = Stockfish::PawnValue;

inline std::string format_score_uci(int score) {
    if (score >= MATE_SCORE - 1000) {
        int mate_in = (MATE_SCORE - score + 1) / 2;
        return "mate " + std::to_string(mate_in);
    }
    if (score <= -MATE_SCORE + 1000) {
        int mate_in = (-MATE_SCORE - score) / 2; 
        return "mate " + std::to_string(mate_in);
    }
    return "cp " + std::to_string(score * 100 / NormalizeToPawnValue);
}

inline void store_tt(uint64_t key, int depth, int score, int static_eval, TTFlag flag, chess::Move best_move, int ply) {
    size_t index = key & TT_MASK;
    // CORRECCIÓN: Nunca sobrescribir profundidades mayores por un flag EXACT
    if (TT[index].key != key || depth >= TT[index].depth) {
        TT[index] = { key, depth, score_to_tt(score, ply), static_eval, flag, best_move };
    }
}

inline bool probe_tt(uint64_t key, int depth, int alpha, int beta, int& score, int& cached_eval, chess::Move& tt_move, int ply) {
    size_t index = key & TT_MASK;
    const TTEntry& entry = TT[index];
    if (entry.key == key) {
        tt_move = entry.best_move;
        cached_eval = entry.static_eval;
        if (entry.depth >= depth) {
            int tt_eval = score_from_tt(entry.score, ply);
            if (entry.flag == TT_EXACT) { score = tt_eval; return true; }
            // CORRECCIÓN: Devolver el tt_eval real, no truncarlo al bound
            if (entry.flag == TT_ALPHA && tt_eval <= alpha) { score = tt_eval; return true; }
            if (entry.flag == TT_BETA  && tt_eval >= beta)  { score = tt_eval; return true; }
        }
    }
    return false;
}

inline void clear_search_history(ThreadWorker& worker) {
    std::fill(&worker.killer_moves[0][0], &worker.killer_moves[0][0] + (MAX_PLY * 2), chess::Move::NO_MOVE);
    for (int c = 0; c < 2; ++c) {
        for (int f = 0; f < 64; ++f) {
            for (int t = 0; t < 64; ++t) {
                worker.history_table[c][f][t] = 0;
            }
        }
        for (int piece = 0; piece < 6; ++piece)
            for (int to = 0; to < 64; ++to)
                for (int victim = 0; victim < 6; ++victim)
                    worker.capture_history[c][piece][to][victim] = 0;
    }
    for (auto& ply_history : worker.low_ply_history)
        for (auto& color_history : ply_history)
            for (auto& from_history : color_history)
                std::fill(std::begin(from_history), std::end(from_history), 0);
    for (auto& color_history : worker.cont_history)
        for (auto& previous_piece : color_history)
            for (auto& previous_square : previous_piece)
                for (auto& piece_history : previous_square)
                    std::fill(std::begin(piece_history),
                              std::end(piece_history),
                              0);

    std::fill(std::begin(worker.pv_length),
              std::end(worker.pv_length),
              0);
}

// ============================================================================
// 2. QUIESCENCE SEARCH (Con Delta Pruning O(1))
// ============================================================================
inline int quiescence(ThreadWorker& worker,
                      chess::Board& board,
                      int alpha,
                      int beta,
                      int ply = 0) {
    worker.nodos_busqueda++;
    check_time_and_longjump(worker);

    if (ply > worker.sel_depth)
        worker.sel_depth = ply;
    if (ply >= MAX_PLY)
        return board.inCheck() ? 0 : evaluate(board, worker.optimism);

    const int alpha_orig = alpha;
    const uint64_t key = board.hash();

    chess::Move tt_move = chess::Move::NO_MOVE;
    int tt_score = 0;
    int cached_eval = NO_EVAL;

    if (probe_tt(key, 0, alpha, beta, tt_score, cached_eval, tt_move, ply))
        return tt_score;

    const bool in_check = board.inCheck();

    // ============================================================
    // STAND PAT SOLO ES VÁLIDO CUANDO NO ESTAMOS EN JAQUE
    // ============================================================
    int stand_pat = -MATE_SCORE;
    int best_value = -MATE_SCORE;
    int futility_base = -MATE_SCORE;

    if (!in_check) {
        stand_pat = (cached_eval != NO_EVAL)
                  ? cached_eval
                  : evaluate(board, worker.optimism);
        best_value = stand_pat;
        futility_base = stand_pat + 306;

        if (stand_pat >= beta) {
            const int cutoff_score = (441 * stand_pat + 583 * beta) / 1024;
            store_tt(key, 0, cutoff_score, stand_pat,
                     TT_BETA, chess::Move::NO_MOVE, ply);
            return cutoff_score;
        }

        if (stand_pat > alpha)
            alpha = stand_pat;
    }

    chess::Movelist moves;

    if (in_check) {
        chess::movegen::legalmoves(moves, board);

        if (moves.empty())
            return -MATE_SCORE + ply;
    } else {
        chess::movegen::legalmoves<
            chess::movegen::MoveGenType::CAPTURE>(moves, board);
        chess::Movelist quiet_moves;
        chess::movegen::legalmoves<
            chess::movegen::MoveGenType::QUIET>(quiet_moves, board);
        for (int i = 0; i < quiet_moves.size(); ++i) {
            if (quiet_moves[i].typeOf() == chess::Move::PROMOTION)
                moves.add(quiet_moves[i]);
        }

        if (moves.empty() &&
            !chess::movegen::anylegalmoves(board))
            return 0;
    }

    const int moving_color = static_cast<int>(board.sideToMove());
    const chess::Move* killers =
        ply < MAX_PLY ? worker.killer_moves[ply] : nullptr;
    MovePicker move_picker(
        board,
        moves.begin(),
        static_cast<std::size_t>(moves.size()),
        tt_move,
        killers,
        worker.history_table[moving_color],
        worker.capture_history[moving_color],
        worker.cont_history[moving_color],
        worker.low_ply_history[std::min(ply, 3)][moving_color],
        worker.moved_piece_stack,
        worker.moved_to_stack,
        ply);

    const chess::Square previous_to =
        ply > 0 ? worker.moved_to_stack[ply - 1] : chess::Square::NO_SQ;
    int move_count = 0;

    for (chess::Move move;
         (move = move_picker.next_move()) != chess::Move::NO_MOVE;) {
        ++move_count;
        const bool is_capture = board.isCapture(move);
        const bool is_promotion =
            move.typeOf() == chess::Move::PROMOTION;
        const bool gives_check =
            board.givesCheck(move) != chess::CheckType::NO_CHECK;

        if (!in_check) {
            int victim_val = 0;

            if (move.typeOf() == chess::Move::ENPASSANT) {
                victim_val =
                    PIECE_VALUES[
                        static_cast<int>(
                            chess::PieceType::PAWN
                        )
                    ];
            } else {
                const auto victim = board.at(move.to());

                if (victim != chess::Piece::NONE) {
                    victim_val =
                        PIECE_VALUES[
                            static_cast<int>(victim.type())
                        ];
                }
            }

            if (move.typeOf() == chess::Move::PROMOTION) {
                victim_val +=
                    PIECE_VALUES[
                        static_cast<int>(move.promotionType())
                    ]
                    -
                    PIECE_VALUES[
                        static_cast<int>(chess::PieceType::PAWN)
                    ];
            }

            if (best_value > -MATE_SCORE + MAX_PLY) {
                if (!gives_check &&
                    move.to() != previous_to &&
                    !is_promotion) {
                    if (move_count > 2)
                        continue;

                    const int futility_value = futility_base + victim_val;
                    if (futility_value <= alpha) {
                        best_value = std::max(best_value, futility_value);
                        continue;
                    }

                    if (!see_ge(board, move, alpha - futility_base)) {
                        best_value = std::max(
                            best_value,
                            std::min(alpha, futility_base));
                        continue;
                    }
                }

                if (!is_capture && !is_promotion)
                    continue;

                // SEE pruning
                if (is_capture) {
                    const bool see_ok =
                        move_picker.last_see_valid()
                            ? move_picker.last_see() >= -74
                            : see_ge(board, move, -74);
                    if (!see_ok)
                        continue;
                }
            }
        }

        board.makeMove(move);

        const bool rubi_pushed = rubi_push(move);
        worker.pos_history.push_back(board.hash());

        const int score =
            -quiescence(
                worker,
                board,
                -beta,
                -alpha,
                ply + 1
            );

        if (rubi_pushed)
            rubi_pop();

        board.unmakeMove(move);

        if (!rubi_pushed)
            rubi_sync(board);

        worker.pos_history.pop_back();

        if (score >= beta) {
            const int cutoff_score = (441 * score + 583 * beta) / 1024;
            store_tt(
                key,
                0,
                cutoff_score,
                stand_pat,
                TT_BETA,
                move,
                ply
            );
            return cutoff_score;
        }

        if (score > best_value)
            best_value = score;
        if (score > alpha)
            alpha = score;
    }

    const TTFlag flag =
        (alpha > alpha_orig)
            ? TT_EXACT
            : TT_ALPHA;

    store_tt(
        key,
        0,
        best_value,
        stand_pat,
        flag,
        chess::Move::NO_MOVE,
        ply
    );

    return alpha;
}

inline int LMR_TABLE[64][256];

inline int lmr_divisor(int depth) {
    const int d = std::min(depth, 16);
    return 3000 + 7 * (d - 8) * (d - 8);
}

// CORRECCIÓN: Inicialización estática automática (Garantiza que LMR funcione)
struct LMRInit {
    LMRInit() {
        for (int d = 0; d < 64; ++d) {
            for (int m = 0; m < 256; ++m) {
                if (d > 0 && m > 0) {
                    LMR_TABLE[d][m] = 1 + static_cast<int>(std::log(d) * std::log(m) / 1.75);
                } else {
                    LMR_TABLE[d][m] = 0;
                }
            }
        }
    }
};
inline LMRInit _lmr_init_instance;

// ============================================================================
// 4. BÚSQUEDA NEGAMAX PRINCIPAL (Con Lazy SEE)
// ============================================================================
inline int negamax(ThreadWorker& worker,
                   chess::Board& board,
                   int depth,
                   int alpha,
                   int beta,
                   int ply,
                   chess::Move excluded_move = chess::Move::NO_MOVE,
                   bool allow_null_move = true) {
    worker.nodos_busqueda++;
    check_time_and_longjump(worker);
    if (ply > worker.sel_depth)
        worker.sel_depth = ply;
    if (ply >= MAX_PLY)
        return board.inCheck() ? 0 : evaluate(board, worker.optimism);

    if (ply < MAX_PLY)
        worker.pv_length[ply] = 0;

    if (ply > 0) {
        if (is_repetition(worker, board) || board.isHalfMoveDraw()) {
            return draw_score(board, worker.optimism);
        }
    }

    int mate_val = MATE_SCORE - ply;
    if (alpha < -mate_val) alpha = -mate_val;
    if (beta > mate_val - 1) beta = mate_val - 1;
    if (alpha >= beta) return alpha;

    bool is_pv_node = (beta - alpha > 1);
    bool in_check = board.inCheck();
    int alpha_orig = alpha;
    uint64_t key = board.hash();
    chess::Move tt_move = chess::Move::NO_MOVE;

    int tt_score = 0;
    int cached_eval = NO_EVAL;
    if (probe_tt(key, depth, alpha, beta, tt_score, cached_eval, tt_move, ply) &&
        excluded_move == chess::Move::NO_MOVE) {
        if (!is_pv_node)
            return tt_score;
    }

    if (depth <= 0) {
        return quiescence(worker, board, alpha, beta, ply);
    }

    int extension = (in_check && ply < MAX_PLY - 1) ? 1 : 0;
    
    int eval = in_check
        ? (ply >= 2 ? worker.static_eval_stack[ply - 2] : 0)
        : cached_eval != NO_EVAL
            ? cached_eval
            : evaluate(board, worker.optimism);
    // ========================================================================
    // RAZORING
    // ========================================================================

    if (excluded_move == chess::Move::NO_MOVE &&
    !is_pv_node &&
    !in_check &&
    eval < alpha - 342 * depth) {

        return quiescence(
            worker,
            board,
            alpha,
            beta,
            ply
        );
    }

    const bool conversion_mode =
        !in_check &&
        eval >= WINNING_EVAL + 100;
    if (ply < MAX_PLY) worker.static_eval_stack[ply] = eval;

    if (excluded_move == chess::Move::NO_MOVE &&
        depth >= 4 &&
        tt_move == chess::Move::NO_MOVE &&
        (is_pv_node || eval + 256 >= beta)) {
        int iid_depth = depth - 2;
        negamax(worker, board, iid_depth, alpha, beta, ply);
        probe_tt(key, 0, alpha, beta, tt_score, cached_eval, tt_move, ply);
    }

    bool improving = !in_check && ply >= 2 && eval >= worker.static_eval_stack[ply - 2];

    if (excluded_move == chess::Move::NO_MOVE && !is_pv_node && !in_check) {
        // ====================================================================
        // FUTILITY PRUNING
        // ====================================================================

        if (depth < 19 &&
            eval >= beta &&
            std::abs(beta) < MATE_SCORE &&
            (tt_move == chess::Move::NO_MOVE || board.isCapture(tt_move))) {

            const bool opponent_worsening =
                (ply > 0)
                    ? eval > -worker.static_eval_stack[ply - 1]
                    : false;

            int futility_mult =
                std::min(
                    45 + depth * 4,
                    85
                );

            // Aproximación de !ttHit usando ausencia de TT move.
            if (tt_move == chess::Move::NO_MOVE)
                futility_mult -= 20;

            const int futility_margin =
                futility_mult * depth
                - (
                    2789 * static_cast<int>(improving)
                    + 335 * static_cast<int>(opponent_worsening)
                  ) * futility_mult / 1024;

            if (eval - futility_margin >= beta) {

                return
                    (661 * beta + 363 * eval) / 1024;
            }
        }

        const bool zugzwang_sensitive =
            board.occ().count() <= 8;

        if (allow_null_move &&
            depth >= 3 &&
            !is_pv_node &&
            beta >= -2000 &&
            eval >= beta
                - 13 * depth
                - 47 * static_cast<int>(improving)
                + 365 &&
            board.hasNonPawnMaterial(board.sideToMove()) &&
            !zugzwang_sensitive) {

            board.makeNullMove();
            if (ply < MAX_PLY) {
                worker.moved_piece_stack[ply] = chess::PieceType::NONE;
                worker.moved_to_stack[ply] = chess::Square::NO_SQ;
            }

            rubi_sync(board);

            worker.pos_history.push_back(
                board.hash()
            );

            const int eval_excess =
                std::max(
                    0,
                (eval - beta) / 256
                );

            int R =
                7
                + depth / 3
                + eval_excess;

            const int null_score =
                -negamax(
                    worker,
                    board,
                    depth - R,
                    -beta,
                    -beta + 1,
                    ply + 1
                );

            board.unmakeNullMove();

            rubi_sync(board);

            worker.pos_history.pop_back();

            if (null_score >= beta) {
                if (null_score >= MATE_SCORE - MAX_PLY)
                    return beta;

                bool verified_cutoff = true;
                if (depth >= 16) {
                    const int verification_score =
                        negamax(
                            worker,
                            board,
                            depth - R,
                            beta - 1,
                            beta,
                            ply,
                            chess::Move::NO_MOVE,
                            false);
                    verified_cutoff = verification_score >= beta;
                }
                if (verified_cutoff)
                    return null_score;
            }
        }

        // ====================================================================
        // PROBCUT
        // ====================================================================

        if (depth >= 3 &&
            std::abs(beta) < MATE_SCORE) {

            const int probcut_beta =
                beta
                + 241
                - 64 * static_cast<int>(improving);

            const int probcut_depth =
                depth -
                (improving ? 5 : 3);

            chess::Movelist prob_moves;
            chess::movegen::legalmoves<
                chess::movegen::MoveGenType::CAPTURE
            >(prob_moves, board);
            chess::Movelist quiet_promotions;
            chess::movegen::legalmoves<
                chess::movegen::MoveGenType::QUIET
            >(quiet_promotions, board);
            for (int i = 0; i < quiet_promotions.size(); ++i) {
                if (quiet_promotions[i].typeOf() == chess::Move::PROMOTION &&
                    quiet_promotions[i].promotionType() ==
                        chess::PieceType::QUEEN) {
                    prob_moves.add(quiet_promotions[i]);
                }
            }
            MovePicker probcut_picker(
                board,
                prob_moves.begin(),
                static_cast<std::size_t>(prob_moves.size()),
                tt_move,
                nullptr,
                nullptr,
                worker.capture_history[
                    static_cast<int>(board.sideToMove())]);

            for (chess::Move move;
                 (move = probcut_picker.next_move()) != chess::Move::NO_MOVE;) {
                const bool see_ok = probcut_picker.last_see_valid()
                    ? probcut_picker.last_see() >= probcut_beta - eval
                    : see_ge(board, move, probcut_beta - eval);
                if (!see_ok)
                    continue;

                if (ply < MAX_PLY) {
                    worker.moved_piece_stack[ply] =
                        board.at(move.from()).type();
                    worker.moved_to_stack[ply] = move.to();
                }
                board.makeMove(move);

                const bool rubi_pushed = rubi_push(move);
                worker.pos_history.push_back(board.hash());

                int probcut_score =
                    -quiescence(
                        worker,
                        board,
                        -probcut_beta,
                        -probcut_beta + 1,
                        ply + 1);

                if (probcut_score >= probcut_beta && probcut_depth > 0) {
                    probcut_score =
                        -negamax(
                            worker,
                            board,
                            probcut_depth,
                            -probcut_beta,
                            -probcut_beta + 1,
                            ply + 1);
                }

                if (rubi_pushed)
                    rubi_pop();

                board.unmakeMove(move);

                if (!rubi_pushed)
                    rubi_sync(board);

                worker.pos_history.pop_back();

                if (probcut_score >= probcut_beta) {
                    store_tt(
                        key,
                        std::max(0, probcut_depth + 1),
                        probcut_score,
                        eval,
                        TT_BETA,
                        move,
                        ply);
                    return probcut_score - (probcut_beta - beta);
                }
            }
        }
    }

    chess::Movelist moves;
    chess::movegen::legalmoves(moves, board);

    int loud_moves = 0;
    for (int i = 0; i < moves.size(); ++i) {
        if (board.isCapture(moves[i]) || moves[i].typeOf() == chess::Move::PROMOTION)
            loud_moves++;
    }
    // Heurística: si ≥25% de las legales son capturas/promociones, o hay jaque, es "ruidosa"
    bool tactical_position = in_check || (loud_moves * 4 >= (int)moves.size());

    if (moves.empty()) {
        if (in_check)
            return -MATE_SCORE + ply;

        return 0;
    }

    int best_score = -std::numeric_limits<int>::max();
    chess::Move best_move = chess::Move::NO_MOVE;
    int moves_searched = 0; 
    int move_count = 0;
    std::array<chess::Move, 256> searched_quiets;
    int num_searched_quiets = 0;
    std::array<chess::Move, 256> searched_captures;
    int num_searched_captures = 0;

    const int moving_color = static_cast<int>(board.sideToMove());
    const chess::Move* killers =
        ply < MAX_PLY ? worker.killer_moves[ply] : nullptr;
    MovePicker move_picker(
        board,
        moves.begin(),
        static_cast<std::size_t>(moves.size()),
        tt_move,
        killers,
        worker.history_table[moving_color],
        worker.capture_history[moving_color],
        worker.cont_history[moving_color],
        worker.low_ply_history[std::min(ply, 3)][moving_color],
        worker.moved_piece_stack,
        worker.moved_to_stack,
        ply);

    for (chess::Move move;
         (move = move_picker.next_move()) != chess::Move::NO_MOVE;) {
        if (move == excluded_move)
            continue;
        ++move_count;
        chess::Color moving_side = board.sideToMove();
        bool is_capture = board.isCapture(move);
        bool is_promotion = (move.typeOf() == chess::Move::PROMOTION);
        bool is_quiet = !is_capture && !is_promotion;
        const chess::PieceType moving_piece = board.at(move.from()).type();

        bool is_bad_capture = false;
        bool see_computed = false;

        auto evaluate_see = [&]() {
            if (!see_computed) {
                if (is_capture) {
                    is_bad_capture =
                        move_picker.last_see_valid()
                            ? move_picker.last_see() < 0
                            : !see_ge(board, move, 0);
                }
                see_computed = true;
            }
        };

        int history = 0;
        int lmr_depth = depth - 1 -
            LMR_TABLE[std::min(depth, 63)][std::min(move_count, 255)];

        if (!is_pv_node &&
            !in_check &&
            ply > 0 &&
            board.hasNonPawnMaterial(moving_side) &&
            best_score > -MATE_SCORE + MAX_PLY) {
            if (is_capture || board.givesCheck(move) != chess::CheckType::NO_CHECK) {
                int capture_stat = 0;
                chess::PieceType captured_type = chess::PieceType::NONE;
                if (is_capture) {
                    const chess::Piece captured = board.at(move.to());
                    captured_type =
                        move.typeOf() == chess::Move::ENPASSANT
                            ? chess::PieceType::PAWN
                            : captured.type();
                    capture_stat = worker.capture_history
                        [static_cast<int>(moving_side)]
                        [static_cast<int>(moving_piece)]
                        [move.to().index()]
                        [static_cast<int>(captured_type)];
                }

                const chess::Piece captured = board.at(move.to());
                const int captured_value =
                    move.typeOf() == chess::Move::ENPASSANT
                        ? PIECE_VALUES[static_cast<int>(chess::PieceType::PAWN)]
                        : captured == chess::Piece::NONE
                              ? 0
                              : PIECE_VALUES[static_cast<int>(captured.type())];

                const bool gives_check =
                    board.givesCheck(move) != chess::CheckType::NO_CHECK;
                if (!gives_check &&
                    lmr_depth < 8 &&
                    !is_promotion &&
                    eval + 234 + 247 * lmr_depth + captured_value
                            + 134 * capture_stat / 1024 <= alpha) {
                    continue;
                }

                const chess::Bitboard own_non_pawns =
                    board.pieces(chess::PieceType::KNIGHT, moving_side) |
                    board.pieces(chess::PieceType::BISHOP, moving_side) |
                    board.pieces(chess::PieceType::ROOK, moving_side) |
                    board.pieces(chess::PieceType::QUEEN, moving_side);
                const bool last_non_pawn =
                    moving_piece != chess::PieceType::PAWN &&
                    own_non_pawns.count() == 1 &&
                    (own_non_pawns & chess::Bitboard::fromSquare(move.from()));
                const int see_margin =
                    177 * depth + capture_stat * 34 / 1024;
                if ((alpha >= 0 || !last_non_pawn) &&
                    !see_ge(board, move, -see_margin)) {
                    continue;
                }
            } else if (!is_promotion) {
                int continuation = 0;
                const int current_piece = static_cast<int>(moving_piece);
                const int current_to = move.to().index();

                for (int previous_offset = 1;
                     previous_offset <= 2;
                     ++previous_offset) {
                    const int previous_ply = ply - previous_offset;
                    if (previous_ply < 0)
                        continue;
                    const int previous_piece = static_cast<int>(
                        worker.moved_piece_stack[previous_ply]);
                    if (previous_piece >= 6)
                        continue;
                    continuation += worker.cont_history
                        [static_cast<int>(moving_side)]
                        [previous_piece]
                        [worker.moved_to_stack[previous_ply].index()]
                        [current_piece]
                        [current_to];
                }

                if (continuation < -4136 * depth)
                    continue;

                const int main_history = worker.history_table
                    [static_cast<int>(moving_side)]
                    [move.from().index()]
                    [move.to().index()];
                history = continuation + 69 * main_history / 32;
                lmr_depth += history / lmr_divisor(depth);

                const int lmr_threshold =
                    (3 + depth * depth) /
                    (2 - static_cast<int>(improving));
                if (move_count >= lmr_threshold && !conversion_mode)
                    continue;

                const int futility_value =
                    eval + 119 * lmr_depth
                         + 90 * static_cast<int>(eval > alpha)
                         + 164;
                if (lmr_depth < 12 &&
                    moves_searched > 0 &&
                    futility_value <= alpha &&
                    !conversion_mode) {
                    continue;
                }

                lmr_depth = std::max(lmr_depth, 0);
                if (!see_ge(board, move, -23 * lmr_depth * lmr_depth))
                    continue;
            }
        }

        if (is_quiet) {
            if (num_searched_quiets < static_cast<int>(searched_quiets.size())) {
                searched_quiets[num_searched_quiets++] = move;
            }
        }
        if (is_capture && num_searched_captures < 256)
            searched_captures[num_searched_captures++] = move;

        int move_extension = extension;
        if (excluded_move == chess::Move::NO_MOVE &&
            !in_check &&
            ply > 0 &&
            depth >= 8 &&
            move == tt_move) {
            const TTEntry& entry = TT[key & TT_MASK];
            if (entry.key == key &&
                entry.depth >= depth - 3 &&
                (entry.flag == TT_BETA || entry.flag == TT_EXACT)) {
                const int tt_value = score_from_tt(entry.score, ply);
                if (tt_value >= beta &&
                    std::abs(tt_value) < MATE_SCORE - MAX_PLY) {
                    const int singular_beta = tt_value - 2 * depth;
                    const int singular_depth = std::max(1, (depth - 1) / 2);
                    const int alternatives_score =
                        negamax(
                            worker,
                            board,
                            singular_depth,
                            singular_beta - 1,
                            singular_beta,
                            ply,
                            move,
                            false);
                    if (alternatives_score < singular_beta) {
                        ++move_extension;
                    } else if (alternatives_score >= beta &&
                               alternatives_score < MATE_SCORE - MAX_PLY) {
                        return alternatives_score;
                    }
                }
            }
        }

        if (ply < MAX_PLY) {
            worker.moved_piece_stack[ply] = moving_piece;
            worker.moved_to_stack[ply] = move.to();
        }
        board.makeMove(move);
        const bool rubi_pushed = rubi_push(move);
        worker.pos_history.push_back(board.hash());
        bool gives_check = board.inCheck();
        moves_searched++;
        
        int score;
        int new_depth = depth - 1 + move_extension;

        if (moves_searched == 1) {
            score = -negamax(worker, board, new_depth, -beta, -alpha, ply + 1);
        } else {
            bool can_lmr = (depth >= 3 && moves_searched > 1 && !in_check && !gives_check && !is_promotion);
            bool needs_full_search = true;

            if (can_lmr) {
                if (is_capture) evaluate_see();
                
                if (is_quiet || (is_capture && is_bad_capture)) {
                    int d = std::min(depth, 63);
                    int m = std::min(moves_searched, 255);
                    int R = LMR_TABLE[d][m];
                    if (tactical_position) R = std::max(1, R - 1);
                    
                    if (!is_pv_node) R++;
                    if (!improving) R++;
                    if (!is_pv_node && depth >= 5) R++;
                    if (is_quiet) {
                        int hist = worker.history_table[static_cast<int>(moving_side)][move.from().index()][move.to().index()];
                        if (hist < 0) R++;
                        else if (hist > 10000) R--;
                        if (hist < -2000 && !is_pv_node) R++;
                    }

                    R = std::clamp(R, 1, new_depth - 1);
                    score = -negamax(worker, board, new_depth - R, -alpha - 1, -alpha, ply + 1);
                    needs_full_search = (score > alpha);
                }
            }

            if (needs_full_search) {
                // Zero Window Search a profundidad completa si falló LMR o si no aplicó
                score = -negamax(worker, board, new_depth, -alpha - 1, -alpha, ply + 1);

                // Si la puntuación entra en la ventana PV, hacemos búsqueda completa
                if (score > alpha && score < beta) {
                    score = -negamax(worker, board, new_depth, -beta, -alpha, ply + 1);
                }
            }
        }

        board.unmakeMove(move);
        if (rubi_pushed) rubi_pop();
        else rubi_sync(board);
        worker.pos_history.pop_back();

        if (score > best_score) {
            best_score = score;
            best_move = move;

            if (ply < MAX_PLY) {
                worker.pv_table[ply][0] = move;

                int child_len = 0;

                if (ply + 1 < MAX_PLY)
                    child_len = worker.pv_length[ply + 1];

                const int copy_len =
                    std::min(child_len, MAX_PLY - ply - 1);

                for (int k = 0; k < copy_len; ++k) {
                    worker.pv_table[ply][k + 1] =
                        worker.pv_table[ply + 1][k];
                }

                worker.pv_length[ply] = copy_len + 1;
            }
        }
        if (score > alpha) alpha = score;

        if (alpha >= beta) {
            const int bonus = std::clamp(16 * depth * depth, 0, 1200);
            const int color = static_cast<int>(board.sideToMove());
            const int previous_piece = ply > 0
                ? static_cast<int>(worker.moved_piece_stack[ply - 1])
                : 6;
            const int previous_to = previous_piece < 6
                ? worker.moved_to_stack[ply - 1].index()
                : -1;

            if (is_quiet && ply < MAX_PLY) {
                if (move != worker.killer_moves[ply][0]) {
                    worker.killer_moves[ply][1] = worker.killer_moves[ply][0];
                    worker.killer_moves[ply][0] = move;
                }

                int& hist_ref = worker.history_table[color][move.from().index()][move.to().index()];
                hist_ref += bonus - (hist_ref * std::abs(bonus) / 16384);

                if (ply < 4) {
                    int& low_ply = worker.low_ply_history[ply][color]
                        [move.from().index()][move.to().index()];
                    low_ply += bonus - low_ply * std::abs(bonus) / 16384;
                }

                if (previous_piece < 6 && previous_to >= 0 && previous_to < 64) {
                    int& continuation = worker.cont_history[color]
                        [previous_piece][previous_to]
                        [static_cast<int>(moving_piece)][move.to().index()];
                    continuation +=
                        bonus - continuation * std::abs(bonus) / 16384;
                }

            }

            for (int q = 0; q < num_searched_quiets; ++q) {
                const chess::Move& failed_quiet = searched_quiets[q];
                if (is_quiet && failed_quiet == move)
                    continue;

                int& failed_history = worker.history_table[color]
                    [failed_quiet.from().index()][failed_quiet.to().index()];
                failed_history -=
                    bonus + failed_history * std::abs(bonus) / 16384;

                if (ply < 4) {
                    int& failed_low_ply = worker.low_ply_history[ply][color]
                        [failed_quiet.from().index()][failed_quiet.to().index()];
                    failed_low_ply -=
                        bonus + failed_low_ply * std::abs(bonus) / 16384;
                }

                if (previous_piece < 6 && previous_to >= 0 && previous_to < 64) {
                    int& failed_continuation = worker.cont_history[color]
                        [previous_piece][previous_to]
                        [static_cast<int>(board.at(failed_quiet.from()).type())]
                        [failed_quiet.to().index()];
                    failed_continuation -=
                        bonus + failed_continuation * std::abs(bonus) / 16384;
                }
            }

            if (is_capture) {
                const chess::Piece victim = board.at(move.to());
                const chess::PieceType victim_type =
                    move.typeOf() == chess::Move::ENPASSANT
                        ? chess::PieceType::PAWN
                        : victim.type();
                int& best_capture_history = worker.capture_history[color]
                    [static_cast<int>(moving_piece)][move.to().index()]
                    [static_cast<int>(victim_type)];
                best_capture_history +=
                    bonus - best_capture_history * std::abs(bonus) / 16384;
            }

            for (int q = 0; q < num_searched_captures; ++q) {
                const chess::Move& failed_capture = searched_captures[q];
                if (is_capture && failed_capture == move)
                    continue;
                const chess::Piece failed_victim =
                    board.at(failed_capture.to());
                const chess::PieceType failed_victim_type =
                    failed_capture.typeOf() == chess::Move::ENPASSANT
                        ? chess::PieceType::PAWN
                        : failed_victim.type();
                int& failed_history = worker.capture_history[color]
                    [static_cast<int>(board.at(failed_capture.from()).type())]
                    [failed_capture.to().index()]
                    [static_cast<int>(failed_victim_type)];
                failed_history -=
                    bonus + failed_history * std::abs(bonus) / 16384;
            }
            break;
        }
    }

    if (best_score == -std::numeric_limits<int>::max()) {
        return alpha_orig;
    }

    TTFlag flag = TT_EXACT;
    if (best_score <= alpha_orig) flag = TT_ALPHA;
    else if (best_score >= beta) flag = TT_BETA;

    if (excluded_move == chess::Move::NO_MOVE)
        store_tt(key, depth, best_score, eval, flag, best_move, ply);

    return best_score;
}