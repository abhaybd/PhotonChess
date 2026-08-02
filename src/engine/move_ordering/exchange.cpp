#include "exchange.h"

#include "../../moves.h"

#include <loguru.hpp>

namespace photon::engine {
namespace {

std::array<int16_t, 6> PIECE_VALUES = {100, 300, 300, 500, 900, 10000};

std::optional<move_t> GetCheapestCapture(const board_t& board, uint8_t square) {
	player_t player = board.playerToMove();
	// TODO: could be optimized. construct the capture move while checking if the square is
	// attacked
	if (board.isSquareAttacked(player, square)) {
		for (piece_t piece : ALL_PIECES) {
			std::vector<move_t> pseudoLegalMoves;
			util::PieceMoves(player, piece, board, pseudoLegalMoves);
			for (const move_t& move : pseudoLegalMoves) {
				if (move.to == square) {
					return move;
				}
			}
		}
	}
	return std::nullopt;
}

int16_t EvaluateExchange(board_t& board, uint8_t square) {
	auto cheapestCapture = GetCheapestCapture(board, square);
	if (!cheapestCapture) {
		return 0;
	}
	piece_t piece = cheapestCapture->getCapturedPiece(board).value();
	auto promotion = cheapestCapture->getPromotion();
	board.doMove(*cheapestCapture);
	int16_t value = PIECE_VALUES[static_cast<int>(piece)] - EvaluateExchange(board, square);
	if (promotion) {
		value += PIECE_VALUES[static_cast<int>(promotion.value())] - 100;
	}
	return value > 0 ? value : 0;
}

} // namespace

int16_t EvaluateExchange(const board_t& board, move_t capture) {
	DCHECK_F(capture.isCapture, "Cannot EvaluateExchange a non-capture move");
	piece_t piece = capture.getCapturedPiece(board).value();
	DCHECK_F(piece != piece_t::king, "Cannot capture a king");

	int16_t value = 0;
	if (auto promotion = capture.getPromotion(); promotion) {
		value = PIECE_VALUES[static_cast<int>(promotion.value())] - 100;
	}

	board_t b = board.cheapCopy();
	b.doMove(capture);
	value += PIECE_VALUES[static_cast<int>(piece)] - EvaluateExchange(b, capture.to);
	return value;
}

} // namespace photon::engine
