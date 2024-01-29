#include "photon/util.h"

#include <loguru.hpp>

namespace photon::util {

player_t OtherPlayer(player_t player) {
	if (player == player_t::white) {
		return player_t::black;
	} else {
		return player_t::white;
	}
}

bool CheckOccupancy(bitboard_t bitboard, int idx) {
	return (bitboard & (1 << idx)) != 0;
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

uint8_t ParseSquare(std::string_view s) {
	CHECK_F(s.size() == 2);
	CHECK_F('a' <= s[0] && s[0] <= 'h');
	CHECK_F('1' <= s[1] && s[1] <= '8');
	int row = s[1] - '1';
	int col = s[0] - 'a';
	return row * 8 + col;
}

} // namespace photon::util
