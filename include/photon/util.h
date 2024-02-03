#pragma once

#include "photon/core.h"

#include <vector>

namespace photon::util {

player_t OtherPlayer(player_t player);
bool CheckOccupancy(bitboard_t bitboard, int idx);

char PieceToChar(piece_t piece);
piece_t CharToPiece(char c);

uint8_t ParseSquare(std::string_view s);
std::string SquareToString(uint8_t square);

board_t MakeBoard(std::string_view fen);
board_t DefaultBoard();

move_t MoveFromLongNotation(player_t player, std::string_view longNotation);
move_t MoveFromUCI(std::string_view uci);

std::string MoveToLongNotation(board_t board, move_t move);
std::string MoveToUCI(move_t move);

} // namespace photon::util
