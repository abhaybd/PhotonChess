#include "nullmove.h"

#include "../../zobrist.h"
#include "photon/profile.h"

namespace photon::engine {

bool CanNullMove(const board_t& board, const std::vector<move_t>& pseudoLegalMoves) {
	PHOTON_PROFILE_FUNCTION();
	// we can null move if a nonpawn piece exists (minimize zugzwang chances)
	// and we're not in check
	player_t player = board.playerToMove();
	return (board.getBitboard(player, piece_t::knight) |
			board.getBitboard(player, piece_t::bishop) |
			board.getBitboard(player, piece_t::rook) |
			board.getBitboard(player, piece_t::queen)) != 0 &&
		   !board.inCheck(player) && board.result(pseudoLegalMoves) == result_t::none;
}

temp_nullmove_handle_t doNullMoveTemp(board_t& board) {
	return temp_nullmove_handle_t(&board);
}

temp_nullmove_handle_t::temp_nullmove_handle_t(board_t* board)
	: board(board), hash(board->hash), fullmove(board->fullmove), enPassant(board->enPassant),
	  lastIrreversibleMove(board->lastIrreversibleMove) {
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
	board->historyHashes.push_back(hash);
	// nullmoves are irreversible, to prevent spurious 3-fold repetition draws
	board->lastIrreversibleMove = board->historyHashes.size() - 1;
}

temp_nullmove_handle_t::temp_nullmove_handle_t(temp_nullmove_handle_t&& other) noexcept
	: board(other.board), hash(other.hash), fullmove(other.fullmove),
	  enPassant(other.enPassant), lastIrreversibleMove(other.lastIrreversibleMove) {
	other.board = nullptr;
}
temp_nullmove_handle_t::~temp_nullmove_handle_t() {
	if (board) {
		board->hash = hash;
		board->fullmove = fullmove;
		board->enPassant = enPassant;
		board->halfmoveClock--;
		board->metadata ^= 1 << 4;
		board->lastIrreversibleMove = lastIrreversibleMove;
		board->historyHashes.pop_back();
	}
}

} // namespace photon::engine
