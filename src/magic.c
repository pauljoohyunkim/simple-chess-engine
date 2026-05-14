#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "magic.h"
#include "chess.h"
#include "helper.h"

typedef unsigned int uint;

static uint64_t get_sparse_uint64_t(uint64_t seed);

static uint64_t get_sparse_uint64_t(uint64_t seed) {
    const uint64_t x = xorshift(seed);
    const uint64_t y = xorshift(x);
    const uint64_t z = xorshift(y);
    return x & y & z;
}

SCE_Return SCE_MagicTable_init(SCE_Precomputation_Tables* const ptr_precomputation_tables) {
    if (ptr_precomputation_tables == NULL) return SCE_INVALID_PARAM;

    uint64_t occupancy_variations[1<<12] = { 0 };
    size_t count = 0;
    uint64_t current_occ;

    srand(time(NULL));

    /*
    // Pick a nonzero random seed value.
    uint64_t x = 0;
    while (x == 0) x = (uint64_t) rand();

    memset(&ptr_precomputation_tables->magic_table, 0, sizeof(SCE_MagicTable));

    // Rooks
    for (uint idx = 0; idx < CHESSBOARD_N_SQUARES; idx++) {
        while (true) {
            x = xorshift(x);    // For each iteration, make sure different seed is used for getting sparse uint64_t
            const uint64_t magic_candidate = get_sparse_uint64_t(x);

            uint64_t premask = (ptr_precomputation_tables->pm_table.rays[NORTH][idx] & ~RANK_8_MASK) | 
                               (ptr_precomputation_tables->pm_table.rays[SOUTH][idx] & ~RANK_1_MASK) |
                               (ptr_precomputation_tables->pm_table.rays[EAST][idx] & ~H_MASK) |
                               (ptr_precomputation_tables->pm_table.rays[WEST][idx] & ~A_MASK);

            // Generate all "subset" of premask.
            do {
                occupancy_variations[count] = current_occ;
                count++;
                current_occ = (current_occ - premask) & premask;
            } while (current_occ != 0);


            
        }
    }
    */

    return SCE_SUCCESS;
}