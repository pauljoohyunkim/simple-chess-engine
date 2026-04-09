#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "chess.h"
#include "fen.h"

#define RETURN_IF_SCE_FAILURE(x, msg) do { if ((x) <= 0) { fprintf(stderr, "%s\n", msg); return SCE_INTERNAL_ERROR; } } while (0);

typedef unsigned int uint;

// Maximum FEN string is 92 including null terminator.
#define FEN_STRING_MAX_LEN (92U)
SCE_Return SCE_Chessboard_FEN_setup(SCE_Chessboard* const ptr_board, const char* const fen) {
    if (ptr_board == NULL || fen == NULL) return SCE_INVALID_PARAM;
    size_t fen_len = strlen(fen);
    if (fen_len >= FEN_STRING_MAX_LEN) return SCE_INVALID_PARAM;

    RETURN_IF_SCE_FAILURE(SCE_Chessboard_clear(ptr_board), "Clearing board failure!");

    uint row = 7U;
    uint col = 0U;
    bool filled = false;
    for (uint i = 0U; i < fen_len; i++) {
        const char c = fen[i];

        if (filled == false) {
            if (isalpha(c)) {
                uint piece_type;
                switch (c) {
                    case 'P':
                        piece_type = W_PAWN;
                        break;
                    case 'N':
                        piece_type = W_KNIGHT;
                        break;
                    case 'B':
                        piece_type = W_BISHOP;
                        break;
                    case 'R':
                        piece_type = W_ROOK;
                        break;
                    case 'Q':
                        piece_type = W_QUEEN;
                        break;
                    case 'K':
                        piece_type = W_KING;
                        break;
                    case 'p':
                        piece_type = B_PAWN;
                        break;
                    case 'n':
                        piece_type = B_KNIGHT;
                        break;
                    case 'b':
                        piece_type = B_BISHOP;
                        break;
                    case 'r':
                        piece_type = B_ROOK;
                        break;
                    case 'q':
                        piece_type = B_QUEEN;
                        break;
                    case 'k':
                        piece_type = B_KING;
                        break;
                    default:
                        return SCE_INVALID_PARAM;
                }
                if (col >= CHESSBOARD_DIMENSION) return SCE_INVALID_PARAM;

                const uint idx = 8 * row + col;
                ptr_board->bitboards[piece_type] ^= (1ULL << idx);
                col++;
            } else if (isdigit(c)) {
                const uint shift = c - '0';
                if (col + shift > CHESSBOARD_DIMENSION) return SCE_INVALID_PARAM;

                col += shift;
            } else if (c == '/') {
                if (row == 0) return SCE_INVALID_PARAM;
                row--;
                col = 0;
            } else if (c == ' ') {
                if (row != 0 || col != 0) return SCE_INVALID_PARAM;
                filled = true;
            } else {
                return SCE_INVALID_PARAM;
            }
            continue;
        }
    }

    return SCE_SUCCESS;
}
