#ifdef DEBUG

#ifndef SCE_DEV_H
#define SCE_DEV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "chess.h"

typedef unsigned int uint;

void print_as_board(const uint64_t val);

// Returns 1 for success, 0 for failure
int place_piece_on_board(SCE_Chessboard* const ptr_board, const char * const an, uint piece_type);

#ifdef __cplusplus
}
#endif

#endif  // SCE_DEV_H
#endif  // DEBUG
