#include "transposition_table.h"

#include "photon/core.h"
#include "photon/engine/eval.h"

#include <bit>

namespace photon::engine {
namespace {
// The following functions are used to normalize entry scores for mates in the ttable
// relative to the current node, to correctly handle mate scores at different depths.
// The mate score is MATE_SCORE - plies, but if we hit that mate at a different depth,
// it needs to be adjusted.

int16_t ScoreToTT(int16_t score, int plies) {
	if (ScoreToMateDistance(score).has_value()) {
		if (score > 0) {
			return static_cast<int16_t>(score + plies);
		} else {
			return static_cast<int16_t>(score - plies);
		}
	}
	return score;
}

int16_t ScoreFromTT(int16_t score, int plies) {
	if (ScoreToMateDistance(score).has_value()) {
		if (score > 0) {
			return static_cast<int16_t>(score - plies);
		} else {
			return static_cast<int16_t>(score + plies);
		}
	}
	return score;
}
} // namespace

transposition_table_t::transposition_table_t(size_t size) : table(size), numValidEntries(0) {
	CHECK_F(std::has_single_bit(size), "Size must be a power of 2");
}

std::optional<transposition_table_t::entry_t> transposition_table_t::get(const board_t& board,
																		 int plies) const {
	size_t idx = board.hash & (table.size() - 1);
	const packed_entry_t& entry = table[idx];
	if (isEntryValid(entry) && entry.hash == packHash(board.hash)) {
		entry_t ret = unpackEntry(entry);
		ret.score = ScoreFromTT(ret.score, plies);
		return ret;
	} else {
		return std::nullopt;
	}
}

void transposition_table_t::set(const board_t& board, int depth, int plies,
								uint64_t rootPosHash, int16_t score, entry_type_t type,
								move_t best_move) {
	size_t idx = board.hash & (table.size() - 1);
	const packed_entry_t& entry = table[idx];
	uint16_t rootPosHashTrunc = static_cast<uint16_t>(rootPosHash & 0xFFFFULL);
	bool isNew = !isEntryValid(entry);
	if (isNew || depth >= entry.depth || rootPosHashTrunc != entry.rootPosHash) {
		if (isNew) {
			numValidEntries++;
		}
		score = ScoreToTT(score, plies);
		table[idx] = packed_entry_t{
			packHash(board.hash),
			static_cast<uint8_t>(depth),
			static_cast<uint8_t>(type),
			rootPosHashTrunc,
			score,
			packMove(best_move),
		};
	}
}

int transposition_table_t::getUsage() const {
	return static_cast<int>(numValidEntries * 1000 / table.size());
}

bool transposition_table_t::isEntryValid(const packed_entry_t& entry) const {
	return entry.hash != 0;
}

transposition_table_t::entry_t
transposition_table_t::unpackEntry(const packed_entry_t& entry) const {
	return entry_t{entry.depth, entry.score, static_cast<entry_type_t>(entry.type),
				   unpackMove(entry.best_move)};
}

move_t transposition_table_t::unpackMove(const packed_move_t& move) const {
	return move_t(move.from, move.to, static_cast<int8_t>(move.promotion) - 1, move.isCapture);
}

transposition_table_t::packed_move_t
transposition_table_t::packMove(const move_t& move) const {
	return packed_move_t{move.from, move.to, static_cast<uint16_t>(move.promotion + 1),
						 move.isCapture};
}

uint32_t transposition_table_t::packHash(uint64_t hash) const {
	// store the upper 32 bits of the hash, since index stores lower bits
	return static_cast<uint32_t>(hash >> 32);
}
} // namespace photon::engine