#include "chess.h"

int main() {
    SCE_Chessboard board;
    SCE_Chessboard_reset(&board);
    SCE_Chessboard_print(&board, WHITE);
    SCE_Chessboard_print(&board, BLACK);

    return 0;
}
