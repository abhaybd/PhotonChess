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
		exact = 0,
		lower_bound = 1,
		upper_bound = 2
	};

	struct entry_t {
		int depth;
		int16_t score;
		entry_type_t type;
		move_t best_move;
	};

	explicit transposition_table_t(size_t size);

	std::optional<entry_t> get(const board_t& board, int plies) const;

	void set(const board_t& board, int depth, int plies, uint64_t rootPosHash, int16_t score,
			 entry_type_t type, move_t best_move);

private:
	struct packed_move_t {
		/** Start square */
		uint16_t from : 6;
		/** End square */
		uint16_t to : 6;
		/** 0 if not promotion, 1+piece_idx otherwise */
		uint16_t promotion : 3;
		/** 1 if capture, 0 otherwise */
		uint16_t isCapture : 1;
	};

	struct packed_entry_t {
		/** Upper 32 bits of the hash, sentinel 0 => invalid entry */
		uint32_t hash;
		uint8_t depth;
		uint8_t type;
		uint16_t rootPosHash;
		int16_t score;
		packed_move_t best_move;
	};

	entry_t unpackEntry(const packed_entry_t& entry) const;
	bool isEntryValid(const packed_entry_t& entry) const;
	move_t unpackMove(const packed_move_t& move) const;
	packed_move_t packMove(const move_t& move) const;
	uint32_t packHash(uint64_t hash) const;

	std::vector<packed_entry_t> table;
};

} // namespace photon::engine
