#include <stdint.h>
#include "helper.h"

#ifndef __GNUC__
// Kernighan's Bit Counting Algorithm
unsigned int count_set_bits(uint64_t n) {
    uint cnt = 0U;
    while (n > 0) {
        n &= (n - 1);
        cnt++;
    }
    return cnt;
}
#endif  // __GNUC__

uint64_t xorshift(uint64_t x) {
    x ^= x << 13U;
    x ^= x >> 17U;
    x ^= x << 5U;
    return x;
}

