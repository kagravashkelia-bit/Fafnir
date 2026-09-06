#pragma once

#include <sstream>

#include "SearchController.hpp"

class UciEngine {
public:
    void run();

private:
    std::thread gui_thread;
    std::atomic<bool> is_running{false};
    
    void main_loop();
    void handle_position(std::stringstream& command);
    void handle_go(std::stringstream& command);

    chess::Board board;
    int num_threads = 1;
    SearchController search;
};
