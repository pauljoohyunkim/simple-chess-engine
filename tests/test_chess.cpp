#include <gtest/gtest.h>
#include "../include/chess.h"
#include "../include/dev.h"

#define BOARD_CLEAR_SETUP(board) \
    SCE_Chessboard board; \
    SCE_Chessboard_clear(&board);


#define BOARD_SETUP(board, precpt_tbl) \
    SCE_PieceMovementPrecomputationTable precpt_tbl; \
    SCE_PieceMovementPrecompute(&precpt_tbl); \
    SCE_Chessboard board; \
    SCE_Chessboard_reset(&board);

TEST(ChessBoard, AN_To_Bitboard) {
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

TEST(ChessBoard, Bitboard_To_AN) {
    char an[3U] = { 0 };
    
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 0U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "A1", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 1U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "B1", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 2U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "C1", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 3U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "D1", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 4U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "E1", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 5U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "F1", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 6U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "G1", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 7U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "H1", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 8U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "A2", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 9U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "B2", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 10U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "C2", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 11U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "D2", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 12U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "E2", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 13U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "F2", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 14U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "G2", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 15U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "H2", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 16U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "A3", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 17U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "B3", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 18U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "C3", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 19U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "D3", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 20U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "E3", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 21U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "F3", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 22U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "G3", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 23U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "H3", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 24U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "A4", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 25U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "B4", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 26U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "C4", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 27U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "D4", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 28U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "E4", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 29U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "F4", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 30U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "G4", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 31U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "H4", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 32U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "A5", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 33U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "B5", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 34U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "C5", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 35U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "D5", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 36U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "E5", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 37U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "F5", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 38U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "G5", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 39U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "H5", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 40U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "A6", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 41U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "B6", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 42U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "C6", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 43U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "D6", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 44U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "E6", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 45U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "F6", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 46U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "G6", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 47U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "H6", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 48U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "A7", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 49U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "B7", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 50U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "C7", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 51U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "D7", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 52U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "E7", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 53U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "F7", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 54U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "G7", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 55U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "H7", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 56U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "A8", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 57U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "B8", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 58U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "C8", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 59U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "D8", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 60U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "E8", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 61U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "F8", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 62U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "G8", 2U), 0);
    ASSERT_EQ(SCE_Bitboard_To_AN(an, 1ULL << 63U), SCE_SUCCESS);
    ASSERT_EQ(memcmp(an, "H8", 2U), 0);
}

TEST(ChessBoard, Initial_Setup) {
    BOARD_SETUP(board, precpt_tbl);

    // White pieces
    ASSERT_TRUE(board.bitboards[W_ROOK] & SCE_AN_To_Bitboard("A1"));
    ASSERT_TRUE(board.bitboards[W_KNIGHT] & SCE_AN_To_Bitboard("B1"));
    ASSERT_TRUE(board.bitboards[W_BISHOP] & SCE_AN_To_Bitboard("C1"));
    ASSERT_TRUE(board.bitboards[W_QUEEN] & SCE_AN_To_Bitboard("D1"));
    ASSERT_TRUE(board.bitboards[W_KING] & SCE_AN_To_Bitboard("E1"));
    ASSERT_TRUE(board.bitboards[W_BISHOP] & SCE_AN_To_Bitboard("F1"));
    ASSERT_TRUE(board.bitboards[W_KNIGHT] & SCE_AN_To_Bitboard("G1"));
    ASSERT_TRUE(board.bitboards[W_ROOK] & SCE_AN_To_Bitboard("H1"));

    ASSERT_TRUE(board.bitboards[W_PAWN] & SCE_AN_To_Bitboard("A2"));
    ASSERT_TRUE(board.bitboards[W_PAWN] & SCE_AN_To_Bitboard("B2"));
    ASSERT_TRUE(board.bitboards[W_PAWN] & SCE_AN_To_Bitboard("C2"));
    ASSERT_TRUE(board.bitboards[W_PAWN] & SCE_AN_To_Bitboard("D2"));
    ASSERT_TRUE(board.bitboards[W_PAWN] & SCE_AN_To_Bitboard("E2"));
    ASSERT_TRUE(board.bitboards[W_PAWN] & SCE_AN_To_Bitboard("F2"));
    ASSERT_TRUE(board.bitboards[W_PAWN] & SCE_AN_To_Bitboard("G2"));
    ASSERT_TRUE(board.bitboards[W_PAWN] & SCE_AN_To_Bitboard("H2"));

    // Black pieces
    ASSERT_TRUE(board.bitboards[B_ROOK] & SCE_AN_To_Bitboard("A8"));
    ASSERT_TRUE(board.bitboards[B_KNIGHT] & SCE_AN_To_Bitboard("B8"));
    ASSERT_TRUE(board.bitboards[B_BISHOP] & SCE_AN_To_Bitboard("C8"));
    ASSERT_TRUE(board.bitboards[B_QUEEN] & SCE_AN_To_Bitboard("D8"));
    ASSERT_TRUE(board.bitboards[B_KING] & SCE_AN_To_Bitboard("E8"));
    ASSERT_TRUE(board.bitboards[B_BISHOP] & SCE_AN_To_Bitboard("F8"));
    ASSERT_TRUE(board.bitboards[B_KNIGHT] & SCE_AN_To_Bitboard("G8"));
    ASSERT_TRUE(board.bitboards[B_ROOK] & SCE_AN_To_Bitboard("H8"));

    ASSERT_TRUE(board.bitboards[B_PAWN] & SCE_AN_To_Bitboard("A7"));
    ASSERT_TRUE(board.bitboards[B_PAWN] & SCE_AN_To_Bitboard("B7"));
    ASSERT_TRUE(board.bitboards[B_PAWN] & SCE_AN_To_Bitboard("C7"));
    ASSERT_TRUE(board.bitboards[B_PAWN] & SCE_AN_To_Bitboard("D7"));
    ASSERT_TRUE(board.bitboards[B_PAWN] & SCE_AN_To_Bitboard("E7"));
    ASSERT_TRUE(board.bitboards[B_PAWN] & SCE_AN_To_Bitboard("F7"));
    ASSERT_TRUE(board.bitboards[B_PAWN] & SCE_AN_To_Bitboard("G7"));
    ASSERT_TRUE(board.bitboards[B_PAWN] & SCE_AN_To_Bitboard("H7"));

    // Between A2 to H6, there should be nothing.

    uint64_t empty = 0U;
    for (uint i = 8*2; i < 8*6; i++) {
        empty ^= (1ULL << i);
    }

    ASSERT_FALSE(SCE_Chessboard_Occupancy(&board) & empty);
}

TEST(ChessBoard, Square_Under_Attack_1) {
    BOARD_CLEAR_SETUP(board);
    SCE_PieceMovementPrecomputationTable precpt_tbl;
    SCE_PieceMovementPrecompute(&precpt_tbl);

    // Place a knight on A1
    ASSERT_EQ(place_piece_on_board(&board, "A1", W_KNIGHT), SCE_SUCCESS);
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("B3"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("C2"), WHITE));

    SCE_Chessboard_print(&board, WHITE);
}

TEST(ChessBoard, Square_Under_Attack_2) {
    BOARD_CLEAR_SETUP(board);
    SCE_PieceMovementPrecomputationTable precpt_tbl;
    SCE_PieceMovementPrecompute(&precpt_tbl);

    // Place a white bishop at C5
    ASSERT_EQ(place_piece_on_board(&board, "C5", W_BISHOP), SCE_SUCCESS);
    // Place a black rook at E7
    ASSERT_EQ(place_piece_on_board(&board, "E7", B_ROOK), SCE_SUCCESS);
    
    // Attacked by white bishop: B4, A3, D6, E7, A7, B6, D4, E3, F2, G1
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("B4"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("A3"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("D6"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("E7"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("A7"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("B6"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("D4"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("E3"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("F2"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("G1"), WHITE));

    // Attacked by black rook: E1, E2, E3, E4, E5, E6, E8, A7, B7, C7, D7, F7, G7, H7
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("E1"), BLACK));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("E2"), BLACK));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("E3"), BLACK));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("E4"), BLACK));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("E5"), BLACK));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("E6"), BLACK));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("E8"), BLACK));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("A7"), BLACK));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("B7"), BLACK));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("C7"), BLACK));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("D7"), BLACK));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("F7"), BLACK));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("G7"), BLACK));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("H7"), BLACK));

    SCE_Chessboard_print(&board, WHITE);
}

TEST(ChessBoard, Square_Under_Attack_3) {
    BOARD_CLEAR_SETUP(board);
    SCE_PieceMovementPrecomputationTable precpt_tbl;
    SCE_PieceMovementPrecompute(&precpt_tbl);

    //   A B C D E F G H
    // 8 R N B Q K B - R
    // 7 P P - - P P P -
    // 6 - - P - - N - P
    // 5 - - - P - - - -
    // 4 - - - - P - - -
    // 3 - - - B - - - P
    // 2 P P P P Q P P -
    // 1 R N B - K - N R
    // From rank 1 to 4, there are only white pieces.
    // From rank 5 to 8, there are only black pieces.
    // 1. e2-e4 h7-h6 2. f1-d3 c7-c6 3. h2-h3 g8-f6 4. d1-e2 d7-d5 

    // Rank 1
    ASSERT_EQ(place_piece_on_board(&board, "A1", W_ROOK), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "B1", W_KNIGHT), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "C1", W_BISHOP), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "E1", W_KING), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "G1", W_KNIGHT), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "H1", W_ROOK), SCE_SUCCESS);

    // Rank 2
    ASSERT_EQ(place_piece_on_board(&board, "A2", W_PAWN), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "B2", W_PAWN), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "C2", W_PAWN), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "D2", W_PAWN), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "E2", W_QUEEN), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "F2", W_PAWN), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "G2", W_PAWN), SCE_SUCCESS);

    // Rank 3
    ASSERT_EQ(place_piece_on_board(&board, "D3", W_BISHOP), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "H3", W_PAWN), SCE_SUCCESS);

    // Rank 4
    ASSERT_EQ(place_piece_on_board(&board, "E4", W_PAWN), SCE_SUCCESS);

    // Rank 5
    ASSERT_EQ(place_piece_on_board(&board, "D5", B_PAWN), SCE_SUCCESS);

    // Rank 6
    ASSERT_EQ(place_piece_on_board(&board, "C6", B_PAWN), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "F6", B_KNIGHT), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "H6", B_PAWN), SCE_SUCCESS);

    // Rank 7
    ASSERT_EQ(place_piece_on_board(&board, "A7", B_PAWN), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "B7", B_PAWN), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "E7", B_PAWN), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "F7", B_PAWN), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "G7", B_PAWN), SCE_SUCCESS);

    // Rank 8
    ASSERT_EQ(place_piece_on_board(&board, "A8", B_ROOK), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "B8", B_KNIGHT), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "C8", B_BISHOP), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "D8", B_QUEEN), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "E8", B_KING), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "F8", B_BISHOP), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "H8", B_ROOK), SCE_SUCCESS);

    // White attacks
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("D5"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("F5"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("A6"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("B5"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("C4"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("E4"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("A3"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("B3"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("C3"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("D3"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("E3"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("F3"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("G3"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("H3"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("A2"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("B1"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("B2"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("D2"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("D1"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("E2"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("F2"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("E1"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("F1"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("F3"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("H3"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("G1"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("H2"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("C2"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("G4"), WHITE));
    ASSERT_TRUE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("H5"), WHITE));

    // White does not attack
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("A1"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("C1"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("H1"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("G2"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("A4"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("B4"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("D4"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("F4"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("H4"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("A5"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("C5"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("E5"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("G5"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("B6"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("C6"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("D6"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("E6"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("F6"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("G6"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("H6"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("A7"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("B7"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("C7"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("D7"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("E7"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("F7"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("G7"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("H7"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("A8"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("B8"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("C8"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("D8"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("E8"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("F8"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("G8"), WHITE));
    ASSERT_FALSE(SCE_IsSquareAttacked(&board, &precpt_tbl, SCE_AN_To_Bitboard("H8"), WHITE));


    SCE_Chessboard_print(&board, WHITE);
}

TEST(MoveGeneration, Pseudomove_1) {
    BOARD_SETUP(board, precpt_tbl)

    SCE_ChessMoveList list;
    list.count = 0;

    ASSERT_EQ(SCE_GenerateLegalMoves(&list, &board, &precpt_tbl), SCE_SUCCESS);
    for (unsigned int i = 0; i < list.count; i++) {
        print_move_to_AN(list.moves[i]);
    }
    ASSERT_EQ(list.count, 8);
}
