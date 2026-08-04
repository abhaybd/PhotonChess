#include "photon/core.h"
#include "photon/engine/eval.h"
#include "photon/profile.h"
#include "photon/util.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <loguru.hpp>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>

#include <uci/Listener.h>
#include <uci/events.h>

using namespace photon;
using namespace std::chrono_literals;

constexpr int PRINT_PV_MIN_DEPTH = 4;

const std::string VERSION = "0.1.0";
std::unique_ptr<board_t> board;
engine::evalstate_ptr_t eval_state;
/** Protects board and eval_state. */
std::mutex searchMutex;

/** Protects goArgs, quitting, and goArgsCV. */
std::mutex argsMutex;
std::optional<uci::arguments_t> goArgs;
std::condition_variable goArgsCV;
bool quitting = false;

std::atomic_flag stopPondering;

std::string argsToStr(const uci::arguments_t& args) {
	std::stringstream ss;
	ss << "{";
	for (const auto& entry : args) {
		ss << "\"" << entry.first << "\": \"" << entry.second << "\", ";
	}
	auto s = ss.str();
	return s.substr(0, s.size() - 2) + "}";
}

void uciCommand(const uci::arguments_t&) {
	LOG_F(INFO, "Received command: uci");
	std::cout << "id name Photon " << VERSION << std::endl;
	std::cout << "id author Abhay Deshpande" << std::endl;
	std::cout << "option name Ponder type check default true" << std::endl;
	std::cout << "uciok" << std::endl;
}

void newGameCommand(const uci::arguments_t&) {
	LOG_F(INFO, "Received command: ucinewgame");
	std::lock_guard searchLock(searchMutex);
	std::lock_guard argsLock(argsMutex);
	board.reset();
	eval_state = engine::CreateEvalState();
}

void positionCommand(const uci::arguments_t& args) {
	// TODO: avoid resetting if board is already in the same position, and just apply moves
	LOG_SCOPE_F(INFO, "Received command: position");
	std::lock_guard lock(searchMutex);

	board.reset();
	// don't reset eval_state if it exists so we can reuse the transposition table if possible
	if (!eval_state) {
		eval_state = engine::CreateEvalState();
	}

	if (auto fenIt = args.find("fen"); fenIt != args.end()) {
		LOG_F(INFO, "Initial FEN: %s", fenIt->second.c_str());
		board = std::make_unique<board_t>(util::MakeBoard(fenIt->second));
	} else if (args.find("startpos") != args.end()) {
		LOG_F(INFO, "Starting from default position");
		board = std::make_unique<board_t>(util::DefaultBoard());
	}

	auto movesIt = args.find("moves");
	if (movesIt != args.end()) {
		std::string_view moves = movesIt->second;
		LOG_F(INFO, "Moves=%s", moves.data());

		size_t pos = 0;
		std::string token;
		while ((pos = moves.find(" ")) != std::string::npos) {
			token = moves.substr(0, pos);
			if (!token.empty()) {
				board->doMove(util::MoveFromUCI(*board, token));
			}
			moves = moves.substr(pos + 1);
		}
		board->doMove(util::MoveFromUCI(*board, moves));

		LOG_F(INFO, "Final FEN: %s", board->fen().c_str());
	}
}

void printPV(std::chrono::steady_clock::time_point start, const engine::evaluation_t& result,
			 const engine::evalmetrics_t& metrics) {
	if (metrics.depth < PRINT_PV_MIN_DEPTH) {
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
	std::cout << ss.str() << std::endl;
}

void goCommand(const uci::arguments_t& args) {
	PHOTON_PROFILE_FUNCTION();

	LOG_SCOPE_F(INFO, "Received command: go");
	LOG_F(INFO, "Args: %s", argsToStr(args).c_str());

	stopPondering.clear();

	engine::searchparams_t params;
	params.ponder = args.contains("ponder");
	if (args.find("depth") != args.end()) {
		params.maxDepth = std::stoi(args.at("depth"));
	}
	if (args.find("movetime") != args.end()) {
		std::chrono::milliseconds moveTime(std::stoi(args.at("movetime")));
		params.maxTime = std::make_pair(moveTime, moveTime);
	} else {
		std::string baseTimeKey, incrementKey;
		if (board->playerToMove() == player_t::white) {
			baseTimeKey = "wtime";
			incrementKey = "winc";
		} else {
			baseTimeKey = "btime";
			incrementKey = "binc";
		}
		if (args.find(baseTimeKey) != args.end()) {
			std::chrono::milliseconds baseTime(std::stoi(args.at(baseTimeKey)));
			std::chrono::milliseconds increment(0);
			if (args.find(incrementKey) != args.end()) {
				increment = std::chrono::milliseconds(std::stoi(args.at(incrementKey)));
			}

			// time management: 5% of remaining time + 50% of increment
			auto softTime = baseTime > 20ms ? baseTime / 20 : 1ms;
			auto hardTime = std::max(baseTime / 20 + increment / 2, 20ms);
			params.maxTime = std::make_pair(softTime, hardTime);
		}
	}
	if (args.find("nodes") != args.end()) {
		params.maxNodes = std::stoi(args.at("nodes"));
	}
	bool isInfinite = args.contains("infinite");
	CHECK_F(params.ponder ||
				(isInfinite != (params.maxDepth || params.maxTime || params.maxNodes)),
			"Infinite search must be specified with no other search limits");

	CHECK_F(args.find("mate") == args.end(), "Mate search not supported");

	auto start = std::chrono::steady_clock::now();
	params.onResult = [start](const engine::evaluation_t& result,
							  const engine::evalmetrics_t& metrics) {
		printPV(start, result, metrics);
	};
	auto [result, metrics] = engine::EvalBoard(*board, params, *eval_state);
	auto end = std::chrono::steady_clock::now();
	std::chrono::duration<double> elapsed = end - start;
	LOG_F(INFO, "Search took %.3f seconds", elapsed.count());

	// if pondering, we can't emit bestmove until we get ponderhit or stop
	if (params.ponder) {
		while (!stopPondering.test()) {
			std::this_thread::sleep_for(1ms);
		}
	}

	CHECK_F(result.moves.size() > 0, "No moves found!");
	std::cout << "bestmove " << util::MoveToUCI(result.moves[0]);
	if (result.moves.size() > 1) {
		std::cout << " ponder " << util::MoveToUCI(result.moves[1]);
	}
	std::cout << std::endl;
}

void goCommandAsync(const uci::arguments_t& args) {
	std::lock_guard lock(argsMutex);
	goArgs = args;
	goArgsCV.notify_one();
}

void goCommandLoop() {
	while (true) {
		uci::arguments_t args;
		{
			std::unique_lock lock(argsMutex);
			goArgsCV.wait(lock, [&]() { return goArgs.has_value() || quitting; });
			if (quitting) {
				break;
			}
			args = *goArgs;
			goArgs.reset();
		}

		{
			std::lock_guard lock(searchMutex);
			goCommand(args);
		}
	}
}

void isReadyCommand(const uci::arguments_t&) {
	LOG_F(INFO, "Received command: isready");
	std::cout << "readyok" << std::endl;
}

void ponderHitCommand(const uci::arguments_t&) {
	LOG_F(INFO, "Received command: ponderhit");
	if (eval_state) {
		stopPondering.test_and_set();
		engine::PonderHit(*eval_state);
	}
}

void stopCommand(const uci::arguments_t&) {
	LOG_F(INFO, "Received command: stop");
	if (eval_state) {
		stopPondering.test_and_set();
		engine::StopSearch(*eval_state);
	}
}

void quit() {
	if (eval_state) {
		stopPondering.test_and_set();
		engine::StopSearch(*eval_state);
	}

	std::lock_guard lock(argsMutex);
	quitting = true;
	goArgsCV.notify_all();
}

int main(int argc, char** argv) {
	loguru::g_preamble_thread = false;
	loguru::g_preamble_date = false;
	loguru::g_stderr_verbosity = loguru::Verbosity_OFF;
	if (std::getenv("PHOTON_DISABLE_LOGGING") == nullptr) {
		const char* logFile = "photonlog.txt";
		if (std::getenv("PHOTON_LOG_FILE") != nullptr) {
			logFile = std::getenv("PHOTON_LOG_FILE");
		}
		loguru::add_file(logFile, loguru::Truncate, loguru::Verbosity_MAX);
	}
	loguru::init(argc, argv);

#ifdef PHOTON_PROFILING_ENABLED
	// init profiling if enabled in this build
	std::shared_ptr<profile::profiler_t> prof;
	if (auto profileFile = std::getenv("PHOTON_PROFILE_FILE"); profileFile != nullptr) {
		prof = profile::Init(profileFile);
	} else {
		prof = profile::Init();
	}
#endif

	LOG_F(INFO, "Photon started");

	uci::Listener listener;

	listener.addListener(uci::event::UCI, uciCommand);
	listener.addListener(uci::event::POSITION, positionCommand);
	listener.addListener(uci::event::UCINEWGAME, newGameCommand);
	listener.addListener(uci::event::ISREADY, isReadyCommand);
	listener.addListener(uci::event::GO, goCommandAsync);
	listener.addListener(uci::event::PONDERHIT, ponderHitCommand);
	listener.addListener(uci::event::STOP, stopCommand);
	listener.addListener(uci::event::QUIT, [&](const uci::arguments_t&) {
		LOG_F(INFO, "Received command: quit");
		listener.stopListening();
		quit();
	});

	std::thread goCommandThread(goCommandLoop);
	listener.setupListener();
	goCommandThread.join();

	return 0;
}
