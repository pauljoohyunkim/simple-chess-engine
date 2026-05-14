#ifndef SCE_MAGIC_H
#define SCE_MAGIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdalign.h>
#include <stdint.h>
#include "chess.h"

typedef struct {
    uint64_t premask;
    uint64_t magic;
    uint8_t bits_used;
} __attribute__((aligned(64))) SCE_Magic;

typedef struct {
    alignas(64) SCE_Magic RookMagicTable[CHESSBOARD_N_SQUARES][(1<<12)];
    alignas(64) SCE_Magic BishopMagicTable[CHESSBOARD_N_SQUARES][(1<<10)];
} SCE_MagicTable;

SCE_Return SCE_MagicTable_init(SCE_MagicTable* const ptr_magic_table);

#ifdef __cplusplus
}
#endif

#endif  // SCE_MAGIC_H
