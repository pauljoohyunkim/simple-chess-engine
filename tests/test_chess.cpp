#include <gtest/gtest.h>
#include "../include/chess.h"

#define BOARD_SETUP(board, precpt_tbl) \
    SCE_PieceMovementPrecomputationTable precpt_tbl; \
    SCE_PieceMovementPrecompute(&precpt_tbl); \
    SCE_Chessboard board; \
    SCE_Chessboard_reset(&board);

TEST(ChessBoard, AN_To_Bitboard_1) {
    //SCE_Chessboard board;
    //SCE_Chessboard_clear(&board);

    ASSERT_EQ(1ULL << 0U, SCE_AN_To_Bitboard("A1"));
    ASSERT_EQ(1ULL << 1U, SCE_AN_To_Bitboard("B1"));
    ASSERT_EQ(1ULL << 2U, SCE_AN_To_Bitboard("C1"));
    ASSERT_EQ(1ULL << 3U, SCE_AN_To_Bitboard("D1"));
    ASSERT_EQ(1ULL << 4U, SCE_AN_To_Bitboard("E1"));
    ASSERT_EQ(1ULL << 5U, SCE_AN_To_Bitboard("F1"));
    ASSERT_EQ(1ULL << 6U, SCE_AN_To_Bitboard("G1"));
    ASSERT_EQ(1ULL << 7U, SCE_AN_To_Bitboard("H1"));
    ASSERT_EQ(1ULL << 8U, SCE_AN_To_Bitboard("A2"));
    ASSERT_EQ(1ULL << 9U, SCE_AN_To_Bitboard("B2"));
    ASSERT_EQ(1ULL << 10U, SCE_AN_To_Bitboard("C2"));
    ASSERT_EQ(1ULL << 11U, SCE_AN_To_Bitboard("D2"));
    ASSERT_EQ(1ULL << 12U, SCE_AN_To_Bitboard("E2"));
    ASSERT_EQ(1ULL << 13U, SCE_AN_To_Bitboard("F2"));
    ASSERT_EQ(1ULL << 14U, SCE_AN_To_Bitboard("G2"));
    ASSERT_EQ(1ULL << 15U, SCE_AN_To_Bitboard("H2"));
    ASSERT_EQ(1ULL << 16U, SCE_AN_To_Bitboard("A3"));
    ASSERT_EQ(1ULL << 17U, SCE_AN_To_Bitboard("B3"));
    ASSERT_EQ(1ULL << 18U, SCE_AN_To_Bitboard("C3"));
    ASSERT_EQ(1ULL << 19U, SCE_AN_To_Bitboard("D3"));
    ASSERT_EQ(1ULL << 20U, SCE_AN_To_Bitboard("E3"));
    ASSERT_EQ(1ULL << 21U, SCE_AN_To_Bitboard("F3"));
    ASSERT_EQ(1ULL << 22U, SCE_AN_To_Bitboard("G3"));
    ASSERT_EQ(1ULL << 23U, SCE_AN_To_Bitboard("H3"));
    ASSERT_EQ(1ULL << 24U, SCE_AN_To_Bitboard("A4"));
    ASSERT_EQ(1ULL << 25U, SCE_AN_To_Bitboard("B4"));
    ASSERT_EQ(1ULL << 26U, SCE_AN_To_Bitboard("C4"));
    ASSERT_EQ(1ULL << 27U, SCE_AN_To_Bitboard("D4"));
    ASSERT_EQ(1ULL << 28U, SCE_AN_To_Bitboard("E4"));
    ASSERT_EQ(1ULL << 29U, SCE_AN_To_Bitboard("F4"));
    ASSERT_EQ(1ULL << 30U, SCE_AN_To_Bitboard("G4"));
    ASSERT_EQ(1ULL << 31U, SCE_AN_To_Bitboard("H4"));
    ASSERT_EQ(1ULL << 32U, SCE_AN_To_Bitboard("A5"));
    ASSERT_EQ(1ULL << 33U, SCE_AN_To_Bitboard("B5"));
    ASSERT_EQ(1ULL << 34U, SCE_AN_To_Bitboard("C5"));
    ASSERT_EQ(1ULL << 35U, SCE_AN_To_Bitboard("D5"));
    ASSERT_EQ(1ULL << 36U, SCE_AN_To_Bitboard("E5"));
    ASSERT_EQ(1ULL << 37U, SCE_AN_To_Bitboard("F5"));
    ASSERT_EQ(1ULL << 38U, SCE_AN_To_Bitboard("G5"));
    ASSERT_EQ(1ULL << 39U, SCE_AN_To_Bitboard("H5"));
    ASSERT_EQ(1ULL << 40U, SCE_AN_To_Bitboard("A6"));
    ASSERT_EQ(1ULL << 41U, SCE_AN_To_Bitboard("B6"));
    ASSERT_EQ(1ULL << 42U, SCE_AN_To_Bitboard("C6"));
    ASSERT_EQ(1ULL << 43U, SCE_AN_To_Bitboard("D6"));
    ASSERT_EQ(1ULL << 44U, SCE_AN_To_Bitboard("E6"));
    ASSERT_EQ(1ULL << 45U, SCE_AN_To_Bitboard("F6"));
    ASSERT_EQ(1ULL << 46U, SCE_AN_To_Bitboard("G6"));
    ASSERT_EQ(1ULL << 47U, SCE_AN_To_Bitboard("H6"));
    ASSERT_EQ(1ULL << 48U, SCE_AN_To_Bitboard("A7"));
    ASSERT_EQ(1ULL << 49U, SCE_AN_To_Bitboard("B7"));
    ASSERT_EQ(1ULL << 50U, SCE_AN_To_Bitboard("C7"));
    ASSERT_EQ(1ULL << 51U, SCE_AN_To_Bitboard("D7"));
    ASSERT_EQ(1ULL << 52U, SCE_AN_To_Bitboard("E7"));
    ASSERT_EQ(1ULL << 53U, SCE_AN_To_Bitboard("F7"));
    ASSERT_EQ(1ULL << 54U, SCE_AN_To_Bitboard("G7"));
    ASSERT_EQ(1ULL << 55U, SCE_AN_To_Bitboard("H7"));
    ASSERT_EQ(1ULL << 56U, SCE_AN_To_Bitboard("A8"));
    ASSERT_EQ(1ULL << 57U, SCE_AN_To_Bitboard("B8"));
    ASSERT_EQ(1ULL << 58U, SCE_AN_To_Bitboard("C8"));
    ASSERT_EQ(1ULL << 59U, SCE_AN_To_Bitboard("D8"));
    ASSERT_EQ(1ULL << 60U, SCE_AN_To_Bitboard("E8"));
    ASSERT_EQ(1ULL << 61U, SCE_AN_To_Bitboard("F8"));
    ASSERT_EQ(1ULL << 62U, SCE_AN_To_Bitboard("G8"));
    ASSERT_EQ(1ULL << 63U, SCE_AN_To_Bitboard("H8"));
    
}

TEST(ChessBoard, Check_Under_Attack_1) {
    BOARD_SETUP(board, precpt_tbl);
    ASSERT_TRUE(SCE_IsSqaureAttacked(&board, &precpt_tbl, 1, WHITE));
}
