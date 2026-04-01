#ifndef SCE_CHESS_H
#define SCE_CHESS_H

#include <stdint.h>

#define SCE_SUCCESS 1
#define SCE_FAILURE 0

#define CHESSBOARD_DIMENSION 8U

#define W_PAWN 0U
#define W_KNIGHT 1U
#define W_BISHOP 2U
#define W_ROOK 3U
#define W_QUEEN 4U
#define W_KING 5U
#define B_PAWN 6U
#define B_KNIGHT 7U
#define B_BISHOP 8U
#define B_ROOK 9U
#define B_QUEEN 10U
#define B_KING 11U

#define N_TYPES_PIECES 12U

#define PAWN_INITIAL_ROW (0xFFULL)
#define KNIGHT_INITIAL_ROW ((1ULL << 6U) ^ (1ULL << 1U))
#define BISHOP_INITIAL_ROW ((1ULL << 5U) ^ (1ULL << 2U))
#define ROOK_INITIAL_ROW ((1ULL << 7U) ^ (1U))
#define QUEEN_INITIAL_ROW (1ULL << 4U)
#define KING_INITIAL_ROW (1ULL << 3U)

typedef enum {
    WHITE = 0,
    BLACK = 1
} PieceColor;

/**
 * @brief Bitboard capturing a chessboard.
 * 
 * Each uint64_t captures the entire board,
 * and the bits represent the occupancy by each type.
 * For example, 00000000 00000000 00000000 00000000 00000000 00000000 11111111 00000000
 * for bitboards[W_PAWN] represents the row of white pawns.
 */
typedef struct {
    uint64_t bitboards[N_TYPES_PIECES];
} SCE_Chessboard;

// 3584 bytes
typedef struct {
    // Leapers
    uint64_t knight_moves[CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION];
    uint64_t king_moves[CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION];
    uint64_t pawn_moves[2][CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION];
    uint64_t pawn_attacks[2][CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION];

    // Sliders
    uint64_t bishop[CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION];
    uint64_t rook[CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION];
    uint64_t queen[CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION];
} SCE_PieceMovementPrecomputationTable;

/**
 * @brief Clear out the board with zeros.
 * 
 * @param ptr_board Pointer to the SCE_Chessboard struct.
 * @return int 1 for success, 0 for failure
 */
int SCE_Chessboard_clear(SCE_Chessboard* const ptr_board);

/**
 * @brief Reset the board to a initial setup.
 * 
 * @param ptr_board Pointer to the SCE_Chessboard struct.
 * @return int 1 for success, 0 for failure
 */
int SCE_Chessboard_reset(SCE_Chessboard* const ptr_board);

/**
 * @brief Print the board to console.
 * 
 * @param ptr_board Pointer to the SCE_Chessboard struct.
 * @return int 1 for success, 0 for failure
 */
int SCE_Chessboard_print(SCE_Chessboard* const ptr_board, PieceColor color);

/**
 * @brief Fill the movement precomputation table.
 * 
 * @param ptr_precomputation_tbl Pointer to the SCE_PieceMovementPrecomputationTable struct.
 * @return int 1 for success, 0 for failure
 */
int SCE_PieceMovementPrecompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);

#endif  // SCE_CHESS_H
