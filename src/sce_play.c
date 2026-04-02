#include <stdlib.h>
#include "chess.h"

int main() {
    SCE_PieceMovementPrecomputationTable precpt_tbl;
    SCE_PieceMovementPrecompute(&precpt_tbl);
    SCE_Chessboard board;
    SCE_Chessboard_reset(&board);
    SCE_Chessboard_print(&board, WHITE);
    SCE_Chessboard_print(&board, BLACK);
    //SCE_IsSqaureAttacked(NULL, NULL, 0, WHITE);
    SCE_IsSqaureAttacked(&board, &precpt_tbl, 5, WHITE);

    return 0;
}
