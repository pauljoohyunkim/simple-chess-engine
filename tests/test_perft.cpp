#include <gtest/gtest.h>
#include "../include/chess.h"
#include "../include/dev.h"
#include "setup.h"

TEST(PERFT, Initial_Depth_1_to_5) {
    const uint testvector[5U] = { 20, 400, 8902, 197281, 4865609 };
    for (uint depth = 1U; depth <= 5U; depth++) {
        BOARD_SETUP(board, precomputation_table)

        const uint count = perft_count(&board, &precomputation_table, depth);

        ASSERT_EQ(count, testvector[depth-1]);
    }

}


//TEST(PERFT, Kiwipete_Depth_1_to_5) {
//
//}
