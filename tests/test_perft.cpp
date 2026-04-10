#include <gtest/gtest.h>
#include "../include/chess.h"
#include "../include/dev.h"
#include "setup.h"

TEST(PERFT, Initial_Depth_1) {
    BOARD_SETUP(board, precomputation_table)

    const uint count = perft_count(&board, &precomputation_table, 1U);

    ASSERT_EQ(count, 20);
}

TEST(PERFT, Initial_Depth_2) {
    BOARD_SETUP(board, precomputation_table)

    const uint count = perft_count(&board, &precomputation_table, 2U);

    ASSERT_EQ(count, 400);
}

TEST(PERFT, Initial_Depth_3) {
    BOARD_SETUP(board, precomputation_table)

    const uint count = perft_count(&board, &precomputation_table, 3U);

    ASSERT_EQ(count, 8902);
}

TEST(PERFT, Initial_Depth_4) {
    BOARD_SETUP(board, precomputation_table)

    const uint count = perft_count(&board, &precomputation_table, 4U);

    ASSERT_EQ(count, 197281);
}

TEST(PERFT, Initial_Depth_5) {
    BOARD_SETUP(board, precomputation_table)

    const uint count = perft_count(&board, &precomputation_table, 5U);

    ASSERT_EQ(count, 4865609);
}
