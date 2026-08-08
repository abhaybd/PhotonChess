#include "pawn_structure.h"

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

constexpr std::array<int16_t, 8> SUPPORTED_PASSER_FILE_BONUS = {
	50, 80, 80, 80, 80, 80, 80, 50,
};
constexpr std::array<int16_t, 8> SUPPORTED_PASSER_RANK_BONUS = {
	0, 10, 10, 20, 50, 90, 120, 0,
};
constexpr std::array<int16_t, 8> PASSED_FILE_BONUS = {
	30, 60, 60, 60, 60, 60, 60, 30,
};
constexpr std::array<int16_t, 8> PASSED_RANK_BONUS = {
	0, 10, 10, 20, 30, 60, 90, 0,
};
constexpr int16_t DOUBLED_PENALTY = 20;
constexpr int16_t WEAK_PENALTY = 10;
// on top of WEAK_PENALTY
constexpr int16_t WEAK_UNOPPOSED_PENALTY = 4;

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

int16_t PawnScore(const board_t& board, player_t player, uint8_t square) {
	bool isWhite = player == player_t::white;
	player_t otherPlayer = util::OtherPlayer(player);

	bool isOpposed = FileHasPawns(board, otherPlayer, square, isWhite);
	bool isDoubled = FileHasPawns(board, player, square, isWhite);

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
		// xor 56 flips the board if black
		int rank = (isWhite ? square : square ^ 56) / 8;
		if (IsSupported(board, player, square)) {
			score += SUPPORTED_PASSER_FILE_BONUS[square % 8];
			score += SUPPORTED_PASSER_RANK_BONUS[rank];
		} else {
			score += PASSED_FILE_BONUS[square % 8];
			score += PASSED_RANK_BONUS[rank];
		}
	}
	if (isDoubled) {
		score -= DOUBLED_PENALTY;
	}
	if (isWeak) {
		score -= WEAK_PENALTY;
		if (!isOpposed) {
			score -= WEAK_UNOPPOSED_PENALTY;
		}
	}

	return score;
}

int16_t PawnScore(const board_t& board, player_t player) {
	bitboard_t pawns = board.getBitboard(player, piece_t::pawn);
	int16_t score = 0;
	while (pawns != 0) {
		uint8_t square = std::countr_zero(pawns);
		score += PawnScore(board, player, square);
		pawns &= ~(1ULL << square);
	}
	return score;
}

} // namespace

int16_t PawnStructureScore(const board_t& board) {
	int16_t score = 0;
	score += PawnScore(board, player_t::white);
	score -= PawnScore(board, player_t::black);
	return score;
}

} // namespace photon::engine
