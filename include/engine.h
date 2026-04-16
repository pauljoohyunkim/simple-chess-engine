#ifndef SCE_ENGINE_H
#define SCE_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "chess.h"

typedef int (*SCE_Eval)(SCE_Chessboard* const);

typedef struct {
    SCE_Eval eval_function;

} SCE_Engine;

/**
 * @brief Evaluate the chessboard from White's perspective
 * 
 * @param ptr_engine Pointer to the SCE_Engine struct.
 * @param ptr_board Pointer to the SCE_Chessboard struct.
 * @return int Centipawn value where positive means advantage for white and negative means advantage for black.
 * 
 * Note that this function does not return success or failure code.
 */
int SCE_Engine_EvaluateBoard(const SCE_Engine* const ptr_engine, const SCE_Chessboard* const ptr_board);

#ifdef __cplusplus
}
#endif
#endif  // SCE_ENGINE_H
