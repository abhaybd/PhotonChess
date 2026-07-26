# PhotonChess-Tools

A collection of python tools to aid development of PhotonChess. Communicates with a built executable via UCI.

Supports:
 - [Sequential Probability Ratio Tests](https://www.chessprogramming.org/Sequential_Probability_Ratio_Test)
 - [Eigenmann Rapid Engine Test (ERET)](https://www.chessprogramming.org/Eigenmann_Rapid_Engine_Test)

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

See individual script documentation for usage information. In general, the working directory is assumed to be the `tools` directory (this directory).

### ERET

Runs the engine against the ERET puzzle suite over UCI and reports how many positions it solves, along with average search stats. Point it at a built engine:

```bash
python scripts/eret.py --engine ../build/photon
```

See `python scripts/eret.py --help` for the full set of options.

### SPRT

Plays many games between two commits to determine, with statistical confidence, whether one is stronger than the other. Each commit is cloned and built automatically. Pass the two revisions to compare:

```bash
python scripts/sprt.py <new_commit> <old_commit>
```

See `python scripts/sprt.py --help` for the full set of options.
