"""
SPRT utility for Photon.
This tool runs SPRT on two commits to test if there is a performance difference between them:
This will print a lot of output, and at the end if it prints H0 or H1 are accepted, it means respectively that the engines are not significantly different, or they are. SPRT results are persisted under `results/` in a json.
"""

import argparse
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime
from pathlib import Path
import os
import subprocess
from tempfile import TemporaryDirectory

from git import Repo


os.environ["PHOTON_DISABLE_LOGGING"] = "1"


def get_args():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("new_commit", help="The revision with the new changes")
    parser.add_argument("old_commit", help="The revision with the old changes to test against")
    parser.add_argument("--builds-dir", type=Path, default=Path("builds"))

    parser.add_argument("--fastchess", type=Path, default=Path("bin/fastchess/fastchess"), help="Path to the fastchess executable")
    parser.add_argument("--opening-book", type=Path, default=Path("bin/8moves_v3.pgn"), help="Path to the PGN opening book")
    parser.add_argument("--rounds", type=int, default=15000, help="Number of rounds to run")
    parser.add_argument("-j", "--jobs", type=int, default=os.cpu_count(), help="Number of concurrent jobs to run")
    parser.add_argument("-a", "--alpha", type=float, default=0.05, help="Alpha level for SPRT")
    parser.add_argument("-b", "--beta", type=float, default=0.05, help="Beta level for SPRT")
    parser.add_argument("--elo-delta", type=float, default=5.0, help="Elo delta for SPRT (nElo if normalized, Elo if logistic)")
    parser.add_argument("--regression", action="store_true", help="Run a regression test instead of gain test")
    parser.add_argument("--time-control", default="8+0.08")
    parser.add_argument("--sprt-model", default="logistic", choices=["normalized", "logistic"], help="SPRT model to use")
    return parser.parse_args()

def find_repo_root(path: Path) -> Path:
    path = path.resolve()
    while not (path / ".git").exists():
        if path == path.parent:
            raise ValueError(f"No .git directory found in {path}")
        path = path.parent
    return path

def clone_and_build(builds_dir: Path, commit: str):
    repo_root = find_repo_root(Path(__file__).parent)
    repo = Repo(repo_root)
    git_url = repo.remote().url

    commit_hash: str = repo.git.rev_parse(commit)
    build_dir = builds_dir / commit_hash
    executable_path = build_dir / "photon"

    # If we've already built the engine, shortcut
    if executable_path.exists():
        return commit_hash, executable_path

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
            ["cmake", "--build", str(build_dir), "--target", "photon", "-j", str(os.cpu_count() // 2)],
            cwd=temp_dir,
            check=True,
        )

    assert executable_path.exists(), f"Engine executable not found at {executable_path}"
    return commit_hash, executable_path

def main():
    args = get_args()

    builds_dir = args.builds_dir.resolve()
    builds_dir.mkdir(exist_ok=True)

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")

    with ThreadPoolExecutor(max_workers=2) as executor:
        future1 = executor.submit(clone_and_build, builds_dir, args.new_commit)
        future2 = executor.submit(clone_and_build, builds_dir, args.old_commit)
        commit1, executable1 = future1.result()
        commit2, executable2 = future2.result()

    if args.regression:
        elo0 = -args.elo_delta
        elo1 = 0
    else:
        elo0 = 0
        elo1 = args.elo_delta

    subprocess.run(
        [
            str(args.fastchess),
            "-engine", f"cmd={executable1}", f"name={commit1}",
            "-engine", f"cmd={executable2}", f"name={commit2}",
            "-config", f"outname=results/sprt_{timestamp}.json",
            "-each", f"tc={args.time_control}",
            "-rounds", str(args.rounds),
            "-repeat",
            "-concurrency", str(args.jobs),
            "-recover",
            "-openings", f"file={args.opening_book}", "format=pgn",
            "-sprt", f"elo0={elo0}", f"elo1={elo1}",
                f"alpha={args.alpha}", f"beta={args.beta}", f"model={args.sprt_model}",
        ],
        check=True,
    )

if __name__ == "__main__":
    main()