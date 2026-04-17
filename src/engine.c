#include <assert.h>
#include "engine.h"

typedef unsigned int uint;

static int SCE_Engine_AlphaBetaNegamax(SCE_Engine *const ptr_engine,
                                       SCE_Chessboard *const ptr_board,
                                       SCE_PieceMovementPrecomputationTable *const ptr_precomputation_tbl,
                                       SCE_ZobristTable *const ptr_table,
                                       const unsigned int depth,
                                       int alpha,
                                       int beta);
static bool SCE_Engine_AddTransposition(SCE_Engine* const ptr_engine, const uint64_t zobrist_hash, const int score, const uint8_t depth, const SCE_ChessMove move, const uint8_t flag);
static SCE_TranspositionTableEntry* SCE_Engine_GetTransposition(SCE_Engine* const ptr_engine, const uint64_t zobrist_hash);

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

// Returns true if succeeded.
static bool SCE_Engine_AddTransposition(SCE_Engine* const ptr_engine, const uint64_t zobrist_hash, const int score, const uint8_t depth, const SCE_ChessMove move, const uint8_t flag) {
    if (ptr_engine == NULL || zobrist_hash == 0U) return false;

    // Hash the zobrist hash for hash map.
    // Modulo by table size (which is exponent of 2).
    const uint64_t key = zobrist_hash & (ptr_engine->transposition_table.table_size - 1U);
    
    // Check if entry exists at the spot.
    if (ptr_engine->transposition_table.entries[key].zobrist_hash == 0 || depth >= ptr_engine->transposition_table.entries[key].depth) {
        // Unassigned! Safe to add.
        ptr_engine->transposition_table.entries[key].zobrist_hash = zobrist_hash;
        ptr_engine->transposition_table.entries[key].score = score;
        ptr_engine->transposition_table.entries[key].move = move;
        ptr_engine->transposition_table.entries[key].depth = depth;
        ptr_engine->transposition_table.entries[key].flag = flag;
    } else {
        return false;
    }

    return true;
}

static SCE_TranspositionTableEntry* SCE_Engine_GetTransposition(SCE_Engine* const ptr_engine, const uint64_t zobrist_hash) {
    if (ptr_engine == NULL || zobrist_hash == 0U) return NULL;

    const uint64_t key = zobrist_hash & (ptr_engine->transposition_table.table_size - 1U);

    if (ptr_engine->transposition_table.entries[key].zobrist_hash) {
        return &ptr_engine->transposition_table.entries[key];
    } else {
        return NULL;
    }
}

static int SCE_Engine_AlphaBetaNegamax(SCE_Engine *const ptr_engine,
                                       SCE_Chessboard *const ptr_board,
                                       SCE_PieceMovementPrecomputationTable *const ptr_precomputation_tbl,
                                       SCE_ZobristTable *const ptr_table,
                                       const unsigned int depth,
                                       int alpha,
                                       int beta) {
    if (depth == 0) {
        return ptr_engine->eval_function(ptr_board);
    }

    // Zobrist-Transposition-Table Lookup
    const SCE_TranspositionTableEntry* const ptr_transposition_entry = SCE_Engine_GetTransposition(ptr_engine, ptr_board->zobrist_hash);

    if (ptr_transposition_entry && ptr_transposition_entry->zobrist_hash == ptr_board->zobrist_hash) {
        if (depth >= ptr_transposition_entry->depth) {
            // Useful result.
            switch (ptr_transposition_entry->flag) {
                case SCE_TF_EXACT:
                    return ptr_transposition_entry->score;
                case SCE_TF_ALPHA:
                    if (ptr_transposition_entry->score <= alpha) return alpha;
                    break;
                case SCE_TF_BETA:
                    if (ptr_transposition_entry->score >= beta) return beta;
                    break;
                default:
                    break;
            }
        }
    }

    const int alpha_original = alpha;

    // Move generation
    // TODO: Deal with best moves.
    int best_move = UNASSIGNED;

    SCE_ChessMoveList moves;
    SCE_Return ret;
    ret = SCE_ChessMoveList_clear(&moves);
    assert(ret == SCE_SUCCESS);
    ret = SCE_GenerateLegalMoves(&moves, ptr_board, ptr_precomputation_tbl, ptr_table);
    assert(ret == SCE_SUCCESS);

    // TODO: MVV-LVA Guessing and sorting

    // TODO: Check if king is in check or stalemate.
    // Iterating through moves
    for (uint i = 0U; i < moves.count; i++) {
        ret = SCE_MakeMove(ptr_board, ptr_precomputation_tbl, ptr_table, moves.moves[i]);
        assert(ret == SCE_SUCCESS);

        const int score = -SCE_Engine_AlphaBetaNegamax(ptr_engine, ptr_board, ptr_precomputation_tbl, ptr_table, depth-1, -beta, -alpha);

        ret = SCE_UnmakeMove(ptr_board, ptr_precomputation_tbl);
        assert(ret == SCE_SUCCESS);

        if (score >= beta) { 
            SCE_Engine_AddTransposition(ptr_engine, ptr_board->zobrist_hash, score, depth, moves.moves[i], SCE_TF_BETA);
            return beta;
        }
        if (score > alpha) { 
            alpha = score;
            best_move = moves.moves[i];
        }
    }

    const uint8_t flag = alpha <= alpha_original ? SCE_TF_ALPHA : SCE_TF_EXACT;
    SCE_Engine_AddTransposition(ptr_engine, ptr_board->zobrist_hash, alpha, depth, best_move == UNASSIGNED ? 0U : (SCE_ChessMove) best_move, flag);

    return alpha;
}

SCE_ChessMove SCE_Engine_AlphaBetaBestMove(SCE_Engine *const ptr_engine,
                                           SCE_Chessboard *const ptr_board,
                                           SCE_PieceMovementPrecomputationTable *const ptr_precomputation_tbl,
                                           SCE_ZobristTable *const ptr_table) {
    int alpha = SCE_ALPHA_INITIAL;
    int beta = SCE_BETA_INITIAL;
    int best_score = SCE_ALPHA_INITIAL;
    int best_move = UNASSIGNED;

    // Move generation
    SCE_ChessMoveList moves;
    SCE_Return ret;
    ret = SCE_ChessMoveList_clear(&moves);
    assert(ret == SCE_SUCCESS);
    ret = SCE_GenerateLegalMoves(&moves, ptr_board, ptr_precomputation_tbl, ptr_table);
    assert(ret == SCE_SUCCESS);

    for (uint i = 0U; i < moves.count; i++) {
        ret = SCE_MakeMove(ptr_board, ptr_precomputation_tbl, ptr_table, moves.moves[i]);
        assert(ret == SCE_SUCCESS);
        const int score = -SCE_Engine_AlphaBetaNegamax(ptr_engine, ptr_board, ptr_precomputation_tbl, ptr_table, ptr_engine->depth-1, -beta, -alpha);

        ret = SCE_UnmakeMove(ptr_board, ptr_precomputation_tbl);
        assert(ret == SCE_SUCCESS);

        if (score > best_score) {
            best_score = score;
            best_move = moves.moves[i];
        }

        if (score > alpha) {
            alpha = score;
        }
    }

    assert(best_move != UNASSIGNED);
    return (SCE_ChessMove) best_move;
}
