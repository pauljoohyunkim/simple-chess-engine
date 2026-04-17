#include "eval/sef.h"

#define COUNT_SET_BITS __builtin_popcountll
// TODO: Implement fallback
#define COUNT_TRAILING_ZEROS(x) __builtin_ctzll(x)
#define COUNT_LEADING_ZEROS(x) __builtin_clzll(x)

// Classic "values" of pieces
#define PAWN_WEIGHT (100)
#define KNIGHT_WEIGHT (320)
#define BISHOP_WEIGHT (330)
#define ROOK_WEIGHT (500)
#define QUEEN_WEIGHT (900)
#define KING_WEIGHT (20000)

// In order: pawn, knight, bishop, rook, queen, and king
const int piece_weights[] = { PAWN_WEIGHT, KNIGHT_WEIGHT, BISHOP_WEIGHT, ROOK_WEIGHT, QUEEN_WEIGHT, KING_WEIGHT,    // WHITE
                              PAWN_WEIGHT, KNIGHT_WEIGHT, BISHOP_WEIGHT, ROOK_WEIGHT, QUEEN_WEIGHT, KING_WEIGHT };  // BLACK

typedef unsigned int uint;

static int SCE_Eval_PawnSquareEval(SCE_Chessboard* const ptr_board, PieceColor color);
static int SCE_Eval_KnightSquareEval(SCE_Chessboard* const ptr_board, PieceColor color);
static int SCE_Eval_BishopSquareEval(SCE_Chessboard* const ptr_board, PieceColor color);
static int SCE_Eval_RookSquareEval(SCE_Chessboard* const ptr_board, PieceColor color);
static int SCE_Eval_QueenSquareEval(SCE_Chessboard* const ptr_board, PieceColor color);
static int SCE_Eval_KingSquareEval(SCE_Chessboard* const ptr_board, PieceColor color);

int SCE_Eval_SimplifiedEvaluationFunction(const SCE_Chessboard* const ptr_board) {
    int centipawns = 0;
    
    // Compute the material sum.
    for (uint piece_type = W_PAWN; piece_type <= W_KING; piece_type++) {
        centipawns += COUNT_SET_BITS(ptr_board->bitboards[piece_type]) * piece_weights[piece_type];
    }
    for (uint piece_type = B_PAWN; piece_type <= B_KING; piece_type++) {
        centipawns -= COUNT_SET_BITS(ptr_board->bitboards[piece_type]) * piece_weights[piece_type];
    }

    // Each piece and their piece-square sum.

    centipawns += SCE_Eval_PawnSquareEval(ptr_board, WHITE);
    centipawns += SCE_Eval_KnightSquareEval(ptr_board, WHITE);
    centipawns += SCE_Eval_BishopSquareEval(ptr_board, WHITE);
    centipawns += SCE_Eval_RookSquareEval(ptr_board, WHITE);
    centipawns += SCE_Eval_QueenSquareEval(ptr_board, WHITE);
    centipawns += SCE_Eval_KingSquareEval(ptr_board, WHITE);

    centipawns += SCE_Eval_PawnSquareEval(ptr_board, BLACK);
    centipawns += SCE_Eval_KnightSquareEval(ptr_board, BLACK);
    centipawns += SCE_Eval_BishopSquareEval(ptr_board, BLACK);
    centipawns += SCE_Eval_RookSquareEval(ptr_board, BLACK);
    centipawns += SCE_Eval_QueenSquareEval(ptr_board, BLACK);
    centipawns += SCE_Eval_KingSquareEval(ptr_board, BLACK);

    return centipawns;
}

static int SCE_Eval_PawnSquareEval(SCE_Chessboard* const ptr_board, PieceColor color) {
    const int pst[] = {
        0,  0,  0,  0,  0,  0,  0,  0,
        5, 10, 10,-20,-20, 10, 10,  5,
        5, -5,-10,  0,  0,-10, -5,  5,
        0,  0,  0, 20, 20,  0,  0,  0,
        5,  5, 10, 25, 25, 10,  5,  5,
        10, 10, 20, 30, 30, 20, 10, 10,
        50, 50, 50, 50, 50, 50, 50, 50,
        0,  0,  0,  0,  0,  0,  0,  0,
    };

    int part_sum = 0;
    uint64_t pieces = ptr_board->bitboards[color == WHITE ? W_PAWN : B_PAWN];
    while (pieces) {
        const uint idx = COUNT_TRAILING_ZEROS(pieces);
        const uint64_t piece = (1ULL << idx);
        part_sum += (color == WHITE ? pst[idx] : -pst[63U-idx]);
        pieces &= ~piece;
    }

    return part_sum;
}

static int SCE_Eval_KnightSquareEval(SCE_Chessboard* const ptr_board, PieceColor color) {
    const int pst[] ={
        -50,-40,-30,-30,-30,-30,-40,-50,
        -40,-20,  0,  5,  5,  0,-20,-40,
        -30,  5, 10, 15, 15, 10,  5,-30,
        -30,  0, 15, 20, 20, 15,  0,-30,
        -30,  5, 15, 20, 20, 15,  5,-30,
        -30,  0, 10, 15, 15, 10,  0,-30,
        -40,-20,  0,  0,  0,  0,-20,-40,
        -50,-40,-30,-30,-30,-30,-40,-50,
    };

    int part_sum = 0;
    uint64_t pieces = ptr_board->bitboards[color == WHITE ? W_KNIGHT : B_KNIGHT];
    while (pieces) {
        const uint idx = COUNT_TRAILING_ZEROS(pieces);
        const uint64_t piece = (1ULL << idx);
        part_sum += (color == WHITE ? pst[idx] : -pst[63U-idx]);
        pieces &= ~piece;
    }

    return part_sum;
}

static int SCE_Eval_BishopSquareEval(SCE_Chessboard* const ptr_board, PieceColor color) {
    const int pst[] ={
        -20,-10,-10,-10,-10,-10,-10,-20,
        -10,  5,  0,  0,  0,  0,  5,-10,
        -10, 10, 10, 10, 10, 10, 10,-10,
        -10,  0, 10, 10, 10, 10,  0,-10,
        -10,  5,  5, 10, 10,  5,  5,-10,
        -10,  0,  5, 10, 10,  5,  0,-10,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -20,-10,-10,-10,-10,-10,-10,-20,
    };

    int part_sum = 0;
    uint64_t pieces = ptr_board->bitboards[color == WHITE ? W_BISHOP : B_BISHOP];
    while (pieces) {
        const uint idx = COUNT_TRAILING_ZEROS(pieces);
        const uint64_t piece = (1ULL << idx);
        part_sum += (color == WHITE ? pst[idx] : -pst[63U-idx]);
        pieces &= ~piece;
    }

    return part_sum;
}

static int SCE_Eval_RookSquareEval(SCE_Chessboard* const ptr_board, PieceColor color) {
    const int pst[] = {
        0,  0,  0,  5,  5,  0,  0,  0
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        5, 10, 10, 10, 10, 10, 10,  5,
        0,  0,  0,  0,  0,  0,  0,  0,
    };

    int part_sum = 0;
    uint64_t pieces = ptr_board->bitboards[color == WHITE ? W_ROOK : B_ROOK];
    while (pieces) {
        const uint idx = COUNT_TRAILING_ZEROS(pieces);
        const uint64_t piece = (1ULL << idx);
        part_sum += (color == WHITE ? pst[idx] : -pst[63U-idx]);
        pieces &= ~piece;
    }

    return part_sum;
}

static int SCE_Eval_QueenSquareEval(SCE_Chessboard* const ptr_board, PieceColor color) {
    const int pst[] = {
        -20,-10,-10, -5, -5,-10,-10,-20
        -10,  0,  5,  0,  0,  0,  0,-10,
        -10,  5,  5,  5,  5,  5,  0,-10,
        0,  0,  5,  5,  5,  5,  0, -5,
        -5,  0,  5,  5,  5,  5,  0, -5,
        -10,  0,  5,  5,  5,  5,  0,-10,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -20,-10,-10, -5, -5,-10,-10,-20,
    };

    int part_sum = 0;
    uint64_t pieces = ptr_board->bitboards[color == WHITE ? W_QUEEN : B_QUEEN];
    while (pieces) {
        const uint idx = COUNT_TRAILING_ZEROS(pieces);
        const uint64_t piece = (1ULL << idx);
        part_sum += (color == WHITE ? pst[idx] : -pst[63U-idx]);
        pieces &= ~piece;
    }

    return part_sum;
}

#define QUEEN_PHASE_WEIGHT 4
#define ROOK_PHASE_WEIGHT 2
#define BISHOP_PHASE_WEIGHT 1
#define KNIGHT_PHASE_WEIGHT 1
#define TOTAL_PHASE_WEIGHT 24
static int SCE_Eval_KingSquareEval(SCE_Chessboard* const ptr_board, PieceColor color) {
    const int pst_middle[] = {
        20, 30, 10,  0,  0, 10, 30, 20,
        20, 20,  0,  0,  0,  0, 20, 20,
        -10,-20,-20,-20,-20,-20,-20,-10,
        -20,-30,-30,-40,-40,-30,-30,-20,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
    };
    const int pst_end[] = {
        -50,-30,-30,-30,-30,-30,-30,-50
        -30,-30,  0,  0,  0,  0,-30,-30,
        -30,-10, 20, 30, 30, 20,-10,-30,
        -30,-10, 30, 40, 40, 30,-10,-30,
        -30,-10, 30, 40, 40, 30,-10,-30,
        -30,-10, 20, 30, 30, 20,-10,-30,
        -30,-20,-10,  0,  0,-10,-20,-30,
        -50,-40,-30,-20,-20,-30,-40,-50,
    };

    // (Middle game * phase + End game * (24 - phase)) / 24
    int mg_sum = 0;
    int eg_sum = 0;
    int phase = 0;
    uint64_t pieces = ptr_board->bitboards[color == WHITE ? W_KING : B_KING];
    while (pieces) {
        const uint idx = COUNT_TRAILING_ZEROS(pieces);
        const uint64_t piece = (1ULL << idx);
        mg_sum += (color == WHITE ? pst_middle[idx] : -pst_middle[63U-idx]);
        eg_sum += (color == WHITE ? pst_end[idx] : -pst_end[63U-idx]);
        pieces &= ~piece;
    }
    phase += QUEEN_PHASE_WEIGHT * ((ptr_board->bitboards[W_QUEEN] ? 1 : 0) + (ptr_board->bitboards[B_QUEEN] ? 1 : 0));
    phase += ROOK_PHASE_WEIGHT * (COUNT_SET_BITS(ptr_board->bitboards[W_ROOK]) + COUNT_SET_BITS(ptr_board->bitboards[B_ROOK]));
    phase += BISHOP_PHASE_WEIGHT * (COUNT_SET_BITS(ptr_board->bitboards[W_BISHOP]) + COUNT_SET_BITS(ptr_board->bitboards[B_BISHOP]));
    phase += KNIGHT_PHASE_WEIGHT * (COUNT_SET_BITS(ptr_board->bitboards[W_KNIGHT]) + COUNT_SET_BITS(ptr_board->bitboards[B_KNIGHT]));

    return (mg_sum * phase) + (eg_sum * (TOTAL_PHASE_WEIGHT - phase)) / TOTAL_PHASE_WEIGHT;
}

