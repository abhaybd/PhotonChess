#include "material.h"
#include "pawn_structure.h"

#include <photon/engine/eval.h>

namespace photon::engine {

int16_t PositionHeuristic(const board_t& board) {
	int16_t score = 0;

	score += MaterialScore(board);
    score += PawnStructureScore(board);
	// TODO: space bonus
	// TODO: temp bonus

	return score;
}

} // namespace photon::engine
