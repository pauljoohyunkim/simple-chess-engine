#include <gtest/gtest.h>
#include "chess.h"
#include "engine.h"

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
        const int delta_evaluated = ptr_engine->delta_eval_function(ctx, &temp_eval_state, ptr_engine, move);

        ASSERT_EQ(SCE_MakeMove(ctx, move), SCE_SUCCESS);
        ctx->eval_state = temp_eval_state;

        const int full_evaluated = ptr_engine->eval_function(ctx, ptr_engine);
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