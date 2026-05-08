# List of tests that only verify basic initialization and should be replaced with more comprehensive functionality tests

## Tests to Review and Enhance

1. **MoveGeneration_Initial** (`test_chess.cpp`)
   - Currently: Verifies that white knight has 4 possible moves from initial position
   - Suggested enhancement: Test move generation for all piece types from initial position, test move generation after various sequences of moves, test edge cases like blocked pieces

2. **MakeMove_Initial** (`test_chess.cpp`)
   - Currently: Verifies that e2e4 move can be made and updates board state correctly
   - Suggested enhancement: Test various types of moves (captures, promotions, castling, en passant) from initial position, test move legality detection, test move making/unmaking symmetry

3. **SEF_Initial** (`test_evolve/test_sef.cpp`)
   - Currently: Verifies engine initializes successfully and evaluation function returns 0 for initial position
   - Suggested enhancement: Test evaluation function with various board positions, test different evaluation functions, test endgame vs middlegame evaluation

4. **SEF_DeltaEval_Initial** (`test_evolve/test_sef.cpp`)
   - Currently: Tests delta evaluation functionality with move generation loop
   - Suggested enhancement: Test delta evaluation with various positions and move sequences, test consistency with full evaluation in complex positions

5. **SEF_DeltaEval_Kiwipete_Depth_2** (`test_evolve/test_sef.cpp`)
   - Currently: Tests delta evaluation at depth 2 with Kiwipete position
   - Suggested enhancement: Test delta evaluation at various depths with different positions, test performance characteristics

6. **HCEF_Initial** (`test_evolve/test_hcef.cpp`)
   - Currently: Verifies context and engine initialization, sets up specific position, checks evaluation consistency
   - Suggested enhancement: Test evaluation function with various board positions, test different evaluation functions, test edge cases like blocked positions

7. **HCEF_DeltaEval_Initial** (`test_evolve/test_hcef.cpp`)
   - Currently: Tests delta evaluation functionality with move generation loop
   - Suggested enhancement: Test delta evaluation with various positions and move sequences, test consistency with full evaluation in complex positions

8. **HCEF_DeltaEval_Kiwipete_Depth_4** (`test_evolve/test_hcef.cpp`)
   - Currently: Tests delta evaluation at depth 4 with Kiwipete position
   - Suggested enhancement: Test delta evaluation at various depths with different positions, test performance characteristics