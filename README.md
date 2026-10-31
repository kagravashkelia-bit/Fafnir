# Fafnir

Motor de ajedrez UCI con evaluación NNUE (SFNNv17 de Stockfish dev) y build
nativo (Windows/Linux) y WASM (Emscripten) para UI web.

## Arquitectura (3 capas)

```
src/ui/       INTERFAZ   - protocolo UCI, entry point, bindings WASM (fafnir_*)
src/engine/   MOTOR      - búsqueda, MovePicker/SEE, evaluación (eval/)
src/core/     REGLAS     - costura sobre la librería de reglas (fase 2)
src/bridges/  PUENTES    - glue propio con código de terceros (probe NNUE)
third_party/  VENDIDO    - ChessLib (MIT) y snapshot Stockfish dev (GPLv3)
```

Regla de dependencias: `ui → engine → core → third_party`. La interfaz nunca
entra en la búsqueda; el motor no sabe de protocolos.

## Compilar

Nativo (Windows/Linux):

    make

Binario en `bin/Fafnir(.exe)`. WASM (requiere Emscripten en PATH):

    make -f Makefile.wasm

Salida en `build-wasm/Fafnir.js` + `Fafnir.wasm`.

## Probar

Suite de regresión EPD (Bratko-Kopec, Kaufman, Nolot, LCT-II, Rybka):

    python tests/tests.py bin/Fafnir.exe --depth 18

## CI

`.github/workflows/build-wasm.yml` compila el WASM en cada push.

Licencias y orígenes del código vendido: ver `third_party/VENDORED.md`.
