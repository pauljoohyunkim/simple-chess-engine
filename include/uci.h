#ifndef SCE_UCI_H
#define SCE_UCI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "chess.h"
#include "engine.h"

// A struct holding pointers for thread operations
typedef struct {
    pthread_mutex_t stdout_mutex;
    pthread_mutex_t context_mutex;
    SCE_Context* ctx;
    SCE_Engine* ptr_engine;
} SCE_UCI_Session;

typedef enum {
    SEARCH_TASK_MASTER,
    SEARCH_TASK_HELPER
} SCE_UCI_SearchTaskWorkerRole;

typedef struct {
    pthread_mutex_t* ptr_stdout_mutex;
    SCE_Engine* ptr_engine;
    SCE_Context ctx;
    SCE_UCI_SearchTaskWorkerRole role;
} SCE_UCI_SearchTask;

/**
 * @brief Convert SCE_ChessMove to UCI move string
 * 
 * @param move Chess move used internally in SCE.
 * @param uci_string Output string
 * @return true Success
 * @return false Failure (Possibly not a correct move, or uci_string is empty)
 * 
 * Note that this simply constructs src + dst + promotion.
 * This does not encode information about castling or en passant or any other special moves.
 * Note that EMPTY_MOVE is handled, and will return true.
 */
bool SCE_MoveToUCIString(const SCE_ChessMove move, char uci_string[6]);

/**
 * @brief Convert UCI move string to SCE_ChessMove
 * 
 * @param uci_string UCI string
 * @return SCE_ChessMove Move parsed from UCI string, or EMPTY_MOVE (0)
 */
SCE_ChessMove SCE_UCIStringToMove(const char* const uci_string);

/**
 * @brief Parse position line
 * 
 * @param ctx Pointer to the SCE_Context struct.
 * @param line The entire line from position command.
 * @return SCE_Return SCE_SUCCESS if successful. Otherwise if failure.
 */
SCE_Return SCE_UCI_ParsePosition(SCE_Context* const ctx, const char* const line);

/**
 * @brief Parse go line
 * 
 * @param session Pointer to the SCE_UCI_Session struct, holding pointers to context and engine.
 * @param line The entire line from position command.
 * @return SCE_Return SCE_SUCCESS if successful. Otherwise if failure.
 */
SCE_Return SCE_UCI_ParseGo(SCE_UCI_Session* const session, const char* const line);

#ifdef __cplusplus
}
#endif

#endif  // SCE_UCI_H
