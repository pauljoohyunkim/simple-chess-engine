#include <assert.h>
#include "eval/pst.h"
#include "engine.h"

typedef unsigned int uint;

static bool SCE_Engine_AddTransposition(SCE_Engine* const ptr_engine, const uint64_t zobrist_hash, const int score, const uint8_t depth, const SCE_ChessMove move, const uint8_t flag);
static SCE_TranspositionTableEntry* SCE_Engine_GetTransposition(SCE_Engine* const ptr_engine, const uint64_t zobrist_hash);
static SCE_Return SCE_Engine_OrderMove_MVVLVA(SCE_ChessMoveList* const ptr_movelist, const SCE_Chessboard* const ptr_board, const int tt_hint_move);
static int SCE_Engine_AlphaBetaNegamax(SCE_Engine *const ptr_engine,
                                       SCE_Chessboard *const ptr_board,
                                       SCE_PieceMovementPrecomputationTable *const ptr_precomputation_tbl,
                                       SCE_ZobristTable *const ptr_table,
                                       const unsigned int depth,
                                       int alpha,
                                       int beta);

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

#define MVV_LVA_TT_HINT_MOVE INT_MAX
#define MVV_LVA_CAPTURE      1000000
#define MVV_LVA_PROMOTION     500000
#define PAWN_VALUE 100
#define KNIGHT_VALUE 320
#define BISHOP_VALUE 330
#define ROOK_VALUE 500
#define QUEEN_VALUE 900
#define KING_VALUE 1000
#define FLIP(x) ((x)^56)
static int SCE_Engine_ScoreMove(const SCE_Chessboard* const ptr_board, const SCE_ChessMove move) {
    assert(ptr_board != NULL);
    
    int score = 0;
    
    const uint flag = move SCE_CHESSMOVE_GET_FLAG;
    const uint moving_piece_idx = move SCE_CHESSMOVE_GET_SRC;
    const uint64_t moving_piece = 1ULL << moving_piece_idx;
    int moving_piece_type = UNASSIGNED;
    int captured_piece_type = UNASSIGNED;
    const int piece_values[] = {
        PAWN_VALUE, KNIGHT_VALUE, BISHOP_VALUE, ROOK_VALUE, QUEEN_VALUE, KING_VALUE,
        PAWN_VALUE, KNIGHT_VALUE, BISHOP_VALUE, ROOK_VALUE, QUEEN_VALUE, KING_VALUE
    };

    // Determine moving piece/attacker
    for (uint piece_type = W_PAWN; piece_type <= B_KING; piece_type++) {
        if (ptr_board->bitboards[piece_type] & moving_piece) {
            moving_piece_type = piece_type;
            break;
        }
    }
    assert(moving_piece_type != UNASSIGNED);

    if ((move SCE_CHESSMOVE_GET_FLAG) & SCE_CHESSMOVE_FLAG_CAPTURE) {
        // This is a capture.
        score += MVV_LVA_CAPTURE;

        // Determine victim
        // Captured piece depends on the flag
        if (flag == SCE_CHESSMOVE_FLAG_EN_PASSANT_CAPTURE) {
            uint64_t captured_piece = ptr_board->to_move == WHITE ? (1ULL << (ptr_board->en_passant_idx - CHESSBOARD_DIMENSION)) : (1ULL << (ptr_board->en_passant_idx + CHESSBOARD_DIMENSION));
            for (uint piece_type = W_PAWN; piece_type <= B_KING; piece_type++) {
                // Determine en passant victim piece type
                if (captured_piece & ptr_board->bitboards[piece_type]) {
                    captured_piece_type = piece_type;
                    break;
                }
            }
        } else {
            for (uint piece_type = W_PAWN; piece_type <= B_KING; piece_type++) {
                // Determine captured piece type
                const uint64_t dst = 1ULL << (move SCE_CHESSMOVE_GET_DST);
                if (dst & ptr_board->bitboards[piece_type]) {
                    captured_piece_type = piece_type;
                }
            }
        }

        assert(captured_piece_type != UNASSIGNED);
        //if (moving_piece_type == UNASSIGNED || captured_piece_type == UNASSIGNED) return 0;

        // MVV-LVA scoring
        const int attacker_value = piece_values[moving_piece_type];
        const int victim_value = piece_values[captured_piece_type];
        
        score += (victim_value * 100) - attacker_value + (flag & SCE_CHESSMOVE_FLAG_FILTER_PROMOTION ? MVV_LVA_PROMOTION : 0);
        return score;
    } else {
        // Determine captured piece type
        const uint dst_idx = move SCE_CHESSMOVE_GET_DST;
        const uint src_idx_adjusted = ptr_board->to_move == WHITE ? moving_piece_idx : FLIP(moving_piece_idx);
        const uint dst_idx_adjusted = ptr_board->to_move == WHITE ? dst_idx : FLIP(dst_idx);
        int score = (flag & SCE_CHESSMOVE_FLAG_FILTER_PROMOTION) ? MVV_LVA_PROMOTION : 0;
        switch (moving_piece_type) {
            case W_PAWN:
            case B_PAWN:
                return score + PST[PST_PAWN][dst_idx_adjusted] - PST[PST_PAWN][src_idx_adjusted];
            case W_KNIGHT:
            case B_KNIGHT:
                return score + PST[PST_KNIGHT][dst_idx_adjusted] - PST[PST_KNIGHT][src_idx_adjusted];
            case W_BISHOP:
            case B_BISHOP:
                return score + PST[PST_BISHOP][dst_idx_adjusted] - PST[PST_BISHOP][src_idx_adjusted];
            case W_ROOK:
            case B_ROOK:
                return score + PST[PST_ROOK][dst_idx_adjusted] - PST[PST_ROOK][src_idx_adjusted];
            case W_QUEEN:
            case B_QUEEN:
                return score + PST[PST_QUEEN][dst_idx_adjusted] - PST[PST_QUEEN][src_idx_adjusted];
            case W_KING:
            case B_KING:
                {
                    const int mg_pst = PST[PST_KING_MIDDLE][dst_idx_adjusted] - PST[PST_KING_MIDDLE][src_idx_adjusted];
                    const int eg_pst = PST[PST_KING_END][dst_idx_adjusted] - PST[PST_KING_END][src_idx_adjusted];
                    const int phase = SCE_Eval_ComputePhase(ptr_board);
                    return score + ((mg_pst * phase) + (eg_pst * (TOTAL_PHASE_WEIGHT - phase))) / TOTAL_PHASE_WEIGHT;
                }
            default:
                return 0;
        }

        return 0;
    }
}

static SCE_Return SCE_Engine_OrderMove_MVVLVA(SCE_ChessMoveList* const ptr_movelist, const SCE_Chessboard* const ptr_board, const int tt_hint_move) {
    if (ptr_movelist == NULL) return SCE_INVALID_BOARD_STATE;

    // Keeps track of how many elements are sorted.
    uint n_sorted = 0U;
    int move_scores[N_MAX_MOVES] = { 0 };

    if (tt_hint_move != UNASSIGNED) {
        // Check if exists, and try first.
        for (uint i = n_sorted; i < ptr_movelist->count; i++) {
            if (ptr_movelist->moves[i] == tt_hint_move) {
                // Swap with the first entry.
                const SCE_ChessMove temp = ptr_movelist->moves[n_sorted];
                ptr_movelist->moves[n_sorted] = tt_hint_move;
                ptr_movelist->moves[i] = temp;
                move_scores[n_sorted] = INT_MAX;
                n_sorted++;
                break;
            }
        }
    }

    // Compute MVV-LVA score.
    for (uint i = n_sorted; i < ptr_movelist->count; i++) {
        move_scores[i] = SCE_Engine_ScoreMove(ptr_board, ptr_movelist->moves[i]);
    }

    // Sort based on score, remembering to update the score array too.
    while (n_sorted < ptr_movelist->count) {
        for (uint i = n_sorted; i < ptr_movelist->count; i++) {
            // TODO
        }
        n_sorted++;
    }


    return SCE_SUCCESS;
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

    int tt_hint_move = UNASSIGNED;
    // Zobrist-Transposition-Table Lookup
    const SCE_TranspositionTableEntry* const ptr_transposition_entry = SCE_Engine_GetTransposition(ptr_engine, ptr_board->zobrist_hash);
    if (ptr_transposition_entry && ptr_transposition_entry->zobrist_hash == ptr_board->zobrist_hash) {
        if (depth >= ptr_transposition_entry->depth) {
            tt_hint_move = ptr_transposition_entry->move;
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
    ret = SCE_Engine_OrderMove_MVVLVA(&moves, ptr_board, tt_hint_move);
    assert(ret == SCE_SUCCESS);

    // TODO: MVV-LVA Guessing and sorting

    // TODO: Check if king is in check or stalemate.
    // Iterating through moves
    /**
     * Note: To help my understanding of alpha-beta, here is an explanation.
     * Alpha: score that I can guarantee I can get.
     * Beta: Score that opponent can guarantee they can get.
     * 
     * Example:
     * 1. Suppose I search for a move, and it evaluates to +5 after searching (This is done recursively, which will be outlined from Step 2). My alpha is +5.
     * 2. I explore second move,
     * 2.1. Exploring this move, the opponent has a move that evaluates to +2. The beta is +2 now.
     * 3. This means if I choose this move, opponent has a move that evaluates to +2. I might as well take the first move.
     */
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
