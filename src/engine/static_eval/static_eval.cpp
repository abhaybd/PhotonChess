#include "material.h"
#include "pawn_structure.h"
#include "phase.h"

#include <photon/engine/eval.h>

namespace photon::engine {

int16_t PositionHeuristic(const board_t& board) {
	int16_t score = 0;
	int16_t phase = GetPhase(board);

	score += MaterialScore(board, phase);
	score += PawnStructureScore(board, phase);
	// TODO: space bonus
	// TODO: temp bonus

	return score;
}

} // namespace photon::engine
