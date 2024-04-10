#pragma once

#include "photon/core.h"

#include <memory>
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

struct evalstate_t;
struct evalstate_deleter_t {
	void operator()(evalstate_t* state) const;
};
using evalstate_ptr_t = std::unique_ptr<evalstate_t, evalstate_deleter_t>;

evalstate_ptr_t CreateEvalState();

float PositionHeuristic(const board_t& board);

std::pair<evaluation_t, evalmetrics_t> EvalBoard(const board_t& board, int depth);

std::pair<evaluation_t, evalmetrics_t> EvalBoard(const board_t& board, int depth,
												 evalstate_t& state);

} // namespace photon::engine
