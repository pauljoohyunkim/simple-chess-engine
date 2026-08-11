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
- The code compiles and runs correctly both with and without `-DNO_FUNCTION_POINTER`.
- The unit tests pass in both configurations (SEF tests are skipped when the macro is defined).
- The engine behaves as expected: when `NO_FUNCTION_POINTER` is defined, it only uses HCEF and the `EvalFunc` UCI option has no effect.

## Notes
- The `SCE_Engine` struct retains the function pointer members even when `NO_FUNCTION_POINTER` is defined, but they are not used (the macros bypass them).
- The initial engine initialization in `src/bin/sce_play.c` and elsewhere continues to work because it passes HCEF function pointers, which are compatible with both modes.