#ifndef SCE_EVAL_PST_H
#define SCE_EVAL_PST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "chess.h"

// Classic "values" of pieces
#define PAWN_WEIGHT (100)
#define KNIGHT_WEIGHT (320)
#define BISHOP_WEIGHT (330)
#define ROOK_WEIGHT (500)
#define QUEEN_WEIGHT (900)
#define KING_WEIGHT (20000)

// Used for phasing between middlegame and endgame.
#define QUEEN_PHASE_WEIGHT 4
#define ROOK_PHASE_WEIGHT 2
#define BISHOP_PHASE_WEIGHT 1
#define KNIGHT_PHASE_WEIGHT 1
#define TOTAL_PHASE_WEIGHT 24

// In order: pawn, knight, bishop, rook, queen, and king
static const int piece_weights[] = { PAWN_WEIGHT, KNIGHT_WEIGHT, BISHOP_WEIGHT, ROOK_WEIGHT, QUEEN_WEIGHT, KING_WEIGHT,    // WHITE
                                     PAWN_WEIGHT, KNIGHT_WEIGHT, BISHOP_WEIGHT, ROOK_WEIGHT, QUEEN_WEIGHT, KING_WEIGHT };  // BLACK

#define PST_PAWN (0U)
#define PST_KNIGHT (1U)
#define PST_BISHOP (2U)
#define PST_ROOK (3U)
#define PST_QUEEN (4U)
#define PST_KING_MIDDLE (5U)
#define PST_KING_END (6U)
static const int PST[7U][CHESSBOARD_DIMENSION * CHESSBOARD_DIMENSION] = {
    {
        0,  0,  0,  0,  0,  0,  0,  0,
        5, 10, 10,-20,-20, 10, 10,  5,
        5, -5,-10,  0,  0,-10, -5,  5,
        0,  0,  0, 20, 20,  0,  0,  0,
        5,  5, 10, 25, 25, 10,  5,  5,
        10, 10, 20, 30, 30, 20, 10, 10,
        50, 50, 50, 50, 50, 50, 50, 50,
        0,  0,  0,  0,  0,  0,  0,  0,
    },
    {
        -50,-40,-30,-30,-30,-30,-40,-50,
        -40,-20,  0,  5,  5,  0,-20,-40,
        -30,  5, 10, 15, 15, 10,  5,-30,
        -30,  0, 15, 20, 20, 15,  0,-30,
        -30,  5, 15, 20, 20, 15,  5,-30,
        -30,  0, 10, 15, 15, 10,  0,-30,
        -40,-20,  0,  0,  0,  0,-20,-40,
        -50,-40,-30,-30,-30,-30,-40,-50,
    },
    {
        -20,-10,-10,-10,-10,-10,-10,-20,
        -10,  5,  0,  0,  0,  0,  5,-10,
        -10, 10, 10, 10, 10, 10, 10,-10,
        -10,  0, 10, 10, 10, 10,  0,-10,
        -10,  5,  5, 10, 10,  5,  5,-10,
        -10,  0,  5, 10, 10,  5,  0,-10,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -20,-10,-10,-10,-10,-10,-10,-20,
    },
    {
        0,  0,  0,  5,  5,  0,  0,  0
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        5, 10, 10, 10, 10, 10, 10,  5,
        0,  0,  0,  0,  0,  0,  0,  0,
    },
    {
        -20,-10,-10, -5, -5,-10,-10,-20
        -10,  0,  5,  0,  0,  0,  0,-10,
        -10,  5,  5,  5,  5,  5,  0,-10,
        0,  0,  5,  5,  5,  5,  0, -5,
        -5,  0,  5,  5,  5,  5,  0, -5,
        -10,  0,  5,  5,  5,  5,  0,-10,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -20,-10,-10, -5, -5,-10,-10,-20,
    },
    {
        20, 30, 10,  0,  0, 10, 30, 20,
        20, 20,  0,  0,  0,  0, 20, 20,
        -10,-20,-20,-20,-20,-20,-20,-10,
        -20,-30,-30,-40,-40,-30,-30,-20,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
    },
    {
        -50,-30,-30,-30,-30,-30,-30,-50
        -30,-30,  0,  0,  0,  0,-30,-30,
        -30,-10, 20, 30, 30, 20,-10,-30,
        -30,-10, 30, 40, 40, 30,-10,-30,
        -30,-10, 30, 40, 40, 30,-10,-30,
        -30,-10, 20, 30, 30, 20,-10,-30,
        -30,-20,-10,  0,  0,-10,-20,-30,
        -50,-40,-30,-20,-20,-30,-40,-50,
    }
};

unsigned int SCE_Eval_ComputePhase(const SCE_Chessboard* const ptr_board);

#ifdef __cplusplus
}
#endif

#endif  // SCE_EVAL_PST_H
