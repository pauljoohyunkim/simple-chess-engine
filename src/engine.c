#include "engine.h"

int SCE_Engine_EvaluateBoard(const SCE_Engine* const ptr_engine, const SCE_Chessboard* const ptr_board) {
    const int centipawn = ptr_engine->eval_function(ptr_board);

    return centipawn;
}
