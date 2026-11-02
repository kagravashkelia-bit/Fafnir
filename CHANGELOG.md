# Changelog

All notable changes to Fafnir are documented in this file.
The format follows [Keep a Changelog](https://keepachangelog.com/) loosely; the
project uses descriptive one-line commit messages instead of semantic presets.

## [2.0] - 2026-11-01

The public history of this repository was consolidated on 2026-11-01 into
three descriptive commits (engine, website/documentation, build & CI fixes).
Earlier development revisions remain reachable through the `fafnir2` and
`1.3` tags; the older releases they once backed have been removed.

### Added
- Native multi-architecture build system in the root `Makefile`
  (`x86-64` safe default / `ssse3` / `sse4.2` / `avx2` / `avx512` / `native`),
  with per-architecture object directories and suffix-signed binaries
  (`bin/Fafnir-<arch>`).
- CI workflow `build-native.yml`: Linux and Windows builds for
  x86-64, SSE4.2 and AVX2 with a UCI smoke test, packaged as artifacts.
- GPL v3 `LICENSE` at the repository root and an overview of vendored code
  provenance in `third_party/VENDORED.md`.

### Changed
- UCI interface declares only implemented options (`Hash`, `Threads`,
  `MultiPV`, `Move Overhead`, `Aggressivity`); unrecognised options now answer
  `info string unknown UCI option`.
- Project renamed FlashBit -> Fafnir (repository, website, identity strings).
- The NNUE network is committed as a plain git blob again (no Git LFS), so
  clones and CI embed the real network without an LFS smudge step.
- `Makefile.wasm` and the CI build are portable across cmd.exe, msys2 sh,
  Linux and macOS shells (idempotent mkdir, no `2>nul`).

### Removed
- The unbuildable JNI bridge (`src/bridges/java_api.{h,cpp}`,
  `integrations/java-jni/`) — its legacy probe API does not compile against
  the vendored `sfdev` snapshot.
- Long dead code: legacy Stockfish `probe/` is no longer referenced from any
  build (documented in `third_party/VENDORED.md`); the CI no longer mutates
  the vendored `incbin.h` at build time (network embedding is disabled for
  WASM via `NNUE_EMBEDDING_OFF`).

### Fixed
- `build-wasm.yml` compiles again (broken since the source reorganisation on
  2026-10-31 picked up JNI/probe sources); the first artifact with the real
  embedded network was produced on 2026-11-01.
- Windows `mkdir` aborting on existing directories during native builds.

## Earlier revisions

Revisions before the consolidation above are tagged `fafnir2` (engine
snapshot release) and `1.3`; their published releases were removed, so these
tags are purely historical markers.
