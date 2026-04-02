#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef DEBUG
#include "dev.h"
#endif
#include "chess.h"

#define RETURN_IF_SCE_FAILURE(x, msg) do { if (!x) { fprintf(stderr, "%s\n", msg); return SCE_FAILURE; } } while (0);


typedef unsigned int uint;


// Static functions for generating components of precomputation table.
static int SCE_Knight_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);
static int SCE_King_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);
static int SCE_Pawn_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);
static int SCE_Rays_Precompute(SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl);

#ifdef __GNUC__
#define COUNT_SET_BITS __builtin_popcountll
// TODO: Implement fallback
#define COUNT_TRAILING_ZEROS __builtin_ctzll
#define COUNT_LEADING_ZEROS __builtin_clzll
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

int SCE_Chessboard_clear(SCE_Chessboard* const ptr_board) {
    if (ptr_board == NULL) return SCE_FAILURE;

    for (uint i = 0U; i < N_TYPES_PIECES; i++) {
        ptr_board->bitboards[i] = 0U;
    }

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

    return SCE_SUCCESS;
}

uint64_t SCE_Chessboard_Occupancy(SCE_Chessboard* const ptr_board) {
    if (ptr_board == NULL) return 0U;

    uint64_t occupancy = 0ULL;
    for (uint i = 0U; i < N_TYPES_PIECES; i++) {
        occupancy ^= ptr_board->bitboards[i];
    }

    return occupancy;
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
            printf("%d ", 8 - i);
        } else {
            printf("%d ", i + 1);
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
    RETURN_IF_SCE_FAILURE(SCE_Knight_Precompute(ptr_precomputation_tbl), "Knight moves table generation failed!");
    RETURN_IF_SCE_FAILURE(SCE_King_Precompute(ptr_precomputation_tbl), "King moves table generation failed!");
    RETURN_IF_SCE_FAILURE(SCE_Pawn_Precompute(ptr_precomputation_tbl), "Pawn moves/attacks table generation failed!");
    RETURN_IF_SCE_FAILURE(SCE_Rays_Precompute(ptr_precomputation_tbl), "Pawn moves/attacks table generation failed!");

    return SCE_SUCCESS;
}

#define DOWN >> 8U
#define UP << 8U
#define LEFT << 1U
#define RIGHT >> 1U
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
            moves ^= (pos RIGHT RIGHT UP);
        }

        // RRD
        if (col >= 2U && row >= 1U) {
            moves ^= (pos RIGHT RIGHT DOWN);
        }

        // RUU
        if (col >= 1U && row <= 5U) {
            moves ^= (pos RIGHT UP UP);
        }

        // RDD
        if (col >= 1U && row >= 2U) {
            moves ^= (pos RIGHT DOWN DOWN);
        }

        // LLU
        if (col <= 5U && row <= 6U) {
            moves ^= (pos LEFT LEFT UP);
        }

        // LLD
        if (col <= 5U && row >= 1U) {
            moves ^= (pos LEFT LEFT DOWN);
        }

        // LUU
        if (col <= 6U && row <= 5U) {
            moves ^= (pos LEFT UP UP);
        }

        // LDD
        if (col <= 6U && row >= 2U) {
            moves ^= (pos LEFT DOWN DOWN);
        }

        ptr_precomputation_tbl->knight_moves[i] = moves;
#ifdef DEBUG
        printf("Knight Table %d\n", i);
        print_as_board(ptr_precomputation_tbl->knight_moves[i]);
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
            moves ^= (pos UP);
        }

        // D
        if (row >= 1U) {
            moves ^= (pos DOWN);
        }

        // L
        if (col <= 6U) {
            moves ^= (pos LEFT);
        }

        // R
        if (col >= 1U) {
            moves ^= (pos RIGHT);
        }

        // RU
        if (col >= 1U && row <= 6U) {
            moves ^= (pos RIGHT UP);
        }

        // RD
        if (col >= 1U && row >= 1U) {
            moves ^= (pos RIGHT DOWN);
        }

        // LU
        if (col <= 6U && row <= 6U) {
            moves ^= (pos LEFT UP);
        }

        // LD
        if (col <= 6U && row >= 1U) {
            moves ^= (pos LEFT DOWN);
        }


        ptr_precomputation_tbl->king_moves[i] = moves;
#ifdef DEBUG
        printf("King Table %d\n", i);
        print_as_board(ptr_precomputation_tbl->king_moves[i]);
#endif
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
        if (col <= 6U && row <= 6U) {
            w_attacks ^= (pos LEFT UP);
        }

        // RU
        if (col >= 1U && row <= 6U) {
            w_attacks ^= (pos RIGHT UP);
        }

        // LD
        if (col <= 6U && row >= 1U) {
            b_attacks ^= (pos LEFT DOWN);
        }

        // RD
        if (col >= 1U && row >= 1U) {
            b_attacks ^= (pos RIGHT DOWN);
        }

        ptr_precomputation_tbl->pawn_moves[WHITE][i] = w_moves;
        ptr_precomputation_tbl->pawn_moves[BLACK][i] = b_moves;
        ptr_precomputation_tbl->pawn_attacks[WHITE][i] = w_attacks;
        ptr_precomputation_tbl->pawn_attacks[BLACK][i] = b_attacks;
#ifdef DEBUG
        printf("White Pawn Movement Table %d\n", i);
        print_as_board(ptr_precomputation_tbl->pawn_moves[WHITE][i]);
        printf("Black Pawn Movement Table %d\n", i);
        print_as_board(ptr_precomputation_tbl->pawn_moves[BLACK][i]);
        printf("White Pawn Attack Table %d\n", i);
        print_as_board(ptr_precomputation_tbl->pawn_attacks[WHITE][i]);
        printf("Black Pawn Attack Table %d\n", i);
        print_as_board(ptr_precomputation_tbl->pawn_attacks[BLACK][i]);
#endif
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
        while (col >= shift) {
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
        while (col + shift < CHESSBOARD_DIMENSION) {
            w_ray ^= (pos LEFT * shift);
            shift++;
        }

        // NORTHEAST
        shift = 1U;
        while (row + shift < CHESSBOARD_DIMENSION && col >= shift) {
            ne_ray ^= ((pos UP * shift) RIGHT * shift);
            shift++;
        }

        // NORTHWEST
        shift = 1U;
        while (row + shift < CHESSBOARD_DIMENSION && col + shift < CHESSBOARD_DIMENSION) {
            nw_ray ^= ((pos UP * shift) LEFT * shift);
            shift++;
        }

        // SOUTHEAST
        shift = 1U;
        while (row >= shift && col >= shift) {
            se_ray ^= ((pos DOWN * shift) RIGHT * shift);
            shift++;
        }

        // SOUTHWEST
        shift = 1U;
        while (row >= shift && col + shift < CHESSBOARD_DIMENSION) {
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
#ifdef DEBUG
        printf("Rays Table %d\n", i);
        print_as_board(ptr_precomputation_tbl->rays[SOUTHWEST][i]);
#endif
    }

    return SCE_SUCCESS;
}
#undef DOWN
#undef UP
#undef LEFT
#undef RIGHT

bool SCE_IsSqaureAttacked(SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_tbl, const uint64_t square, const PieceColor attacked_by) {
    if (ptr_board == NULL || ptr_precomputation_tbl == NULL || (attacked_by != WHITE && attacked_by != BLACK)) {
        fprintf(stderr, "\033[31m[-] Invalid parameter in SCE_IsSquareAttacked\033[0m\n");
        return false;
    }
    
    if (COUNT_SET_BITS(square) != 1) {
        fprintf(stderr, "\033[31m[-] Invalid parameter (square) in SCE_IsSquareAttacked\033[0m\n");
        return false;
    }

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
    if (ptr_precomputation_tbl->knight_moves[square] & attacker_knights) {
        return true;
    }

    // 2. Pawn
    if (ptr_precomputation_tbl->pawn_attacks[attacked_by == WHITE ? BLACK : WHITE][square] & attacker_pawns) {
        return true;
    }

    // 3. King
    if (ptr_precomputation_tbl->king_moves[square] & attacker_king) {
        return true;
    }

    // 4. Sliders
    const RayDirection positive_directions[4U] = { NORTH, WEST, NORTHEAST, NORTHWEST };
    const RayDirection negative_directions[4U] = { EAST, SOUTH, SOUTHEAST, SOUTHWEST };
    const uint64_t occupancy = SCE_Chessboard_Occupancy(ptr_board);

    // TODO: Refactor (Code duplication can be merged)
    // 4.1 Positive Direction
    // Project in positive direction and check for blockers. If blockers exist, check if they are threatening.
    for (uint i = 0U; i < sizeof(positive_directions) / sizeof(RayDirection); i++) {
        const RayDirection ray_direction = positive_directions[i];
        const uint64_t intersection = occupancy & ptr_precomputation_tbl->rays[ray_direction][square];
        const uint64_t blocker = 1 << (63U - COUNT_LEADING_ZEROS(intersection));
        
        switch (ray_direction) {
            case NORTH:
            case WEST:
                // Rook check
                if (blocker & ptr_board->bitboards[attacked_by == WHITE ? W_ROOK : B_ROOK]) {
                    return true;
                }
                break;
            case NORTHEAST:
            case NORTHWEST:
                // Bishop check
                if (blocker & ptr_board->bitboards[attacked_by == WHITE ? W_BISHOP : B_BISHOP]) {
                    return true;
                }
                break;
        }
        // Queen check
        if (blocker & ptr_board->bitboards[attacked_by == WHITE ? W_QUEEN : B_QUEEN]) {
            return true;
        }
    }

    // 4.2 Negative Direction
    // Project in positive direction and check for blockers. If blockers exist, check if they are threatening.
    for (uint i = 0U; i < sizeof(negative_directions) / sizeof(RayDirection); i++) {
        const RayDirection ray_direction = negative_directions[i];
        const uint64_t intersection = occupancy & ptr_precomputation_tbl->rays[ray_direction][square];
        const uint64_t blocker = 1 << COUNT_TRAILING_ZEROS(intersection);
        
        switch (ray_direction) {
            case NORTH:
            case WEST:
                // Rook check
                if (blocker & ptr_board->bitboards[attacked_by == WHITE ? W_ROOK : B_ROOK]) {
                    return true;
                }
                break;
            case NORTHEAST:
            case NORTHWEST:
                // Bishop check
                if (blocker & ptr_board->bitboards[attacked_by == WHITE ? W_BISHOP : B_BISHOP]) {
                    return true;
                }
                break;
        }
        // Queen check
        if (blocker & ptr_board->bitboards[attacked_by == WHITE ? W_QUEEN : B_QUEEN]) {
            return true;
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
