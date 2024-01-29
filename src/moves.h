#include "photon/core.h"

namespace photon::util {

void PawnMoves(bitboard_t pawns, bitboard_t occupancy, bitboard_t enemyOccupancy,
			   player_t player, std::vector<move_t>& moves);
void KnightMoves(bitboard_t knights, bitboard_t playerOccupancy, std::vector<move_t>& moves);
void BishopMoves(bitboard_t bishops, bitboard_t playerOccupancy, bitboard_t enemyOccupancy,
				 std::vector<move_t>& moves);
void RookMoves(bitboard_t rooks, bitboard_t playerOccupancy, bitboard_t enemyOccupancy,
			   std::vector<move_t>& moves);
void QueenMoves(bitboard_t queens, bitboard_t playerOccupancy, bitboard_t enemyOccupancy,
				std::vector<move_t>& moves);
void KingMoves(const board_t& board, player_t player, std::vector<move_t>& moves);

} // namespace photon::util
