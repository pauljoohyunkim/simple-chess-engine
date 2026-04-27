#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "chess.h"
#include "uci.h"

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
