"""
Elo measurement for Photon via a fastchess round-robin + Ordo.

Configured with Hydra (see configs/elo.yaml). Builds Photon commit(s),
optionally includes Maia3-5M reference bots, then fits ratings with Ordo.

Assumes the working directory is `tools/`. Maia requires the optional `[elo]`
extra (`uv sync --extra elo`).

Examples:
  uv run python scripts/elo.py engines.photon=[HEAD]
  uv run python scripts/elo.py engines.photon=[new,old] engines.maia=null \\
      anchor.name=old anchor.rating=2300
"""

from __future__ import annotations

import csv
import os
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from tempfile import TemporaryDirectory

import hydra
from git import Repo
from omegaconf import DictConfig, OmegaConf

os.environ["PHOTON_DISABLE_LOGGING"] = "1"


@dataclass(frozen=True)
class PlayerRating:
    name: str
    rating: float
    error: float | None  # None for the Ordo anchor (fixed rating)
    points: float
    played: int


@dataclass(frozen=True)
class BuiltEngine:
    commit: str
    commit_hash: str
    name: str
    path: Path


def find_repo_root(path: Path) -> Path:
    path = path.resolve()
    while not (path / ".git").exists():
        if path == path.parent:
            raise ValueError(f"No .git directory found in {path}")
        path = path.parent
    return path


def clone_and_build(builds_dir: Path, commit: str) -> BuiltEngine:
    repo_root = find_repo_root(Path(__file__).parent)
    repo = Repo(repo_root)
    git_url = repo.remote().url

    commit_hash: str = repo.git.rev_parse(commit)
    build_dir = builds_dir / commit_hash
    executable_path = build_dir / "photon"

    if not executable_path.exists():
        with TemporaryDirectory() as temp_dir:
            repo = Repo.clone_from(git_url, temp_dir)
            repo.git.checkout(commit)
            build_dir.mkdir(exist_ok=True)

            subprocess.run(
                ["cmake", "-B", str(build_dir), "-DCMAKE_BUILD_TYPE=Release"],
                cwd=temp_dir,
                check=True,
            )
            subprocess.run(
                [
                    "cmake",
                    "--build",
                    str(build_dir),
                    "--target",
                    "photon",
                    "-j",
                    str((os.cpu_count() or 2) // 2),
                ],
                cwd=temp_dir,
                check=True,
            )

    assert executable_path.exists(), f"Engine executable not found at {executable_path}"
    return BuiltEngine(
        commit=commit,
        commit_hash=commit_hash,
        name=f"photon-{commit_hash[:12]}",
        path=executable_path.resolve(),
    )


def resolve_command(cmd: str | Path) -> Path | str:
    """Return an absolute path if cmd is a file path, else require it on PATH."""
    path = Path(cmd)
    if path.is_file() or path.is_absolute() or "/" in str(cmd):
        resolved = path.resolve()
        if not resolved.exists():
            raise SystemExit(f"Not found: {resolved}")
        return resolved
    found = shutil.which(str(cmd))
    if found is not None:
        return found
    venv_bin = Path(__file__).resolve().parent.parent / ".venv" / "bin" / str(cmd)
    if venv_bin.exists():
        return venv_bin
    raise SystemExit(
        f"Command not found on PATH: {cmd}\n"
        "Install Maia with: uv sync --extra elo\n"
        "Then ensure the tools venv is active (or set engines.maia.cmd)."
    )


def require_file(path: Path, hint: str) -> Path:
    resolved = path.resolve()
    if not resolved.exists():
        raise SystemExit(f"Not found: {resolved}\n{hint}")
    return resolved


def resolve_anchor_name(
    anchor_name: str,
    engines: list[BuiltEngine],
    maia_elos: list[int] | None,
) -> str:
    """Map a user anchor (commit / hash / engine name) to a fastchess engine name."""
    engine_names = {e.name for e in engines}
    maia_names = {f"maia{elo}" for elo in (maia_elos or [])}
    if anchor_name in engine_names or anchor_name in maia_names:
        return anchor_name

    repo_root = find_repo_root(Path(__file__).parent)
    repo = Repo(repo_root)
    try:
        want = repo.git.rev_parse(anchor_name)
    except Exception:
        want = None

    if want is not None:
        for e in engines:
            if e.commit_hash == want or e.commit_hash.startswith(anchor_name):
                return e.name
        candidate = f"photon-{want[:12]}"
        if candidate in engine_names:
            return candidate

    known = sorted(engine_names | maia_names)
    raise SystemExit(
        f"Anchor engine {anchor_name!r} not found among tournament engines.\n"
        f"Known: {', '.join(known)}"
    )


def parse_ordo_csv(path: Path) -> list[PlayerRating]:
    players: list[PlayerRating] = []
    with path.open(newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            error_raw = (row.get("ERROR") or "").strip().strip('"')
            error: float | None
            if error_raw in {"", "-", "----"}:
                error = None
            else:
                error = float(error_raw)
            players.append(
                PlayerRating(
                    name=row["PLAYER"].strip().strip('"'),
                    rating=float(row["RATING"]),
                    error=error,
                    points=float(row["POINTS"]),
                    played=int(float(row["PLAYED"])),
                )
            )
    return players


def format_rating_ci(player: PlayerRating, confidence: float) -> str:
    if player.error is None:
        return f"{player.rating:.1f} (anchor)"
    return f"{player.rating:.1f} ± {player.error:.1f} ({confidence:.0f}% CI)"


def print_ratings(
    players: list[PlayerRating],
    highlight_names: list[str],
    confidence: float,
    out_dir: Path,
) -> None:
    by_name = {p.name: p for p in players}
    highlights = [by_name[n] for n in highlight_names if n in by_name]
    if not highlights:
        raise SystemExit(
            f"Highlight engines {highlight_names!r} missing from Ordo output"
        )

    print()
    print("=" * 60)
    if len(highlights) == 1:
        print(f"  Photon  {format_rating_ci(highlights[0], confidence)}")
    else:
        print("  Photon commits:")
        width = max(len(p.name) for p in highlights)
        for p in highlights:
            print(f"    {p.name:<{width}}  {format_rating_ci(p, confidence)}")
    print("=" * 60)
    print()
    print(f"All engines ({confidence:.0f}% CI):")
    highlight_set = set(highlight_names)
    ordered = highlights + [p for p in players if p.name not in highlight_set]
    name_width = max(len(p.name) for p in ordered)
    for p in ordered:
        print(
            f"  {p.name:<{name_width}}  {format_rating_ci(p, confidence)}"
            f"   ({p.points:g}/{p.played})"
        )
    print()
    print(f"Results written to {out_dir.resolve()}")


def validate_cfg(cfg: DictConfig) -> None:
    photon = list(cfg.engines.photon or [])
    if not photon:
        raise SystemExit("engines.photon must list at least one revision")

    use_maia = cfg.engines.maia is not None
    n_maia = len(list(cfg.engines.maia.elos)) if use_maia else 0
    if len(photon) + n_maia < 2:
        raise SystemExit(
            "Need at least two engines: add more engines.photon revisions "
            "and/or enable engines.maia"
        )

    if not use_maia:
        if cfg.anchor is None or not cfg.anchor.name or cfg.anchor.rating is None:
            raise SystemExit(
                "anchor.name and anchor.rating are required when engines.maia is null"
            )
        if str(cfg.anchor.name).startswith("maia"):
            raise SystemExit(
                "engines.maia is null but anchor.name looks like a Maia engine "
                f"({cfg.anchor.name!r}). Set anchor.name to a Photon commit / "
                "short hash / photon-<sha12> and anchor.rating to its known Elo."
            )


def run(cfg: DictConfig) -> None:
    validate_cfg(cfg)

    photon_revs = [str(c) for c in cfg.engines.photon]
    use_maia = cfg.engines.maia is not None
    maia_elos = [int(e) for e in cfg.engines.maia.elos] if use_maia else []
    jobs = int(cfg.jobs) if cfg.jobs is not None else (os.cpu_count() or 1)

    fastchess = require_file(
        Path(str(cfg.paths.fastchess)),
        "Install fastchess under tools/bin (see tools/README.md).",
    )
    ordo = require_file(
        Path(str(cfg.paths.ordo)),
        "Build Ordo under tools/bin/ordo (see tools/README.md).",
    )
    opening_book = require_file(
        Path(str(cfg.paths.opening_book)),
        "Install the opening book under tools/bin (see tools/README.md).",
    )
    maia_cmd = resolve_command(str(cfg.engines.maia.cmd)) if use_maia else None

    builds_dir = Path(str(cfg.paths.builds_dir)).resolve()
    builds_dir.mkdir(exist_ok=True)

    engines_by_commit: dict[str, BuiltEngine] = {}
    with ThreadPoolExecutor(max_workers=min(4, len(photon_revs))) as pool:
        futures = {
            pool.submit(clone_and_build, builds_dir, c): c for c in photon_revs
        }
        for fut in as_completed(futures):
            built = fut.result()
            engines_by_commit[futures[fut]] = built

    engines: list[BuiltEngine] = []
    seen_hashes: set[str] = set()
    for rev in photon_revs:
        e = engines_by_commit[rev]
        if e.commit_hash in seen_hashes:
            print(
                f"Warning: skipping duplicate commit {e.commit} "
                f"({e.commit_hash[:12]})",
                flush=True,
            )
            continue
        seen_hashes.add(e.commit_hash)
        engines.append(e)

    anchor_name = resolve_anchor_name(str(cfg.anchor.name), engines, maia_elos or None)
    anchor_elo = float(cfg.anchor.rating)

    if not use_maia and not any(e.name == anchor_name for e in engines):
        raise SystemExit(
            f"Anchor {anchor_name!r} is not one of the built engines: "
            f"{', '.join(e.name for e in engines)}"
        )

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    out_dir = Path("results") / f"elo_{timestamp}"
    out_dir.mkdir(parents=True, exist_ok=True)
    # Persist resolved config for reproducibility.
    OmegaConf.save(cfg, out_dir / "config.yaml")

    pgn_path = out_dir / "games.pgn"
    ratings_txt = out_dir / "ratings.txt"
    ratings_csv = out_dir / "ratings.csv"

    cmd: list[str] = [str(fastchess)]
    for e in engines:
        cmd.extend(
            [
                "-engine",
                f"cmd={e.path}",
                f"name={e.name}",
                f"tc={cfg.time_control}",
            ]
        )
    if use_maia:
        assert maia_cmd is not None
        maia_args = str(cfg.engines.maia.args or "").strip()
        maia_tc = str(cfg.engines.maia.tc)
        for elo in maia_elos:
            engine = [
                "-engine",
                f"cmd={maia_cmd}",
                f"name=maia{elo}",
                f"option.Elo={elo}",
                "option.Temperature=0",
                "option.MultiPV=1",
                f"tc={maia_tc}",
            ]
            if maia_args:
                engine.append(f"args={maia_args}")
            cmd.extend(engine)

    cmd.extend(
        [
            "-tournament",
            "roundrobin",
            "-rounds",
            str(int(cfg.rounds)),
            "-repeat",
            "-concurrency",
            str(jobs),
            "-recover",
            "-openings",
            f"file={opening_book}",
            "format=pgn",
            "-pgnout",
            f"file={pgn_path}",
            "-config",
            f"outname={out_dir / 'fastchess.json'}",
        ]
    )

    participants = [e.name for e in engines] + [f"maia{e}" for e in maia_elos]
    print(
        f"Running round-robin ({cfg.rounds} rounds) with "
        f"{len(participants)} engines → {pgn_path}",
        flush=True,
    )
    print(f"  Engines: {', '.join(participants)}", flush=True)
    subprocess.run(cmd, check=True)

    if not pgn_path.exists() or pgn_path.stat().st_size == 0:
        raise SystemExit(f"No games written to {pgn_path}")

    print(
        f"Fitting ratings with Ordo (anchor {anchor_name} = {anchor_elo:g}, "
        f"{cfg.ordo.sims} sims, {cfg.ordo.confidence:.0f}% CI)…",
        flush=True,
    )
    subprocess.run(
        [
            str(ordo),
            "-a",
            str(anchor_elo),
            "-A",
            anchor_name,
            "-p",
            str(pgn_path),
            "-o",
            str(ratings_txt),
            "-c",
            str(ratings_csv),
            "-s",
            str(int(cfg.ordo.sims)),
            "-F",
            str(float(cfg.ordo.confidence)),
            "-n",
            str(jobs),
        ],
        check=True,
    )

    players = parse_ordo_csv(ratings_csv)
    print_ratings(
        players,
        [e.name for e in engines],
        float(cfg.ordo.confidence),
        out_dir,
    )
    print(ratings_txt.read_text())


@hydra.main(version_base=None, config_path="../configs", config_name="elo")
def main(cfg: DictConfig) -> None:
    try:
        run(cfg)
    except subprocess.CalledProcessError as exc:
        print(f"Command failed with exit code {exc.returncode}", file=sys.stderr)
        raise SystemExit(exc.returncode) from exc


if __name__ == "__main__":
    main()
