#include "photon/core.h"
#include "photon/util.h"

#include <catch2/catch.hpp>

using namespace photon;
using namespace photon::util;


TEST_CASE("Test PawnMoves on default board", "[util][moves]") {
	board_t board = DefaultBoard();

	std::vector<move_t> whiteMoves;
	PawnMoves(board, player_t::white, whiteMoves);
	REQUIRE(whiteMoves.size() == 16);
	for (uint8_t i = 8; i < 16; ++i) {
		move_t move1 = {i, static_cast<uint8_t>(i + 8)};
		move_t move2 = {i, static_cast<uint8_t>(i + 16)};
		REQUIRE(std::find(whiteMoves.begin(), whiteMoves.end(), move1) != whiteMoves.end());
		REQUIRE(std::find(whiteMoves.begin(), whiteMoves.end(), move2) != whiteMoves.end());
	}

	std::vector<move_t> blackMoves;
	PawnMoves(board, player_t::black, blackMoves);
	REQUIRE(blackMoves.size() == 16);
	for (uint8_t i = 48; i < 56; ++i) {
		move_t move1 = {i, static_cast<uint8_t>(i - 8)};
		move_t move2 = {i, static_cast<uint8_t>(i - 16)};
		REQUIRE(std::find(blackMoves.begin(), blackMoves.end(), move1) != blackMoves.end());
		REQUIRE(std::find(blackMoves.begin(), blackMoves.end(), move2) != blackMoves.end());
	}
}
