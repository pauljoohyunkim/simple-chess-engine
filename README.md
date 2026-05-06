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

Note that most of the internal calculation is done by Cutechess anyways.
#### Prerequisites
* Stockfish
* Cutechess

0. Take an estimate ($R$) of the ELO for SCE.
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
9. Update the estimated ELO and repeat the process again if wanted.

### Benchmark Results (SEF)

| depth | R    | W  | D  | N   | dR     | Approx ELO    |
|-------|------|----|----|-----|--------|---------------|
| 7     | 2100 | 77 | 16 | 100 | 301.33 | 2401.3+/-82.6 |
| 8     | 2100 | 80 | 18 | 100 | 363.2  | 2463.2+/-84.3 |

### Benchmark Results (HCEF)
| depth | R    | W   | D   | N   | dR     | Approx ELO    |
|-------|------|-----|-----|-----|--------|---------------|
| 5     | 2200 | 237 | 141 | 500 | 66.8   | 2266.8+/-26.9 |

### Methodology in the Future
It turns out I could use CLI for this, which is probably better for testing (as I am away from my compute server machine often).

Here is a reference command I could use
```bash
# 4 Concurrent games where SCE uses depth 5 and Stockfish is at ELO 2100 with move time limit set to 2100.
# Playing ten games. Since 10 is even, the color switches back and forth.
cutechess-cli -engine name="SCE" cmd=./bin/sce_uci_engine option.DynamicDeepening=true depth=5 tc=inf -engine name="Stockfish" cmd=stockfish option.UCI_LimitStrength=true option.UCI_Elo=2100 tc=0/1 -each proto=uci -games 10 -repeat -concurrency 4
```
