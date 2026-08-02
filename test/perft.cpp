#include "catch2/generators/catch_generators_range.hpp"
#include "photon/core.h"
#include "photon/util.h"

#include <sstream>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>

using namespace photon;

namespace {

unsigned long long perft(board_t& board, int depth) {
	if (depth == 0) {
		return 1;
	}

	auto moves = board.moves();
	unsigned long long nodes = 0;
	if (depth == 1) {
		return moves.size();
	}
	for (auto& move : moves) {
		auto handle = board.doMoveTemp(move);
		nodes += perft(board, depth - 1);
	}

	return nodes;
}

// test cases from: https://www.chessprogramming.org/Perft_Results
const std::vector<std::pair<std::string, std::vector<unsigned long long>>> FENS = {
	{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
	 {1, 20, 400, 8902, 197281, 4865609}},
	{"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
	 {1, 48, 2039, 97862, 4085603}},
	{"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
	 {1, 14, 191, 2812, 43238, 674624, 11030083}},
	{"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
	 {1, 6, 264, 9467, 422333, 15833292}},
	{"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
	 {1, 44, 1486, 62379, 2103487}},
	{"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
	 {1, 46, 2079, 89890, 3894594}}};

} // namespace

TEST_CASE("Perft", "[core][perft]") {
	auto i = GENERATE(Catch::Generators::range(size_t{0}, FENS.size()));
	const auto& [fen, expectedNodes] = FENS[i];

	const auto depth =
		GENERATE_COPY(Catch::Generators::range(size_t{0}, expectedNodes.size()));
	CAPTURE(i, fen, depth);

	board_t board = util::MakeBoard(fen);
	REQUIRE(perft(board, depth) == expectedNodes[depth]);
}

TEST_CASE("Perft Benchmark", "[core][perft][!benchmark]") {
	auto i = GENERATE(Catch::Generators::range(size_t{0}, FENS.size()));
	const auto& [fen, expectedNodes] = FENS[i];

	CAPTURE(fen);
	board_t board = util::MakeBoard(fen);
	int depth = expectedNodes.size() - 1;
	std::stringstream ss;
	ss << "Perft - Position " << i + 1 << " Depth=" << depth;
	BENCHMARK(ss.str()) {
		return perft(board, depth);
	};
}
