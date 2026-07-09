#pragma once

#include "photon/core.h"

#include <array>

namespace photon::util {

struct zobrist_data_t {
	std::array<std::array<std::array<uint64_t, 64>, ALL_PIECES.size()>, 2> pieceKeys;
	uint64_t playerKey;
	/** Ordering corresponds to bits in board metadata */
	std::array<uint64_t, 4> castleKeys;
	/** Key for each column where en passant is possible */
	std::array<uint64_t, 8> enPassantKeys;
};

const zobrist_data_t& ZobristData();

} // namespace photon::util
