#include "transposition_table.h"

#include <photon/engine/eval.h>

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

transposition_table_t::transposition_table_t(size_t size) : table(size) {
	CHECK_F(std::has_single_bit(size), "Size must be a power of 2");
}

std::optional<transposition_table_t::entry_t> transposition_table_t::get(const board_t& board,
																		 int plies) const {
	size_t idx = board.hash & (table.size() - 1);
	const auto& entry = table[idx];
	if (entry && entry->hash == board.hash) {
		auto ret = *entry;
		ret.score = ScoreFromTT(ret.score, plies);
		return ret;
	} else {
		return std::nullopt;
	}
}

void transposition_table_t::set(const board_t& board, int depth, int plies, int16_t score,
								entry_type_t type, move_t best_move) {
	uint64_t hash = board.hash;
	size_t idx = hash & (table.size() - 1);
	const auto& entry = table[idx];
	if (!entry || depth >= entry->depth) {
		score = ScoreToTT(score, plies);
		table[idx] = {hash, depth, score, type, best_move};
	}
}

} // namespace photon::engine