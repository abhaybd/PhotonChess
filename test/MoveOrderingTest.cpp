#include "../src/engine/move_ordering/history.h"
#include "../src/engine/move_ordering/move_ordering.h"
#include "string_conversions.h" // IWYU pragma: keep

#include <catch2/catch_test_macros.hpp>
#include <photon/core.h>
#include <photon/engine/eval.h>
#include <photon/util.h>

using namespace photon;
using namespace photon::util;
using namespace photon::engine;

struct scoredmove_sorter {
	bool operator()(const scoredmove_t& a, const scoredmove_t& b) const {
		return a.score > b.score;
	}
};

TEST_CASE("Test Move Ordering in QSearch", "[engine][move_ordering]") {
	history_table_t historyTable(100, 10);

	SECTION("No captures is empty") {
		board_t board = DefaultBoard();
		auto moves = board.moves();
		auto scoredMoves = ScoreMoves(board, moves, {}, historyTable, {}, true);
		REQUIRE(scoredMoves.empty());
	}

	SECTION("SEE pruning, MVV-LVA sorting") {
		board_t board = MakeBoard("8/4k2p/4n3/5B2/8/4K3/8/8 w - - 0 1");
		auto scoredMoves = ScoreMoves(board, board.moves(), {}, historyTable, {}, true);
		REQUIRE(scoredMoves.size() == 2);

		std::sort(scoredMoves.begin(), scoredMoves.end(), scoredmove_sorter());
		std::vector<move_t> moves;
		for (const auto& scoredMove : scoredMoves) {
			moves.push_back(scoredMove.move);
		}

		const std::vector<move_t> expectedMoves = {MoveFromUCI(board, "f5e6"),
												   MoveFromUCI(board, "f5h7")};
		REQUIRE(moves == expectedMoves);
	}
}

TEST_CASE("Test Move Ordering", "[engine][move_ordering]") {
    history_table_t historyTable(100, 10);
    board_t board = MakeBoard("8/4p3/2NkP3/8/8/8/5K2/8 b - - 0 1");

    move_t ttMove = MoveFromUCI(board, "d6d5");
    auto scoredMoves = ScoreMoves(board, board.moves(), {}, historyTable, ttMove, false);
    std::sort(scoredMoves.begin(), scoredMoves.end(), scoredmove_sorter());
    std::vector<move_t> moves;
    for (const auto& scoredMove : scoredMoves) {
        moves.push_back(scoredMove.move);
    }

    REQUIRE(moves.size() == 5);

    // check tt move is first, captures are above other quiet moves, in MVV-LVA order
    CAPTURE(moves);
    REQUIRE(moves[0] == ttMove);
    REQUIRE(moves[1] == MoveFromUCI(board, "d6c6"));
    REQUIRE(moves[2] == MoveFromUCI(board, "d6e6"));
}
