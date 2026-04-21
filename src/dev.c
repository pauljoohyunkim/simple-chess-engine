#include <stdio.h>
#include <string.h>
#include "helper.h"
#include "dev.h"
#include "chess.h"

#define RETURN_IF_SCE_FAILURE(x, msg) do { if ((x) <= 0) { fprintf(stderr, "%s\n", msg); return SCE_INTERNAL_ERROR; } } while (0);

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
    ptr_board->mailbox[COUNT_TRAILING_ZEROS(loc)] = piece_type;

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

SCE_Return debug_print_board(const SCE_Context* const ctx) {
    if (ctx == NULL) return SCE_INVALID_PARAM;

    if (SCE_Chessboard_print(ctx, WHITE) != SCE_SUCCESS) return SCE_INVALID_BOARD_STATE;
    for (uint i = 0U; i < ctx->board.history.count; i++) {
        printf("Move: \n");
        print_move_to_AN(ctx->board.history.moves[i]);
        const SCE_UndoState* state = &ctx->board.undo_states[i];
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

unsigned long long perft_count(const SCE_Context* const ctx, const uint depth, const bool root) {
    if (ctx == NULL) return 0U;
    if (depth == 0U) return 1U;

    unsigned long long count = 0U;
    SCE_ChessMoveList pseudolegal_moves;
    RETURN_IF_SCE_FAILURE(SCE_ChessMoveList_clear(&pseudolegal_moves), "Could not clear move list.");
    RETURN_IF_SCE_FAILURE(SCE_GeneratePseudoLegalMoves(&pseudolegal_moves, ctx), "Could not clear move list.");
    for (uint i = 0U; i < pseudolegal_moves.count; i++) {
        // For each move, try making the move. If successful, recursively call the function.
        const SCE_Return ret = SCE_MakeMove(ctx, pseudolegal_moves.moves[i]);
        //const uint64_t zobrist_hash = SCE_Chessboard_ComputeZobristHash(ptr_board, ptr_table);
        //if (zobrist_hash != ptr_board->zobrist_hash) {
        //    return SCE_INTERNAL_ERROR;
        //}
        uint add_count;
        if (ret == SCE_SUCCESS) {
            add_count = perft_count(ctx, depth-1, false);
            count += add_count;
            RETURN_IF_SCE_FAILURE(SCE_UnmakeMove(ctx), "Could not unmake move after a successful makemove.");
        }
        if (root && ret == SCE_SUCCESS) {
            printf("\n");
            print_move_to_AN(pseudolegal_moves.moves[i]);
            printf("%d\n", add_count);
        }
        //print_move_to_AN(pseudolegal_moves.moves[i]);
    }

    return count;
}
