// =============================================================================
//  wasm_api.h -- Fafnir (capa UI): API C expuesta a JavaScript (Emscripten).
//
//  Consumo desde JS:
//      Fafnir.ccall('fafnir_init', ...) o MODULE.EXPORTED_FUNCTIONS.
//  Cada funcion devuelve 1/valor en exito y 0/"0000" en error.
// =============================================================================
#ifndef FAFNIR_WASM_API_H
#define FAFNIR_WASM_API_H
#ifdef __EMSCRIPTEN__

#ifdef __cplusplus
extern "C" {
#endif

int fafnir_init(void);
int fafnir_set_fen(const char* fen);
const char* fafnir_go_movetime(int milliseconds);
void fafnir_stop(void);
int fafnir_eval(void);

#ifdef __cplusplus
}
#endif

#endif // __EMSCRIPTEN__
#endif // FAFNIR_WASM_API_H
