#pragma once

#include "photon/core.h"

namespace photon::engine {

/**
 * @brief Perform Static Exchange Evaluation (SEE) for a given capture.
 *
 * Returns the value of a (possibly multi-move) exchange.
 *
 * @param board The board to evaluate the exchange on.
 * @param capture The capture to evaluate.
 * @return int16_t The value of the exchange, in centipawns.
 * @see https://www.chessprogramming.org/Static_Exchange_Evaluation
 */
int16_t EvaluateExchange(const board_t& board, move_t capture);

}; // namespace photon::engine
