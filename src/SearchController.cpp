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
    for (int current_depth = 1;
         current_depth <= max_depth;
         ++current_depth)
    {
        if (stop_search.load(std::memory_order_relaxed))
            break;

        chess::Board search_board = board;

        try
        {
            negamax(
                worker,
                search_board,
                current_depth,
                -100000,
                100000,
                0
            );
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
    const std::vector<uint64_t>& history)
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
        infinite
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

        // ------------------------------------------------------------
        // RESEARCH
        // ------------------------------------------------------------

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

            // --------------------------------------------------------
            // FAIL LOW
            // --------------------------------------------------------

            if (score <= alpha)
            {
                alpha = std::max(
                    -100000,
                    alpha - delta
                );

                delta += delta / 2;
                continue;
            }

            // --------------------------------------------------------
            // FAIL HIGH
            // --------------------------------------------------------

            if (score >= beta)
            {
                beta = std::min(
                    100000,
                    beta + delta
                );

                delta += delta / 2;
                continue;
            }

            // --------------------------------------------------------
            // BÚSQUEDA COMPLETADA
            // --------------------------------------------------------

            if (workers[0].pv_length[0] > 0)
            {
                current_depth_best =
                    workers[0].pv_table[0][0];
            }

            if (
                current_depth_best ==
                    chess::Move::NO_MOVE
                &&
                !initial_moves.empty()
            )
            {
                current_depth_best =
                    initial_moves[0];
            }

            break;
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

        const auto elapsed_ms =
            std::chrono::duration_cast<
                std::chrono::milliseconds
            >(
                now - search_start_time
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

        const std::string pv_line =
            get_pv_string(workers[0]);

        // ------------------------------------------------------------
        // INFO UCI
        // ------------------------------------------------------------

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

    // ------------------------------------------------------------------------
    // DETENER HELPERS
    // ------------------------------------------------------------------------

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
    int num_threads)
{
    // ------------------------------------------------------------------------
    // DETENER BÚSQUEDA ANTERIOR
    // ------------------------------------------------------------------------

    stop();

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
            history_snapshot
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
    int num_threads)
{
    stop();

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
            history_snapshot
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
             history_snapshot]() mutable
            {
                const chess::Move best_move =
                    iterative_deepening(
                        board,
                        depth,
                        time_ms,
                        infinite,
                        num_threads,
                        history_snapshot
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

                sync_cout(
                    output.str()
                );
            }
        );

#endif
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