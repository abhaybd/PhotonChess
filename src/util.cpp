#include "photon/util.h"

#include <charconv>
#include <loguru.hpp>
#include <regex>
#include <sstream>

namespace photon::util {
namespace {

std::vector<std::string_view> split(std::string_view s, char delim) {
	std::vector<std::string_view> vec;
	std::size_t idx = 0;
	while (true) {
		std::size_t nextIdx = s.find(delim, idx);
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

player_t OtherPlayer(player_t player) {
	if (player == player_t::white) {
		return player_t::black;
	} else {
		return player_t::white;
	}
}

bool CheckOccupancy(bitboard_t bitboard, int idx) {
	return (bitboard & (1ULL << idx)) != 0;
}

char PieceToChar(piece_t piece) {
	switch (piece) {
		case piece_t::pawn:
			return 'P';
		case piece_t::knight:
			return 'N';
		case piece_t::bishop:
			return 'B';
		case piece_t::rook:
			return 'R';
		case piece_t::queen:
			return 'Q';
		case piece_t::king:
			return 'K';
		default:
			CHECK_F(false);
	}
}

piece_t CharToPiece(char c) {
	switch (std::toupper(c)) {
		case 'P':
			return piece_t::pawn;
		case 'N':
			return piece_t::knight;
		case 'B':
			return piece_t::bishop;
		case 'R':
			return piece_t::rook;
		case 'Q':
			return piece_t::queen;
		case 'K':
			return piece_t::king;
		default:
			ABORT_F("Unknown character piece: %c", c);
	}
}

result_t WinResult(player_t player) {
	switch (player) {
		case player_t::white:
			return result_t::white_wins;
		case player_t::black:
			return result_t::black_wins;
		default:
			ABORT_F("Unknown player type: %d", static_cast<int>(player));
	}
}

uint8_t ParseSquare(std::string_view s) {
	CHECK_F(s.size() == 2);
	CHECK_F('a' <= s[0] && s[0] <= 'h');
	CHECK_F('1' <= s[1] && s[1] <= '8');
	int row = s[1] - '1';
	int col = s[0] - 'a';
	return row * 8 + col;
}

std::string SquareToString(uint8_t square) {
	int row = square / 8;
	int col = square % 8;
	std::string s;
	s.push_back('a' + col);
	s.push_back('1' + row);
	return s;
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
				arr[static_cast<int>(p)] |= 1ULL << squareIdx;
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
	CHECK_F(halfmoveRet.ec == std::errc{}, "Unable to parse halfmove clock string: %s",
			std::string(parts[4]).c_str());
	auto fullmoveRet = std::from_chars(parts[5].begin(), parts[5].end(), board.fullmove);
	CHECK_F(fullmoveRet.ec == std::errc{}, "Unable to parse fullmove string: %s",
			std::string(parts[5]).c_str());

	return board;
}

board_t DefaultBoard() {
	return MakeBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

move_t MoveFromLongNotation(player_t player, std::string_view longNotation) {
	move_t move = {0, 0};
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
		std::regex regex("^[NBRQK]?[a-h][1-8][-x][a-h][1-8](?:=[NBRQK])?[+#]?$");
		bool match = std::regex_match(longNotation.cbegin(), longNotation.cend(), regex);
		CHECK_F(match, "Invalid long notation: %s", std::string(longNotation).c_str());
		if (std::isupper(longNotation[0])) {
			longNotation = longNotation.substr(1);
		}
		move.from = ParseSquare(longNotation.substr(0, 2));
		move.to = ParseSquare(longNotation.substr(3, 2));
		size_t eqIdx = longNotation.rfind("=");
		if (eqIdx != std::string_view::npos) {
			piece_t p = CharToPiece(longNotation[eqIdx + 1]);
			move.promotion = static_cast<int8_t>(p);
		}
	}
	return move;
}

move_t MoveFromUCI(std::string_view uci) {
	CHECK_F(uci.size() == 4 || uci.size() == 5);
	uint8_t from = ParseSquare(uci.substr(0, 2));
	uint8_t to = ParseSquare(uci.substr(2, 2));
	int8_t promotion = -1;
	if (uci.size() == 5) {
		piece_t p = CharToPiece(uci[4]);
		promotion = static_cast<int8_t>(p);
	}
	return {from, to, promotion};
}

std::string MoveToLongNotation(board_t board, move_t move) {
	piece_t p = move.getPiece(board);
	std::stringstream ss;
	if (p != piece_t::pawn) {
		ss << PieceToChar(p);
	}
	ss << SquareToString(move.from);
	ss << (move.isCapture(board) ? 'x' : '-');
	ss << SquareToString(move.to);
	return ss.str();
}

std::string MoveToUCI(move_t move) {
	std::stringstream ss;
	ss << SquareToString(move.from);
	ss << SquareToString(move.to);
	if (move.isPromotion()) {
		char c = PieceToChar(*move.getPromotion());
		ss << std::tolower(c, std::locale());
	}
	return ss.str();
}

} // namespace photon::util
