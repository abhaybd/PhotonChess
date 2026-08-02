#include "photon/util.h"

#include <algorithm>

#include <catch2/catch_test_macros.hpp>

using namespace photon;
using namespace photon::util;

TEST_CASE("Test UCI Move Parsing", "[util]") {
	std::string fen = "n3k2r/1P6/4n3/5Pp1/8/8/8/4K2R w Kk g6 0 1";
	board_t board = MakeBoard(fen);

	SECTION("Quiet move") {
		move_t move = MoveFromUCI(board, "f5f6");
		REQUIRE(move.from == ParseSquare("f5"));
		REQUIRE(move.to == ParseSquare("f6"));
		REQUIRE(move.promotion == -1);
		REQUIRE(!move.isCapture);
	}

	SECTION("Capture move") {
		move_t move = MoveFromUCI(board, "f5e6");
		REQUIRE(move.from == ParseSquare("f5"));
		REQUIRE(move.to == ParseSquare("e6"));
		REQUIRE(move.promotion == -1);
		REQUIRE(move.isCapture);
	}

	SECTION("En passant") {
		move_t move = MoveFromUCI(board, "f5g6");
		REQUIRE(move.from == ParseSquare("f5"));
		REQUIRE(move.to == ParseSquare("g6"));
		REQUIRE(move.promotion == -1);
		REQUIRE(move.isCapture);
		REQUIRE(move.isEnPassant(board));
	}

	SECTION("Quiet Promotion") {
		move_t move = MoveFromUCI(board, "b7b8q");
		REQUIRE(move.from == ParseSquare("b7"));
		REQUIRE(move.to == ParseSquare("b8"));
		REQUIRE(move.promotion == static_cast<int8_t>(piece_t::queen));
		REQUIRE(!move.isCapture);
	}

	SECTION("Capture Promotion") {
		move_t move = MoveFromUCI(board, "b7a8q");
		REQUIRE(move.from == ParseSquare("b7"));
		REQUIRE(move.to == ParseSquare("a8"));
		REQUIRE(move.promotion == static_cast<int8_t>(piece_t::queen));
		REQUIRE(move.isCapture);
	}

	SECTION("Kingside castle") {
		move_t move = MoveFromUCI(board, "e1g1");
		REQUIRE(move.from == ParseSquare("e1"));
		REQUIRE(move.to == ParseSquare("g1"));
		REQUIRE(move.promotion == -1);
		REQUIRE(!move.isCapture);
		REQUIRE(move.isCastle(board, castle_t::king));
	}
}

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
