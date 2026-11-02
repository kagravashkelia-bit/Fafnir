# Código vendido (third_party)

Este directorio contiene código de terceros integrado tal cual (o con
parches mínimos documentados). **No se modifica salvo parche documentado.**

## ChessLib (Disservin chess-library)

- Archivo: `chesslib/chesslib.hpp`
- Versión: 0.9.4 (auto-generado, "do not change manually")
- Licencia: MIT (cabecera del propio archivo, copyright 2023 disservin)
- Renovarlo: descargar `chess.hpp` del upstream y reemplazar.

## Stockfish dev (snapshot SFNNv17)

- Directorio: `stockfish/sfdev/` — snapshot del desarrollo de Stockfish en el
  commit `49ea5ded38315cff8e67f4a677a9e7811612fbf6` (fonte citada en
  `stockfish/probe/README.md`).
- Licencia: GPLv3 (`stockfish/probe/LICENSE`, `sfdev/Copying.txt`,
  `sfdev/AUTHORS`).
- Solo se compila el subconjunto requerido por el probe NNUE (ver `Makefile`).
- **No se compila:** `stockfish/probe/` (API legacy de probe FEN→eval para un
  snapshot anterior de Stockfish; no compila contra el snapshot actual y no
  forma parte de ningún binario de este repo: nativo, WASM ni JNI).

### Parche local documentado (único sobre el vendido)

- `sfdev/src/evaluate.h`: `EvalFileDefaultName` apunta a la red en su nueva
  ubicación del repo: `assets/nnue/nn-252f33942263.nnue` (en el upstream era
  solo el nombre de archivo en la raíz). Actualizar este parche al renovar
  el snapshot.

## Red NNUE

- Archivo: `assets/nnue/nn-252f33942263.nnue`
- Red por defecto del snapshot SFNNv17 (una sola salida, sin PSQT).
- SHA-256: `252f33942263bc8b8f740ba8aec3fed5a159ff148113c47a55c18c33d6627ab3`

## incbin

- Archivo: `incbin/incbin.h` (Dale Weiler)
- Licencia: Unlicense (`incbin/UNLICENCE`)
