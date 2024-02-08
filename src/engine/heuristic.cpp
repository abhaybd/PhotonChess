#include "photon/core.h"
#include "photon/engine/eval.h"
#include "pst.h"

#include <array>
#include <cassert>
#include <strings.h>
#include <utility>

namespace photon::engine {
namespace {

constexpr std::array<int, 6> PHASE_SCORE = {0, 1, 1, 2, 4, 0};

float PieceScore(const board_t& board, player_t player, piece_t piece, const pst_t& pst,
				 float phase) {
	assert(phase >= 0.0f && phase <= 1.0f);
	bitboard_t bb = board.getBitboard(player, piece);
	float score = 0.0f;
	while (bb) {
		int idx = ffsll(bb) - 1;
        // clever trick to flip the board
        int sq = player_t::white == player ? idx : idx ^ 56;
		score += pst[sq].first * (1.0f - phase) + pst[sq].second * phase;
		bb &= ~(1ULL << idx);
	}
	return score;
}

float GetPhase(const board_t& board) {
	int phaseScore = 0;
	for (size_t i = 0; i < ALL_PIECES.size(); i++) {
		piece_t piece = ALL_PIECES[i];
		bitboard_t bb = board.getBitboard(player_t::white, piece) |
						board.getBitboard(player_t::black, piece);
		phaseScore += PHASE_SCORE[i] * __builtin_popcountll(bb);
	}
    int phaseInt = 24 - std::min(phaseScore, 24);
	float phase = static_cast<float>(phaseInt) / 24.0f;
	return std::min(std::max(phase, 0.0f), 1.0f);
}

} // namespace

float PositionHeuristic(const board_t& board) {
	float score = 0.0f;
	float phase = GetPhase(board);

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
