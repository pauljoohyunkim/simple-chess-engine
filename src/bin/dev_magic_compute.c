#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include "chess.h"
#include "magic.h"
#include "helper.h"

static void show_help(char** argv);
static uint64_t get_random_uint64_t();
static uint64_t get_sparse_uint64_t();
static uint64_t slow_slider_attacks(int sq, uint64_t occupancy, const int directions[], int dir_count);
static uint64_t get_magic_bishop(const int idx);
static uint64_t get_magic_rook(const int idx);

uint64_t seed;
SCE_Precomputation_Tables precomputation_tables;

typedef enum {
    BISHOP,
    ROOK
} MagicPieceType;

int main(int argc, char** argv) {
    MagicPieceType mpt;
    SCE_Precomputation_Tables_init(&precomputation_tables, NULL);

    // Input check
    if (argc != 3) {
        show_help(argv);
        return 1;
    }
    if (strcmp(argv[1], "rook") == 0) {
        mpt = ROOK;
    } else if (strcmp(argv[1], "bishop") == 0) {
        mpt = BISHOP;
    } else {
        show_help(argv);
        return 1;
    }
    const int idx = SCE_AN_To_Idx(argv[2]);
    if (idx == UNASSIGNED) {
        show_help(argv);
        return 1;
    }

    // Seeding the initial seed.
    srand(time(NULL));
    seed = (uint64_t) rand();

    uint64_t magic = 0U;

    switch (mpt) {
        case BISHOP:
            magic = get_magic_bishop(idx);
            break;
        case ROOK:
            magic = get_magic_rook(idx);
            break;
        default:
            break;
    }

    if (magic) {
        printf("%llx\n", magic);
    }

    return 0;
}

static void show_help(char** argv) {
    fprintf(stderr, "A tool for finding magic value\n");
    fprintf(stderr, "Usage: %s rook/bishop AN\n", argv[0]);
}

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

static uint64_t compute_magic_with_premask_and_direction(const int idx, const uint64_t premask, const int directions[], const int dir_count) {
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

// Compress set to true for N-1 bits used.
static uint64_t get_magic_bishop(const int idx) {
    const int directions[] = { NORTHEAST, NORTHWEST, SOUTHEAST, SOUTHWEST };
    const uint64_t premask = (precomputation_tables.pm_table.rays[NORTHEAST][idx] & ~RANK_8_MASK & ~H_MASK) | 
                             (precomputation_tables.pm_table.rays[NORTHWEST][idx] & ~RANK_8_MASK & ~A_MASK) |
                             (precomputation_tables.pm_table.rays[SOUTHEAST][idx] & ~RANK_1_MASK & ~H_MASK) |
                             (precomputation_tables.pm_table.rays[SOUTHWEST][idx] & ~RANK_1_MASK & ~A_MASK);
    return compute_magic_with_premask_and_direction(idx, premask, directions, 4);
}

// Compress set to true for N-1 bits used.
static uint64_t get_magic_rook(const int idx) {
    const int directions[] = { NORTH, SOUTH, EAST, WEST };
    const uint64_t premask = (precomputation_tables.pm_table.rays[NORTH][idx] & ~RANK_8_MASK) | 
                             (precomputation_tables.pm_table.rays[SOUTH][idx] & ~RANK_1_MASK) |
                             (precomputation_tables.pm_table.rays[EAST][idx] & ~H_MASK) |
                             (precomputation_tables.pm_table.rays[WEST][idx] & ~A_MASK);
    return compute_magic_with_premask_and_direction(idx, premask, directions, 4);
}
