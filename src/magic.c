#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "magic.h"
#include "chess.h"
#include "helper.h"

typedef unsigned int uint;

static uint64_t get_random_uint64_t();
static uint64_t get_sparse_uint64_t();
static uint64_t slow_slider_attacks(int sq, uint64_t occupancy, const int directions[], int dir_count);
static void write_magic_bishop(int idx, SCE_Precomputation_Tables* ptr_precomputation_tables);
static void write_magic_rook(int idx, SCE_Precomputation_Tables* ptr_precomputation_tables);

static uint64_t seed = 0;

static uint64_t get_random_uint64_t() {
    seed = xorshift(seed);
    return seed;
}

// This function does not return zero.
static uint64_t get_sparse_uint64_t() {
    uint64_t result = 0U;
    while (result == 0) {
        const uint64_t x = get_random_uint64_t();
        const uint64_t y = get_random_uint64_t();
        const uint64_t z = get_random_uint64_t();
        result = x & y & z;
    }
    return result;
}

static uint64_t slow_slider_attacks(int idx, uint64_t occupancy, const int directions[], int dir_count) {
    uint64_t attacks = 0ULL;
    int r = idx / CHESSBOARD_DIMENSION;
    int c = idx % CHESSBOARD_DIMENSION;

    for (int d = 0; d < dir_count; d++) {
        int dir = directions[d];
        int curr_r = r, curr_c = c;
        
        while (true) {
            // Move one step in the direction
            switch (dir) {
                case NORTH:
                    curr_r++;
                    break;
                case SOUTH:
                    curr_r--;
                    break;
                case EAST:
                    curr_c++;
                    break;
                case WEST:
                    curr_c--;
                    break;
                case NORTHEAST:
                    curr_r++;
                    curr_c++;
                    break;
                case NORTHWEST:
                    curr_r++;
                    curr_c--;
                    break;
                case SOUTHEAST:
                    curr_r--;
                    curr_c++;
                    break;
                case SOUTHWEST:
                    curr_r--;
                    curr_c--;
                    break;
                default:
                    return 0U;
            }
            
            if (curr_r < 0 || curr_r > 7 || curr_c < 0 || curr_c > 7) break;

            int target_sq = curr_r * CHESSBOARD_DIMENSION + curr_c;
            attacks |= (1ULL << target_sq);

            // If we hit a piece, we stop (but the square itself is included as an attack)
            if (occupancy & (1ULL << target_sq)) break;
        }
    }
    return attacks;
}

static uint64_t compute_magic_with_premask_and_direction(int idx, uint64_t premask, const int directions[], int dir_count) {
    const size_t n_bits = COUNT_SET_BITS(premask);
    const size_t shift = CHESSBOARD_N_SQUARES - n_bits;

    uint64_t occupancy_variations[1<<12] = { 0 };
    size_t count = 0;
    uint64_t reference_attacks[1<<12] = { 0 };
    uint64_t test_table[1<<12] = { 0 };
    // Generate all "subset" of premask.
    {
        uint64_t current_occ = 0;

        do {
            occupancy_variations[count] = current_occ;
            count++;
            current_occ = (current_occ - premask) & premask;
        } while (current_occ != 0);
    }
    // Precompute all reference attacks
    for (unsigned int i = 0; i < (1 << n_bits); i++) {
        reference_attacks[i] = slow_slider_attacks(idx, occupancy_variations[i], directions, dir_count);
    }

    while (true) {
        const uint64_t magic_candidate = get_sparse_uint64_t();
        memset(test_table, 0, sizeof(test_table));
        bool collision = false;

        for (unsigned int i = 0; i < (1 << n_bits); i++) {
            const unsigned int hash = (occupancy_variations[i] * magic_candidate) >> shift;

            if (test_table[hash] == 0) {
                test_table[hash] = reference_attacks[i];
            } else if (test_table[hash] != reference_attacks[i]) {
                collision = true;
                break;
            }
        }

        if (!collision) return magic_candidate;
    }

}

static void write_magic_bishop(int idx, SCE_Precomputation_Tables* ptr_precomputation_tables) {
    const int directions[] = { NORTHEAST, NORTHWEST, SOUTHEAST, SOUTHWEST };
    const uint64_t premask = (ptr_precomputation_tables->pm_table.rays[NORTHEAST][idx] & ~RANK_8_MASK & ~H_MASK) | 
                             (ptr_precomputation_tables->pm_table.rays[NORTHWEST][idx] & ~RANK_8_MASK & ~A_MASK) |
                             (ptr_precomputation_tables->pm_table.rays[SOUTHEAST][idx] & ~RANK_1_MASK & ~H_MASK) |
                             (ptr_precomputation_tables->pm_table.rays[SOUTHWEST][idx] & ~RANK_1_MASK & ~A_MASK);
    const unsigned int n_bits = COUNT_SET_BITS(premask);
    const uint64_t magic = compute_magic_with_premask_and_direction(idx, premask, directions, 4);

    ptr_precomputation_tables->magic_table.bishop[idx].premask = premask;
    ptr_precomputation_tables->magic_table.bishop[idx].magic = magic;
    ptr_precomputation_tables->magic_table.bishop[idx].shift = (CHESSBOARD_N_SQUARES - n_bits);

    uint64_t* base_ptr = &ptr_precomputation_tables->magic_table.BishopMagicTable[idx * (1<<9)];

    uint64_t occupancy = 0U;
    do {
        uint hash = ((occupancy & premask) * magic) >> (CHESSBOARD_N_SQUARES - n_bits);

        uint64_t attacks = slow_slider_attacks(idx, occupancy, directions, 4);

        base_ptr[hash] = attacks;

        occupancy = (occupancy - premask) & premask;
    } while (occupancy != 0);
}

static void write_magic_rook(int idx, SCE_Precomputation_Tables* ptr_precomputation_tables) {
    const int directions[] = { NORTH, SOUTH, EAST, WEST };
    const uint64_t premask = (ptr_precomputation_tables->pm_table.rays[NORTH][idx] & ~RANK_8_MASK) | 
                             (ptr_precomputation_tables->pm_table.rays[SOUTH][idx] & ~RANK_1_MASK) |
                             (ptr_precomputation_tables->pm_table.rays[EAST][idx] & ~H_MASK) |
                             (ptr_precomputation_tables->pm_table.rays[WEST][idx] & ~A_MASK);
    const unsigned int n_bits = COUNT_SET_BITS(premask);
    const uint64_t magic = compute_magic_with_premask_and_direction(idx, premask, directions, 4);

    ptr_precomputation_tables->magic_table.rook[idx].premask = premask;
    ptr_precomputation_tables->magic_table.rook[idx].magic = magic;
    ptr_precomputation_tables->magic_table.rook[idx].shift = (CHESSBOARD_N_SQUARES - n_bits);

    uint64_t* base_ptr = &ptr_precomputation_tables->magic_table.RookMagicTable[idx * (1<<12)];

    uint64_t occupancy = 0U;
    do {
        uint hash = ((occupancy & premask) * magic) >> (CHESSBOARD_N_SQUARES - n_bits);

        uint64_t attacks = slow_slider_attacks(idx, occupancy, directions, 4);

        base_ptr[hash] = attacks;

        occupancy = (occupancy - premask) & premask;
    } while (occupancy != 0);
}

SCE_Return SCE_MagicTable_init(SCE_Precomputation_Tables* ptr_precomputation_tables) {
    if (ptr_precomputation_tables == NULL) return SCE_INVALID_PARAM;

    uint64_t occupancy_variations[1<<12] = { 0 };
    size_t count = 0;
    uint64_t current_occ;

    srand(time(NULL));

    // Pick a nonzero random seed value.
    while (seed == 0) seed = (uint64_t) rand();

    memset(&ptr_precomputation_tables->magic_table, 0, sizeof(SCE_MagicTable));

    for (uint idx = 0; idx < CHESSBOARD_N_SQUARES; idx++) {
        write_magic_bishop(idx, ptr_precomputation_tables);
        write_magic_rook(idx, ptr_precomputation_tables);
    }

    return SCE_SUCCESS;
}

uint64_t SCE_MagicTable_get_rook_attacks(int idx, uint64_t occupancy, const SCE_MagicTable * ptr_magic_table) {
    if (ptr_magic_table == NULL) return 0U;

    const uint64_t blockers = occupancy & ptr_magic_table->rook[idx].premask;

    uint hash = (blockers * ptr_magic_table->rook[idx].magic) >> ptr_magic_table->rook[idx].shift;

    return ptr_magic_table->RookMagicTable[(idx << 12) + hash];
}

uint64_t SCE_MagicTable_get_bishop_attacks(int idx, uint64_t occupancy, const SCE_MagicTable * ptr_magic_table) {
    if (ptr_magic_table == NULL) return 0U;

    const uint64_t blockers = occupancy & ptr_magic_table->bishop[idx].premask;

    uint hash = (blockers * ptr_magic_table->bishop[idx].magic) >> ptr_magic_table->bishop[idx].shift;

    return ptr_magic_table->BishopMagicTable[(idx << 9) + hash];
}

uint64_t SCE_MagicTable_get_queen_attacks(int idx, uint64_t occupancy, const SCE_MagicTable * ptr_magic_table) {
    if (ptr_magic_table == NULL) return 0U;

    const uint64_t rook_attacks = SCE_MagicTable_get_rook_attacks(idx, occupancy, ptr_magic_table);
    const uint64_t bishop_attacks = SCE_MagicTable_get_bishop_attacks(idx, occupancy, ptr_magic_table);

    return rook_attacks | bishop_attacks;
}
