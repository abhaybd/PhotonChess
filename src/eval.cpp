#include "photon/engine/eval.h"

#include "photon/core.h"
#include "photon/util.h"

#include <loguru.hpp>

using namespace photon::util;

namespace photon::engine {
namespace {

constexpr float CHECKMATE_SCORE = 10000.0f;
constexpr std::array<float, 6> PIECE_VALUES = {1.0f, 3.0f, 3.0f, 5.0f, 9.0f, 0.0f};

bool operator<(const evaluation_t& a, const evaluation_t& b) {
	return a.score < b.score;
}

evaluation_t operator-(const evaluation_t& a) {
	return {a.result, -a.score, a.moves};
}

void OrderMoves(const board_t&, std::vector<move_t>&) {
	// TODO: implement move ordering
}

evaluation_t negamax(const board_t& board, int depth, int plies, float alpha, float beta) {
	// TODO: add transposition table
	// TODO: add quiescence search
	player_t player = board.playerToMove();
	result_t result = board.result();
	if (depth == 0 || result != result_t::none) {
		if (result == WinResult(player)) {
			return {result, CHECKMATE_SCORE, {}};
		} else if (result == WinResult(OtherPlayer(player))) {
			return {result, -CHECKMATE_SCORE, {}};
		} else if (result == result_t::draw) {
			return {result, 0.0f, {}};
		} else {
			return {result, PositionHeuristic(board), {}};
		}
	}

	std::vector<move_t> moves = board.moves();
	OrderMoves(board, moves);

	evaluation_t best = {result, std::numeric_limits<float>::lowest(), {}};
	for (move_t m : moves) {
		board_t child = board.doMoveCopy(m);
		auto candidate = -negamax(child, depth - 1, plies + 1, -beta, -alpha);
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

float PositionHeuristic(const board_t& board) {
	// TODO: improve heuristic function
	player_t player = board.playerToMove();
	float score = 0.0f;
	for (piece_t p : ALL_PIECES) {
		int wPieces = __builtin_popcountll(board.getBitboard(player, p));
		int bPieces = __builtin_popcountll(board.getBitboard(OtherPlayer(player), p));
		score += PIECE_VALUES[static_cast<int>(p)] * wPieces;
		score -= PIECE_VALUES[static_cast<int>(p)] * bPieces;
	}
	return score;
}

evaluation_t EvalBoard(const board_t& board, int depth) {
	// TODO: add iterative deepening
	float alpha = std::numeric_limits<float>::lowest();
	float beta = std::numeric_limits<float>::max();
	auto eval = negamax(board, depth, 0, alpha, beta);
	std::vector<move_t> moves(eval.moves.crbegin(), eval.moves.crend());
	eval.moves = std::move(moves);
	return eval;
}

} // namespace photon::engine
