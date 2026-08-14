#include "phase.h"

#include <bit>

namespace photon::engine {
namespace {

constexpr std::array<int, 6> PHASE_SCORE = {0, 1, 1, 2, 4, 0};

}

int16_t GetPhase(const board_t& board) {
	int16_t phaseScore = 0;
	for (size_t i = 0; i < ALL_PIECES.size(); i++) {
		piece_t piece = ALL_PIECES[i];
		bitboard_t bb = board.getBitboard(player_t::white, piece) |
						board.getBitboard(player_t::black, piece);
		phaseScore += PHASE_SCORE[i] * std::popcount(bb);
	}
	int16_t phase = PHASE_EG - std::min(phaseScore, PHASE_EG);
	return phase;
}

int16_t InterpolateScore(int16_t mg, int16_t eg, int16_t phase) {
	return (mg * (PHASE_EG - phase) + eg * phase) / PHASE_EG;
}

} // namespace photon::engine
