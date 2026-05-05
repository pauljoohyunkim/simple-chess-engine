#include <gtest/gtest.h>
#include "chess.h"
#include "engine.h"
#include "eval/hcef.h"
#include "fen.h"

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