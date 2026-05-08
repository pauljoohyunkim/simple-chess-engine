# Todo: Make internal functions static

## src/chess.c
- [ ] SCE_Chessboard_reset
- [x] xorshift
- [ ] SCE_ZobristTable_init
- [ ] SCE_PieceMovementPrecompute
- [x] SCE_Knight_Precompute
- [x] SCE_King_Precompute
- [x] SCE_Pawn_Precompute
- [x] SCE_Rays_Precompute
- [x] SCE_CastlingMask_Precompute
- [x] SCE_Knight_GeneratePseudoLegalMoves
- [x] SCE_King_GeneratePseudoLegalMoves
- [x] SCE_Slider_GeneratePseudoLegalMoves
- [x] SCE_Pawn_GeneratePseudoLegalMoves

## include/chess.h
- [ ] Remove declaration of SCE_Chessboard_reset
- [ ] Remove declaration of SCE_ZobristTable_init
- [ ] Remove declaration of SCE_PieceMovementPrecompute

## src/engine.c
- [x] SCE_Search_MakeMove_Wrapper
- [x] SCE_Engine_AddTransposition
- [x] SCE_Engine_GetTranspositionData
- [x] SCE_Engine_ScoreMove
- [x] SCE_Engine_OrderMove_MVVLVA
- [ ] SCE_DetectRepetition
- [ ] SCE_DetectInsufficientMaterial

## include/engine.h
- [ ] Remove declaration of SCE_DetectRepetition
- [ ] Remove declaration of SCE_DetectInsufficientMaterial
