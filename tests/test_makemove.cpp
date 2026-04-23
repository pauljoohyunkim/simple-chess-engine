#include <gtest/gtest.h>
#include "chess.h"
#include "dev.h"
#include "fen.h"
#include "setup.h"

TEST(MakeMove, MakeMove_Endgame1_WhitePawn_Promote_To_Knight) {
    BOARD_CLEAR_SETUP(board);

    SCE_PieceMovementPrecompute(&ctx);

    SCE_ZobristTable_init(&ctx, NULL);

    ASSERT_EQ(place_piece_on_board(&board, "E1", W_KING), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "E8", B_KING), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "A7", W_PAWN), SCE_SUCCESS);

    const SCE_ChessMove move = (SCE_AN_To_Idx("A7") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("A8") SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_KNIGHT_PROMOTION SCE_CHESSMOVE_SET_FLAG);
    ASSERT_EQ(SCE_MakeMove(&ctx, move), SCE_SUCCESS);

    ASSERT_TRUE(board.bitboards[W_KNIGHT] & SCE_AN_To_Bitboard("A8"));
}

// https://lichess.org/editor/1k5r/3p4/8/4P3/8/8/8/6K1_w_-_-_0_1?color=white
TEST(MakeMove, MakeMove_White_Queen_Promotion_EnPassant) {
    BOARD_CLEAR_SETUP(board);

    SCE_PieceMovementPrecompute(&ctx);

    SCE_ZobristTable_init(&ctx, NULL);

    ASSERT_EQ(place_piece_on_board(&board, "B8", B_KING), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "H8", B_ROOK), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "D7", B_PAWN), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "E5", W_PAWN), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "G1", W_KING), SCE_SUCCESS);
    board.to_move = BLACK;

    // Setting en-passant opportunity
    SCE_ChessMove move = (SCE_AN_To_Idx("D7") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("D5") SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_DOUBLE_PAWN_PUSH SCE_CHESSMOVE_SET_FLAG);
    ASSERT_EQ(SCE_MakeMove(&ctx, move), SCE_SUCCESS);
    ASSERT_EQ(board.en_passant_idx, SCE_AN_To_Idx("D6"));
    
    // En passant
    move = (SCE_AN_To_Idx("E5") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("D6") SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_EN_PASSANT_CAPTURE SCE_CHESSMOVE_SET_FLAG);
    ASSERT_EQ(SCE_MakeMove(&ctx, move), SCE_SUCCESS);
    ASSERT_EQ(board.en_passant_idx, -1);

    // Some blunder by rook
    move = (SCE_AN_To_Idx("H8") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("H3") SCE_CHESSMOVE_SET_DST);
    ASSERT_EQ(SCE_MakeMove(&ctx, move), SCE_SUCCESS);

    // Single push by pawn
    move = (SCE_AN_To_Idx("D6") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("D7") SCE_CHESSMOVE_SET_DST);
    ASSERT_EQ(SCE_MakeMove(&ctx, move), SCE_SUCCESS);

    // Another blunder by rook
    move = (SCE_AN_To_Idx("H3") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("H4") SCE_CHESSMOVE_SET_DST);
    ASSERT_EQ(SCE_MakeMove(&ctx, move), SCE_SUCCESS);

    // Pawn promotion
    move = (SCE_AN_To_Idx("D7") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("D8") SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_QUEEN_PROMOTION SCE_CHESSMOVE_SET_FLAG);
    ASSERT_EQ(SCE_MakeMove(&ctx, move), SCE_SUCCESS);

    ASSERT_TRUE(board.bitboards[W_QUEEN] & SCE_AN_To_Bitboard("D8"));
}

TEST(MakeMove, White_Castling_Kingside_Black_Castling_Queenside) {
    BOARD_CLEAR_SETUP(board);

    SCE_PieceMovementPrecompute(&ctx);

    SCE_ZobristTable_init(&ctx, NULL);

    ASSERT_EQ(place_piece_on_board(&board, "E8", B_KING), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "H8", B_ROOK), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "A8", B_ROOK), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "E1", W_KING), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "H1", W_ROOK), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "A1", W_ROOK), SCE_SUCCESS);
    board.to_move = WHITE;

    debug_print_board(&ctx);

    // Castle king side.
    SCE_ChessMove move = (SCE_AN_To_Idx("E1") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("G1") SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_KING_CASTLE SCE_CHESSMOVE_SET_FLAG);
    ASSERT_EQ(SCE_MakeMove(&ctx, move), SCE_SUCCESS);
    ASSERT_FALSE(board.castling_rights & SCE_CASTLING_RIGHTS_WK);
    ASSERT_FALSE(board.castling_rights & SCE_CASTLING_RIGHTS_WQ);
    ASSERT_TRUE(board.castling_rights & SCE_CASTLING_RIGHTS_BK);
    ASSERT_TRUE(board.castling_rights & SCE_CASTLING_RIGHTS_BQ);

    debug_print_board(&ctx);

    move = (SCE_AN_To_Idx("E8") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("C8") SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_QUEEN_CASTLE SCE_CHESSMOVE_SET_FLAG);
    ASSERT_EQ(SCE_MakeMove(&ctx, move), SCE_SUCCESS);
    ASSERT_FALSE(board.castling_rights & SCE_CASTLING_RIGHTS_WK);
    ASSERT_FALSE(board.castling_rights & SCE_CASTLING_RIGHTS_WQ);
    ASSERT_FALSE(board.castling_rights & SCE_CASTLING_RIGHTS_BK);
    ASSERT_FALSE(board.castling_rights & SCE_CASTLING_RIGHTS_BQ);

    ASSERT_TRUE(board.bitboards[W_KING] & SCE_AN_To_Bitboard("G1"));
    ASSERT_TRUE(board.bitboards[W_ROOK] & SCE_AN_To_Bitboard("F1"));
    ASSERT_TRUE(board.bitboards[B_KING] & SCE_AN_To_Bitboard("C8"));
    ASSERT_TRUE(board.bitboards[B_ROOK] & SCE_AN_To_Bitboard("D8"));
}

TEST(MakeMove, Black_Castling_Kingside_White_Castling_Queenside) {
    BOARD_CLEAR_SETUP(board);

    SCE_PieceMovementPrecompute(&ctx);

    SCE_ZobristTable_init(&ctx, NULL);

    ASSERT_EQ(place_piece_on_board(&board, "E8", B_KING), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "H8", B_ROOK), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "A8", B_ROOK), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "E1", W_KING), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "H1", W_ROOK), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "A1", W_ROOK), SCE_SUCCESS);
    board.to_move = WHITE;

    debug_print_board(&ctx);

    // Castle queen side.
    SCE_ChessMove move = (SCE_AN_To_Idx("E1") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("C1") SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_QUEEN_CASTLE SCE_CHESSMOVE_SET_FLAG);
    ASSERT_EQ(SCE_MakeMove(&ctx, move), SCE_SUCCESS);
    ASSERT_FALSE(board.castling_rights & SCE_CASTLING_RIGHTS_WK);
    ASSERT_FALSE(board.castling_rights & SCE_CASTLING_RIGHTS_WQ);
    ASSERT_TRUE(board.castling_rights & SCE_CASTLING_RIGHTS_BK);
    ASSERT_TRUE(board.castling_rights & SCE_CASTLING_RIGHTS_BQ);

    move = (SCE_AN_To_Idx("E8") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("G8") SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_KING_CASTLE SCE_CHESSMOVE_SET_FLAG);
    ASSERT_EQ(SCE_MakeMove(&ctx, move), SCE_SUCCESS);
    ASSERT_FALSE(board.castling_rights & SCE_CASTLING_RIGHTS_WK);
    ASSERT_FALSE(board.castling_rights & SCE_CASTLING_RIGHTS_WQ);
    ASSERT_FALSE(board.castling_rights & SCE_CASTLING_RIGHTS_BK);
    ASSERT_FALSE(board.castling_rights & SCE_CASTLING_RIGHTS_BQ);

    ASSERT_TRUE(board.bitboards[W_KING] & SCE_AN_To_Bitboard("C1"));
    ASSERT_TRUE(board.bitboards[W_ROOK] & SCE_AN_To_Bitboard("D1"));
    ASSERT_TRUE(board.bitboards[B_KING] & SCE_AN_To_Bitboard("G8"));
    ASSERT_TRUE(board.bitboards[B_ROOK] & SCE_AN_To_Bitboard("F8"));
}

TEST(MakeMove, FoolsMate) {
    BOARD_SETUP(board, precpt_tbl, zobrist_table)

    // W: F2 -> F3
    SCE_ChessMove move = (SCE_AN_To_Idx("F2") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("F3") SCE_CHESSMOVE_SET_DST);
    ASSERT_EQ(SCE_MakeMove(&ctx, move), SCE_SUCCESS);

    // B: E7 -> E5
    move = (SCE_AN_To_Idx("E7") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("E5") SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_DOUBLE_PAWN_PUSH SCE_CHESSMOVE_SET_FLAG);
    ASSERT_EQ(SCE_MakeMove(&ctx, move), SCE_SUCCESS);

    // W: G2 -> G4
    move = (SCE_AN_To_Idx("G2") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("G4") SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_DOUBLE_PAWN_PUSH SCE_CHESSMOVE_SET_FLAG);
    ASSERT_EQ(SCE_MakeMove(&ctx, move), SCE_SUCCESS);

    // Mate by B: D8 -> E4
    move = (SCE_AN_To_Idx("D8") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("H4") SCE_CHESSMOVE_SET_DST);
    ASSERT_EQ(SCE_MakeMove(&ctx, move), SCE_SUCCESS);

    // White king is under attack
    ASSERT_TRUE(SCE_IsSquareAttacked(&ctx, SCE_AN_To_Bitboard("E1"), BLACK));

    // No escape square for white king
    ASSERT_TRUE(SCE_IsSquareAttacked(&ctx, SCE_AN_To_Bitboard("F2"), BLACK));
}

TEST(MakeMove, EnPassant_DiscoveredCheck) {
    BOARD_CLEAR_SETUP(board);

    SCE_PieceMovementPrecompute(&ctx);

    SCE_ZobristTable_init(&ctx, NULL);

    ASSERT_EQ(place_piece_on_board(&board, "E1", W_KING), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "D8", B_KING), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "D1", W_ROOK), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "D4", B_PAWN), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "C2", W_PAWN), SCE_SUCCESS);
    board.to_move = WHITE;

    // Double push by white pawn
    SCE_ChessMove move = (SCE_AN_To_Idx("C2") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("C4") SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_DOUBLE_PAWN_PUSH SCE_CHESSMOVE_SET_FLAG);
    ASSERT_EQ(SCE_MakeMove(&ctx, move), SCE_SUCCESS);

    debug_print_board(&ctx);

    // En passant
    move = (SCE_AN_To_Idx("D4") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("C3") SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_EN_PASSANT_CAPTURE SCE_CHESSMOVE_SET_FLAG);
    ASSERT_EQ(SCE_MakeMove(&ctx, move), SCE_INVALID_MOVE);

    debug_print_board(&ctx);
}

TEST(MakeMove, Castle_Through_Check) {
    BOARD_CLEAR_SETUP(board);

    SCE_PieceMovementPrecompute(&ctx);

    SCE_ZobristTable_init(&ctx, NULL);

    ASSERT_EQ(place_piece_on_board(&board, "E1", W_KING), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "H1", W_ROOK), SCE_SUCCESS);
    ASSERT_EQ(place_piece_on_board(&board, "G2", B_BISHOP), SCE_SUCCESS);
    board.to_move = WHITE;

    debug_print_board(&ctx);

    // Double push by white pawn
    SCE_ChessMove move = (SCE_AN_To_Idx("E1") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("G1") SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_KING_CASTLE SCE_CHESSMOVE_SET_FLAG);
    //ASSERT_EQ(SCE_MakeMove(&board, &precpt_tbl, move), SCE_INVALID_MOVE);
    ASSERT_EQ(SCE_MakeMove(&ctx, move), SCE_INVALID_MOVE);

    debug_print_board(&ctx);
}

TEST(MakeMove, MakeUnmake_PromoCapture) {
    SCE_Context ctx;
    SCE_Context_init(&ctx);

    SCE_Chessboard_FEN_setup(&ctx, "r3k3/1P6/8/8/8/8/8/1K6 w q - 0 1");

    //SCE_ChessMove move = (SCE_AN_To_Idx("B7") SCE_CHESSMOVE_SET_SRC | SCE_AN_To_Idx("A8") SCE_CHESSMOVE_SET_DST | (SCE_CHESSMOVE_FLAG_QUEEN_PROMO_CAPTURE SCE_CHESSMOVE_SET_FLAG));
    SCE_ChessMove move = (SCE_AN_To_Idx("B7") SCE_CHESSMOVE_SET_SRC | SCE_AN_To_Idx("B8") SCE_CHESSMOVE_SET_DST | (SCE_CHESSMOVE_FLAG_QUEEN_PROMOTION SCE_CHESSMOVE_SET_FLAG));
    ASSERT_EQ(SCE_MakeMove(&ctx, move), SCE_SUCCESS);
    debug_print_board(&ctx);

    ASSERT_EQ(SCE_UnmakeMove(&ctx), SCE_SUCCESS);
    debug_print_board(&ctx);
}
