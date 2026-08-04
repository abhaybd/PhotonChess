#include "photon/engine/eval.h"

#include "move_ordering/history.h"
#include "move_ordering/killer.h"
#include "move_ordering/move_ordering.h"
#include "photon/core.h"
#include "photon/profile.h"
#include "photon/util.h"
#include "search/aspiration.h"
#include "search/lmr.h"
#include "search/nullmove.h"
#include "search/transposition_table.h"

#include <algorithm>
#include <atomic>
#include <limits>
#include <loguru.hpp>

using namespace photon::util;

namespace {

using clock = std::chrono::steady_clock;

}

namespace photon::engine {

struct evalstate_t {
	transposition_table_t ttable;
	killer_table_t killerTable;
	history_table_t historyTable;
	/** Hash of the root position of the search */
	uint64_t rootPosHash;
	clock::time_point startTime;
	bool ponder;
	std::atomic_flag stop;
	std::atomic_flag ponderHit;
};

namespace {

constexpr int16_t SCORE_INF = std::numeric_limits<int16_t>::max();
constexpr int16_t CHECKMATE_SCORE = 30000; // should be < SCORE_INF
constexpr int MAX_PLIES = 1000;
constexpr size_t KILLER_TABLE_SIZE = 20;
constexpr int16_t MAX_HISTORY_BONUS = 16384;
constexpr int16_t HISTORY_DEPTH_FACTOR = 16;
constexpr int NMP_REDUCTION = 3;
constexpr int ASPIRATION_WINDOW_DELTA = 50; // initial window half-size, centipawns
constexpr int ASPIRATION_MIN_DEPTH = 3;
constexpr int LMR_MIN_DEPTH = 3;
constexpr int LMR_MIN_IDX = 2;
constexpr int LMR_REDUCTION = 1;
// Any |score| >= this encodes a forced mate
constexpr int16_t MATE_SCORE_BOUND = CHECKMATE_SCORE - MAX_PLIES;
constexpr size_t TTABLE_SIZE = 1ULL << 22;
constexpr int STOP_CHECK_INTERVAL = 1024;
constexpr int TIMEOUT_CHECK_INTERVAL = 10000;

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

std::optional<evaluation_t> operator-(std::optional<evaluation_t>&& a) {
	if (!a) {
		return std::nullopt;
	}
	return -std::move(*a);
}

/**
 * Get the score for a mate in the given number of plies of the current player.
 * Note that the score is negative since the current player is getting mated.
 */
int16_t MateDistanceToScore(int plies) {
	return static_cast<int16_t>(-CHECKMATE_SCORE + plies);
}

// TODO: move some params to dedicated search stack with struct
template <bool isPV>
std::optional<evaluation_t> pvs(board_t& board, const searchparams_t& params, int depth,
								int plies, int16_t alpha, int16_t beta, bool justNullMoved,
								bool allowNullMove, evalmetrics_t& metrics,
								evalstate_t& state) {
	PHOTON_PROFILE_FUNCTION();

	// update metrics
	metrics.nodes++;

	// enforce search limits
	if (!state.ponder && params.maxNodes && metrics.nodes >= *params.maxNodes) {
		LOG_F(INFO, "Node limit reached, stopping search");
		return std::nullopt;
	}
	if (metrics.nodes % TIMEOUT_CHECK_INTERVAL == 0) {
		if (!state.ponder && params.maxTime) {
			auto elapsed = clock::now() - state.startTime;
			if (elapsed >= params.maxTime->second) {
				LOG_F(INFO, "Hard time limit reached, stopping search");
				return std::nullopt;
			}
		}
	}
	if (metrics.nodes % STOP_CHECK_INTERVAL == 0) {
		if (state.stop.test()) {
			LOG_F(INFO, "Stop requested, stopping search");
			state.stop.clear();
			return std::nullopt;
		}
		if (state.ponder && state.ponderHit.test()) {
			state.ponder = false;
			state.startTime = clock::now();
		}
	}

	// TODO: implement endgame tablebases
	player_t player = board.playerToMove();
	bool inCheck = board.inCheck(player);
	std::vector<move_t> plMoves = board.pseudoLegalMoves();

	// handle terminal conditions that don't require legal move generation
	if (board.isNonStalemateDraw()) {
		return evaluation_t{0, {}};
	}

	auto tt_entry = state.ttable.get(board, plies);
	int16_t original_alpha = alpha;
	// disable TT cutoffs on PV nodes, since that causes scout results to prune PV branches
	if (!isPV && tt_entry && tt_entry->depth >= depth) {
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

	if (justNullMoved && depth <= 0) {
		// don't allow going from nullmove to qsearch, so return static eval if needed
		bool hasLegalMoves =
			std::any_of(plMoves.begin(), plMoves.end(),
						[&board](const move_t& m) { return board.isLegal(m); });
		if (!hasLegalMoves) {
			if (inCheck) {
				return evaluation_t{MateDistanceToScore(plies), {}};
			} else {
				return evaluation_t{0, {}};
			}
		} else {
			int16_t score = PositionHeuristic(board);
			if (player == player_t::black) {
				score = -score;
			}
			return evaluation_t{score, {}};
		}
	}

	bool qSearch = depth <= 0 && !inCheck;
	evaluation_t best = {-SCORE_INF, {}};

	if (qSearch) {
		// stand pat in qsearch
		int16_t score = PositionHeuristic(board);
		if (player == player_t::black) {
			score = -score;
		}
		if (score >= beta) {
			// make sure we're not in a terminal position
			result_t result = board.result();
			if (result == WinResult(OtherPlayer(player))) {
				return evaluation_t{MateDistanceToScore(plies), {}};
			} else if (result == result_t::draw) {
				return evaluation_t{0, {}};
			} else if (result == result_t::none) {
				return evaluation_t{score, {}};
			} else {
				ABORT_F("Player to move cannot already have checkmate! result == "
						"WinResult(player)");
			}
		}
		if (score > alpha) {
			alpha = score;
		}
		best.score = score;
	}

	// do null-move pruning
	// TODO: add eval >= beta condition?
	if (!isPV && allowNullMove && beta - alpha == 1 && !justNullMoved &&
		depth >= NMP_REDUCTION && CanNullMove(board)) {
		int d = depth - NMP_REDUCTION;
		std::optional<evaluation_t> candidate;
		{
			auto nmHandle = doNullMoveTemp(board);
			candidate = -pvs<false>(board, params, d, plies + 1, -beta, -beta + 1, true, true,
									metrics, state);
		}
		if (!candidate) {
			return std::nullopt;
		}
		// if NMP would cause cutoff, verify the result with a reduced null-window search
		if (candidate->score >= beta) {
			// disable NMP in verification search
			auto verified =
				pvs<false>(board, params, d, plies, alpha, beta, false, false, metrics, state);
			if (!verified) {
				return std::nullopt;
			}
			if (verified->score >= beta) {
				if (verified->score >= MATE_SCORE_BOUND) {
					verified->score = beta;
				}
				return evaluation_t{verified->score, {}};
			}
		}
	}

	auto tt_move = tt_entry ? std::optional(tt_entry->best_move) : std::nullopt;
	auto killerMoves = state.killerTable.getKillerMoves(plies);
	std::vector<scoredmove_t> scoredMoves =
		ScoreMoves(board, plMoves, killerMoves, state.historyTable, tt_move, qSearch);

	std::vector<move_t> legalMoves;
	legalMoves.reserve(scoredMoves.size());
	lmr_t lmr(LMR_MIN_DEPTH, LMR_MIN_IDX, LMR_REDUCTION);
	for (size_t i = 0; i < scoredMoves.size(); i++) {
		move_t m = SelectMove(scoredMoves, i);
		if (!board.isLegal(m)) {
			continue;
		}
		size_t numLegalMovesSearched = legalMoves.size();
		legalMoves.push_back(m);
		auto handle = board.doMoveTemp(m);
		int d = std::max(depth - 1, 0);
		std::optional<evaluation_t> candidate;
		if (numLegalMovesSearched == 0) {
			candidate = -pvs<isPV>(board, params, d, plies + 1, -beta, -alpha, false,
								   allowNullMove, metrics, state);
		} else {
			int reduction = lmr.getReduction(board, d, numLegalMovesSearched, inCheck, m);
			candidate = -pvs<false>(board, params, d - reduction, plies + 1, -alpha - 1,
									-alpha, false, allowNullMove, metrics, state);

			// if reduced-depth scout search fails high, re-scout at full depth
			if (reduction > 0 && candidate && candidate->score > alpha) {
				candidate = -pvs<false>(board, params, d, plies + 1, -alpha - 1, -alpha, false,
										allowNullMove, metrics, state);
			}

			// if scout search fails high, re-search at full-width
			if (isPV && candidate && candidate->score > alpha) {
				candidate = -pvs<true>(board, params, d, plies + 1, -beta, -alpha, false,
									   allowNullMove, metrics, state);
			}
		}
		if (!candidate) {
			return std::nullopt;
		}
		if (best < *candidate) {
			best = std::move(*candidate);
			best.moves.push_back(m);
		}
		alpha = std::max(alpha, best.score);
		// beta cutoff
		if (alpha >= beta) {
			if (!m.isCapture) {
				state.killerTable.add(m, plies);
				// apply history bonus to the move that caused the beta cutoff
				state.historyTable.update(player, m, depth, true);
				// apply history penalty to the quiet moves that were already searched
				for (size_t j = 0; j < legalMoves.size() - 1; j++) {
					auto quietMove = legalMoves[j];
					if (!quietMove.isCapture) {
						state.historyTable.update(player, quietMove, depth, false);
					}
				}
			}
			break;
		}
	}

	// all terminal conditions other than checkmate and stalemate are handled, check now
	if (legalMoves.empty()) {
		if (!qSearch) {
			if (inCheck) {
				return evaluation_t{MateDistanceToScore(plies), {}};
			} else {
				return evaluation_t{0, {}};
			}
		} else {
			// if we're in qsearch and there are no legal captures, double-check for stalemate
			// note that checkmates are impossible here since we're not in check
			if (board.result() == result_t::draw) {
				return evaluation_t{0, {}};
			}
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
	return evalstate_ptr_t(
		new evalstate_t{transposition_table_t(TTABLE_SIZE),
						killer_table_t(KILLER_TABLE_SIZE),
						history_table_t(MAX_HISTORY_BONUS, HISTORY_DEPTH_FACTOR),
						0ULL,
						clock::now(),
						false,
						{},
						{}});
}

void StopSearch(evalstate_t& state) {
	state.stop.test_and_set();
}

void PonderHit(evalstate_t& state) {
	state.ponderHit.test_and_set();
}

std::pair<evaluation_t, evalmetrics_t>
EvalBoard(const board_t& board, const searchparams_t& params, evalstate_t& state) {
	PHOTON_PROFILE_FUNCTION();
	board_t boardCopy = board;
	evalmetrics_t metrics;
	state.startTime = clock::now();
	state.rootPosHash = board.hash;
	state.killerTable.reset();
	state.historyTable.reset();
	state.stop.clear();
	state.ponderHit.clear();
	state.ponder = params.ponder;

	CHECK_F(!params.maxTime || params.maxTime->first <= params.maxTime->second,
			"Soft time limit must be less than or equal to hard time limit");

	std::optional<evaluation_t> eval;
	int maxDepth = MAX_PLIES;
	if (params.maxDepth.has_value()) {
		if (*params.maxDepth <= MAX_PLIES) {
			maxDepth = *params.maxDepth;
		} else {
			LOG_F(WARNING, "Requested max depth %d is greater than maximum allowable (%d)",
				  *params.maxDepth, MAX_PLIES);
		}
	}

	aspiration_window_t aspiration(ASPIRATION_WINDOW_DELTA, ASPIRATION_MIN_DEPTH, SCORE_INF,
								   MATE_SCORE_BOUND);
	for (int d = 1; d <= maxDepth || (state.ponder && d <= MAX_PLIES); d++) {
		std::optional<evaluation_t> evalOpt;
		bool reSearch = false;
		do {
			auto [alpha, beta] = aspiration.getWindow();
			evalOpt =
				pvs<true>(boardCopy, params, d, 0, alpha, beta, false, true, metrics, state);
			if (evalOpt) {
				reSearch = aspiration.update(d, evalOpt->score);
				if (reSearch) {
					auto newWindow = aspiration.getWindow();
					LOG_F(INFO,
						  "Score %d failed at depth %d, re-searching with window [%d, %d]",
						  evalOpt->score, d, newWindow.first, newWindow.second);
				}
			}
		} while (evalOpt && reSearch);
		if (!evalOpt) {
			// we're terminating early (e.g. time limit) so break out
			break;
		}
		eval = std::move(evalOpt);
		metrics.depth = d;
		metrics.ttableUsage = state.ttable.getUsage();
		LOG_F(INFO, "Searched at depth %d with score %d, nodes=%d", d, eval->score,
			  metrics.nodes);

		if (params.onResult) {
			// reverse PV for passing to callback
			evaluation_t result = *eval;
			std::vector<move_t> moves(result.moves.crbegin(), result.moves.crend());
			result.moves = std::move(moves);
			params.onResult(result, metrics);
		}

		if (!state.ponder && params.maxTime) {
			auto elapsed = clock::now() - state.startTime;
			if (elapsed >= params.maxTime->first) {
				break;
			}
		}
	}
	if (!eval) {
		LOG_F(WARNING,
			  "Search terminated early without a result, returning arbitrary result.");
		auto legalMoves = board.moves();
		CHECK_F(!legalMoves.empty(), "Player to move has no legal moves");
		eval = evaluation_t{0, {legalMoves.front()}};
	}

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
