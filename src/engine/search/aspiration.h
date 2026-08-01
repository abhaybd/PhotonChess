#pragma once

#include <cstdint>
#include <utility>

namespace photon::engine {

class aspiration_window_t {
public:
	aspiration_window_t(int delta, int minDepth, int16_t scoreInf, int16_t mateScoreBound);

	void reset();

	/**
	 * @brief Update the aspiration window with a new score at a given depth.
	 *
	 * @param depth The depth of the search.
	 * @param score The estimated score of the position.
	 * @return true iff a re-search is required.
	 */
	bool update(int depth, int16_t score);

	/**
	 * @brief Get the current search window.
	 *
	 * @return std::pair<int16_t, int16_t> [alpha, beta] search window.
	 */
	std::pair<int16_t, int16_t> getWindow() const;

private:
	struct state_t {
		bool valid = false;
		int depth;
		int16_t score;
		std::pair<int16_t, int16_t> window;

		explicit operator bool() const noexcept;
		state_t* operator->() noexcept;
		const state_t* operator->() const noexcept;
	};

	const int delta;
	const int minDepth;
	const int16_t scoreInf;
	const int16_t mateScoreBound;
	state_t state{};
};

} // namespace photon::engine
