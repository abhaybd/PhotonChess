#pragma once

#include "photon/core.h"
#include <vector>

namespace photon::engine {

struct evaluation_t {
  float score;
  std::vector<move_t> moves;
};

evaluation_t EvalBoard(const board_t& board, uint depth);

} // namespace photon::engine
