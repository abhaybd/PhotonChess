#include "../src/engine/search/lmr.h"

#include "photon/util.h"

#include <catch2/catch_test_macros.hpp>

using namespace photon;
using namespace photon::util;
using namespace photon::engine;

TEST_CASE("LMR", "[engine][lmr]") {
	lmr_t lmr(3, 2, 1);
	board_t board = DefaultBoard();
	move_t move = MoveFromUCI(board, "e2e4");

	SECTION("Standard cases") {
		REQUIRE(lmr.shouldReduce(board, 3, 2, false, move));
		REQUIRE(lmr.getReduction(board, 3, 2, false, move) == 1);

		REQUIRE(lmr.shouldReduce(board, 5, 4, false, move));
		REQUIRE(lmr.getReduction(board, 5, 4, false, move) == 1);
	}

	SECTION("Does not reduce below min depth") {
		REQUIRE_FALSE(lmr.shouldReduce(board, 2, 2, false, move));
		REQUIRE(lmr.getReduction(board, 2, 2, false, move) == 0);

		REQUIRE_FALSE(lmr.shouldReduce(board, 0, 10, false, move));
		REQUIRE(lmr.getReduction(board, 0, 10, false, move) == 0);
	}

	SECTION("Does not reduce early moves") {
		REQUIRE_FALSE(lmr.shouldReduce(board, 3, 0, false, move));
		REQUIRE(lmr.getReduction(board, 3, 0, false, move) == 0);

		REQUIRE_FALSE(lmr.shouldReduce(board, 3, 1, false, move));
		REQUIRE(lmr.getReduction(board, 3, 1, false, move) == 0);
	}

	SECTION("Does not reduce when in check") {
		REQUIRE_FALSE(lmr.shouldReduce(board, 3, 2, true, move));
		REQUIRE(lmr.getReduction(board, 3, 2, true, move) == 0);

		REQUIRE_FALSE(lmr.shouldReduce(board, 8, 20, true, move));
		REQUIRE(lmr.getReduction(board, 8, 20, true, move) == 0);
	}

	SECTION("Respects configured reduction amount") {
		lmr_t deep(3, 2, 3);
		REQUIRE(deep.getReduction(board, 4, 2, false, move) == 3);
		REQUIRE(deep.getReduction(board, 2, 2, false, move) == 0);
	}
}
