#pragma once

#include "sfdev/src/types.h"

namespace chess {
class Board;
class Move;
}

namespace Stockfish::Probe {

struct RawEval {
    int nnue;
};

struct Evaluation {
    int nnue;
    int value;
};

void init();
void sync(const char* fen);
bool push(const chess::Board& previous, const chess::Move& move);
void pop() noexcept;
Evaluation eval_current(int optimism = 0);

int eval(const char* fen, int optimism = 0);
Evaluation eval_detailed(const char* fen, int optimism = 0);
RawEval raw_eval(const char* fen);

int eval(const int pieces[], const int squares[], int pieceAmount,
         bool side, int rule50, int optimism = 0);
Evaluation eval_detailed(const int pieces[], const int squares[], int pieceAmount,
                         bool side, int rule50, int optimism = 0);
RawEval raw_eval(const int pieces[], const int squares[], int pieceAmount,
                 bool side, int rule50);

}  // namespace Stockfish::Probe
