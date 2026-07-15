#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace photon {

/**
 * @brief The bitboard type used to represent the state of the chessboard.
 * Each bit corresponds to a square on the chessboard.
 *
 * i-th LSB corresponds to i-th square: a1, b1, ..., g8, h8
 */
using bitboard_t = uint64_t;

/**
 * @brief Enum representing the two players in a chess game: white and black.
 */
enum class player_t {
	white,
	black
};

/**
 * @brief Enum representing the possible results of a chess game.
 */
enum result_t {
	white_wins,
	black_wins,
	draw,
	none
};

/**
 * @brief Enum representing the two castling options: king-side and queen-side.
 */
enum class castle_t {
	king,
	queen
};

/**
 * @brief Enum representing the different types of chess pieces.
 */
enum class piece_t {
	pawn = 0,
	knight = 1,
	bishop = 2,
	rook = 3,
	queen = 4,
	king = 5
};

/**
 * @brief Array containing all the possible chess pieces.
 */
constexpr std::array<piece_t, 6> ALL_PIECES = {piece_t::pawn, piece_t::knight, piece_t::bishop,
											   piece_t::rook, piece_t::queen,  piece_t::king};

struct move_t;

/**
 * Struct representing the chessboard state.
 */
struct board_t {
	/** Bitboards for white pieces, indexed by piece_t */
	std::array<bitboard_t, ALL_PIECES.size()> white;
	/** Bitboards for black pieces, indexed by piece_t */
	std::array<bitboard_t, ALL_PIECES.size()> black;
	/**
	 * @brief Castling rights and current move.
	 *
	 * 4 LSB are castling rights: [W kingside, W queenside, B kingside, B queenside]
	 * 5th bit is current player to move, 0 => W, 1 => B
	 */
	uint8_t metadata;
	/** En passant square, -1 is not available */
	int8_t enPassant;
	/** Halfmove clock */
	uint8_t halfmoveClock;
	/** Fullmove number */
	int fullmove;
	/** Board hash */
	uint64_t hash;

	/**
	 * @brief Construct a new empty board.
	 * This is in an invalid state, and should be initialized before use.
	 */
	board_t();

	/**
	 * @brief Gets the bitboards for a specific player.
	 * @param player The player
	 * @return The bitboards for the player
	 */
	const std::array<bitboard_t, 6>& getBitboards(player_t player) const;

	/**
	 * @brief Gets the bitboard for a specific player and piece.
	 * @param player The player
	 * @param piece The piece
	 * @return The bitboard for the player and piece
	 */
	bitboard_t& getBitboard(player_t player, piece_t piece);

	/**
	 * @brief Gets the bitboard for a specific player and piece.
	 * @param player The player
	 * @param piece The piece
	 * @return The bitboard for the player and piece
	 */
	bitboard_t getBitboard(player_t player, piece_t piece) const;

	/**
	 * @brief Checks if a player has castling rights for a specific castle.
	 * @param player The player
	 * @param castle The castle
	 * @return True if the player has castling rights for the castle, false otherwise
	 */
	bool hasCastlingRights(player_t player, castle_t castle) const;

	/**
	 * @brief Gets the player who is currently to move.
	 * @return The player who is currently to move
	 */
	player_t playerToMove() const;

	/**
	 * @brief Gets the available en passant square.
	 * @return The available en passant square, or std::nullopt if not available
	 */
	std::optional<int> availableEnPassant() const;

	/**
	 * @brief Checks if a player is in check.
	 * @param player The player
	 * @return True if the player is in check, false otherwise
	 */
	bool inCheck(player_t player) const;

	/**
	 * @brief Gets the result of the game.
	 * @return The result of the game
	 */
	result_t result() const;

	/**
	 * @brief Gets the king square for a specific player.
	 * @param player The player
	 * @return The king square for the player
	 */
	uint8_t getKing(player_t player) const;

	/**
	 * @brief Gets the occupancy map of the chessboard.
	 * @return The occupancy map of the chessboard
	 */
	bitboard_t occupancyMap() const;

	/**
	 * @brief Gets the occupancy map of a specific player.
	 * @param player The player
	 * @return The occupancy map of the player
	 */
	bitboard_t occupancyMap(player_t player) const;

	/**
	 * @brief Performs a move on the chessboard state.
	 * @param move The move to perform
	 * @return The updated chessboard state after the move
	 */
	board_t& doMove(move_t move);

	/**
	 * @brief Performs a move on a copy of the chessboard state.
	 * @param move The move to perform
	 * @return The updated chessboard state after the move
	 */
	board_t doMoveCopy(move_t move) const;

	/**
	 * @brief Gets the FEN representation of the chessboard state.
	 * @return The FEN representation of the chessboard state
	 */
	std::string fen() const;

	/**
	 * @brief Gets a vector of all legal moves from the current chessboard state.
	 * @return A vector of all legal moves
	 */
	std::vector<move_t> moves() const;

	/**
	 * @brief Checks if a square is attacked by a player.
	 * @param player The player
	 * @param square The square to check
	 * @return True if the square is attacked by the player, false otherwise
	 */
	bool isSquareAttacked(player_t player, uint8_t square) const;
};

/**
 * Struct representing a chess move.
 */
struct move_t {
	/** Starting square of the move */
	uint8_t from;
	/** Destination square of the move */
	uint8_t to;
	/** Piece type to promote to (if any), -1 if not a promotion */
	int8_t promotion;
	/** If the move is a capture */
	bool isCapture;

	/**
	 * @brief Constructor for a chess move.
	 * @param from Starting square of the move
	 * @param to Destination square of the move
	 * @param promotion Piece type to promote to (if any), -1 if not a promotion
	 * @param isCapture If the move is a capture
	 */
	move_t(uint8_t from, uint8_t to, int8_t promotion = -1, bool isCapture = false);

	/**
	 * @brief Checks if the move is an en passant capture.
	 * @param board The chessboard state
	 * @return True if the move is an en passant capture, false otherwise
	 */
	bool isEnPassant(const board_t& board) const;

	/**
	 * @brief Gets the player who made the move.
	 * @param board The chessboard state
	 * @return The player who made the move
	 */
	player_t getPlayer(const board_t& board) const;

	/**
	 * @brief Gets the piece that is moved.
	 * @param board The chessboard state
	 * @return The piece that is moved
	 */
	piece_t getPiece(const board_t& board) const;

	/**
	 * @brief Gets the captured piece (if any) in the move.
	 * @param board The chessboard state
	 * @return The captured piece, or std::nullopt if no piece is captured
	 */
	std::optional<piece_t> getCapturedPiece(const board_t& board) const;

	/**
	 * @brief Checks if the move is a castle move.
	 * @param board The chessboard state
	 * @return True if the move is a castle move, false otherwise
	 */
	bool isCastle(const board_t& board) const;

	/**
	 * @brief Checks if the move is a specific castle move.
	 * @param board The chessboard state
	 * @param castle The type of castle move to check
	 * @return True if the move is the specified castle move, false otherwise
	 */
	bool isCastle(const board_t& board, castle_t castle) const;

	/**
	 * @brief Checks if the move is a promotion move.
	 * @return True if the move is a promotion move, false otherwise
	 */
	bool isPromotion() const;

	/**
	 * @brief Gets the promotion piece (if any) in the move.
	 * @return The promotion piece, or std::nullopt if no promotion is made
	 */
	std::optional<piece_t> getPromotion() const;

	/**
	 * @brief Checks if the move is reversible.
	 *
	 * A move is irreversible if it is a capture, a pawn move, or a move which loses castling
	 * rights.
	 *
	 * @param board The chessboard state
	 * @return True if the move is reversible, false otherwise
	 */
	bool isReversible(const board_t& board) const;

	bool operator==(const move_t& other) const;
	bool operator!=(const move_t& other) const;
};

} // namespace photon
