#include "helper.h"
#include "chess.h"
#include "eval/pst.h"

unsigned int SCE_Eval_ComputePhase(const SCE_Chessboard* const ptr_board) {
    unsigned int phase = 0;
    phase += QUEEN_PHASE_WEIGHT * COUNT_SET_BITS(ptr_board->bitboards[W_QUEEN] ^ ptr_board->bitboards[B_QUEEN]);
    phase += ROOK_PHASE_WEIGHT * COUNT_SET_BITS(ptr_board->bitboards[W_ROOK] ^ ptr_board->bitboards[B_ROOK]);
    phase += BISHOP_PHASE_WEIGHT * COUNT_SET_BITS(ptr_board->bitboards[W_BISHOP] ^ ptr_board->bitboards[B_BISHOP]);
    phase += KNIGHT_PHASE_WEIGHT * COUNT_SET_BITS(ptr_board->bitboards[W_KNIGHT] ^ ptr_board->bitboards[B_KNIGHT]);
    //phase = (phase > TOTAL_PHASE_WEIGHT) ? TOTAL_PHASE_WEIGHT : phase;      // Accounting for early promotion

    return phase;
}
