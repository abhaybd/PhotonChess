#include "photon/core.h"
#include "photon/engine/eval.h"
#include "photon/util.h"

#include <iostream>
#include <loguru.hpp>
#include <memory>
#include <sstream>

#include <uci/Listener.h>
#include <uci/events.h>

using namespace photon;

const std::string VERSION = "0.1.0";
std::unique_ptr<board_t> board;

void uciCommand(const uci::arguments_t&) {
	LOG_F(INFO, "Received command: uci");
	std::cout << "id name Photon " << VERSION << std::endl;
	std::cout << "id author Abhay Deshpande" << std::endl;
	std::cout << "uciok" << std::endl;
}

void newUCICommand(const uci::arguments_t&) {
	LOG_F(INFO, "Received command: new uci");
	board.reset();
	// std::cout << "readyok" << std::endl;
}

void positionCommand(const uci::arguments_t& args) {
	LOG_SCOPE_F(INFO, "Received command: position");
	board.reset();

	auto fenIt = args.find("fen");
	if (fenIt != args.end()) {
		LOG_F(INFO, "Initial FEN: %s", fenIt->second.c_str());
		board = std::make_unique<board_t>(util::MakeBoard(fenIt->second));
	} else {
		LOG_F(INFO, "No initial FEN provided, using default board");
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
	LOG_SCOPE_F(INFO, "Received command: go");

	int depth = 4;
	if (args.find("depth") != args.end()) {
		depth = std::stoi(args.at("depth"));
	}

	CHECK_F(args.find("infinite") == args.end(), "Infinite search not supported");

	auto start = std::chrono::high_resolution_clock::now();
	auto result = engine::EvalBoard(*board, depth);
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = end - start;
	LOG_F(INFO, "Search took %.3f seconds", elapsed.count());

	int score = static_cast<int>(result.score * 100);
    std::stringstream ss;
    ss << "info depth " << result.moves.size() << " score cp " << score << " pv ";
    for (size_t i = 0; i < result.moves.size(); i++) {
        ss << util::MoveToUCI(result.moves[i]);
        if (i != result.moves.size() - 1) {
            ss << " ";
        }
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
    loguru::add_file("photonlog.txt", loguru::Truncate, loguru::Verbosity_MAX);
	loguru::init(argc, argv);
	LOG_F(INFO, "Photon started");

	uci::Listener listener;

	listener.addListener(uci::event::UCI, uciCommand);
	listener.addListener(uci::event::POSITION, positionCommand);
	listener.addListener(uci::event::UCINEWGAME, newUCICommand);
	listener.addListener(uci::event::ISREADY, isReadyCommand);
	listener.addListener(uci::event::GO, goCommand);

	listener.setupListener();

	return 0;
}
