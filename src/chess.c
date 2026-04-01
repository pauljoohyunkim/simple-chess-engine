#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef DEBUG
#include "dev.h"
#endif
#include "chess.h"

#define RETURN_IF_SCE_FAILURE(x) do { if (!x) return SCE_FAILURE; } while (0);

typedef unsigned int uint;

static int SCE_Knight_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);
static int SCE_King_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);

int SCE_Chessboard_clear(SCE_Chessboard* const ptr_board) {
    if (ptr_board == NULL) return SCE_FAILURE;

    for (uint i = 0U; i < N_TYPES_PIECES; i++) {
        ptr_board->bitboards[i] = 0U;
    }

    return SCE_SUCCESS;
}

int SCE_Chessboard_reset(SCE_Chessboard* const ptr_board) {
    if (ptr_board == NULL) return SCE_FAILURE;

    RETURN_IF_SCE_FAILURE(SCE_Chessboard_clear(ptr_board));

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

    return SCE_SUCCESS;
}

#define UNASSIGNED (-1)
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
            uint shift = (8U * (7U-i) + (7U-j));
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
            printf("%d ", i + 1);
        } else {
            printf("%d ", 8 - i);
        }
        printf("\n");
    }

    return SCE_SUCCESS;
}
#undef UNASSIGNED

int SCE_PieceMovementPrecompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl) {
    if (ptr_precomputation_tbl == NULL) return SCE_FAILURE;

    // Empty the table.
    memset(ptr_precomputation_tbl, 0, sizeof(SCE_PieceMovementPrecomputationTable));

    // Precomputation: Knight
    RETURN_IF_SCE_FAILURE(SCE_Knight_Precompute(ptr_precomputation_tbl));
    RETURN_IF_SCE_FAILURE(SCE_King_Precompute(ptr_precomputation_tbl));

    return SCE_SUCCESS;
}

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
        if (col >= 2U && row <= 6U) {
            moves ^= (pos >> RIGHT >> RIGHT << UP);
        }

        // RRD
        if (col >= 2U && row >= 1U) {
            moves ^= (pos >> RIGHT >> RIGHT >> DOWN);
        }

        // RUU
        if (col >= 1U && row <= 5U) {
            moves ^= (pos >> RIGHT << UP << UP);
        }

        // RDD
        if (col >= 1U && row >= 2U) {
            moves ^= (pos >> RIGHT >> DOWN >> DOWN);
        }

        // LLU
        if (col <= 5U && row <= 6U) {
            moves ^= (pos << LEFT << LEFT << UP);
        }

        // LLD
        if (col <= 5U && row >= 1U) {
            moves ^= (pos << LEFT << LEFT >> DOWN);
        }

        // LUU
        if (col <= 6U && row <= 5U) {
            moves ^= (pos << LEFT << UP << UP);
        }

        // LDD
        if (col <= 6U && row >= 2U) {
            moves ^= (pos << LEFT >> DOWN >> DOWN);
        }

        ptr_precomputation_tbl->knight[i] = moves;
#ifdef DEBUG
        printf("Knight Table %d\n", i);
        print_as_board(ptr_precomputation_tbl->knight[i]);
#endif
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
            moves ^= (pos << UP);
        }

        // D
        if (row >= 1U) {
            moves ^= (pos >> DOWN);
        }

        // L
        if (col <= 6U) {
            moves ^= (pos << LEFT);
        }

        // R
        if (col >= 1U) {
            moves ^= (pos >> RIGHT);
        }

        // RU
        if (col >= 1U && row <= 6U) {
            moves ^= (pos >> RIGHT << UP);
        }

        // RD
        if (col >= 1U && row >= 1U) {
            moves ^= (pos >> RIGHT >> DOWN);
        }

        // LU
        if (col <= 6U && row <= 6U) {
            moves ^= (pos << LEFT << UP);
        }

        // LD
        if (col <= 6U && row >= 1U) {
            moves ^= (pos << LEFT >> DOWN);
        }


        ptr_precomputation_tbl->king[i] = moves;
#ifdef DEBUG
        printf("King Table %d\n", i);
        print_as_board(ptr_precomputation_tbl->king[i]);
#endif
    }

    return SCE_SUCCESS;
}
