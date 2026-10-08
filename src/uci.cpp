#include "uci.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
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
                   chess::Color side_to_move,
                   int move_overhead_ms) {

    int time_left =
        side_to_move == chess::Color::WHITE ? wtime : btime;

    int increment =
        side_to_move == chess::Color::WHITE ? winc : binc;

    if (time_left <= 0)
        return 10;

    int moves = movestogo > 0 ? movestogo : 30;

    int time_allocated =
        (time_left / moves) + (increment * 3 / 4);

    return std::max(10, time_allocated - move_overhead_ms);
}

std::string uci_option_declarations() {
    return
        "option name Hash type spin default 16 min 1 max 1024\n"
        "option name Threads type spin default 1 min 1 max 128\n"
        "option name MultiPV type spin default 1 min 1 max 256\n"
        "option name SyzygyPath type string default <empty>\n"
        "option name OwnBook type check default false\n"
        "option name Book File type string default <empty>\n"
        "option name Move Overhead type spin default 50 min 0 max 5000\n"
        "option name Aggressivity type spin default 0 min 0 max 100\n";
}

bool parse_option_integer(const std::string& value, int& result) {
    try {
        std::size_t parsed = 0;
        result = std::stoi(value, &parsed);
        return parsed == value.size();
    } catch (const std::invalid_argument&) {
        return false;
    } catch (const std::out_of_range&) {
        return false;
    }
}

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char character) {
                       return static_cast<char>(std::tolower(character));
                   });
    return value;
}

} // namespace

UciEngine::UciEngine() {
    resize_tt(static_cast<std::size_t>(hash_size_mb));
}


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

std::string UciEngine::handle_setoption(std::stringstream& command) {
    search.stop();

    std::string remainder;
    std::getline(command, remainder);
    const auto first = remainder.find_first_not_of(" \t");
    if (first == std::string::npos || remainder.compare(first, 5, "name ") != 0)
        return "info string malformed setoption command";

    remainder.erase(0, first + 5);
    const auto value_marker = remainder.find(" value ");
    const std::string option_name = lowercase(
        remainder.substr(0, value_marker)
    );
    const std::string value = value_marker == std::string::npos
        ? std::string()
        : remainder.substr(value_marker + 7);

    int parsed_value = 0;
    if (option_name == "hash") {
        if (!parse_option_integer(value, parsed_value))
            return "info string invalid value for Hash";

        const int new_size = std::clamp(parsed_value, 1, 1024);
        try {
            resize_tt(static_cast<std::size_t>(new_size));
        } catch (const std::bad_alloc&) {
            return "info string unable to allocate requested Hash size";
        }
        hash_size_mb = new_size;
    } else if (option_name == "threads") {
        if (!parse_option_integer(value, parsed_value))
            return "info string invalid value for Threads";
        num_threads = std::clamp(parsed_value, 1, 128);
    } else if (option_name == "multipv") {
        if (!parse_option_integer(value, parsed_value))
            return "info string invalid value for MultiPV";
        multipv = std::clamp(parsed_value, 1, 256);
    } else if (option_name == "move overhead") {
        if (!parse_option_integer(value, parsed_value))
            return "info string invalid value for Move Overhead";
        move_overhead_ms = std::clamp(parsed_value, 0, 5000);
    } else if (option_name == "aggressivity") {
        if (!parse_option_integer(value, parsed_value))
            return "info string invalid value for Aggressivity";
        search_aggressivity = std::clamp(parsed_value, 0, 100);
    } else if (option_name == "syzygypath") {
        syzygy_path = value;
        if (!value.empty())
            return "info string SyzygyPath saved, but Syzygy probing is not implemented";
    } else if (option_name == "book file") {
        opening_book_file = value;
        if (!value.empty())
            return "info string Book File saved, but opening-book probing is not implemented";
    } else if (option_name == "ownbook") {
        const std::string boolean_value = lowercase(value);
        if (boolean_value != "true" && boolean_value != "false")
            return "info string invalid value for OwnBook";
        own_book = boolean_value == "true";
        if (own_book)
            return "info string OwnBook enabled, but opening-book probing is not implemented";
    } else {
        return "info string unknown UCI option: " + option_name;
    }

    return "";
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
    bool ponder = false;
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

        else if (token == "ponder") {

            ponder = true;
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
                board.sideToMove(),
                move_overhead_ms
            );
    }


    search.start(
        board,
        depth,
        time_allocated,
        infinite,
        num_threads,
        multipv,
        ponder
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
        << "FlashBit 1.3"
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
                "id name FlashBit 1.3\n"
                "id author Anthony\n"
                + uci_option_declarations()
                + "uciok\n"
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
            const auto evaluation = Stockfish::Probe::eval_detailed(
                board.getFen().c_str()
            );

            sync_cout(
                "info string eval "
                + format_score_uci(evaluation.value)
                + " nnue " + std::to_string(evaluation.nnue)
                + " architecture SFNNv17"
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
            const std::string response = handle_setoption(command);
            if (!response.empty())
                sync_cout(response);
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

        else if (name == "ponderhit") {

            search.ponderhit();
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
            "id name FlashBit 1.3\n"
            "id author Anthony\n"
            + uci_option_declarations()
            + "uciok\n";
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

    if (name == "ponderhit") {
        search.ponderhit();
        return "";
    }


    // ========================================================================
    // SETOPTION
    // ========================================================================

    if (name == "setoption")
        return handle_setoption(command);



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
                    board.sideToMove(),
                    move_overhead_ms
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
                num_threads,
                multipv
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