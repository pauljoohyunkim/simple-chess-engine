#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "eval/sef.h"
#include "engine.h"
#include "eval/pst.h"
#include "helper.h"

const unsigned int npp_count_to_depth[] = {
    15,     // 0
    14,     // 1
    14,     // 2
    14,     // 3
    13,     // 4
    13,     // 5
    12,     // 6
    11,     // 7
    11,     // 8
    10,     // 9
    10,     // 10
    10,     // 11
    10,     // 12
    10,     // 13
    9,      // 14
    9,      // 15
    9,      // 16
};

SCE_Engine_SearchControl ctrl = {
    .start_depth = 1,
    .use_lmr = false,
    .lmr_bias = 0,
};

typedef enum {
    SIGNAL_OK = 0,
    SIGNAL_BREAK = 1,
    SIGNAL_CONTINUE = 2
} Signal;


#define TT_TABLE_LOG_2_SIZE 24
#define NPP_DEEPNING_CUTOFF 10
#define DEPTH_MOST_SHALLOW (npp_count_to_depth[sizeof(npp_count_to_depth) / sizeof(npp_count_to_depth[0]) - 1])
#define DEPTH_DEEPEST (npp_count_to_depth[0])
#define NPP_WEIGHT_ENDGAME_START (2500)

// Returns true if end of game.
static Signal player_move(SCE_Context* const ctx, SCE_Engine* const ptr_engine);
static Signal computer_move(SCE_Context* const ctx, SCE_Engine* const ptr_engine);
static Signal check_draw(SCE_Context* const ctx);
static void deepen(const SCE_Context* const ctx, SCE_Engine* const ptr_engine);
static bool deepen_depth(SCE_Context* const ctx, const int new_depth);
static unsigned int calculate_npp(const SCE_Context* const ctx);

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

    SCE_Precomputation_Tables precomputation_tables;
    ret = SCE_Precomputation_Tables_init(&precomputation_tables, NULL);

    SCE_Context ctx;
    ret = SCE_Context_init(&ctx, &precomputation_tables);
    assert(ret == SCE_SUCCESS);
    ctx.depth = DEPTH_MOST_SHALLOW;

    // Chess engine
    SCE_Engine engine;
    ret = SCE_Engine_init(&ctx, &engine, SCE_Eval_SimplifiedEvaluationFunction, SCE_DeltaEval_SimplifiedEvaluationFunction, TT_TABLE_LOG_2_SIZE);
    assert(ret == SCE_SUCCESS);

    printf("All moves are to be in \"E2E4\" form (For promotions, you do not specify the ending, as you will be given the choice)\n");

    if (player == WHITE) {
        while (true) {
            Signal signal;

            SCE_Chessboard_print(&ctx, player);

            deepen(&ctx, &engine);

            do_white_player_move:
            signal = player_move(&ctx, &engine);
            if (signal == SIGNAL_BREAK) break;
            if (signal == SIGNAL_CONTINUE) goto do_white_player_move;
            if (check_draw(&ctx)) break;

            SCE_Chessboard_print(&ctx, player);

            deepen(&ctx, &engine);

            signal = computer_move(&ctx, &engine);
            if (signal == SIGNAL_BREAK) break;
            if (signal == SIGNAL_CONTINUE) continue;
            if (check_draw(&ctx)) break;
        }
    } else {
        while (true) {
            Signal signal;

            SCE_Chessboard_print(&ctx, player);
            deepen(&ctx, &engine);

            signal = computer_move(&ctx, &engine);
            if (signal == SIGNAL_BREAK) break;
            if (signal == SIGNAL_CONTINUE) continue;
            if (check_draw(&ctx)) break;

            SCE_Chessboard_print(&ctx, player);
            deepen(&ctx, &engine);

            do_black_player_move:
            signal = player_move(&ctx, &engine);
            if (signal == SIGNAL_BREAK) break;
            if (signal == SIGNAL_CONTINUE) goto do_black_player_move;
            if (check_draw(&ctx)) break;
        }

    }

    printf("End of game!\n");
    SCE_Engine_release(&engine);

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
    if (strcmp(input, "back") == 0) {
        if (ctx->board.history.count >= 2) {
            ret = SCE_UnmakeMove(ctx);  // Undo computer move
            ret = SCE_UnmakeMove(ctx);  // Undo human move
        }
        return SIGNAL_CONTINUE;
    }
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
    if (calculate_npp(ctx) < NPP_WEIGHT_ENDGAME_START) ctrl.use_lmr = false;
    move = SCE_Engine_IterativeDeepeningAlphaBetaBestMove(ptr_engine, ctx, &ctrl);

    if (move == EMPTY_MOVE) {
        // Double checking if there are absolutely no legal moves.
        SCE_ChessMoveList legal_moves;
        ret = SCE_ChessMoveList_clear(&legal_moves);
        assert(ret == SCE_SUCCESS);
        if (legal_moves.count == 0) {
            printf("Mate!\n");
            return SIGNAL_BREAK;
        } else {
            // Search failure happened!
            // Fallback: Full search
            SCE_Engine_SearchControl full_ctrl = {
                .start_depth = 1,
                .use_lmr = false
            };
            move = SCE_Engine_IterativeDeepeningAlphaBetaBestMove(ptr_engine, ctx, &full_ctrl);

            if (move == EMPTY_MOVE) {
                // Somehow the search failed. Choosing the first move in order not to crash.
                printf("info string Search complete failure. Choosing the first legal move.\n");
                move = legal_moves.moves[0];
            }
        }
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

static Signal check_draw(SCE_Context* const ctx) {
    assert(ctx != NULL);

    // Draw by repetition
    if (ctx->board.history.count >= 2) {
        unsigned int rep_count = 1;
        for (int i = ctx->board.history.count - 2; i >= (int)ctx->board.history.count - (int)ctx->board.half_move_clock; i -= 2) {
            if (ctx->board.undo_states[i].zobrist_hash == ctx->board.zobrist_hash) {
                rep_count++;
            }
        }
        if (rep_count == 3) {
            printf("Draw by repetition.\n");
            return SIGNAL_BREAK;
        }
    }

    // Fifty-move rule
    if (ctx->board.half_move_clock >= 100) {
        printf("Draw by fifty-move rule.\n");
        return SIGNAL_BREAK;
    }

    // Stalemate
    {
        SCE_ChessMoveList movelist;
        SCE_Return ret = SCE_ChessMoveList_clear(&movelist);
        assert(ret == SCE_SUCCESS);
        ret = SCE_GenerateLegalMoves(&movelist, ctx);

        if (movelist.count == 0) {
            // Check if under attack. If not, stalemate.
            if (ctx->board.to_move == WHITE && !SCE_IsSquareAttacked(ctx, ctx->board.bitboards[W_KING], BLACK)) return true;
            if (ctx->board.to_move == BLACK && !SCE_IsSquareAttacked(ctx, ctx->board.bitboards[B_KING], WHITE)) return true;
        }
    }
    if (SCE_DetectInsufficientMaterial(ctx)) {
        printf("Draw by insufficient material.\n");
        return SIGNAL_BREAK;
    }

    return SIGNAL_OK;
}

static void deepen(const SCE_Context* const ctx, SCE_Engine* const ptr_engine) {
    assert(ptr_engine != NULL);

    // Clamp npp_count to below 16
    unsigned int npp_count = COUNT_SET_BITS(SCE_Chessboard_Occupancy(ctx) & ~(ctx->board.bitboards[W_PAWN] | ctx->board.bitboards[B_PAWN]));
    npp_count = npp_count > 16U ? 16U : npp_count;

    const bool deepened = deepen_depth(ctx, npp_count_to_depth[npp_count]);
    if (deepened) {
        printf("Info: Engine search deepening to depth %d!\n", npp_count_to_depth[npp_count]);
    }
}

static bool deepen_depth(SCE_Context* const ctx, const int new_depth) {
    assert(ctx != NULL);

    if (new_depth > ctx->depth) {
        ctx->depth = new_depth;
        return true;
    }
    return false;
}

static unsigned int calculate_npp(const SCE_Context* const ctx) {
    assert(ctx != NULL);

    unsigned int val = 0;
    val += KNIGHT_WEIGHT * COUNT_SET_BITS(ctx->board.bitboards[W_KNIGHT] | ctx->board.bitboards[B_KNIGHT]);
    val += BISHOP_WEIGHT * COUNT_SET_BITS(ctx->board.bitboards[W_BISHOP] | ctx->board.bitboards[B_BISHOP]);
    val += ROOK_WEIGHT * COUNT_SET_BITS(ctx->board.bitboards[W_ROOK] | ctx->board.bitboards[B_ROOK]);
    val += QUEEN_WEIGHT * COUNT_SET_BITS(ctx->board.bitboards[W_QUEEN] | ctx->board.bitboards[B_QUEEN]);

    return val;
}