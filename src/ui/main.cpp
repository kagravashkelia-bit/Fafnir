// =============================================================================
//  main.cpp -- Fafnir (capa UI): entry point native del binario CLI/UCI.
//
//  La API WASM para navegadores vive en wasm_api.cpp ( bindings fafnir_* ),
//  separada de este entry point para que cada binario enclace solo lo suyo.
// =============================================================================
#include <iostream>
#include <string>

#include "fafnir_wasm_api.h"
#include "nnue_probe.h"
#include "uci.hpp"


namespace {

bool g_fafnir_initialized = false;

void initialize_fafnir()
{
    if (g_fafnir_initialized)
        return;

    Stockfish::Probe::init();

    g_fafnir_initialized = true;
}

} // namespace


int main()
{
    try
    {
        initialize_fafnir();
        UciEngine g_engine;
        g_engine.run();
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error inicializando Fafnir: "
                  << e.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cerr << "Error desconocido inicializando Fafnir.\n";
        return 1;
    }
}
