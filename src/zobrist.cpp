#include "zobrist.h"

#include "photon/core.h"

#include <array>
#include <charconv>
#include <mutex>
#include <random>

namespace photon::util {
namespace {

zobrist_data_t ZOBRIST_DATA;
std::once_flag ZOBRIST_INIT;

void initZobrist_internal() {
	std::mt19937_64 mt(42L);
	std::uniform_int_distribution<uint64_t> dist;

	for (auto& playerArr : ZOBRIST_DATA.pieceKeys) {
		for (auto& arr : playerArr) {
			for (uint64_t& key : arr) {
				key = dist(mt);
			}
		}
	}
	ZOBRIST_DATA.playerKey = dist(mt);
	for (uint64_t& key : ZOBRIST_DATA.castleKeys) {
		key = dist(mt);
	}
	for (uint64_t& key : ZOBRIST_DATA.enPassantKeys) {
		key = dist(mt);
	}
}

} // namespace

const zobrist_data_t& ZobristData() {
	std::call_once(ZOBRIST_INIT, initZobrist_internal);
	return ZOBRIST_DATA;
}

} // namespace photon::util
