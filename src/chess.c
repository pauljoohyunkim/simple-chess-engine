#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"

#define RETURN_IF_SCE_FAILURE(x, msg) do { if (!x) { fprintf(stderr, "%s\n", msg); return SCE_FAILURE; } } while (0);
#define UNASSIGNED (-1)

#define MIN(x, y) ((x) > (y) ? (y) : (x))

typedef unsigned int uint;


// Static functions for generating components of precomputation table.
static int SCE_Knight_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);
static int SCE_King_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);
static int SCE_Pawn_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);
static int SCE_Rays_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);
static int SCE_CastlingMask_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);

static int SCE_AddToMoveList(const SCE_ChessMove move, SCE_ChessMoveList* const ptr_movelist);

static int SCE_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);
static int SCE_Knight_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);
static int SCE_King_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);
static int SCE_Slider_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);
static int SCE_Pawn_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);

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

int SCE_ChessMoveList_clear(SCE_ChessMoveList* const ptr_list) {
    if (ptr_list == NULL) return SCE_FAILURE;

    memset(ptr_list, 0, sizeof(SCE_ChessMoveList));

    return SCE_SUCCESS;
}

int SCE_Chessboard_clear(SCE_Chessboard* const ptr_board) {
    if (ptr_board == NULL) return SCE_FAILURE;

    for (uint i = 0U; i < N_TYPES_PIECES; i++) {
        ptr_board->bitboards[i] = 0U;
    }

    ptr_board->en_passant_idx = UNASSIGNED;
    ptr_board->to_move = WHITE;
    RETURN_IF_SCE_FAILURE(SCE_ChessMoveList_clear(&ptr_board->moves), "Error when clearing chess move list");

    return SCE_SUCCESS;
}

int SCE_Chessboard_reset(SCE_Chessboard* const ptr_board) {
    if (ptr_board == NULL) return SCE_FAILURE;

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
    RETURN_IF_SCE_FAILURE(SCE_ChessMoveList_clear(&ptr_board->moves), "Error when clearing chess move list");

    return SCE_SUCCESS;
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

int SCE_Chessboard_print(SCE_Chessboard* const ptr_board, PieceColor color) {
    if (ptr_board == NULL) return SCE_FAILURE;
    if (color != WHITE && color != BLACK) return SCE_FAILURE;

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
                    if (piece_in_square != UNASSIGNED) return SCE_FAILURE;

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
                    return SCE_FAILURE;
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

int SCE_PieceMovementPrecompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_precomputation_tbl == NULL) return SCE_FAILURE;

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
static int SCE_Knight_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_precomputation_tbl == NULL) return SCE_FAILURE;

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

static int SCE_King_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_precomputation_tbl == NULL) return SCE_FAILURE;

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

static int SCE_Pawn_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_precomputation_tbl == NULL) return SCE_FAILURE;

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

static int SCE_Rays_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_precomputation_tbl == NULL) return SCE_FAILURE;

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

static int SCE_CastlingMask_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_precomputation_tbl == NULL) return SCE_FAILURE;

    for (uint i = 0U; i < CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION; i++) {
        ptr_precomputation_tbl->castling_mask[i] = 15U;
    }

    // TODO: Specify nontrivial values for rooks/kings
    //ptr_precomputation_tbl->castling_mask[7U] = 14U;
    //ptr_precomputation_tbl->castling_mask

    return SCE_SUCCESS;
}

static int SCE_AddToMoveList(const SCE_ChessMove move, SCE_ChessMoveList* const ptr_movelist) {
    if (ptr_movelist == NULL) return SCE_FAILURE;
    if (ptr_movelist->count == N_MAX_MOVES - 1U) { 
        fprintf(stderr, "Adding to move list failure: MoveList full.\n");
        return SCE_FAILURE;
    }

    // TODO: Validate move.
    ptr_movelist->moves[ptr_movelist->count] = move;
    ptr_movelist->count++;

    return SCE_SUCCESS;
}

// Generate moves that the piece can physically move to without pins or checks.
static int SCE_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_movelist == NULL || ptr_board == NULL || ptr_precomputation_tbl == NULL) return SCE_FAILURE;

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

static int SCE_Knight_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_movelist == NULL || ptr_board == NULL || ptr_precomputation_tbl == NULL) return SCE_FAILURE;

    const uint piece_types[] = { W_KNIGHT, B_KNIGHT };
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

static int SCE_King_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_movelist == NULL || ptr_board == NULL || ptr_precomputation_tbl == NULL) return SCE_FAILURE;

    const uint piece_types[] = { W_KING, B_KING };
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
    }

    return SCE_SUCCESS;
}

// TODO: Slider capture flag
static int SCE_Slider_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_movelist == NULL || ptr_board == NULL || ptr_precomputation_tbl == NULL) return SCE_FAILURE;

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
        // TODO: Implement logic here.
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
                            const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) ^ (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                            RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                        }
                        break;
                    case EAST:
                        for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                            const uint piece_idx_dst = (piece_idx_src + shift);
                            const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) ^ (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                            RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                        }
                        break;
                    case SOUTH:
                        for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                            const uint piece_idx_dst = (piece_idx_src - CHESSBOARD_DIMENSION * shift);
                            const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) ^ (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                            RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                        }
                        break;
                    case WEST:
                        for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                            const uint piece_idx_dst = (piece_idx_src - shift);
                            const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) ^ (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                            RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                        }
                        break;
                    case NORTHEAST:
                        for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                            const uint piece_idx_dst = (piece_idx_src + (CHESSBOARD_DIMENSION + 1U) * shift);
                            const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) ^ (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                            RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                        }
                        break;
                    case NORTHWEST:
                        for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                            const uint piece_idx_dst = (piece_idx_src + (CHESSBOARD_DIMENSION - 1U) * shift);
                            const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) ^ (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                            RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                        }
                        break;
                    case SOUTHEAST:
                        for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                            const uint piece_idx_dst = (piece_idx_src - (CHESSBOARD_DIMENSION - 1U) * shift);
                            const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) ^ (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                            RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                        }
                        break;
                    case SOUTHWEST:
                        for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                            const uint piece_idx_dst = (piece_idx_src - (CHESSBOARD_DIMENSION + 1U) * shift);
                            const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) ^ (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                            RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
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

static int SCE_Pawn_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_movelist == NULL || ptr_board == NULL || ptr_precomputation_tbl == NULL) return SCE_FAILURE;

    // Four cases:
    // 1. Single Push
    // 2. Double Push (At rank 2, 7)
    // 3. Capture (Seriously why can't these guys capture ahead)

    const uint pawn_types[] = { W_PAWN, B_PAWN };
    const uint64_t occupancy = SCE_Chessboard_Occupancy(ptr_board);
    const uint64_t occupancy_w = SCE_Chessboard_Occupancy_Color(ptr_board, WHITE);
    const uint64_t occupancy_b = SCE_Chessboard_Occupancy_Color(ptr_board, BLACK);
    if (ptr_board->to_move == WHITE) {
        // White pawn
        // 1. Single Push
        uint64_t single_push = (ptr_board->bitboards[W_PAWN] UP) & ~occupancy;

        // 2. Double Push (from rank 2, 7); will be reusing single_push with bitmask for rank 3 and 6.
        // TODO: Need to set En passant square!
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
        // TODO: Need to set En passant square!
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
            if (pawn_idx_dst >= CHESSBOARD_DIMENSION * 2) {
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
            if (pawn_idx_dst >= CHESSBOARD_DIMENSION * 2) {
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
            if (pawn_idx_dst >= CHESSBOARD_DIMENSION * 2) {
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

uint64_t SCE_AN_To_Bitboard(const char* an) {
    if (an == NULL || strlen(an) != 2) {
        fprintf(stderr, "\033[31m[-] Invalid parameter in SCE_AN_To_Bitboard\033[0m\n");
        return 0U;
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
        fprintf(stderr, "\033[31m[-] Invalid parameter (file) in SCE_AN_To_Bitboard\033[0m\n");
        return 0U;
    }

    if ('1' <= rank_char && rank_char <= '8') {
        rank_n = (uint) rank_char - '1';
    } else {
        fprintf(stderr, "\033[31m[-] Invalid parameter (rank) in SCE_AN_To_Bitboard\033[0m\n");
        return 0U;
    }

    return 1ULL << (rank_n * 8) << file_n;
}

int SCE_Bitboard_To_AN(char* const an_out, uint64_t bitboard) {
    if (an_out == NULL || COUNT_SET_BITS(bitboard) != 1) {
        return SCE_FAILURE;
    }

    const uint shift = COUNT_TRAILING_ZEROS(bitboard);  // such that bitboard = 1 << shift
    const uint row = shift / CHESSBOARD_DIMENSION;
    const uint col = shift % CHESSBOARD_DIMENSION;

    an_out[0] = 'A' + (char) col;
    an_out[1] = '1' + (char) row;
    an_out[2] = 0x00;

    return SCE_SUCCESS;
}

int SCE_GenerateLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    // TODO: Implement this
    RETURN_IF_SCE_FAILURE(SCE_GeneratePseudoLegalMoves(ptr_movelist, ptr_board, ptr_precomputation_tbl), "Could not generate pseudolegal moves.\n");
    return SCE_SUCCESS;
}
