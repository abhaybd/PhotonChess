#include "photon/core.h"
#include "photon/engine/eval.h"
#include "photon/profile.h"
#include "photon/util.h"

#include <iostream>
#include <loguru.hpp>
#include <memory>
#include <sstream>

#include <uci/Listener.h>
#include <uci/events.h>

using namespace photon;
using namespace std::chrono_literals;

const std::string VERSION = "0.1.0";
std::unique_ptr<board_t> board;
engine::evalstate_ptr_t eval_state;

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
	std::cout << "uciok" << std::endl;
}

void newGameCommand(const uci::arguments_t&) {
	LOG_F(INFO, "Received command: ucinewgame");
	board.reset();
	eval_state = engine::CreateEvalState();
}

void positionCommand(const uci::arguments_t& args) {
	// TODO: avoid resetting if board is already in the same position, and just apply moves
	LOG_SCOPE_F(INFO, "Received command: position");
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

void goCommand(const uci::arguments_t& args) {
	PHOTON_PROFILE_FUNCTION();

	LOG_SCOPE_F(INFO, "Received command: go");
	LOG_F(INFO, "Args: %s", argsToStr(args).c_str());

	engine::searchparams_t params;
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

			// time management: 5% of remaining time + 50% of increment, min of 50ms
			auto softTime = std::max(baseTime / 20, 50ms);
			auto hardTime = std::max(baseTime / 20 + increment / 2, 50ms);
			params.maxTime = std::make_pair(softTime, hardTime);
		}
	}

	// TODO: add support for infinite search
	CHECK_F(args.find("infinite") == args.end(), "Infinite search not supported");

	auto start = std::chrono::high_resolution_clock::now();
	auto [result, metrics] = engine::EvalBoard(*board, params, *eval_state);
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = end - start;
	auto elapsedMillis = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
	LOG_F(INFO, "Search took %.3f seconds", elapsed.count());

	std::stringstream ss;
	ss << "info multipv 1";
	ss << " depth " << metrics.depth;
	ss << " nodes " << metrics.nodes << " nps "
	   << static_cast<int>(metrics.nodes / elapsed.count());
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
	std::cout << "bestmove " << util::MoveToUCI(result.moves[0]) << std::endl;
}

void isReadyCommand(const uci::arguments_t&) {
	LOG_F(INFO, "Received command: isready");
	std::cout << "readyok" << std::endl;
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
	listener.addListener(uci::event::GO, goCommand);
	listener.addListener(uci::event::QUIT,
						 [&](const uci::arguments_t&) { listener.stopListening(); });

	listener.setupListener();

	return 0;
}
