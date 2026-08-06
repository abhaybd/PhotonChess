# PhotonChess

Photon is a UCI-compliant chess engine written from scratch in C++! It uses Principal-Variation Search, tapered piece-square table static evaluation, and the following search optimizations:
- Iterative Deepening
- Transposition Table (also for move ordering)
- Quiescence Search
- Null-Move Pruning
- Aspiration Windows
- MVV-LVA capture move ordering
- Killer Heuristic
- History Heuristic (with history gravity and malus)
- Static Exchange Evaluation (for move ordering and quiescence search pruning)
- Soft/Hard Time Management
- Lazy Legal Move Filtering
- Late Move Reduction
- Pondering

## Environment Variables

The following environment variables can be used to configure non-search behavior.

| Variable                 | Description                                                                                              | Default              |
| ------------------------ | -------------------------------------------------------------------------------------------------------- | -------------------- |
| `PHOTON_DISABLE_LOGGING` | If set (to any value), disables writing logs to a file.                                                  | unset (logging on)   |
| `PHOTON_LOG_FILE`        | Path to the log file. Ignored when `PHOTON_DISABLE_LOGGING` is set.                                      | `photonlog.txt`      |
| `PHOTON_PROFILE_FILE`    | Path to the profiler output file. Only used in builds configured with `-DPHOTON_PROFILING_ENABLED=ON`.   | `photon_profile.txt` |

## Usage (GUI)

You can play against Photon in a GUI with [cutechess](https://github.com/cutechess/cutechess). Install it or build it from source, add Photon as an engine in the preferences, and then start a new game!

## Development

Photon is developed with C++20. To configure and build:

```bash
mkdir -p build ; cd build
cmake ..
cmake --build . -j
```

By default, photon is compiled in release mode with compiler optimizations enabled. To compile in debug mode with debug symbols, add `-DCMAKE_BUILD_TYPE=Debug`.

### Testing

Photon is tested at several levels, from fast unit tests to full engine-vs-engine matches.

#### Unit tests

Unit tests are written with [Catch2](https://github.com/catchorg/Catch2) and compiled into a single `tests` executable as part of the normal build. From the build directory, run all tests with:

```bash
./tests
```

You can filter by test name or tag, e.g. `./tests "[core]"` to run only the core tests.

#### Perft benchmarks

[Perft](https://www.chessprogramming.org/Perft) (performance test) counts the number of leaf nodes reachable at a given depth from a set of known positions, validating both move generation correctness and speed. The correctness tests run as part of the normal `./tests` run, while the timing benchmarks are tagged `[!benchmark]` and are hidden by default. Run them explicitly with:

```bash
./tests "[perft]" --benchmark-samples 10
```

#### ERET

The [Eigenmann Rapid Engine Test (ERET)](https://www.chessprogramming.org/Eigenmann_Rapid_Engine_Test) measures engine strength by having Photon solve a suite of tactical/positional puzzles over UCI. See `tools/README.md` for setup and usage.

#### SPRT

An [SPRT (Sequential Probability Ratio Test)](https://www.chessprogramming.org/Sequential_Probability_Ratio_Test) plays many games between two commits to determine, with statistical confidence, whether one is stronger than the other. See `tools/README.md` for setup and usage.

### Profiling

Photon can be built with profiling enabled, for debugging. To do so, add `-DPHOTON_PROFILING_ENABLED=ON` to the configure command. When running, the profiler output will be saved to `photon_profile.txt` by default.

### Code Format

See `.clang-format` for style rules. Code can be automatically formated with `clang-format`, or running `./format.sh`. Note that `clang-format` v16 or greater is required.
