#pragma once

#include "chesspp/core.h"

#include <vector>

namespace chesspp::util {

player_t OtherPlayer(player_t player);
bool CheckOccupancy(bitboard_t bitboard, int idx);

char PieceToChar(piece_t piece);
piece_t CharToPiece(char c);

uint8_t ParseSquare(std::string_view s);

void PawnMoves(const board_t& board, player_t player, std::vector<move_t>& moves);
void KnightMoves(const board_t& board, player_t player, std::vector<move_t>& moves);
void BishopMoves(const board_t& board, player_t player, std::vector<move_t>& moves);
void RookMoves(const board_t& board, player_t player, std::vector<move_t>& moves);
void QueenMoves(const board_t& board, player_t player, std::vector<move_t>& moves);
void KingMoves(const board_t& board, player_t player, std::vector<move_t>& moves);

} // namespace chesspp::util
