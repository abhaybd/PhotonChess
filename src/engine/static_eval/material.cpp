#include "material.h"

#include "phase.h"
#include "photon/core.h"
#include "photon/engine/eval.h"
#include "photon/profile.h"
#include "pst.h"

#include <array>
#include <loguru.hpp>
#include <strings.h>

namespace photon::engine {
namespace {

int16_t PieceScore(const board_t& board, player_t player, piece_t piece, const pst_t& pst,
				   int16_t phase) {
	DCHECK_F(phase >= PHASE_MG && phase <= PHASE_EG);
	bitboard_t bb = board.getBitboard(player, piece);
	int16_t score = 0;
	while (bb) {
		int idx = ffsll(bb) - 1;
		// clever trick to flip the board
		int sq = player_t::white == player ? idx : idx ^ 56;
		score += InterpolateScore(pst[sq].first, pst[sq].second, phase);
		bb &= ~(1ULL << idx);
	}
	return score;
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
