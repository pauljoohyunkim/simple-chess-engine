#ifndef SCE_ENGINE_H
#define SCE_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include "chess.h"

typedef int (*SCE_Eval)(SCE_Context* const);
typedef int (*SCE_DeltaEval)(const SCE_Chessboard* const ptr_board, SCE_EvalState* const ptr_eval_state, const SCE_ChessMove move);

#define SCE_ALPHA_INITIAL (INT_MIN / 2)
#define SCE_BETA_INITIAL (INT_MAX / 2)

// Alpha: Upper, Beta: Lower
typedef enum {
    SCE_TF_ALPHA = 0,
    SCE_TF_BETA = 1,
    SCE_TF_EXACT = 2,
} SCE_TranspositionFlag;

typedef struct {
    uint64_t zobrist_hash;  // If 0, this entry is blank.
    int32_t score;
    SCE_ChessMove move;     // Best move at current node.
    uint8_t depth;
    uint8_t flag;
} SCE_TranspositionTableEntry;

typedef struct {
    SCE_TranspositionTableEntry* entries;
    size_t table_size;
} SCE_TranspositionTable;

#define SCE_MAX_PLY 50
typedef struct {
    SCE_Eval eval_function;
    SCE_DeltaEval delta_eval_function;
    SCE_TranspositionTable transposition_table;
    uint8_t depth;
    SCE_ChessMove killer_moves[SCE_MAX_PLY][2];
} SCE_Engine;

/**
 * @brief Sets up SCE_Engine struct.
 * 
 * @param ctx Pointer to the SCE_Context struct
 * @param ptr_engine Pointer to the SCE_Engine struct.
 * @param eval_func Function pointer to evaluation function of the board.
 * @param delta_eval_func Function pointer to incremental evaluation function of the board.
 * @param transposition_table_log2_size Number of entries that transposition table could theoretically hold, applied log2(.).
 * The actual size of the transposition table will be 2^(transposition_table_log2_size) entries.
 * @return SCE_Return SCE_SUCCESS for success, otherwise for failure.
 */
SCE_Return SCE_Engine_init(SCE_Context* const ctx, SCE_Engine* const ptr_engine, const SCE_Eval eval_func, const SCE_DeltaEval delta_eval_func, const unsigned int transposition_table_log2_size);

/**
 * @brief Releases dynamically allocated components within SCE_Engine struct and empties it.
 * 
 * @param ptr_engine Pointer to the SCE_Engine struct.
 * @return SCE_Return SCE_SUCCESS for success, otherwise for failure.
 */
SCE_Return SCE_Engine_release(SCE_Engine* const ptr_engine);

/**
 * @brief Outputs the best move calculated by engine via simple alpha beta search.
 * 
 * @param ptr_engine Pointer to to the SCE_Engine struct
 * @param ctx Pointer to the SCE_Context struct
 * @return int Best move (in which case, can be casted to SCE_ChessMove) or EMPTY_MOVE (0)
 */
SCE_ChessMove SCE_Engine_AlphaBetaBestMove(SCE_Engine *const ptr_engine, SCE_Context *const ctx);

/**
 * @brief Outputs the best move calculated by the engine via iterative deepening with alpha beta.
 * 
 * @param ptr_engine Pointer to to the SCE_Engine struct
 * @param ctx Pointer to the SCE_Context struct
 * @return int Best move (in which case, can be casted to SCE_ChessMove) or EMPTY_MOVE (0)
 */
SCE_ChessMove SCE_Engine_IterativeDeepeningAlphaBetaBestMove(SCE_Engine* const ptr_engine, SCE_Context* const ctx);

#ifdef __cplusplus
}
#endif
#endif  // SCE_ENGINE_H
