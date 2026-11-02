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
// PV DE UNA JUGADA RAÍZ EN FORMATO UCI
// ============================================================================

void append_root_pv(std::stringstream& info, const fb::RootMove& rm)
{
    if (rm.pv.empty())
    {
        info << ' ' << chess::uci::moveToUci(rm.move);
        return;
    }

    for (const chess::Move& move : rm.pv)
    {
        if (move == chess::Move::NO_MOVE)
            break;

        info << ' ' << chess::uci::moveToUci(move);
    }
}

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

    // MultiPV acotado a [1, jugadas legales]
    multipv = std::clamp(
        multipv,
        1,
        static_cast<int>(initial_moves.size())
    );

    // ------------------------------------------------------------------------
    // CONTROL DEL TIEMPO
    // ------------------------------------------------------------------------

    init_search_time(
        max_time_ms,
        infinite,
        ponder
    );

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

    // Solo el hilo principal hace MultiPV; los helpers (Lazy SMP) buscan
    // en modo single-PV y solo alimentan la TT.
    workers[0].multiPV = multipv;

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

    // Score de cada línea MultiPV en la iteración anterior
    // (centro de la aspiration window de esa línea).
    std::vector<int> line_scores(
        static_cast<std::size_t>(multipv), 0
    );

    for (int current_depth = 1;
         current_depth <= max_depth;
         ++current_depth)
    {
        if (stop_search.load(
                std::memory_order_relaxed))
        {
            break;
        }

        workers[0].sel_depth = 0;

        bool search_failed = false;
        int  completed_lines = 0;

        // --------------------------------------------------------------------
        // UNA BÚSQUEDA POR LÍNEA MULTIPV
        // La línea n busca solo root_moves[n..end): las jugadas elegidas
        // por las líneas anteriores quedan excluidas en search<Root>.
        // --------------------------------------------------------------------

        for (int pv_idx = 0; pv_idx < multipv; ++pv_idx)
        {
            workers[0].pvIdx = pv_idx;

            int line_score =
                line_scores[static_cast<std::size_t>(pv_idx)];
            int delta = 50;
            int alpha = -100000;
            int beta  =  100000;

            if (current_depth >= 5)
            {
                alpha = std::max(-100000, line_score - delta);
                beta  = std::min( 100000, line_score + delta);
            }

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
                    line_score = negamax(
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

                if (line_score <= alpha)
                {
                    alpha = std::max(-100000, alpha - delta);
                    delta += delta / 2;
                    continue;
                }

                if (line_score >= beta)
                {
                    beta = std::min(100000, beta + delta);
                    delta += delta / 2;
                    continue;
                }

                break;
            }

            if (search_failed)
                break;

            line_scores[static_cast<std::size_t>(pv_idx)] = line_score;
            ++completed_lines;
        }

        // ------------------------------------------------------------
        // SI SE ABORTÓ
        // Si al menos la línea 1 terminó en esta profundidad, su jugada
        // (root_moves[0]) es válida y se aprovecha.
        // ------------------------------------------------------------

        if (
            search_failed
            ||
            stop_search.load(
                std::memory_order_relaxed
            )
        )
        {
            if (completed_lines >= 1
                && !workers[0].root_moves.empty())
            {
                const fb::RootMove& top = workers[0].root_moves[0];

                if (top.move != chess::Move::NO_MOVE)
                {
                    best_move = top.move;
                    last_best_move = top.move;
                    last_ponder_move =
                        top.pv.size() > 1
                            ? top.pv[1]
                            : chess::Move::NO_MOVE;
                }
            }

            break;
        }

        // ------------------------------------------------------------
        // ACTUALIZAR MEJOR JUGADA (profundidad completada)
        // ------------------------------------------------------------

        const int score = line_scores[0];

        {
            const fb::RootMove& top = workers[0].root_moves[0];

            if (top.move != chess::Move::NO_MOVE)
            {
                best_move = top.move;
                last_best_move = top.move;
            }

            last_ponder_move =
                top.pv.size() > 1
                    ? top.pv[1]
                    : chess::Move::NO_MOVE;
        }

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
        // INFO POR CADA LÍNEA
        // ------------------------------------------------------------

        for (int i = 0; i < multipv; ++i)
        {
            const fb::RootMove& rm =
                workers[0].root_moves[static_cast<std::size_t>(i)];

            std::stringstream info;

            info << "info depth " << current_depth
                 << " seldepth " << workers[0].sel_depth;

            if (multipv > 1)
                info << " multipv " << (i + 1);

            info << " score "
                 << format_score_uci(
                        line_scores[static_cast<std::size_t>(i)])
                 << " nodes " << nodos_totales
                 << " nps " << nps
                 << " time " << elapsed_ms
                 << " pv";

            append_root_pv(info, rm);

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
//   JS -> fafnir_go_movetime()
//      -> UciEngine::fafnir_go_movetime()
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