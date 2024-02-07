#include "photon/core.h"

namespace photon::util {

/**
 * @defgroup MoveGenerators Functions to generate moves for each piece type.
 * Note that this module does not handle legality of moves, only the generation of moves.
 * For example, it does not check if a move puts the player in check, or the legality of
 * castling.
 * @{
 */

/**
 * @brief Generates all possible moves for a given player.
 *
 * This includes castling, but does not check if the moves result in the player being in check.
 *
 * @param board The chess board.
 * @param player The player for whom the moves are generated.
 * @return std::vector<move_t> A vector of all possible moves for the given player.
 */
std::vector<move_t> GenerateMoves(const board_t& board, player_t player);

/**
 * @brief Generates moves for a given piece.
 *
 * @param player The player for whom the moves are generated.
 * @param piece The piece for which the moves are generated.
 * @param board The chess board.
 * @param moves The vector to store the generated moves.
 */
void PieceMoves(player_t player, piece_t piece, board_t& board, std::vector<move_t>& moves);

/**
 * Generates pawn moves for a given set of pawns, including en passant and promotion.
 *
 * @param pawns The bitboard representing the pawns.
 * @param occupancy The bitboard representing the occupancy of all pieces.
 * @param enemyOccupancy The bitboard representing the occupancy of enemy pieces.
 * @param player The player for whom the moves are generated.
 * @param moves The vector to store the generated moves.
 */
void PawnMoves(bitboard_t pawns, bitboard_t occupancy, bitboard_t enemyOccupancy,
			   player_t player, std::optional<int> enPassant, std::vector<move_t>& moves);

/**
 * Generates knight moves for a given set of knights.
 *
 * @param knights The bitboard representing the knights.
 * @param playerOccupancy The bitboard representing the occupancy of player's pieces.
 * @param enemyOccupancy The bitboard representing the occupancy of enemy pieces.
 * @param moves The vector to store the generated moves.
 */
void KnightMoves(bitboard_t knights, bitboard_t playerOccupancy, bitboard_t enemyOccupancy,
				 std::vector<move_t>& moves);

/**
 * Generates bishop moves for a given set of bishops.
 *
 * @param bishops The bitboard representing the bishops.
 * @param playerOccupancy The bitboard representing the occupancy of player's pieces.
 * @param enemyOccupancy The bitboard representing the occupancy of enemy pieces.
 * @param moves The vector to store the generated moves.
 */
void BishopMoves(bitboard_t bishops, bitboard_t playerOccupancy, bitboard_t enemyOccupancy,
				 std::vector<move_t>& moves);

/**
 * Generates rook moves for a given set of rooks.
 *
 * @param rooks The bitboard representing the rooks.
 * @param playerOccupancy The bitboard representing the occupancy of player's pieces.
 * @param enemyOccupancy The bitboard representing the occupancy of enemy pieces.
 * @param moves The vector to store the generated moves.
 */
void RookMoves(bitboard_t rooks, bitboard_t playerOccupancy, bitboard_t enemyOccupancy,
			   std::vector<move_t>& moves);

/**
 * Generates queen moves for a given set of queens.
 *
 * @param queens The bitboard representing the queens.
 * @param playerOccupancy The bitboard representing the occupancy of player's pieces.
 * @param enemyOccupancy The bitboard representing the occupancy of enemy pieces.
 * @param moves The vector to store the generated moves.
 */
void QueenMoves(bitboard_t queens, bitboard_t playerOccupancy, bitboard_t enemyOccupancy,
				std::vector<move_t>& moves);

/**
 * Generates king moves for a given board and player. Does not include castling.
 *
 * @param board The chess board.
 * @param player The player for whom the moves are generated.
 * @param moves The vector to store the generated moves.
 */
void KingMoves(const board_t& board, player_t player, std::vector<move_t>& moves);

/** @} */

/**
 * @defgroup AttackMaskGenerators Functions to generate attacks for each piece type.
 * @{
 */

bitboard_t PieceAttackMoves(piece_t piece, bitboard_t pieceMask, player_t player,
							bitboard_t playerOccupancy, bitboard_t enemyOccupancy);

bitboard_t PawnAttackMask(bitboard_t pawns, bitboard_t enemyOccupancy, player_t player);

bitboard_t KnightAttackMask(bitboard_t knights, bitboard_t playerOccupancy);

bitboard_t BishopAttackMask(bitboard_t bishops, bitboard_t playerOccupancy,
							bitboard_t enemyOccupancy);

bitboard_t RookAttackMask(bitboard_t rooks, bitboard_t playerOccupancy,
						  bitboard_t enemyOccupancy);

bitboard_t QueenAttackMask(bitboard_t queens, bitboard_t playerOccupancy,
						   bitboard_t enemyOccupancy);

bitboard_t KingAttackMask(bitboard_t king, bitboard_t playerOccupancy);

/** @} */

} // namespace photon::util
