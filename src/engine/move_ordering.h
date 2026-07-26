#pragma once

#include "killer.h"
#include "photon/core.h"

namespace photon::engine {

/**
 * @brief A move and associated score.
 */
struct scoredmove_t {
	move_t move;
	uint score;
};

/**
 * @brief Score each move and return in a new vector.
 *
 * @param board The current board
 * @param moves The moves to score
 * @param tt_move The transposition table move to prioritize, if any
 * @param qSearch If true, the moves are being scored for a quiescence search
 * @return std::vector<scoredmove_t> The scored moves, where higher is better
 */
std::vector<scoredmove_t> ScoreMoves(const board_t& board, const std::vector<move_t>& moves,
									 const killer_table_t::killer_moves_t& killerMoves,
									 std::optional<move_t> tt_move, bool qSearch);

/**
 * @brief Select the best move to search from a list of scored moves, starting at @p startIdx
 *
 * This is the best move to search, not necessarily the best move to play.
 *
 * @param scoredMoves The list of scored moves to select from
 * @param startIdx The index to start at
 * @return move_t The best move to search
 */
move_t SelectMove(std::vector<scoredmove_t>& scoredMoves, size_t startIdx = 0);

} // namespace photon::engine
