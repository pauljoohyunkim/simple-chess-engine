#ifndef SCE_CHESS_H
#define SCE_CHESS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "return_code.h"

#define CHESSBOARD_DIMENSION 8U

#define UNASSIGNED (-1)

typedef enum {
    W_PAWN = 0,
    W_KNIGHT = 1,
    W_BISHOP = 2,
    W_ROOK = 3,
    W_QUEEN = 4,
    W_KING = 5,
    B_PAWN = 6,
    B_KNIGHT = 7,
    B_BISHOP = 8,
    B_ROOK = 9,
    B_QUEEN = 10,
    B_KING = 11,
    UNASSIGNED_PIECE_TYPE = UNASSIGNED
} PieceType;

#define N_TYPES_PIECES 12U

#define PAWN_INITIAL_ROW (0xFFULL)
#define KNIGHT_INITIAL_ROW ((1ULL << 6U) ^ (1ULL << 1U))
#define BISHOP_INITIAL_ROW ((1ULL << 5U) ^ (1ULL << 2U))
#define ROOK_INITIAL_ROW ((1ULL << 7U) ^ (1U))
#define QUEEN_INITIAL_ROW (1ULL << 3U)
#define KING_INITIAL_ROW (1ULL << 4U)

// Masks for file A and file H
#define A_MASK 0x101010101010101ULL
#define H_MASK 0x8080808080808080ULL

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
#define SCE_CHESSMOVE_FLAG_QUIET_MOVE (0U)
#define SCE_CHESSMOVE_FLAG_DOUBLE_PAWN_PUSH (1U)
#define SCE_CHESSMOVE_FLAG_KING_CASTLE (2U)
#define SCE_CHESSMOVE_FLAG_QUEEN_CASTLE (3U)
#define SCE_CHESSMOVE_FLAG_CAPTURE (4U)
#define SCE_CHESSMOVE_FLAG_EN_PASSANT_CAPTURE (5U)
#define SCE_CHESSMOVE_FLAG_KNIGHT_PROMOTION (8U)
#define SCE_CHESSMOVE_FLAG_BISHOP_PROMOTION (9U)
#define SCE_CHESSMOVE_FLAG_ROOK_PROMOTION (10U)
#define SCE_CHESSMOVE_FLAG_QUEEN_PROMOTION (11U)
#define SCE_CHESSMOVE_FLAG_KNIGHT_PROMO_CAPTURE (12U)
#define SCE_CHESSMOVE_FLAG_BISHOP_PROMO_CAPTURE (13U)
#define SCE_CHESSMOVE_FLAG_ROOK_PROMO_CAPTURE (14U)
#define SCE_CHESSMOVE_FLAG_QUEEN_PROMO_CAPTURE (15U)
#define SCE_CHESSMOVE_FLAG_FILTER_PROMOTION (8U)

#define SCE_CASTLING_RIGHTS_WK (1U << 0)
#define SCE_CASTLING_RIGHTS_WQ (1U << 1)
#define SCE_CASTLING_RIGHTS_BK (1U << 2)
#define SCE_CASTLING_RIGHTS_BQ (1U << 3)

typedef struct {
    unsigned int moving_piece;
    int captured_piece;
    int en_passant_square;
    uint8_t castling_rights;
    unsigned int half_move_clock;       // 50-move rule
    uint64_t zobrist_hash;
} SCE_UndoState;

#define N_MAX_MOVES (512U)
typedef struct {
    SCE_ChessMove moves[N_MAX_MOVES];
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
    int en_passant_idx;
    PieceColor to_move;
    uint8_t castling_rights;
    unsigned int half_move_clock;
    uint64_t zobrist_hash;
    PieceType mailbox[CHESSBOARD_DIMENSION*CHESSBOARD_DIMENSION];
    SCE_ChessMoveList history;
    SCE_UndoState undo_states[N_MAX_MOVES];
} SCE_Chessboard;

typedef struct {
    uint64_t piece_key[N_TYPES_PIECES][CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION];
    uint64_t castling_keys[16U];
    uint64_t en_passant_keys[9U];
    uint64_t side_key;
} SCE_ZobristTable;

typedef struct {
    // Leapers
    uint64_t knight_moves[CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION];
    uint64_t king_moves[CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION];
    uint64_t pawn_moves[2][CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION];
    uint64_t pawn_attacks[2][CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION];

    // Sliders: Queen, Rook, Bishop
    uint64_t rays[8][CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION];

    // Castling Rights
    uint8_t castling_mask[CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION];
} SCE_PieceMovementPrecomputationTable;

typedef struct {
    SCE_Chessboard board;
    SCE_PieceMovementPrecomputationTable precomputation_table;
    SCE_ZobristTable zobrist_table;
} SCE_Context;

/**
 * @brief Clear out the move list
 * 
 * @param ptr_list Pointer to the SCE_ChessMoveList struct.
 * @return SCE_Return SCE_SUCCESS for success, other for failure.
 */
SCE_Return SCE_ChessMoveList_clear(SCE_ChessMoveList* const ptr_list);

/**
 * @brief Clear out the board with zeros.
 * 
 * @param ctx Pointer to the SCE_Context struct.
 * @return SCE_Return SCE_SUCCESS for success, other for failure.
 */
SCE_Return SCE_Chessboard_clear(SCE_Context* const ctx);

/**
 * @brief Reset the board to a initial setup.
 * 
 * @param ctx Pointer to the SCE_Context struct.
 * @return SCE_Return SCE_SUCCESS for success, other for failure.
 */
SCE_Return SCE_Chessboard_reset(SCE_Context* const ctx);

/**
 * @brief Initialize a table to be used for Zobrist hashing.
 * 
 * @param ctx Pointer to the SCE_Context struct.
 * @param seed Pointer to seed value for random number generation. NULL for randomly picked seed.
 * @return SCE_Return SCE_SUCCESS for success, other for failure.
 */
SCE_Return SCE_ZobristTable_init(SCE_Context* const ctx, const uint64_t* const ptr_seed);

/**
 * @brief Compute the Zobrist hash of the current board. Requires Zobrist table to be precomputed by SCE_ZobristTable_init
 * 
 * @param ctx Pointer to the SCE_Context struct.
 * @return uint64_t Zobrist hash of the board if successful, or 0 for failure.
 */
uint64_t SCE_Chessboard_ComputeZobristHash(SCE_Context* const ctx);

/**
 * @brief Returns the bitboard of occupancy information.
 * 
 * @param ptr_board Pointer to the SCE_Chessboard struct.
 * @return uint64_t bitboard where set bits are occupied, or 0 for error.
 */
uint64_t SCE_Chessboard_Occupancy(const SCE_Context* const ctx);

/**
 * @brief Returns the bitboard of occupancy by color information.
 * 
 * @param ptr_board Pointer to the SCE_Chessboard struct
 * @param color Color of the piece
 * @return uint64_t bitboard where set bits are occupied, or 0 for error.
 */
uint64_t SCE_Chessboard_Occupancy_Color(const SCE_Context* const ctx, const PieceColor color);

/**
 * @brief Print the board to console.
 * 
 * @param ptr_board Pointer to the SCE_Chessboard struct.
 * @return SCE_Return SCE_SUCCESS for success, other for failure.
 */
SCE_Return SCE_Chessboard_print(SCE_Context* const ctx, PieceColor color);

/**
 * @brief Fill the movement precomputation table.
 * 
 * @param ptr_precomputation_tbl Pointer to the SCE_PieceMovementPrecomputationTable struct.
 * @return SCE_Return SCE_SUCCESS for success, other for failure.
 */
SCE_Return SCE_PieceMovementPrecompute(SCE_Context* const ctx);

/**
 * @brief Add move to move list
 * 
 * @param move 
 * @param ptr_movelist 
 * @return SCE_Return SCE_SUCCESS for success, other for failure.
 */
SCE_Return SCE_AddToMoveList(const SCE_ChessMove move, SCE_ChessMoveList* const ptr_movelist);

/**
 * @brief Generates pseudolegal moves
 * 
 * @param ptr_movelist List of moves
 * @param ctx Pointer to the SCE_Context struct.
 * @param tactical Whether or not only to generate tactical (capture/promotion) moves.
 * @return SCE_Return SCE_SUCCESS for success, other for failure.
 */
SCE_Return SCE_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Context* const ctx, const bool tactical);

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
bool SCE_IsSquareAttacked(SCE_Context* const ctx, const uint64_t square, const PieceColor attacked_by);

/**
 * @brief Converts algebraic notation for a square to index
 * 
 * @param an Two-letter string, from "A1" to "H8"
 * @return int index for bitboard, or -1 for error.
 */
int SCE_AN_To_Idx(const char* an);

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
 * @return SCE_Return SCE_SUCCESS for success, other for failure.
 */
SCE_Return SCE_Bitboard_To_AN(char* const an_out, uint64_t bitboard);

/**
 * @brief Attempt to make move. Note that this does not check for pseudo legal moves, hence for human input, must be verified if it is a pseudo legal move.
 * 
 * @param ptr_board Pointer to the SCE_Chessboard struct.
 * @param ptr_precomputation_tbl Pointer to the SCE_PieceMovementPrecomputationTable struct.
 * @param move 
 * @return SCE_Return SCE_SUCCESS for success, other for failure.
 * 
 * In the case of failure, the attempted move will be reverted back.
 */
SCE_Return SCE_MakeMove(SCE_Context* const ctx, const SCE_ChessMove move);

/**
 * @brief Unmake move.
 * 
 * @param ptr_board Pointer to the SCE_Chessboard struct.
 * @param ptr_precomputation_table Pointer to the SCE_PieceMovementPrecomputationTable struct.
 * @return SCE_Return SCE_SUCCESS for success, other for failure.
 */
SCE_Return SCE_UnmakeMove(SCE_Context* const ctx);

/**
 * @brief Generate all legal moves from current position.
 * 
 * @param ptr_movelist List of moves
 * @param ptr_board Pointer to the SCE_Chessboard struct.
 * @param ptr_precomputation_tbl Pointer to the SCE_PieceMovementPrecomputationTable struct.
 * @return SCE_Return SCE_SUCCESS for success, other for failure.
 * 
 * In the case of failure, the attempted move will be reverted back.
 */
SCE_Return SCE_GenerateLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Context* const ctx);

#ifdef __cplusplus
}
#endif
#endif  // SCE_CHESS_H
