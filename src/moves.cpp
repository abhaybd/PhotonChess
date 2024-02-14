#include "moves.h"

#include "photon/util.h"

#include <loguru.hpp>
#include <strings.h>

namespace photon::util {
namespace {

constexpr bitboard_t FILE_A_MASK = 0x0101010101010101ULL;
constexpr bitboard_t FILE_B_MASK = 0x0202020202020202ULL;
constexpr bitboard_t FILE_G_MASK = 0x4040404040404040ULL;
constexpr bitboard_t FILE_H_MASK = 0x8080808080808080ULL;

constexpr bitboard_t RANK_1_MASK = 0x00000000000000FFULL;
constexpr bitboard_t RANK_2_MASK = 0x000000000000FF00ULL;
constexpr bitboard_t RANK_7_MASK = 0x00FF000000000000ULL;
constexpr bitboard_t RANK_8_MASK = 0xFF00000000000000ULL;

constexpr bitboard_t CASTLE_K_MASK_W = 0b01100000ULL;
constexpr bitboard_t CASTLE_K_MASK_B = CASTLE_K_MASK_W << 56;
constexpr bitboard_t CASTLE_Q_MASK_W = 0b00001110ULL;
constexpr bitboard_t CASTLE_Q_MASK_B = CASTLE_Q_MASK_W << 56;

template <typename T>
T shift(T x, int shift) {
	if (shift >= 0) {
		return x << shift;
	} else {
		return x >> -shift;
	}
}

void addMoves(bitboard_t bb, int offset, bool isCapture, std::vector<move_t>& moves) {
	while (bb != 0) {
		int idx = ffsll(bb) - 1;
		DCHECK_F(idx >= 0);
		uint8_t from = idx + offset;
		uint8_t to = idx;
		moves.push_back({from, to, -1, isCapture});
		bb &= ~(1ULL << idx);
	}
}

void addMoves(bitboard_t bb, int offset, bitboard_t enemyOccupancy,
			  std::vector<move_t>& moves) {
	addMoves(bb & enemyOccupancy, offset, true, moves);
	addMoves(bb & ~enemyOccupancy, offset, false, moves);
}

void addPromotions(bitboard_t bb, int offset, bool isCapture, std::vector<move_t>& moves) {
	while (bb != 0) {
		int idx = ffsll(bb) - 1;
		DCHECK_F(idx >= 0);
		uint8_t from = idx + offset;
		uint8_t to = idx;
		for (piece_t p : {piece_t::queen, piece_t::rook, piece_t::bishop, piece_t::knight}) {
			moves.push_back({from, to, static_cast<int8_t>(p), isCapture});
		}
		bb &= ~(1ULL << idx);
	}
}

uint8_t operator""_uc(unsigned long long int x) {
	return static_cast<uint8_t>(x);
}

bool IsAnyAttacked(const board_t& board, bitboard_t bb, player_t player) {
	while (bb != 0) {
		int idx = ffsll(bb) - 1;
		if (board.isSquareAttacked(player, idx)) {
			return true;
		}
		bb &= ~(1ULL << idx);
	}
	return false;
}

} // namespace

std::vector<move_t> GenerateMoves(const board_t& board, player_t player) {
	bitboard_t playerOccupancy = board.occupancyMap(player);
	bitboard_t enemyOccupancy = board.occupancyMap(OtherPlayer(player));
	bitboard_t occupancy = playerOccupancy | enemyOccupancy;

	std::vector<move_t> moves;
	PawnMoves(board.getBitboard(player, piece_t::pawn), occupancy, enemyOccupancy, player,
			  board.availableEnPassant(), moves);
	KnightMoves(board.getBitboard(player, piece_t::knight), playerOccupancy, enemyOccupancy,
				moves);
	BishopMoves(board.getBitboard(player, piece_t::bishop), playerOccupancy, enemyOccupancy,
				moves);
	RookMoves(board.getBitboard(player, piece_t::rook), playerOccupancy, enemyOccupancy,
			  moves);
	QueenMoves(board.getBitboard(player, piece_t::queen), playerOccupancy, enemyOccupancy,
			   moves);
	KingMoves(board, player, moves);

	return moves;
}

void PieceMoves(player_t player, piece_t piece, board_t& board, std::vector<move_t>& moves) {
	bitboard_t playerOccupancy = board.occupancyMap(player);
	bitboard_t enemyOccupancy = board.occupancyMap(OtherPlayer(player));
	switch (piece) {
		case piece_t::pawn:
			PawnMoves(board.getBitboard(player, piece), board.occupancyMap(), enemyOccupancy,
					  player, board.availableEnPassant(), moves);
			break;
		case piece_t::knight:
			KnightMoves(board.getBitboard(player, piece), playerOccupancy, enemyOccupancy,
						moves);
			break;
		case piece_t::bishop:
			BishopMoves(board.getBitboard(player, piece), playerOccupancy, enemyOccupancy,
						moves);
			break;
		case piece_t::rook:
			RookMoves(board.getBitboard(player, piece), playerOccupancy, enemyOccupancy,
					  moves);
			break;
		case piece_t::queen:
			QueenMoves(board.getBitboard(player, piece), playerOccupancy, enemyOccupancy,
					   moves);
			break;
		case piece_t::king:
			KingMoves(board, player, moves);
			break;
		default:
			CHECK_F(false);
	}
}

void PawnMoves(bitboard_t pawns, bitboard_t occupancy, bitboard_t enemyOccupancy,
			   player_t player, std::optional<int> enPassant, std::vector<move_t>& moves) {
	bitboard_t startRank = player == player_t::white ? RANK_2_MASK : RANK_7_MASK;
	bitboard_t promotionRank = player == player_t::white ? RANK_8_MASK : RANK_1_MASK;

	int sign = player == player_t::white ? 1 : -1;
	bitboard_t forward1 = shift(pawns, 8 * sign) & ~occupancy;
	addMoves(forward1 & ~promotionRank, -8 * sign, false, moves);
	bitboard_t forward2 =
		shift(pawns & startRank, 16 * sign) & ~occupancy & ~shift(occupancy, 8 * sign);
	addMoves(forward2, -16 * sign, false, moves);
	bitboard_t queensideCapture =
		shift(pawns & ~FILE_A_MASK, (8 - sign) * sign) & enemyOccupancy;
	addMoves(queensideCapture & ~promotionRank, -(8 - sign) * sign, true, moves);
	bitboard_t kingsideCapture =
		shift(pawns & ~FILE_H_MASK, (8 + sign) * sign) & enemyOccupancy;
	addMoves(kingsideCapture & ~promotionRank, -(8 + sign) * sign, true, moves);

	if (enPassant) {
		bitboard_t enPassantMask = 1ULL << *enPassant;
		bitboard_t enPassantQ = shift(pawns & ~FILE_A_MASK, (8 - sign) * sign) & enPassantMask;
		addMoves(enPassantQ, -(8 - sign) * sign, true, moves);
		bitboard_t enPassantK = shift(pawns & ~FILE_H_MASK, (8 + sign) * sign) & enPassantMask;
		addMoves(enPassantK, -(8 + sign) * sign, true, moves);
	}

	addPromotions(forward1 & promotionRank, -8 * sign, false, moves);
	addPromotions(queensideCapture & promotionRank, -(8 - sign) * sign, true, moves);
	addPromotions(kingsideCapture & promotionRank, -(8 + sign) * sign, true, moves);
}

void KnightMoves(bitboard_t knights, bitboard_t playerOccupancy, bitboard_t enemyOccupancy,
				 std::vector<move_t>& moves) {
	// left-front, front-left, front-right, right-front, etc.
	bitboard_t lf = shift(knights & ~FILE_A_MASK & ~FILE_B_MASK, 6) & ~playerOccupancy;
	addMoves(lf, -6, enemyOccupancy, moves);
	bitboard_t fl = shift(knights & ~FILE_A_MASK, 15) & ~playerOccupancy;
	addMoves(fl, -15, enemyOccupancy, moves);
	bitboard_t fr = shift(knights & ~FILE_H_MASK, 17) & ~playerOccupancy;
	addMoves(fr, -17, enemyOccupancy, moves);
	bitboard_t rf = shift(knights & ~FILE_H_MASK & ~FILE_G_MASK, 10) & ~playerOccupancy;
	addMoves(rf, -10, enemyOccupancy, moves);
	bitboard_t lb = shift(knights & ~FILE_A_MASK & ~FILE_B_MASK, -10) & ~playerOccupancy;
	addMoves(lb, 10, enemyOccupancy, moves);
	bitboard_t bl = shift(knights & ~FILE_A_MASK, -17) & ~playerOccupancy;
	addMoves(bl, 17, enemyOccupancy, moves);
	bitboard_t br = shift(knights & ~FILE_H_MASK, -15) & ~playerOccupancy;
	addMoves(br, 15, enemyOccupancy, moves);
	bitboard_t rb = shift(knights & ~FILE_H_MASK & ~FILE_G_MASK, -6) & ~playerOccupancy;
	addMoves(rb, 6, enemyOccupancy, moves);
}

void BishopMoves(bitboard_t bishops, bitboard_t playerOccupancy, bitboard_t enemyOccupancy,
				 std::vector<move_t>& moves) {
	bitboard_t bb = bishops;
	for (int i = 1; i < 8 && bb != 0; i++) {
		bb = shift(bb & ~FILE_A_MASK, 7) & ~playerOccupancy;
		addMoves(bb, -7 * i, enemyOccupancy, moves);
		bb &= ~enemyOccupancy;
	}

	bb = bishops;
	for (int i = 1; i < 8 && bb != 0; i++) {
		bb = shift(bb & ~FILE_H_MASK, 9) & ~playerOccupancy;
		addMoves(bb, -9 * i, enemyOccupancy, moves);
		bb &= ~enemyOccupancy;
	}

	bb = bishops;
	for (int i = 1; i < 8 && bb != 0; i++) {
		bb = shift(bb & ~FILE_A_MASK, -9) & ~playerOccupancy;
		addMoves(bb, 9 * i, enemyOccupancy, moves);
		bb &= ~enemyOccupancy;
	}

	bb = bishops;
	for (int i = 1; i < 8 && bb != 0; i++) {
		bb = shift(bb & ~FILE_H_MASK, -7) & ~playerOccupancy;
		addMoves(bb, 7 * i, enemyOccupancy, moves);
		bb &= ~enemyOccupancy;
	}
}

void RookMoves(bitboard_t rooks, bitboard_t playerOccupancy, bitboard_t enemyOccupancy,
			   std::vector<move_t>& moves) {
	bitboard_t bb = rooks;
	for (int i = 1; i < 8 && bb != 0; i++) {
		bb = shift(bb, 8) & ~playerOccupancy;
		addMoves(bb, -8 * i, enemyOccupancy, moves);
		bb &= ~enemyOccupancy;
	}

	bb = rooks;
	for (int i = 1; i < 8 && bb != 0; i++) {
		bb = shift(bb & ~FILE_H_MASK, 1) & ~playerOccupancy;
		addMoves(bb, -1 * i, enemyOccupancy, moves);
		bb &= ~enemyOccupancy;
	}

	bb = rooks;
	for (int i = 1; i < 8 && bb != 0; i++) {
		bb = shift(bb, -8) & ~playerOccupancy;
		addMoves(bb, 8 * i, enemyOccupancy, moves);
		bb &= ~enemyOccupancy;
	}

	bb = rooks;
	for (int i = 1; i < 8 && bb != 0; i++) {
		bb = shift(bb & ~FILE_A_MASK, -1) & ~playerOccupancy;
		addMoves(bb, 1 * i, enemyOccupancy, moves);
		bb &= ~enemyOccupancy;
	}
}

void QueenMoves(bitboard_t queens, bitboard_t playerOccupancy, bitboard_t enemyOccupancy,
				std::vector<move_t>& moves) {
	BishopMoves(queens, playerOccupancy, enemyOccupancy, moves);
	RookMoves(queens, playerOccupancy, enemyOccupancy, moves);
}

void KingMoves(const board_t& board, player_t player, std::vector<move_t>& moves) {
	bitboard_t king = board.getBitboard(player, piece_t::king);
	bitboard_t playerOccupancy = board.occupancyMap(player);
	bitboard_t enemyOccupancy = board.occupancyMap(OtherPlayer(player));
	DCHECK_F(king != 0 && king == (king & -king)); // only one king

	addMoves(shift(king, 8) & ~playerOccupancy, -8, enemyOccupancy, moves);
	addMoves(shift(king, -8) & ~playerOccupancy, 8, enemyOccupancy, moves);
	addMoves(shift(king & ~FILE_A_MASK, 7) & ~playerOccupancy, -7, enemyOccupancy, moves);
	addMoves(shift(king & ~FILE_H_MASK, 9) & ~playerOccupancy, -9, enemyOccupancy, moves);
	addMoves(shift(king & ~FILE_H_MASK, 1) & ~playerOccupancy, -1, enemyOccupancy, moves);
	addMoves(shift(king & ~FILE_A_MASK, -1) & ~playerOccupancy, 1, enemyOccupancy, moves);
	addMoves(shift(king & ~FILE_H_MASK, -7) & ~playerOccupancy, 7, enemyOccupancy, moves);
	addMoves(shift(king & ~FILE_A_MASK, -9) & ~playerOccupancy, 9, enemyOccupancy, moves);

	player_t otherPlayer = OtherPlayer(player);
	uint8_t from = ffsll(king) - 1;
	if (!board.isSquareAttacked(otherPlayer, from)) {
		bitboard_t occupancy = board.occupancyMap();
		bitboard_t castleKMask = player == player_t::white ? CASTLE_K_MASK_W : CASTLE_K_MASK_B;
		bitboard_t castleQMask = player == player_t::white ? CASTLE_Q_MASK_W : CASTLE_Q_MASK_B;
		if (board.hasCastlingRights(player, castle_t::king) &&
			(occupancy & castleKMask) == 0) {
			DCHECK_F(from == (player == player_t::white ? 4 : 60));
			DCHECK_F(CheckOccupancy(board.getBitboard(player, piece_t::rook),
									player == player_t::white ? 7 : 63));
			if (!IsAnyAttacked(board, castleKMask, otherPlayer)) {
				moves.push_back(move_t{from, player == player_t::white ? 6_uc : 62_uc});
			}
		}
		if (board.hasCastlingRights(player, castle_t::queen) &&
			(occupancy & castleQMask) == 0) {
			DCHECK_F(from == (player == player_t::white ? 4 : 60));
			DCHECK_F(CheckOccupancy(board.getBitboard(player, piece_t::rook),
									player == player_t::white ? 0 : 56));
			if (!IsAnyAttacked(board, castleQMask, otherPlayer)) {
				moves.push_back(move_t{from, player == player_t::white ? 2_uc : 58_uc});
			}
		}
	}
}

bitboard_t PieceAttackMoves(piece_t piece, bitboard_t pieceMask, player_t player,
							bitboard_t playerOccupancy, bitboard_t enemyOccupancy) {
	switch (piece) {
		case piece_t::pawn:
			return PawnAttackMask(pieceMask, enemyOccupancy, player);
		case piece_t::knight:
			return KnightAttackMask(pieceMask, playerOccupancy);
		case piece_t::bishop:
			return BishopAttackMask(pieceMask, playerOccupancy, enemyOccupancy);
		case piece_t::rook:
			return RookAttackMask(pieceMask, playerOccupancy, enemyOccupancy);
		case piece_t::queen:
			return QueenAttackMask(pieceMask, playerOccupancy, enemyOccupancy);
		case piece_t::king:
			return KingAttackMask(pieceMask, playerOccupancy);
		default:
			CHECK_F(false);
	}
}

bitboard_t PawnAttackMask(bitboard_t pawns, bitboard_t enemyOccupancy, player_t player) {
	int sign = player == player_t::white ? 1 : -1;
	bitboard_t queensideCapture =
		shift(pawns & ~FILE_A_MASK, (8 - sign) * sign) & enemyOccupancy;
	bitboard_t kingsideCapture =
		shift(pawns & ~FILE_H_MASK, (8 + sign) * sign) & enemyOccupancy;
	return queensideCapture | kingsideCapture;
}

bitboard_t KnightAttackMask(bitboard_t knights, bitboard_t playerOccupancy) {
	bitboard_t lf = shift(knights & ~FILE_A_MASK & ~FILE_B_MASK, 6) & ~playerOccupancy;
	bitboard_t fl = shift(knights & ~FILE_A_MASK, 15) & ~playerOccupancy;
	bitboard_t fr = shift(knights & ~FILE_H_MASK, 17) & ~playerOccupancy;
	bitboard_t rf = shift(knights & ~FILE_H_MASK & ~FILE_G_MASK, 10) & ~playerOccupancy;
	bitboard_t lb = shift(knights & ~FILE_A_MASK & ~FILE_B_MASK, -10) & ~playerOccupancy;
	bitboard_t bl = shift(knights & ~FILE_A_MASK, -17) & ~playerOccupancy;
	bitboard_t br = shift(knights & ~FILE_H_MASK, -15) & ~playerOccupancy;
	bitboard_t rb = shift(knights & ~FILE_H_MASK & ~FILE_G_MASK, -6) & ~playerOccupancy;
	return lf | fl | fr | rf | lb | bl | br | rb;
}

bitboard_t BishopAttackMask(bitboard_t bishops, bitboard_t playerOccupancy,
							bitboard_t enemyOccupancy) {
	bitboard_t bb = bishops;
	bitboard_t mask = 0;
	for (int i = 1; i < 8 && bb != 0; i++) {
		bb = shift(bb & ~FILE_A_MASK, 7) & ~playerOccupancy;
		mask |= bb;
		bb &= ~enemyOccupancy;
	}

	bb = bishops;
	for (int i = 1; i < 8 && bb != 0; i++) {
		bb = shift(bb & ~FILE_H_MASK, 9) & ~playerOccupancy;
		mask |= bb;
		bb &= ~enemyOccupancy;
	}

	bb = bishops;
	for (int i = 1; i < 8 && bb != 0; i++) {
		bb = shift(bb & ~FILE_A_MASK, -9) & ~playerOccupancy;
		mask |= bb;
		bb &= ~enemyOccupancy;
	}

	bb = bishops;
	for (int i = 1; i < 8 && bb != 0; i++) {
		bb = shift(bb & ~FILE_H_MASK, -7) & ~playerOccupancy;
		mask |= bb;
		bb &= ~enemyOccupancy;
	}
	return mask;
}

bitboard_t RookAttackMask(bitboard_t rooks, bitboard_t playerOccupancy,
						  bitboard_t enemyOccupancy) {
	bitboard_t bb = rooks;
	bitboard_t mask = 0;
	for (int i = 1; i < 8 && bb != 0; i++) {
		bb = shift(bb, 8) & ~playerOccupancy;
		mask |= bb;
		bb &= ~enemyOccupancy;
	}

	bb = rooks;
	for (int i = 1; i < 8 && bb != 0; i++) {
		bb = shift(bb & ~FILE_H_MASK, 1) & ~playerOccupancy;
		mask |= bb;
		bb &= ~enemyOccupancy;
	}

	bb = rooks;
	for (int i = 1; i < 8 && bb != 0; i++) {
		bb = shift(bb, -8) & ~playerOccupancy;
		mask |= bb;
		bb &= ~enemyOccupancy;
	}

	bb = rooks;
	for (int i = 1; i < 8 && bb != 0; i++) {
		bb = shift(bb & ~FILE_A_MASK, -1) & ~playerOccupancy;
		mask |= bb;
		bb &= ~enemyOccupancy;
	}
	return mask;
}

bitboard_t QueenAttackMask(bitboard_t queens, bitboard_t playerOccupancy,
						   bitboard_t enemyOccupancy) {
	return BishopAttackMask(queens, playerOccupancy, enemyOccupancy) |
		   RookAttackMask(queens, playerOccupancy, enemyOccupancy);
}

bitboard_t KingAttackMask(bitboard_t king, bitboard_t playerOccupancy) {
	bitboard_t mask = 0;
	mask |= shift(king, 8) & ~playerOccupancy;
	mask |= shift(king, -8) & ~playerOccupancy;
	mask |= shift(king & ~FILE_A_MASK, 7) & ~playerOccupancy;
	mask |= shift(king & ~FILE_H_MASK, 9) & ~playerOccupancy;
	mask |= shift(king & ~FILE_H_MASK, 1) & ~playerOccupancy;
	mask |= shift(king & ~FILE_A_MASK, -1) & ~playerOccupancy;
	mask |= shift(king & ~FILE_H_MASK, -7) & ~playerOccupancy;
	mask |= shift(king & ~FILE_A_MASK, -9) & ~playerOccupancy;
	return mask;
}

} // namespace photon::util
