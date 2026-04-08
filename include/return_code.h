#ifndef SCE_RETURN_CODE_H
#define SCE_RETURN_CODE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SCE_INVALID_MOVE = -2,
    SCE_INVALID_BOARD_STATE = -1,
    SCE_INVALID_PARAM = 0,
    SCE_SUCCESS = 1
} SCE_Return;

#define SCE_FAILURE -1

#ifdef __cplusplus
}
#endif

#endif  // SCE_RETURN_CODE_H
