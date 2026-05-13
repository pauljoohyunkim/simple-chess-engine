#ifndef SCE_RETURN_CODE_H
#define SCE_RETURN_CODE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Return codes used throughout the chess engine
 */
typedef enum {
    SCE_MOVELIST_EMPTY = -4,    /**< Move list is empty */
    SCE_INVALID_MOVE = -3,      /**< Invalid move attempted */
    SCE_INVALID_BOARD_STATE = -2, /**< Invalid board state */
    SCE_INVALID_PARAM = -1,     /**< Invalid parameter passed to function */
    SCE_INTERNAL_ERROR = 0,     /**< Internal error occurred */
    SCE_SUCCESS = 1             /**< Operation completed successfully */
} SCE_Return;

#ifdef __cplusplus
}
#endif

#endif  // SCE_RETURN_CODE_H
