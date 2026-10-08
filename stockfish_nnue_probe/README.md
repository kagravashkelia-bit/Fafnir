# Stockfish development SFNNv17 probe

FlashBit uses the official Stockfish development source snapshot at commit
`49ea5ded38315cff8e67f4a677a9e7811612fbf6`, vendored under [`sfdev/src`](./sfdev/src).
The corresponding upstream `AUTHORS` file and GPLv3 license are included in
[`sfdev`](./sfdev).

The probe uses the development branch's single-output NNUE architecture: the
network returns one scalar evaluation and has no separate PSQT output. The
matching default network is `nn-252f33942263.nnue`; it is embedded in native
and WASM executables and is not opened from disk at runtime.

Network SHA-256:
`252f33942263bc8b8f740ba8aec3fed5a159ff148113c47a55c18c33d6627ab3`.

The native `Makefile` and `Makefile.wasm` compile only the Stockfish source
files required by the probe. The former SF19 sources are retained in
[`sf19`](./sf19) but are no longer compiled or used by FlashBit.
