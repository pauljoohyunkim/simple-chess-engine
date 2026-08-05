#ifndef SCE_MAGIC_H
#define SCE_MAGIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdalign.h>
#include <stdint.h>
#include "chess.h"

SCE_Return SCE_MagicTable_init(SCE_Precomputation_Tables* ptr_precomputation_tables);
uint64_t SCE_MagicTable_get_rook_attacks(int idx, uint64_t occupancy, const SCE_MagicTable * ptr_magic_table);
uint64_t SCE_MagicTable_get_bishop_attacks(int idx, uint64_t occupancy, const SCE_MagicTable * ptr_magic_table);
uint64_t SCE_MagicTable_get_queen_attacks(int idx, uint64_t occupancy, const SCE_MagicTable * ptr_magic_table);

#ifdef __cplusplus
}
#endif

#endif  // SCE_MAGIC_H
