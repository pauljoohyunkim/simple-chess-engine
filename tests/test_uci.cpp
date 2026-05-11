#include <gtest/gtest.h>
#include "uci.h"
#include "setup.h"

TEST(UCI, Move_To_UCI_String) {
    char uci_str[6] = { 0 };

    ASSERT_TRUE(SCE_MoveToUCIString(EMPTY_MOVE, uci_str));
    ASSERT_STREQ(uci_str, "0000");

    SCE_ChessMove move = CREATE_MOVE("A1", "H8", SCE_CHESSMOVE_FLAG_CAPTURE);
    ASSERT_TRUE(SCE_MoveToUCIString(move, uci_str));
    ASSERT_STREQ(uci_str, "a1h8");

    move = CREATE_MOVE("A7", "B8", SCE_CHESSMOVE_FLAG_QUEEN_PROMO_CAPTURE);
    ASSERT_TRUE(SCE_MoveToUCIString(move, uci_str));
    ASSERT_STREQ(uci_str, "a7b8q");

    move = CREATE_MOVE("B7", "B8", SCE_CHESSMOVE_FLAG_KNIGHT_PROMO_CAPTURE);
    ASSERT_TRUE(SCE_MoveToUCIString(move, uci_str));
    ASSERT_STREQ(uci_str, "b7b8n");
}

TEST(UCI, UCI_String_To_Move) {
    SCE_ChessMove move = CREATE_MOVE("A2", "H2", SCE_CHESSMOVE_FLAG_QUIET_MOVE);
    ASSERT_EQ(SCE_UCIStringToMove("a2h2"), move);

    move = CREATE_MOVE("B2", "A1", SCE_CHESSMOVE_FLAG_ROOK_PROMOTION);
    ASSERT_EQ(SCE_UCIStringToMove("b2a1r"), move);
}
