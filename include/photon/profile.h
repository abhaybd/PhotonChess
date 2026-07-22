#pragma once

#include <cstdint>
#include <memory>
#include <source_location>
#include <string>

namespace photon::profile {

struct profiler_t;

/** Stable, process-wide identifier for a profiled scope (call site). */
using scope_id_t = uint32_t;

/**
 * @brief Initialize the profiler with the default filename "photon_profile.txt"
 *
 * @return std::shared_ptr<profiler_t> The profiler instance. The profile will be written to
 * the file when the profiler is destroyed.
 */
std::shared_ptr<profiler_t> Init();

/**
 * @brief Initialize the profiler with the given filename
 *
 * @param filename The filename to write the profile to.
 * @return std::shared_ptr<profiler_t> The profiler instance. The profile will be written to
 * the file when the profiler is destroyed.
 */
std::shared_ptr<profiler_t> Init(std::string filename);

/**
 * @brief Register a profiled scope and return its id. This performs the (relatively expensive)
 * name formatting/lookup, and is intended to be called once per call site via a static local
 * (see PHOTON_PROFILE_FUNCTION). The id is stable for the lifetime of the process.
 *
 * @param location The source location of the scope. Usually leave as the default value.
 * @return scope_id_t The id to pass to BeginScope/EndScope.
 */
scope_id_t
RegisterScope(const std::source_location& location = std::source_location::current());

/**
 * @brief Register a profiled scope with an explicit (persistent, unique) name.
 *
 * @param name The name of the scope.
 * @return scope_id_t The id to pass to BeginScope/EndScope.
 */
scope_id_t RegisterScope(std::string name);

/** @brief Begin timing the scope identified by @p id. Prefer the RAII scope_guard_t. */
void BeginScope(scope_id_t id);

/** @brief End timing the scope identified by @p id. Prefer the RAII scope_guard_t. */
void EndScope(scope_id_t id);

/**
 * @brief Stack-allocated RAII guard that times a scope between construction and destruction.
 * Non-copyable and non-movable so that BeginScope/EndScope are always balanced.
 */
struct scope_guard_t {
	explicit scope_guard_t(scope_id_t id) : id_(id) {
		BeginScope(id_);
	}
	~scope_guard_t() {
		EndScope(id_);
	}

	scope_guard_t(const scope_guard_t&) = delete;
	scope_guard_t& operator=(const scope_guard_t&) = delete;
	scope_guard_t(scope_guard_t&&) = delete;
	scope_guard_t& operator=(scope_guard_t&&) = delete;

	scope_id_t id_;
};

} // namespace photon::profile

#ifdef PHOTON_PROFILING_ENABLED

// Profile the enclosing function. The scope id is resolved exactly once per call site via a
// static local, so the hot path only touches an integer id.
#define PHOTON_PROFILE_FUNCTION()                                                             \
	static const ::photon::profile::scope_id_t _photon_scope_id =                             \
		::photon::profile::RegisterScope();                                                   \
	::photon::profile::scope_guard_t _photon_scope_guard(_photon_scope_id)

// Profile a named scope. @p name must be a persistent, unique name.
#define PHOTON_PROFILE_SCOPE(name)                                                            \
	static const ::photon::profile::scope_id_t _photon_scope_id =                             \
		::photon::profile::RegisterScope(name);                                               \
	::photon::profile::scope_guard_t _photon_scope_guard(_photon_scope_id)

#else

// Profiling compiled out: expand to nothing so there is zero runtime cost.
#define PHOTON_PROFILE_FUNCTION() ((void)0)
#define PHOTON_PROFILE_SCOPE(name) ((void)0)

#endif // PHOTON_PROFILING_ENABLED
