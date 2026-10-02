#pragma once

#include <algorithm>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "chesslib.hpp"

extern std::atomic<bool> stop_search;

void sync_cout(const std::string& text);

class SearchController {
public:
    SearchController() = default;
    ~SearchController();

    SearchController(const SearchController&) = delete;
    SearchController& operator=(const SearchController&) = delete;

    void set_threads(int threads) {
        num_threads = std::max(1, threads);
    }

    int get_threads() const {
        return num_threads;
    }

    void set_history(const std::vector<uint64_t>& history);

    void start(
        const chess::Board& board,
        int depth,
        int time_ms,
        bool infinite,
        int num_threads = 1
    );

    chess::Move search_sync(
        const chess::Board& board,
        int depth,
        int time_ms,
        bool infinite,
        int num_threads = 1
    );

    void stop();

private:
    std::thread search_thread;
    int num_threads = 1;
    std::vector<uint64_t> current_history;
};