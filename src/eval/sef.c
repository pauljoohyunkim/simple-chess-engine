#include <assert.h>
#include "helper.h"
#include "eval/pst.h"
#include "eval/sef.h"
#define FLIP(x) ((x)^56)

typedef unsigned int uint;

static int SCE_Eval_PawnSquareEval(SCE_Chessboard* const ptr_board, PieceColor color);
static int SCE_Eval_KnightSquareEval(SCE_Chessboard* const ptr_board, PieceColor color);
static int SCE_Eval_BishopSquareEval(SCE_Chessboard* const ptr_board, PieceColor color);
static int SCE_Eval_RookSquareEval(SCE_Chessboard* const ptr_board, PieceColor color);
static int SCE_Eval_QueenSquareEval(SCE_Chessboard* const ptr_board, PieceColor color);
static int SCE_Eval_KingSquareEval(SCE_Chessboard* const ptr_board, PieceColor color);

int SCE_Eval_SimplifiedEvaluationFunction(SCE_Context* const ctx) {
    assert(ctx != NULL);
    int centipawns = 0;
    
    // Compute the material sum.
    for (uint piece_type = W_PAWN; piece_type <= W_KING; piece_type++) {
        centipawns += COUNT_SET_BITS(ctx->board.bitboards[piece_type]) * piece_weights[piece_type];
    }
    for (uint piece_type = B_PAWN; piece_type <= B_KING; piece_type++) {
        centipawns -= COUNT_SET_BITS(ctx->board.bitboards[piece_type]) * piece_weights[piece_type];
    }

    // Each piece and their piece-square sum.

    centipawns += SCE_Eval_PawnSquareEval(&ctx->board, WHITE);
    centipawns += SCE_Eval_KnightSquareEval(&ctx->board, WHITE);
    centipawns += SCE_Eval_BishopSquareEval(&ctx->board, WHITE);
    centipawns += SCE_Eval_RookSquareEval(&ctx->board, WHITE);
    centipawns += SCE_Eval_QueenSquareEval(&ctx->board, WHITE);
    centipawns += SCE_Eval_KingSquareEval(&ctx->board, WHITE);

    centipawns += SCE_Eval_PawnSquareEval(&ctx->board, BLACK);
    centipawns += SCE_Eval_KnightSquareEval(&ctx->board, BLACK);
    centipawns += SCE_Eval_BishopSquareEval(&ctx->board, BLACK);
    centipawns += SCE_Eval_RookSquareEval(&ctx->board, BLACK);
    centipawns += SCE_Eval_QueenSquareEval(&ctx->board, BLACK);
    centipawns += SCE_Eval_KingSquareEval(&ctx->board, BLACK);

    return centipawns;
}

static int SCE_Eval_PawnSquareEval(SCE_Chessboard* const ptr_board, PieceColor color) {
    const int* pst = PST[PST_PAWN];

    int part_sum = 0;
    uint64_t pieces = ptr_board->bitboards[color == WHITE ? W_PAWN : B_PAWN];
    while (pieces) {
        const uint idx = COUNT_TRAILING_ZEROS(pieces);
        const uint64_t piece = (1ULL << idx);
        part_sum += (color == WHITE ? pst[idx] : -pst[FLIP(idx)]);
        pieces &= ~piece;
    }

    return part_sum;
}

static int SCE_Eval_KnightSquareEval(SCE_Chessboard* const ptr_board, PieceColor color) {
    const int* pst = PST[PST_KNIGHT];

    int part_sum = 0;
    uint64_t pieces = ptr_board->bitboards[color == WHITE ? W_KNIGHT : B_KNIGHT];
    while (pieces) {
        const uint idx = COUNT_TRAILING_ZEROS(pieces);
        const uint64_t piece = (1ULL << idx);
        part_sum += (color == WHITE ? pst[idx] : -pst[FLIP(idx)]);
        pieces &= ~piece;
    }

    return part_sum;
}

static int SCE_Eval_BishopSquareEval(SCE_Chessboard* const ptr_board, PieceColor color) {
    const int* pst = PST[PST_BISHOP];

    int part_sum = 0;
    uint64_t pieces = ptr_board->bitboards[color == WHITE ? W_BISHOP : B_BISHOP];
    while (pieces) {
        const uint idx = COUNT_TRAILING_ZEROS(pieces);
        const uint64_t piece = (1ULL << idx);
        part_sum += (color == WHITE ? pst[idx] : -pst[FLIP(idx)]);
        pieces &= ~piece;
    }

    return part_sum;
}

static int SCE_Eval_RookSquareEval(SCE_Chessboard* const ptr_board, PieceColor color) {
    const int* pst = PST[PST_ROOK];

    int part_sum = 0;
    uint64_t pieces = ptr_board->bitboards[color == WHITE ? W_ROOK : B_ROOK];
    while (pieces) {
        const uint idx = COUNT_TRAILING_ZEROS(pieces);
        const uint64_t piece = (1ULL << idx);
        part_sum += (color == WHITE ? pst[idx] : -pst[FLIP(idx)]);
        pieces &= ~piece;
    }

    return part_sum;
}

static int SCE_Eval_QueenSquareEval(SCE_Chessboard* const ptr_board, PieceColor color) {
    const int* pst = PST[PST_QUEEN];

    int part_sum = 0;
    uint64_t pieces = ptr_board->bitboards[color == WHITE ? W_QUEEN : B_QUEEN];
    while (pieces) {
        const uint idx = COUNT_TRAILING_ZEROS(pieces);
        const uint64_t piece = (1ULL << idx);
        part_sum += (color == WHITE ? pst[idx] : -pst[FLIP(idx)]);
        pieces &= ~piece;
    }

    return part_sum;
}

static int SCE_Eval_KingSquareEval(SCE_Chessboard* const ptr_board, PieceColor color) {
    const int* pst_middle = PST[PST_KING_MIDDLE];
    const int* pst_end = PST[PST_KING_END];

    // (Middle game * phase + End game * (24 - phase)) / 24
    int mg_sum = 0;
    int eg_sum = 0;
    uint64_t pieces = ptr_board->bitboards[color == WHITE ? W_KING : B_KING];
    while (pieces) {
        const uint idx = COUNT_TRAILING_ZEROS(pieces);
        const uint64_t piece = (1ULL << idx);
        mg_sum += (color == WHITE ? pst_middle[idx] : -pst_middle[FLIP(idx)]);
        eg_sum += (color == WHITE ? pst_end[idx] : -pst_end[FLIP(idx)]);
        pieces &= ~piece;
    }
    const int phase = SCE_Eval_ComputePhase(ptr_board);

    return ((mg_sum * phase) + (eg_sum * (TOTAL_PHASE_WEIGHT - phase))) / TOTAL_PHASE_WEIGHT;
}

int SCE_DeltaEval_SimplifiedEvaluationFunction(SCE_Context* const ctx, const SCE_ChessMove move) {

}
