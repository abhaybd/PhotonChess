#pragma once

#include <catch2/catch_tostring.hpp>
#include <photon/core.h>
#include <photon/util.h>

namespace Catch {
template <>
struct StringMaker<photon::move_t> {
	static std::string convert(photon::move_t const& move) {
		return photon::util::MoveToUCI(move);
	}
};
} // namespace Catch
