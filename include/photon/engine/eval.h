#pragma once

#include "photon/core.h"
#include <vector>

namespace photon::engine {

struct evaluation_t {
  result_t result;
  float score;
  std::vector<move_t> moves;

  evaluation_t(evaluation_t&&) = default;
  evaluation_t& operator=(evaluation_t&&) = default;
};

float PositionHeuristic(const board_t& board);

evaluation_t EvalBoard(const board_t& board, int depth);

} // namespace photon::engine
