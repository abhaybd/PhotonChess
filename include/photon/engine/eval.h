#pragma once

#include "photon/core.h"

#include <utility>
#include <vector>

namespace photon::engine {

struct evaluation_t {
	result_t result;
	float score;
	std::vector<move_t> moves;

	evaluation_t(const evaluation_t&) = default;
	evaluation_t(evaluation_t&&) = default;
	evaluation_t& operator=(evaluation_t&&) = default;
};

struct evalmetrics_t {
	int nodes;

	evalmetrics_t() : nodes(0) {}
	evalmetrics_t(const evalmetrics_t&) = default;
};

float PositionHeuristic(const board_t& board);

std::pair<evaluation_t, evalmetrics_t> EvalBoard(const board_t& board, int depth);

} // namespace photon::engine
