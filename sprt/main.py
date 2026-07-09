import argparse
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime
from pathlib import Path
import os
import shutil
import subprocess

from git import Repo

def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("commit1")
    parser.add_argument("commit2")
    parser.add_argument("--builds-dir", type=Path, default=Path("builds"))

    parser.add_argument("--fastchess", type=Path, default=Path("bin/fastchess/fastchess"))
    parser.add_argument("--opening-book", type=Path, default=Path("bin/8moves_v3.pgn"))
    parser.add_argument("--rounds", type=int, default=15000)
    parser.add_argument("-j", "--jobs", type=int, default=os.cpu_count())
    parser.add_argument("-a", "--alpha", type=float, default=0.05)
    parser.add_argument("-b", "--beta", type=float, default=0.05)
    parser.add_argument("--elo-delta", type=float, default=5.0)
    parser.add_argument("--time-control", default="8+0.08")
    parser.add_argument("--sprt-model", default="logistic", choices=["normalized", "logistic"])
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

    commit_hash = repo.git.rev_parse(commit)
    clone_dir = builds_dir / commit_hash
    build_dir = clone_dir / "build"
    executable_path = build_dir / "photon"

    # If we've already built the engine, shortcut. Otherwise delete and rebuild.
    if executable_path.exists():
        return commit_hash, executable_path
    shutil.rmtree(clone_dir, ignore_errors=True)
    clone_dir.mkdir()

    repo = Repo.clone_from(git_url, clone_dir)
    repo.git.checkout(commit)
    build_dir.mkdir(exist_ok=True)

    subprocess.run(
        ["cmake", "-B", str(build_dir), "-DCMAKE_BUILD_TYPE=Release"],
        cwd=str(clone_dir),
        check=True,
    )
    subprocess.run(
        ["cmake", "--build", str(build_dir), "--target", "photon", "-j", str(os.cpu_count() // 2)],
        cwd=str(clone_dir),
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
        future1 = executor.submit(clone_and_build, builds_dir, args.commit1)
        future2 = executor.submit(clone_and_build, builds_dir, args.commit2)
        commit1, executable1 = future1.result()
        commit2, executable2 = future2.result()

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
            "-sprt", "elo0=0", f"elo1={args.elo_delta}",
                f"alpha={args.alpha}", f"beta={args.beta}", f"model={args.sprt_model}",
        ],
        check=True,
    )

if __name__ == "__main__":
    main()