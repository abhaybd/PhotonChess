#include "photon/util.h"

#include <catch2/catch_test_macros.hpp>

using namespace photon;
using namespace photon::util;

TEST_CASE("Test Board Hashing", "[util][hash]") {
	std::string fen = "r3k2r/6p1/8/8/2p2P2/8/1P6/R3K2R w KQkq - 0 1";
	board_t board = MakeBoard(fen);
	std::vector<uint64_t> hashes;
	hashes.push_back(board.hash);

	{
		// test deterministic hash
		board_t board2 = MakeBoard(fen);
		REQUIRE(board.hash == board2.hash);
	}

	// test that hash changes when moves are made
	// test that castles, captures,
	board.doMove(MoveFromUCI(board, "e1g1"));
	REQUIRE(std::find(hashes.begin(), hashes.end(), board.hash) == hashes.end());
	hashes.push_back(board.hash);
	board.doMove(MoveFromUCI(board, "e8c8"));
	REQUIRE(std::find(hashes.begin(), hashes.end(), board.hash) == hashes.end());

	board.doMove(MoveFromUCI(board, "b2b4"));
	board.doMove(MoveFromUCI(board, "c4b3"));
	board.doMove(MoveFromUCI(board, "f4f5"));
	board.doMove(MoveFromUCI(board, "g7g5"));
	board.doMove(MoveFromUCI(board, "f5g6"));
	board.doMove(MoveFromUCI(board, "b3b2"));
	board.doMove(MoveFromUCI(board, "g6g7"));
	INFO(board.fen());
	board.doMove(MoveFromUCI(board, "b2a1q"));
	INFO(board.fen());
	board.doMove(MoveFromUCI(board, "g7h8n"));
	INFO(board.fen());

	std::string fen2 = "2kr3N/8/8/8/8/8/8/q4RK1 b - - 0 6";
	INFO("Board1: " << board.fen());
	INFO("Board2: " << fen2);
	board_t board2 = MakeBoard(fen2);
	REQUIRE(board.hash == board2.hash);
}
