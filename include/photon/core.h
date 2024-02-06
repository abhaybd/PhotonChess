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

enum result_t {
	white_wins,
	black_wins,
	draw,
	none
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

constexpr std::array<piece_t, 6> ALL_PIECES = {piece_t::pawn, piece_t::knight, piece_t::bishop,
											   piece_t::rook, piece_t::queen,  piece_t::king};

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
	bool inCheck(player_t player) const;
	result_t result() const;

	uint8_t getKing(player_t player) const;
	bitboard_t occupancyMap() const;
	bitboard_t occupancyMap(player_t player) const;

	board_t& doMove(move_t move);
	board_t doMoveCopy(move_t move) const;

	std::string fen() const;
	std::vector<move_t> moves() const;
	bool isSquareAttacked(player_t player, uint8_t square) const;
};

struct move_t {
	uint8_t from;
	uint8_t to;
	// -1 if not a promotion, otherwise the integral equivalent of the piece_t value
	int8_t promotion;

	move_t(uint8_t from, uint8_t to, int8_t promotion=-1);

	bool isCapture(const board_t& board) const;
	bool isEnPassant(const board_t& board) const;
	player_t getPlayer(const board_t& board) const;
	piece_t getPiece(const board_t& board) const;
	std::optional<piece_t> getCapturedPiece(const board_t& board) const;
	bool isCastle(const board_t& board) const;
	bool isCastle(const board_t& board, castle_t castle) const;
	bool isPromotion() const;
	std::optional<piece_t> getPromotion() const;

	bool operator==(const move_t& other) const;
};

} // namespace photon
