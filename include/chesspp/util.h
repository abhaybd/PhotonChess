#pragma once

#include "chesspp/core.h"

namespace chesspp::util {

player_t OtherPlayer(player_t player);
bool CheckOccupancy(bitboard_t bitboard, int idx);

char PieceToChar(piece_t piece);
piece_t CharToPiece(char c);

uint8_t ParseSquare(std::string_view s);

} // namespace chesspp::util
