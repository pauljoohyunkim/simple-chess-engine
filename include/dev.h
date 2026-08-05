#ifndef SCE_DEV_H
#define SCE_DEV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "chess.h"

typedef unsigned int uint;

/**
 * @brief Prints a 64-bit value as a chess board
 *
 * @param val The 64-bit value to print as a board
 */
void print_as_board(uint64_t val);

/**
 * @brief Places a piece on the board at the specified algebraic notation position
 *
 * @param ptr_board Pointer to the chessboard to place the piece on
 * @param an Algebraic notation position (e.g., "e4")
 * @param piece_type Type of piece to place
 * @return SCE_Return SCE_SUCCESS for success, other for failure
 */
SCE_Return place_piece_on_board(SCE_Chessboard* ptr_board, const char * an, PieceType piece_type);

/**
 * @brief Prints a move in algebraic notation
 *
 * @param move The move to print in algebraic notation
 * @return SCE_Return SCE_SUCCESS for success, other for failure
 */
SCE_Return print_move_to_AN(SCE_ChessMove move);

/**
 * @brief Prints the current board state for debugging purposes
 *
 * @param ctx Pointer to the SCE_Context struct
 * @return SCE_Return SCE_SUCCESS for success, other for failure
 */
SCE_Return debug_print_board(const SCE_Context * ctx);

/**
 * @brief Calculates the PERFT count from the current board position
 *
 * @param ctx Pointer to the SCE_Context struct
 * @param depth Depth to search for PERFT
 * @param root Whether this is the root node (affects move printing)
 * @return unsigned long long Number of nodes found, or 0 for failure
 */
unsigned long long perft_count(SCE_Context* ctx, uint depth, bool root);

#ifdef __cplusplus
}
#endif

#endif  // SCE_DEV_H
