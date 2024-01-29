#include "chesspp/util.h"

#include <assert.h>
#include <strings.h>

namespace chesspp::util {
namespace {

constexpr bitboard_t FILE_A_MASK = 0x0101010101010101L;
constexpr bitboard_t FILE_H_MASK = 0x8080808080808080L;
constexpr bitboard_t RANK_1_MASK = 0x00000000000000FFL;
constexpr bitboard_t RANK_2_MASK = 0x000000000000FF00L;
constexpr bitboard_t RANK_7_MASK = 0x00FF000000000000L;
constexpr bitboard_t RANK_8_MASK = 0xFF00000000000000L;

template <typename T>
T shift(T x, int shift) {
	if (shift >= 0) {
		return x << shift;
	} else {
		return x >> -shift;
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

	while (forward1 != 0) {
		int idx = ffsll(forward1) - 1;
		assert(idx >= 0);
		uint8_t from = idx - 8 * sign;
		uint8_t to = idx;
		moves.push_back({from, to});
		forward1 &= ~(1L << idx);
	}

	while (forward2 != 0) {
		int idx = ffsll(forward2) - 1;
		assert(idx >= 0);
		uint8_t from = idx - 16 * sign;
		uint8_t to = idx;
		moves.push_back({from, to});
		forward2 &= ~(1L << idx);
	}

	while (queensideCapture != 0) {
		int idx = ffsll(queensideCapture) - 1;
		assert(idx >= 0);
		uint8_t from = idx - (8 - sign) * sign;
		uint8_t to = idx;
		moves.push_back({from, to});
		queensideCapture &= ~(1L << idx);
	}

	while (kingsideCapture != 0) {
		int idx = ffsll(kingsideCapture) - 1;
		assert(idx >= 0);
		uint8_t from = idx - (8 + sign) * sign;
		uint8_t to = idx;
		moves.push_back({from, to});
		kingsideCapture &= ~(1L << idx);
	}
}

} // namespace chesspp::util
