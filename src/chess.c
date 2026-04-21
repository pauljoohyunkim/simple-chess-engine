#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "helper.h"
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

static SCE_Return SCE_Knight_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Context* const ctx, const bool tactical);
static SCE_Return SCE_King_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Context* const ctx, const bool tactical);
static SCE_Return SCE_Slider_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Context* const ctx, const bool tactical);
static SCE_Return SCE_Pawn_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Context* const ctx, const bool tactical);

SCE_Return SCE_ChessMoveList_clear(SCE_ChessMoveList* const ptr_list) {
    if (ptr_list == NULL) return SCE_INVALID_PARAM;

    memset(ptr_list, 0, sizeof(SCE_ChessMoveList));

    return SCE_SUCCESS;
}

SCE_Return SCE_Chessboard_clear(SCE_Context* const ctx) {
    if (ctx == NULL) return SCE_INVALID_PARAM;

    for (uint i = 0U; i < N_TYPES_PIECES; i++) {
        ctx->board.bitboards[i] = 0U;
    }

    ctx->board.en_passant_idx = UNASSIGNED;
    ctx->board.to_move = WHITE;
    ctx->board.castling_rights = SCE_CASTLING_RIGHTS_WK | SCE_CASTLING_RIGHTS_WQ | SCE_CASTLING_RIGHTS_BK | SCE_CASTLING_RIGHTS_BQ;
    ctx->board.half_move_clock = 0U;
    ctx->board.zobrist_hash = 0U;
    for (uint i = 0U; i < sizeof(ctx->board.mailbox) / sizeof(ctx->board.mailbox[0]); i++) {
        ctx->board.mailbox[i] = UNASSIGNED_PIECE_TYPE;
    }
    RETURN_IF_SCE_FAILURE(SCE_ChessMoveList_clear(&ctx->board.history), "Error when clearing chess move list");

    return SCE_SUCCESS;
}

SCE_Return SCE_Chessboard_reset(SCE_Context* const ctx) {
    if (ctx == NULL) return SCE_INVALID_PARAM;

    RETURN_IF_SCE_FAILURE(SCE_Chessboard_clear(ctx), "Error when clearing board!");

    // White pieces
    ctx->board.bitboards[W_PAWN] = PAWN_INITIAL_ROW << (8U * 1U);
    ctx->board.bitboards[W_KNIGHT] = KNIGHT_INITIAL_ROW;
    ctx->board.bitboards[W_BISHOP] = BISHOP_INITIAL_ROW;
    ctx->board.bitboards[W_ROOK] = ROOK_INITIAL_ROW;
    ctx->board.bitboards[W_QUEEN] = QUEEN_INITIAL_ROW;
    ctx->board.bitboards[W_KING] = KING_INITIAL_ROW;

    // Black pieces
    ctx->board.bitboards[B_PAWN] = PAWN_INITIAL_ROW << (8U * 6U);
    ctx->board.bitboards[B_KNIGHT] = KNIGHT_INITIAL_ROW << (8U * 7U);
    ctx->board.bitboards[B_BISHOP] = BISHOP_INITIAL_ROW << (8U * 7U);
    ctx->board.bitboards[B_ROOK] = ROOK_INITIAL_ROW << (8U * 7U);
    ctx->board.bitboards[B_QUEEN] = QUEEN_INITIAL_ROW << (8U * 7U);
    ctx->board.bitboards[B_KING] = KING_INITIAL_ROW << (8U * 7U);

    ctx->board.en_passant_idx = UNASSIGNED;
    ctx->board.to_move = WHITE;
    ctx->board.castling_rights = SCE_CASTLING_RIGHTS_WK | SCE_CASTLING_RIGHTS_WQ | SCE_CASTLING_RIGHTS_BK | SCE_CASTLING_RIGHTS_BQ;
    ctx->board.half_move_clock = 0U;
    ctx->board.zobrist_hash = 0U;
    {
        // Mailbox
        ctx->board.mailbox[0U] = W_ROOK;
        ctx->board.mailbox[1U] = W_KNIGHT;
        ctx->board.mailbox[2U] = W_BISHOP;
        ctx->board.mailbox[3U] = W_QUEEN;
        ctx->board.mailbox[4U] = W_KING;
        ctx->board.mailbox[5U] = W_BISHOP;
        ctx->board.mailbox[6U] = W_KNIGHT;
        ctx->board.mailbox[7U] = W_ROOK;
        for (uint i = CHESSBOARD_DIMENSION; i < CHESSBOARD_DIMENSION * 2U; i++) {
            ctx->board.mailbox[i] = W_PAWN;
        }
        for (uint i = CHESSBOARD_DIMENSION * 6U; i < CHESSBOARD_DIMENSION * 7U; i++) {
            ctx->board.mailbox[i] = B_PAWN;
        }
        ctx->board.mailbox[CHESSBOARD_DIMENSION * 7U + 0U] = B_ROOK;
        ctx->board.mailbox[CHESSBOARD_DIMENSION * 7U + 1U] = B_KNIGHT;
        ctx->board.mailbox[CHESSBOARD_DIMENSION * 7U + 2U] = B_BISHOP;
        ctx->board.mailbox[CHESSBOARD_DIMENSION * 7U + 3U] = B_QUEEN;
        ctx->board.mailbox[CHESSBOARD_DIMENSION * 7U + 4U] = B_KING;
        ctx->board.mailbox[CHESSBOARD_DIMENSION * 7U + 5U] = B_BISHOP;
        ctx->board.mailbox[CHESSBOARD_DIMENSION * 7U + 6U] = B_KNIGHT;
        ctx->board.mailbox[CHESSBOARD_DIMENSION * 7U + 7U] = B_ROOK;
    }
    RETURN_IF_SCE_FAILURE(SCE_ChessMoveList_clear(&ctx->board.history), "Error when clearing chess move list");

    return SCE_SUCCESS;
}

static uint64_t xorshift(uint64_t x) {
    x ^= x << 13U;
    x ^= x >> 17U;
    x ^= x << 5U;
    return x;
}

SCE_Return SCE_ZobristTable_init(SCE_Context* const ctx, const uint64_t* const ptr_seed) {
    if (ctx == NULL) return SCE_INVALID_PARAM;
    if (ptr_seed == NULL) srand(time(NULL));

    uint64_t x = ptr_seed == NULL ? (uint64_t) rand() : (*ptr_seed);

    // Piece keys
    for (uint piece_type = W_PAWN; piece_type <= B_KING; piece_type++) {
        for (uint idx = 0U; idx < CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION; idx++) {
            x = xorshift(x);
            ctx->zobrist_table.piece_key[piece_type][idx] = x;
        }
    }

    // Castling rights
    for (uint i = 0U; i < 16U; i++) {
        x = xorshift(x);
        ctx->zobrist_table.castling_keys[i] = x;
    }

    // En passant
    for (uint i = 0U; i < 9U; i++) {
        x = xorshift(x);
        ctx->zobrist_table.en_passant_keys[i] = x;
    }

    // Side key
    x = xorshift(x);
    ctx->zobrist_table.side_key = x;

    return SCE_SUCCESS;
}

#define SCE_ZOBRIST_EN_PASSANT_UNASSIGNED_KEY (8U)
uint64_t SCE_Chessboard_ComputeZobristHash(SCE_Context* const ctx) {
    if (ctx == NULL) return SCE_INVALID_PARAM;

    uint64_t hash = 0U;

    // Board
    for (uint piece_type = W_PAWN; piece_type <= B_KING; piece_type++) {
        // Find the pieces.
        uint64_t pieces = ctx->board.bitboards[piece_type];
        while (pieces) {
            // Get index of pieces one by one.
            const uint idx = COUNT_TRAILING_ZEROS(pieces);
            hash ^= ctx->zobrist_table.piece_key[piece_type][idx];
            pieces &= ~(1ULL << idx);
        }
    }

    // Castling
    hash ^= ctx->zobrist_table.castling_keys[ctx->board.castling_rights];

    // En passant
    if (ctx->board.en_passant_idx == UNASSIGNED) {
        hash ^= ctx->zobrist_table.en_passant_keys[SCE_ZOBRIST_EN_PASSANT_UNASSIGNED_KEY];
    } else {
        const uint col_idx = ctx->board.en_passant_idx % 8U;
        hash ^= ctx->zobrist_table.en_passant_keys[col_idx];
    }

    // Side
    if (ctx->board.to_move == BLACK) {
        hash ^= ctx->zobrist_table.side_key;
    }

    return hash;
}

uint64_t SCE_Chessboard_Occupancy(const SCE_Context* const ctx) {
    if (ctx == NULL) return 0U;

    uint64_t occupancy = 0ULL;
    for (uint piece_type = 0U; piece_type < N_TYPES_PIECES; piece_type++) {
        occupancy ^= ctx->board.bitboards[piece_type];
    }

    return occupancy;
}

uint64_t SCE_Chessboard_Occupancy_Color(const SCE_Context* const ctx, const PieceColor color) {
    if (ctx == NULL || (color != WHITE && color != BLACK)) return 0U;

    uint64_t occupancy = 0ULL;
    if (color == WHITE) {
        // White
        for (uint piece_type = W_PAWN; piece_type <= W_KING; piece_type++) {
            occupancy ^= ctx->board.bitboards[piece_type];
        }
    } else {
        // Black
        for (uint piece_type = B_PAWN; piece_type <= B_KING; piece_type++) {
            occupancy ^= ctx->board.bitboards[piece_type];
        }
    }

    return occupancy;
}

SCE_Return SCE_Chessboard_print(SCE_Context* const ctx, PieceColor color) {
    if (ctx == NULL) return SCE_INVALID_PARAM;
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

            const PieceType piece_in_square = ctx->board.mailbox[shift];

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
                case UNASSIGNED_PIECE_TYPE:
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

SCE_Return SCE_PieceMovementPrecompute(SCE_Context* const ctx) {
    if (ctx == NULL) return SCE_INVALID_PARAM;

    // Empty the table.
    memset(&ctx->precomputation_table, 0, sizeof(SCE_PieceMovementPrecomputationTable));

    // Precomputation: Knight
    RETURN_IF_SCE_FAILURE(SCE_Knight_Precompute(&ctx->precomputation_table), "Knight moves table generation failed!");
    RETURN_IF_SCE_FAILURE(SCE_King_Precompute(&ctx->precomputation_table), "King moves table generation failed!");
    RETURN_IF_SCE_FAILURE(SCE_Pawn_Precompute(&ctx->precomputation_table), "Pawn moves/attacks table generation failed!");
    RETURN_IF_SCE_FAILURE(SCE_Rays_Precompute(&ctx->precomputation_table), "Pawn moves/attacks table generation failed!");
    RETURN_IF_SCE_FAILURE(SCE_CastlingMask_Precompute(&ctx->precomputation_table), "Castling mask table generation failed!");

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
SCE_Return SCE_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Context* const ctx, const bool tactical) {
    if (ptr_movelist == NULL || ctx == NULL) return SCE_INVALID_PARAM;

    // 1. Generate pseudolegal moves for knights
    RETURN_IF_SCE_FAILURE(SCE_Knight_GeneratePseudoLegalMoves(ptr_movelist, ctx, tactical), "Knight (pseudolegal) move generation failed");

    // 2. Generate pseudolegal moves for kings
    RETURN_IF_SCE_FAILURE(SCE_King_GeneratePseudoLegalMoves(ptr_movelist, ctx, tactical), "King (pseudolegal) move generation failed");

    // 3. Generate pseudolegal moves for sliders (bishop, rook, queen)
    RETURN_IF_SCE_FAILURE(SCE_Slider_GeneratePseudoLegalMoves(ptr_movelist, ctx, tactical), "Slider (pseudolegal) move generation failed");

    // 4. Generate pseudolegal moves for pawns
    RETURN_IF_SCE_FAILURE(SCE_Pawn_GeneratePseudoLegalMoves(ptr_movelist, ctx, tactical), "Pawn (pseudolegal) move generation failed");

    return SCE_SUCCESS;
}

static SCE_Return SCE_Knight_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Context* const ctx, const bool tactical) {
    if (ptr_movelist == NULL || ctx == NULL) return SCE_INVALID_PARAM;

    const uint64_t occupancy_w = SCE_Chessboard_Occupancy_Color(ctx, WHITE);
    const uint64_t occupancy_b = SCE_Chessboard_Occupancy_Color(ctx, BLACK);
    const uint moving_piece_type = ctx->board.to_move == WHITE ? W_KNIGHT : B_KNIGHT;

    // Get all knights
    uint64_t knights = ctx->board.bitboards[moving_piece_type];
    while (knights) {
        // Loop and generate moves for each knight. After generating move for a knight, remove the bit.
        uint knight_idx_src = COUNT_TRAILING_ZEROS(knights);
        // Knight moves, but cannot attack the same color
        uint64_t knight_moves = (ctx->precomputation_table.knight_moves[knight_idx_src] & ~(SCE_Chessboard_Occupancy_Color(ctx, moving_piece_type == W_KNIGHT ? WHITE : BLACK)));
        if (tactical) {
            knight_moves &= moving_piece_type == W_KNIGHT ? occupancy_b : occupancy_w;
        }
        
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

static SCE_Return SCE_King_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Context* const ctx, const bool tactical) {
    if (ptr_movelist == NULL || ctx == NULL) return SCE_INVALID_PARAM;

    const uint64_t occupancy = SCE_Chessboard_Occupancy(ctx);
    const uint64_t occupancy_w = SCE_Chessboard_Occupancy_Color(ctx, WHITE);
    const uint64_t occupancy_b = SCE_Chessboard_Occupancy_Color(ctx, BLACK);
    const uint moving_piece_type = ctx->board.to_move == WHITE ? W_KING : B_KING;

    // Get king
    uint64_t king = ctx->board.bitboards[moving_piece_type];
    if (king) {
        // Loop and generate moves for the king.
        uint king_idx_src = COUNT_TRAILING_ZEROS(king);
        // King moves, but cannot attack the same color
        uint64_t king_moves = (ctx->precomputation_table.king_moves[king_idx_src] & ~(SCE_Chessboard_Occupancy_Color(ctx, moving_piece_type == W_KING ? WHITE : BLACK)));
        if (tactical) {
            king_moves &= moving_piece_type == W_KING ? occupancy_b : occupancy_w;
        }
        
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

        if (!tactical) {
                // 00000110 (from A to H) = 0b01100000
                uint64_t king_side_gap_mask = 0x60ULL;
                // 01110000 (from A to H) = 0b1110
                uint64_t queen_side_gap_mask = 0x0EULL;
                // Castling
                if (moving_piece_type == W_KING) {
                    // White king
                    // King-side
                    if (ctx->board.castling_rights & SCE_CASTLING_RIGHTS_WK) {
                        // Check for gap.
                        if (!(occupancy & king_side_gap_mask)) {
                            const uint king_idx_src = COUNT_TRAILING_ZEROS(KING_INITIAL_ROW);
                            const uint king_idx_dst = king_idx_src + 2U;
                            const SCE_ChessMove move = (king_idx_src SCE_CHESSMOVE_SET_SRC) | (king_idx_dst SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_KING_CASTLE SCE_CHESSMOVE_SET_FLAG);
                            SCE_AddToMoveList(move, ptr_movelist);
                        }
                    }

                    // Queen-side
                    if (ctx->board.castling_rights & SCE_CASTLING_RIGHTS_WQ) {
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
                    if (ctx->board.castling_rights & SCE_CASTLING_RIGHTS_BK) {
                        // Check for gap.
                        if (!(occupancy & (king_side_gap_mask UP * 7))) {
                            const uint king_idx_src = COUNT_TRAILING_ZEROS(KING_INITIAL_ROW UP * 7);
                            const uint king_idx_dst = king_idx_src + 2U;
                            const SCE_ChessMove move = (king_idx_src SCE_CHESSMOVE_SET_SRC) | (king_idx_dst SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_KING_CASTLE SCE_CHESSMOVE_SET_FLAG);
                            SCE_AddToMoveList(move, ptr_movelist);
                        }
                    }

                    // Queen-side
                    if (ctx->board.castling_rights & SCE_CASTLING_RIGHTS_BQ) {
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
        }

    return SCE_SUCCESS;
}

static SCE_Return SCE_Slider_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Context* const ctx, const bool tactical) {
    if (ptr_movelist == NULL || ctx == NULL) return SCE_INVALID_PARAM;

    const uint64_t occupancy = SCE_Chessboard_Occupancy(ctx);
    const uint64_t occupancy_w = SCE_Chessboard_Occupancy_Color(ctx, WHITE);
    const uint64_t occupancy_b = SCE_Chessboard_Occupancy_Color(ctx, BLACK);
    uint piece_types[3U] = { 0 };

    if (ctx->board.to_move == WHITE) {
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

        uint64_t pieces = ctx->board.bitboards[moving_piece_type];
        while (pieces) {
            // Loop and generate moves for each piece. After generating move for a knight, remove the bit.
            uint piece_idx_src = COUNT_TRAILING_ZEROS(pieces);
            uint piece_row = piece_idx_src / CHESSBOARD_DIMENSION;
            uint piece_col = piece_idx_src % CHESSBOARD_DIMENSION;
            const uint64_t blockers[] = {
                ctx->precomputation_table.rays[NORTH][piece_idx_src] & occupancy,
                ctx->precomputation_table.rays[EAST][piece_idx_src] & occupancy,
                ctx->precomputation_table.rays[SOUTH][piece_idx_src] & occupancy,
                ctx->precomputation_table.rays[WEST][piece_idx_src] & occupancy,
                ctx->precomputation_table.rays[NORTHEAST][piece_idx_src] & occupancy,
                ctx->precomputation_table.rays[NORTHWEST][piece_idx_src] & occupancy,
                ctx->precomputation_table.rays[SOUTHEAST][piece_idx_src] & occupancy,
                ctx->precomputation_table.rays[SOUTHWEST][piece_idx_src] & occupancy
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
                    if (blockers[NORTH]) {
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

                    if (blockers[EAST]) {
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

                    if (blockers[SOUTH]) {
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

                    if (blockers[WEST]) {
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
                    if (blockers[NORTHEAST]) {
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

                    if (blockers[NORTHWEST]) {
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

                    if (blockers[SOUTHEAST]) {
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

                    if (blockers[SOUTHWEST]) {
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
                        if (tactical && max_shifts[direction] > 0) {
                            const uint piece_idx_dst = piece_idx_src + CHESSBOARD_DIMENSION * max_shifts[direction];
                            if ((moving_piece_color == WHITE ? occupancy_b : occupancy_w) & (1ULL << piece_idx_dst)) {
                                const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                            }
                        } else {
                            for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                                const uint piece_idx_dst = (piece_idx_src + CHESSBOARD_DIMENSION * shift);
                                const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                                if ((1ULL << piece_idx_dst) & (moving_piece_color == WHITE ? occupancy_b : occupancy_w)) {
                                    RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                                } else {
                                    RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                                }
                            }
                        }
                        break;
                    case EAST:
                        if (tactical && max_shifts[direction] > 0) {
                            const uint piece_idx_dst = (piece_idx_src + max_shifts[direction]);
                            if ((moving_piece_color == WHITE ? occupancy_b : occupancy_w) & (1ULL << piece_idx_dst)) {
                                const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                            }
                        } else {
                            for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                                const uint piece_idx_dst = (piece_idx_src + shift);
                                const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                                if ((1ULL << piece_idx_dst) & (moving_piece_color == WHITE ? occupancy_b : occupancy_w)) {
                                    RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                                } else {
                                    RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                                }
                            }
                        }
                        break;
                    case SOUTH:
                        if (tactical && max_shifts[direction] > 0) {
                            const uint piece_idx_dst = (piece_idx_src - CHESSBOARD_DIMENSION * max_shifts[direction]);
                            if ((moving_piece_color == WHITE ? occupancy_b : occupancy_w) & (1ULL << piece_idx_dst)) {
                                const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                            }
                        } else {
                            for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                                const uint piece_idx_dst = (piece_idx_src - CHESSBOARD_DIMENSION * shift);
                                const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                                if ((1ULL << piece_idx_dst) & (moving_piece_color == WHITE ? occupancy_b : occupancy_w)) {
                                    RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                                } else if (!tactical) {
                                    RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                                }
                            }
                        }
                        break;
                    case WEST:
                        if (tactical && max_shifts[direction] > 0) {
                            const uint piece_idx_dst = (piece_idx_src - max_shifts[direction]);
                            if ((moving_piece_color == WHITE ? occupancy_b : occupancy_w) & (1ULL << piece_idx_dst)) {
                                const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                            }
                        } else {
                            for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                                const uint piece_idx_dst = (piece_idx_src - shift);
                                const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                                if ((1ULL << piece_idx_dst) & (moving_piece_color == WHITE ? occupancy_b : occupancy_w)) {
                                    RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                                } else {
                                    RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                                }
                            }
                        }
                        break;
                    case NORTHEAST:
                        if (tactical && max_shifts[direction] > 0) {
                            const uint piece_idx_dst = (piece_idx_src + (CHESSBOARD_DIMENSION + 1U) * max_shifts[direction]);
                            if ((moving_piece_color == WHITE ? occupancy_b : occupancy_w) & (1ULL << piece_idx_dst)) {
                                const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                            }
                        } else {
                            for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                                const uint piece_idx_dst = (piece_idx_src + (CHESSBOARD_DIMENSION + 1U) * shift);
                                const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                                if ((1ULL << piece_idx_dst) & (moving_piece_color == WHITE ? occupancy_b : occupancy_w)) {
                                    RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                                } else if (!tactical) {
                                    RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                                }
                            }
                        }
                        break;
                    case NORTHWEST:
                        if (tactical && max_shifts[direction] > 0) {
                            const uint piece_idx_dst = (piece_idx_src + (CHESSBOARD_DIMENSION - 1U) * max_shifts[direction]);
                            if ((moving_piece_color == WHITE ? occupancy_b : occupancy_w) & (1ULL << piece_idx_dst)) {
                                const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                            }
                        } else {
                            for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                                const uint piece_idx_dst = (piece_idx_src + (CHESSBOARD_DIMENSION - 1U) * shift);
                                const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                                if ((1ULL << piece_idx_dst) & (moving_piece_color == WHITE ? occupancy_b : occupancy_w)) {
                                    RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                                } else if (!tactical) {
                                    RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                                }
                            }
                        }
                        break;
                    case SOUTHEAST:
                        if (tactical && max_shifts[direction] > 0) {
                            const uint piece_idx_dst = (piece_idx_src - (CHESSBOARD_DIMENSION - 1U) * max_shifts[direction]);
                            if ((moving_piece_color == WHITE ? occupancy_b : occupancy_w) & (1ULL << piece_idx_dst)) {
                                const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                            }
                        } else {
                            for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                                const uint piece_idx_dst = (piece_idx_src - (CHESSBOARD_DIMENSION - 1U) * shift);
                                const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                                if ((1ULL << piece_idx_dst) & (moving_piece_color == WHITE ? occupancy_b : occupancy_w)) {
                                    RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                                } else if (!tactical) {
                                    RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                                }
                            }
                        }
                        break;
                    case SOUTHWEST:
                        if (tactical && max_shifts[direction] > 0) {
                            const uint piece_idx_dst = (piece_idx_src - (CHESSBOARD_DIMENSION + 1U) * max_shifts[direction]);
                            if ((moving_piece_color == WHITE ? occupancy_b : occupancy_w) & (1ULL << piece_idx_dst)) {
                                const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                            }
                        } else {
                            for (uint shift = 1U; shift <= max_shifts[direction]; shift++) {
                                const uint piece_idx_dst = (piece_idx_src - (CHESSBOARD_DIMENSION + 1U) * shift);
                                const SCE_ChessMove move = (piece_idx_src SCE_CHESSMOVE_SET_SRC) | (piece_idx_dst SCE_CHESSMOVE_SET_DST);
                                if ((1ULL << piece_idx_dst) & (moving_piece_color == WHITE ? occupancy_b : occupancy_w)) {
                                    RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_CAPTURE SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add move.");
                                } else if (!tactical) {
                                    RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add move.");
                                }
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

static SCE_Return SCE_Pawn_GeneratePseudoLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Context* const ctx, const bool tactical) {
    if (ptr_movelist == NULL || ctx == NULL) return SCE_INVALID_PARAM;

    // Four cases:
    // 1. Single Push
    // 2. Double Push (At rank 2, 7)
    // 3. Capture (Seriously why can't these guys capture ahead)

    const uint64_t occupancy = SCE_Chessboard_Occupancy(ctx);
    const uint64_t occupancy_w = SCE_Chessboard_Occupancy_Color(ctx, WHITE);
    const uint64_t occupancy_b = SCE_Chessboard_Occupancy_Color(ctx, BLACK);
    if (ctx->board.to_move == WHITE) {
        // White pawn
        // 1. Single Push
        uint64_t single_push = (ctx->board.bitboards[W_PAWN] UP) & ~occupancy;

        // 2. Double Push (from rank 2, 7); will be reusing single_push with bitmask for rank 3 and 6.
        // For tactical move generation, this is skipped.
        const uint64_t filtered = single_push & (PAWN_INITIAL_ROW UP * 2U);
        uint64_t double_push = tactical ? 0U : (filtered UP) & ~occupancy;

        // 3. Capture
        // 3.1. Capture EAST
        uint64_t capture_e = ((ctx->board.bitboards[W_PAWN] & ~H_MASK) UP RIGHT) & occupancy_b;
        // 3.2. Capture WEST
        uint64_t capture_w = ((ctx->board.bitboards[W_PAWN] & ~A_MASK) UP LEFT) & occupancy_b;

        // 1. Single Push
        while (single_push) {
            const uint pawn_idx_dst = COUNT_TRAILING_ZEROS(single_push);
            const uint64_t pawn_dst = 1ULL << pawn_idx_dst;

            const SCE_ChessMove move = ((pawn_idx_dst - CHESSBOARD_DIMENSION) SCE_CHESSMOVE_SET_SRC) ^ (pawn_idx_dst SCE_CHESSMOVE_SET_DST);
            if (pawn_idx_dst < CHESSBOARD_DIMENSION * 7U) {
                // Normal push. Skipped for tactical move generation
                if (!tactical) {
                    RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add pawn move.");
                }
            } else {
                // Promotion
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_KNIGHT_PROMOTION SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (knight promotion) move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_BISHOP_PROMOTION SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (bishop promotion) move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_ROOK_PROMOTION SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (rook promotion) move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_QUEEN_PROMOTION SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (queen promotion) move.");
            }

            single_push &= ~pawn_dst;
        }

        // Set to zero if tactical -> skipped.
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
            if (ctx->board.en_passant_idx != UNASSIGNED) {
                const uint64_t en_passant_square = (1ULL << (unsigned int) ctx->board.en_passant_idx);
                const uint64_t en_passant_attack_eligible = (ctx->board.bitboards[W_PAWN] & (PAWN_INITIAL_ROW UP * 4U));
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
        uint64_t single_push = (ctx->board.bitboards[B_PAWN] DOWN) & ~occupancy;

        // 2. Double Push (from rank 2, 7); will be reusing single_push with bitmask for rank 3 and 6.
        // Skipped if tactical move gneeration
        const uint64_t filtered = single_push & (PAWN_INITIAL_ROW UP * 5U);
        uint64_t double_push = tactical ? 0U : (filtered DOWN) & ~occupancy;

        // 3. Capture
        // 3.1. Capture EAST
        uint64_t capture_e = ((ctx->board.bitboards[B_PAWN] & ~H_MASK) DOWN RIGHT) & occupancy_w;
        // 3.2. Capture WEST
        uint64_t capture_w = ((ctx->board.bitboards[B_PAWN] & ~A_MASK) DOWN LEFT) & occupancy_w;

        // 1. Single Push
        while (single_push) {
            const uint pawn_idx_dst = COUNT_TRAILING_ZEROS(single_push);
            const uint64_t pawn_dst = 1ULL << pawn_idx_dst;

            const SCE_ChessMove move = ((pawn_idx_dst + CHESSBOARD_DIMENSION) SCE_CHESSMOVE_SET_SRC) ^ (pawn_idx_dst SCE_CHESSMOVE_SET_DST);
            if (pawn_idx_dst >= CHESSBOARD_DIMENSION) {
                // Normal push
                // Skipped if tactical
                if (!tactical) {
                    RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, ptr_movelist), "Could not add pawn move.");
                }
            } else {
                // Promotion
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_KNIGHT_PROMOTION SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (knight promotion) move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_BISHOP_PROMOTION SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (bishop promotion) move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_ROOK_PROMOTION SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (rook promotion) move.");
                RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move | (SCE_CHESSMOVE_FLAG_QUEEN_PROMOTION SCE_CHESSMOVE_SET_FLAG), ptr_movelist), "Could not add pawn (queen promotion) move.");
            }

            single_push &= ~pawn_dst;
        }

        // Set to zero if tactical -> skipped.
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
            if (ctx->board.en_passant_idx != UNASSIGNED) {
                const uint64_t en_passant_square = (1ULL << (unsigned int) ctx->board.en_passant_idx);
                const uint64_t en_passant_attack_eligible = (ctx->board.bitboards[B_PAWN] & (PAWN_INITIAL_ROW UP * 3U));
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

bool SCE_IsSquareAttacked(SCE_Context* const ctx, const uint64_t square, const PieceColor attacked_by) {
    if (ctx == NULL || (attacked_by != WHITE && attacked_by != BLACK)) {
        fprintf(stderr, "\033[31m[-] Invalid parameter in SCE_IsSquareAttacked\033[0m\n");
        return false;
    }
    
    if (COUNT_SET_BITS(square) != 1) {
        fprintf(stderr, "\033[31m[-] Invalid parameter (square) in SCE_IsSquareAttacked\033[0m\n");
        return false;
    }

    const uint square_idx = COUNT_TRAILING_ZEROS(square);

    // Load attacker bitboards.
    const uint64_t attacker_pawns = attacked_by == WHITE ? ctx->board.bitboards[W_PAWN] : ctx->board.bitboards[B_PAWN];
    const uint64_t attacker_knights = attacked_by == WHITE ? ctx->board.bitboards[W_KNIGHT] : ctx->board.bitboards[B_KNIGHT];
    const uint64_t attacker_bishops = attacked_by == WHITE ? ctx->board.bitboards[W_BISHOP] : ctx->board.bitboards[B_BISHOP];
    const uint64_t attacker_rooks = attacked_by == WHITE ? ctx->board.bitboards[W_ROOK] : ctx->board.bitboards[B_ROOK];
    const uint64_t attacker_queen = attacked_by == WHITE ? ctx->board.bitboards[W_QUEEN] : ctx->board.bitboards[B_QUEEN];
    const uint64_t attacker_king = attacked_by == WHITE ? ctx->board.bitboards[W_KING] : ctx->board.bitboards[B_KING];

    // Reverse Lookup
    // 1. Knight: From the square, check if any attacker knight is reachable as a knight.
    // 2. Pawn: From the square, check if any attacker pawn is attackable. (Use victim table).
    // 3. King
    // 4. Sliders with rays.

    // 1. Knight
    if (ctx->precomputation_table.knight_moves[square_idx] & attacker_knights) {
        return true;
    }

    // 2. Pawn
    if (ctx->precomputation_table.pawn_attacks[attacked_by == WHITE ? BLACK : WHITE][square_idx] & attacker_pawns) {
        return true;
    }

    // 3. King
    if (ctx->precomputation_table.king_moves[square_idx] & attacker_king) {
        return true;
    }

    // 4. Sliders
    const uint64_t occupancy = SCE_Chessboard_Occupancy(ctx);

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
        const uint64_t intersection = occupancy & ctx->precomputation_table.rays[ray_direction][square_idx];
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

SCE_Return SCE_MakeMove(SCE_Context* const ctx, const SCE_ChessMove move) {
    if (ctx == NULL) return SCE_INVALID_PARAM;

    const uint src_idx = move SCE_CHESSMOVE_GET_SRC;
    const uint64_t src = (1ULL << src_idx);
    const uint dst_idx = move SCE_CHESSMOVE_GET_DST;
    const uint64_t dst = (1ULL << dst_idx);
    const uint flag = move SCE_CHESSMOVE_GET_FLAG;
    const uint64_t occupancy_w = SCE_Chessboard_Occupancy_Color(ctx, WHITE);
    const uint64_t occupancy_b = SCE_Chessboard_Occupancy_Color(ctx, BLACK);
    
    // Check if src piece is to move.
    if ((ctx->board.to_move == WHITE && !(src & occupancy_w)) || (ctx->board.to_move == BLACK && !(src & occupancy_b))) return SCE_INVALID_MOVE;

    // Check if dst piece is the same color as the moving piece. If so, this is not allowed.
    if ((ctx->board.to_move == WHITE && (dst & occupancy_w)) || (ctx->board.to_move == BLACK && (dst & occupancy_b))) return SCE_INVALID_MOVE;

    PieceType moving_piece_type = UNASSIGNED;
    PieceType captured_piece_type = UNASSIGNED;
    {
        // There is a guarantee that one of the piece types will be set here from the src check.
        moving_piece_type = ctx->board.mailbox[src_idx];

        if (flag == SCE_CHESSMOVE_FLAG_EN_PASSANT_CAPTURE) {
            uint captured_piece_idx = ctx->board.to_move == WHITE ? (ctx->board.en_passant_idx - CHESSBOARD_DIMENSION) : (ctx->board.en_passant_idx + CHESSBOARD_DIMENSION);
            captured_piece_type = ctx->board.mailbox[captured_piece_idx];
        } else {
            captured_piece_type = ctx->board.mailbox[dst_idx];
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
            if (SCE_IsSquareAttacked(ctx, src, ctx->board.to_move == WHITE ? BLACK : WHITE)) {
                return SCE_INVALID_MOVE;
            }
        }
    }

    {
        // Journalling
        ctx->board.undo_states[ctx->board.history.count].moving_piece = moving_piece_type;
        ctx->board.undo_states[ctx->board.history.count].captured_piece =  captured_piece_type;
        ctx->board.undo_states[ctx->board.history.count].en_passant_square = ctx->board.en_passant_idx;
        ctx->board.undo_states[ctx->board.history.count].castling_rights = ctx->board.castling_rights;
        ctx->board.undo_states[ctx->board.history.count].half_move_clock = ctx->board.half_move_clock;
        ctx->board.undo_states[ctx->board.history.count].zobrist_hash = ctx->board.zobrist_hash;
        // This automatically increments the count
        RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(move, &ctx->board.history), "Adding to list failed!");
    }

    const int old_en_passant_idx = ctx->board.en_passant_idx;
    {
        // Execution
        // 1. Capture
        // 2. Move
        // 3. Flag action
        if (flag & SCE_CHESSMOVE_FLAG_CAPTURE) {
            if (flag == SCE_CHESSMOVE_FLAG_EN_PASSANT_CAPTURE) {
                uint captured_piece_idx = ctx->board.to_move == WHITE ? (ctx->board.en_passant_idx - CHESSBOARD_DIMENSION) : (ctx->board.en_passant_idx + CHESSBOARD_DIMENSION);
                uint64_t captured_piece = (1ULL << captured_piece_idx);
                ctx->board.bitboards[captured_piece_type] ^= captured_piece;
                ctx->board.mailbox[captured_piece_idx] = UNASSIGNED;    // Clear out the capture square

                // Zobrist: Captured piece
                ctx->board.zobrist_hash ^= ctx->zobrist_table.piece_key[captured_piece_type][COUNT_TRAILING_ZEROS(captured_piece)];
            } else {
                ctx->board.bitboards[captured_piece_type] ^= dst;
                ctx->board.mailbox[dst_idx] = UNASSIGNED;    // Clear out the capture square

                // Zobrist: Captured piece
                ctx->board.zobrist_hash ^= ctx->zobrist_table.piece_key[captured_piece_type][dst_idx];
            }
        }

        // Standard move
        ctx->board.bitboards[moving_piece_type] ^= src | dst;
        ctx->board.mailbox[src_idx] = UNASSIGNED;
        ctx->board.mailbox[dst_idx] = moving_piece_type;

        {
            // Zobrist: Source piece move
            ctx->board.zobrist_hash ^= ctx->zobrist_table.piece_key[moving_piece_type][src_idx];
            ctx->board.zobrist_hash ^= ctx->zobrist_table.piece_key[moving_piece_type][dst_idx];
        }


        switch (flag) {
            // 1. Pawn Double Push: en passant square
            case SCE_CHESSMOVE_FLAG_DOUBLE_PAWN_PUSH:
                ctx->board.en_passant_idx = ctx->board.to_move == WHITE ? src_idx + CHESSBOARD_DIMENSION : src_idx - CHESSBOARD_DIMENSION;
                break;
            // 2. Promotion
            case SCE_CHESSMOVE_FLAG_KNIGHT_PROMOTION:
            case SCE_CHESSMOVE_FLAG_KNIGHT_PROMO_CAPTURE:
                ctx->board.bitboards[ctx->board.to_move == WHITE ? W_PAWN : B_PAWN] ^= dst;
                ctx->board.bitboards[ctx->board.to_move == WHITE ? W_KNIGHT : B_KNIGHT] ^= dst;
                ctx->board.mailbox[dst_idx] = ctx->board.to_move == WHITE ? W_KNIGHT : B_KNIGHT;

                ctx->board.zobrist_hash ^= ctx->zobrist_table.piece_key[ctx->board.to_move == WHITE ? W_PAWN : B_PAWN][dst_idx];
                ctx->board.zobrist_hash ^= ctx->zobrist_table.piece_key[ctx->board.to_move == WHITE ? W_KNIGHT : B_KNIGHT][dst_idx];
                break;
            case SCE_CHESSMOVE_FLAG_BISHOP_PROMOTION:
            case SCE_CHESSMOVE_FLAG_BISHOP_PROMO_CAPTURE:
                ctx->board.bitboards[ctx->board.to_move == WHITE ? W_PAWN : B_PAWN] ^= dst;
                ctx->board.bitboards[ctx->board.to_move == WHITE ? W_BISHOP : B_BISHOP] ^= dst;
                ctx->board.mailbox[dst_idx] = ctx->board.to_move == WHITE ? W_BISHOP : B_BISHOP;

                ctx->board.zobrist_hash ^= ctx->zobrist_table.piece_key[ctx->board.to_move == WHITE ? W_PAWN : B_PAWN][dst_idx];
                ctx->board.zobrist_hash ^= ctx->zobrist_table.piece_key[ctx->board.to_move == WHITE ? W_BISHOP : B_BISHOP][dst_idx];
                break;
            case SCE_CHESSMOVE_FLAG_ROOK_PROMOTION:
            case SCE_CHESSMOVE_FLAG_ROOK_PROMO_CAPTURE:
                ctx->board.bitboards[ctx->board.to_move == WHITE ? W_PAWN : B_PAWN] ^= dst;
                ctx->board.bitboards[ctx->board.to_move == WHITE ? W_ROOK : B_ROOK] ^= dst;
                ctx->board.mailbox[dst_idx] = ctx->board.to_move == WHITE ? W_ROOK : B_ROOK;

                ctx->board.zobrist_hash ^= ctx->zobrist_table.piece_key[ctx->board.to_move == WHITE ? W_PAWN : B_PAWN][dst_idx];
                ctx->board.zobrist_hash ^= ctx->zobrist_table.piece_key[ctx->board.to_move == WHITE ? W_ROOK : B_ROOK][dst_idx];
                break;
            case SCE_CHESSMOVE_FLAG_QUEEN_PROMOTION:
            case SCE_CHESSMOVE_FLAG_QUEEN_PROMO_CAPTURE:
                ctx->board.bitboards[ctx->board.to_move == WHITE ? W_PAWN : B_PAWN] ^= dst;
                ctx->board.bitboards[ctx->board.to_move == WHITE ? W_QUEEN : B_QUEEN] ^= dst;
                ctx->board.mailbox[dst_idx] = ctx->board.to_move == WHITE ? W_QUEEN : B_QUEEN;

                ctx->board.zobrist_hash ^= ctx->zobrist_table.piece_key[ctx->board.to_move == WHITE ? W_PAWN : B_PAWN][dst_idx];
                ctx->board.zobrist_hash ^= ctx->zobrist_table.piece_key[ctx->board.to_move == WHITE ? W_QUEEN : B_QUEEN][dst_idx];
                break;
            // 3. Castling
            case SCE_CHESSMOVE_FLAG_KING_CASTLE:
                {
                    const uint rook_idx_src = dst_idx + 1U;
                    const uint rook_idx_dst = dst_idx - 1U;
                    const uint64_t rook_src = (1ULL << rook_idx_src);
                    const uint64_t rook_dst = (1ULL << rook_idx_dst);
                    ctx->board.bitboards[ctx->board.to_move == WHITE ? W_ROOK : B_ROOK] ^= (rook_src ^ rook_dst);
                    ctx->board.mailbox[rook_idx_src] = UNASSIGNED;
                    ctx->board.mailbox[rook_idx_dst] = ctx->board.to_move == WHITE ? W_ROOK : B_ROOK;

                    ctx->board.zobrist_hash ^= ctx->zobrist_table.piece_key[ctx->board.to_move == WHITE ? W_ROOK : B_ROOK][rook_idx_src];
                    ctx->board.zobrist_hash ^= ctx->zobrist_table.piece_key[ctx->board.to_move == WHITE ? W_ROOK : B_ROOK][rook_idx_dst];
                }
                break;
            case SCE_CHESSMOVE_FLAG_QUEEN_CASTLE:
                {
                    const uint rook_idx_src = dst_idx - 2U;
                    const uint rook_idx_dst = dst_idx + 1U;
                    const uint64_t rook_src = (1ULL << rook_idx_src);
                    const uint64_t rook_dst = (1ULL << rook_idx_dst);
                    ctx->board.bitboards[ctx->board.to_move == WHITE ? W_ROOK : B_ROOK] ^= (rook_src ^ rook_dst);
                    ctx->board.mailbox[rook_idx_src] = UNASSIGNED;
                    ctx->board.mailbox[rook_idx_dst] = ctx->board.to_move == WHITE ? W_ROOK : B_ROOK;

                    ctx->board.zobrist_hash ^= ctx->zobrist_table.piece_key[ctx->board.to_move == WHITE ? W_ROOK : B_ROOK][rook_idx_src];
                    ctx->board.zobrist_hash ^= ctx->zobrist_table.piece_key[ctx->board.to_move == WHITE ? W_ROOK : B_ROOK][rook_idx_dst];
                }
                break;
            default:
                break;
        }
    }
    
    // Since successful, switch to_move
    ctx->board.to_move = ctx->board.to_move == WHITE ? BLACK : WHITE;
    
    // Zobrist: Side
    ctx->board.zobrist_hash ^= ctx->zobrist_table.side_key;

    // For non double-push move, unset en passant square
    if (flag != SCE_CHESSMOVE_FLAG_DOUBLE_PAWN_PUSH) {
        ctx->board.en_passant_idx = UNASSIGNED;
    }

    // Zobrist: En passant
    if (old_en_passant_idx == UNASSIGNED) {
        ctx->board.zobrist_hash ^= ctx->zobrist_table.en_passant_keys[SCE_ZOBRIST_EN_PASSANT_UNASSIGNED_KEY];
    } else {
        ctx->board.zobrist_hash ^= ctx->zobrist_table.en_passant_keys[old_en_passant_idx % CHESSBOARD_DIMENSION];
    }
    if (ctx->board.en_passant_idx == UNASSIGNED) {
        ctx->board.zobrist_hash ^= ctx->zobrist_table.en_passant_keys[SCE_ZOBRIST_EN_PASSANT_UNASSIGNED_KEY];
    } else {
        ctx->board.zobrist_hash ^= ctx->zobrist_table.en_passant_keys[ctx->board.en_passant_idx % CHESSBOARD_DIMENSION];
    }


    // Update castling right
    const uint8_t old_castling_rights = ctx->board.castling_rights;
    ctx->board.castling_rights &= ctx->precomputation_table.castling_mask[src_idx] & ctx->precomputation_table.castling_mask[dst_idx];
    
    // Zobrist: Castling
    ctx->board.zobrist_hash ^= ctx->zobrist_table.castling_keys[old_castling_rights];
    ctx->board.zobrist_hash ^= ctx->zobrist_table.castling_keys[ctx->board.castling_rights];

    // Final Checks:
    // 1. Is previous king in check?
    const uint64_t prev_king_square = ctx->board.bitboards[ctx->board.to_move == WHITE ? B_KING : W_KING];
    if (SCE_IsSquareAttacked(ctx, prev_king_square, ctx->board.to_move)) {
        RETURN_IF_SCE_FAILURE(SCE_UnmakeMove(ctx), "King is in check, but could not unmake.");
        return SCE_INVALID_MOVE;
    }
    // 2. For castling, is through square under attack?
    if (flag == SCE_CHESSMOVE_FLAG_KING_CASTLE || flag == SCE_CHESSMOVE_FLAG_QUEEN_CASTLE) {
        const uint through_idx = flag == SCE_CHESSMOVE_FLAG_KING_CASTLE ? src_idx + 1U : src_idx - 1U;
        const uint64_t through = (1ULL << through_idx);
        if (SCE_IsSquareAttacked(ctx, through, ctx->board.to_move)) {
            RETURN_IF_SCE_FAILURE(SCE_UnmakeMove(ctx), "King is in check, but could not unmake.");
            return SCE_INVALID_MOVE;
        }
    }
    // 3. Update half-move clock
    if (moving_piece_type == W_PAWN || moving_piece_type == B_PAWN || captured_piece_type != UNASSIGNED) {
        ctx->board.half_move_clock = 0U;
    } else {
        ctx->board.half_move_clock++;
    }
    
    return SCE_SUCCESS;
}

SCE_Return SCE_UnmakeMove(SCE_Context* const ctx) {
    if (ctx == NULL) return SCE_INVALID_PARAM;
    if (ctx->board.history.count == 0U) return SCE_MOVELIST_EMPTY;

    // Index to latest move/undo state
    const uint move_idx = ctx->board.history.count - 1U;
    const uint src_idx = ctx->board.history.moves[move_idx] SCE_CHESSMOVE_GET_SRC;
    const uint64_t src = 1ULL << src_idx;
    const uint dst_idx = ctx->board.history.moves[move_idx] SCE_CHESSMOVE_GET_DST;
    const uint64_t dst = 1ULL << dst_idx;
    const uint flag = ctx->board.history.moves[move_idx] SCE_CHESSMOVE_GET_FLAG;
    const uint moving_piece = ctx->board.undo_states[move_idx].moving_piece;
    const int captured_piece = ctx->board.undo_states[move_idx].captured_piece;

    ctx->board.en_passant_idx = ctx->board.undo_states[move_idx].en_passant_square;
    ctx->board.to_move = ctx->board.to_move == WHITE ? BLACK : WHITE;
    ctx->board.castling_rights = ctx->board.undo_states[move_idx].castling_rights;
    ctx->board.half_move_clock = ctx->board.undo_states[move_idx].half_move_clock;
    ctx->board.zobrist_hash = ctx->board.undo_states[move_idx].zobrist_hash;

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
            ctx->board.bitboards[ctx->board.to_move == WHITE ? W_PAWN : B_PAWN] ^= dst;
            ctx->board.bitboards[ctx->board.to_move == WHITE ? W_KNIGHT : B_KNIGHT] ^= dst;
            ctx->board.mailbox[dst_idx] = ctx->board.to_move == WHITE ? W_PAWN : B_PAWN;
            break;
        case SCE_CHESSMOVE_FLAG_BISHOP_PROMOTION:
        case SCE_CHESSMOVE_FLAG_BISHOP_PROMO_CAPTURE:
            ctx->board.bitboards[ctx->board.to_move == WHITE ? W_PAWN : B_PAWN] ^= dst;
            ctx->board.bitboards[ctx->board.to_move == WHITE ? W_BISHOP : B_BISHOP] ^= dst;
            ctx->board.mailbox[dst_idx] = ctx->board.to_move == WHITE ? W_PAWN : B_PAWN;
            break;
        case SCE_CHESSMOVE_FLAG_ROOK_PROMOTION:
        case SCE_CHESSMOVE_FLAG_ROOK_PROMO_CAPTURE:
            ctx->board.bitboards[ctx->board.to_move == WHITE ? W_PAWN : B_PAWN] ^= dst;
            ctx->board.bitboards[ctx->board.to_move == WHITE ? W_ROOK : B_ROOK] ^= dst;
            ctx->board.mailbox[dst_idx] = ctx->board.to_move == WHITE ? W_PAWN : B_PAWN;
            break;
        case SCE_CHESSMOVE_FLAG_QUEEN_PROMOTION:
        case SCE_CHESSMOVE_FLAG_QUEEN_PROMO_CAPTURE:
            ctx->board.bitboards[ctx->board.to_move == WHITE ? W_PAWN : B_PAWN] ^= dst;
            ctx->board.bitboards[ctx->board.to_move == WHITE ? W_QUEEN : B_QUEEN] ^= dst;
            ctx->board.mailbox[dst_idx] = ctx->board.to_move == WHITE ? W_PAWN : B_PAWN;
            break;
        // 3. Castling
        case SCE_CHESSMOVE_FLAG_KING_CASTLE:
            {
                const uint rook_idx_src = dst_idx + 1U;
                const uint rook_idx_dst = dst_idx - 1U;
                const uint64_t rook_src = (1ULL << rook_idx_src);
                const uint64_t rook_dst = (1ULL << rook_idx_dst);
                ctx->board.bitboards[ctx->board.to_move == WHITE ? W_ROOK : B_ROOK] ^= (rook_src ^ rook_dst);
                ctx->board.mailbox[rook_idx_src] = ctx->board.to_move == WHITE ? W_ROOK : B_ROOK;
                ctx->board.mailbox[rook_idx_dst] = UNASSIGNED;
            }
            break;
        case SCE_CHESSMOVE_FLAG_QUEEN_CASTLE:
            {
                const uint rook_idx_src = dst_idx - 2U;
                const uint rook_idx_dst = dst_idx + 1U;
                const uint64_t rook_src = (1ULL << rook_idx_src);
                const uint64_t rook_dst = (1ULL << rook_idx_dst);
                ctx->board.bitboards[ctx->board.to_move == WHITE ? W_ROOK : B_ROOK] ^= (rook_src ^ rook_dst);
                ctx->board.mailbox[rook_idx_src] = ctx->board.to_move == WHITE ? W_ROOK : B_ROOK;
                ctx->board.mailbox[rook_idx_dst] = UNASSIGNED;
            }
            break;
        default:
            break;
    }

    
    // Standard undo-move
    ctx->board.bitboards[moving_piece] ^= src | dst;
    ctx->board.mailbox[dst_idx] = UNASSIGNED;
    ctx->board.mailbox[src_idx] = moving_piece;

    if (captured_piece != UNASSIGNED) {
        if (flag == SCE_CHESSMOVE_FLAG_EN_PASSANT_CAPTURE) {
            const uint captured_idx = ctx->board.to_move == WHITE ? (dst_idx - CHESSBOARD_DIMENSION) : (dst_idx + CHESSBOARD_DIMENSION);
            ctx->board.bitboards[captured_piece] ^= (1ULL << captured_idx);
            ctx->board.mailbox[captured_idx] = captured_piece;
        } else {
            ctx->board.bitboards[captured_piece] ^= dst;
            ctx->board.mailbox[dst_idx] = captured_piece;
        }
    }


    // Decrement move count at the end.
    ctx->board.history.count--;
    return SCE_SUCCESS;
}

SCE_Return SCE_GenerateLegalMoves(SCE_ChessMoveList* const ptr_movelist, SCE_Context* const ctx) {
    if (ptr_movelist == NULL || ctx == NULL) return SCE_INVALID_PARAM;
    if (ptr_movelist->count != 0) return SCE_INVALID_PARAM;

    SCE_ChessMoveList pseudolegal_moves;
    RETURN_IF_SCE_FAILURE(SCE_ChessMoveList_clear(&pseudolegal_moves), "Could not clear move list.");
    RETURN_IF_SCE_FAILURE(SCE_GeneratePseudoLegalMoves(&pseudolegal_moves, ctx, false), "Could not generate pseudo legal movelist.");
    for (uint i = 0U; i < pseudolegal_moves.count; i++) {
        const SCE_Return ret = SCE_MakeMove(ctx, pseudolegal_moves.moves[i]);
        if (ret == SCE_SUCCESS) {
            RETURN_IF_SCE_FAILURE(SCE_AddToMoveList(pseudolegal_moves.moves[i], ptr_movelist), "Could not add move to legal move list.");
            RETURN_IF_SCE_FAILURE(SCE_UnmakeMove(ctx), "Could not unmake!");
        }
    }

    return SCE_SUCCESS;
}
