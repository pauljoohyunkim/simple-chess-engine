# TODO: Implement NO_FUNCTION_POINTER macro approach

## Completed Changes
1. Added macros `EVAL_FUNCTION` and `DELTA_EVAL_FUNCTION` in `include/engine.h`:
   - When `NO_FUNCTION_POINTER` is defined, they directly call the HCEF functions.
   - Otherwise, they use the function pointers in `SCE_Engine`.
2. Modified `src/engine.c`:
   - Made parameter validation in `SCE_Engine_init` conditional on `NO_FUNCTION_POINTER` (only validate function pointers when the macro is not defined).
   - Replaced the initial evaluation call with `EVAL_FUNCTION(ctx, ptr_engine)`.
   - Replaced the delta evaluation call in `SCE_Search_MakeMove_Wrapper` with `DELTA_EVAL_FUNCTION`.
3. Modified `src/uci.c`:
   - Wrapped the `EvalFunc` option handling in `#ifndef NO_FUNCTION_POINTER` so the option is ignored when the macro is defined.
   - Replaced the function pointer call in the principal variation (PV) printing section with the `EVAL_FUNCTION` macro.
4. Updated unit tests:
   - Wrapped the two SEF tests in `tests/test_engine.cpp` with `#ifndef NO_FUNCTION_POINTER`.
   - Wrapped the entire file content in `tests/eval/test_sef.cpp` with `#ifndef NO_FUNCTION_POINTER`.

## Verification Needed
After making the above changes, verify:
- [x] The code compiles and runs correctly both with and without `-DNO_FUNCTION_POINTER`.
- [x] The unit tests pass in both configurations (SEF tests are skipped when the macro is defined).
- [x] The engine behaves as expected: when `NO_FUNCTION_POINTER` is defined, it only uses HCEF and the `EvalFunc` UCI option has no effect.

## Verification Results
- **With `-DNO_FUNCTION_POINTER`:** 33 tests pass (HCEF, PERFT, MakeMove, FEN, ChessBoard, MoveGeneration, Zobrist, UCI). Engine runs correctly using only HCEF evaluation.
- **Without `-DNO_FUNCTION_POINTER`:** All 39 tests pass (all above + 2 SEF engine tests). Function-pointer dispatch works normally.
- **Key fix:** Changed `tests/eval/delta_eval_test.h` to use `EVAL_FUNCTION()` / `DELTA_EVAL_FUNCTION()` macros instead of raw function pointer access, which was causing a segfault (`0x0` in `DeltaEvalTest`) when compiled with `NO_FUNCTION_POINTER` because the pointers were never initialized.
- **No more scattered conditionals:** All call sites now use the `EVAL_FUNCTION` / `DELTA_EVAL_FUNCTION` macros uniformly. No `#ifndef NO_FUNCTION_POINTER` needed outside of `engine.h`, `src/engine.c`, and test file wrappers.

## Notes
- The `SCE_Engine` struct retains the function pointer members even when `NO_FUNCTION_POINTER` is defined, but they are not used (the macros bypass them).
- The initial engine initialization in `src/bin/sce_play.c` and elsewhere continues to work because it passes HCEF function pointers, which are compatible with both modes.