# Fafnir

Motor de ajedrez UCI en C++17 con evaluación NNUE (arquitectura SFNNv17 del
desarrollo de Stockfish), búsqueda moderna de nivel competitivo y builds
nativos para Windows y Linux.

> **Sitio web oficial:** https://kagravashkelia-bit.github.io/Fafnir/
> (publicado automáticamente vía GitHub Pages desde `web/`)

---

## Características

### Búsqueda (`src/engine/search.hpp`)

El núcleo de búsqueda sigue el estilo del desarrollo de Stockfish, con
implementación propia:

- **Tablas de transposición** con clusters de 3 entradas, generación y
  reemplazo por edad (1–1024 MB configurables).
- **Historial completo de heurísticas**: main history, low-ply history,
  capture history, continuation history (2×2), pawn history, y
  **correction history** (pawn / minor / non-pawn / continuation) para
  corregir el sesgo de la evaluación estática por tipo de estructura.
- **MovePicker propio** con las etapas de Stockfish: TT move, buenas
  capturas (MVV-LVA + SEE), quiets ordenados por historial, malas capturas,
  malos quiets, evasiones, ProbCut y qsearch.
- **SEE (static exchange evaluation)** con soporte de pinned pieces y
  **detección cuckoo** de repetición inminente; detección de empate por
  repetición y regla de 50 jugadas integrada en la pila.
- **Podados y reducciones**: razoring, futility pruning, null-move pruning
  con verificación, IIR (improving iteration reduction), ProbCut,
  extensión singular (+ multicut y extensiones negativas), LMR con
  refundición superficial del historial de quiets y futility en nodos no-PV.
- **Gestión de tiempo** con límites soft/hard, soporte `ponder`, control
  por `movetime`, `depth`, `wtime/btime`, incrementos y `movestogo`.
- **`Aggressivity` (0–100)**: opción propia que modula el optimismo
  aplicado a la evaluación durante la búsqueda.
- Profundidad máxima de pila: `MAX_PLY = 246`.

### Evaluación NNUE (`src/bridges/nnue_probe.cpp` + `third_party/stockfish`)

- Arquitectura **SFNNv17** del snapshot del desarrollo de Stockfish
  (commit `49ea5ded38315cff8e67f4a677a9e7811612fbf6`), con red de **salida
  única sin PSQT**.
- Red por defecto: `assets/nnue/nn-252f33942263.nnue`, **embebida en el
  binario** con `incbin` (no requiere archivos externos en producción).
- Feature sets: `half_ka_v2_hm` (con espejo horizontal),
  `full_threats` y `pp_3wide`, compilados sólo en el subconjunto requerido
  por el probe.
- La evaluación se cuantiza y se muestra en unidades de peón como
  Stockfish 17.x (`NORMALIZE_TO_PAWN_VALUE = 356`).

### Interfaces adicionales

- **API JNI** (`src/bridges/java_api.{h,cpp}`) para consumir el evaluador
  NNUE desde Java (`NNUEBridge`: `init`, `evalFen`, `evalArray`,
  `fasterEvalArray`).
- **API C para WASM** (`src/ui/fafnir_wasm_api.h`):
  `fafnir_init`, `fafnir_set_fen`, `fafnir_go_movetime`, `fafnir_stop`,
  `fafnir_eval`. Ver estado más abajo.

---

## Protocolo UCI

Comandos soportados: `uci`, `isready`, `ucinewgame`, `position`
(`startpos` / `fen` + `moves`), `go` (`depth`, `movetime`, `wtime`,
`btime`, `winc`, `binc`, `movestogo`, `ponder`, `infinite`), `stop`,
`setoption`.

Opciones (`setoption name … value …`). El motor declara solo las opciones
que se implementan de verdad; las opciones no implementadas responden con
`info string unknown UCI option`:

| Opción | Rango | Defecto | Implementación |
|---|---|---|---|
| `Hash` | 1–1024 MB | 16 | Real: tabla de transposición redimensionable en tiempo real. |
| `Threads` | 1–128 | 1 | Real: Lazy SMP con helper threads (`SearchController.cpp`). |
| `MultiPV` | 1–256 | 1 | Real: búsqueda de la raíz por variación (`search.hpp`). |
| `Move Overhead` | 0–5000 ms | 50 | Real: margen sobre el reloj gestionado en `go`. |
| `Aggressivity` | 0–100 | 0 | Real: optimismo aplicado a la evaluación en la búsqueda. |

Tabla base y SYZYGY / libros de aperturas: **no declarados todavía** — se
añadirán junto con su implementación real, sin anunciar opciones no
funcionales.

Cadena de identidad: `id name Fafnir 2` — `id author Anthony`.

---

## Compilar

Nativo (Windows/Linux), requiere un toolchain C++17 (g++; clang también
funciona con `CXX=clang++`).

El objetivo por defecto construye el binario **seguro para cualquier CPU
x86_64** (SSE2 únicamente); nunca crashea por conjunto de instrucciones, por
más optimizada que sea la máquina de compilación:

    make

Compilaciones por arquitectura explícita (estilo Stockfish):

    make ARCH=x86-64-sse4.2     # SSE4.2 + POPCNT   (x86-64-v2)
    make ARCH=x86-64-avx2       # AVX2 + BMI2       (x86-64-v3)
    make ARCH=x86-64-avx512     # AVX-512 + VNNI    (x86-64-v4)
    make ARCH=native            # sintonizada para esta máquina
    make all-arches             # bin/Fafnir-<arch> para todas (sin plugins)

Cada arquitectura compila en su propio `build/<ARCH>/`, de modo que nunca se
mezclan flags entre binarios. Los binarios sufijados se generan en `bin/`.
El binario `bin/Fafnir(.exe)` refleja siempre la última arquitectura usada.

### WebAssembly (en preparación)

    make -f Makefile.wasm

Requiere Emscripten en el `PATH`. Salida en `build-wasm/Fafnir.js` +
`Fafnir.wasm`. La edición web pública sigue **en preparación**; mientras
tanto, la CI compila el artefacto `Fafnir-WASM-Build` en cada push (el build
fallaba desde la reorganización del código porque el workflow incluía el
puente JNI `java_api.cpp`, que requiere JNI y no es compilable en WASM;
el `find` del workflow ya excluye ese archivo).

---

## Probar

Suite de regresión EPD (Bratko-Kopec, Kaufman, Nolot, LCT-II, Rybka):

    python tests/tests.py bin/Fafnir.exe --depth 18

En `tests/records/` se archivan partidas de control en PGN, entre ellas
tablas contra **Stockfish** y **Reckless 0.9.0** (En Croissant / Lichess).

---

## CI

- `build-native.yml` — compila binarios nativos multi-arquitectura
  (x86-64 seguro, SSE4.2, AVX2; Windows y Linux) en cada push, con
  smoke test UCI, y los sube como artefactos.
- `build-wasm.yml` — compila el WASM en cada push a `main`/`master`.
- `deploy-pages.yml` — publica **el sitio web** (`web/`) en GitHub Pages
  en cada cambio; URL: <https://kagravashkelia-bit.github.io/Fafnir/>.

## Licencias y código vendido

**Licencia del proyecto: GNU GPL v3** (ver [`LICENSE`](LICENSE) en la raíz).
Fafnir enlaza y distribuye código vendido de Stockfish (GPL v3), por lo que el
motor completo debe distribuirse bajo los términos de la GPL v3; el texto
verbatim se replica en la raíz del repositorio.

Desglose por componente:

- **ChessLib** (Disservin, v0.9.4) — MIT, sin restricción adicional.
- **Stockfish dev** (snapshot SFNNv17) — GPLv3; el parche local único
  está documentado.
- **NNUE** `nn-252f33942263.nnue` — red por defecto del snapshot
  (SHA-256 `252f33…7ab3`), redistribuida bajo la licencia del upstream.
- **incbin** (Dale Weiler) — Unlicense (dominio público).
- **Sitio web** (`web/`) — puede reutilizarse como referencia conforme a
  la GPL v3 en su conjunto del proyecto.

Detalles completos y procedencia: [`third_party/VENDORED.md`](third_party/VENDORED.md).

---

## Estructura

```
src/ui         INTERFAZ   protocolo UCI, entry point, bindings WASM (fafnir_*)
src/engine     MOTOR      búsqueda, MovePicker/SEE, evaluación (eval/)
src/core       REGLAS     costura sobre la librería de reglas
src/bridges    PUENTES    glue propio con código de terceros (probe NNUE, JNI)
third_party    VENDIDO    ChessLib (MIT) y snapshot Stockfish dev (GPLv3)
web            WEB        sitio oficial (GitHub Pages)
tests          PRUEBAS    suite EPD + partidas PGN registradas
```

Regla de dependencias: `ui → engine → core → third_party`. La interfaz
nunca entra en la búsqueda; el motor no sabe de protocolos.
