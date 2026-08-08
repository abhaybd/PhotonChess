# PhotonChess-Tools

A collection of python tools to aid development of PhotonChess. Communicates with a built executable via UCI.

Supports:
 - [Sequential Probability Ratio Tests](https://www.chessprogramming.org/Sequential_Probability_Ratio_Test)
 - [Eigenmann Rapid Engine Test (ERET)](https://www.chessprogramming.org/Eigenmann_Rapid_Engine_Test)
 - Elo measurement against [Maia3-5M](https://github.com/CSSLab/maia3) reference bots (via fastchess + [Ordo](https://github.com/michiguel/Ordo))

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

For Elo measurement, also build Ordo:

```bash
cd bin
git clone https://github.com/michiguel/Ordo.git ordo
cd ordo
make -j
```

Maia3 is optional and torch-heavy; install it via the `[elo]` extras group:

```bash
uv sync --extra elo
maia3-cache --model maia3-5m   # pre-download Hugging Face weights (optional; first UCI run downloads otherwise)
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

### Elo

Builds Photon commit(s), runs a fastchess round-robin, then fits ratings with Ordo (95% CI). Configuration is Hydra/YAML — default config is [`configs/elo.yaml`](configs/elo.yaml) (Photon vs Maia3-5M). Requires the `[elo]` extra (Maia + Hydra) and Ordo (see Installation).

```bash
uv sync --extra elo
# Measure HEAD against Maia (default config)
uv run python scripts/elo.py engines.photon=[HEAD]
# Multiple Photon commits + Maia
uv run python scripts/elo.py engines.photon=[newsha,oldsha]
# Commits only (no Maia): null out maia and set an anchor
uv run python scripts/elo.py engines.photon=[newsha,oldsha] engines.maia=null \
    anchor.name=oldsha anchor.rating=2300
# Smoke test
uv run python scripts/elo.py engines.photon=[HEAD] rounds=1 jobs=2
```

Overrides use Hydra syntax (`key=value`, lists as `[a,b]`). The summary prints Photon rating(s) first with ± CI. Results (resolved config, PGN, Ordo ratings) land under `results/elo_<timestamp>/`. See `configs/elo.yaml` for all knobs.
