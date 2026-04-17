#include <assert.h>
#include "engine.h"

typedef unsigned int uint;

SCE_Return SCE_Engine_init(SCE_Engine* const ptr_engine, const SCE_Eval eval_func, const unsigned int transposition_table_log2_size) {
    if (ptr_engine == NULL || eval_func == NULL || transposition_table_log2_size == 0) return SCE_INVALID_PARAM;

    const size_t n_entries = 1ULL << transposition_table_log2_size;

    ptr_engine->transposition_table.entries = (SCE_TranspositionTableEntry*) calloc(n_entries, sizeof(SCE_TranspositionTableEntry));
    if (ptr_engine->transposition_table.entries == NULL) return SCE_INTERNAL_ERROR;
    ptr_engine->transposition_table.table_size = n_entries;

    ptr_engine->eval_function = eval_func;

    return SCE_SUCCESS;
}

SCE_Return SCE_Engine_release(SCE_Engine* const ptr_engine) {
    if (ptr_engine == NULL) return SCE_INVALID_PARAM;

    free(ptr_engine->transposition_table.entries);
    ptr_engine->transposition_table.entries = NULL;
    ptr_engine->transposition_table.table_size = 0;
    ptr_engine->eval_function = NULL;

    return SCE_SUCCESS;
}

int SCE_Engine_AlphaBetaNegamax(SCE_Engine *const ptr_engine,
                                SCE_Chessboard *const ptr_board,
                                SCE_PieceMovementPrecomputationTable *const ptr_precomputation_tbl,
                                SCE_ZobristTable *const ptr_table,
                                const unsigned int depth,
                                const int alpha,
                                const int beta) {
    if (depth == 0) {
        return ptr_engine->eval_function(ptr_board);
    }

    // TODO: Zobrist-Transposition-Table Lookup

    // Move generation
    SCE_ChessMoveList moves;
    SCE_Return ret;
    ret = SCE_ChessMoveList_clear(&moves);
    assert(ret == SCE_SUCCESS);
    ret = SCE_GenerateLegalMoves(&moves, ptr_board, ptr_precomputation_tbl, ptr_table);
    assert(ret == SCE_SUCCESS);

    return 0;
}
