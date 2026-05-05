#include <gtest/gtest.h>
#include "chess.h"
#include "engine.h"
#include "eval/hcef.h"
#include "dev.h"
#include "fen.h"

#define DEBUG_TT_N_SIZE (24U)

static void DeltaEvalTest(SCE_Context* const ctx, SCE_Engine* const ptr_engine, const int depth) {
    if (depth == 0) {
        return;
    }

    SCE_ChessMoveList movelist;
    ASSERT_EQ(SCE_ChessMoveList_clear(&movelist), SCE_SUCCESS);

    ASSERT_EQ(SCE_GenerateLegalMoves(&movelist, ctx), SCE_SUCCESS);

    for (unsigned int i = 0; i < movelist.count; i++) {
        const SCE_ChessMove move = movelist.moves[i];

        SCE_EvalState temp_eval_state = ctx->eval_state;
        const int delta_evaluated = SCE_DeltaEval_HandcraftedEvaluationFunction(ctx, &temp_eval_state, ptr_engine, move);

        ASSERT_EQ(SCE_MakeMove(ctx, move), SCE_SUCCESS);
        ctx->eval_state = temp_eval_state;

        const int full_evaluated = SCE_Eval_HandcraftedEvaluationFunction(ctx, ptr_engine);
        ASSERT_EQ(delta_evaluated, full_evaluated);
        ASSERT_EQ(temp_eval_state.eg_score, ctx->eval_state.eg_score);
        ASSERT_EQ(temp_eval_state.mg_score, ctx->eval_state.mg_score);
        ASSERT_EQ(temp_eval_state.phase, ctx->eval_state.phase);
        //ASSERT_TRUE(temp_eval_state.phase <= 24);
        //ASSERT_TRUE(ctx->eval_state.phase <= 24);

        DeltaEvalTest(ctx, ptr_engine, depth-1);


        ASSERT_EQ(SCE_UnmakeMove(ctx), SCE_SUCCESS);
    }
}

TEST(HCEF, Initial) {
    SCE_Return ret; 
    SCE_Precomputation_Tables precomputation_table;
    ret = SCE_Precomputation_Tables_init(&precomputation_table, NULL);
    ASSERT_EQ(ret, SCE_SUCCESS);

    SCE_Context ctx;
    ret = SCE_Context_init(&ctx, &precomputation_table);
    ASSERT_EQ(ret, SCE_SUCCESS);

    // Note that initialization is not necessary as only interested in PHT
    SCE_Engine engine;
    memset(&engine.pawn_hash_table, 0, sizeof(engine.pawn_hash_table));

    ret = SCE_Chessboard_FEN_setup(&ctx, "1k6/3p4/4p3/8/2P4p/7P/1PP2p2/5K2 w - - 0 1");
    ASSERT_EQ(ret, SCE_SUCCESS);

    const int score1 = SCE_Eval_HandcraftedEvaluationFunction(&ctx, &engine);
    const int score2 = SCE_Eval_HandcraftedEvaluationFunction(&ctx, &engine);
    ASSERT_EQ(score1, score2);
}

TEST(HCEF, DeltaEval_Initial) {
    SCE_Context ctx;
    SCE_Precomputation_Tables precomputation_tables;
    ASSERT_EQ(SCE_Precomputation_Tables_init(&precomputation_tables, NULL), SCE_SUCCESS);
    ASSERT_EQ(SCE_Context_init(&ctx, &precomputation_tables), SCE_SUCCESS);

    // While engine generation is not needed, this is used for precomputing the first evaluation.
    SCE_Engine engine;
    ASSERT_EQ(SCE_Engine_init(&ctx, &engine, SCE_Eval_HandcraftedEvaluationFunction, SCE_DeltaEval_HandcraftedEvaluationFunction, DEBUG_TT_N_SIZE), SCE_SUCCESS);

    SCE_ChessMoveList movelist;
    ASSERT_EQ(SCE_ChessMoveList_clear(&movelist), SCE_SUCCESS);

    ASSERT_EQ(SCE_GenerateLegalMoves(&movelist, &ctx), SCE_SUCCESS);
    ASSERT_NE(movelist.count, 0);

    // For each move, see if the evaluations match.
    for (unsigned int i = 0; i < movelist.count; i++) {
        const SCE_ChessMove move = movelist.moves[i];

        SCE_EvalState temp_eval_state = ctx.eval_state;
        const int delta_evaluated = SCE_DeltaEval_HandcraftedEvaluationFunction(&ctx, &temp_eval_state, &engine, move);

        ASSERT_EQ(SCE_MakeMove(&ctx, move), SCE_SUCCESS);
        ctx.eval_state = temp_eval_state;
        const int full_evaluated = SCE_Eval_HandcraftedEvaluationFunction(&ctx, &engine);

        assert(delta_evaluated == full_evaluated);
        ASSERT_EQ(delta_evaluated, full_evaluated);

        ASSERT_EQ(SCE_UnmakeMove(&ctx), SCE_SUCCESS);
    }
    ASSERT_EQ(SCE_Engine_release(&engine), SCE_SUCCESS);
}

#ifndef UNITTEST_FULL
TEST(HCEF, DeltaEval_Kiwipete_Depth_4) {
#else
TEST(HCEF, DeltaEval_Kiwipete_Depth_6) {
#endif
    SCE_Context ctx;
    SCE_Precomputation_Tables precomputation_tables;
    ASSERT_EQ(SCE_Precomputation_Tables_init(&precomputation_tables, NULL), SCE_SUCCESS);
    ASSERT_EQ(SCE_Context_init(&ctx, &precomputation_tables), SCE_SUCCESS);
    ASSERT_EQ(SCE_Chessboard_FEN_setup(&ctx, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 0"), SCE_SUCCESS);

    // While engine generation is not needed, this is used for precomputing the first evaluation.
    SCE_Engine engine;
    ASSERT_EQ(SCE_Engine_init(&ctx, &engine, SCE_Eval_HandcraftedEvaluationFunction, SCE_DeltaEval_HandcraftedEvaluationFunction, DEBUG_TT_N_SIZE), SCE_SUCCESS);

#ifndef UNITTEST_FULL
    DeltaEvalTest(&ctx, &engine, 4);
#else
    DeltaEvalTest(&ctx, &engine, 6);
#endif

    ASSERT_EQ(SCE_Engine_release(&engine), SCE_SUCCESS);
}
