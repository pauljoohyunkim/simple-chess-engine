#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "chess.h"
#include "fen.h"

#define RETURN_IF_SCE_FAILURE(x, msg) do { if ((x) <= 0) { fprintf(stderr, "%s\n", msg); return SCE_INTERNAL_ERROR; } } while (0);

#define FEN_STEP_PIECE_PLACEMENT (0U)
#define FEN_STEP_ACTIVE_COLOR (1U)
#define FEN_STEP_CASTLING_RIGHTS (2U)
#define FEN_STEP_EN_PASSANT_TARGET_SQUARE (3U)
#define FEN_STEP_HALFMOVE_CLOCK (4U)
#define FEN_STEP_FULLMOVE_NUMBER (5U)

typedef unsigned int uint;

static uint count_occurence(const char* const str, const char c);

// Maximum FEN string is 92 including null terminator.
#define FEN_STRING_MAX_LEN (92U)
SCE_Return SCE_Chessboard_FEN_setup(SCE_Context* const ctx, const char* const fen) {
    if (ctx == NULL || fen == NULL) return SCE_INVALID_PARAM;
    size_t fen_len = strlen(fen);
    if (fen_len >= FEN_STRING_MAX_LEN) return SCE_INVALID_PARAM;

    if (count_occurence(fen, ' ') != 5U) return SCE_INVALID_PARAM;

    // Now guaranteed to have 6 chunks.
    char fen_cpy[FEN_STRING_MAX_LEN] = { 0 };
    memcpy(fen_cpy, fen, fen_len);

    char* indexer = NULL;
    const char* piece_placement_str = fen_cpy;
    indexer = strchr(piece_placement_str, ' ');
    *indexer = '\0';
    indexer++;

    const char* active_color_str = indexer;
    indexer = strchr(active_color_str, ' ');
    *indexer = '\0';
    indexer++;

    const char* castling_rights_str = indexer;
    indexer = strchr(castling_rights_str, ' ');
    *indexer = '\0';
    indexer++;

    const char* en_passant_target_square_str = indexer;
    indexer = strchr(en_passant_target_square_str, ' ');
    *indexer = '\0';
    indexer++;
    
    const char* halfmove_clock_str = indexer;
    indexer = strchr(halfmove_clock_str, ' ');
    *indexer = '\0';
    indexer++;
    
    const char* fullmove_number = indexer;

    RETURN_IF_SCE_FAILURE(SCE_Chessboard_clear(ctx), "Clearing board failure!");
    ctx->board.castling_rights = 0U;

    uint row = 7U;
    uint col = 0U;
    uint fen_step = FEN_STEP_PIECE_PLACEMENT;
    if (fen_step == FEN_STEP_PIECE_PLACEMENT) {
        const size_t len = strlen(piece_placement_str);
        for (uint i = 0U; i < len; i++) {
            const char c = piece_placement_str[i];

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
                ctx->board.bitboards[piece_type] ^= (1ULL << idx);
                col++;
            } else if (isdigit(c)) {
                const uint shift = c - '0';
                if (col + shift > CHESSBOARD_DIMENSION) return SCE_INVALID_PARAM;

                col += shift;
            } else if (c == '/') {
                if (row == 0) return SCE_INVALID_PARAM;
                row--;
                col = 0;
            } else {
                return SCE_INVALID_PARAM;
            }
            continue;
        }
        if (row != 0 || col != CHESSBOARD_DIMENSION) return SCE_INVALID_PARAM;
        fen_step = FEN_STEP_ACTIVE_COLOR;
    }

    if (fen_step == FEN_STEP_ACTIVE_COLOR) {
        const size_t len = strlen(active_color_str);
        if (len != 1U) return SCE_INVALID_PARAM;

        switch (active_color_str[0]) {
            case 'w':
                ctx->board.to_move = WHITE;
                break;
            case 'b':
                ctx->board.to_move = BLACK;
                break;
            default:
                return SCE_INVALID_PARAM;
        }

        fen_step = FEN_STEP_CASTLING_RIGHTS;
    }

    if (fen_step == FEN_STEP_CASTLING_RIGHTS) {
        const size_t len = strlen(castling_rights_str);
        if (len > 4U) return SCE_INVALID_PARAM;
        if (count_occurence(castling_rights_str, '-') > 1U) return SCE_INVALID_PARAM;
        if (count_occurence(castling_rights_str, '-') == 0U) {
            for (uint i = 0U; i < len; i++) {
                const char c = castling_rights_str[i];

                switch (c) {
                    case 'K':
                        ctx->board.castling_rights |= SCE_CASTLING_RIGHTS_WK;
                        break;
                    case 'Q':
                        ctx->board.castling_rights |= SCE_CASTLING_RIGHTS_WQ;
                        break;
                    case 'k':
                        ctx->board.castling_rights |= SCE_CASTLING_RIGHTS_BK;
                        break;
                    case 'q':
                        ctx->board.castling_rights |= SCE_CASTLING_RIGHTS_BQ;
                        break;
                    default:
                        return SCE_INVALID_PARAM;
                }
            }
        } else {
            // No piece has castling rights.
            ctx->board.castling_rights = 0U;
        }

        fen_step = FEN_STEP_EN_PASSANT_TARGET_SQUARE;
    }

    if (fen_step == FEN_STEP_EN_PASSANT_TARGET_SQUARE) {
        const size_t len = strlen(en_passant_target_square_str);
        if (len > 2U) return SCE_INVALID_PARAM;
        
        if (len == 1U) {
            if (en_passant_target_square_str[0] != '-') return SCE_INVALID_PARAM;
            ctx->board.en_passant_idx = UNASSIGNED;
        }
        if (len == 2U) {
            // Error is handled automatically as -1 is "UNASSIGNED"
            const int idx = SCE_AN_To_Idx(en_passant_target_square_str);
            ctx->board.en_passant_idx = idx;
        }
        fen_step = FEN_STEP_HALFMOVE_CLOCK;
    }

    if (fen_step == FEN_STEP_HALFMOVE_CLOCK) {
        const size_t len = strlen(halfmove_clock_str);
        if (len > 3U) return SCE_INVALID_PARAM;

        // Sanity check.
        for (uint i = 0U; i < len; i++) {
            if (!isdigit(halfmove_clock_str[i])) return SCE_INVALID_PARAM;
        }

        ctx->board.half_move_clock = atoi(halfmove_clock_str);
        fen_step = FEN_STEP_FULLMOVE_NUMBER;
    }

    // If current step is not fullmove number, it means parsing stopped at one of the previous steps.
    // This is not allowed.
    // Also fullmove number is not handled.
    if (fen_step != FEN_STEP_FULLMOVE_NUMBER) return SCE_INVALID_PARAM;

    return SCE_SUCCESS;
}

static uint count_occurence(const char* const str, const char c) {
    if (str == NULL || c == '\0') return 0U;
    const size_t len = strlen(str);
    uint count = 0U;

    for (uint i = 0U; i < len; i++) {
        if (str[i] == c) count++;
    }

    return count;
}
