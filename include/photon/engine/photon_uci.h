#pragma once

#include "photon/core.h"
#include "photon/engine/eval.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <thread>

namespace photon::engine {

struct position_cmd_t {
	std::optional<std::string> fen;
	std::vector<std::string> moves;
};

struct go_cmd_t {
	bool ponder = false;
	std::optional<int> depth;
	std::optional<std::chrono::milliseconds> movetime;
	std::optional<std::chrono::milliseconds> wtime;
	std::optional<std::chrono::milliseconds> btime;
	std::optional<std::chrono::milliseconds> winc;
	std::optional<std::chrono::milliseconds> binc;
	std::optional<int> nodes;
	bool infinite = false;
};

/**
 * @brief Provides a programmatic UCI interface to the Photon engine.
 *
 * The provided ostream will be written to as a standard UCI output stream.
 *
 * @see https://publish.obsidian.md/modern-uci-doc/UCI+Docs/Intro
 */
class PhotonUCI {
public:
	/**
	 * @brief Don't write UCI output to any output stream.
	 */
	PhotonUCI();

	/**
	 * @param out The output stream to write UCI output to.
	 */
	PhotonUCI(std::ostream& out);
	~PhotonUCI();

	void uci();
	void ucinewgame();
	/**
	 * @brief Blocks while IO operations are being performed.
	 */
	void isready();
	void stop();

	void ponderhit();
	void position(const position_cmd_t& cmd);
	/**
	 * @brief Send a go command to the engine.
	 *
	 * @param cmd The go command parameters.
	 * @return std::string The bestmove line emitted by the engine, possibly with a ponder
	 * argument.
	 */
	std::string go(const go_cmd_t& cmd);
	/**
	 * @brief Send a go command without blocking until it completes. The UCI output will be
	 * written to the provided output stream.
	 *
	 * @param cmd The go command parameters.
	 */
	void goAsync(const go_cmd_t& cmd);

private:
	std::mutex outMutex;
	std::ostream& out;

	std::mutex searchMutex;
	board_t board;
	evalstate_ptr_t evalState;

	std::mutex argsMutex;
	std::optional<go_cmd_t> goCmd;
	std::condition_variable goCmdCV;
	bool quitting = false;

	std::atomic_flag stopSearch;

	std::thread goCommandThread;

	/** Same as go() but assumes the searchMutex is held while executing. */
	std::string goLocked(const go_cmd_t& cmd);

	void printPV(std::chrono::steady_clock::time_point start,
				 const engine::evaluation_t& result, const engine::evalmetrics_t& metrics,
				 bool force = false);
	void goCommandLoop() noexcept;
};

} // namespace photon::engine

std::ostream& operator<<(std::ostream& out, const photon::engine::position_cmd_t& cmd);
std::ostream& operator<<(std::ostream& out, const photon::engine::go_cmd_t& cmd);
