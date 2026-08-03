#include "lmr.h"

namespace photon::engine {

lmr_t::lmr_t(int minDepth, int minIdx, int reduction)
	: minDepth(minDepth), minIdx(minIdx), reduction(reduction) {}

bool lmr_t::shouldReduce(const board_t&, int depth, int numLegalMovesSearched, bool inCheck,
						 move_t) const {
	return depth >= minDepth && numLegalMovesSearched >= minIdx && !inCheck;
}

int lmr_t::getReduction(const board_t& board, int depth, int numLegalMovesSearched,
						bool inCheck, move_t move) const {
	if (shouldReduce(board, depth, numLegalMovesSearched, inCheck, move)) {
		return reduction;
	} else {
		return 0;
	}
}

} // namespace photon::engine
