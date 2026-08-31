#include "photon/engine/photon_uci.h"

#include <chrono>
#include <iostream>
#include <loguru.hpp>

#include <uci/Listener.h>
#include <uci/events.h>

using namespace photon;
using namespace std::chrono_literals;

engine::position_cmd_t parsePositionArgs(const uci::arguments_t& args) {
	engine::position_cmd_t cmd;
	if (auto fenIt = args.find("fen"); fenIt != args.end()) {
		cmd.fen = fenIt->second;
	}
	if (auto movesIt = args.find("moves"); movesIt != args.end()) {
		std::string_view movesStr = movesIt->second;
		LOG_F(INFO, "Moves=%s", movesStr.data());

		size_t pos = 0;
		std::string token;
		while ((pos = movesStr.find(" ")) != std::string::npos) {
			token = movesStr.substr(0, pos);
			if (!token.empty()) {
				cmd.moves.push_back(token);
			}
			movesStr = movesStr.substr(pos + 1);
		}
		if (!movesStr.empty()) {
			cmd.moves.push_back(std::string(movesStr));
		}
	}
	return cmd;
}

engine::go_cmd_t parseGoArgs(const uci::arguments_t& args) {
	engine::go_cmd_t cmd;
	cmd.ponder = args.contains("ponder");
	if (auto depthIt = args.find("depth"); depthIt != args.end()) {
		cmd.depth = std::stoi(depthIt->second);
	}
	if (auto movetimeIt = args.find("movetime"); movetimeIt != args.end()) {
		cmd.movetime = std::chrono::milliseconds(std::stoi(movetimeIt->second));
	}
	if (auto wtimeIt = args.find("wtime"); wtimeIt != args.end()) {
		cmd.wtime = std::chrono::milliseconds(std::stoi(wtimeIt->second));
	}
	if (auto btimeIt = args.find("btime"); btimeIt != args.end()) {
		cmd.btime = std::chrono::milliseconds(std::stoi(btimeIt->second));
	}
	if (auto wincIt = args.find("winc"); wincIt != args.end()) {
		cmd.winc = std::chrono::milliseconds(std::stoi(wincIt->second));
	}
	if (auto bincIt = args.find("binc"); bincIt != args.end()) {
		cmd.binc = std::chrono::milliseconds(std::stoi(bincIt->second));
	}
	if (auto nodesIt = args.find("nodes"); nodesIt != args.end()) {
		cmd.nodes = std::stoi(nodesIt->second);
	}
	if (args.contains("infinite")) {
		cmd.infinite = true;
	}
	CHECK_F(args.find("mate") == args.end(), "go mate is not supported");
	return cmd;
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

	engine::PhotonUCI photon(std::cout);
	uci::Listener listener;

	listener.addListener(uci::event::UCI,
						 [&photon](const uci::arguments_t&) { photon.uci(); });
	listener.addListener(uci::event::POSITION, [&photon](const uci::arguments_t& args) {
		photon.position(parsePositionArgs(args));
	});
	listener.addListener(uci::event::UCINEWGAME,
						 [&photon](const uci::arguments_t&) { photon.ucinewgame(); });
	listener.addListener(uci::event::ISREADY,
						 [&photon](const uci::arguments_t&) { photon.isready(); });
	listener.addListener(uci::event::GO, [&photon](const uci::arguments_t& args) {
		photon.goAsync(parseGoArgs(args));
	});
	listener.addListener(uci::event::PONDERHIT,
						 [&photon](const uci::arguments_t&) { photon.ponderhit(); });
	listener.addListener(uci::event::STOP,
						 [&photon](const uci::arguments_t&) { photon.stop(); });
	listener.addListener(uci::event::QUIT,
						 [&](const uci::arguments_t&) { listener.stopListening(); });

	listener.setupListener();

	return 0;
}
