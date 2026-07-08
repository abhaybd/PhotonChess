#include "photon/engine/eval.h"

#include "move_ordering.h"
#include "photon/core.h"
#include "photon/util.h"

#include <loguru.hpp>

using namespace photon::util;

namespace photon::engine {
namespace {

constexpr float CHECKMATE_SCORE = 10000.0f;

bool operator<(const evaluation_t& a, const evaluation_t& b) {
	if (a.score != b.score) {
		return a.score < b.score;
	}
	// choose shorter variation if winning, if losing choose longer
	if (a.score >= 0) {
		return a.moves.size() > b.moves.size();
	} else {
		return a.moves.size() < b.moves.size();
	}
}

evaluation_t operator-(evaluation_t&& a) {
	evaluation_t ret = std::move(a);
	ret.score = -ret.score;
	return ret;
}

evaluation_t negamax(const board_t& board, int depth, int plies, float alpha, float beta,
					 evalmetrics_t& metrics) {
	// update metrics
	metrics.nodes++;

	// TODO: add transposition table
	// TODO: add quiescence search
	player_t player = board.playerToMove();
	result_t result = board.result();
	if (depth == 0 || result != result_t::none) {
		if (result == result_t::none) {
			float score = PositionHeuristic(board);
			if (player == player_t::black) {
				score = -score;
			}
			return {result, score, {}};
		} else if (result == WinResult(OtherPlayer(player))) {
			return {result, -CHECKMATE_SCORE, {}};
		} else if (result == result_t::draw) {
			return {result, 0.0f, {}};
		} else {
			ABORT_F("Player to move cannot already have checkmate! result == WinResult(player)");
		}
	}

	std::vector<move_t> moves = board.moves();
	std::vector<scoredmove_t> scoredMoves = ScoreMoves(board, moves);

	evaluation_t best = {result, std::numeric_limits<float>::lowest(), {}};
	for (size_t i = 0; i < scoredMoves.size(); i++) {
		move_t m = SelectMove(scoredMoves, i);
		board_t child = board.doMoveCopy(m);
		auto candidate = -negamax(child, depth - 1, plies + 1, -beta, -alpha, metrics);
		if (best < candidate) {
			best = std::move(candidate);
			best.moves.push_back(m);
		}
		alpha = std::max(alpha, best.score);
		if (alpha >= beta) {
			break;
		}
	}
	return best;
}

} // namespace

std::pair<evaluation_t, evalmetrics_t> EvalBoard(const board_t& board, int depth) {
	// TODO: add iterative deepening
	evalmetrics_t metrics;
	float alpha = std::numeric_limits<float>::lowest();
	float beta = std::numeric_limits<float>::max();
	evaluation_t eval = negamax(board, depth, 0, alpha, beta, metrics);
	std::vector<move_t> moves(eval.moves.crbegin(), eval.moves.crend());
	eval.moves = std::move(moves);
	return std::make_pair(eval, metrics);
}

} // namespace photon::engine
