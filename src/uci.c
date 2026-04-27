#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "chess.h"
#include "uci.h"
#include "fen.h"

typedef unsigned int uint;

#define PROMO_TYPE_KNIGHT 0
#define PROMO_TYPE_BISHOP 1
#define PROMO_TYPE_ROOK 2
#define PROMO_TYPE_QUEEN 3
bool SCE_MoveToUCIString(const SCE_ChessMove move, char uci_string[6]) {
    if (uci_string == NULL) return false;
    memset(uci_string, 0, 6);
    if (move == EMPTY_MOVE) {
        memset(uci_string, '0', 4);
        return true;
    }

    const uint src_idx = move SCE_CHESSMOVE_GET_SRC;
    const uint dst_idx = move SCE_CHESSMOVE_GET_DST;
    const int flag = move SCE_CHESSMOVE_GET_FLAG;

    SCE_Return ret = SCE_Bitboard_To_AN(uci_string, (1ULL << src_idx));
    if (ret != SCE_SUCCESS) return false;
    ret = SCE_Bitboard_To_AN(&uci_string[2], (1ULL << dst_idx));
    if (ret != SCE_SUCCESS) return false;

    if (flag & SCE_CHESSMOVE_FLAG_FILTER_PROMOTION) {
        const uint promo_type_idx = flag & ~(SCE_CHESSMOVE_FLAG_FILTER_PROMOTION | SCE_CHESSMOVE_FLAG_CAPTURE);
        switch (promo_type_idx) {
            case PROMO_TYPE_KNIGHT:
                uci_string[4] = 'n';
                break;
            case PROMO_TYPE_BISHOP:
                uci_string[4] = 'b';
                break;
            case PROMO_TYPE_ROOK:
                uci_string[4] = 'r';
                break;
            case PROMO_TYPE_QUEEN:
                uci_string[4] = 'q';
                break;
            default:
                return false;
        }
    }

    // By default, UCI expects lowercase strings.
    for (uint i = 0; i < 6; i++) {
        uci_string[i] = tolower(uci_string[i]);
    }

    return true;
}

SCE_ChessMove SCE_UCIStringToMove(const char* const uci_string) {
    // UCI string has to be 4 or 5 (for promotion)
    if (strlen(uci_string) < 4) return EMPTY_MOVE;
    if (strlen(uci_string) > 5) return EMPTY_MOVE;

    char src_an[3] = { 0 };
    char dst_an[3] = { 0 };
    strncpy(src_an, uci_string, 2);
    strncpy(dst_an, &uci_string[2], 2);

    const int src_idx = SCE_AN_To_Idx(src_an);
    const int dst_idx = SCE_AN_To_Idx(dst_an);
    if (src_idx == UNASSIGNED || dst_idx == UNASSIGNED) return EMPTY_MOVE;

    int flag = 0;
    if (strlen(uci_string) == 5) {
        switch (uci_string[4]) {
            case 'n':
                flag = SCE_CHESSMOVE_FLAG_KNIGHT_PROMOTION;
                break;
            case 'b':
                flag = SCE_CHESSMOVE_FLAG_BISHOP_PROMOTION;
                break;
            case 'r':
                flag = SCE_CHESSMOVE_FLAG_ROOK_PROMOTION;
                break;
            case 'q':
                flag = SCE_CHESSMOVE_FLAG_QUEEN_PROMOTION;
                break;
            default:
                return EMPTY_MOVE;
        }
    }

    const SCE_ChessMove move = (src_idx SCE_CHESSMOVE_SET_SRC) | (dst_idx SCE_CHESSMOVE_SET_DST) | (flag SCE_CHESSMOVE_SET_FLAG);

    return move;
}

bool SCE_UCI_ParsePosition(SCE_Context* const ctx, const char* const line) {
    if (ctx == NULL || line == NULL) return false;
    if (strncmp(line, "position ", 9) != 0) return false;

    const char* moves_substr = strstr(line, "moves");
    if (strncmp(&line[9], "fen", 3) == 0) {
        char fen_str[93] = { 0 };
        if (moves_substr) {
            memcpy(fen_str, &line[13], moves_substr - &line[13] - 1);
        } else {
            strcpy(fen_str, line);
        }
        SCE_Return ret = SCE_Chessboard_FEN_setup(ctx, fen_str);
        if (ret != SCE_SUCCESS) {
            return false;
        }
    }
    if (strncmp(&line[9], "startpos", 8) == 0) {
        SCE_Return ret = SCE_Chessboard_FEN_setup(ctx, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        if (ret != SCE_SUCCESS) {
            return false;
        }
    }
    if (moves_substr) {
        char moves_substr_cpy[BUFSIZ] = { 0 };
        strcpy(moves_substr_cpy, &moves_substr[6]);

        {
            // Replace newline with '\0'
            char* pos = strchr(moves_substr_cpy, '\n');
            if (pos) {
                *pos = '\0';
            }
        }

        char* saveptr;
        char* move_str = strtok_r(moves_substr_cpy, " ", &saveptr);
        while (move_str) {
            const SCE_ChessMove ordered_move = SCE_UCIStringToMove(move_str);
            if (ordered_move == EMPTY_MOVE) continue;
            const uint ordered_move_src_idx = ordered_move SCE_CHESSMOVE_GET_SRC;
            const uint ordered_move_dst_idx = ordered_move SCE_CHESSMOVE_GET_DST;
            const int ordered_move_flag = ordered_move SCE_CHESSMOVE_GET_FLAG;

            // Before making move, need to generate legal moves and compare.
            SCE_ChessMoveList movelist;
            SCE_Return ret = SCE_ChessMoveList_clear(&movelist);
            if (ret != SCE_SUCCESS) return false;
            ret = SCE_GenerateLegalMoves(&movelist, ctx);
            if (ret != SCE_SUCCESS) return false;

            if (movelist.count == 0) return false;

            for (uint i = 0; i < movelist.count; i++) {
                const SCE_ChessMove move = movelist.moves[i];
                const uint src_idx = move SCE_CHESSMOVE_GET_SRC;
                const uint dst_idx = move SCE_CHESSMOVE_GET_DST;
                const int flag = move SCE_CHESSMOVE_GET_FLAG;

                if (src_idx != ordered_move_src_idx) continue;
                if (dst_idx != ordered_move_dst_idx) continue;
                if ((ordered_move_flag & SCE_CHESSMOVE_FLAG_FILTER_PROMOTION) & (flag & SCE_CHESSMOVE_FLAG_FILTER_PROMOTION)) {
                    // TODO: Promotion handling
                    // Check if last two bits are equivalent.
                    if ((ordered_move_flag & 3) != (flag & 3)) continue;

                    ret = SCE_MakeMove(ctx, move);
                    if (ret != SCE_SUCCESS) return false;
                    break;
                } else {
                    ret = SCE_MakeMove(ctx, move);
                    if (ret != SCE_SUCCESS) return false;
                    break;
                }
            }

            printf("%s\n", move_str);
            move_str = strtok_r(NULL, " ", &saveptr);
        }
    }

    return true;
}
