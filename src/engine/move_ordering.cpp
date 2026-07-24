#include "move_ordering.h"

#include <algorithm>
#include <loguru.hpp>

namespace photon::engine {
namespace {

/**
 * @brief The score associated with capturing a piece with another piece, for move ordering.
 *
 * Indexed by [attacker][victim].
 */
std::array<std::array<int, ALL_PIECES.size()>, ALL_PIECES.size()> mvv_lva = {
	{{15, 25, 35, 45, 55, 0},	// pawn
	 {14, 24, 34, 44, 54, 0},	// knight
	 {13, 23, 33, 43, 53, 0},	// bishop
	 {12, 22, 32, 42, 52, 0},	// rook
	 {11, 21, 31, 41, 51, 0},	// queen
	 {10, 20, 30, 40, 50, 0}}}; // king

int MVV_LVA(piece_t attacker, piece_t victim) {
	DCHECK_F(victim != piece_t::king);
	return mvv_lva[static_cast<int>(attacker)][static_cast<int>(victim)];
}

} // namespace

std::vector<scoredmove_t> ScoreMoves(const board_t& board, const std::vector<move_t>& moves,
									 std::optional<move_t> tt_move, bool qSearch) {
	std::vector<scoredmove_t> scoredMoves;
	scoredMoves.reserve(moves.size());
	for (move_t m : moves) {
		int score = 0;
		if (m.isCapture) {
			auto victimOpt = m.getCapturedPiece(board);
			DCHECK_F(victimOpt.has_value());
			score += MVV_LVA(m.getPiece(board), *victimOpt);
		}
		if (tt_move && m == *tt_move) {
			score += 100;
		}
		// TODO: add killer heuristic
		// if qsearch, only search captures and promotions
		if (!qSearch || m.isCapture || m.isPromotion()) {
			scoredMoves.push_back({m, score});
		}
	}
	return scoredMoves;
}

move_t SelectMove(std::vector<scoredmove_t>& scoredMoves, size_t startIdx) {
	for (size_t i = startIdx + 1; i < scoredMoves.size(); i++) {
		if (scoredMoves[i].score > scoredMoves[startIdx].score) {
			std::swap(scoredMoves[i], scoredMoves[startIdx]);
		}
	}
	return scoredMoves[startIdx].move;
}

} // namespace photon::engine
