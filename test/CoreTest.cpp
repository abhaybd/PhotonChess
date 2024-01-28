#define CATCH_CONFIG_MAIN
#include "chesspp/core.h"

#include "chesspp/util.h"

#include <catch2/catch.hpp>

using namespace chesspp;
using namespace chesspp::util;

TEST_CASE("Test board factory functions", "[core]") {
	board_t board = DefaultBoard();

	REQUIRE(board.getBitboard(player_t::white, piece_t::pawn) == 0b11111111L << 8);
	REQUIRE(board.getBitboard(player_t::white, piece_t::knight) == 0b01000010L);
	REQUIRE(board.getBitboard(player_t::white, piece_t::bishop) == 0b00100100L);
	REQUIRE(board.getBitboard(player_t::white, piece_t::rook) == 0b10000001L);
	REQUIRE(board.getBitboard(player_t::white, piece_t::queen) == 0b00001000L);
	REQUIRE(board.getBitboard(player_t::white, piece_t::king) == 0b00010000L);

	REQUIRE(board.getBitboard(player_t::black, piece_t::pawn) == 0b11111111L << 48);
	REQUIRE(board.getBitboard(player_t::black, piece_t::knight) == 0b01000010L << 56);
	REQUIRE(board.getBitboard(player_t::black, piece_t::bishop) == 0b00100100L << 56);
	REQUIRE(board.getBitboard(player_t::black, piece_t::rook) == 0b10000001L << 56);
	REQUIRE(board.getBitboard(player_t::black, piece_t::queen) == 0b00001000L << 56);
	REQUIRE(board.getBitboard(player_t::black, piece_t::king) == 0b00010000L << 56);

	REQUIRE(board.hasCastlingRights(player_t::white, castle_t::king));
	REQUIRE(board.hasCastlingRights(player_t::white, castle_t::queen));
	REQUIRE(board.hasCastlingRights(player_t::black, castle_t::king));
	REQUIRE(board.hasCastlingRights(player_t::black, castle_t::queen));

	REQUIRE_FALSE(board.availableEnPassant().has_value());
	REQUIRE(board.playerToMove() == player_t::white);
	REQUIRE(board.halfmoveClock == 0);
	REQUIRE(board.fullmove == 1);
}

TEST_CASE("Test string parsing and serialization") {
	SECTION("Test ParseSquare") {
		REQUIRE(ParseSquare("a1") == 0);
		REQUIRE(ParseSquare("e7") == 52);
		REQUIRE(ParseSquare("h8") == 63);
	}

	SECTION("Test MakeMove") {
		move_t kingsideCastleW = MakeMove(player_t::white, "O-O");
		REQUIRE(kingsideCastleW.from == ParseSquare("e1"));
		REQUIRE(kingsideCastleW.to == ParseSquare("g1"));
		move_t kingsideCastleB = MakeMove(player_t::black, "O-O");
		REQUIRE(kingsideCastleB.from == ParseSquare("e8"));
		REQUIRE(kingsideCastleB.to == ParseSquare("g8"));

		move_t queensideCastleW = MakeMove(player_t::white, "O-O-O");
		REQUIRE(queensideCastleW.from == ParseSquare("e1"));
		REQUIRE(queensideCastleW.to == ParseSquare("c1"));
		move_t queensideCastleB = MakeMove(player_t::black, "O-O-O");
		REQUIRE(queensideCastleB.from == ParseSquare("e8"));
		REQUIRE(queensideCastleB.to == ParseSquare("c8"));

		move_t move1 = MakeMove(player_t::white, "Nb1-c3");
		REQUIRE(move1.from == ParseSquare("b1"));
		REQUIRE(move1.to == ParseSquare("c3"));

		move_t move2 = MakeMove(player_t::black, "e7-e5");
		REQUIRE(move2.from == ParseSquare("e7"));
		REQUIRE(move2.to == ParseSquare("e5"));
	}
}
