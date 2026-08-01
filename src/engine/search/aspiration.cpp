#include "aspiration.h"

#include <algorithm>
#include <loguru.hpp>

namespace photon::engine {

aspiration_window_t::state_t::operator bool() const noexcept {
	return valid;
}

aspiration_window_t::state_t* aspiration_window_t::state_t::operator->() noexcept {
	CHECK_F(valid);
	return this;
}

const aspiration_window_t::state_t* aspiration_window_t::state_t::operator->() const noexcept {
	CHECK_F(valid);
	return this;
}

aspiration_window_t::aspiration_window_t(int delta, int minDepth, int16_t scoreInf,
										 int16_t mateScoreBound)
	: delta(delta), minDepth(minDepth), scoreInf(scoreInf), mateScoreBound(mateScoreBound) {
	reset();
}

void aspiration_window_t::reset() {
	state.valid = false;
}

bool aspiration_window_t::update(int depth, int16_t score) {
	if (depth < minDepth || !state) {
		state = {true, depth, score, {score - delta, score + delta}};
		return false;
	}

	int16_t lastScore = state->score;
	auto& [alpha, beta] = state->window;
	state->depth = depth;

	// if failed low or high, re-search with expanded window
	// (half-open if mate found)
	if (score <= alpha) {
		if (score <= -mateScoreBound) {
			alpha = -scoreInf;
		} else {
			alpha = std::max(-scoreInf, alpha - (lastScore - alpha));
		}
	} else if (score >= beta) {
		if (score >= mateScoreBound) {
			beta = scoreInf;
		} else {
			beta = std::min(+scoreInf, beta + (beta - lastScore));
		}
	} else {
		// no re-search needed, recenter bounds around new score
		// to prepare for next depth
		alpha = std::max(-scoreInf, score - delta);
		beta = std::min(+scoreInf, score + delta);
		state->score = score;
		return false;
	}

	return true;
}

std::pair<int16_t, int16_t> aspiration_window_t::getWindow() const {
	if (state && state->depth >= minDepth) {
		return state->window;
	} else {
		return {-scoreInf, scoreInf};
	}
}

} // namespace photon::engine
