#include <gtest/gtest.h>
#include "uci.h"

TEST(UCI, Move_To_UCI_String) {
    char uci_str[6] = { 0 };

    ASSERT_TRUE(SCE_MoveToUCIString(EMPTY_MOVE, uci_str));
    ASSERT_STREQ(uci_str, "0000");

    SCE_ChessMove move = (SCE_AN_To_Idx("A1") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("H8") SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG);
    ASSERT_TRUE(SCE_MoveToUCIString(move, uci_str));
    ASSERT_STREQ(uci_str, "a1h8");

    move = (SCE_AN_To_Idx("A7") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("B8") SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_QUEEN_PROMO_CAPTURE SCE_CHESSMOVE_SET_FLAG);
    ASSERT_TRUE(SCE_MoveToUCIString(move, uci_str));
    ASSERT_STREQ(uci_str, "a7b8q");

    move = (SCE_AN_To_Idx("B7") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("B8") SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_KNIGHT_PROMO_CAPTURE SCE_CHESSMOVE_SET_FLAG);
    ASSERT_TRUE(SCE_MoveToUCIString(move, uci_str));
    ASSERT_STREQ(uci_str, "b7b8n");
}
