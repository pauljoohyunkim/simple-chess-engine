#ifndef SCE_EVAL_HCEF_H
#define SCE_EVAL_HCEF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../engine.h"

/**
 * @brief Evaluates the current position using the handcrafted evaluation function
 *
 * @param ctx Pointer to the SCE_Context struct
 * @param ptr_engine Pointer to the SCE_Engine struct
 * @return int Evaluation score of the position
 */
int SCE_Eval_HandcraftedEvaluationFunction(SCE_Context* const ctx, SCE_Engine* const ptr_engine);

/**
 * @brief Updates the evaluation incrementally based on a move made
 *
 * @param ctx Pointer to the SCE_Context struct
 * @param ptr_eval_state Pointer to the SCE_EvalState struct to update
 * @param ptr_engine Pointer to the SCE_Engine struct
 * @param move The move that was made
 * @return int Change in evaluation score due to the move
 */
int SCE_DeltaEval_HandcraftedEvaluationFunction(SCE_Context* const ctx, SCE_EvalState* const ptr_eval_state, SCE_Engine* const ptr_engine, const SCE_ChessMove move);

#ifdef __cplusplus
}
#endif
#endif  // SCE_EVAL_HCEF_H
