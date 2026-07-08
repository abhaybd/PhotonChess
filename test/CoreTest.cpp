#define CATCH_CONFIG_MAIN
#include "photon/core.h"

#include "photon/util.h"

#include <catch2/catch_test_macros.hpp>

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

TEST_CASE("Test fen", "[core]") {
	SECTION("Test DefaultBoard") {
		board_t board = DefaultBoard();
		REQUIRE(board.fen() == "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	}

	SECTION("Test more complicated") {
		std::string fen =
			"r1bqk2r/pp1p1ppp/3b3n/1BpPp3/1n2P3/5N2/PPP2PPP/RNBQ1RK1 w kq c6 0 7";
		board_t board = MakeBoard(fen);
		REQUIRE(board.fen() == fen);
	}
}

TEST_CASE("Test Checkmate", "[core]") {
	board_t board = DefaultBoard();
	board.doMove(MoveFromLongNotation(player_t::white, "f2-f3"));
	REQUIRE(board.result() == result_t::none);
	board.doMove(MoveFromLongNotation(player_t::black, "e7-e5"));
	REQUIRE(board.result() == result_t::none);
	board.doMove(MoveFromLongNotation(player_t::white, "g2-g4"));
	REQUIRE(board.result() == result_t::none);
	board.doMove(MoveFromLongNotation(player_t::black, "Qd8-h4#"));
	REQUIRE(board.result() == result_t::black_wins);
}

TEST_CASE("Test doMove", "[core]") {
	// A bunch of moves that test e.p., capturing, pawn moves, castling, etc.
	// This should cover most/all relevant cases
	std::vector<std::pair<std::string, std::string>> game = {
		{"e2-e4", "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"},
		{"e7-e5", "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2"},
		{"Ng1-f3", "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2"},
		{"Nb8-c6", "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3"},
		{"Nf3xe5", "r1bqkbnr/pppp1ppp/2n5/4N3/4P3/8/PPPP1PPP/RNBQKB1R b KQkq - 0 3"},
		{"Nc6xe5", "r1bqkbnr/pppp1ppp/8/4n3/4P3/8/PPPP1PPP/RNBQKB1R w KQkq - 0 4"},
		{"d2-d4", "r1bqkbnr/pppp1ppp/8/4n3/3PP3/8/PPP2PPP/RNBQKB1R b KQkq d3 0 4"},
		{"Bf8-e7", "r1bqk1nr/ppppbppp/8/4n3/3PP3/8/PPP2PPP/RNBQKB1R w KQkq - 1 5"},
		{"d4-d5", "r1bqk1nr/ppppbppp/8/3Pn3/4P3/8/PPP2PPP/RNBQKB1R b KQkq - 0 5"},
		{"c7-c5", "r1bqk1nr/pp1pbppp/8/2pPn3/4P3/8/PPP2PPP/RNBQKB1R w KQkq c6 0 6"},
		{"d5xc6", "r1bqk1nr/pp1pbppp/2P5/4n3/4P3/8/PPP2PPP/RNBQKB1R b KQkq - 0 6"}, // e.p.
		{"d7xc6", "r1bqk1nr/pp2bppp/2p5/4n3/4P3/8/PPP2PPP/RNBQKB1R w KQkq - 0 7"},
		{"Bf1-d3", "r1bqk1nr/pp2bppp/2p5/4n3/4P3/3B4/PPP2PPP/RNBQK2R b KQkq - 1 7"},
		{"Bc8-e6", "r2qk1nr/pp2bppp/2p1b3/4n3/4P3/3B4/PPP2PPP/RNBQK2R w KQkq - 2 8"},
		{"O-O", "r2qk1nr/pp2bppp/2p1b3/4n3/4P3/3B4/PPP2PPP/RNBQ1RK1 b kq - 3 8"},
		{"Ng8-f6", "r2qk2r/pp2bppp/2p1bn2/4n3/4P3/3B4/PPP2PPP/RNBQ1RK1 w kq - 4 9"},
		{"Nb1-c3", "r2qk2r/pp2bppp/2p1bn2/4n3/4P3/2NB4/PPP2PPP/R1BQ1RK1 b kq - 5 9"},
		{"Rh8-g8", "r2qk1r1/pp2bppp/2p1bn2/4n3/4P3/2NB4/PPP2PPP/R1BQ1RK1 w q - 6 10"},
		{"a2-a3", "r2qk1r1/pp2bppp/2p1bn2/4n3/4P3/P1NB4/1PP2PPP/R1BQ1RK1 b q - 0 10"},
		{"Qd8-d7", "r3k1r1/pp1qbppp/2p1bn2/4n3/4P3/P1NB4/1PP2PPP/R1BQ1RK1 w q - 1 11"},
		{"Nc3-d5", "r3k1r1/pp1qbppp/2p1bn2/3Nn3/4P3/P2B4/1PP2PPP/R1BQ1RK1 b q - 2 11"},
		{"O-O-O", "2kr2r1/pp1qbppp/2p1bn2/3Nn3/4P3/P2B4/1PP2PPP/R1BQ1RK1 w - - 3 12"}};

	board_t board = DefaultBoard();
	for (size_t i = 0; i < game.size(); i++) {
		auto& pair = game[i];
		INFO("Ply " << i << ": " << pair.first);
		move_t move = MoveFromLongNotation(board.playerToMove(), pair.first);
		board.doMove(move);
		REQUIRE(board.fen() == pair.second);
	}
}

TEST_CASE("Test move generation", "[core]") {
	{
		// white to move
		board_t board = MakeBoard("r3k2r/p7/4n3/2B2pPb/4Q3/8/6p1/R3K2R w KQkq f6 0 1");
		auto moves = board.moves();

		std::vector<std::string> trueMoves = {"Ke1-d2", "Ke1-f2", "g5xf6",	"g5-g6",  "Rh1-g1",
											  "Rh1-f1", "Rh1-h2", "Rh1-h3", "Rh1-h4", "Rh1xh5",
											  "Ra1-b1", "Ra1-c1", "Ra1-d1", "Ra1-a2", "Ra1-a3",
											  "Ra1-a4", "Ra1-a5", "Ra1-a6", "Ra1xa7"};

		uint8_t bishop = ParseSquare("c5");
		for (uint8_t idx = ParseSquare("g1"); idx <= ParseSquare("a7"); idx += 7) {
			if (idx != bishop) {
				trueMoves.emplace_back(MoveToLongNotation(board, {bishop, idx}));
			}
		}
		for (uint8_t idx = ParseSquare("a3"); idx <= ParseSquare("f8"); idx += 9) {
			if (idx != bishop) {
				trueMoves.emplace_back(MoveToLongNotation(board, {bishop, idx}));
			}
		}

		uint8_t queen = ParseSquare("e4");
		for (uint8_t idx = ParseSquare("g2"); idx <= ParseSquare("a8"); idx += 7) {
			if (idx != queen) {
				trueMoves.emplace_back(MoveToLongNotation(board, {queen, idx}));
			}
		}
		for (uint8_t idx = ParseSquare("b1"); idx <= ParseSquare("f5"); idx += 9) {
			if (idx != queen) {
				trueMoves.emplace_back(MoveToLongNotation(board, {queen, idx}));
			}
		}
		for (uint8_t idx = ParseSquare("a4"); idx <= ParseSquare("h4"); idx++) {
			if (idx != queen) {
				trueMoves.emplace_back(MoveToLongNotation(board, {queen, idx}));
			}
		}
		for (uint8_t idx = ParseSquare("e2"); idx <= ParseSquare("e6"); idx += 8) {
			if (idx != queen) {
				trueMoves.emplace_back(MoveToLongNotation(board, {queen, idx}));
			}
		}

		REQUIRE(moves.size() == trueMoves.size());
		for (auto& moveStr : trueMoves) {
			move_t move = MoveFromLongNotation(board.playerToMove(), moveStr);
			REQUIRE(std::find(moves.begin(), moves.end(), move) != moves.end());
		}
	}
	{
		// black to move
		board_t board = MakeBoard("r3k2r/p7/4n3/2B2pPb/4Q3/8/6p1/R3K2R b KQkq - 0 1");
		auto moves = board.moves();

		std::vector<std::string> trueMoves = {
			"O-O-O",   "Ke8-d8",   "Ke8-d7",   "Ke8-f7",   "Ra8-b8",   "Ra8-c8",
			"Ra8-d8",  "Rh8-g8",   "Rh8-f8",   "Rh8-h7",   "Rh8-h6",   "a7-a6",
			"a7-a5",   "f5-f4",	   "f5xe4",	   "g2-g1=Q+", "g2-g1=R+", "g2-g1=B",
			"g2-g1=N", "g2xh1=Q+", "g2xh1=R+", "g2xh1=B",  "g2xh1=N",  "Bh5-g6",
			"Bh5-f7",  "Bh5-g4",   "Bh5-f3",   "Bh5-e2",   "Bh5-d1"};
		REQUIRE(moves.size() == trueMoves.size());
		for (auto& moveStr : trueMoves) {
			move_t move = MoveFromLongNotation(board.playerToMove(), moveStr);
			REQUIRE(std::find(moves.begin(), moves.end(), move) != moves.end());
		}
	}
}
