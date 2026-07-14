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
