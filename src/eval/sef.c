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

int SCE_DeltaEval_SimplifiedEvaluationFunction(const SCE_Chessboard* const ptr_board, SCE_EvalState* const ptr_eval_state, const SCE_ChessMove move) {
    const uint src_idx = move SCE_CHESSMOVE_GET_SRC;
    const uint dst_idx = move SCE_CHESSMOVE_GET_DST;
    const int flag = move SCE_CHESSMOVE_GET_FLAG;
    const PieceType src_piece_type = ptr_board->mailbox[src_idx];
    assert(src_piece_type != UNASSIGNED_PIECE_TYPE);

    const PieceColor src_color = src_piece_type < B_PAWN ? WHITE : BLACK;
    const int sign = src_color == WHITE ? 1 : -1;

    const uint src_adjusted_pst_idx = src_color == WHITE ? src_idx : FLIP(src_idx);

    // 1. "Subtract" piece value from old square
    if (src_piece_type != W_KING && src_piece_type != B_KING) {
        // Regular pieces (0~4, 6~10 -> 0~4)
        const uint pst_idx = src_piece_type % 6U;
        const int* pst = PST[pst_idx];

        ptr_eval_state->mg_score -= src_color == WHITE ? pst[src_idx] : -pst[FLIP(src_idx)];
        ptr_eval_state->eg_score -= src_color == WHITE ? pst[src_idx] : -pst[FLIP(src_idx)];
        ptr_eval_state->mg_score += src_color == WHITE ? pst[dst_idx] : -pst[FLIP(dst_idx)];
        ptr_eval_state->eg_score += src_color == WHITE ? pst[dst_idx] : -pst[FLIP(dst_idx)];
    } else {
        // Moving a king piece
        const int* pst_king_mg = PST[PST_KING_MIDDLE];
        const int* pst_king_eg = PST[PST_KING_END];

        ptr_eval_state->mg_score -= src_color == WHITE ? pst_king_mg[src_idx] : -pst_king_mg[FLIP(src_idx)];
        ptr_eval_state->eg_score -= src_color == WHITE ? pst_king_eg[src_idx] : -pst_king_eg[FLIP(src_idx)];
        ptr_eval_state->mg_score += src_color == WHITE ? pst_king_mg[dst_idx] : -pst_king_mg[FLIP(dst_idx)];
        ptr_eval_state->eg_score += src_color == WHITE ? pst_king_eg[dst_idx] : -pst_king_eg[FLIP(dst_idx)];
    }

    // 2. Deal with
    // 2.1 Capture
    if (flag & SCE_CHESSMOVE_FLAG_CAPTURE) {
        // 2.1.1 En passant
        if (flag == SCE_CHESSMOVE_FLAG_EN_PASSANT_CAPTURE) {
            const PieceType captured_piece_type = src_color == WHITE ? B_PAWN : W_PAWN;
            const uint captured_piece_idx = src_color == WHITE ? ptr_board->en_passant_idx - CHESSBOARD_DIMENSION : ptr_board->en_passant_idx + CHESSBOARD_DIMENSION;
            const int* pst = PST[PST_PAWN];
            // PST
            ptr_eval_state->mg_score += src_color == WHITE ? pst[FLIP(captured_piece_idx)] : -pst[captured_piece_idx];
            ptr_eval_state->eg_score += src_color == WHITE ? pst[FLIP(captured_piece_idx)] : -pst[captured_piece_idx];
            // Material
            ptr_eval_state->mg_score += src_color == WHITE ? PAWN_WEIGHT : -PAWN_WEIGHT;
            ptr_eval_state->eg_score += src_color == WHITE ? PAWN_WEIGHT : -PAWN_WEIGHT;
        } else {
            const PieceType captured_piece_type = ptr_board->mailbox[dst_idx];
            assert(captured_piece_type != W_KING && captured_piece_type != B_KING);
            
            // Non-king piece
            const uint pst_idx = captured_piece_type % 6U;
            const int* pst = PST[pst_idx];
            // PST
            ptr_eval_state->mg_score += src_color == WHITE ? pst[FLIP(dst_idx)] : -pst[dst_idx];
            ptr_eval_state->eg_score += src_color == WHITE ? pst[FLIP(dst_idx)] : -pst[dst_idx];
            // Material
            ptr_eval_state->mg_score += src_color == WHITE ? piece_weights[pst_idx] : -piece_weights[pst_idx];
            ptr_eval_state->eg_score += src_color == WHITE ? piece_weights[pst_idx] : -piece_weights[pst_idx];

            // Phase update
            switch (pst_idx) {
                case PST_KNIGHT:
                    ptr_eval_state->phase -= KNIGHT_PHASE_WEIGHT;
                    break;
                case PST_BISHOP:
                    ptr_eval_state->phase -= BISHOP_PHASE_WEIGHT;
                    break;
                case PST_ROOK:
                    ptr_eval_state->phase -= ROOK_PHASE_WEIGHT;
                    break;
                case PST_QUEEN:
                    ptr_eval_state->phase -= QUEEN_PHASE_WEIGHT;
                    break;
                default:
                    break;
            }
        }
    }

    // 2.2 Promotion
    if (flag & SCE_CHESSMOVE_FLAG_FILTER_PROMOTION) {

    }

    // 2.3 Castling
    if (flag == SCE_CHESSMOVE_FLAG_KING_CASTLE || flag == SCE_CHESSMOVE_FLAG_QUEEN_CASTLE) {

    }

    // Clamp phase for computation to less than 24 (even though eval_state can have larger phase due to promotion)
    const int phase = ptr_eval_state->phase > TOTAL_PHASE_WEIGHT ? TOTAL_PHASE_WEIGHT : ptr_eval_state->phase;

    // TODO
    return 0;
}
