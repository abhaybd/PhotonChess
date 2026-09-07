#include "photon/engine/photon_uci.h"

#include <chrono>
#include <emscripten.h>
#include <iostream>
#include <loguru.hpp>
#include <memory>
#include <pthread.h>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

using namespace photon;

namespace {

std::unique_ptr<engine::PhotonUCI> g_photon;

std::vector<std::string> splitTokens(std::string_view line) {
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream in{std::string(line)};
	while (in >> token) {
		tokens.push_back(std::move(token));
	}
	return tokens;
}

engine::position_cmd_t parsePosition(const std::vector<std::string>& tokens, size_t i) {
	engine::position_cmd_t cmd;
	if (i < tokens.size() && tokens[i] == "fen") {
		++i;
		if (i + 6 <= tokens.size()) {
			cmd.fen = tokens[i] + " " + tokens[i + 1] + " " + tokens[i + 2] + " " +
					  tokens[i + 3] + " " + tokens[i + 4] + " " + tokens[i + 5];
			i += 6;
		}
	} else if (i < tokens.size() && tokens[i] == "startpos") {
		++i;
	}

	if (i < tokens.size() && tokens[i] == "moves") {
		++i;
		while (i < tokens.size()) {
			cmd.moves.push_back(tokens[i++]);
		}
	}
	return cmd;
}

engine::go_cmd_t parseGo(const std::vector<std::string>& tokens, size_t i) {
	engine::go_cmd_t cmd;
	while (i < tokens.size()) {
		const std::string& t = tokens[i];
		auto takeInt = [&](auto setter) {
			if (i + 1 < tokens.size()) {
				setter(tokens[i + 1]);
				i += 2;
				return true;
			}
			++i;
			return false;
		};

		if (t == "ponder") {
			cmd.ponder = true;
			++i;
		} else if (t == "infinite") {
			cmd.infinite = true;
			++i;
		} else if (t == "depth") {
			takeInt([&](const std::string& v) { cmd.depth = std::stoi(v); });
		} else if (t == "movetime") {
			takeInt([&](const std::string& v) {
				cmd.movetime = std::chrono::milliseconds(std::stoi(v));
			});
		} else if (t == "wtime") {
			takeInt([&](const std::string& v) {
				cmd.wtime = std::chrono::milliseconds(std::stoi(v));
			});
		} else if (t == "btime") {
			takeInt([&](const std::string& v) {
				cmd.btime = std::chrono::milliseconds(std::stoi(v));
			});
		} else if (t == "winc") {
			takeInt([&](const std::string& v) {
				cmd.winc = std::chrono::milliseconds(std::stoi(v));
			});
		} else if (t == "binc") {
			takeInt([&](const std::string& v) {
				cmd.binc = std::chrono::milliseconds(std::stoi(v));
			});
		} else if (t == "nodes") {
			takeInt([&](const std::string& v) { cmd.nodes = std::stoi(v); });
		} else {
			++i;
		}
	}
	return cmd;
}

void handleCommand(std::string_view line) {
	auto tokens = splitTokens(line);
	if (tokens.empty() || !g_photon) {
		return;
	}

	const std::string& cmd = tokens[0];
	if (cmd == "uci") {
		g_photon->uci();
	} else if (cmd == "isready") {
		g_photon->isready();
	} else if (cmd == "ucinewgame") {
		g_photon->ucinewgame();
	} else if (cmd == "position") {
		g_photon->position(parsePosition(tokens, 1));
	} else if (cmd == "go") {
		g_photon->goAsync(parseGo(tokens, 1));
	} else if (cmd == "stop") {
		g_photon->stop();
	} else if (cmd == "ponderhit") {
		g_photon->ponderhit();
	} else if (cmd == "quit") {
		g_photon->stop();
	}
}

} // namespace

extern "C" {

int pthread_getname_np(pthread_t, char* name, size_t len) {
	if (len > 0) {
		name[0] = '\0';
	}
	return 0;
}

int pthread_setname_np(pthread_t, const char*) {
	return 0;
}

EMSCRIPTEN_KEEPALIVE
void photon_init() {
	if (g_photon) {
		return;
	}

	loguru::g_preamble_thread = false;
	loguru::g_preamble_date = false;
	loguru::g_stderr_verbosity = loguru::Verbosity_OFF;
	int argc = 1;
	char arg0[] = "photon";
	char* argv[] = {arg0, nullptr};
	loguru::init(argc, argv);

	g_photon = std::make_unique<engine::PhotonUCI>(std::cout);
	LOG_F(INFO, "Photon WASM started");
}

EMSCRIPTEN_KEEPALIVE
void photon_uci_cmd(const char* line) {
	if (!line) {
		return;
	}
	handleCommand(line);
}

} // extern "C"

int main() {
	photon_init();
	return 0;
}
