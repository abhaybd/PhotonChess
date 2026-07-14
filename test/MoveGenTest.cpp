#include "../src/moves.h"
#include "photon/core.h"
#include "photon/util.h"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>

using namespace photon;
using namespace photon::util;

namespace {

template <typename T>
bool contains(const T& container, const typename T::value_type& value) {
	return std::find(container.begin(), container.end(), value) != container.end();
}

int bitcount(bitboard_t b) {
	int count = 0;
	while (b) {
		b &= b - 1;
		count++;
	}
	return count;
}

} // namespace

TEST_CASE("Test PawnMoves on default board", "[util][moves]") {
	board_t board = DefaultBoard();

	std::vector<move_t> whiteMoves;
	PawnMoves(board.getBitboard(player_t::white, piece_t::pawn), board.occupancyMap(),
			  board.occupancyMap(player_t::black), player_t::white, board.availableEnPassant(),
			  whiteMoves);
	REQUIRE(whiteMoves.size() == 16);
	for (uint8_t i = 8; i < 16; ++i) {
		move_t move1 = {i, static_cast<uint8_t>(i + 8)};
		move_t move2 = {i, static_cast<uint8_t>(i + 16)};
		REQUIRE(contains(whiteMoves, move1));
		REQUIRE(contains(whiteMoves, move2));
	}

	std::vector<move_t> blackMoves;
	PawnMoves(board.getBitboard(player_t::black, piece_t::pawn), board.occupancyMap(),
			  board.occupancyMap(player_t::white), player_t::black, board.availableEnPassant(),
			  blackMoves);
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
	PawnMoves(board.getBitboard(player_t::white, piece_t::pawn), board.occupancyMap(),
			  board.occupancyMap(player_t::black), player_t::white, board.availableEnPassant(),
			  whiteMoves);
	REQUIRE(whiteMoves.size() == 14);
	REQUIRE(contains(whiteMoves, MoveFromLongNotation(player_t::white, "a4xb5")));
	REQUIRE(contains(whiteMoves, MoveFromLongNotation(player_t::white, "h4xg5")));
	for (uint8_t i = ParseSquare("b4"); i <= ParseSquare("g4"); ++i) {
		move_t left = {i, static_cast<uint8_t>(i + 7)};
		move_t right = {i, static_cast<uint8_t>(i + 9)};
		REQUIRE(contains(whiteMoves, left));
		REQUIRE(contains(whiteMoves, right));
	}

	std::vector<move_t> blackMoves;
	PawnMoves(board.getBitboard(player_t::black, piece_t::pawn), board.occupancyMap(),
			  board.occupancyMap(player_t::white), player_t::black, board.availableEnPassant(),
			  blackMoves);
	REQUIRE(blackMoves.size() == 14);
	REQUIRE(contains(blackMoves, MoveFromLongNotation(player_t::black, "a5xb4")));
	REQUIRE(contains(blackMoves, MoveFromLongNotation(player_t::black, "h5xg4")));
	for (uint8_t i = ParseSquare("b5"); i <= ParseSquare("g5"); ++i) {
		move_t left = {i, static_cast<uint8_t>(i - 7)};
		move_t right = {i, static_cast<uint8_t>(i - 9)};
		REQUIRE(contains(blackMoves, left));
		REQUIRE(contains(blackMoves, right));
	}
}

TEST_CASE("Test KnightMoves", "[util][moves]") {
	board_t board = DefaultBoard();

	bitboard_t white = board.occupancyMap(player_t::white);
	bitboard_t black = board.occupancyMap(player_t::black);

	std::vector<move_t> whiteMoves;
	KnightMoves(board.getBitboard(player_t::white, piece_t::knight), white, black, whiteMoves);
	REQUIRE(whiteMoves.size() == 4);
	REQUIRE(contains(whiteMoves, MoveFromLongNotation(player_t::white, "Nb1-a3")));
	REQUIRE(contains(whiteMoves, MoveFromLongNotation(player_t::white, "Nb1-c3")));
	REQUIRE(contains(whiteMoves, MoveFromLongNotation(player_t::white, "Ng1-f3")));
	REQUIRE(contains(whiteMoves, MoveFromLongNotation(player_t::white, "Ng1-h3")));

	std::vector<move_t> blackMoves;
	KnightMoves(board.getBitboard(player_t::black, piece_t::knight), black, white, blackMoves);
	REQUIRE(blackMoves.size() == 4);
	REQUIRE(contains(blackMoves, MoveFromLongNotation(player_t::black, "Nb8-a6")));
	REQUIRE(contains(blackMoves, MoveFromLongNotation(player_t::black, "Nb8-c6")));
	REQUIRE(contains(blackMoves, MoveFromLongNotation(player_t::black, "Ng8-f6")));
	REQUIRE(contains(blackMoves, MoveFromLongNotation(player_t::black, "Ng8-h6")));
}

TEST_CASE("Test BishopMoves", "[util][moves]") {
	SECTION("Test no moves") {
		board_t board = DefaultBoard();

		std::vector<move_t> whiteMoves;
		BishopMoves(board.getBitboard(player_t::white, piece_t::bishop),
					board.occupancyMap(player_t::white), board.occupancyMap(player_t::black),
					whiteMoves);
		REQUIRE(whiteMoves.size() == 0);

		std::vector<move_t> blackMoves;
		BishopMoves(board.getBitboard(player_t::black, piece_t::bishop),
					board.occupancyMap(player_t::black), board.occupancyMap(player_t::white),
					blackMoves);
		REQUIRE(blackMoves.size() == 0);
	}

	SECTION("Test has moves") {
		board_t board =
			MakeBoard("rnbqk1nr/pppp1ppp/8/2b1p3/2B1P3/8/PPPP1PPP/RNBQK1NR w KQkq - 0 1");
		bitboard_t whiteMap = board.occupancyMap(player_t::white);
		bitboard_t blackMap = board.occupancyMap(player_t::black);

		std::vector<move_t> whiteMoves;
		BishopMoves(board.getBitboard(player_t::white, piece_t::bishop), whiteMap, blackMap,
					whiteMoves);
		REQUIRE(whiteMoves.size() == 9);
		REQUIRE(contains(whiteMoves, MoveFromLongNotation(player_t::white, "Bc4-b3")));
		REQUIRE(contains(whiteMoves, MoveFromLongNotation(player_t::white, "Bc4-d5")));
		REQUIRE(contains(whiteMoves, MoveFromLongNotation(player_t::white, "Bc4-e6")));
		REQUIRE(contains(whiteMoves, MoveFromLongNotation(player_t::white, "Bc4xf7")));
		REQUIRE(contains(whiteMoves, MoveFromLongNotation(player_t::white, "Bc4-b5")));
		REQUIRE(contains(whiteMoves, MoveFromLongNotation(player_t::white, "Bc4-a6")));
		REQUIRE(contains(whiteMoves, MoveFromLongNotation(player_t::white, "Bc4-d3")));
		REQUIRE(contains(whiteMoves, MoveFromLongNotation(player_t::white, "Bc4-e2")));
		REQUIRE(contains(whiteMoves, MoveFromLongNotation(player_t::white, "Bc4-f1")));

		std::vector<move_t> blackMoves;
		BishopMoves(board.getBitboard(player_t::black, piece_t::bishop), blackMap, whiteMap,
					blackMoves);
		REQUIRE(blackMoves.size() == 9);
		REQUIRE(contains(blackMoves, MoveFromLongNotation(player_t::black, "Bc5-b6")));
		REQUIRE(contains(blackMoves, MoveFromLongNotation(player_t::black, "Bc5-d4")));
		REQUIRE(contains(blackMoves, MoveFromLongNotation(player_t::black, "Bc5-e3")));
		REQUIRE(contains(blackMoves, MoveFromLongNotation(player_t::black, "Bc5xf2")));
		REQUIRE(contains(blackMoves, MoveFromLongNotation(player_t::black, "Bc5-a3")));
		REQUIRE(contains(blackMoves, MoveFromLongNotation(player_t::black, "Bc5-b4")));
		REQUIRE(contains(blackMoves, MoveFromLongNotation(player_t::black, "Bc5-d6")));
		REQUIRE(contains(blackMoves, MoveFromLongNotation(player_t::black, "Bc5-e7")));
		REQUIRE(contains(blackMoves, MoveFromLongNotation(player_t::black, "Bc5-f8")));
	}
}

TEST_CASE("Test RookMoves", "[util][moves]") {
	board_t board = MakeBoard("1nbqkbnr/1ppppppp/3r4/p7/7P/4R3/PPPPPPP1/RNBQKBN1 w Qk - 0 1");
	bitboard_t whiteMap = board.occupancyMap(player_t::white);
	bitboard_t blackMap = board.occupancyMap(player_t::black);

	std::vector<move_t> whiteMoves;
	RookMoves(board.getBitboard(player_t::white, piece_t::rook), whiteMap, blackMap,
			  whiteMoves);
	REQUIRE(whiteMoves.size() == 11);
	uint8_t from = ParseSquare("e3");
	for (uint8_t to = ParseSquare("a3"); to <= ParseSquare("h3"); to++) {
		if (to == from) {
			continue;
		}
		INFO("to: " << SquareToString(to));
		REQUIRE(contains(whiteMoves, move_t{from, to}));
	}
	for (uint8_t to = ParseSquare("e4"); to <= ParseSquare("e7"); to += 8) {
		INFO("to: " << SquareToString(to));
		REQUIRE(contains(whiteMoves, move_t{from, to}));
	}

	std::vector<move_t> blackMoves;
	RookMoves(board.getBitboard(player_t::black, piece_t::rook), blackMap, whiteMap,
			  blackMoves);
	REQUIRE(blackMoves.size() == 11);
	from = ParseSquare("d6");
	for (uint8_t to = ParseSquare("a6"); to <= ParseSquare("h6"); to++) {
		if (to == from) {
			continue;
		}
		INFO("to: " << SquareToString(to));
		REQUIRE(contains(blackMoves, move_t{from, to}));
	}
	for (uint8_t to = ParseSquare("d2"); to <= ParseSquare("d5"); to += 8) {
		INFO("to: " << SquareToString(to));
		REQUIRE(contains(blackMoves, move_t{from, to}));
	}
}

TEST_CASE("Test QueenMoves", "[util][moves]") {
	board_t board =
		MakeBoard("r1bqk1nr/pppp1ppp/2n5/2b1p3/2B1P3/5Q2/PPPP1PPP/RNB1K1NR w KQkq - 0 1");
	bitboard_t whiteMap = board.occupancyMap(player_t::white);
	bitboard_t blackMap = board.occupancyMap(player_t::black);

	std::vector<move_t> whiteMoves;
	QueenMoves(board.getBitboard(player_t::white, piece_t::queen), whiteMap, blackMap,
			   whiteMoves);
	REQUIRE(whiteMoves.size() == 15);
	uint8_t from = ParseSquare("f3");
	for (uint8_t to = ParseSquare("a3"); to <= ParseSquare("h3"); to++) {
		if (to == from) {
			continue;
		}
		INFO("to: " << SquareToString(to));
		REQUIRE(contains(whiteMoves, move_t{from, to}));
	}
	for (uint8_t to = ParseSquare("f4"); to <= ParseSquare("f7"); to += 8) {
		INFO("to: " << SquareToString(to));
		REQUIRE(contains(whiteMoves, move_t{from, to}));
	}
	for (uint8_t to = ParseSquare("d1"); to <= ParseSquare("h5"); to += 9) {
		if (to == from) {
			continue;
		}
		INFO("to: " << SquareToString(to));
		REQUIRE(contains(whiteMoves, move_t{from, to}));
	}

	std::vector<move_t> blackMoves;
	QueenMoves(board.getBitboard(player_t::black, piece_t::queen), blackMap, whiteMap,
			   blackMoves);
	REQUIRE(blackMoves.size() == 4);
	from = ParseSquare("d8");
	for (uint8_t to = ParseSquare("h4"); to <= ParseSquare("e7"); to += 7) {
		INFO("to: " << SquareToString(to));
		REQUIRE(contains(blackMoves, move_t{from, to}));
	}
}

TEST_CASE("Test KingMoves", "[util][moves]") {
	board_t board = MakeBoard("r3kbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQK2R w KQkq e6 0 1");

	std::vector<move_t> whiteMoves;
	KingMoves(board, player_t::white, whiteMoves);
	REQUIRE(whiteMoves.size() == 3);
	REQUIRE(contains(whiteMoves, MoveFromLongNotation(player_t::white, "Ke1-f1")));
	REQUIRE(contains(whiteMoves, MoveFromLongNotation(player_t::white, "Ke1-e2")));
	REQUIRE(contains(whiteMoves, MoveFromLongNotation(player_t::white, "O-O")));

	std::vector<move_t> blackMoves;
	KingMoves(board, player_t::black, blackMoves);
	REQUIRE(blackMoves.size() == 3);
	REQUIRE(contains(blackMoves, MoveFromLongNotation(player_t::black, "Ke8-d8")));
	REQUIRE(contains(blackMoves, MoveFromLongNotation(player_t::black, "Ke8-e7")));
	REQUIRE(contains(blackMoves, MoveFromLongNotation(player_t::black, "O-O-O")));
}

TEST_CASE("Test PawnAttackMask", "[util][moves]") {
	board_t board =
		MakeBoard("rnbqkbnr/ppp1p1pp/8/3p1p2/4P3/5P2/PPPP2PP/RNBQKBNR b KQkq - 0 1");

	bitboard_t whiteAttacks =
		PawnAttackMask(board.getBitboard(player_t::white, piece_t::pawn),
					   board.occupancyMap(player_t::black), player_t::white);
	REQUIRE(bitcount(whiteAttacks) == 2);
	REQUIRE((whiteAttacks & (1ULL << ParseSquare("d5"))) != 0);
	REQUIRE((whiteAttacks & (1ULL << ParseSquare("f5"))) != 0);

	bitboard_t blackAttacks =
		PawnAttackMask(board.getBitboard(player_t::black, piece_t::pawn),
					   board.occupancyMap(player_t::white), player_t::black);
	REQUIRE(bitcount(blackAttacks) == 1);
	REQUIRE((blackAttacks & (1ULL << ParseSquare("e4"))) != 0);
}

TEST_CASE("Test KnightAttackMask", "[util][moves]") {
	board_t board =
		MakeBoard("r1bqkbnr/pppp1ppp/2n5/4N3/4P3/8/PPPP1PPP/RNBQKB1R b KQkq - 0 1");

	bitboard_t whiteAttacks =
		KnightAttackMask(board.getBitboard(player_t::white, piece_t::knight),
						 board.occupancyMap(player_t::white));
	REQUIRE(bitcount(whiteAttacks) == 10);
	REQUIRE(whiteAttacks == ((1ULL << ParseSquare("c6")) | (1ULL << ParseSquare("d7")) |
							 (1ULL << ParseSquare("f7")) | (1ULL << ParseSquare("g6")) |
							 (1ULL << ParseSquare("g4")) | (1ULL << ParseSquare("f3")) |
							 (1ULL << ParseSquare("d3")) | (1ULL << ParseSquare("c4")) |
							 (1ULL << ParseSquare("a3")) | (1ULL << ParseSquare("c3"))));

	bitboard_t blackAttacks =
		KnightAttackMask(board.getBitboard(player_t::black, piece_t::knight),
						 board.occupancyMap(player_t::black));
	REQUIRE(bitcount(blackAttacks) == 8);
	REQUIRE(blackAttacks == ((1ULL << ParseSquare("b8")) | (1ULL << ParseSquare("e7")) |
							 (1ULL << ParseSquare("e5")) | (1ULL << ParseSquare("d4")) |
							 (1ULL << ParseSquare("b4")) | (1ULL << ParseSquare("a5")) |
							 (1ULL << ParseSquare("f6")) | (1ULL << ParseSquare("h6"))));
}

TEST_CASE("Test PieceAttackMasks", "[util][moves]") {
	board_t board =
		MakeBoard("r3k1nr/ppp2ppp/1bnpqb2/4N2Q/2B1P2P/3P2R1/PPP2PP1/RNB1K3 w - - 0 1");

	for (piece_t p :
		 {piece_t::knight, piece_t::bishop, piece_t::rook, piece_t::queen, piece_t::king}) {
		bitboard_t whiteAttacks = PieceAttackMoves(
			p, board.getBitboard(player_t::white, p), player_t::white,
			board.occupancyMap(player_t::white), board.occupancyMap(player_t::black));
		std::vector<move_t> whiteMoves;
		PieceMoves(player_t::white, p, board, whiteMoves);

		bitboard_t trueWhiteAttacks = 0;
		for (const move_t& move : whiteMoves) {
			trueWhiteAttacks |= 1ULL << move.to;
		}

		REQUIRE(whiteAttacks == trueWhiteAttacks);

		bitboard_t blackAttacks = PieceAttackMoves(
			p, board.getBitboard(player_t::black, p), player_t::black,
			board.occupancyMap(player_t::black), board.occupancyMap(player_t::white));
		std::vector<move_t> blackMoves;
		PieceMoves(player_t::black, p, board, blackMoves);

		bitboard_t trueBlackAttacks = 0;
		for (const move_t& move : blackMoves) {
			trueBlackAttacks |= 1ULL << move.to;
		}

		REQUIRE(blackAttacks == trueBlackAttacks);
	}
}
