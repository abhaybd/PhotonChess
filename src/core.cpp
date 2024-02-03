#include "photon/core.h"

#include "moves.h"
#include "photon/util.h"

#include <assert.h>
#include <charconv>
#include <loguru.hpp>

using namespace photon::util;

namespace photon {

const std::array<bitboard_t, 6>& board_t::getBitboards(player_t player) const {
	switch (player) {
		case player_t::white:
			return white;
		case player_t::black:
			return black;
		default:
			CHECK_F(false);
	}
}

bitboard_t& board_t::getBitboard(player_t player, piece_t piece) {
	int idx = static_cast<int>(piece);
	switch (player) {
		case player_t::white:
			return white[idx];
		case player_t::black:
			return black[idx];
		default:
			CHECK_F(false);
	}
}

bitboard_t board_t::getBitboard(player_t player, piece_t piece) const {
	return getBitboards(player)[static_cast<int>(piece)];
}

std::string board_t::fen() const {
	// TODO: implement
	return "";
}

bool board_t::hasCastlingRights(player_t player, castle_t castle) const {
	int idx = 0;
	if (player == player_t::black) {
		idx += 2;
	}
	if (castle == castle_t::queen) {
		idx += 1;
	}
	return (metadata & (1 << idx)) != 0;
}

player_t board_t::playerToMove() const {
	return (metadata & (1 << 5)) == 0 ? player_t::white : player_t::black;
}

std::optional<int> board_t::availableEnPassant() const {
	if (enPassant >= 0) {
		return enPassant;
	} else {
		return std::nullopt;
	}
}

bitboard_t board_t::occupancyMap() const {
	return occupancyMap(player_t::white) | occupancyMap(player_t::black);
}

bitboard_t board_t::occupancyMap(player_t player) const {
	bitboard_t ret = 0;
	for (bitboard_t board : getBitboards(player)) {
		ret |= board;
	}
	return ret;
}

board_t& board_t::doMove(move_t move) {
	assert(move.getPlayer(*this) == playerToMove());
	player_t player = playerToMove();
	piece_t piece = move.getPiece(*this);
	auto captured = move.getCapturedPiece(*this);
	if (captured) {
		assert(move.isCapture(*this));
		assert(getBitboard(OtherPlayer(player), *captured) & (1ULL << move.to));
		getBitboard(OtherPlayer(player), *captured) &= ~(1ULL << move.to);
	}
	bitboard_t& bb = getBitboard(player, piece);
	bb &= ~(1ULL << move.from);
	bb |= 1ULL << move.to;

	// TODO: handle pawn promotion

	// move rook if castling
	if (move.isCastle(*this, castle_t::king)) {
		bitboard_t& rook = getBitboard(player, piece_t::rook);
		rook &= ~(1ULL << (player == player_t::white ? 7 : 63));
		rook |= 1ULL << (player == player_t::white ? 5 : 61);
	} else if (move.isCastle(*this, castle_t::queen)) {
		bitboard_t& rook = getBitboard(player, piece_t::rook);
		rook &= ~(1ULL << (player == player_t::white ? 0 : 56));
		rook |= 1ULL << (player == player_t::white ? 3 : 59);
	}

	// handle e.p. capture
	if (move.isEnPassant(*this)) {
		bitboard_t& pawn = getBitboard(player, piece_t::pawn);
		bitboard_t mask = 1ULL << move.to;
		if (player == player_t::white) {
			mask >>= 8;
		} else {
			mask <<= 8;
		}
		pawn &= ~mask;
	}

	// update clocks
	if (piece == piece_t::pawn || captured) {
		halfmoveClock = 0;
	} else {
		halfmoveClock++;
	}
	if (player == player_t::black) {
		fullmove++;
	}
	// toggle player to move
	metadata ^= 1 << 5;
	// remove castling rights if king moves
	if (piece == piece_t::king) {
		metadata &= ~(0b11 << (player == player_t::white ? 0 : 2));
	}
	// remove castling rights if rook moves
	if (piece == piece_t::rook) {
		if (player == player_t::white) {
			if (move.from == 0) {
				metadata &= ~0b10;
			} else if (move.from == 7) {
				metadata &= ~0b1;
			}
		} else {
			if (move.from == 56) {
				metadata &= ~0b1000;
			} else if (move.from == 63) {
				metadata &= ~0b0100;
			}
		}
	}

	// handle e.p. rights
	if (piece == piece_t::pawn &&
		std::abs(static_cast<int>(move.from) - static_cast<int>(move.to)) == 16) {
		enPassant = player == player_t::white ? move.from + 8 : move.from - 8;
	} else {
		enPassant = -1;
	}

	return *this;
}

board_t board_t::doMoveCopy(move_t move) const {
	board_t copy = *this;
	copy.doMove(move);
	return copy;
}

std::vector<move_t> board_t::moves(player_t player) const {
	std::vector<move_t> moves = GenerateMoves(*this, player);

	// TODO Filter out moves that leave the king in check

	return moves;
}

bool board_t::isSquareAttacked(player_t player, uint8_t square) const {
	player_t otherPlayer = OtherPlayer(player);
	bitboard_t enemyOccupancy = occupancyMap(OtherPlayer(player));
	bitboard_t playerOccupancy = occupancyMap(player);

	bitboard_t bb = 1ULL << square;

	bitboard_t pawnAttacks = PawnAttackMask(bb, playerOccupancy, otherPlayer);
	if (pawnAttacks & getBitboard(player, piece_t::pawn)) {
		return true;
	}
	bitboard_t knightAttacks = KnightAttackMask(bb, enemyOccupancy);
	if (knightAttacks & getBitboard(player, piece_t::knight)) {
		return true;
	}
	bitboard_t bishopAttacks = BishopAttackMask(bb, enemyOccupancy, playerOccupancy);
	if (bishopAttacks &
		(getBitboard(player, piece_t::bishop) | getBitboard(player, piece_t::queen))) {
		return true;
	}
	bitboard_t rookAttacks = RookAttackMask(bb, enemyOccupancy, playerOccupancy);
	if (rookAttacks &
		(getBitboard(player, piece_t::rook) | getBitboard(player, piece_t::queen))) {
		return true;
	}
	bitboard_t kingAttacks = KingAttackMask(bb, enemyOccupancy);
	if (kingAttacks & getBitboard(player, piece_t::king)) {
		return true;
	}
	return false;
}

bool move_t::isCapture(const board_t& board) const {
	player_t player = getPlayer(board);
	if (CheckOccupancy(board.occupancyMap(player), from) &&
		CheckOccupancy(board.occupancyMap(OtherPlayer(player)), to)) {
		return true;
	}
	return isEnPassant(board);
}

bool move_t::isEnPassant(const board_t& board) const {
	// check that en passant is available and this move goes to that square
	auto ep = board.availableEnPassant();
	if (!ep || ep != to) {
		return false;
	}
	// if this move is a pawn move, it must be en passant
	player_t player = getPlayer(board);
	return CheckOccupancy(board.getBitboard(player, piece_t::pawn), from);
}

player_t move_t::getPlayer(const board_t& board) const {
	player_t player = board.playerToMove();
	assert(CheckOccupancy(board.occupancyMap(player), from));
	return player;
}

piece_t move_t::getPiece(const board_t& board) const {
	player_t player = getPlayer(board);
	for (piece_t p : ALL_PIECES) {
		if (CheckOccupancy(board.getBitboard(player, p), from)) {
			return p;
		}
	}
	CHECK_F(false, "No piece found at square %s", SquareToString(from).c_str());
}

std::optional<piece_t> move_t::getCapturedPiece(const board_t& board) const {
	for (piece_t p : ALL_PIECES) {
		if (CheckOccupancy(board.getBitboard(OtherPlayer(getPlayer(board)), p), to)) {
			return p;
		}
	}
	if (isEnPassant(board)) {
		return piece_t::pawn;
	}
	return std::nullopt;
}

bool move_t::isCastle(const board_t& board) const {
	return isCastle(board, castle_t::king) || isCastle(board, castle_t::queen);
}

bool move_t::isCastle(const board_t& board, castle_t castle) const {
	player_t player = getPlayer(board);
	if (!board.hasCastlingRights(player, castle)) {
		return false;
	}

	if (!CheckOccupancy(board.getBitboard(player, piece_t::king), from)) {
		return false;
	}

	if (from / 8 != to / 8) {
		return false;
	}

	int toCol = to % 8;
	return castle == castle_t::king ? toCol == 6 : toCol == 2;
}

bool move_t::operator==(const move_t& other) const {
	return from == other.from && to == other.to;
}

} // namespace photon
