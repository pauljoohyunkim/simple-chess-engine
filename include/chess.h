#ifndef SCE_CHESS_H
#define SCE_CHESS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
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
#define QUEEN_INITIAL_ROW (1ULL << 3U)
#define KING_INITIAL_ROW (1ULL << 4U)

typedef enum {
    WHITE = 0,
    BLACK = 1
} PieceColor;

typedef enum {
    NORTH = 0,
    EAST = 1,
    SOUTH = 2,
    WEST = 3,
    NORTHEAST = 4,
    NORTHWEST = 5,
    SOUTHEAST = 6,
    SOUTHWEST = 7
} RayDirection;

/**
 * Bits 0-5: Source (0-65)
 * Bits 6-11: Destination ((0-63) << 6U)
 * Bits 12-15: Flag ((0-15) << 12U)
 *
 */
typedef uint16_t SCE_ChessMove;
#define SCE_CHESSMOVE_SET_SRC << 0U
#define SCE_CHESSMOVE_SET_DST << 6U
#define SCE_CHESSMOVE_SET_FLAG << 12U
#define SCE_CHESSMOVE_GET_SRC & 63U
#define SCE_CHESSMOVE_GET_DST >> 6U & 63U
#define SCE_CHESSMOVE_GET_FLAG >> 12U & 63U

#define N_MAX_LEGAL_PSEUDOMOVES 256U
typedef struct {
    SCE_ChessMove moves[N_MAX_LEGAL_PSEUDOMOVES];
    unsigned int count;
} SCE_ChessMoveList;

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

    // Sliders: Queen, Rook, Bishop
    uint64_t rays[8][CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION];
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
 * @brief Returns the bitboard of occupancy information.
 * 
 * @param ptr_board Pointer to the SCE_Chessboard struct.
 * @return uint64_t bitboard where set bits are occupied, or 0 for error.
 */
uint64_t SCE_Chessboard_Occupancy(const SCE_Chessboard* const ptr_board);

/**
 * @brief Returns the bitboard of occupancy by color information.
 * 
 * @param ptr_board Pointer to the SCE_Chessboard struct
 * @param color Color of the piece
 * @return uint64_t bitboard where set bits are occupied, or 0 for error.
 */
uint64_t SCE_Chessboard_Occupancy_Color(const SCE_Chessboard* const ptr_board, const PieceColor color);

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

/**
 * @brief Checks if the square is under attack by certain color.
 * 
 * @param ptr_board Pointer to the SCE_Chessboard struct.
 * @param ptr_precomputation_tbl Pointer to the SCE_PieceMovementPrecomputationTable struct.
 * @param square uint64_t value with a single bit active marking the square. It is paramount that only a single bit is active.
 * @param attacked_by Color of the piece attacking the square.
 * @return true The square is under attack.
 * @return false The square is not under attack or error. If there is an error, there will be error message output to STDERR.
 */
bool SCE_IsSquareAttacked(SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl, const uint64_t square, const PieceColor attacked_by);

/**
 * @brief Converts algebraic notation for a square to bitboard
 * 
 * @param an Two-letter string, from "A1" to "H8"
 * @return uint64_t bitboard, or 0 for error.
 */
uint64_t SCE_AN_To_Bitboard(const char* an);

/**
 * @brief Converts bitboard with single bit to algebraic notation
 * 
 * @param an_out Char array. Needs at least three spaces (for null terminator)
 * @param bitboard uin64_t representation of board. Only single bit must be set.
 * @return int 1 for success, 0 for failure
 */
int SCE_Bitboard_To_AN(char* const an_out, uint64_t bitboard);

/**
 * @brief Generates legal moves
 * 
 * @param ptr_movelist List of moves
 * @param ptr_board Pointer to the SCE_Chessboard struct.
 * @param ptr_precomputation_tbl Pointer to the SCE_PieceMovementPrecomputationTable struct.
 * @return int 1 for success, 0 for failure.
 */
int SCE_GenerateLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);

#ifdef __cplusplus
}
#endif
#endif  // SCE_CHESS_H
