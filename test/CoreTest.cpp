#define CATCH_CONFIG_MAIN
#include "photon/core.h"

#include "photon/util.h"

#include <catch2/catch.hpp>

using namespace photon;
using namespace photon::util;

TEST_CASE("Test board factory functions", "[core]") {
	board_t board = DefaultBoard();

	REQUIRE(board.getBitboard(player_t::white, piece_t::pawn) == 0b11111111ULL << 8);
	REQUIRE(board.getBitboard(player_t::white, piece_t::knight) == 0b01000010ULL);
	REQUIRE(board.getBitboard(player_t::white, piece_t::bishop) == 0b00100100ULL);
	REQUIRE(board.getBitboard(player_t::white, piece_t::rook) == 0b10000001ULL);
	REQUIRE(board.getBitboard(player_t::white, piece_t::queen) == 0b00001000ULL);
	REQUIRE(board.getBitboard(player_t::white, piece_t::king) == 0b00010000ULL);

	REQUIRE(board.getBitboard(player_t::black, piece_t::pawn) == 0b11111111ULL << 48);
	REQUIRE(board.getBitboard(player_t::black, piece_t::knight) == 0b01000010ULL << 56);
	REQUIRE(board.getBitboard(player_t::black, piece_t::bishop) == 0b00100100ULL << 56);
	REQUIRE(board.getBitboard(player_t::black, piece_t::rook) == 0b10000001ULL << 56);
	REQUIRE(board.getBitboard(player_t::black, piece_t::queen) == 0b00001000ULL << 56);
	REQUIRE(board.getBitboard(player_t::black, piece_t::king) == 0b00010000ULL << 56);

	REQUIRE(board.hasCastlingRights(player_t::white, castle_t::king));
	REQUIRE(board.hasCastlingRights(player_t::white, castle_t::queen));
	REQUIRE(board.hasCastlingRights(player_t::black, castle_t::king));
	REQUIRE(board.hasCastlingRights(player_t::black, castle_t::queen));

	REQUIRE_FALSE(board.availableEnPassant().has_value());
	REQUIRE(board.playerToMove() == player_t::white);
	REQUIRE(board.halfmoveClock == 0);
	REQUIRE(board.fullmove == 1);
}

TEST_CASE("Test string parsing and serialization", "[core]") {
	SECTION("Test ParseSquare") {
		REQUIRE(ParseSquare("a1") == 0);
		REQUIRE(ParseSquare("e7") == 52);
		REQUIRE(ParseSquare("h8") == 63);
	}

	SECTION("Test MoveFromLongNotation") {
		move_t kingsideCastleW = MoveFromLongNotation(player_t::white, "O-O");
		REQUIRE(kingsideCastleW.from == ParseSquare("e1"));
		REQUIRE(kingsideCastleW.to == ParseSquare("g1"));
		move_t kingsideCastleB = MoveFromLongNotation(player_t::black, "O-O");
		REQUIRE(kingsideCastleB.from == ParseSquare("e8"));
		REQUIRE(kingsideCastleB.to == ParseSquare("g8"));

		move_t queensideCastleW = MoveFromLongNotation(player_t::white, "O-O-O");
		REQUIRE(queensideCastleW.from == ParseSquare("e1"));
		REQUIRE(queensideCastleW.to == ParseSquare("c1"));
		move_t queensideCastleB = MoveFromLongNotation(player_t::black, "O-O-O");
		REQUIRE(queensideCastleB.from == ParseSquare("e8"));
		REQUIRE(queensideCastleB.to == ParseSquare("c8"));

		move_t move1 = MoveFromLongNotation(player_t::white, "Nb1-c3");
		REQUIRE(move1.from == ParseSquare("b1"));
		REQUIRE(move1.to == ParseSquare("c3"));

		move_t move2 = MoveFromLongNotation(player_t::black, "e7-e5");
		REQUIRE(move2.from == ParseSquare("e7"));
		REQUIRE(move2.to == ParseSquare("e5"));
	}
}

TEST_CASE("Test occupancy map", "[core]") {
	board_t board = DefaultBoard();
	bitboard_t occupancy = board.occupancyMap();
	bitboard_t white = board.occupancyMap(player_t::white);
	bitboard_t black = board.occupancyMap(player_t::black);

	REQUIRE(white == 0xFFFF);
	REQUIRE(black == 0xFFFF000000000000);
	REQUIRE(occupancy == 0xFFFF00000000FFFF);
}

TEST_CASE("Test IsSquareAttacked", "[core]") {
	board_t board = DefaultBoard();
	for (int i = ParseSquare("a1"); i <= ParseSquare("h2"); i++) {
		INFO("Square: " << SquareToString(i));
		REQUIRE_FALSE(board.isSquareAttacked(player_t::black, i));
	}

	for (int i = ParseSquare("a3"); i <= ParseSquare("h3"); i++) {
		INFO("Square: " << SquareToString(i));
		REQUIRE(board.isSquareAttacked(player_t::white, i));
		REQUIRE_FALSE(board.isSquareAttacked(player_t::black, i));
	}

	for (int i = ParseSquare("a4"); i <= ParseSquare("h5"); i++) {
		INFO("Square: " << SquareToString(i));
		REQUIRE_FALSE(board.isSquareAttacked(player_t::white, i));
		REQUIRE_FALSE(board.isSquareAttacked(player_t::black, i));
	}

	for (int i = ParseSquare("a6"); i <= ParseSquare("h6"); i++) {
		INFO("Square: " << SquareToString(i));
		REQUIRE_FALSE(board.isSquareAttacked(player_t::white, i));
		REQUIRE(board.isSquareAttacked(player_t::black, i));
	}

	for (int i = ParseSquare("a8"); i <= ParseSquare("h8"); i++) {
		INFO("Square: " << SquareToString(i));
		REQUIRE_FALSE(board.isSquareAttacked(player_t::white, i));
	}
}
