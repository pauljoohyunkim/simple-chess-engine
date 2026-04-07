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

// https://lichess.org/editor/1k5r/3p4/8/4P3/8/8/8/6K1_w_-_-_0_1?color=white
TEST(MakeMove, MakeMove_White_Queen_Promotion_EnPassant) {
    BOARD_CLEAR_SETUP(board);

    SCE_PieceMovementPrecomputationTable precpt_tbl;
    SCE_PieceMovementPrecompute(&precpt_tbl);

    ASSERT_EQ(place_piece_on_board(&board, "B8", B_KING), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "H8", B_ROOK), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "D7", B_PAWN), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "E5", W_PAWN), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "G1", W_KING), SCE_SUCCESS);
    board.to_move = BLACK;

    // Setting en-passant opportunity
    SCE_ChessMove move = (SCE_AN_To_Idx("D7") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("D5") SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_DOUBLE_PAWN_PUSH SCE_CHESSMOVE_SET_FLAG);
    ASSERT_EQ(SCE_MakeMove(&board, &precpt_tbl, move), SCE_SUCCESS);
    
    // En passant
    move = (SCE_AN_To_Idx("E5") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("D6") SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_EN_PASSANT_CAPTURE SCE_CHESSMOVE_SET_FLAG);
    ASSERT_EQ(SCE_MakeMove(&board, &precpt_tbl, move), SCE_SUCCESS);

    // Some blunder by rook
    move = (SCE_AN_To_Idx("H8") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("H3") SCE_CHESSMOVE_SET_DST);
    ASSERT_EQ(SCE_MakeMove(&board, &precpt_tbl, move), SCE_SUCCESS);

    // Single push by pawn
    move = (SCE_AN_To_Idx("D6") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("D7") SCE_CHESSMOVE_SET_DST);
    ASSERT_EQ(SCE_MakeMove(&board, &precpt_tbl, move), SCE_SUCCESS);

    // Another blunder by rook
    move = (SCE_AN_To_Idx("H3") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("H4") SCE_CHESSMOVE_SET_DST);
    ASSERT_EQ(SCE_MakeMove(&board, &precpt_tbl, move), SCE_SUCCESS);

    // Pawn promotion
    move = (SCE_AN_To_Idx("D7") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("D8") SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_QUEEN_PROMOTION SCE_CHESSMOVE_SET_FLAG);
    ASSERT_EQ(SCE_MakeMove(&board, &precpt_tbl, move), SCE_SUCCESS);

    ASSERT_TRUE(board.bitboards[W_QUEEN] & SCE_AN_To_Bitboard("D8"));
}
