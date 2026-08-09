#include "photon/core.h"
#include "photon/engine/eval.h"
#include "photon/profile.h"
#include "pst.h"

#include <array>
#include <loguru.hpp>
#include <strings.h>

namespace photon::engine {
namespace {

constexpr std::array<int, 6> PHASE_SCORE = {0, 1, 1, 2, 4, 0};

int16_t PieceScore(const board_t& board, player_t player, piece_t piece, const pst_t& pst,
				   int16_t phase) {
	DCHECK_F(phase >= 0 && phase <= 24);
	bitboard_t bb = board.getBitboard(player, piece);
	int16_t score = 0;
	while (bb) {
		int idx = ffsll(bb) - 1;
		// clever trick to flip the board
		int sq = player_t::white == player ? idx : idx ^ 56;
		// linearly interpolate, but use integer arithmetic
		score += (pst[sq].first * (24 - phase) + pst[sq].second * phase) / 24;
		bb &= ~(1ULL << idx);
	}
	return score;
}

/**
 * @brief Get the phase of the game, in the range [0, 24], where 0 is the middlegame and 24 is
 * the endgame.
 *
 * @param board The board to get the phase of
 * @return int16_t The phase of the game
 */
int16_t GetPhase(const board_t& board) {
	int16_t phaseScore = 0;
	for (size_t i = 0; i < ALL_PIECES.size(); i++) {
		piece_t piece = ALL_PIECES[i];
		bitboard_t bb = board.getBitboard(player_t::white, piece) |
						board.getBitboard(player_t::black, piece);
		phaseScore += PHASE_SCORE[i] * std::popcount(bb);
	}
	int16_t phase = 24 - std::min(phaseScore, static_cast<int16_t>(24));
	return phase;
}

} // namespace

int16_t PositionHeuristic(const board_t& board) {
	PHOTON_PROFILE_FUNCTION();

	int16_t score = 0;
	int16_t phase = GetPhase(board);

	score += PieceScore(board, player_t::white, piece_t::pawn, PAWN_PST, phase);
	score -= PieceScore(board, player_t::black, piece_t::pawn, PAWN_PST, phase);
	score += PieceScore(board, player_t::white, piece_t::knight, KNIGHT_PST, phase);
	score -= PieceScore(board, player_t::black, piece_t::knight, KNIGHT_PST, phase);
	score += PieceScore(board, player_t::white, piece_t::bishop, BISHOP_PST, phase);
	score -= PieceScore(board, player_t::black, piece_t::bishop, BISHOP_PST, phase);
	score += PieceScore(board, player_t::white, piece_t::rook, ROOK_PST, phase);
	score -= PieceScore(board, player_t::black, piece_t::rook, ROOK_PST, phase);
	score += PieceScore(board, player_t::white, piece_t::queen, QUEEN_PST, phase);
	score -= PieceScore(board, player_t::black, piece_t::queen, QUEEN_PST, phase);
	score += PieceScore(board, player_t::white, piece_t::king, KING_PST, phase);
	score -= PieceScore(board, player_t::black, piece_t::king, KING_PST, phase);

	return score;
}

} // namespace photon::engine
