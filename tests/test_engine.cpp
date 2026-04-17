#include <gtest/gtest.h>
#include "engine.h"
#include "eval/sef.h"
#include "setup.h"

#define DEBUG_TT_N_SIZE (24U)

TEST(Engine_SEF, Search) {
    BOARD_SETUP(board, precomputation_table, zobrist_table);

    SCE_Engine engine;
    ASSERT_EQ(SCE_Engine_init(&engine, SCE_Eval_SimplifiedEvaluationFunction, DEBUG_TT_N_SIZE), SCE_SUCCESS);

    SCE_Engine_AlphaBetaNegamax(&engine, &board, &precomputation_table, &zobrist_table, 1, SCE_ALPHA_INITIAL, SCE_BETA_INITIAL);

    ASSERT_EQ(SCE_Engine_release(&engine), SCE_SUCCESS);
}
