// =============================================================================
//  wasm_api.cpp -- Fafnir (capa UI): bindings Emscripten para la UI web.
//
//  Esta capa no sabe jugar al ajedrez: traduce llamadas JS al engine.
//  Solo se compila en builds WASM (make -f Makefile.wasm).
// =============================================================================
#include <exception>
#include <iostream>
#include <string>

#include "fafnir_wasm_api.h"
#include "nnue_probe.h"
#include "uci.hpp"

#include <emscripten/emscripten.h>


namespace {

UciEngine g_engine;
bool g_fafnir_initialized = false;

void initialize_fafnir()
{
    if (g_fafnir_initialized)
        return;

    Stockfish::Probe::init();

    g_fafnir_initialized = true;
}

} // namespace


extern "C" {

EMSCRIPTEN_KEEPALIVE
int fafnir_init()
{
    try
    {
        initialize_fafnir();
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fafnir init error: "
                  << e.what() << '\n';
        return 0;
    }
    catch (...)
    {
        std::cerr << "Fafnir init error desconocido.\n";
        return 0;
    }
}

EMSCRIPTEN_KEEPALIVE
int fafnir_set_fen(const char* fen)
{
    if (!fen)
        return 0;

    try
    {
        initialize_fafnir();
        g_engine.fafnir_set_fen(fen);
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fafnir FEN error: "
                  << e.what() << '\n';
        return 0;
    }
    catch (...)
    {
        std::cerr << "Fafnir FEN error desconocido.\n";
        return 0;
    }
}

EMSCRIPTEN_KEEPALIVE
const char* fafnir_go_movetime(int milliseconds)
{
    static std::string result;

    try
    {
        initialize_fafnir();

        result = g_engine.fafnir_go_movetime(milliseconds);
        return result.c_str();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fafnir search error: "
                  << e.what() << '\n';

        result = "0000";
        return result.c_str();
    }
    catch (...)
    {
        std::cerr << "Fafnir search error desconocido.\n";

        result = "0000";
        return result.c_str();
    }
}

EMSCRIPTEN_KEEPALIVE
void fafnir_stop()
{
    try
    {
        g_engine.fafnir_stop();
    }
    catch (...)
    {
    }
}

EMSCRIPTEN_KEEPALIVE
int fafnir_eval()
{
    try
    {
        initialize_fafnir();
        return g_engine.fafnir_eval();
    }
    catch (...)
    {
        return 0;
    }
}

} // extern "C"
