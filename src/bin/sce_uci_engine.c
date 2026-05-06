#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "chess.h"
#include "engine.h"
#include "eval/sef.h"
#include "eval/hcef.h"
#include "uci.h"


static const SCE_Engine_SearchControl master_ctrl_initial = {
    .use_lmr = false,
    .start_depth = 1,
    .lmr_bias = 0,
};

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    SCE_Context ctx;

    SCE_Precomputation_Tables precomputation_tables;
    #ifdef NODE_COUNT
    uint64_t seed = 1U;
    SCE_Return ret = SCE_Precomputation_Tables_init(&precomputation_tables, &seed);
    #else
    SCE_Return ret = SCE_Precomputation_Tables_init(&precomputation_tables, NULL);
    #endif
    assert(ret == SCE_SUCCESS);

    ret = SCE_Context_init(&ctx, &precomputation_tables);
    assert(ret == SCE_SUCCESS);

    SCE_Engine engine;
    ret = SCE_Engine_init(&ctx, &engine, SCE_Eval_HandcraftedEvaluationFunction, SCE_DeltaEval_HandcraftedEvaluationFunction, UCI_TT_TABLE_LOG_2_SIZE);
    assert(ret == SCE_SUCCESS);

    SCE_Engine_SearchControl master_ctrl = master_ctrl_initial;

    SCE_UCI_Session session = {
        .stdout_mutex = PTHREAD_MUTEX_INITIALIZER,
        .context_mutex = PTHREAD_MUTEX_INITIALIZER,
        .ctx = &ctx,
        .ptr_engine = &engine,
        .n_helper_threads = 4,
        .ptr_master_ctrl = &master_ctrl,
        .use_dynamic_deepening = false
    };


    char line[BUFSIZ] = { 0 };
    while (fgets(line, sizeof(line)-1, stdin)) {
        if (strncmp(line, "ucinewgame", 10) == 0) {
            ret = SCE_Engine_release(&engine);
            assert(ret == SCE_SUCCESS);
            ret = SCE_Context_init(&ctx, &precomputation_tables);
            assert(ret == SCE_SUCCESS);
            ret = SCE_Engine_init(&ctx, &engine, SCE_Eval_SimplifiedEvaluationFunction, SCE_DeltaEval_SimplifiedEvaluationFunction, UCI_TT_TABLE_LOG_2_SIZE);
            assert(ret == SCE_SUCCESS);
            master_ctrl = master_ctrl_initial;
        } else if (strncmp(line, "uci", 3) == 0) {
            pthread_mutex_lock(&session.stdout_mutex);
            printf("id name SimpleChessEngine\n");
            printf("id author Paul Joo-Hyun Kim\n");
            printf("option name DynamicDeepening type check default false\n");
            printf("option name EvalFunc type spin default 1 min 0 max 1\n");
            printf("uciok\n");
            pthread_mutex_unlock(&session.stdout_mutex);
            continue;
        } else if (strncmp(line, "setoption", 9) == 0) {
            ret = SCE_UCI_ParseSetoption(&session, line);
            continue;
        } else if (strncmp(line, "isready", 7) == 0) {
            pthread_mutex_lock(&session.stdout_mutex);
            printf("readyok\n");
            pthread_mutex_unlock(&session.stdout_mutex);
            continue;
        } else if (strncmp(line, "position", 8) == 0) {
            ret = SCE_UCI_ParsePosition(&ctx, line);
            continue;
        } else if (strncmp(line, "go", 2) == 0) {
            ret = SCE_UCI_ParseGo(&session, line);
            continue;
        } else if (strncmp(line, "print", 5) == 0) {
            pthread_mutex_lock(&session.stdout_mutex);
            ret = SCE_Chessboard_print(&ctx, WHITE);
            assert(ret = SCE_SUCCESS);
            pthread_mutex_unlock(&session.stdout_mutex);
            continue;
        } else if (strncmp(line, "quit", 4) == 0) {
            break;
        }
        //fflush(stdout);
    }

    SCE_Engine_release(&engine);
}
