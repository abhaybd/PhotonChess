#include "history.h"

#include <algorithm>
#include <cstdlib>

namespace photon::engine {

history_table_t::history_table_t(int16_t maxBonus, int16_t depthFactor)
	: maxBonus(maxBonus), depthFactor(depthFactor) {}

void history_table_t::update(player_t player, move_t move, int depth, bool isBonus) {
	int bonus = depth * depth * depthFactor;
	if (!isBonus) {
		bonus = -bonus;
	}
	int16_t clampedBonus =
		static_cast<int16_t>(std::clamp(bonus, -maxBonus, static_cast<int>(maxBonus)));
	size_t playerIdx = static_cast<size_t>(player);
	auto& moveHistory = history[playerIdx][move.from][move.to];
	moveHistory += clampedBonus - moveHistory * std::abs(clampedBonus) / maxBonus;
}

void history_table_t::reset() {
	history = {};
}

int history_table_t::getHistoryScore(player_t player, move_t move) const {
	size_t playerIdx = static_cast<size_t>(player);
	return history[playerIdx][move.from][move.to];
}

} // namespace photon::engine
