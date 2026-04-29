#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "chess.h"
#include "engine.h"
#include "eval/sef.h"
#include "uci.h"

#define TT_TABLE_LOG_2_SIZE 24

int main(int argc, char** argv) {
    SCE_Context ctx;
    SCE_Return ret = SCE_Context_init(&ctx);
    assert(ret == SCE_SUCCESS);

    SCE_Engine engine;
    ret = SCE_Engine_init(&ctx, &engine, SCE_Eval_SimplifiedEvaluationFunction, SCE_DeltaEval_SimplifiedEvaluationFunction, TT_TABLE_LOG_2_SIZE);
    assert(ret == SCE_SUCCESS);

    SCE_UCI_Session session = {
        .stdout_mutex = PTHREAD_MUTEX_INITIALIZER,
        .context_mutex = PTHREAD_MUTEX_INITIALIZER,
        .ctx = &ctx,
        .ptr_engine = &engine,
        .n_helper_threads = 0
    };

    char line[BUFSIZ] = { 0 };
    while (fgets(line, sizeof(line)-1, stdin)) {
        if (strncmp(line, "uci", 3) == 0) {
            pthread_mutex_lock(&session.stdout_mutex);
            printf("id name SimpleChessEngine\n");
            printf("id author Paul Joo-Hyun Kim\n");
            printf("uciok\n");
            pthread_mutex_unlock(&session.stdout_mutex);
            continue;
        }
        if (strncmp(line, "isready", 7) == 0) {
            pthread_mutex_lock(&session.stdout_mutex);
            printf("readyok\n");
            pthread_mutex_unlock(&session.stdout_mutex);
            continue;
        }
        if (strncmp(line, "position", 8) == 0) {
            SCE_UCI_ParsePosition(&ctx, line);
            continue;
        }
        if (strncmp(line, "go", 2) == 0) {
            SCE_UCI_ParseGo(&session, line);
            continue;
        }
        if (strncmp(line, "print", 5) == 0) {
            pthread_mutex_lock(&session.stdout_mutex);
            SCE_Chessboard_print(&ctx, WHITE);
            pthread_mutex_unlock(&session.stdout_mutex);
            continue;
        }
        if (strncmp(line, "quit", 4) == 0) {
            break;
        }
        fflush(stdout);
    }

    SCE_Engine_release(&engine);
}
