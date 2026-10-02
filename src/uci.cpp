#include "uci.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "evaluate.hpp"
#include "search.hpp"

#ifdef sync_cout
#undef sync_cout
#endif

namespace {

int calculate_time(int wtime,
                   int btime,
                   int winc,
                   int binc,
                   int movestogo,
                   chess::Color side_to_move) {

    int time_left =
        side_to_move == chess::Color::WHITE ? wtime : btime;

    int increment =
        side_to_move == chess::Color::WHITE ? winc : binc;

    if (time_left <= 0)
        return 10;

    int moves = movestogo > 0 ? movestogo : 30;

    int time_allocated =
        (time_left / moves) + (increment * 3 / 4);

    return std::max(10, time_allocated - 50);
}

} // namespace


// ============================================================================
// POSITION
// ============================================================================

void UciEngine::handle_position(std::stringstream& command)
{
    search.stop();

    std::string position_type;

    if (!(command >> position_type))
        return;


    // ------------------------------------------------------------------------
    // STARTPOS
    // ------------------------------------------------------------------------

    if (position_type == "startpos") {

        board = chess::Board();

        // Consume "moves" if present.
        std::string token;

        if (command >> token) {

            if (token != "moves") {
                // No "moves", so nothing else to process.
                return;
            }
        }
    }


    // ------------------------------------------------------------------------
    // FEN
    // ------------------------------------------------------------------------

    else if (position_type == "fen") {

        std::string fen;
        std::string token;

        while (command >> token) {

            if (token == "moves")
                break;

            if (!fen.empty())
                fen += " ";

            fen += token;
        }

        if (!fen.empty())
            board.setFen(fen);
    }


    // ------------------------------------------------------------------------
    // UNKNOWN POSITION TYPE
    // ------------------------------------------------------------------------

    else {
        return;
    }


    // ------------------------------------------------------------------------
    // RECONSTRUIR HISTORY
    // ------------------------------------------------------------------------

    std::vector<uint64_t> history;
    history.reserve(128);

    history.push_back(board.hash());


    // ------------------------------------------------------------------------
    // APPLY MOVES
    // ------------------------------------------------------------------------

    std::string move_text;

    while (command >> move_text) {

        if (move_text == "moves")
            continue;

        chess::Move move =
            chess::uci::uciToMove(board, move_text);

        if (move != chess::Move::NO_MOVE) {

            board.makeMove(move);

            history.push_back(board.hash());
        }
    }


    // ------------------------------------------------------------------------
    // SYNC NNUE / RUBI
    // ------------------------------------------------------------------------

    rubi_sync(board);

    search.set_history(history);
}


// ============================================================================
// GO
// ============================================================================

void UciEngine::handle_go(std::stringstream& command)
{
    search.stop();

    int depth = 64;

    int movetime = 0;

    int wtime = 0;
    int btime = 0;

    int winc = 0;
    int binc = 0;

    int movestogo = 0;

    bool infinite = false;
    bool depth_specified = false;


    std::string token;

    while (command >> token) {

        if (token == "depth") {

            command >> depth;

            depth_specified = true;
        }

        else if (token == "movetime") {

            command >> movetime;
        }

        else if (token == "wtime") {

            command >> wtime;
        }

        else if (token == "btime") {

            command >> btime;
        }

        else if (token == "winc") {

            command >> winc;
        }

        else if (token == "binc") {

            command >> binc;
        }

        else if (token == "movestogo") {

            command >> movestogo;
        }

        else if (token == "infinite") {

            infinite = true;
        }
    }


    if (infinite && !depth_specified)
        depth = 64;


    int time_allocated = movetime;


    if (time_allocated == 0 &&
        (wtime > 0 || btime > 0)) {

        time_allocated =
            calculate_time(
                wtime,
                btime,
                winc,
                binc,
                movestogo,
                board.sideToMove()
            );
    }


    search.start(
        board,
        depth,
        time_allocated,
        infinite,
        num_threads
    );
}


// ============================================================================
// RUN
// ============================================================================

void UciEngine::run()
{

#ifndef __EMSCRIPTEN__

    is_running = true;

    gui_thread =
        std::thread(
            &UciEngine::main_loop,
            this
        );

    if (gui_thread.joinable())
        gui_thread.join();

#else

    // En WASM no usamos stdin/stdout como interfaz UCI.
    //
    // JavaScript debe utilizar command_sync()
    // o las funciones flashbit_* exportadas.
    //
    is_running = true;

#endif
}


// ============================================================================
// FLASHBIT - SET FEN
// ============================================================================

void UciEngine::flashbit_set_fen(const std::string& fen)
{
    search.stop();

    if (fen.empty())
        return;


    board.setFen(fen);

    rubi_sync(board);


    std::vector<uint64_t> history;
    history.reserve(1);

    history.push_back(board.hash());

    search.set_history(history);
}


// ============================================================================
// FLASHBIT - GO MOVETIME
// ============================================================================

std::string UciEngine::flashbit_go_movetime(int milliseconds)
{
    const int time_ms =
        std::max(1, milliseconds);


    chess::Move best_move =
        search.search_sync(
            board,
            64,
            time_ms,
            false,
            num_threads
        );


    if (best_move == chess::Move::NO_MOVE)
        return "0000";


    return chess::uci::moveToUci(best_move);
}


// ============================================================================
// FLASHBIT - STOP
// ============================================================================

void UciEngine::flashbit_stop()
{
    search.stop();
}


// ============================================================================
// FLASHBIT - EVAL
// ============================================================================

int UciEngine::flashbit_eval()
{
    return evaluate(board);
}


// ============================================================================
// UCI CONSOLE LOOP
// ============================================================================

void UciEngine::main_loop()
{
    std::cout
        << "FlashBit 1.2 NNUE"
        << std::endl;


    std::string line;


    while (is_running &&
           std::getline(std::cin, line)) {

        if (line.empty())
            continue;


        std::stringstream command(line);

        std::string name;

        command >> name;


        // ====================================================================
        // UCI
        // ====================================================================

        if (name == "uci") {

            sync_cout(
                "id name FlashBit 1.2 NNUE\n"
                "id author Anthony\n"
                "option name Hash type spin "
                "default 16 min 1 max 1024\n"
                "option name Threads type spin "
                "default 1 min 1 max 128\n"
                "uciok\n"
            );
        }


        // ====================================================================
        // ISREADY
        // ====================================================================

        else if (name == "isready") {

            sync_cout(
                "readyok\n"
            );
        }


        // ====================================================================
        // EVAL
        // ====================================================================

        else if (name == "eval") {

            sync_cout(
                format_score_uci(evaluate(board))
                + "\n\n"
            );
        }


        // ====================================================================
        // UCINEWGAME
        // ====================================================================

        else if (name == "ucinewgame") {

            search.stop();

            clear_tt();

            board = chess::Board();

            rubi_sync(board);


            std::vector<uint64_t> history;

            history.push_back(
                board.hash()
            );

            search.set_history(history);


            sync_cout(
                "readyok\n"
            );
        }


        // ====================================================================
        // POSITION
        // ====================================================================

        else if (name == "position") {

            handle_position(command);
        }


        // ====================================================================
        // MOVE / USERMOVE
        // ====================================================================

        else if (name == "move" ||
                 name == "usermove") {

            search.stop();


            std::string move_text;

            if (command >> move_text) {

                chess::Move move =
                    chess::uci::uciToMove(
                        board,
                        move_text
                    );


                if (move != chess::Move::NO_MOVE) {

                    board.makeMove(move);

                    rubi_sync(board);
                }
            }
        }


        // ====================================================================
        // SETOPTION
        // ====================================================================

        else if (name == "setoption") {

            std::string token;
            std::string opt_name;
            std::string opt_val;


            command >> token;


            if (token == "name") {

                command >> opt_name;

                command >> token;


                if (token == "value") {

                    command >> opt_val;


                    if (opt_name == "Threads" ||
                        opt_name == "threads") {

                        try {

                            num_threads =
                                std::clamp(
                                    std::stoi(opt_val),
                                    1,
                                    128
                                );
                        }
                        catch (...) {

                            num_threads = 1;
                        }
                    }
                }
            }
        }


        // ====================================================================
        // GO
        // ====================================================================

        else if (name == "go") {

            handle_go(command);
        }


        // ====================================================================
        // STOP
        // ====================================================================

        else if (name == "stop") {

            search.stop();
        }


        // ====================================================================
        // QUIT
        // ====================================================================

        else if (name == "quit") {

            search.stop();

            is_running = false;

            break;
        }
    }
}


// ============================================================================
// SYNCHRONOUS UCI FOR WASM / JAVASCRIPT
// ============================================================================
//
// Permite hacer:
//
//     command_sync("uci")
//     command_sync("isready")
//     command_sync("position startpos")
//     command_sync("go movetime 3000")
//
// sin depender de std::getline(std::cin, ...).
// ============================================================================

std::string UciEngine::command_sync(
    const std::string& line)
{
    std::stringstream command(line);

    std::string name;


    if (!(command >> name))
        return "";



    // ========================================================================
    // UCI
    // ========================================================================

    if (name == "uci") {

        return
            "id name FlashBit 1.2 NNUE\n"
            "id author Anthony\n"
            "option name Hash type spin "
            "default 16 min 1 max 1024\n"
            "option name Threads type spin "
            "default 1 min 1 max 128\n"
            "uciok\n";
    }



    // ========================================================================
    // ISREADY
    // ========================================================================

    if (name == "isready") {

        return "readyok\n";
    }



    // ========================================================================
    // UCINEWGAME
    // ========================================================================

    if (name == "ucinewgame") {

        search.stop();

        clear_tt();

        board = chess::Board();

        rubi_sync(board);


        std::vector<uint64_t> history;

        history.push_back(
            board.hash()
        );

        search.set_history(history);


        return "";
    }



    // ========================================================================
    // POSITION
    // ========================================================================

    if (name == "position") {

        handle_position(command);

        return "";
    }



    // ========================================================================
    // EVAL
    // ========================================================================

    if (name == "eval") {

        return
            format_score_uci(
                evaluate(board)
            )
            + "\n";
    }



    // ========================================================================
    // STOP
    // ========================================================================

    if (name == "stop") {

        search.stop();

        return "";
    }



    // ========================================================================
    // SETOPTION
    // ========================================================================

    if (name == "setoption") {

        std::string token;
        std::string opt_name;
        std::string opt_val;


        command >> token;


        if (token == "name") {

            command >> opt_name;

            command >> token;


            if (token == "value") {

                command >> opt_val;


                if (opt_name == "Threads" ||
                    opt_name == "threads") {

                    try {

                        num_threads =
                            std::clamp(
                                std::stoi(opt_val),
                                1,
                                128
                            );
                    }
                    catch (...) {

                        num_threads = 1;
                    }
                }
            }
        }


        return "";
    }



    // ========================================================================
    // GO
    // ========================================================================

    if (name == "go") {

        int depth = 128;

        int movetime = 0;

        int wtime = 0;
        int btime = 0;

        int winc = 0;
        int binc = 0;

        int movestogo = 0;

        bool infinite = false;

        bool depth_specified = false;


        std::string token;


        while (command >> token) {

            if (token == "depth") {

                command >> depth;

                depth_specified = true;
            }

            else if (token == "movetime") {

                command >> movetime;
            }

            else if (token == "wtime") {

                command >> wtime;
            }

            else if (token == "btime") {

                command >> btime;
            }

            else if (token == "winc") {

                command >> winc;
            }

            else if (token == "binc") {

                command >> binc;
            }

            else if (token == "movestogo") {

                command >> movestogo;
            }

            else if (token == "infinite") {

                infinite = true;
            }
        }


        // --------------------------------------------------------------------
        // DEPTH / TIME
        // --------------------------------------------------------------------

        if (infinite &&
            !depth_specified) {

            depth = 64;
        }


        int time_allocated = movetime;


        if (time_allocated == 0 &&
            (wtime > 0 || btime > 0)) {

            time_allocated =
                calculate_time(
                    wtime,
                    btime,
                    winc,
                    binc,
                    movestogo,
                    board.sideToMove()
                );
        }


        // --------------------------------------------------------------------
        // SYNCHRONOUS WASM SEARCH
        // --------------------------------------------------------------------

        const chess::Move best_move =
            search.search_sync(
                board,
                depth,
                time_allocated,
                infinite,
                num_threads
            );


        if (best_move == chess::Move::NO_MOVE)
            return "bestmove 0000\n";


        return
            "bestmove " +
            chess::uci::moveToUci(best_move) +
            "\n";
    }



    // ========================================================================
    // QUIT
    // ========================================================================

    if (name == "quit") {

        search.stop();

        is_running = false;

        return "";
    }



    // ========================================================================
    // UNKNOWN COMMAND
    // ========================================================================

    return "";
}