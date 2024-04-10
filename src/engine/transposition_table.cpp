#include "transposition_table.h"

namespace photon::engine {

transposition_table_t::transposition_table_t(size_t size) : table(size) {
	CHECK_F(size == (size & -size), "Size must be a power of 2");
}

std::optional<transposition_table_t::entry_t>
transposition_table_t::get(const board_t& board) const {
	size_t hash = util::ZobristHash(board);
	size_t idx = hash & (table.size() - 1);
	return table[idx]->hash == hash ? table[idx] : std::nullopt;
}

void transposition_table_t::set(const board_t& board, int depth, float score,
								entry_type_t type, move_t best_move) {
	size_t hash = util::ZobristHash(board);
	size_t idx = hash & (table.size() - 1);
	table[idx] = {hash, depth, score, type, best_move};
}

} // namespace photon::engine