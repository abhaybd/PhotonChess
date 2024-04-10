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
engine::evalstate_ptr_t eval_state;

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
	// TODO: reset any other board state once added
}

void positionCommand(const uci::arguments_t& args) {
	LOG_SCOPE_F(INFO, "Received command: position");
	board.reset();
	eval_state = engine::CreateEvalState();

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
	// TODO: think for time budget

	CHECK_F(args.find("infinite") == args.end(), "Infinite search not supported");

	auto start = std::chrono::high_resolution_clock::now();
	auto [result, metrics] = engine::EvalBoard(*board, depth, *eval_state);
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = end - start;
	auto elapsedMillis = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
	LOG_F(INFO, "Search took %.3f seconds", elapsed.count());

	std::stringstream ss;
	ss << "info multipv 1";
	ss << " depth " << depth;
	ss << " nodes " << metrics.nodes << " nps "
	   << static_cast<int>(metrics.nodes / elapsed.count());
	ss << " time " << elapsedMillis.count() << " score ";
	if (result.result == util::WinResult(board->playerToMove())) {
		ss << "mate " << result.moves.size();
	} else if (result.result == util::WinResult(util::OtherPlayer(board->playerToMove()))) {
		ss << "mate -" << result.moves.size();
	} else {
		int score = static_cast<int>(result.score * 100.0f);
		ss << "cp " << score;
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
		loguru::add_file("photonlog.txt", loguru::Truncate, loguru::Verbosity_MAX);
	}
	loguru::init(argc, argv);
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
