#include <assert.h>
#include "eval/sef.h"
#include "eval/hcef.h"

int SCE_Eval_HandcraftedEvaluationFunction(SCE_Context* const ctx) {
    assert(ctx != NULL);

    int score = SCE_Eval_SimplifiedEvaluationFunction(ctx);

    return score;
}

int SCE_DeltaEval_HandcraftedEvaluationFunction(const SCE_Chessboard* const ptr_board, SCE_EvalState* const ptr_eval_state, const SCE_ChessMove move) {
    assert(ptr_board != NULL);
    assert(ptr_eval_state != NULL);

    int delta_score = SCE_DeltaEval_SimplifiedEvaluationFunction(ptr_board, ptr_eval_state, move);

    return delta_score;
}
