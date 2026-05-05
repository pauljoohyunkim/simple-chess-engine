#include <assert.h>
#include "eval/pst.h"
#include "eval/sef.h"
#include "eval/hcef.h"
#include "helper.h"

typedef unsigned int uint;

static void SCE_Eval_HandcraftedEvaluationFunction_DoublePawn(int* const mg_score, int* const eg_score, const SCE_Chessboard* const ptr_board);
static void SCE_Eval_HandcraftedEvaluationFunction_PassedPawn(int* const mg_score, int* const eg_score, uint64_t* passed_pawns, const SCE_Context* const ctx);

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

static void SCE_Eval_HandcraftedEvaluationFunction_PassedPawn(int* const mg_score, int* const eg_score, uint64_t* passed_pawns, const SCE_Context* const ctx) {
    assert(mg_score != NULL);
    assert(eg_score != NULL);
    assert(passed_pawns != NULL);
    assert(ctx != NULL);

    *passed_pawns = 0U;
    int mg_weights[] = { 0, 0, 0, 5, 15, 40, 80, 0 };
    int eg_weights[] = { 0, 5, 5, 20, 40, 80, 150, 0 };
    int local_mg_score = 0;
    int local_eg_score = 0;
    const uint64_t occupancy = SCE_Chessboard_Occupancy(ctx);
    for (uint i = 0U; i < CHESSBOARD_DIMENSION; i++) {
        uint64_t w_pawns_in_file = ctx->board.bitboards[W_PAWN] & ChessboardFileMasks[i];
        uint64_t b_pawns_in_file = ctx->board.bitboards[B_PAWN] & ChessboardFileMasks[i];
        // White pawn
        if (w_pawns_in_file) {
            const uint leading_pawn_idx = (63U - COUNT_LEADING_ZEROS(w_pawns_in_file));
            const uint row = leading_pawn_idx / 8;
            const uint col = leading_pawn_idx % 8;
            uint64_t passed_pawn_mask = ctx->precomputation_tables->front_span_masks[WHITE][leading_pawn_idx];
            // Check if enemy (black) pawn exists
            if (!(passed_pawn_mask & ctx->board.bitboards[B_PAWN])) {
                // Add it to passed pawns.
                *passed_pawns |= (1ULL << leading_pawn_idx);

                // Blockage penalty (passed pawn being blocked right in front is not as good)
                if ((1ULL << (leading_pawn_idx + CHESSBOARD_DIMENSION)) & occupancy) {
                    local_mg_score += mg_weights[row] / 2;
                    local_eg_score += eg_weights[row] / 2;
                } else {
                    // Normal point
                    local_mg_score += mg_weights[row];
                    local_eg_score += eg_weights[row];
                }
                
            }
        }

        // Black pawn
        if (b_pawns_in_file) {
            const uint leading_pawn_idx = COUNT_TRAILING_ZEROS(b_pawns_in_file);
            const uint row = leading_pawn_idx / 8;
            const uint col = leading_pawn_idx % 8;
            uint64_t passed_pawn_mask = ctx->precomputation_tables->front_span_masks[BLACK][leading_pawn_idx];
            // Check if enemy (white) pawn exists
            if (!(passed_pawn_mask & ctx->board.bitboards[W_PAWN])) {
                // Add it to passed pawns.
                *passed_pawns |= (1ULL << leading_pawn_idx);

                // Blockage penalty
                if ((1ULL << (leading_pawn_idx - CHESSBOARD_DIMENSION)) & occupancy) {
                    local_mg_score -= mg_weights[CHESSBOARD_DIMENSION-1U-row] / 2;
                    local_eg_score -= eg_weights[CHESSBOARD_DIMENSION-1U-row] / 2;
                } else {
                    // Point
                    local_mg_score -= mg_weights[CHESSBOARD_DIMENSION-1U-row];
                    local_eg_score -= eg_weights[CHESSBOARD_DIMENSION-1U-row];
                }
            }
        }
    }

    *mg_score = local_mg_score;
    *eg_score = local_eg_score;
}

int SCE_Eval_HandcraftedEvaluationFunction(SCE_Context* const ctx, SCE_Engine* const ptr_engine) {
    assert(ctx != NULL);

    // This fills ctx->eval_stat
    SCE_Eval_SimplifiedEvaluationFunction(ctx, ptr_engine);

    int pawn_contrib_mg = 0;
    int pawn_contrib_eg = 0;
    {
        SCE_PawnHashTableEntry pht_entry;
        if (SCE_Engine_GetPawnHashData(&pht_entry, ptr_engine, ctx->board.pawn_zobrist_hash)) {
            // Read success
            pawn_contrib_mg = SCE_PHT_GET_MG_SCORE(pht_entry.score_data);
            pawn_contrib_eg = SCE_PHT_GET_EG_SCORE(pht_entry.score_data);
        } else {
            uint64_t passed_pawns = 0U;
            {
                // Double pawns
                int double_pawn_mg = 0;
                int double_pawn_eg = 0;
                SCE_Eval_HandcraftedEvaluationFunction_DoublePawn(&double_pawn_mg, &double_pawn_eg, &ctx->board);
                pawn_contrib_mg += double_pawn_mg;
                pawn_contrib_eg += double_pawn_eg;
            }
            {
                // Passed pawns
                int passed_pawn_mg = 0;
                int passed_pawn_eg = 0;
                SCE_Eval_HandcraftedEvaluationFunction_PassedPawn(&passed_pawn_mg, &passed_pawn_eg, &passed_pawns, ctx);
                pawn_contrib_mg += passed_pawn_mg;
                pawn_contrib_eg += passed_pawn_eg;
            }
            {
                // TODO: Weak pawns
            }
            // Cache
            // For now, 0U: Not taking into account for weak pawns yet for testing.
            SCE_Engine_AddPawnHashData(ptr_engine, ctx->board.pawn_zobrist_hash, pawn_contrib_mg, pawn_contrib_eg, passed_pawns, 0U);
        }
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
