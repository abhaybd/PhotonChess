#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <loguru.hpp>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <photon/profile.h>

using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;
using namespace std::chrono_literals;

namespace photon::profile {
namespace {

using duration_t = std::chrono::nanoseconds;

struct profile_data_t {
	/** Self time: time spent in scope, excluding time in called scopes */
	duration_t tottime = 0ns;
	/** Cumulative time: wall time this scope is on the stack (outermost frames only) */
	duration_t cumtime = 0ns;
	/** Total number of invocations (including recursive re-entries) */
	unsigned int count = 0;
	/** Number of frames for this scope currently on the stack (recursion depth) */
	unsigned int active = 0;
};

/** A single live frame on the profiling call stack. */
struct stack_frame_t {
	scope_id_t id;
	high_resolution_clock::time_point start;
	/** Time spent in this frame's direct children, used to derive self time. */
	duration_t childtime = 0ns;
};

struct sort_profile_by_cumtime {
	bool operator()(const std::pair<std::string, profile_data_t>& a,
					const std::pair<std::string, profile_data_t>& b) const {
		return a.second.cumtime < b.second.cumtime;
	}
};

std::ostream& print_width(std::ostream& file, int width, const std::string& value) {
	for (int i = static_cast<int>(value.size()); i < width; i++) {
		file << " ";
	}
	file << value;
	return file;
}

std::string format_fixed(double value, int precision) {
	std::ostringstream ss;
	ss << std::fixed << std::setprecision(precision) << value;
	return ss.str();
}

// Process-wide scope registry. Ids are stable for the lifetime of the process and are
// independent of any individual profiler instance, so they can be safely cached in the
// static locals emitted by PHOTON_PROFILE_FUNCTION.
std::vector<std::string>& scopeNames() {
	static std::vector<std::string> names;
	return names;
}

std::unordered_map<std::string, scope_id_t>& scopeIds() {
	static std::unordered_map<std::string, scope_id_t> ids;
	return ids;
}

// Non-owning pointer to the active profiler. The owning shared_ptr is held by the caller of
// Init(); this raw pointer lets the hot path avoid an atomic weak_ptr::lock() per scope. This
// is safe because all profiled scopes are nested inside the profiler's lifetime.
profiler_t* g_profiler = nullptr;
} // namespace

struct profiler_t {
	profiler_t(std::string filename)
		: filename(filename), startTime(high_resolution_clock::now()) {}
	~profiler_t() {
		std::vector<std::pair<std::string, profile_data_t>> sortedScopeData;
		for (scope_id_t id = 0; id < scopeData.size(); id++) {
			if (scopeData[id].count == 0) {
				continue;
			}
			sortedScopeData.emplace_back(scopeNames()[id], scopeData[id]);
		}
		std::sort(sortedScopeData.begin(), sortedScopeData.end(), sort_profile_by_cumtime());
		std::reverse(sortedScopeData.begin(), sortedScopeData.end());

		auto totalElapsed =
			duration_cast<milliseconds>(high_resolution_clock::now() - startTime);
		std::ofstream file(filename);
		file << "Total elapsed: " << totalElapsed.count() << "ms\n";
		file << "Ordered by: cumulative time\n\n";
		file << "  ncalls\ttottime(ms)\tpercall(us)\tcumtime(ms)\tpercall(us)\tname\n";
		for (const auto& [name, data] : sortedScopeData) {
			double totMs = data.tottime.count() / 1e6;
			double cumMs = data.cumtime.count() / 1e6;
			double totPerCallUs = data.count ? (data.tottime.count() / 1e3) / data.count : 0.0;
			double cumPerCallUs = data.count ? (data.cumtime.count() / 1e3) / data.count : 0.0;
			print_width(file, 8, std::to_string(data.count)) << "\t";
			print_width(file, 11, format_fixed(totMs, 3)) << "\t";
			print_width(file, 11, format_fixed(totPerCallUs, 3)) << "\t";
			print_width(file, 11, format_fixed(cumMs, 3)) << "\t";
			print_width(file, 11, format_fixed(cumPerCallUs, 3)) << "\t";
			file << name << "\n";
		}
		file.close();

		// cleanup
		g_profiler = nullptr;
	}

	const std::string filename;
	high_resolution_clock::time_point startTime;
	std::vector<profile_data_t> scopeData;
	std::vector<stack_frame_t> scopeStack;
};

std::shared_ptr<profiler_t> Init() {
	return Init("photon_profile.txt");
}

std::shared_ptr<profiler_t> Init(std::string filename) {
	CHECK_F(g_profiler == nullptr, "Profiler already initialized with filename %s",
			g_profiler->filename.c_str());
	auto prof = std::make_shared<profiler_t>(filename);
	g_profiler = prof.get();
	return prof;
}

scope_id_t RegisterScope(std::string name) {
	auto& ids = scopeIds();
	if (auto it = ids.find(name); it != ids.end()) {
		return it->second;
	}
	auto id = static_cast<scope_id_t>(scopeNames().size());
	scopeNames().push_back(name);
	ids.emplace(std::move(name), id);
	return id;
}

scope_id_t RegisterScope(const std::source_location& location) {
	std::stringstream ss;
	ss << location.file_name() << ":" << location.line() << ":" << location.function_name();
	return RegisterScope(ss.str());
}

void BeginScope(scope_id_t id) {
	profiler_t* prof = g_profiler;
	if (!prof) {
		return;
	}
	if (id >= prof->scopeData.size()) {
		prof->scopeData.resize(id + 1);
	}
	auto& data = prof->scopeData[id];
	data.count++;
	data.active++;
	prof->scopeStack.push_back({id, high_resolution_clock::now(), 0ns});
}

void EndScope(scope_id_t id) {
	profiler_t* prof = g_profiler;
	if (!prof) {
		return;
	}
	auto now = high_resolution_clock::now();

	CHECK_F(!prof->scopeStack.empty() && prof->scopeStack.back().id == id,
			"Scope stack mismatch");
	stack_frame_t frame = prof->scopeStack.back();
	prof->scopeStack.pop_back();

	auto elapsed = duration_cast<duration_t>(now - frame.start);
	// self time = wall time of this frame minus time spent in its direct children
	auto self = elapsed - frame.childtime;

	auto& data = prof->scopeData[id];
	data.tottime += self;
	data.active--;
	// only the outermost frame of a (possibly recursive) scope contributes to cumulative
	// time, so nested re-entries aren't double counted
	if (data.active == 0) {
		data.cumtime += elapsed;
	}

	// attribute this frame's full elapsed time to its parent as child time
	if (!prof->scopeStack.empty()) {
		prof->scopeStack.back().childtime += elapsed;
	}
}
} // namespace photon::profile
