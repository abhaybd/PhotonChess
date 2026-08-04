#include "photon/util.h"

#include "zobrist.h"

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

board_t MakeBoard(const std::vector<std::string_view>& parts) {
	CHECK_F(parts.size() == 6, "Malformed FEN string: has %zu parts, expected 6",
			parts.size());
	std::vector<std::string_view> rowsRev = split(parts[0], '/');
	board_t board;
	board.white.fill(0);
	board.black.fill(0);

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
	// e.p. should only be set if it can be played, so validate this
	if (board.enPassant >= 0) {
		player_t player = board.playerToMove();
		int square = board.enPassant + (player == player_t::white ? -8 : 8);
		bitboard_t row = 0xFFULL << (8 * (square / 8));
		bitboard_t mask = 1ULL << square;
		mask = ((mask << 1) | (mask >> 1)) & row;
		if (!(mask & board.getBitboard(player, piece_t::pawn))) {
			board.enPassant = -1;
		}
	}

	auto halfmoveRet = std::from_chars(parts[4].data(), parts[4].data() + parts[4].size(),
									   board.halfmoveClock);
	CHECK_F(halfmoveRet.ec == std::errc{}, "Unable to parse halfmove clock string: %s",
			std::string(parts[4]).c_str());
	auto fullmoveRet =
		std::from_chars(parts[5].data(), parts[5].data() + parts[5].size(), board.fullmove);
	CHECK_F(fullmoveRet.ec == std::errc{}, "Unable to parse fullmove string: %s",
			std::string(parts[5]).c_str());

	board.hash = ZobristHash(board);

	return board;
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

player_t SquareColor(uint8_t square) {
	int r = square / 8;
	int c = square % 8;
	return (r + c) % 2 == 0 ? player_t::black : player_t::white;
}

std::string SquareToString(uint8_t square) {
	int row = square / 8;
	int col = square % 8;
	std::string s;
	s.push_back('a' + col);
	s.push_back('1' + row);
	return s;
}

board_t MakeBoard(std::istream& stream) {
	std::vector<std::string> parts_str;
	for (int i = 0; i < 6; i++) {
		std::string s;
		stream >> s;
		CHECK_F(!s.empty() && !stream.fail(), "Invalid FEN string");
		parts_str.push_back(s);
	}
	std::vector<std::string_view> parts;
	for (const std::string& s : parts_str) {
		parts.push_back(s);
	}
	return MakeBoard(parts);
}

board_t MakeBoard(std::string_view fen) {
	std::vector<std::string_view> parts = split(fen, ' ');
	return MakeBoard(parts);
}

board_t DefaultBoard() {
	return MakeBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

uint64_t ZobristHash(const board_t& board) {
	auto& zobrist = ZobristData();

	uint64_t hash = 0;
	for (player_t player : {player_t::white, player_t::black}) {
		for (piece_t piece : ALL_PIECES) {
			bitboard_t bb = board.getBitboard(player, piece);
			while (bb != 0) {
				int idx = ffsll(bb) - 1;
				hash ^=
					zobrist.pieceKeys[static_cast<int>(player)][static_cast<int>(piece)][idx];
				bb &= bb - 1;
			}
		}
	}
	if (board.playerToMove() == player_t::black) {
		hash ^= zobrist.playerKey;
	}
	for (int i = 0; i < 4; i++) {
		if (board.metadata & (1 << i)) {
			hash ^= zobrist.castleKeys[i];
		}
	}
	if (board.enPassant >= 0) {
		hash ^= zobrist.enPassantKeys[board.enPassant % 8];
	}
	return hash;
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
		move.isCapture = longNotation[2] == 'x';
		move.to = ParseSquare(longNotation.substr(3, 2));
		size_t eqIdx = longNotation.rfind("=");
		if (eqIdx != std::string_view::npos) {
			piece_t p = CharToPiece(longNotation[eqIdx + 1]);
			move.promotion = static_cast<int8_t>(p);
		}
	}
	return move;
}

move_t MoveFromShortNotation(const board_t& board, std::string_view short_notation) {
	player_t player = board.playerToMove();
	if (short_notation == "O-O") {
		if (player == player_t::white) {
			return {ParseSquare("e1"), ParseSquare("g1")};
		} else {
			return {ParseSquare("e8"), ParseSquare("g8")};
		}
	} else if (short_notation == "O-O-O") {
		if (player == player_t::white) {
			return {ParseSquare("e1"), ParseSquare("c1")};
		} else {
			return {ParseSquare("e8"), ParseSquare("c8")};
		}
	} else {
		std::regex regex("^([NBRQK][a-h1-8]?x?|[a-h]x)?([a-h][1-8])(?:=([NBRQ]))?[+#]?$");
		std::match_results<std::string_view::const_iterator> m;
		bool match =
			std::regex_match(short_notation.cbegin(), short_notation.cend(), m, regex);
		CHECK_F(match, "Invalid short notation: %s", std::string(short_notation).c_str());

		uint8_t to = ParseSquare(m[2].str());
		std::optional<piece_t> promotion;
		if (!m[3].str().empty()) {
			promotion = CharToPiece(m[3].str()[0]);
		}

		piece_t piece;
		bool is_capture;
		std::optional<char> disambiguator;
		if (m[1].str().empty()) {
			is_capture = false;
			piece = piece_t::pawn;
		} else {
			std::string s = m[1];
			is_capture = s.back() == 'x';
			piece = std::islower(s[0]) ? piece_t::pawn : CharToPiece(s[0]);
			if (piece != piece_t::pawn && (s.size() == 3 || (!is_capture && s.size() == 2))) {
				disambiguator = s[1];
			}
		}

		std::vector<move_t> moves = board.moves();
		for (move_t move : moves) {
			if (move.to == to && move.getPiece(board) == piece &&
				move.isCapture == is_capture && move.getPromotion() == promotion) {
				if (disambiguator) {
					std::string square_str = SquareToString(move.from);
					if (std::isalpha(*disambiguator) && square_str[0] == *disambiguator) {
						return move;
					} else if (std::isdigit(*disambiguator) &&
							   square_str[1] == *disambiguator) {
						return move;
					}
				} else {
					return move;
				}
			}
		}
		CHECK_F(false, "No matching move found for short notation: %s",
				std::string(short_notation).c_str());
	}
}

move_t MoveFromUCI(std::string_view uci, bool isCapture) {
	CHECK_F(uci.size() == 4 || uci.size() == 5);
	uint8_t from = ParseSquare(uci.substr(0, 2));
	uint8_t to = ParseSquare(uci.substr(2, 2));
	int8_t promotion = -1;
	if (uci.size() == 5) {
		piece_t p = CharToPiece(uci[4]);
		promotion = static_cast<int8_t>(p);
	}
	return {from, to, promotion, isCapture};
}

move_t MoveFromUCI(const board_t& board, std::string_view uci) {
	move_t m = MoveFromUCI(uci, false);
	player_t player = CheckOccupancy(board.occupancyMap(player_t::white), m.from)
						  ? player_t::white
						  : player_t::black;
	CHECK_F(CheckOccupancy(board.occupancyMap(player), m.from));
	auto ep = board.availableEnPassant();
	if (ep && ep == m.to && m.getPiece(board) == piece_t::pawn) {
		m.isCapture = true;
	} else {
		m.isCapture = CheckOccupancy(board.occupancyMap(OtherPlayer(player)), m.to);
	}
	return m;
}

std::string MoveToLongNotation(board_t board, move_t move) {
	std::stringstream ss;
	if (move.isCastle(board, castle_t::king)) {
		ss << "O-O";
	} else if (move.isCastle(board, castle_t::queen)) {
		ss << "O-O-O";
	} else {
		piece_t p = move.getPiece(board);
		if (p != piece_t::pawn) {
			ss << PieceToChar(p);
		}
		ss << SquareToString(move.from);
		ss << (move.isCapture ? 'x' : '-');
		ss << SquareToString(move.to);
		if (move.isPromotion()) {
			ss << '=' << PieceToChar(*move.getPromotion());
		}
	}
	return ss.str();
}

std::string MoveToShortNotation(const board_t& board, move_t move) {
	std::stringstream ss;
	if (move.isCastle(board, castle_t::king)) {
		ss << "O-O";
	} else if (move.isCastle(board, castle_t::queen)) {
		ss << "O-O-O";
	} else {
		piece_t p = move.getPiece(board);
		if (p != piece_t::pawn) {
			ss << PieceToChar(p);
			if (move.isCapture) {
				ss << 'x';
			}
		} else if (move.isCapture) {
			ss << SquareToString(move.from)[0] << 'x';
		}
		ss << SquareToString(move.to);
		if (move.isPromotion()) {
			ss << '=' << PieceToChar(*move.getPromotion());
		}
	}
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
