#ifndef SCE_HELPER_H
#define SCE_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
    #define COUNT_SET_BITS(x) __builtin_popcountll(x)
    // TODO: Implement fallback
    #define COUNT_TRAILING_ZEROS(x) __builtin_ctzll(x)
    #define COUNT_LEADING_ZEROS(x) __builtin_clzll(x)
#else
    #include <stdint.h>

    /**
     * @brief Counts the number of set bits in a 64-bit value
     *
     * @param n The 64-bit value to count set bits in
     * @return unsigned int Number of set bits in the value
     */
    unsigned int count_set_bits(uint64_t n);
    #define COUNT_SET_BITS(x) count_set_bits(x)

#endif  // __GNUC__

#ifdef __cplusplus
}
#endif

#endif  // SCE_HELPER_H
