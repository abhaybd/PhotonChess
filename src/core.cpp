#include "photon/core.h"

#include "moves.h"
#include "photon/util.h"
#include "zobrist.h"

#include <loguru.hpp>
#include <sstream>
#include <strings.h>

using namespace photon::util;

namespace photon {

board_t::board_t() : metadata(0), enPassant(-1), halfmoveClock(0), fullmove(1) {
	white.fill(0);
	black.fill(0);
	hash = ZobristHash(*this);
}

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
	std::stringstream ss;
	for (int rank = 7; rank >= 0; rank--) {
		int empty = 0;
		for (int file = 0; file < 8; file++) {
			int idx = rank * 8 + file;
			bool found = false;
			for (piece_t piece : ALL_PIECES) {
				bitboard_t whiteBB = getBitboard(player_t::white, piece);
				bitboard_t blackBB = getBitboard(player_t::black, piece);
				if (CheckOccupancy(whiteBB | blackBB, idx)) {
					if (empty > 0) {
						ss << empty;
						empty = 0;
					}
					if (CheckOccupancy(whiteBB, idx)) {
						ss << PieceToChar(piece);
					} else {
						ss << std::tolower(PieceToChar(piece), std::locale());
					}
					found = true;
					break;
				}
			}
			if (!found) {
				empty++;
			}
		}
		if (empty > 0) {
			ss << empty;
		}
		if (rank > 0) {
			ss << '/';
		}
	}
	ss << ' ' << (playerToMove() == player_t::white ? 'w' : 'b') << ' ';
	bool castleRights = false;
	if (hasCastlingRights(player_t::white, castle_t::king)) {
		ss << 'K';
		castleRights = true;
	}
	if (hasCastlingRights(player_t::white, castle_t::queen)) {
		ss << 'Q';
		castleRights = true;
	}
	if (hasCastlingRights(player_t::black, castle_t::king)) {
		ss << 'k';
		castleRights = true;
	}
	if (hasCastlingRights(player_t::black, castle_t::queen)) {
		ss << 'q';
		castleRights = true;
	}
	if (!castleRights) {
		ss << '-';
	}
	ss << ' ';
	if (enPassant >= 0) {
		ss << SquareToString(enPassant);
	} else {
		ss << '-';
	}
	ss << ' ';
	ss << static_cast<int>(halfmoveClock) << " " << fullmove;
	return ss.str();
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
	return (metadata & (1 << 4)) == 0 ? player_t::white : player_t::black;
}

std::optional<int> board_t::availableEnPassant() const {
	if (enPassant >= 0) {
		return enPassant;
	} else {
		return std::nullopt;
	}
}

bool board_t::inCheck(player_t player) const {
	return isSquareAttacked(OtherPlayer(player), getKing(player));
}

result_t board_t::result() const {
	player_t player = playerToMove();
	// 50 move rule
	if (halfmoveClock >= 100) {
		return result_t::draw;
	}
	// threefold repetition
	if (historyHashes.size() >= 8) {
		int count = 0;
		for (int i = historyHashes.size() - 4; i >= 0; i -= 2) {
			if (historyHashes[i] == hash) {
				count++;
				if (count >= 2) {
					return result_t::draw;
				}
			}
		}
	}
	// stalemate or checkmate
	if (moves().empty()) {
		return inCheck(player) ? WinResult(OtherPlayer(player)) : result_t::draw;
	}
	return result_t::none;
}

uint8_t board_t::getKing(player_t player) const {
	return ffsll(getBitboard(player, piece_t::king)) - 1;
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
	DCHECK_F(move.getPlayer(*this) == playerToMove());

	if (move.isReversible(*this)) {
		historyHashes.push_back(hash);
	} else {
		historyHashes.clear();
	}

	player_t player = playerToMove();
	player_t otherPlayer = OtherPlayer(player);

	// get data from move before mutating this
	piece_t piece = move.getPiece(*this);
	auto captured = move.getCapturedPiece(*this);
	bool isKCastle = move.isCastle(*this, castle_t::king);
	bool isQCastle = move.isCastle(*this, castle_t::queen);
	bool isEP = move.isEnPassant(*this);
	auto promotion = move.getPromotion();
	uint8_t oldMetadata = metadata;
	auto& zobrist = ZobristData();

	if (captured && !isEP) {
		DCHECK_F(move.isCapture);
		DCHECK_F((getBitboard(otherPlayer, *captured) & (1ULL << move.to)) != 0);
		getBitboard(otherPlayer, *captured) &= ~(1ULL << move.to);
		hash ^= zobrist.pieceKeys[static_cast<int>(otherPlayer)][static_cast<int>(*captured)]
								 [move.to];

		// remove castling rights if rook is captured
		if (*captured == piece_t::rook) {
			if (move.to == 0) {
				metadata &= ~0b10;
			} else if (move.to == 7) {
				metadata &= ~0b1;
			} else if (move.to == 56) {
				metadata &= ~0b1000;
			} else if (move.to == 63) {
				metadata &= ~0b100;
			}
		}
	}
	bitboard_t& bb = getBitboard(player, piece);
	bb &= ~(1ULL << move.from);
	hash ^= zobrist.pieceKeys[static_cast<int>(player)][static_cast<int>(piece)][move.from];

	if (promotion) {
		getBitboard(player, *promotion) |= 1ULL << move.to;
		hash ^=
			zobrist.pieceKeys[static_cast<int>(player)][static_cast<int>(*promotion)][move.to];
	} else {
		bb |= 1ULL << move.to;
		hash ^= zobrist.pieceKeys[static_cast<int>(player)][static_cast<int>(piece)][move.to];
	}

	// move rook if castling
	if (isKCastle || isQCastle) {
		uint8_t rookFrom, rookTo;
		if (isKCastle) {
			rookFrom = player == player_t::white ? 7 : 63;
			rookTo = player == player_t::white ? 5 : 61;
		} else {
			rookFrom = player == player_t::white ? 0 : 56;
			rookTo = player == player_t::white ? 3 : 59;
		}
		bitboard_t& rook = getBitboard(player, piece_t::rook);
		rook &= ~(1ULL << rookFrom);
		rook |= 1ULL << rookTo;
		hash ^= zobrist.pieceKeys[static_cast<int>(player)][static_cast<int>(piece_t::rook)]
								 [rookFrom];
		hash ^=
			zobrist
				.pieceKeys[static_cast<int>(player)][static_cast<int>(piece_t::rook)][rookTo];
	}

	// handle e.p. capture
	if (isEP) {
		bitboard_t& pawn = getBitboard(otherPlayer, piece_t::pawn);
		uint8_t ep_square = move.to;
		ep_square += player == player_t::white ? -8 : 8;
		bitboard_t mask = 1ULL << ep_square;
		pawn &= ~mask;
		hash ^= zobrist.pieceKeys[static_cast<int>(otherPlayer)]
								 [static_cast<int>(piece_t::pawn)][ep_square];
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
	metadata ^= 1 << 4;
	hash ^= zobrist.playerKey;

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

	// update hash for castling rights changes
	uint8_t castlingRightsDiff = 0b1111 & (oldMetadata ^ metadata);
	for (int i = 0; i < 4; i++) {
		if (castlingRightsDiff & (1 << i)) {
			hash ^= zobrist.castleKeys[i];
		}
	}

	// handle e.p. rights
	if (enPassant >= 0) {
		// remove old e.p.
		hash ^= zobrist.enPassantKeys[enPassant % 8];
	}
	enPassant = -1;
	if (piece == piece_t::pawn &&
		std::abs(static_cast<int>(move.from) - static_cast<int>(move.to)) == 16) {
		bitboard_t row = 0xFFULL << (8 * (move.to / 8));
		bitboard_t mask = 1ULL << move.to;
		mask = ((mask << 1) | (mask >> 1)) & row;
		// only set e.p. (and add to hash) if e.p. can be played
		if (mask & getBitboard(otherPlayer, piece_t::pawn)) {
			enPassant = player == player_t::white ? move.from + 8 : move.from - 8;
			hash ^= zobrist.enPassantKeys[enPassant % 8];
		}
	}

	return *this;
}

board_t board_t::doMoveCopy(move_t move) const {
	board_t copy = *this;
	copy.doMove(move);
	return copy;
}

std::vector<move_t> board_t::moves() const {
	player_t player = playerToMove();
	std::vector<move_t> moves = GenerateMoves(*this, player);

	std::vector<move_t> legalMoves;
	legalMoves.reserve(moves.size());

	for (move_t move : moves) {
		board_t copy = doMoveCopy(move);
		if (!copy.inCheck(player)) {
			legalMoves.push_back(move);
		}
	}

	return legalMoves;
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

move_t::move_t(uint8_t from, uint8_t to, int8_t promotion, bool isCapture)
	: from(from), to(to), promotion(promotion), isCapture(isCapture) {}

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
	DCHECK_F(CheckOccupancy(board.occupancyMap(player), from));
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
	if (isCapture) {
		for (piece_t p : ALL_PIECES) {
			if (CheckOccupancy(board.getBitboard(OtherPlayer(getPlayer(board)), p), to)) {
				return p;
			}
		}
		if (isEnPassant(board)) {
			return piece_t::pawn;
		}
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

bool move_t::isPromotion() const {
	return promotion >= 0;
}

std::optional<piece_t> move_t::getPromotion() const {
	if (promotion >= 0) {
		return static_cast<piece_t>(promotion);
	} else {
		return std::nullopt;
	}
}

bool move_t::isReversible(const board_t& board) const {
	if (isCapture) {
		return false;
	}

	piece_t piece = getPiece(board);
	if (piece == piece_t::pawn) {
		return false;
	}

	if (piece == piece_t::king) {
		player_t player = getPlayer(board);
		bool canCastleK = board.hasCastlingRights(player, castle_t::king);
		bool canCastleQ = board.hasCastlingRights(player, castle_t::queen);
		return !canCastleK && !canCastleQ;
	} else if (piece == piece_t::rook) {
		player_t player = getPlayer(board);
		if (((player == player_t::white && from == 0) ||
			 (player == player_t::black && from == 56)) &&
			board.hasCastlingRights(player, castle_t::queen)) {
			return false;
		}
		if (((player == player_t::white && from == 7) ||
			 (player == player_t::black && from == 63)) &&
			board.hasCastlingRights(player, castle_t::king)) {
			return false;
		}
	}

	return true;
}

bool move_t::operator==(const move_t& other) const {
	return from == other.from && to == other.to;
}

bool move_t::operator!=(const move_t& other) const {
	return !(*this == other);
}

} // namespace photon
