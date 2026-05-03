# Simple Chess Engine
Here is my attempt at a chess engine.

So far it uses simplified evaluation function (SEF) with alpha-beta negamax search + quiescence negamax search
as its engine.

![Screenshot](./screenshot.png)

## Building
To build the playable binary,
```bash
make bin    # Run the binary at bin/sce_play
```
Note that you can only play as white at the moment, as this is a test binary,
though I might make you be able to choose.

To build the unit test,
```bash
make test   # Run the test at bin/test
```
To build the documentation (although at the moment it is not maintained, it may be later)
```bash
make doc    # Requires Doxygen
```

## Note
There are clearly a lot of work to do, but so far I think it is of good quality for a PoC engine
(for something that was built hastily in 3 weeks)!

## Benchmark
### Methodology
Here are the steps taken for the measurements.
I acknowledge that the measurement could be more formal,
but I do not want to pull my hair out for this hobby project.
#### Prerequisites
* Stockfish
* Cutechess

0. Take an estimate of the ELO for SCE.
1. Download and install Stockfish and Cutechess.
2. Open Cutechess GUI
    * I wanted to use Cutechess CLI, but it seems to be buggy on my machine...
3. Add `bin/sce_uci_engine` as a new engine in Cutechess GUI.
    * Go to Advanced, and toggle on the DynamicDeepening option.
4. Add Stockfish as a new engine in Cutechess GUI.
    * Go to Advanced, and toggle UCI_LimitStrength on.
    * Also set UCI_Elo value to the ELO estimate from Step 0.
5. Play 100 games of tournament between SCE and Stockfish.
6. Using the number of wins ($W$) and draws ($D$), calculate the expected score ($E$).
```math
E = \frac{W + 0.5 D}{N}
```
where
* $W$ is the number of wins.
* $D$ is the number of draws.
* $N$ is the total number of games played.

7. Calculate the ELO difference by the following formula.
```math
\Delta R = -400 \log_{10} \left( \frac{1}{E} - 1 \right)
```

8. Add this $\Delta R$ to the estimated ELO.

    
