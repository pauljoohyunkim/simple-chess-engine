#ifndef SCE_FEN_H
#define SCE_FEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "chess.h"

/**
 * @brief Set up the board with FEN string
 * 
 * @param ptr_board Pointer to the SCE_Chessboard struct
 * @param fen FEN string (Only piece placement)
 * @return SCE_Return SCE_SUCCESS for success, other for failure.
 * 
 * Note that history will be empty as it is impossible to determine the moves.
 */
SCE_Return SCE_Chessboard_FEN_setup(SCE_Chessboard* const ptr_board, const char* const fen);

#ifdef __cplusplus
}
#endif

#endif  // SCE_FEN_H
