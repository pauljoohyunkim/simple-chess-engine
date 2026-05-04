#include <assert.h>
#include "eval/pst.h"
#include "eval/sef.h"
#include "eval/hcef.h"
#include "helper.h"

typedef unsigned int uint;

static void SCE_Eval_HandcraftedEvaluationFunction_DoublePawn(int* const mg_score, int* const eg_score, const SCE_Chessboard* const ptr_board);
static void SCE_Eval_HandcraftedEvaluationFunction_PassedPawn(int* const mg_score, int* const eg_score, uint64_t* passed_pawns, const SCE_Chessboard* const ptr_board);

#define DOUBLE_PAWN_PENALTY_MG (15)
#define DOUBLE_PAWN_PENALTY_EG (20)
static void SCE_Eval_HandcraftedEvaluationFunction_DoublePawn(int* const mg_score, int* const eg_score, const SCE_Chessboard* const ptr_board) {
    assert(mg_score != NULL);
    assert(eg_score != NULL);
    assert(ptr_board != NULL);

    int local_mg_score = 0;
    int local_eg_score = 0;

    for (uint i = 0U; i < CHESSBOARD_DIMENSION; i++) {
        const uint64_t file_mask = ChessboardFileMasks[i];

        const uint w_setbits = COUNT_SET_BITS(file_mask & ptr_board->bitboards[W_PAWN]);
        const uint b_setbits = COUNT_SET_BITS(file_mask & ptr_board->bitboards[B_PAWN]);

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

static void SCE_Eval_HandcraftedEvaluationFunction_PassedPawn(int* const mg_score, int* const eg_score, uint64_t* passed_pawns, const SCE_Chessboard* const ptr_board) {
    assert(mg_score != NULL);
    assert(eg_score != NULL);
    assert(passed_pawns != NULL);
    assert(ptr_board != NULL);

    *passed_pawns = 0U;
    for (uint i = 0U; i < CHESSBOARD_DIMENSION; i++) {
        uint64_t w_pawns_in_file = ptr_board->bitboards[W_PAWN] & ChessboardFileMasks[i];
        uint64_t b_pawns_in_file = ptr_board->bitboards[B_PAWN] & ChessboardFileMasks[i];
        // White pawn
        if (w_pawns_in_file) {
            uint leading_pawn_idx = (63U - COUNT_LEADING_ZEROS(w_pawns_in_file));
            // TODO: Use Precomputation table.
            //uint64_t passed_pawn_mask = 
            // Check if enemy (black) pawn exists in the front and adjacent files.
        }

        // Black pawn
        if (b_pawns_in_file) {
            uint leading_pawn_idx = COUNT_TRAILING_ZEROS(b_pawns_in_file);
        }
    }
}

int SCE_Eval_HandcraftedEvaluationFunction(SCE_Context* const ctx, SCE_Engine* const ptr_engine) {
    assert(ctx != NULL);

    // This fills ctx->eval_stat
    SCE_Eval_SimplifiedEvaluationFunction(ctx, ptr_engine);

    int pawn_contrib_mg = 0;
    int pawn_contrib_eg = 0;
    {
        int double_pawn_mg = 0;
        int double_pawn_eg = 0;
        SCE_PawnHashTableEntry pht_entry;
        if (SCE_Engine_GetPawnHashData(&pht_entry, ptr_engine, ctx->board.pawn_zobrist_hash)) {
            // Read success
            double_pawn_mg = SCE_PHT_GET_MG_SCORE(pht_entry.score_data);
            double_pawn_eg = SCE_PHT_GET_EG_SCORE(pht_entry.score_data);
        } else {
            SCE_Eval_HandcraftedEvaluationFunction_DoublePawn(&double_pawn_mg, &double_pawn_eg, &ctx->board);
            // Cache
            // For now, 0U: Not taking into account for weak pawns or passed pawns yet for testing.
            SCE_Engine_AddPawnHashData(ptr_engine, ctx->board.pawn_zobrist_hash, double_pawn_mg, double_pawn_eg, 0U, 0U);
        }
        pawn_contrib_mg += double_pawn_mg;
        pawn_contrib_eg += double_pawn_eg;
    }
    

    const int phase = ctx->eval_state.phase > TOTAL_PHASE_WEIGHT ? TOTAL_PHASE_WEIGHT : ctx->eval_state.phase;
    const int mg_score = ctx->eval_state.mg_score + pawn_contrib_mg;
    const int eg_score = ctx->eval_state.eg_score + pawn_contrib_eg;

    return (mg_score * phase + eg_score * (TOTAL_PHASE_WEIGHT - phase)) / TOTAL_PHASE_WEIGHT;
}

int SCE_DeltaEval_HandcraftedEvaluationFunction(SCE_Context* const ctx, SCE_EvalState* const ptr_eval_state, SCE_Engine* const ptr_engine, const SCE_ChessMove move) {
    assert(ctx != NULL);
    assert(ptr_eval_state != NULL);

    int delta_score = SCE_DeltaEval_SimplifiedEvaluationFunction(ctx, ptr_eval_state, ptr_engine, move);

    int pawn_contrib_mg = 0;
    int pawn_contrib_eg = 0;
    SCE_PawnHashTableEntry pht_entry;
    if (SCE_Engine_GetPawnHashData(&pht_entry, ptr_engine, ctx->board.pawn_zobrist_hash)) {
        // Read success
        pawn_contrib_mg = SCE_PHT_GET_MG_SCORE(pht_entry.score_data);
        pawn_contrib_eg = SCE_PHT_GET_EG_SCORE(pht_entry.score_data);
    } else {
        SCE_Eval_HandcraftedEvaluationFunction_DoublePawn(&pawn_contrib_mg, &pawn_contrib_eg, &ctx->board);
        // Cache
        // For now, 0U: Not taking into account for weak pawns or passed pawns yet for testing.
        // TODO: Update
        //SCE_Engine_AddPawnHashData(ptr_engine, ctx->board.pawn_zobrist_hash, double_pawn_mg, double_pawn_eg, 0U, 0U);
    }

    // TODO: Tapered Eval
    return delta_score;
}
