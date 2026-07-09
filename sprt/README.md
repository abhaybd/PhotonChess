# SPRT

This is a small tool to run [Sequential Probability Ratio Tests](https://www.chessprogramming.org/Sequential_Probability_Ratio_Test) with the photon engine.

## Installation

First, install fastchess:

```bash
mkdir bin ; cd bin
git clone https://github.com/Disservin/fastchess.git
cd fastchess
make -j
```

Then install the opening book:

```bash
cd bin
wget https://github.com/official-stockfish/books/raw/refs/heads/master/8moves_v3.pgn.zip
unzip 8moves_v3.pgn.zip
rm 8moves_v3.pgn.zip
```

## Usage

This tool runs SPRT on two commits to test if there is a performance difference between them:

```bash
PHOTON_DISABLE_LOGGING=1 uv run main.py <commit_1> <commit_2>
```

This will print a lot of output, and at the end if it prints H0 or H1 are accepted, it means respectively that the engines are not significantly different, or they are. SPRT results are persisted under `results/` in a json.
