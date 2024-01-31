#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace photon {

// i-th bit corresponds to i-th square: a1, b1, ..., g8, h8
using bitboard_t = uint64_t;

enum class player_t {
	white,
	black
};

enum class castle_t {
	king,
	queen
};

enum class piece_t {
	pawn = 0,
	knight = 1,
	bishop = 2,
	rook = 3,
	queen = 4,
	king = 5
};

struct move_t;

struct board_t {
	board_t() : metadata(0), enPassant(-1), halfmoveClock(0), fullmove(0) {
		for (auto& x : white) {
			x = 0;
		}
		for (auto& x : black) {
			x = 0;
		}
	}

	std::array<bitboard_t, 6> white;
	std::array<bitboard_t, 6> black;
	/*
	 * 4 LSB are castling rights: [W kingside, W queenside, B kingside, B queenside]
	 * 5th bit is current player to move, 0 => W, 1 => B
	 */
	uint8_t metadata;
	// [0-63] is en passant square, -1 is not available
	int8_t enPassant;
	uint8_t halfmoveClock;
	int fullmove;

	const std::array<bitboard_t, 6>& getBitboards(player_t player) const;
	bitboard_t& getBitboard(player_t player, piece_t piece);
	bitboard_t getBitboard(player_t player, piece_t piece) const;
	bool hasCastlingRights(player_t player, castle_t castle) const;
	player_t playerToMove() const;
	std::optional<int> availableEnPassant() const;

	bitboard_t occupancyMap() const;
	bitboard_t occupancyMap(player_t player) const;

	std::string fen() const;
	std::vector<move_t> moves(player_t player) const;
	bool isSquareAttacked(player_t player, uint8_t square) const;
};

struct move_t {
	uint8_t from;
	uint8_t to;

	bool isCapture(const board_t& board) const;
	bool isEnPassant(const board_t& board) const;
	player_t getPlayer(const board_t& board) const;
	bool isCastle(const board_t& board) const;
	bool isCastle(const board_t& board, castle_t castle) const;

	bool operator==(const move_t& other) const;
};

board_t MakeBoard(std::string_view fen);
board_t DefaultBoard();
move_t MakeMove(player_t player, std::string_view longNotation);

} // namespace photon
