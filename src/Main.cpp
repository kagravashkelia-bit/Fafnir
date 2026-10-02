#include <exception>
#include <iostream>
#include <string>

#include "uci.hpp"
#include "probe.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

namespace {

UciEngine g_engine;
bool g_flashbit_initialized = false;

void initialize_flashbit()
{
    if (g_flashbit_initialized)
        return;

    Stockfish::Probe::init(
        "nn-49c1193b131c.nnue",
        "nn-37f18f62d772.nnue"
    );

    g_flashbit_initialized = true;
}

} // namespace

#ifdef __EMSCRIPTEN__

extern "C" {

EMSCRIPTEN_KEEPALIVE
int flashbit_init()
{
    try
    {
        initialize_flashbit();
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "FlashBit init error: "
                  << e.what() << '\n';
        return 0;
    }
    catch (...)
    {
        std::cerr << "FlashBit init error desconocido.\n";
        return 0;
    }
}

EMSCRIPTEN_KEEPALIVE
int flashbit_set_fen(const char* fen)
{
    if (!fen)
        return 0;

    try
    {
        initialize_flashbit();
        g_engine.flashbit_set_fen(fen);
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "FlashBit FEN error: "
                  << e.what() << '\n';
        return 0;
    }
    catch (...)
    {
        std::cerr << "FlashBit FEN error desconocido.\n";
        return 0;
    }
}

EMSCRIPTEN_KEEPALIVE
const char* flashbit_go_movetime(int milliseconds)
{
    static std::string result;

    try
    {
        initialize_flashbit();

        result = g_engine.flashbit_go_movetime(milliseconds);
        return result.c_str();
    }
    catch (const std::exception& e)
    {
        std::cerr << "FlashBit search error: "
                  << e.what() << '\n';

        result = "0000";
        return result.c_str();
    }
    catch (...)
    {
        std::cerr << "FlashBit search error desconocido.\n";

        result = "0000";
        return result.c_str();
    }
}

EMSCRIPTEN_KEEPALIVE
void flashbit_stop()
{
    try
    {
        g_engine.flashbit_stop();
    }
    catch (...)
    {
    }
}

EMSCRIPTEN_KEEPALIVE
int flashbit_eval()
{
    try
    {
        initialize_flashbit();
        return g_engine.flashbit_eval();
    }
    catch (...)
    {
        return 0;
    }
}

} // extern "C"

int main()
{
    return 0;
}

#else

int main()
{
    try
    {
        initialize_flashbit();
        g_engine.run();
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error inicializando FlashBit: "
                  << e.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cerr << "Error desconocido inicializando FlashBit.\n";
        return 1;
    }
}

#endif