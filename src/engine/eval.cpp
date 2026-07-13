#include "photon/engine/eval.h"

#include "move_ordering.h"
#include "photon/core.h"
#include "photon/util.h"
#include "transposition_table.h"

#include <limits>
#include <loguru.hpp>

using namespace photon::util;

namespace photon::engine {

struct evalstate_t {
	transposition_table_t ttable;
};

namespace {

constexpr float CHECKMATE_SCORE = 10000.0f;
constexpr size_t TTABLE_SIZE = 1ULL << 20;

bool operator<(const evaluation_t& a, const evaluation_t& b) {
	return a.score < b.score;
}

evaluation_t operator-(evaluation_t&& a) {
	evaluation_t ret = std::move(a);
	ret.score = -ret.score;
	return ret;
}

evaluation_t negamax(const board_t& board, int depth, int plies, float alpha, float beta,
					 evalmetrics_t& metrics, evalstate_t& state) {
	// update metrics
	metrics.nodes++;

	player_t player = board.playerToMove();
	result_t result = board.result();

	auto tt_entry = state.ttable.get(board);
	float original_alpha = alpha;
	if (tt_entry && tt_entry->depth >= depth) {
		switch (tt_entry->type) {
			case transposition_table_t::entry_type_t::exact:
				return evaluation_t{result, tt_entry->score, {tt_entry->best_move}};

			case transposition_table_t::entry_type_t::lower_bound:
				alpha = std::max(alpha, tt_entry->score);
				break;

			case transposition_table_t::entry_type_t::upper_bound:
				beta = std::min(beta, tt_entry->score);
				break;
		}
		if (alpha >= beta) {
			return {result, tt_entry->score, {tt_entry->best_move}};
		}
	}

	// TODO: add quiescence search
	if (depth == 0 || result != result_t::none) {
		if (result == result_t::none) {
			float score = PositionHeuristic(board);
			if (player == player_t::black) {
				score = -score;
			}
			return {result, score, {}};
		} else if (result == WinResult(OtherPlayer(player))) {
			// penalize mated positions by the number of plies to the checkmate
			return {result, -CHECKMATE_SCORE + plies, {}};
		} else if (result == result_t::draw) {
			return {result, 0.0f, {}};
		} else {
			ABORT_F(
				"Player to move cannot already have checkmate! result == WinResult(player)");
		}
	}

	std::vector<move_t> moves = board.moves();
	auto tt_move = tt_entry ? std::optional(tt_entry->best_move) : std::nullopt;
	std::vector<scoredmove_t> scoredMoves = ScoreMoves(board, moves, tt_move);

	evaluation_t best = {result, std::numeric_limits<float>::lowest(), {}};
	for (size_t i = 0; i < scoredMoves.size(); i++) {
		move_t m = SelectMove(scoredMoves, i);
		board_t child = board.doMoveCopy(m);
		auto candidate = -negamax(child, depth - 1, plies + 1, -beta, -alpha, metrics, state);
		if (best < candidate) {
			best = std::move(candidate);
			best.moves.push_back(m);
		}
		alpha = std::max(alpha, best.score);
		if (alpha >= beta) {
			break;
		}
	}

	transposition_table_t::entry_type_t entry_type;
	if (best.score <= original_alpha) {
		entry_type = transposition_table_t::entry_type_t::upper_bound;
	} else if (best.score >= beta) {
		entry_type = transposition_table_t::entry_type_t::lower_bound;
	} else {
		entry_type = transposition_table_t::entry_type_t::exact;
	}
	state.ttable.set(board, depth, best.score, entry_type, best.moves.back());

	return best;
}

} // namespace

void evalstate_deleter_t::operator()(evalstate_t* state) const {
	delete state;
}

evalstate_ptr_t CreateEvalState() {
	return evalstate_ptr_t(new evalstate_t{transposition_table_t(TTABLE_SIZE)});
}

std::pair<evaluation_t, evalmetrics_t> EvalBoard(const board_t& board, int depth,
												 evalstate_t& state) {
	evalmetrics_t metrics;
	float alpha = std::numeric_limits<float>::lowest();
	float beta = std::numeric_limits<float>::max();
	for (int d = 1; d < depth; d++) {
		negamax(board, d, 0, alpha, beta, metrics, state);
	}
	evaluation_t eval = negamax(board, depth, 0, alpha, beta, metrics, state);
	std::vector<move_t> moves(eval.moves.crbegin(), eval.moves.crend());
	eval.moves = std::move(moves);
	return std::make_pair(eval, metrics);
}

std::pair<evaluation_t, evalmetrics_t> EvalBoard(const board_t& board, int depth) {
	auto state = CreateEvalState();
	return EvalBoard(board, depth, *state);
}

} // namespace photon::engine
