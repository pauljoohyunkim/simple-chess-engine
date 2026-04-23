#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "eval/sef.h"
#include "engine.h"
#include "eval/pst.h"

#define TT_TABLE_LOG_2_SIZE 20
#define PHASE_DEEPNING_CUTOFF 15
#define INITIAL_DEPTH 7
#define DEEPENED_DEPTH 9

int main() {
    SCE_Return ret;
    // For now, player gets to be white.
    const PieceColor player = WHITE;

    SCE_Context ctx;

    ret = SCE_PieceMovementPrecompute(&ctx);
    ret = SCE_Chessboard_reset(&ctx);
    ret = SCE_ZobristTable_init(&ctx, NULL);

    // Chess engine
    SCE_Engine engine;
    ret = SCE_Engine_init(&engine, SCE_Eval_SimplifiedEvaluationFunction, NULL, TT_TABLE_LOG_2_SIZE);
    engine.depth = INITIAL_DEPTH;
    

    printf("All moves are to be in \"E2E4\" form\n");
    
    while (true) {
        char input[10] = { 0 };
        char src_an[3] = { 0 };
        char dst_an[3] = { 0 };
        SCE_ChessMoveList legal_move_list;
        ret = SCE_ChessMoveList_clear(&legal_move_list);

        ret = SCE_GenerateLegalMoves(&legal_move_list, &ctx);

        if (legal_move_list.count == 0) break;

        printf("Eval: %0.2f\n", (float) SCE_Eval_SimplifiedEvaluationFunction(&ctx) / 100);
        SCE_Chessboard_print(&ctx, player);

        // Get move from user
        printf("Your move: ");
        scanf("%s", input);
        strncpy(src_an, input, 2);
        strncpy(dst_an, input+2, 2);

        const int src_idx = SCE_AN_To_Idx(src_an);
        const int dst_idx = SCE_AN_To_Idx(dst_an);

        if (src_idx == UNASSIGNED || dst_idx == UNASSIGNED) {
            fprintf(stderr, "Wrong input! Try again\n");
            continue;
        }
        
        int move = UNASSIGNED;
        SCE_ChessMoveList movelist;
        ret = SCE_ChessMoveList_clear(&movelist);
        // Check if move is one of the legal moves.
        for (unsigned int i = 0U; i < legal_move_list.count; i++) {
            const SCE_ChessMove legal_move = legal_move_list.moves[i];
            const unsigned int legal_src_idx = legal_move SCE_CHESSMOVE_GET_SRC;
            const unsigned int legal_dst_idx = legal_move SCE_CHESSMOVE_GET_DST;
            if (legal_src_idx == src_idx && legal_dst_idx == dst_idx) {
                ret = SCE_AddToMoveList(legal_move, &movelist);
            }
        }
        if (movelist.count == 0) {
            fprintf(stderr, "Not a legal move. Try again\n");
            continue;
        } else if (movelist.count == 1) {
            move = movelist.moves[0];
        } else {
            // Promotion
            unsigned int choice;
            printf("Available moves:");
            for (unsigned int i = 0U; i < movelist.count; i++) {
                //const unsigned int src_idx = movelist.moves[i] SCE_CHESSMOVE_GET_SRC;
                //const unsigned int dst_idx = movelist.moves[i] SCE_CHESSMOVE_GET_DST;
                const int flag = movelist.moves[i] SCE_CHESSMOVE_GET_FLAG;
                printf("%d: ");
                switch (flag) {
                    case SCE_CHESSMOVE_FLAG_KNIGHT_PROMOTION:
                    case SCE_CHESSMOVE_FLAG_KNIGHT_PROMO_CAPTURE:
                        printf("Promote to knight");
                        break;
                    case SCE_CHESSMOVE_FLAG_BISHOP_PROMOTION:
                    case SCE_CHESSMOVE_FLAG_BISHOP_PROMO_CAPTURE:
                        printf("Promote to bishop");
                        break;
                    case SCE_CHESSMOVE_FLAG_ROOK_PROMOTION:
                    case SCE_CHESSMOVE_FLAG_ROOK_PROMO_CAPTURE:
                        printf("Promote to rook");
                        break;
                    case SCE_CHESSMOVE_FLAG_QUEEN_PROMOTION:
                    case SCE_CHESSMOVE_FLAG_QUEEN_PROMO_CAPTURE:
                        printf("Promote to queen");
                        break;
                    default:
                        break;
                }
            }
            printf("Choice: ");
            scanf("%d", &choice);
            if (choice >= movelist.count) {
                fprintf(stderr, "Wrong index!\n");
                continue;
            }
        }
        if ((move SCE_CHESSMOVE_GET_FLAG) & SCE_CHESSMOVE_FLAG_CAPTURE) {
            printf("(Capture)");
        }
        printf("\n");

        // Making player move.
        ret = SCE_MakeMove(&ctx, move);
        printf("Eval: %0.2f\n", (float) SCE_Eval_SimplifiedEvaluationFunction(&ctx) / 100);
        SCE_Chessboard_print(&ctx, player);

        const unsigned int phase = SCE_Eval_ComputePhase(&ctx.board);
        if (phase < PHASE_DEEPNING_CUTOFF) {
            if (engine.depth < 8) {
                printf("Warning: Engine phase deepening! Will be harder on you :)\n");
            }
            engine.depth = DEEPENED_DEPTH;
        }

        // ------------------------------------------------
        // Now computer's perspective
        //move = SCE_Engine_AlphaBetaBestMove(&engine, &ctx);
        move = SCE_Engine_IterativeDeepeningAlphaBetaBestMove(&engine, &ctx);
        if (move == UNASSIGNED) {
            printf("Mate!\n");
            break;
        }
        ret = SCE_MakeMove(&ctx, move);
        {
            ret = SCE_Bitboard_To_AN(src_an, 1ULL << (move SCE_CHESSMOVE_GET_SRC));
            ret = SCE_Bitboard_To_AN(dst_an, 1ULL << (move SCE_CHESSMOVE_GET_DST));
            printf("Computer: %s -> %s", src_an, dst_an);
            if ((move SCE_CHESSMOVE_GET_FLAG) & SCE_CHESSMOVE_FLAG_CAPTURE) {
                printf(" (Capture)");
            }
            printf("\n");
        }
    }

    printf("End of game!\n");

    return 0;
}
