#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "chess.h"

#define RETURN_IF_SCE_FAILURE(x, msg) do { if ((x) <= 0) { fprintf(stderr, "%s\n", msg); return SCE_INTERNAL_ERROR; } } while (0);

#define MIN(x, y) ((x) > (y) ? (y) : (x))

typedef unsigned int uint;

static uint64_t xorshift(uint64_t x);

// Static functions for generating components of precomputation table.
static SCE_Return SCE_Knight_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);
static SCE_Return SCE_King_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);
static SCE_Return SCE_Pawn_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);
static SCE_Return SCE_Rays_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);
static SCE_Return SCE_CastlingMask_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);

static SCE_Return SCE_AddToMoveList(const SCE_ChessMove move, SCE_ChessMoveList* const ptr_movelist);

static SCE_Return SCE_Knight_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);
static SCE_Return SCE_King_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);
static SCE_Return SCE_Slider_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);
static SCE_Return SCE_Pawn_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);

#ifdef __GNUC__
#define COUNT_SET_BITS __builtin_popcountll
// TODO: Implement fallback
#define COUNT_TRAILING_ZEROS(x) __builtin_ctzll(x)
#define COUNT_LEADING_ZEROS(x) __builtin_clzll(x)
#else
// Miscellaneous static functions
static uint count_set_bits(uint64_t n);

// Kernighan's Bit Counting Algorithm
static uint count_set_bits(uint64_t n) {
    uint cnt = 0U;
    while (n > 0) {
        n &= (n - 1);
        cnt++;
    }
    return cnt;
}
#endif

SCE_Return SCE_ChessMoveList_clear(SCE_ChessMoveList* const ptr_list) {
    if (ptr_list == NULL) return SCE_INVALID_PARAM;

    memset(ptr_list, 0, sizeof(SCE_ChessMoveList));

    return SCE_SUCCESS;
}

SCE_Return SCE_Chessboard_clear(SCE_Chessboard* const ptr_board) {
    if (ptr_board == NULL) return SCE_INVALID_PARAM;

    for (uint i = 0U; i < N_TYPES_PIECES; i++) {
        ptr_board->bitboards[i] = 0U;
    }

    ptr_board->en_passant_idx = UNASSIGNED;
    ptr_board->to_move = WHITE;
    ptr_board->castling_rights = SCE_CASTLING_RIGHTS_WK | SCE_CASTLING_RIGHTS_WQ | SCE_CASTLING_RIGHTS_BK | SCE_CASTLING_RIGHTS_BQ;
    ptr_board->half_move_clock = 0U;
    ptr_board->zobrist_hash = 0U;
    RETURN_IF_SCE_FAILURE(SCE_ChessMoveList_clear(&ptr_board->history), "Error when clearing chess move list");

    return SCE_SUCCESS;
}

SCE_Return SCE_Chessboard_reset(SCE_Chessboard* const ptr_board) {
    if (ptr_board == NULL) return SCE_INVALID_PARAM;

    RETURN_IF_SCE_FAILURE(SCE_Chessboard_clear(ptr_board), "Error when clearing board!");

    // White pieces
    ptr_board->bitboards[W_PAWN] = PAWN_INITIAL_ROW << (8U * 1U);
    ptr_board->bitboards[W_KNIGHT] = KNIGHT_INITIAL_ROW;
    ptr_board->bitboards[W_BISHOP] = BISHOP_INITIAL_ROW;
    ptr_board->bitboards[W_ROOK] = ROOK_INITIAL_ROW;
    ptr_board->bitboards[W_QUEEN] = QUEEN_INITIAL_ROW;
    ptr_board->bitboards[W_KING] = KING_INITIAL_ROW;

    // Black pieces
    ptr_board->bitboards[B_PAWN] = PAWN_INITIAL_ROW << (8U * 6U);
    ptr_board->bitboards[B_KNIGHT] = KNIGHT_INITIAL_ROW << (8U * 7U);
    ptr_board->bitboards[B_BISHOP] = BISHOP_INITIAL_ROW << (8U * 7U);
    ptr_board->bitboards[B_ROOK] = ROOK_INITIAL_ROW << (8U * 7U);
    ptr_board->bitboards[B_QUEEN] = QUEEN_INITIAL_ROW << (8U * 7U);
    ptr_board->bitboards[B_KING] = KING_INITIAL_ROW << (8U * 7U);

    ptr_board->en_passant_idx = UNASSIGNED;
    ptr_board->to_move = WHITE;
    ptr_board->castling_rights = SCE_CASTLING_RIGHTS_WK | SCE_CASTLING_RIGHTS_WQ | SCE_CASTLING_RIGHTS_BK | SCE_CASTLING_RIGHTS_BQ;
    ptr_board->half_move_clock = 0U;
    ptr_board->zobrist_hash = 0U;
    RETURN_IF_SCE_FAILURE(SCE_ChessMoveList_clear(&ptr_board->history), "Error when clearing chess move list");

    return SCE_SUCCESS;
}

static uint64_t xorshift(uint64_t x) {
    x ^= x << 13U;
    x ^= x >> 17U;
    x ^= x << 5U;
    return x;
}

SCE_Return SCE_ZobristTable_init(SCE_ZobristTable* const ptr_table, const uint64_t* const ptr_seed) {
    if (ptr_table == NULL) return SCE_INVALID_PARAM;
    if (ptr_seed == NULL) srand(time(NULL));

    uint64_t x = ptr_seed == NULL ? rand() : (*ptr_seed);

    // Piece keys
    for (uint piece_type = W_PAWN; piece_type <= B_KING; piece_type++) {
        for (uint idx = 0U; idx < CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION; idx++) {
            x = xorshift(x);
            ptr_table->piece_key[piece_type][idx] = x;
        }
    }

    // Castling rights
    for (uint i = 0U; i < 16U; i++) {
        x = xorshift(x);
        ptr_table->castling_keys[i] = x;
    }

    // En passant
    for (uint i = 0U; i < 9U; i++) {
        x = xorshift(x);
        ptr_table->en_passant_keys[i] = x;
    }

    // Side key
    x = xorshift(x);
    ptr_table->side_key = x;

    return SCE_SUCCESS;
}

#define SCE_ZOBRIST_EN_PASSANT_UNASSIGNED_KEY (8U)
uint64_t SCE_Chessboard_ComputeZobristHash(SCE_Chessboard* const ptr_board, SCE_ZobristTable* const ptr_table) {
    if (ptr_board == NULL || ptr_table == NULL) return SCE_INVALID_PARAM;

    uint64_t hash = 0U;

    // Board
    for (uint piece_type = W_PAWN; piece_type <= B_KING; piece_type++) {
        // Find the pieces.
        uint64_t pieces = ptr_board->bitboards[piece_type];
        while (pieces) {
            // Get index of pieces one by one.
            const uint idx = COUNT_TRAILING_ZEROS(pieces);
            hash ^= ptr_table->piece_key[piece_type][idx];
            pieces &= ~(1ULL << idx);
        }
    }

    // Castling
    hash ^= ptr_table->castling_keys[ptr_board->castling_rights];

    // En passant
    if (ptr_board->en_passant_idx == UNASSIGNED) {
        hash ^= ptr_table->en_passant_keys[SCE_ZOBRIST_EN_PASSANT_UNASSIGNED_KEY];
    } else {
        const uint col_idx = ptr_board->en_passant_idx % 8U;
        hash ^= ptr_table->en_passant_keys[col_idx];
    }

    // Side
    if (ptr_board->to_move == BLACK) {
        hash ^= ptr_table->side_key;
    }

    return hash;
}

uint64_t SCE_Chessboard_Occupancy(const SCE_Chessboard* const ptr_board) {
    if (ptr_board == NULL) return 0U;

    uint64_t occupancy = 0ULL;
    for (uint piece_type = 0U; piece_type < N_TYPES_PIECES; piece_type++) {
        occupancy ^= ptr_board->bitboards[piece_type];
    }

    return occupancy;
}

uint64_t SCE_Chessboard_Occupancy_Color(const SCE_Chessboard* const ptr_board, const PieceColor color) {
    if (ptr_board == NULL || (color != WHITE && color != BLACK)) return 0U;

    uint64_t occupancy = 0ULL;
    if (color == WHITE) {
        // White
        for (uint piece_type = W_PAWN; piece_type <= W_KING; piece_type++) {
            occupancy ^= ptr_board->bitboards[piece_type];
        }
    } else {
        // Black
        for (uint piece_type = B_PAWN; piece_type <= B_KING; piece_type++) {
            occupancy ^= ptr_board->bitboards[piece_type];
        }
    }

    return occupancy;
}

SCE_Return SCE_Chessboard_print(SCE_Chessboard* const ptr_board, PieceColor color) {
    if (ptr_board == NULL) return SCE_INVALID_PARAM;
    if (color != WHITE && color != BLACK) return SCE_INVALID_PARAM;

    printf("\n");

    // Print file
    if (color == WHITE) {
        for (uint i = 0; i < CHESSBOARD_DIMENSION; i++) {
            printf("%c ", 'A' + i);
        }
    } else {
        for (uint i = 0; i < CHESSBOARD_DIMENSION; i++) {
            printf("%c ", 'H' - i);
        }
    }

    printf("\n");

    for (uint i = 0; i < CHESSBOARD_DIMENSION; i++) {
        for (uint j = 0; j < CHESSBOARD_DIMENSION; j++) {
            uint shift = (8U * (7U-i) + j);
            if (color == BLACK) {
                shift = 63U - shift;
            }
            uint64_t pos = 1ULL << shift;

            int piece_in_square = UNASSIGNED;
            // For each square, check if any of the type exists.
            for (uint piece_type = 0U; piece_type < N_TYPES_PIECES; piece_type++) {
                if (ptr_board->bitboards[piece_type] & pos) {
                    // Check exclusive ownership of square.
                    if (piece_in_square != UNASSIGNED) return SCE_INVALID_BOARD_STATE;

                    piece_in_square = (int) piece_type;
                }
            }

            switch (piece_in_square) {
                case W_PAWN:
                    printf("\x1b[37mP\x1b[0m");
                    break;
                case W_KNIGHT:
                    printf("\x1b[37mN\x1b[0m");
                    break;
                case W_BISHOP:
                    printf("\x1b[37mB\x1b[0m");
                    break;
                case W_ROOK:
                    printf("\x1b[37mR\x1b[0m");
                    break;
                case W_QUEEN:
                    printf("\x1b[37mQ\x1b[0m");
                    break;
                case W_KING:
                    printf("\x1b[37mK\x1b[0m");
                    break;
                case B_PAWN:
                    printf("\x1b[90mP\x1b[0m");
                    break;
                case B_KNIGHT:
                    printf("\x1b[90mN\x1b[0m");
                    break;
                case B_BISHOP:
                    printf("\x1b[90mB\x1b[0m");
                    break;
                case B_ROOK:
                    printf("\x1b[90mR\x1b[0m");
                    break;
                case B_QUEEN:
                    printf("\x1b[90mQ\x1b[0m");
                    break;
                case B_KING:
                    printf("\x1b[90mK\x1b[0m");
                    break;
                case UNASSIGNED:
                    printf("-");
                    break;
                default:
                    return SCE_INVALID_BOARD_STATE;
            }
            printf(" ");
            //printf("%d", piece_in_square);
        }

        // Print rank
        if (color == WHITE) {
            printf("%d ", 8 - i);
        } else {
            printf("%d ", i + 1);
        }
        printf("\n");
    }

    return SCE_SUCCESS;
}

SCE_Return SCE_PieceMovementPrecompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_precomputation_tbl == NULL) return SCE_INVALID_PARAM;

    // Empty the table.
    memset(ptr_precomputation_tbl, 0, sizeof(SCE_PieceMovementPrecomputationTable));

    // Precomputation: Knight
    RETURN_IF_SCE_FAILURE(SCE_Knight_Precompute(ptr_precomputation_tbl), "Knight moves table generation failed!");
    RETURN_IF_SCE_FAILURE(SCE_King_Precompute(ptr_precomputation_tbl), "King moves table generation failed!");
    RETURN_IF_SCE_FAILURE(SCE_Pawn_Precompute(ptr_precomputation_tbl), "Pawn moves/attacks table generation failed!");
    RETURN_IF_SCE_FAILURE(SCE_Rays_Precompute(ptr_precomputation_tbl), "Pawn moves/attacks table generation failed!");
    RETURN_IF_SCE_FAILURE(SCE_CastlingMask_Precompute(ptr_precomputation_tbl), "Castling mask table generation failed!");

    return SCE_SUCCESS;
}

#define DOWN >> 8U
#define UP << 8U
#define LEFT >> 1U
#define RIGHT << 1U
static SCE_Return SCE_Knight_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_precomputation_tbl == NULL) return SCE_INVALID_PARAM;

    for (uint i = 0U; i < CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION; i++) {
        const uint64_t pos = 1ULL << i;
        // Right-down originated.
        const uint row = i / CHESSBOARD_DIMENSION;
        const uint col = i % CHESSBOARD_DIMENSION;
        uint64_t moves = 0ULL;

        // 8 cases
        // RRU
        // RRD
        // RUU
        // RDD
        // LLU
        // LLD
        // LUU
        // LDD

        // For each case, check if applicable. If so, xor to moves.

        // RRU
        if (col <= 5U && row <= 6U) {
            moves ^= (pos RIGHT RIGHT UP);
        }

        // RRD
        if (col <= 5U && row >= 1U) {
            moves ^= (pos RIGHT RIGHT DOWN);
        }

        // RUU
        if (col <= 6U && row <= 5U) {
            moves ^= (pos RIGHT UP UP);
        }

        // RDD
        if (col <= 6U && row >= 2U) {
            moves ^= (pos RIGHT DOWN DOWN);
        }

        // LLU
        if (col >= 2U && row <= 6U) {
            moves ^= (pos LEFT LEFT UP);
        }

        // LLD
        if (col >= 2U && row >= 1U) {
            moves ^= (pos LEFT LEFT DOWN);
        }

        // LUU
        if (col >= 1U && row <= 5U) {
            moves ^= (pos LEFT UP UP);
        }

        // LDD
        if (col >= 1U && row >= 2U) {
            moves ^= (pos LEFT DOWN DOWN);
        }

        ptr_precomputation_tbl->knight_moves[i] = moves;
    }

    return SCE_SUCCESS;
}

static SCE_Return SCE_King_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_precomputation_tbl == NULL) return SCE_INVALID_PARAM;

    for (uint i = 0U; i < CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION; i++) {
        const uint64_t pos = 1ULL << i;
        // Right-down originated.
        const uint row = i / CHESSBOARD_DIMENSION;
        const uint col = i % CHESSBOARD_DIMENSION;
        uint64_t moves = 0ULL;

        // 8 cases
        // U
        // D
        // L
        // R
        // RU
        // RD
        // LU
        // LD

        // For each case, check if applicable. If so, xor to moves.

        // U
        if (row <= 6U) {
            moves ^= (pos UP);
        }

        // D
        if (row >= 1U) {
            moves ^= (pos DOWN);
        }

        // L
        if (col >= 1U) {
            moves ^= (pos LEFT);
        }

        // R
        if (col <= 6U) {
            moves ^= (pos RIGHT);
        }

        // RU
        if (col <= 6U && row <= 6U) {
            moves ^= (pos RIGHT UP);
        }

        // RD
        if (col <= 6U && row >= 1U) {
            moves ^= (pos RIGHT DOWN);
        }

        // LU
        if (col >= 1U && row <= 6U) {
            moves ^= (pos LEFT UP);
        }

        // LD
        if (col >= 1U && row >= 1U) {
            moves ^= (pos LEFT DOWN);
        }


        ptr_precomputation_tbl->king_moves[i] = moves;
    }

    return SCE_SUCCESS;
}

static SCE_Return SCE_Pawn_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_precomputation_tbl == NULL) return SCE_INVALID_PARAM;

    for (uint i = 0U; i < CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION; i++) {
        const uint64_t pos = 1ULL << i;
        // Right-down originated.
        const uint row = i / CHESSBOARD_DIMENSION;
        const uint col = i % CHESSBOARD_DIMENSION;
        uint64_t w_moves = 0ULL;
        uint64_t w_attacks = 0ULL;
        uint64_t b_moves = 0ULL;
        uint64_t b_attacks = 0ULL;

        // 2 cases for white moves
        // U
        // UU: Starting

        // 2 cases for black moves
        // D
        // DD: Starting

        // 2 cases for white attacks
        // LU
        // RU

        // 2 cases for black attacks
        // LD
        // RD

        // For each case, check if applicable. If so, xor to moves.


        // MOVES
        // U
        if (row <= 6U) {
            w_moves ^= (pos UP);
        }

        // UU
        if (row == 1U) {
            w_moves ^= (pos UP UP);
        }

        // D
        if (row >= 1U) {
            b_moves ^= (pos DOWN);
        }

        // DD
        if (row == 6U) {
            b_moves ^= (pos DOWN DOWN);
        }

        // ATTACKS
        // LU
        if (col >= 1U && row <= 6U) {
            w_attacks ^= (pos LEFT UP);
        }

        // RU
        if (col <= 6U && row <= 6U) {
            w_attacks ^= (pos RIGHT UP);
        }

        // LD
        if (col >= 1U && row >= 1U) {
            b_attacks ^= (pos LEFT DOWN);
        }

        // RD
        if (col <= 6U && row >= 1U) {
            b_attacks ^= (pos RIGHT DOWN);
        }

        ptr_precomputation_tbl->pawn_moves[WHITE][i] = w_moves;
        ptr_precomputation_tbl->pawn_moves[BLACK][i] = b_moves;
        ptr_precomputation_tbl->pawn_attacks[WHITE][i] = w_attacks;
        ptr_precomputation_tbl->pawn_attacks[BLACK][i] = b_attacks;
    }

    return SCE_SUCCESS;

}

static SCE_Return SCE_Rays_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_precomputation_tbl == NULL) return SCE_INVALID_PARAM;

    for (uint i = 0U; i < CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION; i++) {
        const uint64_t pos = 1ULL << i;
        // Right-down originated.
        const uint row = i / CHESSBOARD_DIMENSION;
        const uint col = i % CHESSBOARD_DIMENSION;
        uint64_t n_ray = 0ULL;
        uint64_t e_ray = 0ULL;
        uint64_t s_ray = 0ULL;
        uint64_t w_ray = 0ULL;
        uint64_t ne_ray = 0ULL;
        uint64_t nw_ray = 0ULL;
        uint64_t se_ray = 0ULL;
        uint64_t sw_ray = 0ULL;

        // 8 cases
        // N
        // E
        // S
        // W
        // NE
        // NW
        // SE
        // SW

        // For each case, check if applicable. If so, xor to moves.

        // NORTH
        uint shift = 1U;
        while (row + shift < CHESSBOARD_DIMENSION) {
            n_ray ^= (pos UP * shift);
            shift++;
        }

        // EAST
        shift = 1U;
        while (col + shift < CHESSBOARD_DIMENSION) {
            e_ray ^= (pos RIGHT * shift);
            shift++;
        }

        // SOUTH
        shift = 1U;
        while (row >= shift) {
            s_ray ^= (pos DOWN * shift);
            shift++;
        }

        // WEST
        shift = 1U;
        while (col >= shift) {
            w_ray ^= (pos LEFT * shift);
            shift++;
        }

        // NORTHEAST
        shift = 1U;
        while (row + shift < CHESSBOARD_DIMENSION && col + shift < CHESSBOARD_DIMENSION) {
            ne_ray ^= ((pos UP * shift) RIGHT * shift);
            shift++;
        }

        // NORTHWEST
        shift = 1U;
        while (row + shift < CHESSBOARD_DIMENSION && col >= shift) {
            nw_ray ^= ((pos UP * shift) LEFT * shift);
            shift++;
        }

        // SOUTHEAST
        shift = 1U;
        while (row >= shift && col + shift < CHESSBOARD_DIMENSION) {
            se_ray ^= ((pos DOWN * shift) RIGHT * shift);
            shift++;
        }

        // SOUTHWEST
        shift = 1U;
        while (row >= shift && col >= shift) {
            sw_ray ^= ((pos DOWN * shift) LEFT * shift);
            shift++;
        }

        ptr_precomputation_tbl->rays[NORTH][i] = n_ray;
        ptr_precomputation_tbl->rays[EAST][i] = e_ray;
        ptr_precomputation_tbl->rays[SOUTH][i] = s_ray;
        ptr_precomputation_tbl->rays[WEST][i] = w_ray;
        ptr_precomputation_tbl->rays[NORTHEAST][i] = ne_ray;
        ptr_precomputation_tbl->rays[NORTHWEST][i] = nw_ray;
        ptr_precomputation_tbl->rays[SOUTHEAST][i] = se_ray;
        ptr_precomputation_tbl->rays[SOUTHWEST][i] = sw_ray;
    }

    return SCE_SUCCESS;
}

static SCE_Return SCE_CastlingMask_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_precomputation_tbl == NULL) return SCE_INVALID_PARAM;

    for (uint i = 0U; i < CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION; i++) {
        ptr_precomputation_tbl->castling_mask[i] = 15U;
    }

    ptr_precomputation_tbl->castling_mask[SCE_AN_To_Idx("A1")] &= ~SCE_CASTLING_RIGHTS_WQ;
    ptr_precomputation_tbl->castling_mask[SCE_AN_To_Idx("H1")] &= ~SCE_CASTLING_RIGHTS_WK;
    ptr_precomputation_tbl->castling_mask[SCE_AN_To_Idx("E1")] &= ~(SCE_CASTLING_RIGHTS_WK | SCE_CASTLING_RIGHTS_WQ);
    ptr_precomputation_tbl->castling_mask[SCE_AN_To_Idx("A8")] &= ~SCE_CASTLING_RIGHTS_BQ;
    ptr_precomputation_tbl->castling_mask[SCE_AN_To_Idx("H8")] &= ~SCE_CASTLING_RIGHTS_BK;
    ptr_precomputation_tbl->castling_mask[SCE_AN_To_Idx("E8")] &= ~(SCE_CASTLING_RIGHTS_BK | SCE_CASTLING_RIGHTS_BQ);


    return SCE_SUCCESS;
}

static SCE_Return SCE_AddToMoveList(const SCE_ChessMove move, SCE_ChessMoveList* const ptr_movelist) {
    if (ptr_movelist == NULL) return SCE_INVALID_PARAM;
    if (ptr_movelist->count == N_MAX_MOVES - 1U) { 
        fprintf(stderr, "Adding to move list failure: MoveList full.\n");
        return SCE_INTERNAL_ERROR;
    }

    {
        // Check if src and dst are the same.
        uint src_idx = move SCE_CHESSMOVE_GET_SRC;
        uint dst_idx = move SCE_CHESSMOVE_GET_DST;

        if (src_idx == dst_idx) return SCE_INVALID_MOVE;
        if (src_idx >= 64 || dst_idx >= 64) return SCE_INVALID_MOVE;
    }

    ptr_movelist->moves[ptr_movelist->count] = move;
    ptr_movelist->count++;

    return SCE_SUCCESS;
}

// Generate moves that the piece can physically move to without pins or checks.
SCE_Return SCE_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_movelist == NULL || ptr_board == NULL || ptr_precomputation_tbl == NULL) return SCE_INVALID_PARAM;

    // 1. Generate pseudolegal moves for knights
    RETURN_IF_SCE_FAILURE(SCE_Knight_GeneratePseudoLegalMoves(ptr_movelist, ptr_board, ptr_precomputation_tbl), "Knight (pseudolegal) move generation failed");

    // 2. Generate pseudolegal moves for kings
    RETURN_IF_SCE_FAILURE(SCE_King_GeneratePseudoLegalMoves(ptr_movelist, ptr_board, ptr_precomputation_tbl), "King (pseudolegal) move generation failed");

    // 3. Generate pseudolegal moves for sliders (bishop, rook, queen)
    RETURN_IF_SCE_FAILURE(SCE_Slider_GeneratePseudoLegalMoves(ptr_movelist, ptr_board, ptr_precomputation_tbl), "Slider (pseudolegal) move generation failed");

    // 4. Generate pseudolegal moves for pawns
    RETURN_IF_SCE_FAILURE(SCE_Pawn_GeneratePseudoLegalMoves(ptr_movelist, ptr_board, ptr_precomputation_tbl), "Pawn (pseudolegal) move generation failed");

    return SCE_SUCCESS;
}

static SCE_Return SCE_Knight_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_movelist == NULL || ptr_board == NULL || ptr_precomputation_tbl == NULL) return SCE_INVALID_PARAM;

    const uint64_t occupancy_w = SCE_Chessboard_Occupancy_Color(ptr_board, WHITE);
    const uint64_t occupancy_b = SCE_Chessboard_Occupancy_Color(ptr_board, BLACK);
    const uint moving_piece_type = ptr_board->to_move == WHITE ? W_KNIGHT : B_KNIGHT;

    // Get all knights
    uint64_t knights = ptr_board->bitboards[moving_piece_type];
    while (knights) {
        // Loop and generate moves for each knight. After generating move for a knight, remove the bit.
        uint knight_idx_src = COUNT_TRAILING_ZEROS(knights);
        // Knight moves, but cannot attack the same color
        uint64_t knight_moves = (ptr_precomputation_tbl->knight_moves[knight_idx_src] & ~(SCE_Chessboard_Occupancy_Color(ptr_board, moving_piece_type == W_KNIGHT ? WHITE : BLACK)));
        
        while (knight_moves) {
            // For each moves, add to list.
            uint knight_idx_dst = COUNT_TRAILING_ZEROS(knight_moves);
            uint64_t knight_dst = 1ULL << knight_idx_dst;
            const SCE_ChessMove move = (knight_idx_src SCE_CHESSMOVE_SET_SRC) ^ (knight_idx_dst SCE_CHESSMOVE_SET_DST);
            if (knight_dst & (moving_piece_type == W_KNIGHT ? occupancy_b : occupancy_w)) {
                SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist);
            } else {
                SCE_AddToMoveList(move, ptr_movelist);
            }

            // Remove from the knight_moves.
            knight_moves &= ~(1ULL << knight_idx_dst);
        }

        // Remove from the knights
        knights &= ~(1ULL << knight_idx_src);
    }

    return SCE_SUCCESS;
}

static SCE_Return SCE_King_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_movelist == NULL || ptr_board == NULL || ptr_precomputation_tbl == NULL) return SCE_INVALID_PARAM;

    const uint64_t occupancy = SCE_Chessboard_Occupancy(ptr_board);
    const uint64_t occupancy_w = SCE_Chessboard_Occupancy_Color(ptr_board, WHITE);
    const uint64_t occupancy_b = SCE_Chessboard_Occupancy_Color(ptr_board, BLACK);
    const uint moving_piece_type = ptr_board->to_move == WHITE ? W_KING : B_KING;

    // Get king
    uint64_t king = ptr_board->bitboards[moving_piece_type];
    if (king) {
        // Loop and generate moves for the king.
        uint king_idx_src = COUNT_TRAILING_ZEROS(king);
        // King moves, but cannot attack the same color
        uint64_t king_moves = (ptr_precomputation_tbl->king_moves[king_idx_src] & ~(SCE_Chessboard_Occupancy_Color(ptr_board, moving_piece_type == W_KING ? WHITE : BLACK)));
        
        while (king_moves) {
            // For each moves, add to list.
            uint king_idx_dst = COUNT_TRAILING_ZEROS(king_moves);
            uint64_t king_dst = 1ULL << king_idx_dst;
            const SCE_ChessMove move = (king_idx_src SCE_CHESSMOVE_SET_SRC) ^ (king_idx_dst SCE_CHESSMOVE_SET_DST);
            if (king_dst & (moving_piece_type == W_KING ? occupancy_b : occupancy_w)) {
                SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist);
            } else {
                SCE_AddToMoveList(move, ptr_movelist);
            }

            // Remove from the king_moves.
            king_moves &= ~(1ULL << king_idx_dst);
        }


        // 00000110 (from A to H) = 0b01100000
        uint64_t king_side_gap_mask = 0x60ULL;
        // 01110000 (from A to H) = 0b1110
        uint64_t queen_side_gap_mask = 0x0EULL;
        // Castling
        if (moving_piece_type == W_KING) {
            // White king
            // King-side
            if (ptr_board->castling_rights & SCE_CASTLING_RIGHTS_WK) {
                // Check for gap.
                if (!(occupancy & king_side_gap_mask)) {
                    const uint king_idx_src = COUNT_TRAILING_ZEROS(KING_INITIAL_ROW);
                    const uint king_idx_dst = king_idx_src + 2U;
                    const SCE_ChessMove move = (king_idx_src SCE_CHESSMOVE_SET_SRC) | (king_idx_dst SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_KING_CASTLE SCE_CHESSMOVE_SET_FLAG);
                    SCE_AddToMoveList(move, ptr_movelist);
                }
            }

            // Queen-side
            if (ptr_board->castling_rights & SCE_CASTLING_RIGHTS_WQ) {
                // Check for gap.
                if (!(occupancy & queen_side_gap_mask)) {
                    const uint king_idx_src = COUNT_TRAILING_ZEROS(KING_INITIAL_ROW);
                    const uint king_idx_dst = king_idx_src - 2U;
                    const SCE_ChessMove move = (king_idx_src SCE_CHESSMOVE_SET_SRC) | (king_idx_dst SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_QUEEN_CASTLE SCE_CHESSMOVE_SET_FLAG);
                    SCE_AddToMoveList(move, ptr_movelist);
                }
            }
            
        } else {
            // Black king
            // King-side
            if (ptr_board->castling_rights & SCE_CASTLING_RIGHTS_BK) {
                // Check for gap.
                if (!(occupancy & (king_side_gap_mask UP * 7))) {
                    const uint king_idx_src = COUNT_TRAILING_ZEROS(KING_INITIAL_ROW UP * 7);
                    const uint king_idx_dst = king_idx_src + 2U;
                    const SCE_ChessMove move = (king_idx_src SCE_CHESSMOVE_SET_SRC) | (king_idx_dst SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_KING_CASTLE SCE_CHESSMOVE_SET_FLAG);
                    SCE_AddToMoveList(move, ptr_movelist);
                }
            }

            // Queen-side
            if (ptr_board->castling_rights & SCE_CASTLING_RIGHTS_BQ) {
                // Check for gap.
                if (!(occupancy & (queen_side_gap_mask UP * 7))) {
                    const uint king_idx_src = COUNT_TRAILING_ZEROS(KING_INITIAL_ROW UP * 7);
                    const uint king_idx_dst = king_idx_src - 2U;
                    const SCE_ChessMove move = (king_idx_src SCE_CHESSMOVE_SET_SRC) | (king_idx_dst SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_QUEEN_CASTLE SCE_CHESSMOVE_SET_FLAG);
                    SCE_AddToMoveList(move, ptr_movelist);
                }
            }
        }
    }

    return SCE_SUCCESS;
}

static SCE_Return SCE_Slider_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_movelist == NULL || ptr_board == NULL || ptr_precomputation_tbl == NULL) return SCE_INVALID_PARAM;

    const uint64_t occupancy = SCE_Chessboard_Occupancy(ptr_board);
    const uint64_t occupancy_w = SCE_Chessboard_Occupancy_Color(ptr_board, WHITE);
    const uint64_t occupancy_b = SCE_Chessboard_Occupancy_Color(ptr_board, BLACK);
    uint piece_types[3U] = { 0 };

    if (ptr_board->to_move == WHITE) {
        piece_types[0] = W_ROOK;
        piece_types[1] = W_BISHOP;
        piece_types[2] = W_QUEEN;
    } else {
        piece_types[0] = B_ROOK;
        piece_types[1] = B_BISHOP;
        piece_types[2] = B_QUEEN;
    }

    for (uint i = 0U; i < sizeof(piece_types)/sizeof(piece_types[0]); i++) {
        const uint moving_piece_type = piece_types[i];
        const PieceColor moving_piece_color = (moving_piece_type >= W_PAWN && moving_piece_type <= W_KING) ? WHITE : BLACK;

        uint64_t pieces = ptr_board->bitboards[moving_piece_type];
        while (pieces) {
            // Loop and generate moves for each piece. After generating move for a knight, remove the bit.
            uint piece_idx_src = COUNT_TRAILING_ZEROS(pieces);
            uint piece_row = piece_idx_src / CHESSBOARD_DIMENSION;
            uint piece_col = piece_idx_src % CHESSBOARD_DIMENSION;
            const uint64_t blockers[] = {
                ptr_precomputation_tbl->rays[NORTH][piece_idx_src] & occupancy,
                ptr_precomputation_tbl->rays[EAST][piece_idx_src] & occupancy,
                ptr_precomputation_tbl->rays[SOUTH][piece_idx_src] & occupancy,
                ptr_precomputation_tbl->rays[WEST][piece_idx_src] & occupancy,
                ptr_precomputation_tbl->rays[NORTHEAST][piece_idx_src] & occupancy,
                ptr_precomputation_tbl->rays[NORTHWEST][piece_idx_src] & occupancy,
                ptr_precomputation_tbl->rays[SOUTHEAST][piece_idx_src] & occupancy,
                ptr_precomputation_tbl->rays[SOUTHWEST][piece_idx_src] & occupancy
            };
            const uint blockers_idx[] = {
                blockers[NORTH] ? COUNT_TRAILING_ZEROS(blockers[NORTH]) : 0U,
                blockers[EAST] ? COUNT_TRAILING_ZEROS(blockers[EAST]) : 0U,
                blockers[SOUTH] ? (63U - COUNT_LEADING_ZEROS(blockers[SOUTH])) : 0U,
                blockers[WEST] ? (63U - COUNT_LEADING_ZEROS(blockers[WEST])) : 0U,
                blockers[NORTHEAST] ? COUNT_TRAILING_ZEROS(blockers[NORTHEAST]) : 0U,
                blockers[NORTHWEST] ? COUNT_TRAILING_ZEROS(blockers[NORTHWEST]) : 0U,
                blockers[SOUTHEAST] ? (63U - COUNT_LEADING_ZEROS(blockers[SOUTHEAST])) : 0U,
                blockers[SOUTHWEST] ? (63U - COUNT_LEADING_ZEROS(blockers[SOUTHWEST])) : 0U
            };

            // If there is a blocker, filter depending on what color the blocker is.
            // If there isn't a blocker, the entire ray is pseudolegal.

            uint max_shifts[8U] = { 0 };

            // Horizontal Vertical Check (Rook-like)
            switch (moving_piece_type) {
                case W_ROOK:
                case B_ROOK:
                case W_QUEEN:
                case B_QUEEN:
                    if (blockers_idx[NORTH]) {
                        uint blocker_idx = blockers_idx[NORTH];
                        uint blocker_row = blocker_idx / CHESSBOARD_DIMENSION;
                        // Check color
                        if ((1ULL << blocker_idx) & (moving_piece_color == WHITE ? occupancy_w : occupancy_b)) {
                            // Blocker is the same color as moving piece.
                            max_shifts[NORTH] = blocker_row - piece_row - 1U;
                        } else {
                            // Blocker is enemy. Can be captured.
                            max_shifts[NORTH] = blocker_row - piece_row;
                        }
                    } else {
                        max_shifts[NORTH] = (CHESSBOARD_DIMENSION - 1U - piece_row);
                    }

                    if (blockers_idx[EAST]) {
                        uint blocker_idx = blockers_idx[EAST];
                        uint blocker_col = blocker_idx % CHESSBOARD_DIMENSION;
                        // Check color
                        if ((1ULL << blocker_idx) & (moving_piece_color == WHITE ? occupancy_w : occupancy_b)) {
                            // Blocker is the same color as moving piece.
                            max_shifts[EAST] = blocker_col - piece_col - 1U;
                        } else {
                            // Blocker is enemy. Can be captured.
                            max_shifts[EAST] = blocker_col - piece_col;
                        }
                    } else {
                        max_shifts[EAST] = (CHESSBOARD_DIMENSION - 1U - piece_col);
                    }

                    if (blockers_idx[SOUTH]) {
                        uint blocker_idx = blockers_idx[SOUTH];
                        uint blocker_row = blocker_idx / CHESSBOARD_DIMENSION;
                        // Check color
                        if ((1ULL << blocker_idx) & (moving_piece_color == WHITE ? occupancy_w : occupancy_b)) {
                            // Blocker is the same color as moving piece.
                            max_shifts[SOUTH] = piece_row - blocker_row - 1U;
                        } else {
                            // Blocker is enemy. Can be captured.
                            max_shifts[SOUTH] = piece_row - blocker_row;
                        }
                    } else {
                        max_shifts[SOUTH] = piece_row;
                    }

                    if (blockers_idx[WEST]) {
                        uint blocker_idx = blockers_idx[WEST];
                        uint blocker_col = blocker_idx % CHESSBOARD_DIMENSION;
                        // Check color
                        if ((1ULL << blocker_idx) & (moving_piece_color == WHITE ? occupancy_w : occupancy_b)) {
                            // Blocker is the same color as moving piece.
                            max_shifts[WEST] = piece_col - blocker_col - 1U;
                        } else {
                            // Blocker is enemy. Can be captured.
                            max_shifts[WEST] = piece_col - blocker_col;
                        }
                    } else {
                        max_shifts[WEST] = piece_col;
                    }

                    break;
                default:
                    break;
            }

            // Diagonal Check (Bishop-like)
            switch (moving_piece_type) {
                case W_BISHOP:
                case B_BISHOP:
                case W_QUEEN:
                case B_QUEEN:
                    if (blockers_idx[NORTHEAST]) {
                        uint blocker_idx = blockers_idx[NORTHEAST];
                        uint blocker_row = blocker_idx / CHESSBOARD_DIMENSION;
                        uint blocker_col = blocker_idx % CHESSBOARD_DIMENSION;
                        // Check color
                        if ((1ULL << blocker_idx) & (moving_piece_color == WHITE ? occupancy_w : occupancy_b)) {
                            // Blocker is the same color as moving piece.
                            max_shifts[NORTHEAST] = blocker_col - piece_col - 1U;
                        } else {
                            // Blocker is enemy. Can be captured.
                            max_shifts[NORTHEAST] = blocker_col - piece_col;
                        }
                    } else {
                        max_shifts[NORTHEAST] = MIN(CHESSBOARD_DIMENSION - piece_row, CHESSBOARD_DIMENSION - piece_col) - 1U;
                    }

                    if (blockers_idx[NORTHWEST]) {
                        uint blocker_idx = blockers_idx[NORTHWEST];
                        uint blocker_row = blocker_idx / CHESSBOARD_DIMENSION;
                        uint blocker_col = blocker_idx % CHESSBOARD_DIMENSION;
                        // Check color
                        if ((1ULL << blocker_idx) & (moving_piece_color == WHITE ? occupancy_w : occupancy_b)) {
                            // Blocker is the same color as moving piece.
                            max_shifts[NORTHWEST] = piece_col - blocker_col - 1U;
                        } else {
                            // Blocker is enemy. Can be captured.
                            max_shifts[NORTHWEST] = piece_col - blocker_col;
                        }
                    } else {
                        max_shifts[NORTHWEST] = MIN(CHESSBOARD_DIMENSION - piece_row - 1U, piece_col);
                    }

                    if (blockers_idx[SOUTHEAST]) {
                        uint blocker_idx = blockers_idx[SOUTHEAST];
                        uint blocker_row = blocker_idx / CHESSBOARD_DIMENSION;
                        uint blocker_col = blocker_idx % CHESSBOARD_DIMENSION;
                        // Check color
                        if ((1ULL << blocker_idx) & (moving_piece_color == WHITE ? occupancy_w : occupancy_b)) {
                            // Blocker is the same color as moving piece.
                            max_shifts[SOUTHEAST] = blocker_col - piece_col - 1U;
                        } else {
                            // Blocker is enemy. Can be captured.
                            max_shifts[SOUTHEAST] = blocker_col - piece_col;
                        }
                    } else {
                        max_shifts[SOUTHEAST] = MIN(piece_row, CHESSBOARD_DIMENSION - piece_col - 1U);
                    }

                    if (blockers_idx[SOUTHWEST]) {
                        uint blocker_idx = blockers_idx[SOUTHWEST];
                        uint blocker_row = blocker_idx / CHESSBOARD_DIMENSION;
                        uint blocker_col = blocker_idx % CHESSBOARD_DIMENSION;
                        // Check color
                        if ((1ULL << blocker_idx) & (moving_piece_color == WHITE ? occupancy_w : occupancy_b)) {
                            // Blocker is the same color as moving piece.
                            max_shifts[SOUTHWEST] = piece_col - blocker_col - 1U;
                        } else {
                            // Blocker is enemy. Can be captured.
                            max_shifts[SOUTHWEST] = piece_col - blocker_col;
                        }
                    } else {
                        max_shifts[SOUTHWEST] = MIN(piece_row, piece_col);
                    }

                    break;
                default:
                    break;
            }

            // Based on max_shifts, add moves.
            for (RayDirection direction = NORTH; direction <= SOUTHWEST; direction++) {
                switch (direction) {
                    case NORTH:
                        for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                            const uint piece_idx_dst = (piece_idx_src + CHESSBOARD_DIMENSION * shift);
                            const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                            if ((1ULL << piece_idx_dst) & (moving_piece_color == WHITE ? occupancy_b : occupancy_w)) {
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                            } else {
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                            }
                        }
                        break;
                    case EAST:
                        for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                            const uint piece_idx_dst = (piece_idx_src + shift);
                            const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                            if ((1ULL << piece_idx_dst) & (moving_piece_color == WHITE ? occupancy_b : occupancy_w)) {
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                            } else {
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                            }
                        }
                        break;
                    case SOUTH:
                        for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                            const uint piece_idx_dst = (piece_idx_src - CHESSBOARD_DIMENSION * shift);
                            const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                            if ((1ULL << piece_idx_dst) & (moving_piece_color == WHITE ? occupancy_b : occupancy_w)) {
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                            } else {
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                            }
                        }
                        break;
                    case WEST:
                        for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                            const uint piece_idx_dst = (piece_idx_src - shift);
                            const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                            if ((1ULL << piece_idx_dst) & (moving_piece_color == WHITE ? occupancy_b : occupancy_w)) {
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                            } else {
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                            }
                        }
                        break;
                    case NORTHEAST:
                        for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                            const uint piece_idx_dst = (piece_idx_src + (CHESSBOARD_DIMENSION + 1U) * shift);
                            const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                            if ((1ULL << piece_idx_dst) & (moving_piece_color == WHITE ? occupancy_b : occupancy_w)) {
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                            } else {
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                            }
                        }
                        break;
                    case NORTHWEST:
                        for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                            const uint piece_idx_dst = (piece_idx_src + (CHESSBOARD_DIMENSION - 1U) * shift);
                            const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                            if ((1ULL << piece_idx_dst) & (moving_piece_color == WHITE ? occupancy_b : occupancy_w)) {
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                            } else {
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                            }
                        }
                        break;
                    case SOUTHEAST:
                        for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                            const uint piece_idx_dst = (piece_idx_src - (CHESSBOARD_DIMENSION - 1U) * shift);
                            const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                            if ((1ULL << piece_idx_dst) & (moving_piece_color == WHITE ? occupancy_b : occupancy_w)) {
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                            } else {
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                            }
                        }
                        break;
                    case SOUTHWEST:
                        for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                            const uint piece_idx_dst = (piece_idx_src - (CHESSBOARD_DIMENSION + 1U) * shift);
                            const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                            if ((1ULL << piece_idx_dst) & (moving_piece_color == WHITE ? occupancy_b : occupancy_w)) {
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                            } else {
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                            }
                        }
                        break;
                    default:
                        return false;
                }
            }

            // Remove piece
            pieces &= ~(1ULL << piece_idx_src);
        }
    }

    return SCE_SUCCESS;
}

static SCE_Return SCE_Pawn_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_movelist == NULL || ptr_board == NULL || ptr_precomputation_tbl == NULL) return SCE_INVALID_PARAM;

    // Four cases:
    // 1. Single Push
    // 2. Double Push (At rank 2, 7)
    // 3. Capture (Seriously why can't these guys capture ahead)

    const uint64_t occupancy = SCE_Chessboard_Occupancy(ptr_board);
    const uint64_t occupancy_w = SCE_Chessboard_Occupancy_Color(ptr_board, WHITE);
    const uint64_t occupancy_b = SCE_Chessboard_Occupancy_Color(ptr_board, BLACK);
    if (ptr_board->to_move == WHITE) {
        // White pawn
        // 1. Single Push
        uint64_t single_push = (ptr_board->bitboards[W_PAWN] UP) & ~occupancy;

        // 2. Double Push (from rank 2, 7); will be reusing single_push with bitmask for rank 3 and 6.
        const uint64_t filtered = single_push & (PAWN_INITIAL_ROW UP * 2U);
        uint64_t double_push = (filtered UP) & ~occupancy;

        // 3. Capture
        // 3.1. Capture EAST
        uint64_t capture_e = ((ptr_board->bitboards[W_PAWN] & ~H_MASK) UP RIGHT) & occupancy_b;
        // 3.2. Capture WEST
        uint64_t capture_w = ((ptr_board->bitboards[W_PAWN] & ~A_MASK) UP LEFT) & occupancy_b;

        // 1. Single Push
        while (single_push) {
            const uint pawn_idx_dst = COUNT_TRAILING_ZEROS(single_push);
            const uint64_t pawn_dst = 1ULL << pawn_idx_dst;

            const SCE_ChessMove move = ((pawn_idx_dst - CHESSBOARD_DIMENSION) SCE_CHESSMOVE_SET_SRC) ^ (pawn_idx_dst SCE_CHESSMOVE_SET_DST);
            if (pawn_idx_dst < CHESSBOARD_DIMENSION * 7U) {
                // Normal push
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add pawn move.");
            } else {
                // Promotion
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_KNIGHT_PROMOTION SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (knight promotion) move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_BISHOP_PROMOTION SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (bishop promotion) move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_ROOK_PROMOTION SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (rook promotion) move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_QUEEN_PROMOTION SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (queen promotion) move.");
            }

            single_push &= ~pawn_dst;
        }

        while (double_push) {
            const uint pawn_idx_dst = COUNT_TRAILING_ZEROS(double_push);
            const uint64_t pawn_dst = 1ULL << pawn_idx_dst;

            const SCE_ChessMove move = (((pawn_idx_dst - CHESSBOARD_DIMENSION * 2U) SCE_CHESSMOVE_SET_SRC) ^ (pawn_idx_dst SCE_CHESSMOVE_SET_DST)) | (SCE_CHESSMOVE_FLAG_DOUBLE_PAWN_PUSH SCE_CHESSMOVE_SET_FLAG);
            RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add (double) pawn move.");

            double_push &= ~pawn_dst;
        }

        while (capture_e) {
            const uint pawn_idx_dst = COUNT_TRAILING_ZEROS(capture_e);
            const uint64_t pawn_dst = 1ULL << pawn_idx_dst;

            const SCE_ChessMove move = (((pawn_idx_dst - 9U) SCE_CHESSMOVE_SET_SRC) ^ (pawn_idx_dst SCE_CHESSMOVE_SET_DST)) | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG);
            if (pawn_idx_dst < CHESSBOARD_DIMENSION * 7U) {
                // Normal capture
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add pawn capture move.");
            } else {
                // Promotion + Capture
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_KNIGHT_PROMO_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (knight promotion) capture move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_BISHOP_PROMO_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (bishop promotion) capture move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_ROOK_PROMO_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (rook promotion) capture move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_QUEEN_PROMO_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (queen promotion) capture move.");
            }

            capture_e &= ~pawn_dst;
        }

        while (capture_w) {
            const uint pawn_idx_dst = COUNT_TRAILING_ZEROS(capture_w);
            const uint64_t pawn_dst = 1ULL << pawn_idx_dst;

            const SCE_ChessMove move = (((pawn_idx_dst - 7U) SCE_CHESSMOVE_SET_SRC) ^ (pawn_idx_dst SCE_CHESSMOVE_SET_DST)) | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG);
            if (pawn_idx_dst < CHESSBOARD_DIMENSION * 7U) {
                // Normal capture
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add pawn capture move.");
            } else {
                // Promotion + Capture
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_KNIGHT_PROMO_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (knight promotion) capture move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_BISHOP_PROMO_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (bishop promotion) capture move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_ROOK_PROMO_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (rook promotion) capture move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_QUEEN_PROMO_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (queen promotion) capture move.");
            }

            capture_w &= ~pawn_dst;
        }

        // En passant
        {
            if (ptr_board->en_passant_idx != UNASSIGNED) {
                const uint64_t en_passant_square = (1ULL << (unsigned int) ptr_board->en_passant_idx);
                const uint64_t en_passant_attack_eligible = (ptr_board->bitboards[W_PAWN] & (PAWN_INITIAL_ROW UP * 4U));
                // East
                {
                    const uint64_t pawn_dst = ((en_passant_attack_eligible & ~H_MASK) UP RIGHT) & en_passant_square;
                    if (pawn_dst) {
                        const uint pawn_dst_idx = COUNT_TRAILING_ZEROS(pawn_dst);
                        const SCE_ChessMove move = ((pawn_dst_idx - 9U) SCE_CHESSMOVE_SET_SRC) ^ (pawn_dst_idx SCE_CHESSMOVE_SET_DST);
                        RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_EN_PASSANT_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn en-passant capture move.");
                    }
                }

                // West
                {
                    const uint64_t pawn_dst = ((en_passant_attack_eligible & ~A_MASK) UP LEFT) & en_passant_square;
                    if (pawn_dst) {
                        const uint pawn_dst_idx = COUNT_TRAILING_ZEROS(pawn_dst);
                        const SCE_ChessMove move = ((pawn_dst_idx - 7U) SCE_CHESSMOVE_SET_SRC) ^ (pawn_dst_idx SCE_CHESSMOVE_SET_DST);
                        RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_EN_PASSANT_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn en-passant capture move.");
                    }
                }
            }
        }
    } else {
        // Black pawn
        // 1. Single Push
        uint64_t single_push = (ptr_board->bitboards[B_PAWN] DOWN) & ~occupancy;

        // 2. Double Push (from rank 2, 7); will be reusing single_push with bitmask for rank 3 and 6.
        const uint64_t filtered = single_push & (PAWN_INITIAL_ROW UP * 5U);
        uint64_t double_push = (filtered DOWN) & ~occupancy;

        // 3. Capture
        // 3.1. Capture EAST
        uint64_t capture_e = ((ptr_board->bitboards[B_PAWN] & ~H_MASK) DOWN RIGHT) & occupancy_w;
        // 3.2. Capture WEST
        uint64_t capture_w = ((ptr_board->bitboards[B_PAWN] & ~A_MASK) DOWN LEFT) & occupancy_w;

        // 1. Single Push
        while (single_push) {
            const uint pawn_idx_dst = COUNT_TRAILING_ZEROS(single_push);
            const uint64_t pawn_dst = 1ULL << pawn_idx_dst;

            const SCE_ChessMove move = ((pawn_idx_dst + CHESSBOARD_DIMENSION) SCE_CHESSMOVE_SET_SRC) ^ (pawn_idx_dst SCE_CHESSMOVE_SET_DST);
            if (pawn_idx_dst >= CHESSBOARD_DIMENSION) {
                // Normal push
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add pawn move.");
            } else {
                // Promotion
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_KNIGHT_PROMOTION SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (knight promotion) move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_BISHOP_PROMOTION SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (bishop promotion) move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_ROOK_PROMOTION SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (rook promotion) move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_QUEEN_PROMOTION SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (queen promotion) move.");
            }

            single_push &= ~pawn_dst;
        }

        while (double_push) {
            const uint pawn_idx_dst = 63U - COUNT_LEADING_ZEROS(double_push);
            const uint64_t pawn_dst = 1ULL << pawn_idx_dst;

            const SCE_ChessMove move = (((pawn_idx_dst + CHESSBOARD_DIMENSION * 2U) SCE_CHESSMOVE_SET_SRC) ^ (pawn_idx_dst SCE_CHESSMOVE_SET_DST)) | (SCE_CHESSMOVE_FLAG_DOUBLE_PAWN_PUSH SCE_CHESSMOVE_SET_FLAG);
            RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add (double) pawn move.");

            double_push &= ~pawn_dst;
        }

        while (capture_e) {
            const uint pawn_idx_dst = 63U - COUNT_LEADING_ZEROS(capture_e);
            const uint64_t pawn_dst = 1ULL << pawn_idx_dst;

            const SCE_ChessMove move = (((pawn_idx_dst + 7U) SCE_CHESSMOVE_SET_SRC) ^ (pawn_idx_dst SCE_CHESSMOVE_SET_DST)) | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG);
            if (pawn_idx_dst >= CHESSBOARD_DIMENSION) {
                // Normal push
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add pawn move.");
            } else {
                // Promotion + Capture
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_KNIGHT_PROMO_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (knight promotion) capture move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_BISHOP_PROMO_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (bishop promotion) capture move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_ROOK_PROMO_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (rook promotion) capture move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_QUEEN_PROMO_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (queen promotion) capture move.");
            }

            capture_e &= ~pawn_dst;
        }

        while (capture_w) {
            const uint pawn_idx_dst = 63U - COUNT_LEADING_ZEROS(capture_w);
            const uint64_t pawn_dst = 1ULL << pawn_idx_dst;

            const SCE_ChessMove move = (((pawn_idx_dst + 9U) SCE_CHESSMOVE_SET_SRC) ^ (pawn_idx_dst SCE_CHESSMOVE_SET_DST)) | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG);
            if (pawn_idx_dst >= CHESSBOARD_DIMENSION) {
                // Normal push
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add pawn move.");
            } else {
                // Promotion + Capture
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_KNIGHT_PROMO_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (knight promotion) capture move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_BISHOP_PROMO_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (bishop promotion) capture move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_ROOK_PROMO_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (rook promotion) capture move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_QUEEN_PROMO_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (queen promotion) capture move.");
            }

            capture_w &= ~pawn_dst;
        }

        // En passant
        {
            if (ptr_board->en_passant_idx != UNASSIGNED) {
                const uint64_t en_passant_square = (1ULL << (unsigned int) ptr_board->en_passant_idx);
                const uint64_t en_passant_attack_eligible = (ptr_board->bitboards[B_PAWN] & (PAWN_INITIAL_ROW UP * 3U));
                // East
                {
                    const uint64_t pawn_dst = ((en_passant_attack_eligible & ~H_MASK) DOWN RIGHT) & en_passant_square;
                    if (pawn_dst) {
                        const uint pawn_dst_idx = COUNT_TRAILING_ZEROS(pawn_dst);
                        const SCE_ChessMove move = ((pawn_dst_idx + 7U) SCE_CHESSMOVE_SET_SRC) ^ (pawn_dst_idx SCE_CHESSMOVE_SET_DST);
                        RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_EN_PASSANT_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn en-passant capture move.");
                    }
                }

                // West
                {
                    const uint64_t pawn_dst = ((en_passant_attack_eligible & ~A_MASK) DOWN LEFT) & en_passant_square;
                    if (pawn_dst) {
                        const uint pawn_dst_idx = COUNT_TRAILING_ZEROS(pawn_dst);
                        const SCE_ChessMove move = ((pawn_dst_idx + 9U) SCE_CHESSMOVE_SET_SRC) ^ (pawn_dst_idx SCE_CHESSMOVE_SET_DST);
                        RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_EN_PASSANT_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn en-passant capture move.");
                    }
                }
            }
        }
    }

    return SCE_SUCCESS;
}

bool SCE_IsSquareAttacked(SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl, const uint64_t square, const PieceColor attacked_by) {
    if (ptr_board == NULL || ptr_precomputation_tbl == NULL || (attacked_by != WHITE && attacked_by != BLACK)) {
        fprintf(stderr, "\033[31m[-] Invalid parameter in SCE_IsSquareAttacked\033[0m\n");
        return false;
    }
    
    if (COUNT_SET_BITS(square) != 1) {
        fprintf(stderr, "\033[31m[-] Invalid parameter (square) in SCE_IsSquareAttacked\033[0m\n");
        return false;
    }

    const uint square_idx = COUNT_TRAILING_ZEROS(square);

    // Load attacker bitboards.
    const uint64_t attacker_pawns = attacked_by == WHITE ? ptr_board->bitboards[W_PAWN] : ptr_board->bitboards[B_PAWN];
    const uint64_t attacker_knights = attacked_by == WHITE ? ptr_board->bitboards[W_KNIGHT] : ptr_board->bitboards[B_KNIGHT];
    const uint64_t attacker_bishops = attacked_by == WHITE ? ptr_board->bitboards[W_BISHOP] : ptr_board->bitboards[B_BISHOP];
    const uint64_t attacker_rooks = attacked_by == WHITE ? ptr_board->bitboards[W_ROOK] : ptr_board->bitboards[B_ROOK];
    const uint64_t attacker_queen = attacked_by == WHITE ? ptr_board->bitboards[W_QUEEN] : ptr_board->bitboards[B_QUEEN];
    const uint64_t attacker_king = attacked_by == WHITE ? ptr_board->bitboards[W_KING] : ptr_board->bitboards[B_KING];

    // Reverse Lookup
    // 1. Knight: From the square, check if any attacker knight is reachable as a knight.
    // 2. Pawn: From the square, check if any attacker pawn is attackable. (Use victim table).
    // 3. King
    // 4. Sliders with rays.

    // 1. Knight
    if (ptr_precomputation_tbl->knight_moves[square_idx] & attacker_knights) {
        return true;
    }

    // 2. Pawn
    if (ptr_precomputation_tbl->pawn_attacks[attacked_by == WHITE ? BLACK : WHITE][square_idx] & attacker_pawns) {
        return true;
    }

    // 3. King
    if (ptr_precomputation_tbl->king_moves[square_idx] & attacker_king) {
        return true;
    }

    // 4. Sliders
    const uint64_t occupancy = SCE_Chessboard_Occupancy(ptr_board);

    for (RayDirection ray_direction = NORTH; ray_direction <= SOUTHWEST; ray_direction++) {
        int sign_of_direction = 0;
        switch (ray_direction) {
            case NORTH:
            case EAST:
            case NORTHWEST:
            case NORTHEAST:
               sign_of_direction = 1;
               break;
            case SOUTH:
            case WEST:
            case SOUTHEAST:
            case SOUTHWEST:
                sign_of_direction = -1;
                break;
            default:
                return false;
        }
        const uint64_t intersection = occupancy & ptr_precomputation_tbl->rays[ray_direction][square_idx];
        if (intersection) {
            const uint64_t blocker = 1ULL << ( sign_of_direction > 0 ? COUNT_TRAILING_ZEROS(intersection) : 63U - COUNT_LEADING_ZEROS(intersection));
            
            switch (ray_direction) {
                case NORTH:
                case SOUTH:
                case EAST:
                case WEST:
                    // Rook check
                    if (blocker & attacker_rooks) {
                        return true;
                    }
                    break;
                case NORTHEAST:
                case NORTHWEST:
                case SOUTHEAST:
                case SOUTHWEST:
                    // Bishop check
                    if (blocker & attacker_bishops) {
                        return true;
                    }
                    break;
            }
            // Queen check
            if (blocker & attacker_queen) {
                return true;
            }
        }
    }

    return false;
}

int SCE_AN_To_Idx(const char* an) {
    if (an == NULL || strlen(an) != 2) {
        fprintf(stderr, "\033[31m[-] Invalid parameter in converting from AN\033[0m\n");
        return -1;
    }

    const char file_char = an[0];
    const char rank_char = an[1];
    uint file_n;
    uint rank_n;

    if ('a' <= file_char && file_char <= 'h') {
        file_n = (uint) file_char - 'a';
    } else if ('A' <= file_char && file_char <= 'H') {
        file_n = (uint) file_char - 'A';
    } else {
        fprintf(stderr, "\033[31m[-] Invalid parameter (file) in converting from AN\033[0m\n");
        return -1;
    }

    if ('1' <= rank_char && rank_char <= '8') {
        rank_n = (uint) rank_char - '1';
    } else {
        fprintf(stderr, "\033[31m[-] Invalid parameter (rank) in converting from AN\033[0m\n");
        return -1;
    }

    return rank_n * 8 + file_n;
}

uint64_t SCE_AN_To_Bitboard(const char* an) {
    if (an == NULL || strlen(an) != 2) {
        fprintf(stderr, "\033[31m[-] Invalid parameter in SCE_AN_To_Bitboard\033[0m\n");
        return 0U;
    }

    int idx = SCE_AN_To_Idx(an);
    if (idx == -1) {
        fprintf(stderr, "\033[31m[-] Invalid parameter in SCE_AN_To_Bitboard\033[0m\n");
        return 0U;
    }

    return 1ULL << (uint) idx;
}

SCE_Return SCE_Bitboard_To_AN(char* const an_out, uint64_t bitboard) {
    if (an_out == NULL || COUNT_SET_BITS(bitboard) != 1) {
        return SCE_INVALID_PARAM;
    }

    const uint shift = COUNT_TRAILING_ZEROS(bitboard);  // such that bitboard = 1 << shift
    const uint row = shift / CHESSBOARD_DIMENSION;
    const uint col = shift % CHESSBOARD_DIMENSION;

    an_out[0] = 'A' + (char) col;
    an_out[1] = '1' + (char) row;
    an_out[2] = 0x00;

    return SCE_SUCCESS;
}

SCE_Return SCE_MakeMove(SCE_Chessboard* const ptr_board, SCE_PieceMovementPrecomputationTable* const ptr_precomputation_table, const SCE_ZobristTable* const ptr_table, const SCE_ChessMove move) {
    if (ptr_board == NULL || ptr_precomputation_table == NULL || ptr_table == NULL) return SCE_INVALID_PARAM;

    const uint src_idx = move SCE_CHESSMOVE_GET_SRC;
    const uint64_t src = (1ULL << src_idx);
    const uint dst_idx = move SCE_CHESSMOVE_GET_DST;
    const uint64_t dst = (1ULL << dst_idx);
    const uint flag = move SCE_CHESSMOVE_GET_FLAG;
    const uint64_t occupancy_w = SCE_Chessboard_Occupancy_Color(ptr_board, WHITE);
    const uint64_t occupancy_b = SCE_Chessboard_Occupancy_Color(ptr_board, BLACK);
    
    // Check if src piece is to move.
    if ((ptr_board->to_move == WHITE && !(src & occupancy_w)) || (ptr_board->to_move == BLACK && !(src & occupancy_b))) return SCE_INVALID_MOVE;

    // Check if dst piece is the same color as the moving piece. If so, this is not allowed.
    if ((ptr_board->to_move == WHITE && (dst & occupancy_w)) || (ptr_board->to_move == BLACK && (dst & occupancy_b))) return SCE_INVALID_MOVE;

    int moving_piece_type = UNASSIGNED;
    int captured_piece_type = UNASSIGNED;
    {
        // There is a guarantee that one of the piece types will be set here from the src check.
        for (uint piece_type = W_PAWN; piece_type <= B_KING; piece_type++) {
            // Determine moving piece type
            if (src & ptr_board->bitboards[piece_type]) {
                moving_piece_type = piece_type;
            }
            // Determine captured piece type
            if (dst & ptr_board->bitboards[piece_type]) {
                captured_piece_type = piece_type;
            }
        }

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
                if (dst & ptr_board->bitboards[piece_type]) {
                    captured_piece_type = piece_type;
                }
            }

        }

        // Sanity check
        if (moving_piece_type == UNASSIGNED) return SCE_INVALID_MOVE;
        if ((flag & SCE_CHESSMOVE_FLAG_CAPTURE) && (captured_piece_type == UNASSIGNED)) {
            // Flag said the piece would capture something, but it was a big fat lie!
            return SCE_INVALID_MOVE;
        }
    }

    {
        // Castling pre-journal check: If king is under attack, castling is not allowed.
        if ((flag == SCE_CHESSMOVE_FLAG_KING_CASTLE || flag == SCE_CHESSMOVE_FLAG_QUEEN_CASTLE) && (moving_piece_type == W_KING || moving_piece_type == B_KING)) {
            if (SCE_IsSquareAttacked(ptr_board, ptr_precomputation_table, src, ptr_board->to_move == WHITE ? BLACK : WHITE)) {
                return SCE_INVALID_MOVE;
            }
        }
    }

    {
        // Journalling
        ptr_board->undo_states[ptr_board->history.count].moving_piece = moving_piece_type;
        ptr_board->undo_states[ptr_board->history.count].captured_piece =  captured_piece_type;
        ptr_board->undo_states[ptr_board->history.count].en_passant_square = ptr_board->en_passant_idx;
        ptr_board->undo_states[ptr_board->history.count].castling_rights = ptr_board->castling_rights;
        ptr_board->undo_states[ptr_board->history.count].half_move_clock = ptr_board->half_move_clock;
        ptr_board->undo_states[ptr_board->history.count].zobrist_hash = ptr_board->zobrist_hash;
        // This automatically increments the count
        // TODO: Check for success.
        RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, &ptr_board->history), "Adding to list failed!");
    }

    const int old_en_passant_idx = ptr_board->en_passant_idx;
    {
        // Execution
        // 1. Capture
        // 2. Move
        // 3. Flag action
        if (flag & SCE_CHESSMOVE_FLAG_CAPTURE) {
            if (flag == SCE_CHESSMOVE_FLAG_EN_PASSANT_CAPTURE) {
                uint64_t captured_piece = ptr_board->to_move == WHITE ? (1ULL << (ptr_board->en_passant_idx - CHESSBOARD_DIMENSION)) : (1ULL << (ptr_board->en_passant_idx + CHESSBOARD_DIMENSION));
                ptr_board->bitboards[captured_piece_type] ^= captured_piece;

                // Zobrist: Captured piece
                ptr_board->zobrist_hash ^= ptr_table->piece_key[captured_piece_type][COUNT_TRAILING_ZEROS(captured_piece)];
            } else {
                ptr_board->bitboards[captured_piece_type] ^= dst;

                // Zobrist: Captured piece
                ptr_board->zobrist_hash ^= ptr_table->piece_key[captured_piece_type][dst_idx];
            }
        }

        // Standard move
        ptr_board->bitboards[moving_piece_type] ^= src | dst;

        {
            // Zobrist: Source piece move
            ptr_board->zobrist_hash ^= ptr_table->piece_key[moving_piece_type][src_idx];
            ptr_board->zobrist_hash ^= ptr_table->piece_key[moving_piece_type][dst_idx];
        }


        switch (flag) {
            // 1. Pawn Double Push: en passant square
            case SCE_CHESSMOVE_FLAG_DOUBLE_PAWN_PUSH:
                ptr_board->en_passant_idx = ptr_board->to_move == WHITE ? src_idx + CHESSBOARD_DIMENSION : src_idx - CHESSBOARD_DIMENSION;
                break;
            // 2. Promotion
            case SCE_CHESSMOVE_FLAG_KNIGHT_PROMOTION:
            case SCE_CHESSMOVE_FLAG_KNIGHT_PROMO_CAPTURE:
                ptr_board->bitboards[ptr_board->to_move == WHITE ? W_PAWN : B_PAWN] ^= dst;
                ptr_board->bitboards[ptr_board->to_move == WHITE ? W_KNIGHT : B_KNIGHT] ^= dst;

                ptr_board->zobrist_hash ^= ptr_table->piece_key[ptr_board->to_move == WHITE ? W_PAWN : B_PAWN][dst_idx];
                ptr_board->zobrist_hash ^= ptr_table->piece_key[ptr_board->to_move == WHITE ? W_KNIGHT : B_KNIGHT][dst_idx];
                break;
            case SCE_CHESSMOVE_FLAG_BISHOP_PROMOTION:
            case SCE_CHESSMOVE_FLAG_BISHOP_PROMO_CAPTURE:
                ptr_board->bitboards[ptr_board->to_move == WHITE ? W_PAWN : B_PAWN] ^= dst;
                ptr_board->bitboards[ptr_board->to_move == WHITE ? W_BISHOP : B_BISHOP] ^= dst;

                ptr_board->zobrist_hash ^= ptr_table->piece_key[ptr_board->to_move == WHITE ? W_PAWN : B_PAWN][dst_idx];
                ptr_board->zobrist_hash ^= ptr_table->piece_key[ptr_board->to_move == WHITE ? W_BISHOP : B_BISHOP][dst_idx];
                break;
            case SCE_CHESSMOVE_FLAG_ROOK_PROMOTION:
            case SCE_CHESSMOVE_FLAG_ROOK_PROMO_CAPTURE:
                ptr_board->bitboards[ptr_board->to_move == WHITE ? W_PAWN : B_PAWN] ^= dst;
                ptr_board->bitboards[ptr_board->to_move == WHITE ? W_ROOK : B_ROOK] ^= dst;

                ptr_board->zobrist_hash ^= ptr_table->piece_key[ptr_board->to_move == WHITE ? W_PAWN : B_PAWN][dst_idx];
                ptr_board->zobrist_hash ^= ptr_table->piece_key[ptr_board->to_move == WHITE ? W_ROOK : B_ROOK][dst_idx];
                break;
            case SCE_CHESSMOVE_FLAG_QUEEN_PROMOTION:
            case SCE_CHESSMOVE_FLAG_QUEEN_PROMO_CAPTURE:
                ptr_board->bitboards[ptr_board->to_move == WHITE ? W_PAWN : B_PAWN] ^= dst;
                ptr_board->bitboards[ptr_board->to_move == WHITE ? W_QUEEN : B_QUEEN] ^= dst;

                ptr_board->zobrist_hash ^= ptr_table->piece_key[ptr_board->to_move == WHITE ? W_PAWN : B_PAWN][dst_idx];
                ptr_board->zobrist_hash ^= ptr_table->piece_key[ptr_board->to_move == WHITE ? W_QUEEN : B_QUEEN][dst_idx];
                break;
            // 3. Castling
            case SCE_CHESSMOVE_FLAG_KING_CASTLE:
                {
                    const uint rook_idx_src = dst_idx + 1U;
                    const uint rook_idx_dst = dst_idx - 1U;
                    const uint64_t rook_src = (1ULL << rook_idx_src);
                    const uint64_t rook_dst = (1ULL << rook_idx_dst);
                    ptr_board->bitboards[ptr_board->to_move == WHITE ? W_ROOK : B_ROOK] ^= (rook_src ^ rook_dst);

                    ptr_board->zobrist_hash ^= ptr_table->piece_key[ptr_board->to_move == WHITE ? W_ROOK : B_ROOK][rook_idx_src];
                    ptr_board->zobrist_hash ^= ptr_table->piece_key[ptr_board->to_move == WHITE ? W_ROOK : B_ROOK][rook_idx_dst];
                }
                break;
            case SCE_CHESSMOVE_FLAG_QUEEN_CASTLE:
                {
                    const uint rook_idx_src = dst_idx - 2U;
                    const uint rook_idx_dst = dst_idx + 1U;
                    const uint64_t rook_src = (1ULL << rook_idx_src);
                    const uint64_t rook_dst = (1ULL << rook_idx_dst);
                    ptr_board->bitboards[ptr_board->to_move == WHITE ? W_ROOK : B_ROOK] ^= (rook_src ^ rook_dst);

                    ptr_board->zobrist_hash ^= ptr_table->piece_key[ptr_board->to_move == WHITE ? W_ROOK : B_ROOK][rook_idx_src];
                    ptr_board->zobrist_hash ^= ptr_table->piece_key[ptr_board->to_move == WHITE ? W_ROOK : B_ROOK][rook_idx_dst];
                }
                break;
            default:
                break;
        }
    }
    
    // Since successful, switch to_move
    ptr_board->to_move = ptr_board->to_move == WHITE ? BLACK : WHITE;
    
    // Zobrist: Side
    ptr_board->zobrist_hash ^= ptr_table->side_key;

    // For non double-push move, unset en passant square
    if (flag != SCE_CHESSMOVE_FLAG_DOUBLE_PAWN_PUSH) {
        ptr_board->en_passant_idx = UNASSIGNED;
    }

    // Zobrist: En passant
    if (old_en_passant_idx == UNASSIGNED) {
        ptr_board->zobrist_hash ^= ptr_table->en_passant_keys[SCE_ZOBRIST_EN_PASSANT_UNASSIGNED_KEY];
    } else {
        ptr_board->zobrist_hash ^= ptr_table->en_passant_keys[old_en_passant_idx % CHESSBOARD_DIMENSION];
    }
    if (ptr_board->en_passant_idx == UNASSIGNED) {
        ptr_board->zobrist_hash ^= ptr_table->en_passant_keys[SCE_ZOBRIST_EN_PASSANT_UNASSIGNED_KEY];
    } else {
        ptr_board->zobrist_hash ^= ptr_table->en_passant_keys[ptr_board->en_passant_idx % CHESSBOARD_DIMENSION];
    }


    // Update castling right
    const uint8_t old_castling_rights = ptr_board->castling_rights;
    ptr_board->castling_rights &= ptr_precomputation_table->castling_mask[src_idx] & ptr_precomputation_table->castling_mask[dst_idx];
    
    // Zobrist: Castling
    ptr_board->zobrist_hash ^= ptr_table->castling_keys[old_castling_rights];
    ptr_board->zobrist_hash ^= ptr_table->castling_keys[ptr_board->castling_rights];

    // Final Checks:
    // 1. Is previous king in check?
    const uint64_t prev_king_square = ptr_board->bitboards[ptr_board->to_move == WHITE ? B_KING : W_KING];
    if (SCE_IsSquareAttacked(ptr_board, ptr_precomputation_table, prev_king_square, ptr_board->to_move)) {
        RETURN_IF_SCE_FAILURE(SCE_UnmakeMove(ptr_board, ptr_precomputation_table), "King is in check, but could not unmake.");
        return SCE_INVALID_MOVE;
    }
    // 2. For castling, is through square under attack?
    if (flag == SCE_CHESSMOVE_FLAG_KING_CASTLE || flag == SCE_CHESSMOVE_FLAG_QUEEN_CASTLE) {
        const uint through_idx = flag == SCE_CHESSMOVE_FLAG_KING_CASTLE ? src_idx + 1U : src_idx - 1U;
        const uint64_t through = (1ULL << through_idx);
        if (SCE_IsSquareAttacked(ptr_board, ptr_precomputation_table, through, ptr_board->to_move)) {
            RETURN_IF_SCE_FAILURE(SCE_UnmakeMove(ptr_board, ptr_precomputation_table), "King is in check, but could not unmake.");
            return SCE_INVALID_MOVE;
        }
    }
    // 3. Update half-move clock
    if (moving_piece_type == W_PAWN || moving_piece_type == B_PAWN || captured_piece_type != UNASSIGNED) {
        ptr_board->half_move_clock = 0U;
    } else {
        ptr_board->half_move_clock++;
    }
    
    return SCE_SUCCESS;
}

SCE_Return SCE_UnmakeMove(SCE_Chessboard* const ptr_board, SCE_PieceMovementPrecomputationTable* const ptr_precomputation_table) {
    if (ptr_board == NULL || ptr_precomputation_table == NULL) return SCE_INVALID_PARAM;
    if (ptr_board->history.count == 0U) return SCE_MOVELIST_EMPTY;

    // Index to latest move/undo state
    const uint move_idx = ptr_board->history.count - 1U;
    const uint src_idx = ptr_board->history.moves[move_idx] SCE_CHESSMOVE_GET_SRC;
    const uint64_t src = 1ULL << src_idx;
    const uint dst_idx = ptr_board->history.moves[move_idx] SCE_CHESSMOVE_GET_DST;
    const uint64_t dst = 1ULL << dst_idx;
    const uint flag = ptr_board->history.moves[move_idx] SCE_CHESSMOVE_GET_FLAG;
    const uint moving_piece = ptr_board->undo_states[move_idx].moving_piece;
    const int captured_piece = ptr_board->undo_states[move_idx].captured_piece;

    ptr_board->en_passant_idx = ptr_board->undo_states[move_idx].en_passant_square;
    ptr_board->to_move = ptr_board->to_move == WHITE ? BLACK : WHITE;
    ptr_board->castling_rights = ptr_board->undo_states[move_idx].castling_rights;
    ptr_board->half_move_clock = ptr_board->undo_states[move_idx].half_move_clock;
    ptr_board->zobrist_hash = ptr_board->undo_states[move_idx].zobrist_hash;

    // Restoration
    // 1. Flag action
    // 2. Move
    // 3. Capture
    switch (flag) {
        // 1. Pawn Double Push: en passant square
        case SCE_CHESSMOVE_FLAG_DOUBLE_PAWN_PUSH:
            break;
        // 2. Promotion
        case SCE_CHESSMOVE_FLAG_KNIGHT_PROMOTION:
        case SCE_CHESSMOVE_FLAG_KNIGHT_PROMO_CAPTURE:
            ptr_board->bitboards[ptr_board->to_move == WHITE ? W_PAWN : B_PAWN] ^= dst;
            ptr_board->bitboards[ptr_board->to_move == WHITE ? W_KNIGHT : B_KNIGHT] ^= dst;
            break;
        case SCE_CHESSMOVE_FLAG_BISHOP_PROMOTION:
        case SCE_CHESSMOVE_FLAG_BISHOP_PROMO_CAPTURE:
            ptr_board->bitboards[ptr_board->to_move == WHITE ? W_PAWN : B_PAWN] ^= dst;
            ptr_board->bitboards[ptr_board->to_move == WHITE ? W_BISHOP : B_BISHOP] ^= dst;
            break;
        case SCE_CHESSMOVE_FLAG_ROOK_PROMOTION:
        case SCE_CHESSMOVE_FLAG_ROOK_PROMO_CAPTURE:
            ptr_board->bitboards[ptr_board->to_move == WHITE ? W_PAWN : B_PAWN] ^= dst;
            ptr_board->bitboards[ptr_board->to_move == WHITE ? W_ROOK : B_ROOK] ^= dst;
            break;
        case SCE_CHESSMOVE_FLAG_QUEEN_PROMOTION:
        case SCE_CHESSMOVE_FLAG_QUEEN_PROMO_CAPTURE:
            ptr_board->bitboards[ptr_board->to_move == WHITE ? W_PAWN : B_PAWN] ^= dst;
            ptr_board->bitboards[ptr_board->to_move == WHITE ? W_QUEEN : B_QUEEN] ^= dst;
            break;
        // 3. Castling
        case SCE_CHESSMOVE_FLAG_KING_CASTLE:
            {
                const uint rook_idx_src = dst_idx + 1U;
                const uint rook_idx_dst = dst_idx - 1U;
                const uint64_t rook_src = (1ULL << rook_idx_src);
                const uint64_t rook_dst = (1ULL << rook_idx_dst);
                ptr_board->bitboards[ptr_board->to_move == WHITE ? W_ROOK : B_ROOK] ^= (rook_src ^ rook_dst);
            }
            break;
        case SCE_CHESSMOVE_FLAG_QUEEN_CASTLE:
            {
                const uint rook_idx_src = dst_idx - 2U;
                const uint rook_idx_dst = dst_idx + 1U;
                const uint64_t rook_src = (1ULL << rook_idx_src);
                const uint64_t rook_dst = (1ULL << rook_idx_dst);
                ptr_board->bitboards[ptr_board->to_move == WHITE ? W_ROOK : B_ROOK] ^= (rook_src ^ rook_dst);
            }
            break;
        default:
            break;
    }

    
    // Standard undo-move
    ptr_board->bitboards[moving_piece] ^= src | dst;

    if (captured_piece != UNASSIGNED) {
        if (flag == SCE_CHESSMOVE_FLAG_EN_PASSANT_CAPTURE) {
            ptr_board->bitboards[captured_piece] ^= ptr_board->to_move == WHITE ? (1ULL << (dst_idx - CHESSBOARD_DIMENSION)) : (1ULL << (dst_idx + CHESSBOARD_DIMENSION));
        } else {
            ptr_board->bitboards[captured_piece] ^= dst;
        }
    }


    // Decrement move count at the end.
    ptr_board->history.count--;
    return SCE_SUCCESS;
}

SCE_Return SCE_GenerateLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Chessboard* const ptr_board, SCE_PieceMovementPrecomputationTable* const ptr_precomputation_table, const SCE_ZobristTable* const ptr_table) {
    if (ptr_movelist == NULL || ptr_board == NULL || ptr_precomputation_table == NULL || ptr_table == NULL) return SCE_INVALID_PARAM;
    if (ptr_movelist->count != 0) return SCE_INVALID_PARAM;

    SCE_ChessMoveList pseudolegal_moves;
    RETURN_IF_SCE_FAILURE(SCE_ChessMoveList_clear(&pseudolegal_moves), "Could not clear move list.");
    RETURN_IF_SCE_FAILURE(SCE_GeneratePseudoLegalMoves(&pseudolegal_moves, ptr_board, ptr_precomputation_table), "Could not generate pseudo legal movelist.");
    for (uint i = 0U; i < pseudolegal_moves.count; i++) {
        const SCE_Return ret = SCE_MakeMove(ptr_board, ptr_precomputation_table, ptr_table, pseudolegal_moves.moves[i]);
        if (ret == SCE_SUCCESS) {
            RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(pseudolegal_moves.moves[i], ptr_movelist), "Could not add move to legal move list.");
            RETURN_IF_SCE_FAILURE(SCE_UnmakeMove(ptr_board, ptr_precomputation_table), "Could not unmake!");
        }
    }

    return SCE_SUCCESS;
}
