#include "photon/util.h"

#include <assert.h>
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

template <typename T>
T shift(T x, int shift) {
	if (shift >= 0) {
		return x << shift;
	} else {
		return x >> -shift;
	}
}

void addMoves(bitboard_t bb, int offset, std::vector<move_t>& moves) {
	while (bb != 0) {
		int idx = ffsll(bb) - 1;
		assert(idx >= 0);
		uint8_t from = idx + offset;
		uint8_t to = idx;
		moves.push_back({from, to});
		bb &= ~(1ULL << idx);
	}
}
} // namespace

void PawnMoves(const board_t& board, player_t player, std::vector<move_t>& moves) {
	bitboard_t pawns = board.getBitboard(player, piece_t::pawn);
	bitboard_t occupancy = board.occupancyMap();
	bitboard_t enemyOccupancy = board.occupancyMap(OtherPlayer(player));

	bitboard_t startRank = player == player_t::white ? RANK_2_MASK : RANK_7_MASK;
	bitboard_t promotionRank = player == player_t::white ? RANK_8_MASK : RANK_1_MASK;

	int sign = player == player_t::white ? 1 : -1;
	bitboard_t forward1 = shift(pawns, 8 * sign) & ~occupancy & ~promotionRank;
	bitboard_t forward2 =
		shift(pawns & startRank, 16 * sign) & ~occupancy & ~shift(occupancy, 8 * sign);
	bitboard_t queensideCapture =
		shift(pawns & ~FILE_A_MASK, (8 - sign) * sign) & enemyOccupancy & ~promotionRank;
	bitboard_t kingsideCapture =
		shift(pawns & ~FILE_H_MASK, (8 + sign) * sign) & enemyOccupancy & ~promotionRank;

	// TODO add en passant
	// TODO add promotion (both by moving and by capturing)

	addMoves(forward1, -8 * sign, moves);
	addMoves(forward2, -16 * sign, moves);
	addMoves(queensideCapture, -(8 - sign) * sign, moves);
	addMoves(kingsideCapture, -(8 + sign) * sign, moves);
}

void KnightMoves(const board_t& board, player_t player, std::vector<move_t>& moves) {
	bitboard_t knights = board.getBitboard(player, piece_t::knight);
	bitboard_t playerOccupancy = board.occupancyMap(player);

	// left-front, front-left, front-right, right-front, etc.
	bitboard_t lf = shift(knights & ~FILE_A_MASK & ~FILE_B_MASK, 6) & ~playerOccupancy;
	addMoves(lf, -6, moves);
	bitboard_t fl = shift(knights & ~FILE_A_MASK, 15) & ~playerOccupancy;
	addMoves(fl, -15, moves);
	bitboard_t fr = shift(knights & ~FILE_H_MASK, 17) & ~playerOccupancy;
	addMoves(fr, -17, moves);
	bitboard_t rf = shift(knights & ~FILE_H_MASK & ~FILE_G_MASK, 10) & ~playerOccupancy;
	addMoves(rf, -10, moves);
	bitboard_t lb = shift(knights & ~FILE_A_MASK & ~FILE_B_MASK, -10) & ~playerOccupancy;
	addMoves(lb, 10, moves);
	bitboard_t bl = shift(knights & ~FILE_A_MASK, -17) & ~playerOccupancy;
	addMoves(bl, 17, moves);
	bitboard_t br = shift(knights & ~FILE_H_MASK, -15) & ~playerOccupancy;
	addMoves(br, 15, moves);
	bitboard_t rb = shift(knights & ~FILE_H_MASK & ~FILE_G_MASK, -6) & ~playerOccupancy;
	addMoves(rb, 6, moves);
}

void BishopMoves(const board_t& board, player_t player, std::vector<move_t>& moves) {
	bitboard_t bishops = board.getBitboard(player, piece_t::bishop);
	bitboard_t playerOccupancy = board.occupancyMap(player);
	bitboard_t enemyOccupancy = board.occupancyMap(OtherPlayer(player));

	bitboard_t bb = bishops;
	for (int i = 1; i < 8 && bb != 0; i++) {
		bb = shift(bb & ~FILE_A_MASK, 7) & ~playerOccupancy;
		addMoves(bb, -7 * i, moves);
		bb &= ~enemyOccupancy;
	}

	bb = bishops;
	for (int i = 1; i < 8 && bb != 0; i++) {
		bb = shift(bb & ~FILE_H_MASK, 9) & ~playerOccupancy;
		addMoves(bb, -9 * i, moves);
		bb &= ~enemyOccupancy;
	}

	bb = bishops;
	for (int i = 1; i < 8 && bb != 0; i++) {
		bb = shift(bb & ~FILE_A_MASK, -9) & ~playerOccupancy;
		addMoves(bb, 9 * i, moves);
		bb &= ~enemyOccupancy;
	}

	bb = bishops;
	for (int i = 1; i < 8 && bb != 0; i++) {
		bb = shift(bb & ~FILE_H_MASK, -7) & ~playerOccupancy;
		addMoves(bb, 7 * i, moves);
		bb &= ~enemyOccupancy;
	}
}

} // namespace photon::util
