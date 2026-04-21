#include "helper.h"
#include "chess.h"
#include "eval/pst.h"

unsigned int SCE_Eval_ComputePhase(const SCE_Chessboard* const ptr_board) {
    unsigned int phase = 0;
    phase += QUEEN_PHASE_WEIGHT * ((ptr_board->bitboards[W_QUEEN] ? 1 : 0) + (ptr_board->bitboards[B_QUEEN] ? 1 : 0));
    phase += ROOK_PHASE_WEIGHT * (COUNT_SET_BITS(ptr_board->bitboards[W_ROOK]) + COUNT_SET_BITS(ptr_board->bitboards[B_ROOK]));
    phase += BISHOP_PHASE_WEIGHT * (COUNT_SET_BITS(ptr_board->bitboards[W_BISHOP]) + COUNT_SET_BITS(ptr_board->bitboards[B_BISHOP]));
    phase += KNIGHT_PHASE_WEIGHT * (COUNT_SET_BITS(ptr_board->bitboards[W_KNIGHT]) + COUNT_SET_BITS(ptr_board->bitboards[B_KNIGHT]));
    phase = (phase > TOTAL_PHASE_WEIGHT) ? TOTAL_PHASE_WEIGHT : phase;      // Accounting for early promotion

    return phase;
}
