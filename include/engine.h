#ifndef SCE_ENGINE_H
#define SCE_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include "chess.h"

typedef int (*SCE_Eval)(const SCE_Chessboard* const);

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

typedef struct {
    SCE_Eval eval_function;
    SCE_TranspositionTable transposition_table;
    uint8_t depth;
} SCE_Engine;

/**
 * @brief Sets up SCE_Engine struct.
 * 
 * @param ptr_engine Pointer to the SCE_Engine struct.
 * @param eval_func Function pointer to evaluation function of the board.
 * @param transposition_table_log2_size Number of entries that transposition table could theoretically hold, applied log2(.).
 * The actual size of the transposition table will be 2^(transposition_table_log2_size) entries.
 * @return SCE_Return SCE_SUCCESS for success, otherwise for failure.
 */
SCE_Return SCE_Engine_init(SCE_Engine* const ptr_engine, const SCE_Eval eval_func, const unsigned int transposition_table_log2_size);

/**
 * @brief Releases dynamically allocated components within SCE_Engine struct and empties it.
 * 
 * @param ptr_engine Pointer to the SCE_Engine struct.
 * @return SCE_Return SCE_SUCCESS for success, otherwise for failure.
 */
SCE_Return SCE_Engine_release(SCE_Engine* const ptr_engine);

/**
 * @brief Outputs the best move calculated by engine.
 * 
 * @param ptr_engine Pointer to the SCE_Engine struct.
 * @param ptr_board Pointer to the SCE_Chessboard struct.
 * @param ptr_precomputation_tbl Pointer to the SCE_PieceMovementPrecomputationTable struct.
 * @param ptr_table Pointer to the SCE_ZobristTable struct.
 * @return int Best move (in which case, can be casted to SCE_ChessMove) or UNASSIGNED (-1)
 */
int SCE_Engine_AlphaBetaBestMove(SCE_Engine *const ptr_engine,
                                           SCE_Chessboard *const ptr_board,
                                           SCE_PieceMovementPrecomputationTable *const ptr_precomputation_tbl,
                                           SCE_ZobristTable *const ptr_table);

#ifdef __cplusplus
}
#endif
#endif  // SCE_ENGINE_H
