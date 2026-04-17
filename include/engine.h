#ifndef SCE_ENGINE_H
#define SCE_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include "chess.h"

typedef int (*SCE_Eval)(const SCE_Chessboard* const);

// Alpha: Upper, Beta: Lower
typedef enum {
    SCE_TF_ALPHA = 0,
    SCE_TF_BETA = 1,
    SCE_TF_EXACT = 2,
} SCE_TranspositionFlag;

typedef struct {
    uint64_t zobrist_hash;
    int score;
    SCE_ChessMove move;     // Best move at current node.
    unsigned int depth;
    SCE_TranspositionFlag flag;
} SCE_TranspositionTableEntry;

typedef struct {
    SCE_TranspositionTableEntry* entries;
    size_t table_size;
} SCE_TranspositionTable;

typedef struct {
    SCE_Eval eval_function;
    SCE_TranspositionTable transposition_table;
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
 * @brief Evaluate the chessboard from White's perspective
 * 
 * @param ptr_engine Pointer to the SCE_Engine struct.
 * @param ptr_board Pointer to the SCE_Chessboard struct.
 * @return int Centipawn value where positive means advantage for white and negative means advantage for black.
 * 
 * Note that this function does not return success or failure code.
 */
int SCE_Engine_EvaluateBoard(const SCE_Engine* const ptr_engine, const SCE_Chessboard* const ptr_board);

#ifdef __cplusplus
}
#endif
#endif  // SCE_ENGINE_H
