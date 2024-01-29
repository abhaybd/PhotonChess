#pragma once

#include "photon/core.h"

namespace photon {

float EvalBoard(const board_t& board);

move_t FindMove(const board_t& board, player_t player, uint depth);

} // namespace photon
