#include <assert.h>
#include "eval/pst.h"
#include "eval/sef.h"
#include "eval/hcef.h"
#include "helper.h"

typedef unsigned int uint;

static void SCE_Eval_HandcraftedEvaluationFunction_DoublePawn(int* const mg_score, int* const eg_score, SCE_Context* const ctx);

#define DOUBLE_PAWN_PENALTY_MG (15)
#define DOUBLE_PAWN_PENALTY_EG (20)
static void SCE_Eval_HandcraftedEvaluationFunction_DoublePawn(int* const mg_score, int* const eg_score, SCE_Context* const ctx) {
    assert(mg_score != NULL);
    assert(eg_score != NULL);
    assert(ctx != NULL);

    int local_mg_score = 0;
    int local_eg_score = 0;

    for (uint i = 0U; i < CHESSBOARD_DIMENSION; i++) {
        const uint64_t file_mask = ChessboardFileMasks[i];

        const uint w_setbits = COUNT_SET_BITS(file_mask & ctx->board.bitboards[W_PAWN]);
        const uint b_setbits = COUNT_SET_BITS(file_mask & ctx->board.bitboards[B_PAWN]);

        if (w_setbits >= 2U) {
            local_mg_score -= (w_setbits - 1U) * DOUBLE_PAWN_PENALTY_MG;
            local_eg_score -= (w_setbits - 1U) * DOUBLE_PAWN_PENALTY_EG;
        }
        if (b_setbits >= 2U) {
            local_mg_score += (b_setbits - 1U) * DOUBLE_PAWN_PENALTY_MG;
            local_eg_score += (b_setbits - 1U) * DOUBLE_PAWN_PENALTY_EG;
        }
    }

    *mg_score = local_mg_score;
    *eg_score = local_eg_score;
}

int SCE_Eval_HandcraftedEvaluationFunction(SCE_Context* const ctx, SCE_Engine* const ptr_engine) {
    assert(ctx != NULL);

    // This fills ctx->eval_stat
    SCE_Eval_SimplifiedEvaluationFunction(ctx, ptr_engine);

    int double_pawn_mg;
    int double_pawn_eg;
    SCE_PawnHashTableEntry pht_entry;
    if (SCE_Engine_GetPawnHashData(&pht_entry, ptr_engine, ctx->board.pawn_zobrist_hash)) {
        // Read success
        double_pawn_mg = SCE_PHT_GET_MG_SCORE(pht_entry.score_data);
        double_pawn_eg = SCE_PHT_GET_EG_SCORE(pht_entry.score_data);
    } else {
        SCE_Eval_HandcraftedEvaluationFunction_DoublePawn(&double_pawn_mg, &double_pawn_eg, ctx);
        // Cache
        // For now, 0U: Not taking into account for weak pawns or passed pawns yet for testing.
        SCE_Engine_AddPawnHashData(ptr_engine, ctx->board.pawn_zobrist_hash, double_pawn_mg, double_pawn_eg, 0U, 0U);
    }
    

    const int phase = ctx->eval_state.phase > TOTAL_PHASE_WEIGHT ? TOTAL_PHASE_WEIGHT : ctx->eval_state.phase;
    const int mg_score = ctx->eval_state.mg_score + double_pawn_mg;
    const int eg_score = ctx->eval_state.eg_score + double_pawn_eg;

    return (mg_score * phase + eg_score * (TOTAL_PHASE_WEIGHT - phase)) / TOTAL_PHASE_WEIGHT;
}

int SCE_DeltaEval_HandcraftedEvaluationFunction(SCE_Chessboard* const ptr_board, SCE_EvalState* const ptr_eval_state, SCE_Engine* const ptr_engine, const SCE_ChessMove move) {
    assert(ptr_board != NULL);
    assert(ptr_eval_state != NULL);

    int delta_score = SCE_DeltaEval_SimplifiedEvaluationFunction(ptr_board, ptr_eval_state, ptr_engine, move);

    return delta_score;
}
