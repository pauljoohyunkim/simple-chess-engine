#include <assert.h>
#include "eval/sef.h"
#include "eval/hcef.h"

int SCE_Eval_HandcraftedEvaluationFunction(SCE_Context* const ctx, SCE_Engine* const ptr_engine) {
    assert(ctx != NULL);

    int score = SCE_Eval_SimplifiedEvaluationFunction(ctx, ptr_engine);

    return score;
}
int SCE_DeltaEval_HandcraftedEvaluationFunction(SCE_Chessboard* const ptr_board, SCE_EvalState* const ptr_eval_state, SCE_Engine* const ptr_engine, const SCE_ChessMove move) {
    assert(ptr_board != NULL);
    assert(ptr_eval_state != NULL);

    int delta_score = SCE_DeltaEval_SimplifiedEvaluationFunction(ptr_board, ptr_eval_state, ptr_engine, move);

    return delta_score;
}
