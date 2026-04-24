#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "eval/sef.h"
#include "engine.h"
#include "eval/pst.h"
#include "helper.h"

typedef enum {
    SIGNAL_OK = 0,
    SIGNAL_BREAK = 1,
    SIGNAL_CONTINUE = 2
} Signal;


#define TT_TABLE_LOG_2_SIZE 20
#define N_PIECES_DEEPNING_CUTOFF 10
#define INITIAL_DEPTH 10
#define DEEPENED_DEPTH 11

// Returns true if end of game.
static Signal player_move(SCE_Context* const ctx, SCE_Engine* const ptr_engine);
static Signal computer_move(SCE_Context* const ctx, SCE_Engine* const ptr_engine);
static bool deepen_depth(SCE_Engine* const ptr_engine, const int new_depth);

int main(int argc, char** argv) {
    if (argc == 1) {
        fprintf(stderr, "[-] Usage: sce_play white/black\n");
        return EXIT_FAILURE;
    }
    if ((strcmp(argv[1], "white") != 0) && (strcmp(argv[1], "black") != 0)) {
        fprintf(stderr, "[-] Usage: sce_play white/black\n");
        return EXIT_FAILURE;
    }

    SCE_Return ret;
    const PieceColor player = strcmp(argv[1], "white") == 0 ? WHITE : BLACK;

    SCE_Context ctx;
    ret = SCE_Context_init(&ctx);
    assert(ret == SCE_SUCCESS);

    // Chess engine
    SCE_Engine engine;
    ret = SCE_Engine_init(&ctx, &engine, SCE_Eval_SimplifiedEvaluationFunction, SCE_DeltaEval_SimplifiedEvaluationFunction, TT_TABLE_LOG_2_SIZE);
    assert(ret == SCE_SUCCESS);
    engine.depth = INITIAL_DEPTH;

    SCE_Chessboard_print(&ctx, player);
    printf("All moves are to be in \"E2E4\" form\n");
    
    while (true) {
        Signal signal;
        signal = player_move(&ctx, &engine);
        if (signal == SIGNAL_BREAK) break;
        if (signal == SIGNAL_CONTINUE) continue;

        SCE_Chessboard_print(&ctx, player);

        const uint64_t occupancy_wo_pawn = SCE_Chessboard_Occupancy(&ctx) ^ ctx.board.bitboards[W_PAWN] ^ ctx.board.bitboards[B_PAWN];
        const unsigned int n_pieces = COUNT_SET_BITS(occupancy_wo_pawn);
        if (n_pieces < N_PIECES_DEEPNING_CUTOFF) {
            if (deepen_depth(&engine, DEEPENED_DEPTH)) {
                printf("Warning: Engine search deepening! Will be harder on you :)\n");
            }
        }

        signal = computer_move(&ctx, &engine);
        if (signal == SIGNAL_BREAK) break;
        if (signal == SIGNAL_CONTINUE) continue;
        SCE_Chessboard_print(&ctx, player);
    }

    printf("End of game!\n");

    return 0;
}

static Signal player_move(SCE_Context* const ctx, SCE_Engine* const ptr_engine) {
    SCE_Return ret;
    char input[10] = { 0 };
    char src_an[3] = { 0 };
    char dst_an[3] = { 0 };
    SCE_ChessMoveList legal_move_list;
    ret = SCE_ChessMoveList_clear(&legal_move_list);
    assert(ret == SCE_SUCCESS);

    ret = SCE_GenerateLegalMoves(&legal_move_list, ctx);
    assert(ret == SCE_SUCCESS);

    if (legal_move_list.count == 0) return SIGNAL_BREAK;

    //printf("Eval: %0.2f\n", (float) SCE_Eval_SimplifiedEvaluationFunction(ctx) / 100);
    //SCE_Chessboard_print(ctx, player_color);

    // Get move from user
    printf("Your move: ");
    scanf("%s", input);
    strncpy(src_an, input, 2);
    strncpy(dst_an, input+2, 2);

    const int src_idx = SCE_AN_To_Idx(src_an);
    const int dst_idx = SCE_AN_To_Idx(dst_an);

    if (src_idx == UNASSIGNED || dst_idx == UNASSIGNED) {
        fprintf(stderr, "Wrong input! Try again\n");
        return SIGNAL_CONTINUE;
    }
    
    int move = UNASSIGNED;
    SCE_ChessMoveList movelist;
    ret = SCE_ChessMoveList_clear(&movelist);
    assert(ret == SCE_SUCCESS);
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
        return SIGNAL_CONTINUE;
    } else if (movelist.count == 1) {
        move = movelist.moves[0];
    } else {
        // Promotion
        unsigned int choice;
        printf("Available moves:\n");
        for (unsigned int i = 0U; i < movelist.count; i++) {
            //const unsigned int src_idx = movelist.moves[i] SCE_CHESSMOVE_GET_SRC;
            //const unsigned int dst_idx = movelist.moves[i] SCE_CHESSMOVE_GET_DST;
            const int flag = movelist.moves[i] SCE_CHESSMOVE_GET_FLAG;
            printf("%d: ", i);
            switch (flag) {
                case SCE_CHESSMOVE_FLAG_KNIGHT_PROMOTION:
                case SCE_CHESSMOVE_FLAG_KNIGHT_PROMO_CAPTURE:
                    printf("Promote to knight\n");
                    break;
                case SCE_CHESSMOVE_FLAG_BISHOP_PROMOTION:
                case SCE_CHESSMOVE_FLAG_BISHOP_PROMO_CAPTURE:
                    printf("Promote to bishop\n");
                    break;
                case SCE_CHESSMOVE_FLAG_ROOK_PROMOTION:
                case SCE_CHESSMOVE_FLAG_ROOK_PROMO_CAPTURE:
                    printf("Promote to rook\n");
                    break;
                case SCE_CHESSMOVE_FLAG_QUEEN_PROMOTION:
                case SCE_CHESSMOVE_FLAG_QUEEN_PROMO_CAPTURE:
                    printf("Promote to queen\n");
                    break;
                default:
                    break;
            }
        }
        printf("Choice: ");
        scanf("%d", &choice);
        if (choice >= movelist.count) {
            fprintf(stderr, "Wrong index!\n");
            return SIGNAL_CONTINUE;
        }
        move = movelist.moves[choice];
    }
    if ((move SCE_CHESSMOVE_GET_FLAG) & SCE_CHESSMOVE_FLAG_CAPTURE) {
        printf("(Capture)");
    }
    printf("\n");

    // Making player move.
    ret = SCE_MakeMove(ctx, move);
    assert(ret == SCE_SUCCESS);
    printf("Eval: %0.2f\n", (float) ptr_engine->eval_function(ctx) / 100);     // Note: This internally updates the score cache, hence necessary!

    return SIGNAL_OK;
}

static Signal computer_move(SCE_Context* const ctx, SCE_Engine* const ptr_engine) {
    // ------------------------------------------------
    // Now computer's perspective
    //move = SCE_Engine_AlphaBetaBestMove(&engine, &ctx);
    SCE_Return ret;
    SCE_ChessMove move;
    move = SCE_Engine_IterativeDeepeningAlphaBetaBestMove(ptr_engine, ctx);
    if (move == EMPTY_MOVE) {
        printf("Mate!\n");
        return SIGNAL_BREAK;
    }
    ret = SCE_MakeMove(ctx, move);
    assert(ret == SCE_SUCCESS);
    {
        char src_an[3] = { 0 };
        char dst_an[3] = { 0 };
        ret = SCE_Bitboard_To_AN(src_an, 1ULL << (move SCE_CHESSMOVE_GET_SRC));
        ret = SCE_Bitboard_To_AN(dst_an, 1ULL << (move SCE_CHESSMOVE_GET_DST));
        printf("Computer: %s -> %s", src_an, dst_an);
        if ((move SCE_CHESSMOVE_GET_FLAG) & SCE_CHESSMOVE_FLAG_CAPTURE) {
            printf(" (Capture)");
        }
        printf("\n");
    }
    printf("Eval: %0.2f\n", (float) ptr_engine->eval_function(ctx) / 100);     // Note: This internally updates the score cache, hence necessary!
    return SIGNAL_OK;
}

static bool deepen_depth(SCE_Engine* const ptr_engine, const int new_depth) {
    assert(ptr_engine != NULL);

    if (new_depth > ptr_engine->depth) {
        ptr_engine->depth = new_depth;
        return true;
    }
    return false;
}