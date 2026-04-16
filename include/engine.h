#ifndef SCE_ENGINE_H
#define SCE_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "chess.h"

typedef SCE_Return (*SCE_Eval)(SCE_Chessboard* const);

typedef struct {
    SCE_Eval eval_function;

} SCE_Engine;

#ifdef __cplusplus
}
#endif
#endif  // SCE_ENGINE_H
