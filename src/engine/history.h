#pragma once

#include "photon/core.h"

#include <array>

namespace photon::engine {

class history_table_t {
public:
	explicit history_table_t(int16_t maxBonus, int16_t depthFactor);

	void update(player_t player, move_t move, int depth, bool isBonus);

	void reset();

	int getHistoryScore(player_t player, move_t move) const;

private:
	int16_t maxBonus;
	int16_t depthFactor;
	std::array<std::array<std::array<int16_t, 64>, 64>, 2> history{};
};

} // namespace photon::engine
