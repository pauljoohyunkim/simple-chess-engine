#ifndef SCE_MAGIC_H
#define SCE_MAGIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdalign.h>
#include <stdint.h>
#include "chess.h"

SCE_Return SCE_MagicTable_init(SCE_Precomputation_Tables* const ptr_precomputation_tables);

#ifdef __cplusplus
}
#endif

#endif  // SCE_MAGIC_H
