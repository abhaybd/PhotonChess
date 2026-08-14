#include "pawn_structure.h"

#include "phase.h"
#include "photon/util.h"

#include <bit>
#include <loguru.hpp>

namespace photon::engine {
namespace {

constexpr bitboard_t FILE_A_MASK = []() constexpr {
	bitboard_t mask = 0;
	for (int i = 0; i < 8; ++i) {
		mask |= 1ULL << (i * 8);
	}
	return mask;
}();

constexpr std::array<bitboard_t, 8> FILE_MASKS = {
	FILE_A_MASK,	  FILE_A_MASK << 1, FILE_A_MASK << 2, FILE_A_MASK << 3,
	FILE_A_MASK << 4, FILE_A_MASK << 5, FILE_A_MASK << 6, FILE_A_MASK << 7,
};

constexpr std::array<std::pair<int16_t, int16_t>, 8> PASSED_FILE_BONUS = {
	{{4, 8}, {2, 6}, {-4, -4}, {-10, -8}, {-10, -8}, {-4, -4}, {2, 6}, {4, 8}}};
constexpr std::array<std::pair<int16_t, int16_t>, 8> PASSED_RANK_BONUS = {
	{{0, 0}, {2, 15}, {6, 20}, {12, 32}, {35, 60}, {75, 105}, {130, 170}, {0, 0}}};
/** Indexed by rank */
constexpr std::array<std::pair<int16_t, int16_t>, 8> SUPPORTED_PASSER_BONUS = {
	{{0, 0}, {4, 6}, {5, 9}, {8, 14}, {13, 20}, {20, 30}, {28, 40}, {0, 0}}};

constexpr std::pair<int16_t, int16_t> DOUBLED_PENALTY = {12, 45};
constexpr std::pair<int16_t, int16_t> WEAK_PENALTY = {8, 20};
// on top of WEAK_PENALTY
constexpr std::pair<int16_t, int16_t> WEAK_UNOPPOSED_PENALTY = {10, 22};

int16_t InterpolateScore(std::pair<int16_t, int16_t> score, int16_t phase) {
	return engine::InterpolateScore(score.first, score.second, phase);
}

/**
 * Check if the file has pawns of a given color starting from a given square and going forwards
 * or backwards (relative to white). Exclude the starting square unless specified.
 */
bool FileHasPawns(const board_t& board, player_t player, uint8_t square, bool forward,
				  bool includeStart = false) {
	bitboard_t pawns = board.getBitboard(player, piece_t::pawn);
	bitboard_t fileMask = FILE_MASKS[square % 8];

	// consists of bits 0..square-1 or square+1..63 if backward or forward
	// uses the fact that subtracting 1 makes all zeros below the one into ones
	bitboard_t rowMask;
	if (forward) {
		if (square < 63) {
			rowMask = ~((1ULL << (square + 1)) - 1);
		} else {
			// 1ULL << (63+1) overflows, so special-case h8
			rowMask = 0;
		}
	} else {
		rowMask = (1ULL << square) - 1;
	}
	if (includeStart) {
		rowMask |= 1ULL << square;
	}

	return (pawns & fileMask & rowMask) != 0;
}

bool IsSupported(const board_t& board, player_t player, uint8_t square) {
	DCHECK_F(square > 7 && square < 56, "Illegal square for pawn: %d", square);

	int stepBack = player == player_t::white ? -8 : 8;
	bitboard_t pawns = board.getBitboard(player, piece_t::pawn);
	if (square % 8 != 0) {
		if (util::CheckOccupancy(pawns, square - 1)) {
			return true;
		}
		if (util::CheckOccupancy(pawns, square - 1 + stepBack)) {
			return true;
		}
	}
	if (square % 8 != 7) {
		if (util::CheckOccupancy(pawns, square + 1)) {
			return true;
		}
		if (util::CheckOccupancy(pawns, square + 1 + stepBack)) {
			return true;
		}
	}
	return false;
}

int16_t PawnScore(const board_t& board, player_t player, uint8_t square, int16_t phase) {
	bool isWhite = player == player_t::white;
	player_t otherPlayer = util::OtherPlayer(player);

	bool isOpposed = FileHasPawns(board, otherPlayer, square, isWhite);
	bool isDoubled = FileHasPawns(board, player, square, isWhite);
	bool isSupported = IsSupported(board, player, square);

	bool isPassed = !isOpposed && !isDoubled;
	if (isPassed) {
		if (square % 8 != 0 && FileHasPawns(board, otherPlayer, square - 1, isWhite)) {
			isPassed = false;
		} else if (square % 8 != 7 && FileHasPawns(board, otherPlayer, square + 1, isWhite)) {
			isPassed = false;
		}
	}

	bool isWeak = true;
	if (square % 8 != 0 && FileHasPawns(board, player, square - 1, !isWhite, true)) {
		isWeak = false;
	} else if (square % 8 != 7 && FileHasPawns(board, player, square + 1, !isWhite, true)) {
		isWeak = false;
	}

	int16_t score = 0;

	if (isPassed) {
		int file = square % 8;
		// xor 56 flips the board if black
		int rank = (isWhite ? square : square ^ 56) / 8;
		score += InterpolateScore(PASSED_RANK_BONUS[rank], phase);
		score += InterpolateScore(PASSED_FILE_BONUS[file], phase);
		if (isSupported) {
			score += InterpolateScore(SUPPORTED_PASSER_BONUS[rank], phase);
		}
	}
	if (isDoubled && !isSupported) {
		score -= InterpolateScore(DOUBLED_PENALTY, phase);
	}
	if (isWeak) {
		score -= InterpolateScore(WEAK_PENALTY, phase);
		if (!isOpposed) {
			score -= InterpolateScore(WEAK_UNOPPOSED_PENALTY, phase);
		}
	}

	return score;
}

int16_t PawnScore(const board_t& board, player_t player, int16_t phase) {
	bitboard_t pawns = board.getBitboard(player, piece_t::pawn);
	int16_t score = 0;
	while (pawns != 0) {
		uint8_t square = std::countr_zero(pawns);
		score += PawnScore(board, player, square, phase);
		pawns &= ~(1ULL << square);
	}
	return score;
}

} // namespace

int16_t PawnStructureScore(const board_t& board, int16_t phase) {
	int16_t score = 0;
	score += PawnScore(board, player_t::white, phase);
	score -= PawnScore(board, player_t::black, phase);
	return score;
}

} // namespace photon::engine
