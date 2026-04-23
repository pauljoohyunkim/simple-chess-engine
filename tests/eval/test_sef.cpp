#include <gtest/gtest.h>
#include "chess.h"
#include "eval/sef.h"
#include "fen.h"
#include "dev.h"
#include "../setup.h"

#define DEBUG_TT_N_SIZE (24U)

void DeltaEvalTest(SCE_Context* const ctx, const int depth) {
    if (depth == 0) {
        return;
    }

    SCE_ChessMoveList movelist;
    ASSERT_EQ(SCE_ChessMoveList_clear(&movelist), SCE_SUCCESS);

    ASSERT_EQ(SCE_GenerateLegalMoves(&movelist, ctx), SCE_SUCCESS);

    for (unsigned int i = 0; i < movelist.count; i++) {
        const SCE_ChessMove move = movelist.moves[i];

        SCE_EvalState temp_eval_state = ctx->eval_state;
        const int delta_evaluated = SCE_DeltaEval_SimplifiedEvaluationFunction(&ctx->board, &temp_eval_state, move);

        ASSERT_EQ(SCE_MakeMove(ctx, move), SCE_SUCCESS);
        ctx->eval_state = temp_eval_state;

        const int full_evaluated = SCE_Eval_SimplifiedEvaluationFunction(ctx);
        ASSERT_EQ(delta_evaluated, full_evaluated);
        ASSERT_EQ(temp_eval_state.eg_score, ctx->eval_state.eg_score);
        ASSERT_EQ(temp_eval_state.mg_score, ctx->eval_state.mg_score);
        if (temp_eval_state.phase != ctx->eval_state.phase) {
            debug_print_board(ctx);
            //int asdf = 0;
        }
        ASSERT_EQ(temp_eval_state.phase, ctx->eval_state.phase);
        //ASSERT_TRUE(temp_eval_state.phase <= 24);
        //ASSERT_TRUE(ctx->eval_state.phase <= 24);

        DeltaEvalTest(ctx, depth-1);


        ASSERT_EQ(SCE_UnmakeMove(ctx), SCE_SUCCESS);
    }
}

TEST(SEF, Initial) {
    BOARD_SETUP(board, precomputation_table, zobrist_table);

    ASSERT_EQ(SCE_Eval_SimplifiedEvaluationFunction(&ctx), 0);
}

TEST(SEF, DeltaEval_Initial) {
    SCE_Context ctx;
    ASSERT_EQ(SCE_Context_init(&ctx), SCE_SUCCESS);

    // While engine generation is not needed, this is used for precomputing the first evaluation.
    SCE_Engine engine;
    ASSERT_EQ(SCE_Engine_init(&ctx, &engine, SCE_Eval_SimplifiedEvaluationFunction, SCE_DeltaEval_SimplifiedEvaluationFunction, DEBUG_TT_N_SIZE), SCE_SUCCESS);

    SCE_ChessMoveList movelist;
    ASSERT_EQ(SCE_ChessMoveList_clear(&movelist), SCE_SUCCESS);

    ASSERT_EQ(SCE_GenerateLegalMoves(&movelist, &ctx), SCE_SUCCESS);
    ASSERT_NE(movelist.count, 0);

    // For each move, see if the evaluations match.
    for (unsigned int i = 0; i < movelist.count; i++) {
        const SCE_ChessMove move = movelist.moves[i];

        SCE_EvalState temp_eval_state = ctx.eval_state;
        const int delta_evaluated = SCE_DeltaEval_SimplifiedEvaluationFunction(&ctx.board, &temp_eval_state, move);

        ASSERT_EQ(SCE_MakeMove(&ctx, move), SCE_SUCCESS);
        ctx.eval_state = temp_eval_state;
        const int full_evaluated = SCE_Eval_SimplifiedEvaluationFunction(&ctx);

        assert(delta_evaluated == full_evaluated);
        ASSERT_EQ(delta_evaluated, full_evaluated);

        ASSERT_EQ(SCE_UnmakeMove(&ctx), SCE_SUCCESS);
    }
}

TEST(SEF, DeltaEval_Kiwipete_Depth_2) {
    SCE_Context ctx;
    ASSERT_EQ(SCE_Context_init(&ctx), SCE_SUCCESS);
    ASSERT_EQ(SCE_Chessboard_FEN_setup(&ctx, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 0"), SCE_SUCCESS);

    // While engine generation is not needed, this is used for precomputing the first evaluation.
    SCE_Engine engine;
    ASSERT_EQ(SCE_Engine_init(&ctx, &engine, SCE_Eval_SimplifiedEvaluationFunction, SCE_DeltaEval_SimplifiedEvaluationFunction, DEBUG_TT_N_SIZE), SCE_SUCCESS);

    SCE_ChessMoveList movelist;
    ASSERT_EQ(SCE_ChessMoveList_clear(&movelist), SCE_SUCCESS);

    ASSERT_EQ(SCE_GenerateLegalMoves(&movelist, &ctx), SCE_SUCCESS);
    ASSERT_NE(movelist.count, 0);

    // For each move, see if the evaluations match.
    for (unsigned int i = 0; i < movelist.count; i++) {
        const SCE_ChessMove move = movelist.moves[i];

        SCE_EvalState temp_eval_state = ctx.eval_state;
        const int delta_evaluated = SCE_DeltaEval_SimplifiedEvaluationFunction(&ctx.board, &temp_eval_state, move);
        ASSERT_EQ(SCE_MakeMove(&ctx, move), SCE_SUCCESS);
        ctx.eval_state = temp_eval_state;
        {
            SCE_ChessMoveList movelist2;
            ASSERT_EQ(SCE_ChessMoveList_clear(&movelist2), SCE_SUCCESS);

            ASSERT_EQ(SCE_GenerateLegalMoves(&movelist2, &ctx), SCE_SUCCESS);
            ASSERT_NE(movelist2.count, 0);

            for (unsigned int j = 0; j < movelist2.count; j++) {
                const SCE_ChessMove move2 = movelist2.moves[j];

                SCE_EvalState temp_eval_state_2 = ctx.eval_state;
                const int delta_evaluated2 = SCE_DeltaEval_SimplifiedEvaluationFunction(&ctx.board, &temp_eval_state_2, move2);

                ASSERT_EQ(SCE_MakeMove(&ctx, move2), SCE_SUCCESS);
                const int full_evaluated2 = SCE_Eval_SimplifiedEvaluationFunction(&ctx);
                
                ASSERT_EQ(delta_evaluated2, full_evaluated2);
                ASSERT_EQ(SCE_UnmakeMove(&ctx), SCE_SUCCESS);
            }
            
        }
        const int full_evaluated = SCE_Eval_SimplifiedEvaluationFunction(&ctx);

        ASSERT_EQ(delta_evaluated, full_evaluated);

        ASSERT_EQ(SCE_UnmakeMove(&ctx), SCE_SUCCESS);
    }
}

TEST(SEF, DeltaEval_Kiwipete_Depth_3) {
    SCE_Context ctx;
    ASSERT_EQ(SCE_Context_init(&ctx), SCE_SUCCESS);
    ASSERT_EQ(SCE_Chessboard_FEN_setup(&ctx, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 0"), SCE_SUCCESS);

    // While engine generation is not needed, this is used for precomputing the first evaluation.
    SCE_Engine engine;
    ASSERT_EQ(SCE_Engine_init(&ctx, &engine, SCE_Eval_SimplifiedEvaluationFunction, SCE_DeltaEval_SimplifiedEvaluationFunction, DEBUG_TT_N_SIZE), SCE_SUCCESS);

    DeltaEvalTest(&ctx, 4);
}
