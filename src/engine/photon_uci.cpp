#include "photon/engine/photon_uci.h"

#include "photon/profile.h"
#include "photon/util.h"

#include <loguru.hpp>
#include <sstream>

using namespace std::chrono_literals;

namespace {
std::ostream& operator<<(std::ostream& out, const std::vector<std::string>& moves) {
	out << "[";
	for (size_t i = 0; i < moves.size(); ++i) {
		out << moves[i];
		if (i < moves.size() - 1) {
			out << ", ";
		}
	}
	out << "]";
	return out;
}
} // namespace

std::ostream& operator<<(std::ostream& out, const photon::engine::position_cmd_t& cmd) {
	out << "position " << (cmd.fen ? *cmd.fen : "startpos") << " " << cmd.moves;
	return out;
}

std::ostream& operator<<(std::ostream& out, const photon::engine::go_cmd_t& cmd) {
	out << "go";
	if (cmd.ponder) {
		out << " ponder";
	}
	if (cmd.depth) {
		out << " depth " << *cmd.depth;
	}
	if (cmd.movetime) {
		out << " movetime " << cmd.movetime->count();
	}
	if (cmd.wtime) {
		out << " wtime " << cmd.wtime->count();
	}
	if (cmd.btime) {
		out << " btime " << cmd.btime->count();
	}
	if (cmd.winc) {
		out << " winc " << cmd.winc->count();
	}
	if (cmd.binc) {
		out << " binc " << cmd.binc->count();
	}
	if (cmd.nodes) {
		out << " nodes " << *cmd.nodes;
	}
	if (cmd.infinite) {
		out << " infinite";
	}
	return out;
}

namespace photon::engine {
namespace {
const std::string VERSION = "0.1.0";
constexpr int PRINT_PV_MIN_DEPTH = 7;

class NullBuffer : public std::streambuf {
public:
	int overflow(int c) override {
		return c;
	}
};

std::ostream& NullStream() {
	static NullBuffer buffer;
	static std::ostream stream(&buffer);
	return stream;
}

template <typename T>
std::string toString(const T& value) {
	std::stringstream ss;
	ss << value;
	return ss.str();
}

} // namespace

PhotonUCI::PhotonUCI() : PhotonUCI(NullStream()) {}

PhotonUCI::PhotonUCI(std::ostream& out)
	: out(out), board(util::DefaultBoard()), evalState(CreateEvalState()),
	  goCommandThread(&PhotonUCI::goCommandLoop, this) {}

PhotonUCI::~PhotonUCI() {
	if (evalState) {
		stopSearch.test_and_set();
		StopSearch(*evalState);
	}

	{
		std::lock_guard lock(argsMutex);
		quitting = true;
		goCmdCV.notify_all();
	}
	// must release argsMutex before joining the thread
	goCommandThread.join();
}

void PhotonUCI::uci() {
	LOG_F(INFO, "Received command: uci");
	std::lock_guard lock(outMutex);
	out << "id name Photon " << VERSION << "\n"
		<< "id author Abhay Deshpande\n"
		<< "option name Ponder type check default true\n"
		<< "uciok" << std::endl;
}

void PhotonUCI::ucinewgame() {
	LOG_F(INFO, "Received command: ucinewgame");
	std::lock_guard searchLock(searchMutex);
	std::lock_guard argsLock(argsMutex);
	board = util::DefaultBoard();
	evalState = CreateEvalState();
}

void PhotonUCI::isready() {
	LOG_F(INFO, "Received command: isready");
	std::lock_guard lock(outMutex);
	out << "readyok" << std::endl;
}

void PhotonUCI::stop() {
	LOG_F(INFO, "Received command: stop");
	if (evalState) {
		stopSearch.test_and_set();
		StopSearch(*evalState);
	}
}

void PhotonUCI::ponderhit() {
	LOG_F(INFO, "Received command: ponderhit");
	if (evalState) {
		stopSearch.test_and_set();
		PonderHit(*evalState);
	}
}

void PhotonUCI::position(const position_cmd_t& cmd) {
	// TODO: avoid resetting if board is already in the same position, and just apply moves
	LOG_SCOPE_F(INFO, "Received command: position");
	std::lock_guard lock(searchMutex);

	// don't reset eval_state if it exists so we can reuse the transposition table if possible
	if (!evalState) {
		evalState = engine::CreateEvalState();
	}

	if (cmd.fen) {
		LOG_F(INFO, "Initial FEN: %s", cmd.fen->c_str());
		board = util::MakeBoard(*cmd.fen);
	} else {
		LOG_F(INFO, "Starting from default position");
		board = util::DefaultBoard();
	}

	if (!cmd.moves.empty()) {
		LOG_F(INFO, "Moves: %s", toString(cmd.moves).c_str());
		for (const auto& move : cmd.moves) {
			board.doMove(util::MoveFromUCI(board, move));
		}
		LOG_F(INFO, "Final FEN: %s", board.fen().c_str());
	}
}

std::string PhotonUCI::go(const go_cmd_t& cmd) {
	std::lock_guard lock(searchMutex);
	return goLocked(cmd);
}

std::string PhotonUCI::goLocked(const go_cmd_t& cmd) {
	PHOTON_PROFILE_FUNCTION();

	LOG_SCOPE_F(INFO, "Received command: go");
	LOG_F(INFO, "Args: %s", toString(cmd).c_str());

	stopSearch.clear();

	engine::searchparams_t params;
	params.ponder = cmd.ponder;
	params.maxDepth = cmd.depth;
	params.maxNodes = cmd.nodes;

	if (cmd.movetime) {
		params.maxTime = std::make_pair(*cmd.movetime, *cmd.movetime);
	} else {
		std::optional<std::chrono::milliseconds> baseTimeOpt, incrementOpt;
		if (board.playerToMove() == player_t::white) {
			baseTimeOpt = cmd.wtime;
			incrementOpt = cmd.winc;
		} else {
			baseTimeOpt = cmd.btime;
			incrementOpt = cmd.binc;
		}
		if (baseTimeOpt) {
			auto baseTime = *baseTimeOpt;
			auto increment = incrementOpt.value_or(0ms);

			// TODO: improve time management to use movestogo
			// time management: 5% of remaining time + 50% of increment
			auto softTime = baseTime > 20ms ? baseTime / 20 : 1ms;
			auto hardTime = std::max(baseTime / 20 + increment / 2, 20ms);
			params.maxTime = std::make_pair(softTime, hardTime);
		}
	}

	CHECK_F(params.ponder || !cmd.infinite ||
				!(params.maxDepth || params.maxTime || params.maxNodes),
			"Infinite search must be specified with no other search limits");
	bool isInfinite =
		cmd.infinite || (!params.maxDepth && !params.maxTime && !params.maxNodes);

	auto start = std::chrono::steady_clock::now();
	params.onResult = [this, start](const engine::evaluation_t& result,
									const engine::evalmetrics_t& metrics) {
		printPV(start, result, metrics);
	};
	auto [result, metrics] = engine::EvalBoard(board, params, *evalState);
	auto end = std::chrono::steady_clock::now();
	std::chrono::duration<double> elapsed = end - start;
	LOG_F(INFO, "Search took %.3f seconds", elapsed.count());

	// print final info (duplication with in-search printing is fine)
	printPV(start, result, metrics, true);

	// if pondering/infinite, we can't emit bestmove until signaled by gui
	if (params.ponder || isInfinite) {
		while (!stopSearch.test()) {
			std::this_thread::sleep_for(1ms);
		}
	}

	CHECK_F(result.moves.size() > 0, "No moves found!");
	std::stringstream ss;
	ss << "bestmove " << util::MoveToUCI(result.moves[0]);
	if (result.moves.size() > 1) {
		ss << " ponder " << util::MoveToUCI(result.moves[1]);
	}
	{
		std::lock_guard lock(outMutex);
		out << ss.str() << std::endl;
	}

	return ss.str();
}

void PhotonUCI::goAsync(const go_cmd_t& cmd) {
	std::lock_guard lock(argsMutex);
	goCmd = cmd;
	goCmdCV.notify_one();
}

void PhotonUCI::printPV(std::chrono::steady_clock::time_point start,
						const engine::evaluation_t& result,
						const engine::evalmetrics_t& metrics, bool force) {
	if (metrics.depth < PRINT_PV_MIN_DEPTH && !force) {
		return;
	}

	std::chrono::duration<double> elapsed = decltype(start)::clock::now() - start;
	auto elapsedMillis = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);

	std::stringstream ss;
	ss << "info";
	ss << " depth " << metrics.depth;
	ss << " nodes " << metrics.nodes;
	ss << " nps " << static_cast<int>(metrics.nodes / elapsed.count());
	ss << " time " << elapsedMillis.count();
	ss << " hashfull " << metrics.ttableUsage;

	ss << " score ";
	// report mate in fullmoves or score
	auto mateDist = engine::ScoreToMateDistance(result.score);
	if (mateDist.has_value()) {
		if (result.score > 0) {
			ss << "mate " << (*mateDist + 1) / 2;
		} else {
			ss << "mate -" << (*mateDist + 1) / 2;
		}
	} else {
		ss << "cp " << result.score;
	}

	ss << " pv";
	for (move_t move : result.moves) {
		ss << " " << util::MoveToUCI(move);
	}
	LOG_F(INFO, "Sending info: %s", ss.str().c_str());
	std::lock_guard lock(outMutex);
	out << ss.str() << std::endl;
}

void PhotonUCI::goCommandLoop() noexcept {
	while (true) {
		go_cmd_t cmd;
		{
			std::unique_lock lock(argsMutex);
			goCmdCV.wait(lock, [&]() { return goCmd.has_value() || quitting; });
			if (quitting) {
				break;
			}
			cmd = *goCmd;
			goCmd.reset();
		}

		{
			std::lock_guard lock(searchMutex);
			goLocked(cmd);
		}
	}
}

} // namespace photon::engine
