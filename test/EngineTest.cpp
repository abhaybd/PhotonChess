#include "photon/core.h"
#include "photon/engine/eval.h"
#include "photon/util.h"

#include <catch2/catch_test_macros.hpp>

using namespace photon;
using namespace photon::engine;
using namespace photon::util;

namespace {

void assertMovesEqual(const std::vector<move_t>& moves, const std::vector<move_t>& solution) {
	REQUIRE(moves.size() == solution.size());
	for (size_t i = 0; i < moves.size(); i++) {
		INFO("Ply " << i << ": " << MoveToUCI(moves[i]) << " != " << MoveToUCI(solution[i]));
		REQUIRE(moves[i] == solution[i]);
	}
}

} // namespace

TEST_CASE("Test mate in 1 ply", "[engine]") {
	board_t board =
		MakeBoard("r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 0 1");
	evaluation_t eval = EvalBoard(board, 2).first;

	std::vector<move_t> solution = {MoveFromUCI("h5f7")};

    REQUIRE(eval.result == result_t::white_wins);
	assertMovesEqual(eval.moves, solution);
}

TEST_CASE("Test mate in 3 ply", "[engine]") {
	board_t board =
		MakeBoard("r1bq2r1/b4pk1/p1pp1p2/1p2pP2/1P2P1PB/3P4/1PPQ2P1/R3K2R w KQ - 0 1");
	evaluation_t eval = EvalBoard(board, 4).first;

	std::vector<move_t> solution = {MoveFromUCI("d2h6"), MoveFromUCI("g7h6"),
									MoveFromUCI("h4f6")};

    REQUIRE(eval.result == result_t::white_wins);
	assertMovesEqual(eval.moves, solution);
}
