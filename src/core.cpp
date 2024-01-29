#include "chesspp/core.h"
#include "chesspp/util.h"

#include <assert.h>
#include <charconv>
#include <loguru.hpp>

using namespace chesspp::util;

namespace chesspp {

namespace {

std::vector<std::string_view> split(std::string_view s, char delim) {
	std::vector<std::string_view> vec;
	int idx = 0;
	while (true) {
		int nextIdx = s.find(delim, idx);
		if (nextIdx != std::string_view::npos) {
			vec.push_back(s.substr(idx, nextIdx - idx));
			idx = nextIdx + 1;
		} else {
			vec.push_back(s.substr(idx));
			break;
		}
	}
	return vec;
}

} // namespace

const std::array<bitboard_t, 6>& board_t::getBitboards(player_t player) const {
	switch (player) {
		case player_t::white:
			return white;
		case player_t::black:
			return black;
		default:
			assert(false);
	}
}

bitboard_t board_t::getBitboard(player_t player, piece_t piece) const {
	return getBitboards(player)[static_cast<int>(piece)];
}

std::string board_t::fen() const {
	// TODO: implement
	return "";
}

bool board_t::hasCastlingRights(player_t player, castle_t castle) const {
	int idx = 0;
	if (player == player_t::black) {
		idx += 2;
	}
	if (castle == castle_t::queen) {
		idx += 1;
	}
	return (metadata & (1 << idx)) != 0;
}

player_t board_t::playerToMove() const {
	return (metadata & (1 << 5)) == 0 ? player_t::white : player_t::black;
}

std::optional<int> board_t::availableEnPassant() const {
	if (enPassant >= 0) {
		return enPassant;
	} else {
		return std::nullopt;
	}
}

bitboard_t board_t::occupancyMap() const {
	return occupancyMap(player_t::white) | occupancyMap(player_t::black);
}

bitboard_t board_t::occupancyMap(player_t player) const {
	bitboard_t ret = 0;
	for (bitboard_t board : getBitboards(player)) {
		ret |= board;
	}
	return ret;
}

std::vector<move_t> board_t::moves() const {
	// TODO: implement
	return {};
}

bool move_t::isCapture(const board_t& board) const {
	player_t player = getPlayer(board);
	if (CheckOccupancy(board.occupancyMap(player), from) &&
		CheckOccupancy(board.occupancyMap(OtherPlayer(player)), to)) {
		return true;
	}
	return isEnPassant(board);
}

bool move_t::isEnPassant(const board_t& board) const {
	// check that en passant is available and this move goes to that square
	auto ep = board.availableEnPassant();
	if (!ep || ep != to) {
		return false;
	}
	// if this move is a pawn move, it must be en passant
	player_t player = getPlayer(board);
	return CheckOccupancy(board.getBitboard(player, piece_t::pawn), from);
}

player_t move_t::getPlayer(const board_t& board) const {
	player_t player = board.playerToMove();
	assert(CheckOccupancy(board.occupancyMap(player), from));
	return player;
}

bool move_t::isCastle(const board_t& board) const {
	return isCastle(board, castle_t::king) || isCastle(board, castle_t::queen);
}

bool move_t::isCastle(const board_t& board, castle_t castle) const {
	player_t player = getPlayer(board);
	if (!board.hasCastlingRights(player, castle)) {
		return false;
	}

	if (!CheckOccupancy(board.getBitboard(player, piece_t::king), from)) {
		return false;
	}

	if (from / 8 != to / 8) {
		return false;
	}

	int toCol = to % 8;
	return castle == castle_t::king ? toCol == 6 : toCol == 2;
}

bool move_t::operator==(const move_t& other) const {
	return from == other.from && to == other.to;
}

board_t MakeBoard(std::string_view fen) {
	std::vector<std::string_view> parts = split(fen, ' ');
	std::vector<std::string_view> rowsRev = split(parts[0], '/');

	board_t board;

	int squareIdx = 0;
	for (auto it = rowsRev.crbegin(); it < rowsRev.crend(); ++it) {
		for (char c : *it) {
			if (std::isdigit(c)) {
				squareIdx += c - '0';
			} else {
				bool isWhite = std::isupper(c);
				if (!isWhite) {
					c = std::toupper(c);
				}

				auto& arr = isWhite ? board.white : board.black;
				piece_t p = CharToPiece(c);
				arr[static_cast<int>(p)] |= 1L << squareIdx;
				squareIdx++;
			}
		}
	}

	if (parts[1] == "b") {
		board.metadata |= 1 << 4;
	}

	if (parts[2].find('K') != std::string_view::npos) {
		board.metadata |= 1;
	}
	if (parts[2].find('Q') != std::string_view::npos) {
		board.metadata |= 1 << 1;
	}
	if (parts[2].find('k') != std::string_view::npos) {
		board.metadata |= 1 << 2;
	}
	if (parts[2].find('q') != std::string_view::npos) {
		board.metadata |= 1 << 3;
	}

	board.enPassant = parts[3] == "-" ? -1 : ParseSquare(parts[3]);
	auto halfmoveRet = std::from_chars(parts[4].begin(), parts[4].end(), board.halfmoveClock);
	CHECK_F(halfmoveRet.ec == std::errc{}, "Unable to parse halfmove clock string: %.*s",
			static_cast<int>(parts[4].length()), parts[4].data());
	auto fullmoveRet = std::from_chars(parts[5].begin(), parts[5].end(), board.fullmove);
	CHECK_F(fullmoveRet.ec == std::errc{}, "Unable to parse fullmove string: %.*s",
			static_cast<int>(parts[5].length()), parts[5].data());

	return board;
}

board_t DefaultBoard() {
	return MakeBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

move_t MakeMove(player_t player, std::string_view longNotation) {
	move_t move;
	if (longNotation == "O-O") {
		if (player == player_t::white) {
			move.from = ParseSquare("e1");
			move.to = ParseSquare("g1");
		} else {
			move.from = ParseSquare("e8");
			move.to = ParseSquare("g8");
		}
	} else if (longNotation == "O-O-O") {
		if (player == player_t::white) {
			move.from = ParseSquare("e1");
			move.to = ParseSquare("c1");
		} else {
			move.from = ParseSquare("e8");
			move.to = ParseSquare("c8");
		}
	} else {
		// TODO: add promotion
		CHECK_F(longNotation.length() == 5 || longNotation.length() == 6,
				"Invalid format for long notation: %.*s",
				static_cast<int>(longNotation.length()), longNotation.data());
		if (longNotation.length() == 6) {
			CharToPiece(longNotation[0]);
			longNotation = longNotation.substr(1);
		}
		move.from = ParseSquare(longNotation.substr(0, 2));
		move.to = ParseSquare(longNotation.substr(3));
	}
	return move;
}

} // namespace chesspp
