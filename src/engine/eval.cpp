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
	std::chrono::high_resolution_clock::time_point startTime;
};

namespace {

constexpr float CHECKMATE_SCORE = 10000.0f;
constexpr size_t TTABLE_SIZE = 1ULL << 20;
constexpr int HARD_TIME_CHECK_INTERVAL = 10000;

bool operator<(const evaluation_t& a, const evaluation_t& b) {
	return a.score < b.score;
}

evaluation_t operator-(evaluation_t&& a) {
	evaluation_t ret = std::move(a);
	ret.score = -ret.score;
	return ret;
}

std::optional<evaluation_t> negamax(const board_t& board, const searchparams_t& params, int depth, int plies,
					 float alpha, float beta, evalmetrics_t& metrics, evalstate_t& state) {
	// update metrics
	metrics.nodes++;

	// enforce time limit
	if (params.maxTime && metrics.nodes % HARD_TIME_CHECK_INTERVAL == 0) {
		auto elapsed = std::chrono::high_resolution_clock::now() - state.startTime;
		if (elapsed >= params.maxTime->second) {
			LOG_F(INFO, "Hard time limit reached, stopping search");
			return std::nullopt;
		}
	}

	player_t player = board.playerToMove();
	result_t result = board.result();

	auto tt_entry = state.ttable.get(board);
	float original_alpha = alpha;
	if (tt_entry && tt_entry->depth >= depth) {
		switch (tt_entry->type) {
			case transposition_table_t::entry_type_t::exact:
				// TODO: load result from ttable
				return evaluation_t{result, tt_entry->score, {tt_entry->best_move}};

			case transposition_table_t::entry_type_t::lower_bound:
				alpha = std::max(alpha, tt_entry->score);
				break;

			case transposition_table_t::entry_type_t::upper_bound:
				beta = std::min(beta, tt_entry->score);
				break;
		}
		if (alpha >= beta) {
			return evaluation_t{result, tt_entry->score, {tt_entry->best_move}};
		}
	}

	// TODO: add quiescence search
	if (depth == 0 || result != result_t::none) {
		if (result == result_t::none) {
			float score = PositionHeuristic(board);
			if (player == player_t::black) {
				score = -score;
			}
			return evaluation_t{result, score, {}};
		} else if (result == WinResult(OtherPlayer(player))) {
			// penalize mated positions by the number of plies to the checkmate
			return evaluation_t{result, -CHECKMATE_SCORE + plies, {}};
		} else if (result == result_t::draw) {
			return evaluation_t{result, 0.0f, {}};
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
		auto candidateOpt = negamax(child, params, depth - 1, plies + 1, -beta, -alpha, metrics, state);
		if (!candidateOpt) {
			return std::nullopt;
		}
		auto candidate = -(*std::move(candidateOpt));
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
	return evalstate_ptr_t(new evalstate_t{transposition_table_t(TTABLE_SIZE), std::chrono::high_resolution_clock::now()});
}

std::pair<evaluation_t, evalmetrics_t>
EvalBoard(const board_t& board, const searchparams_t& params, evalstate_t& state) {
	evalmetrics_t metrics;
	float alpha = std::numeric_limits<float>::lowest();
	float beta = std::numeric_limits<float>::max();
	state.startTime = std::chrono::high_resolution_clock::now();

	CHECK_F(params.maxDepth.has_value() || params.maxTime.has_value(),
			"Either maxDepth or maxTime must be specified");
	CHECK_F(!params.maxTime || params.maxTime->first <= params.maxTime->second,
			"Soft time limit must be less than or equal to hard time limit");

	std::optional<evaluation_t> eval;
	for (int d = 1; d <= params.maxDepth.value_or(std::numeric_limits<int>::max()); d++) {
		auto evalOpt = negamax(board, params, d, 0, alpha, beta, metrics, state);
		if (!evalOpt) {
			// we're terminating early (e.g. time limit) so break out
			break;
		}
		eval = evalOpt;
		metrics.depth = d;
		if (params.maxTime) {
			auto elapsed = std::chrono::high_resolution_clock::now() - state.startTime;
			if (elapsed >= params.maxTime->first) {
				break;
			}
		}
	}
	CHECK_F(eval.has_value(), "Search terminated without returning a result");

	std::vector<move_t> moves(eval->moves.crbegin(), eval->moves.crend());
	eval->moves = std::move(moves);
	return std::make_pair(*eval, metrics);
}

std::pair<evaluation_t, evalmetrics_t> EvalBoard(const board_t& board,
												 const searchparams_t& params) {
	auto state = CreateEvalState();
	return EvalBoard(board, params, *state);
}

} // namespace photon::engine
