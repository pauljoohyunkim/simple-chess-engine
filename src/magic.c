#include <string.h>
#include "magic.h"
#include "chess.h"
#include "helper.h"

SCE_Return SCE_MagicTable_init(SCE_MagicTable* const ptr_magic_table) {
    if (ptr_magic_table == NULL) return SCE_INVALID_PARAM;

    memset(ptr_magic_table, 0, sizeof(SCE_MagicTable));

    return SCE_SUCCESS;
}