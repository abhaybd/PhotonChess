#include "../src/engine/move_ordering/exchange.h"

#include <catch2/catch_test_macros.hpp>
#include <photon/core.h>
#include <photon/util.h>

using namespace photon;
using namespace photon::util;
using namespace photon::engine;

struct test_case_t {
	std::string fen;
	std::string moveStr;
	int16_t value;
};

void TestSEE(const std::vector<test_case_t>& cases) {
	for (size_t i = 0; i < cases.size(); i++) {
		const auto& testCase = cases[i];
		CAPTURE(i);
		CAPTURE(testCase.fen);
		CAPTURE(testCase.moveStr);
		CAPTURE(testCase.value);
		board_t board = MakeBoard(testCase.fen);
		move_t move = MoveFromUCI(board, testCase.moveStr);
		REQUIRE(EvaluateExchange(board, move) == testCase.value);
	}
}

TEST_CASE("Test SEE", "[engine][SEE]") {
	SECTION("Basic SEE") {
		TestSEE({
			{"4k3/8/8/3n4/4P3/8/8/4K3 w - - 0 1", "e4d5", 300},
			{"4k3/8/2p5/3p4/8/2N5/8/4K3 w - - 0 1", "c3d5", -200},
			{"4k3/8/2p5/3p4/8/8/8/3QK3 w - - 0 1", "d1d5", -800},
		});
	}

	SECTION("MVV-LVA Ordering") {
		TestSEE({
			{"4k3/8/2p5/3n4/4P3/8/8/3QK3 w - - 0 1", "e4d5", 300},
			{"4k3/8/2p5/3n4/4P3/8/8/3QK3 w - - 0 1", "d1d5", -500},
		});
	}

	SECTION("King Handling") {
		TestSEE({
			{"8/8/3k4/3q4/8/1B6/8/3QK3 w - - 0 1", "b3d5", 900},
			{"8/8/3k4/3q4/8/1B6/8/4K3 w - - 0 1", "b3d5", 600},
			{"8/8/3k4/3B4/8/8/8/4K3 b - - 0 1", "d6d5", 300},
		});
	}

	SECTION("X-Ray Attacks") {
		TestSEE({
			{"3r3k/8/8/3p4/8/8/3R4/3RK3 w - - 0 1", "d2d5", 100},
			{"3r3k/8/8/3p4/8/8/3R4/3QK3 w - - 0 1", "d2d5", 100},
			{"4k3/8/4p3/3p4/8/1B6/Q7/4K3 w - - 0 1", "b3d5", -100},
			{"4k3/8/4p3/3p4/2P5/1B6/8/4K3 w - - 0 1", "c4d5", 100},
			{"3qk3/3r4/3r4/3p4/8/3R4/3R4/3QK3 w - - 0 1", "d3d5", -400},
		});
	}

	SECTION("Pins") {
		TestSEE({
			{"4k3/8/2p5/3p4/B3P3/8/8/4K3 w - - 0 1", "e4d5", 0},
			{"3k4/3r4/8/3p4/8/2N5/8/3RK3 w - - 0 1", "c3d5", 100},
			{"1k6/1b6/8/3p4/8/1B6/8/1R2K3 w - - 0 1", "b3d5", -200},
		});
	}

	SECTION("En Passant") {
		TestSEE({
			{"4k3/2p5/8/3pP3/8/8/8/4K3 w - d6 0 1", "e5d6", 0},
			{"4k3/2p5/8/3pP3/8/8/8/3RK3 w - d6 0 1", "e5d6", 100},
		});
	}

	SECTION("Promotion") {
		TestSEE({
			{"r3k3/1P6/8/8/8/8/8/4K3 w - - 0 1", "b7a8q", 1300},
			{"rk6/1P6/8/8/8/8/8/4K3 w - - 0 1", "b7a8q", 400},
		});
	}

	SECTION("Advanced") {
		TestSEE({
			{"1k1r4/1ppn3p/p4b2/4n3/8/P2N2P1/1PP1R1BP/2K1Q3 w - - 0 1", "e2e5", 100},
			{"1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w - - 0 1", "d3e5", -200},
		});
	}
}
