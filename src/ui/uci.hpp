#pragma once

#include <atomic>
#include <sstream>
#include <string>
#include <thread>

#include "chesslib.hpp"
#include "SearchController.hpp"

class UciEngine {
public:
    UciEngine();
    ~UciEngine() = default;

    UciEngine(const UciEngine&) = delete;
    UciEngine& operator=(const UciEngine&) = delete;

    void run();
    void main_loop();

    void handle_position(std::stringstream& command);
    void handle_go(std::stringstream& command);
    std::string handle_setoption(std::stringstream& command);

    std::string command_sync(const std::string& line);

    void fafnir_set_fen(const std::string& fen);
    std::string fafnir_go_movetime(int milliseconds);
    void fafnir_stop();
    int fafnir_eval();

private:
    chess::Board board;
    SearchController search;

    std::thread gui_thread;
    std::atomic<bool> is_running{false};

    int num_threads = 1;
    int multipv = 1;
    int hash_size_mb = 16;
    int move_overhead_ms = 50;
};