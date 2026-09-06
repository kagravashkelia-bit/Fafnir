#include "SearchController.hpp"
#include "NegamaxQuinenseSearch.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

// ============================================================================
// VARIABLES GLOBALES DE CONTROL
// ============================================================================
std::atomic<bool> stop_search(false);
chess::Move last_best_move = chess::Move::NO_MOVE;
std::mutex cout_mutex;

void sync_cout(const std::string& text) {
    std::lock_guard<std::mutex> lock(cout_mutex);
    // std::endl garantiza el salto de línea Y el vaciado absoluto del buffer
    std::cout << text << std::endl; 
}

namespace {

// ============================================================================
// FUNCIONES AUXILIARES DE LA BÚSQUEDA (Solo lectura y reporte)
// ============================================================================
std::string get_pv_string(chess::Board board, int depth) {
    std::string pv;
    chess::Move tt_move = chess::Move::NO_MOVE;
    int dummy_score = 0;

    // Usamos una copia del tablero (pasado por valor) para no alterar el real
    for (int ply = 0; ply < depth; ++ply) {
        int dummy_eval = 0;
        
        // CORRECCIÓN 1: Ponemos 0 en la profundidad para que siempre extraiga el movimiento, 
        // sin importar a qué profundidad se guardó en la tabla.
        if (!probe_tt(board.hash(), 0, -100000, 100000,
                      dummy_score, dummy_eval, tt_move, 0)) {
            break;
        }
        
        if (tt_move == chess::Move::NO_MOVE) break;

        pv += chess::uci::moveToUci(tt_move) + " ";
        board.makeMove(tt_move);
    }
    return pv;
}

// ============================================================================
// GESTIÓN DE HILOS SECUNDARIOS (LAZY SMP)
// ============================================================================
void helper_thread_worker(ThreadWorker& worker, chess::Board board, int max_depth) {
    for (int current_depth = 1; current_depth <= max_depth; ++current_depth) {
        if (stop_search) break;

        set_nnue_search_depth(current_depth);
        
        // Cada hilo trabaja sobre su propia copia del tablero
        chess::Board search_board = board; 
        try {
            // El hilo secundario ignora las ventanas de aspiración para explorar más
            negamax(worker, search_board, current_depth, -100000, 100000, 0);
        } catch (const SearchAbortedException&) {
            break; // Salida limpia si el controlador principal aborta la búsqueda
        }
    }
}

// ============================================================================
// NÚCLEO DEL CONTROLADOR: ITERATIVE DEEPENING
// ============================================================================
chess::Move iterative_deepening(chess::Board board, int max_depth, int max_time_ms, bool infinite, int num_threads) {
    chess::Move best_move = chess::Move::NO_MOVE;
    last_best_move = chess::Move::NO_MOVE;
    stop_search = false;
    
    // CORRECCIÓN 2: Salvavidas inicial. Obtenemos un movimiento legal válido desde el inicio
    // por si la búsqueda se cancela muy rápido, jamás mandará 0000.
    chess::Movelist initial_moves;
    chess::movegen::legalmoves(initial_moves, board);
    if (!initial_moves.empty()) {
        last_best_move = initial_moves[0];
        best_move = initial_moves[0];
    }

    init_search_time(max_time_ms, infinite);

    int alpha = -100000;
    int beta = 100000;
    int delta = 50;
    int score = 0;

    // 1. Inicializar Trabajadores (Memoria independiente para cada hilo)
    std::vector<ThreadWorker> workers(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        workers[i].id = i;
        workers[i].pos_history.clear();
        workers[i].pos_history.push_back(board.hash());
    }

    // 2. Lanzar Hilos Auxiliares (Lazy SMP)
    std::vector<std::thread> helper_threads;
    for (int i = 1; i < num_threads; ++i) {
        helper_threads.emplace_back(helper_thread_worker, std::ref(workers[i]), board, max_depth);
    }

    // 3. Hilo Principal: Ventanas de Aspiración y Control de Tiempo
    for (int current_depth = 1; current_depth <= max_depth; ++current_depth) {
        if (stop_search) break;

        set_nnue_search_depth(current_depth);

        // Ajuste de Ventanas de Aspiración
        if (current_depth >= 5) {
            alpha = std::max(-100000, score - delta);
            beta = std::min(100000, score + delta);
        } else {
            alpha = -100000;
            beta = 100000;
        }

        bool search_failed = false;
        chess::Move current_depth_best = chess::Move::NO_MOVE; 

        while (true) {
            chess::Board search_board = board;
            try {
                score = negamax(workers[0], search_board, current_depth, alpha, beta, 0);
            } catch (const SearchAbortedException&) {
                search_failed = true;
                break;
            }

            if (stop_search) {
                search_failed = true;
                break;
            }

            // Fallas de la ventana de aspiración (Research)
            if (score <= alpha) {
                alpha = std::max(-100000, alpha - delta);
                delta += delta / 2;
            } else if (score >= beta) {
                beta = std::min(100000, beta + delta);
                delta += delta / 2;
            } else {
                // Búsqueda exitosa, obtenemos la jugada confirmada desde la Transposition Table
                int dummy_score = 0, dummy_eval = 0;
                probe_tt(board.hash(), current_depth, -100000, 100000, dummy_score, dummy_eval, current_depth_best, 0);
                
                // CORRECCIÓN 3: Si la Tabla de Transposición perdió la jugada, usamos el salvavidas.
                if (current_depth_best == chess::Move::NO_MOVE && !initial_moves.empty()) {
                    current_depth_best = initial_moves[0];
                }
                
                break;
            }
        }

        if (search_failed || stop_search) break;
        delta = 50; // Reiniciar delta para la siguiente profundidad

        // Consolidar la mejor jugada
        if (current_depth_best != chess::Move::NO_MOVE) {
            best_move = current_depth_best;
            last_best_move = current_depth_best;
        }

        // 4. Recopilar Estadísticas y Reportar (UCI)
        uint64_t nodos_totales = 0;
        for (const auto& w : workers) {
            nodos_totales += w.nodos_busqueda;
        }

        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - search_start_time).count();
        uint64_t nps = elapsed_ms > 0 ? static_cast<uint64_t>(nodos_totales) * 1000 / elapsed_ms : 0;
        std::string pv_line = get_pv_string(board, current_depth);

        std::stringstream info;
        info << "info depth " << current_depth
             << " score " << format_score_uci(score)
             << " nodes " << nodos_totales
             << " nps " << nps
             << " time " << elapsed_ms;
             
        if (!pv_line.empty()) {
            info << " pv " << pv_line;
        } else if (best_move != chess::Move::NO_MOVE) {
            info << " pv " << chess::uci::moveToUci(best_move);
        }
        sync_cout(info.str());

        // El deadline se comprueba dentro de la búsqueda para consumir todo movetime.
    }

    // 6. Limpieza y Sincronización de Hilos
    stop_search = true;
    for (auto& t : helper_threads) {
        if (t.joinable()) t.join();
    }

    return best_move;
}

} // namespace

// ============================================================================
// INTERFAZ PÚBLICA DEL CONTROLADOR
// ============================================================================
SearchController::~SearchController() {
    stop();
}

void SearchController::start(const chess::Board& board, int depth, int time_ms, bool infinite, int num_threads) {
    stop(); // Asegurarnos de que cualquier búsqueda anterior haya terminado
    stop_search = false;

    search_thread = std::thread([this, board, depth, time_ms, infinite, num_threads]() {
        chess::Move best_move = iterative_deepening(board, depth, time_ms, infinite, num_threads);
        chess::Move final_move = (best_move != chess::Move::NO_MOVE) ? best_move : last_best_move;

        std::stringstream output;
        output << "bestmove " 
               << ((final_move != chess::Move::NO_MOVE) ? chess::uci::moveToUci(final_move) : "0000");
        sync_cout(output.str());
    });
}

void SearchController::stop() {
    stop_search = true;
    if (search_thread.joinable()) {
        search_thread.join();
    }
}