#pragma once

#include "photon/core.h"

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace photon::engine {

struct evaluation_t {
	/** Score in centipawns */
	int16_t score;
	/** Principal variation in reverse order */
	std::vector<move_t> moves;
};

struct evalmetrics_t {
	int nodes = 0;
	int depth = 0;
	int ttableUsage = 0;
};

struct searchparams_t {
	bool ponder = false;
	std::optional<int> maxDepth;
	/** (soft, hard) time limits for search */
	std::optional<std::pair<std::chrono::milliseconds, std::chrono::milliseconds>> maxTime;
	std::optional<int> maxNodes;
	std::function<void(const evaluation_t&, const evalmetrics_t&)> onResult;
};

struct evalstate_t;
struct evalstate_deleter_t {
	void operator()(evalstate_t* state) const;
};
using evalstate_ptr_t = std::unique_ptr<evalstate_t, evalstate_deleter_t>;

evalstate_ptr_t CreateEvalState();

int16_t PositionHeuristic(const board_t& board);

void StopSearch(evalstate_t& state);

void PonderHit(evalstate_t& state);

std::pair<evaluation_t, evalmetrics_t> EvalBoard(const board_t& board,
												 const searchparams_t& params);

std::pair<evaluation_t, evalmetrics_t>
EvalBoard(const board_t& board, const searchparams_t& params, evalstate_t& state);

/**
 * @brief Get the mate distance from a score, if applicable.
 *
 * @param score The raw evaluation score
 * @return std::optional<int> The mate distance in plies, or std::nullopt if not applicable.
 */
std::optional<int> ScoreToMateDistance(int16_t score);

} // namespace photon::engine
