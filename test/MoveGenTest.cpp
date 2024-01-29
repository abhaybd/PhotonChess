#include "photon/core.h"
#include "photon/util.h"

#include <catch2/catch.hpp>

using namespace photon;
using namespace photon::util;

namespace {

template <typename T>
bool contains(const T& container, const typename T::value_type& value) {
	return std::find(container.begin(), container.end(), value) != container.end();
}

} // namespace

TEST_CASE("Test PawnMoves on default board", "[util][moves]") {
	board_t board = DefaultBoard();

	std::vector<move_t> whiteMoves;
	PawnMoves(board, player_t::white, whiteMoves);
	REQUIRE(whiteMoves.size() == 16);
	for (uint8_t i = 8; i < 16; ++i) {
		move_t move1 = {i, static_cast<uint8_t>(i + 8)};
		move_t move2 = {i, static_cast<uint8_t>(i + 16)};
		REQUIRE(contains(whiteMoves, move1));
		REQUIRE(contains(whiteMoves, move2));
	}

	std::vector<move_t> blackMoves;
	PawnMoves(board, player_t::black, blackMoves);
	REQUIRE(blackMoves.size() == 16);
	for (uint8_t i = 48; i < 56; ++i) {
		move_t move1 = {i, static_cast<uint8_t>(i - 8)};
		move_t move2 = {i, static_cast<uint8_t>(i - 16)};
		REQUIRE(contains(blackMoves, move1));
		REQUIRE(contains(blackMoves, move2));
	}
}

TEST_CASE("Test PawnMoves with capturing", "[util][moves]") {
	board_t board = DefaultBoard();
	board.getBitboard(player_t::white, piece_t::pawn) <<= 16;
	board.getBitboard(player_t::black, piece_t::pawn) >>= 16;

	std::vector<move_t> whiteMoves;
	PawnMoves(board, player_t::white, whiteMoves);
	REQUIRE(whiteMoves.size() == 14);
	REQUIRE(contains(whiteMoves, MakeMove(player_t::white, "a4xb5")));
	REQUIRE(contains(whiteMoves, MakeMove(player_t::white, "h4xg5")));
	for (uint8_t i = ParseSquare("b4"); i <= ParseSquare("g4"); ++i) {
		move_t left = {i, static_cast<uint8_t>(i + 7)};
		move_t right = {i, static_cast<uint8_t>(i + 9)};
		REQUIRE(contains(whiteMoves, left));
		REQUIRE(contains(whiteMoves, right));
	}

	std::vector<move_t> blackMoves;
	PawnMoves(board, player_t::black, blackMoves);
	REQUIRE(blackMoves.size() == 14);
	REQUIRE(contains(blackMoves, MakeMove(player_t::black, "a5xb4")));
	REQUIRE(contains(blackMoves, MakeMove(player_t::black, "h5xg4")));
	for (uint8_t i = ParseSquare("b5"); i <= ParseSquare("g5"); ++i) {
		move_t left = {i, static_cast<uint8_t>(i - 7)};
		move_t right = {i, static_cast<uint8_t>(i - 9)};
		REQUIRE(contains(blackMoves, left));
		REQUIRE(contains(blackMoves, right));
	}
}

TEST_CASE("Test KnightMoves", "[util][moves]") {
	board_t board = DefaultBoard();

	std::vector<move_t> whiteMoves;
	KnightMoves(board, player_t::white, whiteMoves);
	REQUIRE(whiteMoves.size() == 4);
	REQUIRE(contains(whiteMoves, MakeMove(player_t::white, "Nb1-a3")));
	REQUIRE(contains(whiteMoves, MakeMove(player_t::white, "Nb1-c3")));
	REQUIRE(contains(whiteMoves, MakeMove(player_t::white, "Ng1-f3")));
	REQUIRE(contains(whiteMoves, MakeMove(player_t::white, "Ng1-h3")));

	std::vector<move_t> blackMoves;
	KnightMoves(board, player_t::black, blackMoves);
	REQUIRE(blackMoves.size() == 4);
	REQUIRE(contains(blackMoves, MakeMove(player_t::black, "Nb8-a6")));
	REQUIRE(contains(blackMoves, MakeMove(player_t::black, "Nb8-c6")));
	REQUIRE(contains(blackMoves, MakeMove(player_t::black, "Ng8-f6")));
	REQUIRE(contains(blackMoves, MakeMove(player_t::black, "Ng8-h6")));
}

TEST_CASE("Test BishopMoves", "[util][moves]") {
	SECTION("Test no moves") {
		board_t board = DefaultBoard();

		std::vector<move_t> whiteMoves;
		BishopMoves(board, player_t::white, whiteMoves);
		REQUIRE(whiteMoves.size() == 0);

		std::vector<move_t> blackMoves;
		BishopMoves(board, player_t::black, blackMoves);
		REQUIRE(blackMoves.size() == 0);
	}

	SECTION("Test has moves") {
		board_t board = MakeBoard("rnbqk1nr/pppp1ppp/8/2b1p3/2B1P3/8/PPPP1PPP/RNBQK1NR w KQkq - 0 1");

		std::vector<move_t> whiteMoves;
		BishopMoves(board, player_t::white, whiteMoves);
		REQUIRE(whiteMoves.size() == 9);
		REQUIRE(contains(whiteMoves, MakeMove(player_t::white, "Bc4-b3")));
		REQUIRE(contains(whiteMoves, MakeMove(player_t::white, "Bc4-d5")));
		REQUIRE(contains(whiteMoves, MakeMove(player_t::white, "Bc4-e6")));
		REQUIRE(contains(whiteMoves, MakeMove(player_t::white, "Bc4xf7")));
		REQUIRE(contains(whiteMoves, MakeMove(player_t::white, "Bc4-b5")));
		REQUIRE(contains(whiteMoves, MakeMove(player_t::white, "Bc4-a6")));
		REQUIRE(contains(whiteMoves, MakeMove(player_t::white, "Bc4-d3")));
		REQUIRE(contains(whiteMoves, MakeMove(player_t::white, "Bc4-e2")));
		REQUIRE(contains(whiteMoves, MakeMove(player_t::white, "Bc4-f1")));

		std::vector<move_t> blackMoves;
		BishopMoves(board, player_t::black, blackMoves);
		REQUIRE(blackMoves.size() == 9);
		REQUIRE(contains(blackMoves, MakeMove(player_t::black, "Bc5-b6")));
		REQUIRE(contains(blackMoves, MakeMove(player_t::black, "Bc5-d4")));
		REQUIRE(contains(blackMoves, MakeMove(player_t::black, "Bc5-e3")));
		REQUIRE(contains(blackMoves, MakeMove(player_t::black, "Bc5xf2")));
		REQUIRE(contains(blackMoves, MakeMove(player_t::black, "Bc5-a3")));
		REQUIRE(contains(blackMoves, MakeMove(player_t::black, "Bc5-b4")));
		REQUIRE(contains(blackMoves, MakeMove(player_t::black, "Bc5-d6")));
		REQUIRE(contains(blackMoves, MakeMove(player_t::black, "Bc5-e7")));
		REQUIRE(contains(blackMoves, MakeMove(player_t::black, "Bc5-f8")));
	}
}
