#include <gtest/gtest.h>
#include "eval/sef.h"
#include "../setup.h"

TEST(SEF, Initial) {
    BOARD_SETUP(board, precomputation_table, zobrist_table);

    ASSERT_EQ(SCE_Eval_SimplifiedEvaluationFunction(&board), 0);
}