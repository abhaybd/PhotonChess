#include "../src/engine/aspiration.h"

#include <ostream>

// must be before catch2 includes
std::ostream& operator<<(std::ostream& os, std::pair<int16_t, int16_t> const& value) {
	os << "[" << value.first << ", " << value.second << "]";
	return os;
}

#include <catch2/catch_test_macros.hpp>

using namespace photon;
using namespace photon::engine;

namespace {

std::pair<int16_t, int16_t> window(int16_t low, int16_t high) {
	return {low, high};
}

} // namespace

TEST_CASE("Test Aspiration Window", "[engine][aspiration]") {
	int16_t scoreInf = 10000;
	int16_t mateScoreBound = 9000;
	int minDepth = 5;
	int delta = 50;
	aspiration_window_t aspiration(delta, minDepth, scoreInf, mateScoreBound);

	REQUIRE(aspiration.getWindow() == window(-scoreInf, scoreInf));

	for (int d = 1; d < 5; d++) {
		REQUIRE(!aspiration.update(d, 0));
		REQUIRE(aspiration.getWindow() == window(-scoreInf, scoreInf));
	}

	REQUIRE(aspiration.update(5, 51));
	REQUIRE(aspiration.getWindow() == window(-50, 100));

	REQUIRE(!aspiration.update(5, 51));
	REQUIRE(aspiration.getWindow() == window(1, 101));

	// test that window half-width doubles upon re-search
	REQUIRE(aspiration.update(6, 0));
	REQUIRE(aspiration.getWindow() == window(-49, 101));
	REQUIRE(aspiration.update(6, -51));
	REQUIRE(aspiration.getWindow() == window(-149, 101));
	REQUIRE(!aspiration.update(6, 0));
	REQUIRE(aspiration.getWindow() == window(-50, 50));

	// test that windows expands to half-open when mate score bound is hit
	REQUIRE(aspiration.update(7, mateScoreBound + 1));
	REQUIRE(aspiration.getWindow() == window(-50, scoreInf));
	REQUIRE(aspiration.update(7, -scoreInf - 1));
	REQUIRE(aspiration.getWindow() == window(-scoreInf, scoreInf));
	REQUIRE(!aspiration.update(7, 0));
	REQUIRE(aspiration.getWindow() == window(-50, 50));
}
