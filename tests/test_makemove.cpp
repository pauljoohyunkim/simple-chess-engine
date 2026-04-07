#include <gtest/gtest.h>
#include "../include/chess.h"
#include "../include/dev.h"
#include "setup.h"

TEST(MakeMove, MakeMove_Endgame1_WhitePawn_Promote_To_Knight) {
    BOARD_CLEAR_SETUP(board);

    SCE_PieceMovementPrecomputationTable precpt_tbl;
    SCE_PieceMovementPrecompute(&precpt_tbl);

    ASSERT_EQ(place_piece_on_board(&board, "E1", W_KING), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "E8", B_KING), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "A7", W_PAWN), SCE_SUCCESS);

    const SCE_ChessMove move = (SCE_AN_To_Idx("A7") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("A8") SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_KNIGHT_PROMOTION SCE_CHESSMOVE_SET_FLAG);
    ASSERT_EQ(SCE_MakeMove(&board, &precpt_tbl, move), SCE_SUCCESS);

    ASSERT_TRUE(board.bitboards[W_KNIGHT] & SCE_AN_To_Bitboard("A8"));
}
