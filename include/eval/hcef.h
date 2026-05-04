#ifndef SCE_EVAL_HCEF_H
#define SCE_EVAL_HCEF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine.h"

int SCE_Eval_HandcraftedEvaluationFunction(SCE_Context* const ctx, SCE_Engine* const ptr_engine);

int SCE_DeltaEval_HandcraftedEvaluationFunction(SCE_Context* const ctx, SCE_EvalState* const ptr_eval_state, SCE_Engine* const ptr_engine, const SCE_ChessMove move);

#ifdef __cplusplus
}
#endif
#endif  // SCE_EVAL_HCEF_H
