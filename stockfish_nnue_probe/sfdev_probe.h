// =============================================================================
//  sfdev_probe.h  --  sonda NNUE de Stockfish dev para FlashBit
//
//  * eval_current()/eval_detailed() devuelven EXACTAMENTE Eval::evaluate de SF
//    (mezcla optimismo/complejidad, material, damping rule50, clamp TB).
//  * push()/pop() son incrementales; push_null() hace el null move incremental
//    (sin resync por FEN). Si el Position de sfdev no expone una firma conocida
//    de do_null_move, push_null() devuelve false SIN tocar el estado.
// =============================================================================
#pragma once

#include <cstdint>

#include "chesslib.hpp"

// search.hpp usa esta macro para activar el null move incremental
#define FB_PROBE_HAS_NULL 1

namespace Stockfish::Probe {

struct Evaluation {
    int nnue;    // salida de la red antes de mezclar con optimismo/material
    int value;   // valor final de Stockfish (perspectiva del lado que mueve)
};

struct RawEval {
    int nnue;
};

void init();

// Contexto por hilo
void sync(const char* fen);
bool push(const chess::Board& before, const chess::Move& move);
bool push_null(const chess::Board& before);          // false = no soportado/no sincronizado (sin efectos)
void pop() noexcept;                                 // deshace el ultimo push o push_null
Evaluation eval_current(int optimism = 0);

// Evaluaciones sueltas (no tocan el contexto incremental)
int        eval(const char* fen, int optimism = 0);
Evaluation eval_detailed(const char* fen, int optimism = 0);
RawEval    raw_eval(const char* fen);

int        eval(const int pieces[], const int squares[], int piece_amount, bool side, int rule50 = 0, int optimism = 0);
Evaluation eval_detailed(const int pieces[], const int squares[], int piece_amount, bool side, int rule50 = 0, int optimism = 0);
RawEval    raw_eval(const int pieces[], const int squares[], int piece_amount, bool side, int rule50 = 0);

}  // namespace Stockfish::Probe