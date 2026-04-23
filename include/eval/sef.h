#ifndef SCE_EVAL_SEF_H
#define SCE_EVAL_SEF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine.h"

int SCE_Eval_SimplifiedEvaluationFunction(SCE_Context* const ctx);

int SCE_DeltaEval_SimplifiedEvaluationFunction(const SCE_Chessboard* const ptr_board, SCE_EvalState* const ptr_eval_state, const SCE_ChessMove move);

#ifdef __cplusplus
}
#endif
#endif  // SCE_EVAL_SEF_H
