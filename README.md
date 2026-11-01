# Fafnir

UCI chess engine written in C++17 with NNUE evaluation (Stockfish development SFNNv17 architecture), a modern competitive-level search, and native builds for Windows and Linux.

> **Official website:** https://kagravashkelia-bit.github.io/Fafnir/
> (published automatically via GitHub Pages from `web/`)

---

## Features

### Search (`src/engine/search.hpp`)

The search core follows the style of Stockfish development, with an in-house implementation:

- **Transposition tables** with clusters of 3 entries, generation, and age-based replacement (1–1024 MB configurable).
- **Complete history heuristics**: main history, low-ply history, capture history, continuation history (2×2), pawn history, and **correction history** (pawn / minor / non-pawn / continuation) to correct static evaluation bias by structure type.
- **Dedicated MovePicker** with the Stockfish stages: TT move, good captures (MVV-LVA + SEE), quiets ordered by history, bad captures, bad quiets, evasions, ProbCut, and qsearch.
- **SEE (static exchange evaluation)** with support for pinned pieces and **cuckoo detection** of imminent repetition; draw detection by repetition and the fifty-move rule integrated into the stack.
- **Pruning and reductions**: razoring, futility pruning, null-move pruning with verification, IIR (improving iteration reduction), ProbCut, singular extension (+ multicut and negative extensions), LMR with shallow re-search of quiet history and futility at non-PV nodes.
- **Time management** with soft/hard limits, `ponder` support, control by `movetime`, `depth`, `wtime/btime`, increments, and `movestogo`.
- **`Aggressivity` (0–100)**: dedicated option that modulates the optimism applied to evaluation during search.
- Maximum stack depth: `MAX_PLY = 246`.

### NNUE Evaluation (`src/bridges/nnue_probe.cpp` + `third_party/stockfish`)

- **SFNNv17** architecture from the Stockfish development snapshot (commit `49ea5ded38315cff8e67f4a677a9e7811612fbf6`), with a **single-output network without PSQT**.
- Default network: `assets/nnue/nn-252f33942263.nnue`, **embedded in the binary** with `incbin` (no external files are required in production).
- Feature sets: `half_ka_v2_hm` (with horizontal mirroring), `full_threats`, and `pp_3wide`, compiled only in the subset required by the probe.
- Evaluation is quantized and displayed in pawn units as in Stockfish 17.x (`NORMALIZE_TO_PAWN_VALUE = 356`).

### Additional Interfaces

- **C API for WASM** (`src/ui/fafnir_wasm_api.h`): `fafnir_init`, `fafnir_set_fen`, `fafnir_go_movetime`, `fafnir_stop`, `fafnir_eval`. See status below.

---

## UCI Protocol

Supported commands: `uci`, `isready`, `ucinewgame`, `position` (`startpos` / `fen` + `moves`), `go` (`depth`, `movetime`, `wtime`, `btime`, `winc`, `binc`, `movestogo`, `ponder`, `infinite`), `stop`, `setoption`.

Options (`setoption name … value …`). The engine declares only the options that are actually implemented; options that are not implemented respond with `info string unknown UCI option`:

| Option | Range | Default | Implementation |
|---|---|---|---|
| `Hash` | 1–1024 MB | 16 | Real: resizable transposition table in real time. |
| `Threads` | 1–128 | 1 | Real: Lazy SMP with helper threads (`SearchController.cpp`). |
| `MultiPV` | 1–256 | 1 | Real: root search by variation (`search.hpp`). |
| `Move Overhead` | 0–5000 ms | 50 | Real: margin over the clock managed in `go`. |
| `Aggressivity` | 0–100 | 0 | Real: optimism applied to evaluation during search. |

Tablebases and SYZYGY / opening books: **not yet declared** — they will be added together with their real implementation, without announcing non-functional options.

Identity chain: `id name Fafnir 2` — `id author Anthony`.

---

## Build

Native (Windows/Linux), requiring a C++17 toolchain (g++; clang also works with `CXX=clang++`).

The default target builds a binary **safe for any x86_64 CPU** (SSE2 only); it never crashes because of the instruction set, regardless of how optimized the build machine is:

    make

Explicit architecture builds (Stockfish style):

    make ARCH=x86-64-sse4.2     # SSE4.2 + POPCNT   (x86-64-v2)
    make ARCH=x86-64-avx2       # AVX2 + BMI2       (x86-64-v3)
    make ARCH=x86-64-avx512     # AVX-512 + VNNI    (x86-64-v4)
    make ARCH=native            # tuned for this machine
    make all-arches             # bin/Fafnir-<arch> for all (without plugins)

Each architecture compiles into its own `build/<ARCH>/`, so flags are never mixed between binaries. Suffixed binaries are generated in `bin/`. The `bin/Fafnir(.exe)` binary always reflects the last architecture used.

### WebAssembly (in preparation)

    make -f Makefile.wasm

Requires Emscripten in the `PATH`. Output in `build-wasm/Fafnir.js` + `Fafnir.wasm`. The public web edition remains **in preparation**; in the meantime, CI builds the `Fafnir-WASM-Build` artifact on every push (the build had been failing since the code reorganization because the workflow's `find` pulled in code that was not compilable against the current snapshot: the classic JNI bridge `java_api.cpp` (requires `jni.h`) and the legacy `probe.cpp` from the previous Stockfish snapshot; both were removed/excluded, and the workflow no longer mutates the vendored `incbin.h`).

---

## Testing

EPD regression suite (Bratko-Kopec, Kaufman, Nolot, LCT-II, Rybka):

    python tests/tests.py bin/Fafnir.exe --depth 18

In `tests/records/`, control games are archived in PGN, including draws against **Stockfish** and **Reckless 0.9.0** (En Croissant / Lichess).

---

## CI

- `build-native.yml` — builds multi-architecture native binaries (safe x86-64, SSE4.2, AVX2; Windows and Linux) on every push, with a UCI smoke test, and uploads them as artifacts.
- `build-wasm.yml` — builds the WASM on every push to `main`/`master`.
- `deploy-pages.yml` — publishes **the website** (`web/`) to GitHub Pages on every change; URL: <https://kagravashkelia-bit.github.io/Fafnir/>.

## Licenses and Vendored Code

**Project license: GNU GPL v3** (see [`LICENSE`](LICENSE) at the repository root).
Fafnir links and distributes vendored Stockfish code (GPL v3), so the complete engine must be distributed under the terms of the GPL v3; the verbatim text is replicated at the repository root.

Component breakdown:

- **ChessLib** (Disservin, v0.9.4) — MIT, with no additional restriction.
- **Stockfish dev** (SFNNv17 snapshot) — GPLv3; the single local patch is documented.
- **NNUE** `nn-252f33942263.nnue` — default network of the snapshot (SHA-256 `252f33…7ab3`), redistributed under the upstream license.
- **incbin** (Dale Weiler) — Unlicense (public domain).
- **Website** (`web/`) — may be reused as a reference under the GPL v3 as part of the project as a whole.

Complete details and provenance: [`third_party/VENDORED.md`](third_party/VENDORED.md).

---

## Structure
