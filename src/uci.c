#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include "chess.h"
#include "uci.h"
#include "fen.h"

typedef unsigned int uint;

static void* SCE_Search_Thread_Wrapper(void* ptr_engine);

#define PROMO_TYPE_KNIGHT 0
#define PROMO_TYPE_BISHOP 1
#define PROMO_TYPE_ROOK 2
#define PROMO_TYPE_QUEEN 3
bool SCE_MoveToUCIString(const SCE_ChessMove move, char uci_string[6]) {
    if (uci_string == NULL) return false;
    memset(uci_string, 0, 6);
    if (move == EMPTY_MOVE) {
        memset(uci_string, '0', 4);
        return true;
    }

    const uint src_idx = move SCE_CHESSMOVE_GET_SRC;
    const uint dst_idx = move SCE_CHESSMOVE_GET_DST;
    const int flag = move SCE_CHESSMOVE_GET_FLAG;

    SCE_Return ret = SCE_Bitboard_To_AN(uci_string, (1ULL << src_idx));
    if (ret != SCE_SUCCESS) return false;
    ret = SCE_Bitboard_To_AN(&uci_string[2], (1ULL << dst_idx));
    if (ret != SCE_SUCCESS) return false;

    if (flag & SCE_CHESSMOVE_FLAG_FILTER_PROMOTION) {
        const uint promo_type_idx = flag & ~(SCE_CHESSMOVE_FLAG_FILTER_PROMOTION | SCE_CHESSMOVE_FLAG_CAPTURE);
        switch (promo_type_idx) {
            case PROMO_TYPE_KNIGHT:
                uci_string[4] = 'n';
                break;
            case PROMO_TYPE_BISHOP:
                uci_string[4] = 'b';
                break;
            case PROMO_TYPE_ROOK:
                uci_string[4] = 'r';
                break;
            case PROMO_TYPE_QUEEN:
                uci_string[4] = 'q';
                break;
            default:
                return false;
        }
    }

    // By default, UCI expects lowercase strings.
    for (uint i = 0; i < 6; i++) {
        uci_string[i] = tolower(uci_string[i]);
    }

    return true;
}

SCE_ChessMove SCE_UCIStringToMove(const char* const uci_string) {
    // UCI string has to be 4 or 5 (for promotion)
    if (strlen(uci_string) < 4) return EMPTY_MOVE;
    if (strlen(uci_string) > 5) return EMPTY_MOVE;

    char src_an[3] = { 0 };
    char dst_an[3] = { 0 };
    strncpy(src_an, uci_string, 2);
    strncpy(dst_an, &uci_string[2], 2);

    const int src_idx = SCE_AN_To_Idx(src_an);
    const int dst_idx = SCE_AN_To_Idx(dst_an);
    if (src_idx == UNASSIGNED || dst_idx == UNASSIGNED) return EMPTY_MOVE;

    int flag = 0;
    if (strlen(uci_string) == 5) {
        switch (uci_string[4]) {
            case 'n':
                flag = SCE_CHESSMOVE_FLAG_KNIGHT_PROMOTION;
                break;
            case 'b':
                flag = SCE_CHESSMOVE_FLAG_BISHOP_PROMOTION;
                break;
            case 'r':
                flag = SCE_CHESSMOVE_FLAG_ROOK_PROMOTION;
                break;
            case 'q':
                flag = SCE_CHESSMOVE_FLAG_QUEEN_PROMOTION;
                break;
            default:
                return EMPTY_MOVE;
        }
    }

    const SCE_ChessMove move = (src_idx SCE_CHESSMOVE_SET_SRC) | (dst_idx SCE_CHESSMOVE_SET_DST) | (flag SCE_CHESSMOVE_SET_FLAG);

    return move;
}

SCE_Return SCE_UCI_ParsePosition(SCE_Context* const ctx, const char* const line) {
    if (ctx == NULL || line == NULL) return SCE_INVALID_PARAM;
    if (strncmp(line, "position ", 9) != 0) return SCE_INVALID_PARAM;

    char line_cpy[BUFSIZ] = { 0 };
    strncpy(line_cpy, line, sizeof(line_cpy)-1);
    {
        // Replace newline with '\0'
        char* pos = strchr(line_cpy, '\n');
        if (pos) {
            *pos = '\0';
        }
    }

    // Separate out mandatory part and optional part
    char* const moves_substr = strstr(line_cpy, "moves");
    if (moves_substr) {
        *(moves_substr-1) = '\0';
    }

    // Mandatory Part
    {
        char* saveptr = NULL;
        // "position"
        char* word = strtok_r(line_cpy, " ", &saveptr);
        // "fen" | "startpos"
        word = strtok_r(NULL, " ", &saveptr);
        if (strcmp(word, "fen") == 0) {
            char* fen_str = strtok_r(NULL, " ", &saveptr);
            SCE_Return ret = SCE_Chessboard_FEN_setup(ctx, fen_str);
            if (ret != SCE_SUCCESS) {
                return SCE_INVALID_PARAM;
            }
        } else if (strcmp(word, "startpos") == 0) {
            SCE_Return ret = SCE_Chessboard_FEN_setup(ctx, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            if (ret != SCE_SUCCESS) {
                return SCE_INTERNAL_ERROR;
            }
        } else {
            return SCE_INVALID_PARAM;
        }
    }

    // Optional Moves
    if (moves_substr) {
        char moves_substr_cpy[BUFSIZ] = { 0 };
        strcpy(moves_substr_cpy, &moves_substr[6]);

        char* saveptr;
        char* move_str = strtok_r(moves_substr_cpy, " ", &saveptr);
        while (move_str) {
            const SCE_ChessMove ordered_move = SCE_UCIStringToMove(move_str);
            if (ordered_move == EMPTY_MOVE) continue;
            const uint ordered_move_src_idx = ordered_move SCE_CHESSMOVE_GET_SRC;
            const uint ordered_move_dst_idx = ordered_move SCE_CHESSMOVE_GET_DST;
            const int ordered_move_flag = ordered_move SCE_CHESSMOVE_GET_FLAG;

            // Before making move, need to generate legal moves and compare.
            SCE_ChessMoveList movelist;
            SCE_Return ret = SCE_ChessMoveList_clear(&movelist);
            if (ret != SCE_SUCCESS) return SCE_INTERNAL_ERROR;
            ret = SCE_GenerateLegalMoves(&movelist, ctx);
            if (ret != SCE_SUCCESS) return SCE_INVALID_PARAM;

            if (movelist.count == 0) return SCE_INVALID_PARAM;

            for (uint i = 0; i < movelist.count; i++) {
                const SCE_ChessMove move = movelist.moves[i];
                const uint src_idx = move SCE_CHESSMOVE_GET_SRC;
                const uint dst_idx = move SCE_CHESSMOVE_GET_DST;
                const int flag = move SCE_CHESSMOVE_GET_FLAG;

                if (src_idx != ordered_move_src_idx) continue;
                if (dst_idx != ordered_move_dst_idx) continue;
                if ((ordered_move_flag & SCE_CHESSMOVE_FLAG_FILTER_PROMOTION) & (flag & SCE_CHESSMOVE_FLAG_FILTER_PROMOTION)) {
                    // TODO: Promotion handling
                    // Check if last two bits are equivalent.
                    if ((ordered_move_flag & 3) != (flag & 3)) continue;

                    ret = SCE_MakeMove(ctx, move);
                    if (ret != SCE_SUCCESS) return SCE_INTERNAL_ERROR;
                    break;
                } else {
                    ret = SCE_MakeMove(ctx, move);
                    if (ret != SCE_SUCCESS) return SCE_INTERNAL_ERROR;
                    break;
                }
            }

            move_str = strtok_r(NULL, " ", &saveptr);
        }

    }

    return SCE_SUCCESS;
}

static void* SCE_Search_Thread_Wrapper(void* arg) {
    if (arg == NULL) return NULL;
    SCE_UCI_SearchTask* task = (SCE_UCI_SearchTask*) arg;

    // Run the search here.
    const SCE_ChessMove move = SCE_Engine_IterativeDeepeningAlphaBetaBestMove(task->ptr_engine, &task->ctx);
    char uci_str[6] = { 0 };
    if (SCE_MoveToUCIString(move, uci_str)) {
        pthread_mutex_lock(task->ptr_stdout_mutex);
        printf("Role: %s\n", task->role == SEARCH_TASK_MASTER ? "master" : "helper");
        printf("Visited: %d\n", task->ctx.nodes_visited);
        printf("bestmove %s\n", uci_str);
        pthread_mutex_unlock(task->ptr_stdout_mutex);
    }
    free(task);

    return NULL;
}

SCE_Return SCE_UCI_ParseGo(SCE_UCI_Session* const session, const char* const line) {
    if (session == NULL || line == NULL) return SCE_INVALID_PARAM;
    if (strncmp(line, "go", 2) != 0) return SCE_INVALID_PARAM;
    char line_cpy[BUFSIZ] = { 0 };
    strncpy(line_cpy, line, sizeof(line_cpy)-1);
    {
        // Replace newline with '\0'
        char* pos = strchr(line_cpy, '\n');
        if (pos) {
            *pos = '\0';
        }
    }

    int depth;
    {
        // TODO: Parse other options
        // For now only parse depth command.
        char* saveptr = NULL;
        char* word = strtok_r(line_cpy, " ", &saveptr);         // "go"
        word = strtok_r(NULL, " ", &saveptr);
        if (word && (strcmp(word, "depth") == 0)) {
            // Depth
            word = strtok_r(NULL, " ", &saveptr);
            depth = atoi(word);
        }
    }

    // Create task on heap for thread
    SCE_UCI_SearchTask* task = (SCE_UCI_SearchTask*) malloc(sizeof(SCE_UCI_SearchTask));
    //SCE_UCI_SearchTask* task2 = (SCE_UCI_SearchTask*) malloc(sizeof(SCE_UCI_SearchTask));
    // Copying context
    pthread_mutex_lock(&session->context_mutex);
    memcpy(&task->ctx, session->ctx, sizeof(SCE_Context));
    //memcpy(&task2->ctx, session->ctx, sizeof(SCE_Context));
    pthread_mutex_unlock(&session->context_mutex);
    task->ctx.depth = depth;
    task->ctx.nodes_visited = 0;
    task->ptr_engine = session->ptr_engine;
    task->ptr_stdout_mutex = &session->stdout_mutex;
    task->role = SEARCH_TASK_MASTER;
    //task2->ctx.depth = depth+1;
    //task2->ptr_engine = session->ptr_engine;
    //task2->ptr_stdout_mutex = &session->stdout_mutex;
    //task2->role = SEARCH_TASK_HELPER;
    //task2->ctx.nodes_visited = 0;

    session->ptr_engine->stop_searching = false;
    pthread_t search_thread;
    //pthread_t search_thread2;
    pthread_create(&search_thread, NULL, SCE_Search_Thread_Wrapper, (void*) task);
    //pthread_create(&search_thread2, NULL, SCE_Search_Thread_Wrapper, (void*) task2);
    pthread_detach(search_thread);
    //pthread_detach(search_thread2);

    return SCE_SUCCESS;
}
