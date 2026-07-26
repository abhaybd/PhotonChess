#include "../src/engine/killer.h"
#include "photon/core.h"
#include "photon/util.h"

#include <catch2/catch_test_macros.hpp>

using namespace photon;
using namespace photon::util;
using namespace photon::engine;

namespace {

bool isKiller(const killer_table_t& killerTable, move_t move, uint ply) {
	bool isKiller = killerTable.isKiller(move, ply);
	auto killerMoves = killerTable.getKillerMoves(ply);
	bool isInRange = std::find(killerMoves.begin(), killerMoves.end(), move) != killerMoves.end();
	REQUIRE(isKiller == isInRange);
	return isKiller;
}

}

TEST_CASE("Test killer table", "[engine]") {
	killer_table_t killerTable(10);

	move_t move1 = MoveFromUCI("e2e4", false);
	move_t move2 = MoveFromUCI("e2e5", false);
	move_t move3 = MoveFromUCI("e2e6", false);

	killerTable.add(move1, 0);
	killerTable.add(move2, 0);
	REQUIRE(isKiller(killerTable, move1, 0));
	REQUIRE(isKiller(killerTable, move2, 0));
	REQUIRE(!isKiller(killerTable, move3, 0));

	killerTable.add(move3, 0);
	REQUIRE(!isKiller(killerTable, move1, 0));
	REQUIRE(isKiller(killerTable, move2, 0));
	REQUIRE(isKiller(killerTable, move3, 0));

	killerTable.add(move3, 0);
	REQUIRE(!isKiller(killerTable, move1, 0));
	REQUIRE(isKiller(killerTable, move2, 0));
	REQUIRE(isKiller(killerTable, move3, 0));
}
