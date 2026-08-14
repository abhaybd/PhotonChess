#pragma once

#include "photon/core.h"

namespace photon::engine {

constexpr int16_t PHASE_MG = 0;
constexpr int16_t PHASE_EG = 24;

/**
 * @brief Get the phase of the game, in the range [PHASE_MG, PHASE_EG].
 * @param board The board to get the phase of
 * @return int16_t The phase of the game
 */
int16_t GetPhase(const board_t& board);

/**
 * @brief Linearly interpolate (taper) between middlegame and endgame scores
 * based on the game phase.
 *
 * @param mg The middlegame score
 * @param eg The endgame score
 * @param phase The phase of the game
 * @return int16_t The interpolated score
 */
int16_t InterpolateScore(int16_t mg, int16_t eg, int16_t phase);

} // namespace photon::engine
