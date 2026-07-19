#pragma once

#include "photon/core.h"

#include <cstdint>
#include <loguru.hpp>
#include <optional>
#include <vector>

namespace photon::engine {

class transposition_table_t {
public:
	enum class entry_type_t {
		exact,
		lower_bound,
		upper_bound
	};

	struct entry_t {
		uint64_t hash;
		int depth;
		int16_t score;
		entry_type_t type;
		move_t best_move;
	};

	explicit transposition_table_t(size_t size);

	std::optional<entry_t> get(const board_t& board) const;

	void set(const board_t& board, int depth, int16_t score, entry_type_t type,
			 move_t best_move);

private:
	std::vector<std::optional<entry_t>> table;
};

} // namespace photon::engine
