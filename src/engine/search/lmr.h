#pragma once

#include "photon/core.h"

namespace photon::engine {

class lmr_t {
public:
	lmr_t(int minDepth, int minIdx, int reduction);
	bool shouldReduce(const board_t& board, int depth, int numLegalMovesSearched, bool inCheck,
					  move_t move) const;
	int getReduction(const board_t& board, int depth, int numLegalMovesSearched, bool inCheck,
					 move_t move) const;

private:
	int minDepth;
	int minIdx;
	int reduction;
};

} // namespace photon::engine
