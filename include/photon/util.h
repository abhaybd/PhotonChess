#pragma once

#include "photon/core.h"

#include <istream>
#include <vector>

/**
 * @namespace photon::util
 * @brief Contains utility functions and types for the Photon Chess engine.
 */
namespace photon::util {

/**
 * @brief Returns the opposite of the given player.
 * @param player The current player.
 * @return The opposite player.
 */
player_t OtherPlayer(player_t player);

/**
 * @brief Checks if a specific index is occupied in a bitboard.
 * @param bitboard The bitboard to check.
 * @param idx The index to check.
 * @return True if the index is occupied, false otherwise.
 */
bool CheckOccupancy(bitboard_t bitboard, int idx);

/**
 * @brief Converts a piece to its corresponding character representation.
 * @param piece The piece to convert.
 * @return The character representation of the piece.
 */
char PieceToChar(piece_t piece);

/**
 * @brief Converts a character to its corresponding piece representation.
 * @param c The character to convert.
 * @return The piece representation of the character.
 */
piece_t CharToPiece(char c);

/**
 * @brief Returns the game result associated with a given player winning.
 * @param player The player in question.
 * @return The result if this player wins.
 */
result_t WinResult(player_t player);

/**
 * @brief Parses a square from a string representation.
 * @param s The string representation of the square.
 * @return The parsed square.
 */
uint8_t ParseSquare(std::string_view s);

/**
 * @brief Converts a square to its string representation.
 * @param square The square to convert.
 * @return The string representation of the square.
 */
std::string SquareToString(uint8_t square);

/**
 * @brief Creates a board from a FEN string in a stream.
 * @param stream A stream from which the FEN string will be read.
 * @return The created board.
 */
board_t MakeBoard(std::istream& stream);

/**
 * @brief Creates a board from a FEN string.
 * @param fen The FEN string.
 * @return The created board.
 */
board_t MakeBoard(std::string_view fen);

/**
 * @brief Creates a default board.
 * @return The default board.
 */
board_t DefaultBoard();

/**
 * @brief Creates a move from long algebraic notation.
 * @param player The player making the move.
 * @param longNotation The long algebraic notation of the move.
 * @return The created move.
 */
move_t MoveFromLongNotation(player_t player, std::string_view longNotation);

/**
 * @brief Creates a move from short algebraic notation.
 *
 * @param board The current board. The move must be for the player whose turn it is.
 * @param short_notation The short algebraic notation of the move, with disambiguation if
 * necessary.
 * @return move_t The created move.
 */
move_t MoveFromShortNotation(const board_t& board, std::string_view short_notation);

/**
 * @brief Creates a move from UCI (Universal Chess Interface) notation.
 * @param uci The UCI notation of the move.
 * @param isCapture Whether the move is a capture.
 * @return The created move.
 */
move_t MoveFromUCI(std::string_view uci, bool isCapture = false);

/**
 * @brief Creates a move from UCI (Universal Chess Interface) notation.
 * @param board The current board.
 * @param uci The UCI notation of the move.
 * @return The created move.
 */
move_t MoveFromUCI(const board_t& board, std::string_view uci);

/**
 * @brief Converts a move to its long algebraic notation.
 *
 * Does not notate checks or checkmates.
 *
 * @param board The current board.
 * @param move The move to convert.
 * @return The long algebraic notation of the move.
 */
std::string MoveToLongNotation(board_t board, move_t move);

/**
 * @brief Converts a move to its short algebraic notation.
 *
 * Does not notate checks or checkmates.
 *
 * @param board The current board.
 * @param move The move to convert.
 * @return std::string The short algebraic notation of the move.
 */
std::string MoveToShortNotation(const board_t& board, move_t move);

/**
 * @brief Converts a move to its UCI (Universal Chess Interface) notation.
 * @param move The move to convert.
 * @return The UCI notation of the move.
 */
std::string MoveToUCI(move_t move);

} // namespace photon::util
