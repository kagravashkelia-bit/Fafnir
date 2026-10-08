#include "SearchController.hpp"
#include "search.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include <thread>

#ifdef __EMSCRIPTEN__
    // WASM actual sin -pthread.
    // No se ejecutan std::thread durante la búsqueda.
#endif

// ============================================================================
// EVITAR CONFLICTOS CON sync_cout
// ============================================================================

#ifdef sync_cout
#undef sync_cout
#endif

// ============================================================================
// VARIABLES GLOBALES DE CONTROL
// ============================================================================

std::atomic<bool> stop_search(false);

chess::Move last_best_move = chess::Move::NO_MOVE;
chess::Move last_ponder_move = chess::Move::NO_MOVE;

std::mutex cout_mutex;

// ============================================================================
// SALIDA SINCRONIZADA
// ============================================================================

void sync_cout(const std::string& text)
{
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << text << std::endl;
}

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

namespace
{

struct RootVariation
{
    int score;
    chess::Move move;
    std::vector<chess::Move> pv;
};

// ============================================================================
// OBTENER PV EN FORMATO UCI
// ============================================================================

std::string get_pv_string(const ThreadWorker& worker)
{
    std::string pv;

    const int len =
        std::min(worker.pv_length[0], MAX_PLY);

    for (int i = 0; i < len; ++i)
    {
        const chess::Move& move =
            worker.pv_table[0][i];

        if (move == chess::Move::NO_MOVE)
            break;

        if (!pv.empty())
            pv += ' ';

        pv += chess::uci::moveToUci(move);
    }

    return pv;
}

// ============================================================================
// HILO AUXILIAR LAZY SMP
// ============================================================================

#ifndef __EMSCRIPTEN__

void helper_thread_worker(
    ThreadWorker& worker,
    chess::Board board,
    int max_depth)
{
    int previous_score = 0;

    for (int current_depth = 1;
         current_depth <= max_depth;
         ++current_depth)
    {
        if (stop_search.load(std::memory_order_relaxed))
            break;

        chess::Board search_board = board;

        try
        {
            const int score = negamax(
                worker,
                search_board,
                current_depth,
                -100000,
                100000,
                0
            );

            worker.optimism = apply_aggressivity_to_optimism(
                (score - previous_score) / 2
            );
            previous_score = score;
        }
        catch (const SearchAbortedException&)
        {
            break;
        }
        catch (...)
        {
            break;
        }
    }
}

#endif

// ============================================================================
// BÚSQUEDA ITERATIVA
// ============================================================================

chess::Move iterative_deepening(
    chess::Board board,
    int max_depth,
    int max_time_ms,
    bool infinite,
    int num_threads,
    const std::vector<uint64_t>& history,
    int multipv,
    bool ponder)
{
    // ------------------------------------------------------------------------
    // PROFUNDIDAD
    // ------------------------------------------------------------------------

    if (max_depth <= 0)
        max_depth = MAX_PLY;

    max_depth =
        std::min(max_depth, MAX_PLY);

    // ------------------------------------------------------------------------
    // HILOS
    // ------------------------------------------------------------------------

    if (num_threads < 1)
        num_threads = 1;

#ifdef __EMSCRIPTEN__

    // WASM actual sin pthreads.
    num_threads = 1;

#endif

    // ------------------------------------------------------------------------
    // ESTADO INICIAL
    // ------------------------------------------------------------------------

    chess::Move best_move =
        chess::Move::NO_MOVE;

    last_best_move =
        chess::Move::NO_MOVE;
    last_ponder_move =
        chess::Move::NO_MOVE;

    stop_search.store(
        false,
        std::memory_order_relaxed
    );

    // ------------------------------------------------------------------------
    // MOVIMIENTOS LEGALES INICIALES
    // ------------------------------------------------------------------------

    chess::Movelist initial_moves;

    chess::movegen::legalmoves(
        initial_moves,
        board
    );

    if (initial_moves.empty())
        return chess::Move::NO_MOVE;

    // Salvavidas: siempre tenemos una jugada legal.
    best_move = initial_moves[0];
    last_best_move = initial_moves[0];

    // ------------------------------------------------------------------------
    // CONTROL DEL TIEMPO
    // ------------------------------------------------------------------------

    init_search_time(
        max_time_ms,
        infinite,
        ponder
    );

    // ------------------------------------------------------------------------
    // ASPIRATION WINDOW
    // ------------------------------------------------------------------------

    int alpha = -100000;
    int beta  = 100000;
    int delta = 50;
    int score = 0;

    // ------------------------------------------------------------------------
    // WORKERS
    // ------------------------------------------------------------------------

    std::vector<ThreadWorker> workers(
        static_cast<std::size_t>(num_threads)
    );

    for (int i = 0; i < num_threads; ++i)
    {
        workers[i].id = i;

        if (!history.empty())
        {
            workers[i].pos_history = history;
        }
        else
        {
            workers[i].pos_history = {
                board.hash()
            };
        }
    }

#ifndef __EMSCRIPTEN__

    // ------------------------------------------------------------------------
    // LAZY SMP
    // ------------------------------------------------------------------------

    std::vector<std::thread> helper_threads;

    if (num_threads > 1)
    {
        helper_threads.reserve(
            static_cast<std::size_t>(
                num_threads - 1
            )
        );

        for (int i = 1; i < num_threads; ++i)
        {
            helper_threads.emplace_back(
                helper_thread_worker,
                std::ref(workers[i]),
                board,
                max_depth
            );
        }
    }

#endif

    // ------------------------------------------------------------------------
    // ITERATIVE DEEPENING
    // ------------------------------------------------------------------------

    int previous_root_score = 0;

    for (int current_depth = 1;
         current_depth <= max_depth;
         ++current_depth)
    {
        if (stop_search.load(
                std::memory_order_relaxed))
        {
            break;
        }

        // ------------------------------------------------------------
        // ASPIRATION WINDOW
        // ------------------------------------------------------------

        if (current_depth >= 5)
        {
            alpha = std::max(
                -100000,
                score - delta
            );

            beta = std::min(
                100000,
                score + delta
            );
        }
        else
        {
            alpha = -100000;
            beta  = 100000;
        }

        workers[0].sel_depth = 0;

        bool search_failed = false;

        chess::Move current_depth_best =
            chess::Move::NO_MOVE;

        std::vector<RootVariation> root_variations;

        if (multipv > 1)
        {
            root_variations.reserve(
                static_cast<std::size_t>(initial_moves.size())
            );

            for (int move_index = 0;
                 move_index < initial_moves.size();
                 ++move_index)
            {
                if (stop_search.load(std::memory_order_relaxed))
                {
                    search_failed = true;
                    break;
                }

                const chess::Move root_move = initial_moves[move_index];
                chess::Board search_board = board;
                search_board.makeMove(root_move);
                rubi_sync(search_board);
                workers[0].pos_history.push_back(search_board.hash());

                int root_score = 0;
                try
                {
                    root_score = -negamax(
                        workers[0],
                        search_board,
                        current_depth - 1,
                        -100000,
                        100000,
                        1
                    );
                }
                catch (const SearchAbortedException&)
                {
                    search_failed = true;
                }

                workers[0].pos_history.pop_back();
                rubi_sync(board);

                if (search_failed)
                    break;

                RootVariation variation{
                    root_score,
                    root_move,
                    {root_move}
                };

                const int child_pv_length =
                    std::min(workers[0].pv_length[1], MAX_PLY - 1);
                for (int pv_index = 0;
                     pv_index < child_pv_length;
                     ++pv_index)
                {
                    const chess::Move pv_move =
                        workers[0].pv_table[1][pv_index];
                    if (pv_move == chess::Move::NO_MOVE)
                        break;
                    variation.pv.push_back(pv_move);
                }

                root_variations.push_back(std::move(variation));
            }

            if (!search_failed)
            {
                std::stable_sort(
                    root_variations.begin(),
                    root_variations.end(),
                    [](const RootVariation& left,
                       const RootVariation& right) {
                        return left.score > right.score;
                    }
                );

                if (!root_variations.empty())
                {
                    score = root_variations.front().score;
                    current_depth_best = root_variations.front().move;

                    workers[0].pv_length[0] = static_cast<int>(
                        std::min(root_variations.front().pv.size(),
                                 static_cast<std::size_t>(MAX_PLY))
                    );
                    for (int pv_index = 0;
                         pv_index < workers[0].pv_length[0];
                         ++pv_index)
                    {
                        workers[0].pv_table[0][pv_index] =
                            root_variations.front().pv[
                                static_cast<std::size_t>(pv_index)];
                    }
                }
            }
        }
        else
        {
            // Preserve the original single-PV search path and its NPS.
            while (true)
            {
                if (stop_search.load(
                        std::memory_order_relaxed))
                {
                    search_failed = true;
                    break;
                }

                chess::Board search_board = board;

                try
                {
                    score = negamax(
                        workers[0],
                        search_board,
                        current_depth,
                        alpha,
                        beta,
                        0
                    );
                }
                catch (const SearchAbortedException&)
                {
                    search_failed = true;
                    break;
                }

                if (score <= alpha)
                {
                    alpha = std::max(-100000, alpha - delta);
                    delta += delta / 2;
                    continue;
                }

                if (score >= beta)
                {
                    beta = std::min(100000, beta + delta);
                    delta += delta / 2;
                    continue;
                }

                if (workers[0].pv_length[0] > 0)
                    current_depth_best = workers[0].pv_table[0][0];

                if (current_depth_best == chess::Move::NO_MOVE
                    && !initial_moves.empty())
                    current_depth_best = initial_moves[0];

                break;
            }
        }

        // ------------------------------------------------------------
        // SI SE ABORTÓ
        // ------------------------------------------------------------

        if (
            search_failed
            ||
            stop_search.load(
                std::memory_order_relaxed
            )
        )
        {
            break;
        }

        // ------------------------------------------------------------
        // REINICIAR ASPIRATION
        // ------------------------------------------------------------

        delta = 50;

        // ------------------------------------------------------------
        // ACTUALIZAR MEJOR JUGADA
        // ------------------------------------------------------------

        if (
            current_depth_best !=
            chess::Move::NO_MOVE
        )
        {
            best_move =
                current_depth_best;

            last_best_move =
                current_depth_best;
        }

        last_ponder_move =
            workers[0].pv_length[0] > 1
                ? workers[0].pv_table[0][1]
                : chess::Move::NO_MOVE;

        workers[0].optimism = apply_aggressivity_to_optimism(
            (score - previous_root_score) / 2
        );
        previous_root_score = score;

        // ------------------------------------------------------------
        // ESTADÍSTICAS
        // ------------------------------------------------------------

        uint64_t nodos_totales = 0;

        for (const auto& worker : workers)
        {
            nodos_totales +=
                worker.nodos_busqueda;
        }

        const auto now =
            std::chrono::high_resolution_clock::now();

        const auto search_start =
            search_uses_ponder
                ? search_start_time_snapshot()
                : search_start_time;

        const auto elapsed_ms =
            std::chrono::duration_cast<
                std::chrono::milliseconds
            >(
                now - search_start
            ).count();

        const uint64_t nps =
            elapsed_ms > 0
                ? static_cast<uint64_t>(
                      nodos_totales
                  ) * 1000ULL /
                  static_cast<uint64_t>(
                      elapsed_ms
                  )
                : 0ULL;

        // ------------------------------------------------------------
        // PV
        // ------------------------------------------------------------

        if (multipv > 1)
        {
            const std::size_t variation_count = std::min(
                root_variations.size(),
                static_cast<std::size_t>(multipv)
            );
            for (std::size_t i = 0; i < variation_count; ++i)
            {
                std::stringstream info;
                info << "info depth " << current_depth
                     << " seldepth " << workers[0].sel_depth
                     << " multipv " << i + 1
                     << " score " << format_score_uci(root_variations[i].score)
                     << " nodes " << nodos_totales
                     << " nps " << nps
                     << " time " << elapsed_ms
                     << " pv";
                for (const chess::Move& move : root_variations[i].pv)
                    info << ' ' << chess::uci::moveToUci(move);
                sync_cout(info.str());
            }
        }
        else
        {
            const std::string pv_line =
                get_pv_string(workers[0]);

            std::stringstream info;

            info << "info depth "
                 << current_depth

                 << " seldepth "
                 << workers[0].sel_depth

                 << " score "
                 << format_score_uci(score)

                 << " nodes "
                 << nodos_totales

                 << " nps "
                 << nps

                 << " time "
                 << elapsed_ms;

            if (!pv_line.empty())
            {
                info << " pv "
                     << pv_line;
            }
            else if (
                best_move !=
                chess::Move::NO_MOVE
            )
            {
                info << " pv "
                     << chess::uci::moveToUci(
                            best_move
                        );
            }

            sync_cout(info.str());
        }
    }

    // ------------------------------------------------------------------------
    // DETENER HELPERS
    // ------------------------------------------------------------------------

    if (ponder)
        wait_for_ponderhit();

    stop_search.store(
        true,
        std::memory_order_relaxed
    );

#ifndef __EMSCRIPTEN__

    for (auto& thread : helper_threads)
    {
        if (thread.joinable())
            thread.join();
    }

#endif

    // ------------------------------------------------------------------------
    // DEVOLVER SIEMPRE UNA JUGADA LEGAL
    // ------------------------------------------------------------------------

    if (
        best_move ==
        chess::Move::NO_MOVE
    )
    {
        best_move =
            last_best_move;
    }

    return best_move;
}

} // namespace

// ============================================================================
// DESTRUCTOR
// ============================================================================

SearchController::~SearchController()
{
    stop();
}

// ============================================================================
// HISTORIAL
// ============================================================================

void SearchController::set_history(
    const std::vector<uint64_t>& history)
{
    current_history = history;
}

// ============================================================================
// SEARCH SYNC
//
// WASM:
//   JS -> flashbit_go_movetime()
//      -> UciEngine::flashbit_go_movetime()
//      -> SearchController::search_sync()
//      -> iterative_deepening()
//      -> Move
//
// ============================================================================

chess::Move SearchController::search_sync(
    const chess::Board& board,
    int depth,
    int time_ms,
    bool infinite,
    int num_threads,
    int multipv)
{
    // ------------------------------------------------------------------------
    // DETENER BÚSQUEDA ANTERIOR
    // ------------------------------------------------------------------------

    stop();
    ponderhit_requested.store(false, std::memory_order_relaxed);

    // ------------------------------------------------------------------------
    // COPIA ESTABLE DEL HISTORIAL
    // ------------------------------------------------------------------------

    const std::vector<uint64_t> history_snapshot =
        current_history;

    // ------------------------------------------------------------------------
    // NORMALIZAR
    // ------------------------------------------------------------------------

    if (depth <= 0)
        depth = MAX_PLY;

    if (num_threads < 1)
        num_threads = 1;

#ifdef __EMSCRIPTEN__

    num_threads = 1;

#endif

    // ------------------------------------------------------------------------
    // BÚSQUEDA SINCRÓNICA
    // ------------------------------------------------------------------------

    const chess::Move best_move =
        iterative_deepening(
            board,
            depth,
            time_ms,
            infinite,
            num_threads,
            history_snapshot,
            multipv,
            false
        );

    // ------------------------------------------------------------------------
    // DEVOLVER MEJOR RESULTADO
    // ------------------------------------------------------------------------

    if (
        best_move !=
        chess::Move::NO_MOVE
    )
    {
        return best_move;
    }

    return last_best_move;
}

// ============================================================================
// START
// ============================================================================

void SearchController::start(
    const chess::Board& board,
    int depth,
    int time_ms,
    bool infinite,
    int num_threads,
    int multipv,
    bool ponder)
{
    stop();
    ponderhit_requested.store(false, std::memory_order_relaxed);

    stop_search.store(
        false,
        std::memory_order_relaxed
    );

    // ------------------------------------------------------------------------
    // HISTORIAL ESTABLE
    // ------------------------------------------------------------------------

    const std::vector<uint64_t> history_snapshot =
        current_history;

#ifdef __EMSCRIPTEN__

    // ------------------------------------------------------------------------
    // WASM SIN PTHREADS
    // ------------------------------------------------------------------------

    const chess::Move best_move =
        iterative_deepening(
            board,
            depth,
            time_ms,
            infinite,
            1,
            history_snapshot,
            multipv,
            ponder
        );

    const chess::Move final_move =
        best_move != chess::Move::NO_MOVE
            ? best_move
            : last_best_move;

    std::stringstream output;

    output << "bestmove "
           << (
                final_move !=
                chess::Move::NO_MOVE
                    ? chess::uci::moveToUci(
                          final_move
                      )
                    : "0000"
              );
    if (last_ponder_move != chess::Move::NO_MOVE)
        output << " ponder " << chess::uci::moveToUci(last_ponder_move);

    sync_cout(output.str());

#else

    // ------------------------------------------------------------------------
    // NATIVO
    // ------------------------------------------------------------------------

    search_thread =
        std::thread(
            [this,
             board,
             depth,
             time_ms,
             infinite,
             num_threads,
             multipv,
             ponder,
             history_snapshot]() mutable
            {
                const chess::Move best_move =
                    iterative_deepening(
                        board,
                        depth,
                        time_ms,
                        infinite,
                        num_threads,
                        history_snapshot,
                        multipv,
                        ponder
                    );

                const chess::Move final_move =
                    best_move !=
                    chess::Move::NO_MOVE
                        ? best_move
                        : last_best_move;

                std::stringstream output;

                output << "bestmove "
                       << (
                            final_move !=
                            chess::Move::NO_MOVE
                                ? chess::uci::moveToUci(
                                      final_move
                                  )
                                : "0000"
                          );
                if (last_ponder_move != chess::Move::NO_MOVE)
                    output << " ponder "
                           << chess::uci::moveToUci(last_ponder_move);

                sync_cout(
                    output.str()
                );
            }
        );

#endif
}

void SearchController::ponderhit()
{
    ponderhit_requested.store(true, std::memory_order_relaxed);
}

// ============================================================================
// STOP
// ============================================================================

void SearchController::stop()
{
    stop_search.store(
        true,
        std::memory_order_relaxed
    );

#ifndef __EMSCRIPTEN__

    if (search_thread.joinable())
    {
        search_thread.join();
    }

#endif
}