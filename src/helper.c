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
