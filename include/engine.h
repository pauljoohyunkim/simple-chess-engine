#ifndef SCE_ENGINE_H
#define SCE_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include "chess.h"

// Forward declare SCE_Engine
struct SCE_Engine;
typedef struct SCE_Engine SCE_Engine;

typedef int (*SCE_Eval)(SCE_Context* const ctx, SCE_Engine* const ptr_engine);
typedef int (*SCE_DeltaEval)(SCE_Chessboard* const ptr_board, SCE_EvalState* const ptr_eval_state, SCE_Engine* const ptr_engine, const SCE_ChessMove move);

#define SCE_ALPHA_INITIAL (INT32_MIN / 2)
#define SCE_BETA_INITIAL (INT32_MAX / 2)

// Alpha: Upper, Beta: Lower
typedef enum {
    SCE_TF_ALPHA = 0,
    SCE_TF_BETA = 1,
    SCE_TF_EXACT = 2,
} SCE_TranspositionFlag;

 // data(8) = score(4) | move(2) | depth(1) | flag(1)
typedef struct {
    uint64_t zobrist_hash_chksum; // zobrist_hash ^ data.
    uint64_t data;
} __attribute__((aligned(16))) SCE_TranspositionTableEntry;
#define SCE_TT_SET_SCORE << 32U
#define SCE_TT_SET_MOVE << 16U
#define SCE_TT_SET_DEPTH << 8U
#define SCE_TT_SET_FLAG << 0U
#define SCE_TT_GET_SCORE(d) (((int32_t)((d) >> 32)) & 0xFFFFFFFFULL)
#define SCE_TT_GET_MOVE(d)  (((SCE_ChessMove)((d) >> 16)) & 0xFFFFULL)
#define SCE_TT_GET_DEPTH(d) (((uint8_t)((d) >> 8)) & 0xFFULL)
#define SCE_TT_GET_FLAG(d)  (((uint8_t)((d) >> 0)) & 0xFFULL)

typedef struct {
    uint64_t pawn_zobrist_hash_chksum;  // zobrist_hash ^ score_data ^ passed_pawns ^ weak_pawns
    uint64_t score_data;    // score_data(8) =  mg_score(4) | eg_score(4)
    uint64_t passed_pawns;  // Bitboard of passed pawns of both colors
    uint64_t weak_pawns;    // Bitboard of weak pawns (backward, isolated, etc.)
} __attribute__((aligned(32))) SCE_PawnHashTableEntry;
#define SCE_PHT_SET_MG_SCORE << 32U
#define SCE_PHT_SET_EG_SCORE << 0U
#define SCE_PHT_GET_MG_SCORE(d) (((int32_t)((d) >> 32)) & 0xFFFFFFFFULL)
#define SCE_PHT_GET_EG_SCORE(d) (((int32_t)((d) >> 0)) & 0xFFFFFFFFULL)

typedef struct {
    SCE_TranspositionTableEntry* entries;
    size_t table_size;
} SCE_TranspositionTable;

#define PHT_N_ENTRIES_LOG_2 (16)
typedef SCE_PawnHashTableEntry SCE_PawnHashTable[1 << PHT_N_ENTRIES_LOG_2];

struct SCE_Engine {
    volatile bool stop_searching;
    SCE_Eval eval_function;
    SCE_DeltaEval delta_eval_function;
    SCE_TranspositionTable transposition_table;
    SCE_PawnHashTable pawn_hash_table;
    SCE_ChessMove killer_moves[SCE_MAX_PLY][2];
};

typedef struct {
    unsigned int start_depth;   // Set to 0 unless multi-threading with helpers.
    bool use_lmr;
    int lmr_bias;
    //bool check_timeout;
    //uint64_t time_limit;
} SCE_Engine_SearchControl;

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
 * @brief Add pawn hash entry to pawn hash table
 * 
 * @param ptr_engine Pointer to the SCE_Engine struct.
 * @param pawn_zobrist_hash Zobrist hash only accounting locations of pawns.
 * @param mg_score Middlegame score
 * @param eg_score Endgame score
 * @param passed_pawns Bitboard of passed pawns
 * @param weak_pawns Bitboard of weak pawns (backward, isolated, etc.)
 * @return true Successful
 * @return false Failure
 */
inline bool SCE_Engine_AddPawnHashData(SCE_Engine* const ptr_engine, const uint64_t pawn_zobrist_hash, const int32_t mg_score, const int32_t eg_score, const uint64_t passed_pawns, const uint64_t weak_pawns);

/**
 * @brief Get pawn hash entry from pawn hash table
 * 
 * @param entry Pointer to the SCE_PawnHashTableEntry struct where the lookup will be written if successful.
 * @param ptr_engine Pointer to the SCE_Engine struct.
 * @param pawn_zobrist_hash Zobrist hash of only pawn locations
 * @return true Successful
 * @return false Failure
 */
inline bool SCE_Engine_GetPawnHashData(SCE_PawnHashTableEntry* entry, SCE_Engine* const ptr_engine, const uint64_t pawn_zobrist_hash);

/**
 * @brief Returns whether or not there has been a repetition (for draw rule)
 * 
 * @param ctx Pointer to the SCE_Context struct.
 * @return true If there is a repetition.
 * @return false If there is no repetition.
 */
bool SCE_DetectRepetition(const SCE_Context* const ctx);

/**
 * @brief Returns whether or not there are insufficient materials (for draw rule)
 * 
 * @param ctx Pointer to the SCE_Context struct.
 * @return true If insufficient materials
 * @return false Otherwise
 */
bool SCE_DetectInsufficientMaterial(const SCE_Context* const ctx);

/**
 * @brief Outputs the best move calculated by engine via simple alpha beta search.
 * 
 * @param ptr_engine Pointer to to the SCE_Engine struct
 * @param ctx Pointer to the SCE_Context struct
 * @return int Best move (in which case, can be casted to SCE_ChessMove) or EMPTY_MOVE (0)
 */
SCE_ChessMove SCE_Engine_AlphaBetaBestMove(SCE_Engine *const ptr_engine, SCE_Context* const ctx, const SCE_Engine_SearchControl* const ptr_ctrl);

/**
 * @brief Outputs the best move calculated by the engine via iterative deepening with alpha beta.
 * 
 * @param ptr_engine Pointer to to the SCE_Engine struct
 * @param ctx Pointer to the SCE_Context struct
 * @return int Best move (in which case, can be casted to SCE_ChessMove) or EMPTY_MOVE (0)
 */
SCE_ChessMove SCE_Engine_IterativeDeepeningAlphaBetaBestMove(SCE_Engine* const ptr_engine, SCE_Context* const ctx, const SCE_Engine_SearchControl* const ptr_ctrl);

#ifdef __cplusplus
}
#endif
#endif  // SCE_ENGINE_H
