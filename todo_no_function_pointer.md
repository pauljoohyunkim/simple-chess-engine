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

## Remaining Changes
### Code Changes
1. In `src/uci.c`, replace the function pointer call in the principal variation (PV) printing section with the `EVAL_FUNCTION` macro:
   ```c
   // Find this line (around line 517):
   const int pv_score = session->ptr_engine->eval_function(&ctx_pv, session->ptr_engine);
   // Replace with:
   const int pv_score = EVAL_FUNCTION(&ctx_pv, session->ptr_engine);
   ```

### Unit Test Updates
We need to update the unit tests to skip SEF-related tests when `NO_FUNCTION_POINTER` is defined, since SEF will not be used.

1. In `tests/test_engine.cpp`:
   - Wrap the two SEF tests (`Engine_SEF, AlphaBetaBestMove` and `Engine_SEF, IterativeDeepeningBestMove`) with `#ifndef NO_FUNCTION_POINTER`.
   - Example:
     ```c
     #ifndef NO_FUNCTION_POINTER
     TEST(Engine_SEF, AlphaBetaBestMove) { ... }
     TEST(Engine_SEF, IterativeDeepeningBestMove) { ... }
     #endif
     ```

2. In `tests/eval/test_sef.cpp`:
   - Wrap the entire file content in `#ifndef NO_FUNCTION_POINTER` (or wrap each test individually).
   - Example:
     ```c
     #ifndef NO_FUNCTION_POINTER
     // ... entire existing content ...
     #endif
     ```

### Verification
After making the above changes, verify:
- The code compiles and runs correctly both with and without `-DNO_FUNCTION_POINTER`.
- The unit tests pass in both configurations (SEF tests are skipped when the macro is defined).
- The engine behaves as expected: when `NO_FUNCTION_POINTER` is defined, it only uses HCEF and the `EvalFunc` UCI option has no effect.

## Notes
- The `SCE_Engine` struct retains the function pointer members even when `NO_FUNCTION_POINTER` is defined, but they are not used (the macros bypass them).
- The initial engine initialization in `src/bin/sce_play.c` and elsewhere continues to work because it passes HCEF function pointers, which are compatible with both modes.