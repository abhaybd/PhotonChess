#pragma once

#include "chesspp.h"

namespace chesspp {

float EvalBoard(const board_t &board);

move_t FindMove(const board_t &board, player_t player, uint depth);

}