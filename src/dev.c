#include <stdio.h>
#include "dev.h"
#include "chess.h"

typedef unsigned int uint;

void print_as_board(const uint64_t val) {
    printf("\n");
    for (uint i = 0; i < CHESSBOARD_DIMENSION; i++) {
        for (uint j = 0; j < CHESSBOARD_DIMENSION; j++) {
            uint64_t pos = 1ULL << ((7-i) * CHESSBOARD_DIMENSION + j);
            if (pos & val) {
                printf("*");
            } else {
                printf("-");
            }
        }
        printf("\n");
    }
    printf("\n");
}

SCE_Return place_piece_on_board(SCE_Chessboard* const ptr_board, const char * const an, uint piece_type) {
    if (ptr_board == NULL || an == NULL || piece_type >= N_TYPES_PIECES) return SCE_INVALID_PARAM;

    // Get bitboard form of algebraic notation
    const uint64_t loc = SCE_AN_To_Bitboard(an);
    if (loc == 0) return SCE_INTERNAL_ERROR;

    // Empty out the specific loc.
    for (uint i = 0; i < N_TYPES_PIECES; i++) {
        ptr_board->bitboards[i] &= ~loc;
    }

    ptr_board->bitboards[piece_type] ^= loc;

    return SCE_SUCCESS;
}

SCE_Return print_move_to_AN(const SCE_ChessMove move) {
    const uint64_t src = 1ULL << (move SCE_CHESSMOVE_GET_SRC);
    const uint64_t dst = 1ULL << (move SCE_CHESSMOVE_GET_DST);
    const uint flag = move SCE_CHESSMOVE_GET_FLAG;

    char an_src[3U] = { 0 };
    char an_dst[3U] = { 0 };
    //int ret = SCE_SUCCESS;

    SCE_Bitboard_To_AN(an_src, src);
    SCE_Bitboard_To_AN(an_dst, dst);

    printf("%s -> %s ", an_src, an_dst);
    switch (flag & 15U) {
        case SCE_CHESSMOVE_FLAG_QUIET_MOVE:
            printf("QUIET_MOVE ");
            break;
        case SCE_CHESSMOVE_FLAG_DOUBLE_PAWN_PUSH:
            printf("DOUBLE_PAWN_PUSH ");
            break;
        case SCE_CHESSMOVE_FLAG_KING_CASTLE:
            printf("KING_CASTLE ");
            break;
        case SCE_CHESSMOVE_FLAG_QUEEN_CASTLE:
            printf("QUEEN_CASTLE ");
            break;
        case SCE_CHESSMOVE_FLAG_CAPTURE:
            printf("CAPTURE ");
            break;
        case SCE_CHESSMOVE_FLAG_EN_PASSANT_CAPTURE:
            printf("EN_PASSANT_CAPTURE ");
            break;
        case SCE_CHESSMOVE_FLAG_KNIGHT_PROMOTION:
            printf("KNIGHT_PROMOTION ");
            break;
        case SCE_CHESSMOVE_FLAG_BISHOP_PROMOTION:
            printf("BISHOP_PROMOTION ");
            break;
        case SCE_CHESSMOVE_FLAG_ROOK_PROMOTION:
            printf("ROOK_PROMOTION ");
            break;
        case SCE_CHESSMOVE_FLAG_QUEEN_PROMOTION:
            printf("QUEEN_PROMOTION ");
            break;
        case SCE_CHESSMOVE_FLAG_KNIGHT_PROMO_CAPTURE:
            printf("KNIGHT_PROMO_CAPTURE ");
            break;
        case SCE_CHESSMOVE_FLAG_BISHOP_PROMO_CAPTURE:
            printf("BISHOP_PROMO_CAPTURE ");
            break;
        case SCE_CHESSMOVE_FLAG_ROOK_PROMO_CAPTURE:
            printf("ROOK_PROMO_CAPTURE ");
            break;
        case SCE_CHESSMOVE_FLAG_QUEEN_PROMO_CAPTURE:
            printf("QUEEN_PROMO_CAPTURE ");
            break;
        default:
            return SCE_INVALID_MOVE;
    }

    printf("\n");

    return SCE_SUCCESS;
}

SCE_Return debug_print_board(const SCE_Chessboard* const ptr_board) {
    if (ptr_board == NULL) return SCE_INVALID_PARAM;

    SCE_Chessboard_print(ptr_board, WHITE);
    for (uint i = 0U; i < ptr_board->history.count; i++) {
        printf("Move: \n");
        print_move_to_AN(ptr_board->history.moves[i]);
        const SCE_UndoState* state = &ptr_board->undo_states[i];
        printf("UndoState:\n");
        printf("(MP, CP, EP, CastleR, HalfMoveClock, Hash): (%d, %d, %d, %d, %d, %ld)\n",
            state->moving_piece,
            state->captured_piece,
            state->en_passant_square,
            state->castling_rights,
            state->half_move_clock,
            state->zobrist_hash);
    }

    return SCE_SUCCESS;
}

uint perft_count(const SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_table, const uint depth) {
    if (ptr_board == NULL || ptr_precomputation_table == NULL) return 0U;

    uint count = 0U;
    for (uint ply = 0U; ply < depth; ply++) {
        
    }

    return count;
}
