#include "nullmove.h"

#include "../zobrist.h"

namespace photon::engine {

bool CanNullMove(const board_t& board) {
	// we can null move if a nonpawn piece exists (minimize zugzwang chances)
	// and we're not in check
	player_t player = board.playerToMove();
	return (board.getBitboard(player, piece_t::knight) |
			board.getBitboard(player, piece_t::bishop) |
			board.getBitboard(player, piece_t::rook) |
			board.getBitboard(player, piece_t::queen)) != 0 &&
		   !board.inCheck(player);
}

temp_nullmove_handle_t doNullMoveTemp(board_t& board) {
	return temp_nullmove_handle_t(&board);
}

temp_nullmove_handle_t::temp_nullmove_handle_t(board_t* board)
	: board(board), hash(board->hash), fullmove(board->fullmove), enPassant(board->enPassant) {
	const auto& zobrist = util::ZobristData();

	board->halfmoveClock++;
	if (board->playerToMove() == player_t::black) {
		board->fullmove++;
	}
	board->metadata ^= 1 << 4;
	board->hash ^= zobrist.playerKey;
	if (board->enPassant >= 0) {
		board->hash ^= zobrist.enPassantKeys[board->enPassant % 8];
		board->enPassant = -1;
	}
}

temp_nullmove_handle_t::temp_nullmove_handle_t(temp_nullmove_handle_t&& other) noexcept
	: board(other.board), hash(other.hash), fullmove(other.fullmove),
	  enPassant(other.enPassant) {
	other.board = nullptr;
}
temp_nullmove_handle_t::~temp_nullmove_handle_t() {
	if (board) {
		board->hash = hash;
		board->fullmove = fullmove;
		board->enPassant = enPassant;
		board->halfmoveClock--;
		board->metadata ^= 1 << 4;
	}
}

} // namespace photon::engine
