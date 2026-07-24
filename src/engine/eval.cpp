#include "photon/engine/eval.h"

#include "move_ordering.h"
#include "photon/core.h"
#include "photon/profile.h"
#include "photon/util.h"
#include "transposition_table.h"

#include <limits>
#include <loguru.hpp>

using namespace photon::util;

namespace photon::engine {

struct evalstate_t {
	transposition_table_t ttable;
	/** Hash of the root position of the search */
	uint64_t rootPosHash;
	std::chrono::high_resolution_clock::time_point startTime;
};

namespace {

constexpr int16_t SCORE_INF = std::numeric_limits<int16_t>::max();
constexpr int16_t CHECKMATE_SCORE = 30000; // should be < SCORE_INF
constexpr int MAX_PLIES = 1000;
// Any |score| >= this encodes a forced mate
constexpr int16_t MATE_SCORE_BOUND = CHECKMATE_SCORE - MAX_PLIES;
constexpr size_t TTABLE_SIZE = 1ULL << 22;
constexpr int HARD_TIME_CHECK_INTERVAL = 10000;

bool operator<(const evaluation_t& a, const evaluation_t& b) {
	return a.score < b.score;
}

evaluation_t operator-(evaluation_t&& a) {
	evaluation_t ret = std::move(a);
	// -2^15 cannot be negated (since max int16 is 2^15 - 1)
	DCHECK_F(ret.score != std::numeric_limits<int16_t>::lowest());
	ret.score = -ret.score;
	return ret;
}

std::optional<evaluation_t> negamax(board_t& board, const searchparams_t& params, int depth,
									int plies, int16_t alpha, int16_t beta,
									evalmetrics_t& metrics, evalstate_t& state) {
	PHOTON_PROFILE_FUNCTION();

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

	// TODO: is there a way to check for terminal states without generating all moves?
	std::vector<move_t> moves = board.moves();

	player_t player = board.playerToMove();
	result_t result = board.result(!moves.empty());

	if (result == WinResult(OtherPlayer(player))) {
		// penalize mated positions by the number of plies to the checkmate
		return evaluation_t{static_cast<int16_t>(-CHECKMATE_SCORE + plies), {}};
	} else if (result == result_t::draw) {
		return evaluation_t{0, {}};
	} else if (result == WinResult(player)) {
		ABORT_F("Player to move cannot already have checkmate! result == WinResult(player)");
	}

	auto tt_entry = state.ttable.get(board, plies);
	int16_t original_alpha = alpha;
	if (tt_entry && tt_entry->depth >= depth) {
		switch (tt_entry->type) {
			case transposition_table_t::entry_type_t::exact:
				return evaluation_t{tt_entry->score, {tt_entry->best_move}};

			case transposition_table_t::entry_type_t::lower_bound:
				alpha = std::max(alpha, tt_entry->score);
				break;

			case transposition_table_t::entry_type_t::upper_bound:
				beta = std::min(beta, tt_entry->score);
				break;
		}
		if (alpha >= beta) {
			return evaluation_t{tt_entry->score, {tt_entry->best_move}};
		}
	}

	bool qSearch =
		depth <= 0 && !board.inCheck(player); // short-circuits so inCheck not always called
	auto tt_move = tt_entry ? std::optional(tt_entry->best_move) : std::nullopt;
	std::vector<scoredmove_t> scoredMoves = ScoreMoves(board, moves, tt_move, qSearch);

	evaluation_t best = {-SCORE_INF, {}};

	if (qSearch) {
		// stand pat in qsearch
		int16_t score = PositionHeuristic(board);
		if (player == player_t::black) {
			score = -score;
		}
		if (score >= beta) {
			return evaluation_t{score, {}};
		}
		if (score > alpha) {
			alpha = score;
		}
		best.score = score;
	}

	for (size_t i = 0; i < scoredMoves.size(); i++) {
		move_t m = SelectMove(scoredMoves, i);
		auto handle = board.doMoveTemp(m);
		int d = std::max(depth - 1, 0);
		auto candidateOpt =
			negamax(board, params, d, plies + 1, -beta, -alpha, metrics, state);
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

	// don't update ttable if standing pat in qsearch
	if (!best.moves.empty()) {
		transposition_table_t::entry_type_t entry_type;
		if (best.score <= original_alpha) {
			entry_type = transposition_table_t::entry_type_t::upper_bound;
		} else if (best.score >= beta) {
			entry_type = transposition_table_t::entry_type_t::lower_bound;
		} else {
			entry_type = transposition_table_t::entry_type_t::exact;
		}
		state.ttable.set(board, depth, plies, state.rootPosHash, best.score, entry_type,
						 best.moves.back());
	}

	return best;
}

} // namespace

void evalstate_deleter_t::operator()(evalstate_t* state) const {
	delete state;
}

evalstate_ptr_t CreateEvalState() {
	return evalstate_ptr_t(new evalstate_t{transposition_table_t(TTABLE_SIZE), 0ULL,
										   std::chrono::high_resolution_clock::now()});
}

std::pair<evaluation_t, evalmetrics_t>
EvalBoard(const board_t& board, const searchparams_t& params, evalstate_t& state) {
	PHOTON_PROFILE_FUNCTION();
	board_t boardCopy = board;
	evalmetrics_t metrics;
	int16_t alpha = -SCORE_INF;
	int16_t beta = SCORE_INF;
	state.startTime = std::chrono::high_resolution_clock::now();
	state.rootPosHash = board.hash;

	CHECK_F(params.maxDepth.has_value() || params.maxTime.has_value(),
			"Either maxDepth or maxTime must be specified");
	CHECK_F(!params.maxTime || params.maxTime->first <= params.maxTime->second,
			"Soft time limit must be less than or equal to hard time limit");

	std::optional<evaluation_t> eval;
	int maxDepth = MAX_PLIES;
	if (params.maxDepth.has_value()) {
		if (*params.maxDepth < MAX_PLIES) {
			maxDepth = *params.maxDepth;
		} else {
			LOG_F(WARNING, "Requested max depth %d is greater than maximum allowable (%d)",
				  *params.maxDepth, MAX_PLIES);
		}
	}
	for (int d = 1; d <= maxDepth; d++) {
		auto evalOpt = negamax(boardCopy, params, d, 0, alpha, beta, metrics, state);
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

std::optional<int> ScoreToMateDistance(int16_t score) {
	if (score >= MATE_SCORE_BOUND) {
		return CHECKMATE_SCORE - score;
	} else if (score <= -MATE_SCORE_BOUND) {
		return CHECKMATE_SCORE + score;
	} else {
		return std::nullopt;
	}
}

} // namespace photon::engine
