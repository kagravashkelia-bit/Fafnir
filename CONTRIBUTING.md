# Contributing to Fafnir

Thanks for considering a contribution. Fafnir is built around a few strict
rules; please keep them in mind before opening a pull request.

## Build

A C++17 toolchain is required (g++; clang also accepted via `CXX=clang++`).
On Windows, MSYS2 MinGW64 (`pacman -S mingw-w64-x86_64-toolchain make`) works
out of the box; GNU make from a POSIX shell or from cmd.exe are both supported
by the Makefile.

```sh
make                      # safe SSE2 build -> bin/Fafnir(.exe)
make ARCH=x86-64-avx2     # AVX2 + BMI2 (x86-64-v3)
make all-arches           # every architecture, suffixed binaries in bin/
```

## Before you open a PR

1. **Smoke test the UCI protocol** (the CI does the same):

   `printf 'uci\nisready\nquit\n' | ./bin/Fafnir` must print `uciok` and
   `readyok`.

2. **Run the EPD regression suite** (`python tests/tests.py bin/Fafnir --depth 18`).
   Search-behaviour regressions are considered blocking.

3. **Keep the build green.** Do not commit to `main` directly if
   `build-native.yml`, `build-wasm.yml` or `deploy-pages.yml` are failing.

## House rules

- **Vendored code is frozen.** `third_party/` is not to be modified except for
  a patch that is documented in `third_party/VENDORED.md` (currently exactly
  one patch: the embedded-network location in `sfdev/src/evaluate.h`).
- **Declare only what you implement.** New UCI options must be implemented;
  the engine must never advertise non-functional options.
- **English commit messages**, one line of 72 columns or less, imperative
  mood, optionally followed by a body explaining the *why*.
- Layer discipline: `src/ui` may not call into `src/engine` internals, and the
  engine may not know about protocol layers. Bridges live in `src/bridges`.

## Report a bug

Open a GitHub issue with the UCI command transcript, the exact `position` and
`go` line, and your compiler/architecture (`make help` prints the valid
architectures). Security-sensitive reports go through
`SECURITY.md`.
