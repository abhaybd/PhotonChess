#pragma once

#include "photon/core.h"

namespace photon::engine {

bool CanNullMove(const board_t& board, const std::vector<move_t>& pseudoLegalMoves);

class temp_nullmove_handle_t;

temp_nullmove_handle_t doNullMoveTemp(board_t& board);

class temp_nullmove_handle_t {
public:
	temp_nullmove_handle_t(board_t* board);
	temp_nullmove_handle_t(temp_nullmove_handle_t&& other) noexcept;
	temp_nullmove_handle_t(const temp_nullmove_handle_t& other) = delete;
	~temp_nullmove_handle_t();

	temp_nullmove_handle_t& operator=(const temp_nullmove_handle_t& other) = delete;
	temp_nullmove_handle_t& operator=(temp_nullmove_handle_t&& other) = delete;

private:
	board_t* board;
	uint64_t hash;
	int fullmove;
	int8_t enPassant;
	int16_t lastIrreversibleMove;
};

} // namespace photon::engine
