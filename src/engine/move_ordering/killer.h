#pragma once

#include "photon/core.h"

#include <array>
#include <cstddef>
#include <ranges>
#include <vector>

namespace photon::engine {

constexpr size_t NUM_KILLER_MOVES = 2;

class killer_table_t {
public:
	/**
	 * @brief Construct a new killer table with a given size
	 *
	 * @param size Number of plies to store killers for
	 */
	explicit killer_table_t(size_t size);

	void add(move_t move, uint ply);

	bool isKiller(move_t move, uint ply) const;

	void reset();

private:
	/** Maps plies to killer moves. */
	std::vector<std::array<move_t, NUM_KILLER_MOVES>> killers;
	/** Number of valid killers for each ply, in range [0, NUM_KILLER_MOVES] */
	std::vector<uint> validKillers;

public:
	using killer_moves_t =
		std::ranges::subrange<decltype(killers)::value_type::const_iterator>;

	killer_moves_t getKillerMoves(uint ply) const;
};

} // namespace photon::engine
