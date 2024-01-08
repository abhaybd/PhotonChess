#include "util.h"

#include <loguru.hpp>

namespace chesspp::util {

player_t OtherPlayer(player_t player) {
	if (player == player_t::white) {
		return player_t::black;
	} else {
		return player_t::white;
	}
}

bool CheckOccupancy(bitboard_t bitboard, int idx) {
	return (bitboard & (1 << idx)) != 0;
}

} // namespace chesspp::util
