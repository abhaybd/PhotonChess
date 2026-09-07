#include "killer.h"

#include <algorithm>

namespace photon::engine {

killer_table_t::killer_table_t(size_t size) : killers(size), validKillers(size) {
	reset();
}

void killer_table_t::add(move_t move, unsigned int ply) {
	if (ply >= killers.size()) {
		return;
	}
	auto& moves = killers[ply];
	unsigned int& numValid = validKillers[ply];
	auto end = moves.begin() + numValid;
	auto it = std::find(moves.begin(), end, move);

	if (it == end) {
		if (numValid < moves.size()) {
			numValid++;
		}
		it = moves.begin() + numValid - 1;
	}

	while (it != moves.begin()) {
		*it = std::move(*(it - 1));
		--it;
	}
	*it = move;
}

bool killer_table_t::isKiller(move_t move, unsigned int ply) const {
	if (ply >= validKillers.size()) {
		return false;
	}
	const auto& moves = killers[ply];
	auto end = moves.begin() + validKillers[ply];
	return std::find(moves.begin(), end, move) != end;
}

void killer_table_t::reset() {
	for (auto& x : validKillers) {
		x = 0;
	}
}

killer_table_t::killer_moves_t killer_table_t::getKillerMoves(unsigned int ply) const {
	if (ply >= validKillers.size()) {
		return {};
	}
	const auto& moves = killers[ply];
	return std::ranges::subrange(moves.begin(), moves.begin() + validKillers[ply]);
}

} // namespace photon::engine
