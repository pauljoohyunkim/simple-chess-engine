#include <gtest/gtest.h>
#include "engine.h"
#include "eval/sef.h"
#include "setup.h"

#define DEBUG_TT_N_SIZE (24U)

TEST(Engine_SEF, AlphaBetaBestMove) {
    BOARD_SETUP(board, precomputation_table, zobrist_table);

    SCE_Engine engine;
    ASSERT_EQ(SCE_Engine_init(&ctx, &engine, SCE_Eval_SimplifiedEvaluationFunction, SCE_DeltaEval_SimplifiedEvaluationFunction, DEBUG_TT_N_SIZE), SCE_SUCCESS);

    engine.depth = 8;

    SCE_ChessMove move = SCE_Engine_AlphaBetaBestMove(&engine, &ctx);

    ASSERT_NE(move, EMPTY_MOVE);
    ASSERT_EQ(SCE_Engine_release(&engine), SCE_SUCCESS);
}

TEST(Engine_SEF, IterativeDeepeningBestMove) {
    BOARD_SETUP(board, precomputation_table, zobrist_table);

    SCE_Engine engine;
    ASSERT_EQ(SCE_Engine_init(&ctx, &engine, SCE_Eval_SimplifiedEvaluationFunction, SCE_DeltaEval_SimplifiedEvaluationFunction, DEBUG_TT_N_SIZE), SCE_SUCCESS);

    engine.depth = 8;

    SCE_ChessMove move = SCE_Engine_IterativeDeepeningAlphaBetaBestMove(&engine, &ctx);

    ASSERT_NE(move, EMPTY_MOVE);
    ASSERT_EQ(SCE_Engine_release(&engine), SCE_SUCCESS);
}
