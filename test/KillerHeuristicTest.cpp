#include "../src/engine/killer.h"
#include "photon/core.h"
#include "photon/util.h"

#include <catch2/catch_test_macros.hpp>

using namespace photon;
using namespace photon::util;
using namespace photon::engine;

TEST_CASE("Test killer table", "[engine]") {
	killer_table_t killerTable(10);

	move_t move1 = MoveFromUCI("e2e4", false);
	move_t move2 = MoveFromUCI("e2e5", false);
	move_t move3 = MoveFromUCI("e2e6", false);

	killerTable.add(move1, 0);
	killerTable.add(move2, 0);
	REQUIRE(killerTable.isKiller(move1, 0));
	REQUIRE(killerTable.isKiller(move2, 0));
	REQUIRE(!killerTable.isKiller(move3, 0));

	killerTable.add(move3, 0);
	REQUIRE(!killerTable.isKiller(move1, 0));
	REQUIRE(killerTable.isKiller(move2, 0));
	REQUIRE(killerTable.isKiller(move3, 0));

	killerTable.add(move3, 0);
	REQUIRE(!killerTable.isKiller(move1, 0));
	REQUIRE(killerTable.isKiller(move2, 0));
	REQUIRE(killerTable.isKiller(move3, 0));
}
