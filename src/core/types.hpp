// =============================================================================
//  types.hpp -- Fafnir Core (fase 2).
//
//  Costura (seam) entre Engine/UI y la biblioteca de reglas vendida.
//  Hoy el Core delega 1:1 en ChessLib (third_party/chesslib); el objetivo
//  es que search.hpp y la capa UI nunca dependan directamente de un
//  tercero, para poder sustituir la libreria de reglas si el dia lo piden.
// =============================================================================
#pragma once

#if __has_include("chesslib.hpp")
    #include "chesslib.hpp"
#else
    #include "third_party/chesslib/chesslib.hpp"
#endif

namespace fafnir::core {

using Board  = chess::Board;
using Move   = chess::Move;
using Piece  = chess::Piece;
using Color  = chess::Color;
using Square = chess::Square;

} // namespace fafnir::core
