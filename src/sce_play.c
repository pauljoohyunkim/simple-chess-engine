#include "chess.h"

int main() {
    SCE_PieceMovementPrecomputationTable precpt_tbl;
    SCE_PieceMovementPrecompute(&precpt_tbl);
    SCE_Chessboard board;
    SCE_Chessboard_reset(&board);
    SCE_Chessboard_print(&board, WHITE);
    SCE_Chessboard_print(&board, BLACK);

    return 0;
}
