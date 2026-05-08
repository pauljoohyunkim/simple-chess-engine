#include <assert.h>
#include "eval/pst.h"
#include "eval/sef.h"
#include "eval/hcef.h"
#include "helper.h"
#include "dev.h"

static const int passed_pawn_mg_weights[] = { 0, 0, 0, 5, 15, 40, 80, 0 };
static const int passed_pawn_eg_weights[] = { 0, 5, 5, 20, 40, 80, 150, 0 };

typedef unsigned int uint;

static void SCE_Eval_HandcraftedEvaluationFunction_DoublePawn(int* const mg_score, int* const eg_score, const uint64_t w_pawn, const uint64_t b_pawn);
static void SCE_Eval_HandcraftedEvaluationFunction_PassedPawn(int* const mg_score, int* const eg_score, uint64_t* passed_pawns, const uint64_t w_pawn, const uint64_t b_pawn, const SCE_Precomputation_Tables* const ptr_precomputation_tables);
static void SCE_Eval_HandcraftedEvaluationFunction_PHT(int* const mg_score, int* const eg_score, uint64_t* const passed_pawns, uint64_t* const weak_pawns, const uint64_t pawn_zobrist_hash, const uint64_t w_pawn, const uint64_t b_pawn, SCE_Engine* const ptr_engine, const SCE_Precomputation_Tables* const ptr_precomputation_tables);
static void SCE_Eval_HandcraftedEvaluationFunction_DynamicCheck(int* const mg_score, int* const eg_score, uint64_t passed_pawns, const uint64_t occupancy_w, const uint64_t occupancy_b);

#define DOUBLE_PAWN_PENALTY_MG (15)
#define DOUBLE_PAWN_PENALTY_EG (20)
static void SCE_Eval_HandcraftedEvaluationFunction_DoublePawn(int* const mg_score, int* const eg_score, const uint64_t w_pawn, const uint64_t b_pawn) {
    assert(mg_score != NULL);
    assert(eg_score != NULL);

    int local_mg_score = 0;
    int local_eg_score = 0;

    for (uint i = 0U; i < CHESSBOARD_DIMENSION; i++) {
        const uint64_t file_mask = ChessboardFileMasks[i];

        const uint w_setbits = COUNT_SET_BITS(file_mask & w_pawn);
        const uint b_setbits = COUNT_SET_BITS(file_mask & b_pawn);

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

static void SCE_Eval_HandcraftedEvaluationFunction_PassedPawn(int* const mg_score, int* const eg_score, uint64_t* passed_pawns, const uint64_t w_pawn, const uint64_t b_pawn, const SCE_Precomputation_Tables* const ptr_precomputation_tables) {
    assert(mg_score != NULL);
    assert(eg_score != NULL);
    assert(passed_pawns != NULL);
    assert(ptr_precomputation_tables != NULL);

    *passed_pawns = 0U;
    int local_mg_score = 0;
    int local_eg_score = 0;
    for (uint i = 0U; i < CHESSBOARD_DIMENSION; i++) {
        uint64_t w_pawns_in_file = w_pawn & ChessboardFileMasks[i];
        uint64_t b_pawns_in_file = b_pawn & ChessboardFileMasks[i];
        // White pawn
        if (w_pawns_in_file) {
            const uint leading_pawn_idx = (63U - COUNT_LEADING_ZEROS(w_pawns_in_file));
            const uint row = leading_pawn_idx / 8;
            uint64_t passed_pawn_mask = ptr_precomputation_tables->front_span_masks[WHITE][leading_pawn_idx];
            // Check if enemy (black) pawn exists
            if (!(passed_pawn_mask & b_pawn)) {
                // Add it to passed pawns.
                *passed_pawns |= (1ULL << leading_pawn_idx);

                /*
                // Blockage penalty (passed pawn being blocked right in front is not as good)
                if ((1ULL << (leading_pawn_idx + CHESSBOARD_DIMENSION)) & occupancy) {
                    local_mg_score += mg_weights[row] / 2;
                    local_eg_score += eg_weights[row] / 2;
                } else {
                }
                */
                // Normal point
                local_mg_score += passed_pawn_mg_weights[row];
                local_eg_score += passed_pawn_eg_weights[row];
                
            }
        }

        // Black pawn
        if (b_pawns_in_file) {
            const uint leading_pawn_idx = COUNT_TRAILING_ZEROS(b_pawns_in_file);
            const uint row = leading_pawn_idx / 8;
            uint64_t passed_pawn_mask = ptr_precomputation_tables->front_span_masks[BLACK][leading_pawn_idx];
            // Check if enemy (white) pawn exists
            if (!(passed_pawn_mask & w_pawn)) {
                // Add it to passed pawns.
                *passed_pawns |= (1ULL << leading_pawn_idx);

                /*
                // Blockage penalty
                if ((1ULL << (leading_pawn_idx - CHESSBOARD_DIMENSION)) & occupancy) {
                    local_mg_score -= mg_weights[CHESSBOARD_DIMENSION-1U-row] / 2;
                    local_eg_score -= eg_weights[CHESSBOARD_DIMENSION-1U-row] / 2;
                } else {
                    // Point
                    local_mg_score -= mg_weights[CHESSBOARD_DIMENSION-1U-row];
                    local_eg_score -= eg_weights[CHESSBOARD_DIMENSION-1U-row];
                }
                */
                local_mg_score -= passed_pawn_mg_weights[CHESSBOARD_DIMENSION-1U-row];
                local_eg_score -= passed_pawn_eg_weights[CHESSBOARD_DIMENSION-1U-row];
            }
        }
    }

    *mg_score = local_mg_score;
    *eg_score = local_eg_score;
}

static void SCE_Eval_HandcraftedEvaluationFunction_PHT(int* const mg_score, int* const eg_score, uint64_t* const passed_pawns, uint64_t* const weak_pawns, const uint64_t pawn_zobrist_hash, const uint64_t w_pawn, const uint64_t b_pawn, SCE_Engine* const ptr_engine, const SCE_Precomputation_Tables* const ptr_precomputation_tables) {
    assert(mg_score != NULL);
    assert(eg_score != NULL);
    assert(passed_pawns != NULL);
    assert(weak_pawns != NULL);
    assert(pawn_zobrist_hash != 0);
    assert(ptr_engine != NULL);
    assert(ptr_precomputation_tables != NULL);

    int pawn_contrib_mg = 0;
    int pawn_contrib_eg = 0;
    {
        SCE_PawnHashTableEntry pht_entry;
        if (SCE_Engine_GetPawnHashData(&pht_entry, ptr_engine, pawn_zobrist_hash)) {
            // Read success
            pawn_contrib_mg = SCE_PHT_GET_MG_SCORE(pht_entry.score_data);
            pawn_contrib_eg = SCE_PHT_GET_EG_SCORE(pht_entry.score_data);
            *passed_pawns = pht_entry.passed_pawns;
        } else {
            {
                // Double pawns
                int double_pawn_mg = 0;
                int double_pawn_eg = 0;
                SCE_Eval_HandcraftedEvaluationFunction_DoublePawn(&double_pawn_mg, &double_pawn_eg, w_pawn, b_pawn);
                pawn_contrib_mg += double_pawn_mg;
                pawn_contrib_eg += double_pawn_eg;
            }
            {
                // Passed pawns
                int passed_pawn_mg = 0;
                int passed_pawn_eg = 0;
                SCE_Eval_HandcraftedEvaluationFunction_PassedPawn(&passed_pawn_mg, &passed_pawn_eg, passed_pawns, w_pawn, b_pawn, ptr_precomputation_tables);
                pawn_contrib_mg += passed_pawn_mg;
                pawn_contrib_eg += passed_pawn_eg;
            }
            {
                // TODO: Weak pawns
            }
            // Cache
            // For now, 0U: Not taking into account for weak pawns yet for testing.
            SCE_Engine_AddPawnHashData(ptr_engine, pawn_zobrist_hash, pawn_contrib_mg, pawn_contrib_eg, *passed_pawns, 0U);
        }
    }
    *mg_score = pawn_contrib_mg;
    *eg_score = pawn_contrib_eg;
}

static void SCE_Eval_HandcraftedEvaluationFunction_DynamicCheck(int* const mg_score, int* const eg_score, uint64_t passed_pawns, const uint64_t occupancy_w, const uint64_t occupancy_b) {
    assert(mg_score != NULL);
    assert(eg_score != NULL);

    *mg_score = 0;
    *eg_score = 0;

    const uint64_t occupancy = occupancy_w | occupancy_b;

    while (passed_pawns) {
        // Bit scan
        // 1. Get index.
        // 2. Get the type
        // 3. Check for blockage.
        // 4. Update
        const uint idx = COUNT_TRAILING_ZEROS(passed_pawns);
        const uint row = idx / CHESSBOARD_DIMENSION;
        const uint64_t passed_pawn = 1ULL << idx;
        assert(passed_pawn & occupancy);

        if (passed_pawn & occupancy_w) {
            // White pawn
            if ((1ULL << (idx + CHESSBOARD_DIMENSION)) & occupancy) {
                // Blocked
                *mg_score -= passed_pawn_mg_weights[row] / 2;
                *eg_score -= passed_pawn_mg_weights[row] / 2;
            }
        } else {
            // Black pawn
            if ((1ULL << (idx - CHESSBOARD_DIMENSION)) & occupancy) {
                // Blocked
                *mg_score += passed_pawn_mg_weights[CHESSBOARD_DIMENSION - 1U - row] / 2;
                *eg_score += passed_pawn_eg_weights[CHESSBOARD_DIMENSION - 1U - row] / 2;
            }
        }

        // Remove from the passed pawns for scanning.
        passed_pawns &= ~passed_pawn;
    }
}

int SCE_Eval_HandcraftedEvaluationFunction(SCE_Context* const ctx, SCE_Engine* const ptr_engine) {
    assert(ctx != NULL);

    // This fills ctx->eval_stat
    SCE_Eval_SimplifiedEvaluationFunction(ctx, ptr_engine);

    int pawn_contrib_mg = 0;
    int pawn_contrib_eg = 0;
    uint64_t passed_pawns = 0U;
    uint64_t weak_pawns = 0U;
    {
        int pawn_contrib_pht_mg = 0;
        int pawn_contrib_pht_eg = 0;
        // This updates passed_pawns
        SCE_Eval_HandcraftedEvaluationFunction_PHT(&pawn_contrib_pht_mg, &pawn_contrib_pht_eg, &passed_pawns, &weak_pawns, ctx->board.pawn_zobrist_hash, ctx->board.bitboards[W_PAWN], ctx->board.bitboards[B_PAWN], ptr_engine, ctx->precomputation_tables);
        pawn_contrib_mg += pawn_contrib_pht_mg;
        pawn_contrib_eg += pawn_contrib_pht_eg;
    }
    {
        int pawn_contrib_dynamic_mg = 0;
        int pawn_contrib_dynamic_eg = 0;
        const uint64_t occupancy_w = SCE_Chessboard_Occupancy_Color(ctx, WHITE);
        const uint64_t occupancy_b = SCE_Chessboard_Occupancy_Color(ctx, BLACK);
        SCE_Eval_HandcraftedEvaluationFunction_DynamicCheck(&pawn_contrib_dynamic_mg, &pawn_contrib_dynamic_eg, passed_pawns, occupancy_w, occupancy_b);
        pawn_contrib_mg += pawn_contrib_dynamic_mg;
        pawn_contrib_eg += pawn_contrib_dynamic_eg;
    }
    

    const int phase = ctx->eval_state.phase > TOTAL_PHASE_WEIGHT ? TOTAL_PHASE_WEIGHT : ctx->eval_state.phase;
    const int mg_score = ctx->eval_state.mg_score + pawn_contrib_mg;
    const int eg_score = ctx->eval_state.eg_score + pawn_contrib_eg;

    return (mg_score * phase + eg_score * (TOTAL_PHASE_WEIGHT - phase)) / TOTAL_PHASE_WEIGHT;
}

#define IS_WHITE(piecetype) (piecetype < B_PAWN)
#define IS_BLACK(piecetype) (piecetype >= B_PAWN)
int SCE_DeltaEval_HandcraftedEvaluationFunction(SCE_Context* const ctx, SCE_EvalState* const ptr_eval_state, SCE_Engine* const ptr_engine, const SCE_ChessMove move) {
    assert(ctx != NULL);
    assert(ptr_eval_state != NULL);
    assert(ptr_engine != NULL);
    assert(move != EMPTY_MOVE);

    uint64_t w_pawns = ctx->board.bitboards[W_PAWN];
    uint64_t b_pawns = ctx->board.bitboards[B_PAWN];
    uint64_t occupancy_w = SCE_Chessboard_Occupancy_Color(ctx, WHITE);
    uint64_t occupancy_b = SCE_Chessboard_Occupancy_Color(ctx, BLACK);
    uint64_t pawn_zobrist_hash = ctx->board.pawn_zobrist_hash;

    // TODO: Modify occupancy_w, occupancy_b, pawn_zobrist_hash, w_pawns, b_pawns depending on the move.
    // 0. No pawn involved general moves.
    // 1. Move is a promotion/promo-capture.
    // 2. Move is a pawn move (normal pawn move, pawn capture, en passant)
    // 3. Move is capture not involving pawns
    // 4. Move is castling.
    // Use the implementation of MakeMove as a guide.
    {
        const uint src_idx = move SCE_CHESSMOVE_GET_SRC;
        const uint dst_idx = move SCE_CHESSMOVE_GET_DST;
        const uint64_t src = 1ULL << src_idx;
        const uint64_t dst = 1ULL << dst_idx;
        int captured_idx = UNASSIGNED;
        const int flag = move SCE_CHESSMOVE_GET_FLAG;
        const PieceType src_piece_type = ctx->board.mailbox[src_idx];
        PieceType captured_piece_type = UNASSIGNED_PIECE_TYPE;
        assert(src_piece_type != UNASSIGNED_PIECE_TYPE);

        // Determine captured piece type.
        if (flag == SCE_CHESSMOVE_FLAG_EN_PASSANT_CAPTURE) {
            captured_idx = IS_WHITE(src_piece_type) ? ctx->board.en_passant_idx - CHESSBOARD_DIMENSION : ctx->board.en_passant_idx + CHESSBOARD_DIMENSION;
            captured_piece_type = ctx->board.mailbox[captured_idx];
        } else {
            captured_idx = dst_idx;
            captured_piece_type = ctx->board.mailbox[dst_idx];
        }

        // Execution
        // 1. Capture
        // 2. Move
        // 3. Flag action

        // 1. Capture
        if (flag & SCE_CHESSMOVE_FLAG_CAPTURE) {
            assert(captured_piece_type != UNASSIGNED_PIECE_TYPE);
            const uint64_t captured_piece = 1ULL << captured_idx;
            if (flag == SCE_CHESSMOVE_FLAG_EN_PASSANT_CAPTURE) {
                // En passant
                assert(captured_piece_type == W_PAWN || captured_piece_type == B_PAWN);
                if (captured_piece_type == W_PAWN) {
                    w_pawns ^= captured_piece;
                    occupancy_w ^= captured_piece;
                    pawn_zobrist_hash ^= ctx->precomputation_tables->zobrist_table.piece_key[captured_piece_type][captured_idx];
                } else {
                    b_pawns ^= captured_piece;
                    occupancy_b ^= captured_piece;
                    pawn_zobrist_hash ^= ctx->precomputation_tables->zobrist_table.piece_key[captured_piece_type][captured_idx];
                }
            } else {
                // Pawn specific
                switch (captured_piece_type) {
                    case W_PAWN:
                        w_pawns ^= captured_piece;
                        pawn_zobrist_hash ^= ctx->precomputation_tables->zobrist_table.piece_key[captured_piece_type][captured_idx];
                        break;
                    case B_PAWN:
                        b_pawns ^= captured_piece;
                        pawn_zobrist_hash ^= ctx->precomputation_tables->zobrist_table.piece_key[captured_piece_type][captured_idx];
                        break;
                    default:
                        break;
                }
                if (IS_WHITE(captured_piece_type)) {
                    // White captured
                    occupancy_w ^= captured_piece;
                } else {
                    // Black captured
                    occupancy_b ^= captured_piece;
                }
            }
        }

        // 2. Standard move
        if (IS_WHITE(src_piece_type)) {
            if (src_piece_type == W_PAWN) {
                // Pawn specifics
                w_pawns ^= src | dst;
                pawn_zobrist_hash ^= ctx->precomputation_tables->zobrist_table.piece_key[src_piece_type][src_idx];
                pawn_zobrist_hash ^= ctx->precomputation_tables->zobrist_table.piece_key[src_piece_type][dst_idx];
            }
            occupancy_w ^= src | dst;

        } else {
            // Black
            if (src_piece_type == B_PAWN) {
                // Pawn specifics
                b_pawns ^= src | dst;
                pawn_zobrist_hash ^= ctx->precomputation_tables->zobrist_table.piece_key[src_piece_type][src_idx];
                pawn_zobrist_hash ^= ctx->precomputation_tables->zobrist_table.piece_key[src_piece_type][dst_idx];
            }
            occupancy_b ^= src | dst;
        }

        // 3. Flag
        switch (flag) {
            case SCE_CHESSMOVE_FLAG_KNIGHT_PROMOTION:
            case SCE_CHESSMOVE_FLAG_KNIGHT_PROMO_CAPTURE:
            case SCE_CHESSMOVE_FLAG_BISHOP_PROMOTION:
            case SCE_CHESSMOVE_FLAG_BISHOP_PROMO_CAPTURE:
            case SCE_CHESSMOVE_FLAG_ROOK_PROMOTION:
            case SCE_CHESSMOVE_FLAG_ROOK_PROMO_CAPTURE:
            case SCE_CHESSMOVE_FLAG_QUEEN_PROMOTION:
            case SCE_CHESSMOVE_FLAG_QUEEN_PROMO_CAPTURE:
                pawn_zobrist_hash ^= ctx->precomputation_tables->zobrist_table.piece_key[src_piece_type][dst_idx];
                
                if (IS_WHITE(src_piece_type)) {
                    w_pawns ^= dst;
                } else {
                    b_pawns ^= dst;
                }
                break;
            case SCE_CHESSMOVE_FLAG_KING_CASTLE:
                {
                    const uint rook_idx_src = dst_idx + 1U;
                    const uint rook_idx_dst = dst_idx - 1U;
                    const uint64_t rook_src = 1ULL << rook_idx_src;
                    const uint64_t rook_dst = 1ULL << rook_idx_dst;
                    if (IS_WHITE(src_piece_type)) {
                        occupancy_w ^= rook_src ^ rook_dst;
                    } else {
                        occupancy_b ^= rook_src ^ rook_dst;
                    }
                }
                break;
            case SCE_CHESSMOVE_FLAG_QUEEN_CASTLE:
                {
                    const uint rook_idx_src = dst_idx - 2U;
                    const uint rook_idx_dst = dst_idx + 1U;
                    const uint64_t rook_src = 1ULL << rook_idx_src;
                    const uint64_t rook_dst = 1ULL << rook_idx_dst;
                    if (IS_WHITE(src_piece_type)) {
                        occupancy_w ^= rook_src ^ rook_dst;
                    } else {
                        occupancy_b ^= rook_src ^ rook_dst;
                    }
                }
                break;
            default:
                break;
        }
    }

    SCE_DeltaEval_SimplifiedEvaluationFunction(ctx, ptr_eval_state, ptr_engine, move);

    int pawn_contrib_mg = 0;
    int pawn_contrib_eg = 0;
    uint64_t passed_pawns = 0U;
    uint64_t weak_pawns = 0U;
    {
        int pawn_contrib_pht_mg = 0;
        int pawn_contrib_pht_eg = 0;
        SCE_Eval_HandcraftedEvaluationFunction_PHT(&pawn_contrib_pht_mg, &pawn_contrib_pht_eg, &passed_pawns, &weak_pawns, pawn_zobrist_hash, w_pawns, b_pawns, ptr_engine, ctx->precomputation_tables);
        pawn_contrib_mg += pawn_contrib_pht_mg;
        pawn_contrib_eg += pawn_contrib_pht_eg;
    }
    {
        
        int pawn_contrib_dynamic_mg = 0;
        int pawn_contrib_dynamic_eg = 0;
        SCE_Eval_HandcraftedEvaluationFunction_DynamicCheck(&pawn_contrib_dynamic_mg, &pawn_contrib_dynamic_eg, passed_pawns, occupancy_w, occupancy_b);
        pawn_contrib_mg += pawn_contrib_dynamic_mg;
        pawn_contrib_eg += pawn_contrib_dynamic_eg;
    }

    const int phase = ptr_eval_state->phase > TOTAL_PHASE_WEIGHT ? TOTAL_PHASE_WEIGHT : ptr_eval_state->phase;
    const int mg_score = ptr_eval_state->mg_score + pawn_contrib_mg;
    const int eg_score = ptr_eval_state->eg_score + pawn_contrib_eg;

    return (mg_score * phase + eg_score * (TOTAL_PHASE_WEIGHT - phase)) / TOTAL_PHASE_WEIGHT;
}
