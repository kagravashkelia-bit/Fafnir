# Security Policy

## Supported versions

Fafnir is a single-line C++17 project; the current `main` branch (and its
latest tagged release) is the only supported configuration.

## Reporting a vulnerability

Please report security issues privately through GitHub
[Security Advisories](https://github.com/kagravashkelia-bit/Fafnir/security/advisories/new)
for this repository, or open a GitHub issue without reproducing details if a
private channel is not possible.

When reporting, include:

- the architecture you built for (`make help` lists the accepted values),
- compiler and operating system,
- the exact UCI transcript that triggers the issue.

## Scope

Relevant areas: the UCI protocol parser (`src/ui/uci.cpp`), FEN parsing inside
the vendored position core (`third_party/stockfish/sfdev/src`), file loading
of the embedded/open NNUE network, and the search stack limits. Vendored
upstream code is tracked as-is; provenance and versions are documented in
`third_party/VENDORED.md`.

No fuzzing or sanitiser runs are currently integrated into the CI; that is
acknowledged openly here rather than claimed.
