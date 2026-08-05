#ifndef SCE_EVAL_SEF_H
#define SCE_EVAL_SEF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../engine.h"

/**
 * @brief Evaluates the current position using the simplified evaluation function
 *
 * @param ctx Pointer to the SCE_Context struct
 * @param ptr_engine Pointer to the SCE_Engine struct
 * @return int Evaluation score of the position
 */
int SCE_Eval_SimplifiedEvaluationFunction(SCE_Context* ctx, SCE_Engine* ptr_engine);

/**
 * @brief Updates the evaluation incrementally based on a move made (simplified version)
 *
 * @param ctx Pointer to the SCE_Context struct
 * @param ptr_eval_state Pointer to the SCE_EvalState struct to update
 * @param ptr_engine Pointer to the SCE_Engine struct
 * @param move The move that was made
 * @return int Change in evaluation score due to the move
 */
int SCE_DeltaEval_SimplifiedEvaluationFunction(SCE_Context* ctx, SCE_EvalState* ptr_eval_state, SCE_Engine* ptr_engine, SCE_ChessMove move);

#ifdef __cplusplus
}
#endif
#endif  // SCE_EVAL_SEF_H
