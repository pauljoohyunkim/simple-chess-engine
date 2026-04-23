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
static int SCE_Eval_KingSquareEval(SCE_Chessboard* const ptr_board, PieceColor color, const bool is_mg);

int SCE_Eval_SimplifiedEvaluationFunction(SCE_Context* const ctx) {
    assert(ctx != NULL);
    int material_sum = 0;
    int pst_sum = 0;
    
    // Compute the material sum.
    for (uint piece_type = W_PAWN; piece_type <= W_KING; piece_type++) {
        material_sum += COUNT_SET_BITS(ctx->board.bitboards[piece_type]) * piece_weights[piece_type];
    }
    for (uint piece_type = B_PAWN; piece_type <= B_KING; piece_type++) {
        material_sum -= COUNT_SET_BITS(ctx->board.bitboards[piece_type]) * piece_weights[piece_type];
    }

    ctx->eval_state.phase = SCE_Eval_ComputePhase(&ctx->board);
    ctx->eval_state.mg_score = material_sum;
    ctx->eval_state.eg_score = material_sum;

    // Each piece and their piece-square sum.
    // Except for kings, all pieces are equivalent for MG and EG
    pst_sum += SCE_Eval_PawnSquareEval(&ctx->board, WHITE);
    pst_sum += SCE_Eval_KnightSquareEval(&ctx->board, WHITE);
    pst_sum += SCE_Eval_BishopSquareEval(&ctx->board, WHITE);
    pst_sum += SCE_Eval_RookSquareEval(&ctx->board, WHITE);
    pst_sum += SCE_Eval_QueenSquareEval(&ctx->board, WHITE);

    pst_sum += SCE_Eval_PawnSquareEval(&ctx->board, BLACK);
    pst_sum += SCE_Eval_KnightSquareEval(&ctx->board, BLACK);
    pst_sum += SCE_Eval_BishopSquareEval(&ctx->board, BLACK);
    pst_sum += SCE_Eval_RookSquareEval(&ctx->board, BLACK);
    pst_sum += SCE_Eval_QueenSquareEval(&ctx->board, BLACK);

    ctx->eval_state.mg_score += pst_sum;
    ctx->eval_state.eg_score += pst_sum;

    // For kings, mg and eg scores are different.
    ctx->eval_state.mg_score += SCE_Eval_KingSquareEval(&ctx->board, WHITE, true);
    ctx->eval_state.mg_score += SCE_Eval_KingSquareEval(&ctx->board, BLACK, true);
    ctx->eval_state.eg_score += SCE_Eval_KingSquareEval(&ctx->board, WHITE, false);
    ctx->eval_state.eg_score += SCE_Eval_KingSquareEval(&ctx->board, BLACK, false);

    // (Middle game * phase + End game * (24 - phase)) / 24
    const int phase = ctx->eval_state.phase;
    const int mg_score = ctx->eval_state.mg_score;
    const int eg_score = ctx->eval_state.eg_score;
    return (mg_score * phase + eg_score * (TOTAL_PHASE_WEIGHT - phase)) / 24;
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

static int SCE_Eval_KingSquareEval(SCE_Chessboard* const ptr_board, PieceColor color, const bool is_mg) {
    const int* pst = PST[is_mg ? PST_KING_MIDDLE : PST_KING_END];

    int part_sum = 0;
    uint64_t pieces = ptr_board->bitboards[color == WHITE ? W_KING : B_KING];
    while (pieces) {
        const uint idx = COUNT_TRAILING_ZEROS(pieces);
        const uint64_t piece = (1ULL << idx);
        part_sum += (color == WHITE ? pst[idx] : -pst[FLIP(idx)]);
        pieces &= ~piece;
    }

    return part_sum;
}

int SCE_DeltaEval_SimplifiedEvaluationFunction(SCE_Context* const ctx, const SCE_ChessMove move) {

}
