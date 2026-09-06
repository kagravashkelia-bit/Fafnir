#include <iostream>
#include <fstream>
#include <cstdio>
#include "UciEngine.hpp"
#include "probe.h" // Interfaz de Stockfish Probe

// Símbolos generados por 'ld' para Big Net y Small Net
extern "C" {
    // Big Net (nn-b1a57edbea57.nnue)
    extern const unsigned char _binary_nn_b1a57edbea57_nnue_start[];
    extern const unsigned char _binary_nn_b1a57edbea57_nnue_end[];

    // Small Net (nn-baff1ede1f90.nnue)
    extern const unsigned char _binary_nn_baff1ede1f90_nnue_start[];
    extern const unsigned char _binary_nn_baff1ede1f90_nnue_end[];
}

int main() {
    // 1. Obtener la Big Net embebida
    const unsigned char* big_net_data = _binary_nn_b1a57edbea57_nnue_start;
    size_t big_net_size = _binary_nn_b1a57edbea57_nnue_end - _binary_nn_b1a57edbea57_nnue_start;

    // 2. Obtener la Small Net embebida
    const unsigned char* small_net_data = _binary_nn_baff1ede1f90_nnue_start;
    size_t small_net_size = _binary_nn_baff1ede1f90_nnue_end - _binary_nn_baff1ede1f90_nnue_start;

    // 3. Extraer ambas redes a archivos temporales para la sonda
    const std::string tmp_big_path = "embedded_big.tmp";
    const std::string tmp_small_path = "embedded_small.tmp";

    {
        std::ofstream tmp_big(tmp_big_path, std::ios::binary);
        tmp_big.write(reinterpret_cast<const char*>(big_net_data), big_net_size);

        std::ofstream tmp_small(tmp_small_path, std::ios::binary);
        tmp_small.write(reinterpret_cast<const char*>(small_net_data), small_net_size);
    }

    // 4. Inicializar el motor de evaluación de Stockfish con ambas redes
    try {
        Stockfish::Probe::init(tmp_big_path.c_str(), tmp_small_path.c_str());
        std::cout << "info string Stockfish Dual NNUE (Big: nn-b1a57edbea57 | Small: nn-baff1ede1f90) cargadas correctamente." << std::endl;
    } catch (...) {
        std::cerr << "Error crítico al inicializar Stockfish Probe con las redes embebidas." << std::endl;
        std::remove(tmp_big_path.c_str());
        std::remove(tmp_small_path.c_str());
        return 1;
    }

    // 5. Iniciar el bucle del motor UCI
    UciEngine engine;
    engine.run();

    // 6. Limpieza de archivos temporales al salir
    std::remove(tmp_big_path.c_str());
    std::remove(tmp_small_path.c_str());

    return 0;
}