#include "material.h"
#include "photon/engine/eval.h"

namespace photon::engine {

int16_t PositionHeuristic(const board_t& board) {
	int16_t score = 0;

	score += MaterialScore(board);

	return score;
}

} // namespace photon::engine
