#ifndef SCE_UCI_H
#define SCE_UCI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "chess.h"

/**
 * @brief Convert SCE_ChessMove to UCI move string
 * 
 * @param move Chess move used internally in SCE.
 * @param uci_string Output string
 * @return true Success
 * @return false Failure (Possibly not a correct move, or uci_string is empty)
 * 
 * Note that this simply constructs src + dst + promotion.
 * This does not encode information about castling or en passant or any other special moves.
 * Note that EMPTY_MOVE is handled, and will return true.
 */
bool SCE_MoveToUCIString(const SCE_ChessMove move, char uci_string[6]);

/**
 * @brief Convert UCI move string to SCE_ChessMove
 * 
 * @param uci_string UCI string
 * @return SCE_ChessMove Move parsed from UCI string, or EMPTY_MOVE (0)
 */
SCE_ChessMove SCE_UCIStringToMove(const char* const uci_string);

#ifdef __cplusplus
}
#endif

#endif  // SCE_UCI_H
