#include "exchange.h"

#include "../../moves.h"
#include "photon/profile.h"
#include "photon/util.h"

#include <bit>
#include <loguru.hpp>

namespace photon::engine {
namespace {

constexpr std::array<bitboard_t, 2> PROMOTION_MASKS = {0xFFULL << 56, 0xFFULL};

std::array<int16_t, 6> PIECE_VALUES = {100, 300, 300, 500, 900, 10000};

std::optional<move_t> GetCheapestCapture(const board_t& board, uint8_t square) {
	PHOTON_PROFILE_FUNCTION();

	player_t player = board.playerToMove();
	player_t otherPlayer = util::OtherPlayer(player);
	bitboard_t enemyOccupancy = board.occupancyMap(otherPlayer);
	bitboard_t playerOccupancy = board.occupancyMap(player);

	bitboard_t bb = 1ULL << square;

	// en passant can't show up mid-SEE, so no need to handle it
	bitboard_t pawnAttacks = util::PawnAttackMask(bb, playerOccupancy, otherPlayer);
	if (auto mask = pawnAttacks & board.getBitboard(player, piece_t::pawn); mask) {
		int8_t promotion = -1;
		if (bb & PROMOTION_MASKS[static_cast<int>(player)]) {
			promotion = static_cast<int8_t>(piece_t::queen);
		}
		return move_t(std::countr_zero(mask), square, promotion, true);
	}
	bitboard_t knightAttacks = util::KnightAttackMask(bb, enemyOccupancy);
	if (auto mask = knightAttacks & board.getBitboard(player, piece_t::knight); mask) {
		return move_t(std::countr_zero(mask), square, -1, true);
	}
	bitboard_t bishopAttacks = util::BishopAttackMask(bb, enemyOccupancy, playerOccupancy);
	if (auto mask = bishopAttacks & board.getBitboard(player, piece_t::bishop); mask) {
		return move_t(std::countr_zero(mask), square, -1, true);
	}
	bitboard_t rookAttacks = util::RookAttackMask(bb, enemyOccupancy, playerOccupancy);
	if (auto mask = rookAttacks & board.getBitboard(player, piece_t::rook); mask) {
		return move_t(std::countr_zero(mask), square, -1, true);
	}
	if (auto mask = (bishopAttacks | rookAttacks) & board.getBitboard(player, piece_t::queen);
		mask) {
		return move_t(std::countr_zero(mask), square, -1, true);
	}
	bitboard_t kingAttacks = util::KingAttackMask(bb, enemyOccupancy);
	if (auto mask = kingAttacks & board.getBitboard(player, piece_t::king); mask) {
		return move_t(std::countr_zero(mask), square, -1, true);
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
	PHOTON_PROFILE_FUNCTION();

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
