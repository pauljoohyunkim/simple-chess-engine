#ifdef DEBUG

#include <stdio.h>
#include "chess.h"

typedef unsigned int uint;

void print_as_board(const uint64_t val) {
    printf("\n");
    for (uint i = 0; i < CHESSBOARD_DIMENSION; i++) {
        for (uint j = 0; j < CHESSBOARD_DIMENSION; j++) {
            uint64_t pos = 1ULL << ((7-i) * CHESSBOARD_DIMENSION + (7-j));
            if (pos & val) {
                printf("*");
            } else {
                printf("-");
            }
        }
        printf("\n");
    }
    printf("\n");
}

#endif // DEBUG
