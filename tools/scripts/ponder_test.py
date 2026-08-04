"""
Ponder / infinite search test harness for Photon.

Drives a built Photon binary over UCI and checks:
  - no bestmove while still pondering (time / depth / mate must not end ponder early)
  - ponderhit continues search and applies prepared limits before bestmove
  - pondermiss (stop) yields bestmove promptly
  - stop after ponderhit still yields bestmove
  - go infinite must not emit bestmove until stop (including mate positions)

Assumes the working directory is `tools/` (same as the other scripts), unless
`--engine` is an absolute path.

Example:
  python scripts/ponder_test.py --engine ../build/photon
"""

from __future__ import annotations

import argparse
import os
import queue
import subprocess
import sys
import threading
import time
from dataclasses import dataclass
from pathlib import Path

os.environ["PHOTON_DISABLE_LOGGING"] = "1"

STARTPOS = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
# White to move, mate in 1 (Rf8#)
MATE_IN_ONE = "6k1/8/6K1/8/8/8/8/5R2 w - - 0 1"


@dataclass
class BestMove:
    move: str
    ponder: str | None
    raw: str


@dataclass
class TestResult:
    name: str
    passed: bool
    detail: str
    elapsed_s: float


class UCIEngine:
    """UCI engine process with a background stdout reader."""

    def __init__(self, path: Path, startup_timeout_s: float = 5.0):
        self._proc = subprocess.Popen(
            [str(path)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            bufsize=1,
        )
        assert self._proc.stdin is not None
        assert self._proc.stdout is not None
        self._stdin = self._proc.stdin
        self._lines: queue.Queue[str | None] = queue.Queue()
        self._reader = threading.Thread(target=self._read_loop, daemon=True)
        self._reader.start()

        self.send("uci")
        self.wait_for("uciok", timeout_s=startup_timeout_s)
        self.send("isready")
        self.wait_for("readyok", timeout_s=startup_timeout_s)

    def _read_loop(self) -> None:
        assert self._proc.stdout is not None
        try:
            while True:
                line = self._proc.stdout.readline()
                if not line:
                    break
                self._lines.put(line.rstrip("\n"))
        finally:
            self._lines.put(None)

    def send(self, command: str) -> None:
        if self._proc.poll() is not None:
            raise RuntimeError(f"Engine already exited (code {self._proc.returncode})")
        self._stdin.write(command + "\n")
        self._stdin.flush()

    def _get_line(self, timeout_s: float) -> str:
        try:
            line = self._lines.get(timeout=timeout_s)
        except queue.Empty as e:
            raise TimeoutError(f"Timed out after {timeout_s:.3f}s waiting for engine output") from e
        if line is None:
            raise RuntimeError("Engine exited unexpectedly")
        return line

    def wait_for(self, prefix: str, timeout_s: float = 5.0) -> str:
        deadline = time.monotonic() + timeout_s
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TimeoutError(f"Timed out waiting for {prefix!r}")
            line = self._get_line(remaining)
            if line.startswith(prefix):
                return line

    def drain(self, duration_s: float) -> list[str]:
        """Collect lines for duration_s without failing if idle."""
        deadline = time.monotonic() + duration_s
        lines: list[str] = []
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                break
            try:
                line = self._lines.get(timeout=min(remaining, 0.05))
            except queue.Empty:
                continue
            if line is None:
                raise RuntimeError("Engine exited unexpectedly")
            lines.append(line)
        return lines

    def expect_no_bestmove(self, duration_s: float) -> list[str]:
        lines = self.drain(duration_s)
        bestmoves = [ln for ln in lines if ln.startswith("bestmove")]
        if bestmoves:
            raise AssertionError(
                f"Unexpected bestmove after {duration_s:.3f}s: {bestmoves[0]}"
            )
        return lines

    def wait_for_bestmove(self, timeout_s: float) -> BestMove:
        deadline = time.monotonic() + timeout_s
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TimeoutError(f"Timed out after {timeout_s:.3f}s waiting for bestmove")
            line = self._get_line(remaining)
            if line.startswith("bestmove"):
                return _parse_bestmove(line)

    def new_game(self) -> None:
        self.send("ucinewgame")
        self.send("isready")
        self.wait_for("readyok")

    def set_position(self, fen: str = STARTPOS, moves: list[str] | None = None) -> None:
        cmd = f"position fen {fen}"
        if moves:
            cmd += " moves " + " ".join(moves)
        self.send(cmd)

    def close(self) -> None:
        if self._proc.poll() is not None:
            return
        try:
            self.send("quit")
            self._proc.wait(timeout=5)
        except Exception:
            self._proc.kill()
            self._proc.wait(timeout=2)


def _parse_bestmove(line: str) -> BestMove:
    parts = line.split()
    if len(parts) < 2 or parts[1] == "(none)":
        raise AssertionError(f"Invalid bestmove line: {line}")
    ponder = None
    if len(parts) >= 4 and parts[2] == "ponder":
        ponder = parts[3]
    return BestMove(move=parts[1], ponder=ponder, raw=line)


def _run_test(name: str, fn) -> TestResult:
    t0 = time.monotonic()
    try:
        detail = fn() or "ok"
        return TestResult(name=name, passed=True, detail=detail, elapsed_s=time.monotonic() - t0)
    except Exception as e:
        return TestResult(
            name=name, passed=False, detail=f"{type(e).__name__}: {e}", elapsed_s=time.monotonic() - t0
        )


def test_no_early_exit_with_time(engine: UCIEngine) -> str:
    """Time limits must not end ponder; bestmove only after stop."""
    engine.new_game()
    engine.set_position(STARTPOS)
    # soft ≈ wtime/20 = 50ms; wait well past that and past a 200ms movetime-style budget
    engine.send("go ponder wtime 1000 btime 1000 winc 0 binc 0")
    engine.expect_no_bestmove(0.6)
    engine.send("stop")
    bm = engine.wait_for_bestmove(2.0)
    return f"stop → {bm.raw}"


def test_no_early_exit_with_movetime(engine: UCIEngine) -> str:
    """Prepared movetime must not end ponder early."""
    engine.new_game()
    engine.set_position(STARTPOS)
    engine.send("go ponder movetime 250")
    engine.expect_no_bestmove(0.6)
    engine.send("stop")
    bm = engine.wait_for_bestmove(2.0)
    return f"stop → {bm.raw}"


def test_no_early_exit_with_depth(engine: UCIEngine) -> str:
    """Depth limit must not emit bestmove while still pondering."""
    engine.new_game()
    engine.set_position(STARTPOS)
    engine.send("go ponder depth 3")
    # depth 3 finishes almost immediately; engine must keep pondering / wait
    engine.expect_no_bestmove(0.5)
    engine.send("stop")
    bm = engine.wait_for_bestmove(2.0)
    return f"stop → {bm.raw}"


def test_no_early_exit_on_mate(engine: UCIEngine) -> str:
    """UCI: do not exit ponder even if it's mate."""
    engine.new_game()
    engine.set_position(MATE_IN_ONE)
    engine.send("go ponder wtime 30000 btime 30000")
    engine.expect_no_bestmove(0.8)
    engine.send("stop")
    bm = engine.wait_for_bestmove(2.0)
    return f"stop → {bm.raw}"


def test_pondermiss_stop(engine: UCIEngine) -> str:
    """Opponent played something else: stop → bestmove promptly."""
    engine.new_game()
    engine.set_position(STARTPOS)
    engine.send("go ponder wtime 60000 btime 60000")
    time.sleep(0.15)
    engine.expect_no_bestmove(0.05)
    t0 = time.monotonic()
    engine.send("stop")
    bm = engine.wait_for_bestmove(2.0)
    latency = time.monotonic() - t0
    if latency > 1.0:
        raise AssertionError(f"Pondermiss bestmove too slow: {latency:.3f}s")
    return f"latency={latency:.3f}s → {bm.raw}"


def test_ponderhit_applies_movetime(engine: UCIEngine) -> str:
    """
    After ponderhit, prepared movetime must apply from the hit.

    Waits longer than movetime while pondering (must not bestmove), then hits.
    If the engine wrongly finished during ponder and only waited, bestmove would
    be immediate after ponderhit — that must fail.
    """
    movetime_s = 0.35
    engine.new_game()
    engine.set_position(STARTPOS)
    engine.send(f"go ponder movetime {int(movetime_s * 1000)}")
    engine.expect_no_bestmove(movetime_s + 0.35)
    t0 = time.monotonic()
    engine.send("ponderhit")
    bm = engine.wait_for_bestmove(movetime_s + 2.0)
    elapsed = time.monotonic() - t0
    # Allow some scheduling slack, but require a real post-hit search.
    min_s = movetime_s * 0.45
    max_s = movetime_s + 1.5
    if elapsed < min_s:
        raise AssertionError(
            f"bestmove {elapsed:.3f}s after ponderhit; expected ~{movetime_s:.3f}s "
            f"(search likely exited early during ponder)"
        )
    if elapsed > max_s:
        raise AssertionError(f"bestmove too slow after ponderhit: {elapsed:.3f}s")
    return f"post-hit {elapsed:.3f}s → {bm.raw}"


def test_ponderhit_then_stop(engine: UCIEngine) -> str:
    """Abort after ponderhit must still produce bestmove."""
    engine.new_game()
    engine.set_position(STARTPOS)
    engine.send("go ponder wtime 60000 btime 60000 winc 0 binc 0")
    time.sleep(0.2)
    engine.expect_no_bestmove(0.05)
    engine.send("ponderhit")
    time.sleep(0.05)
    t0 = time.monotonic()
    engine.send("stop")
    bm = engine.wait_for_bestmove(2.0)
    latency = time.monotonic() - t0
    if latency > 1.0:
        raise AssertionError(f"stop after ponderhit too slow: {latency:.3f}s")
    return f"latency={latency:.3f}s → {bm.raw}"


def test_normal_go_can_offer_ponder(engine: UCIEngine) -> str:
    """Non-ponder go may include a ponder token on bestmove."""
    engine.new_game()
    engine.set_position(STARTPOS)
    engine.send("go movetime 200")
    bm = engine.wait_for_bestmove(3.0)
    if bm.ponder is None:
        return f"{bm.raw} (no ponder token — allowed)"
    return bm.raw


def test_infinite_no_early_exit(engine: UCIEngine) -> str:
    """go infinite must not emit bestmove until stop."""
    engine.new_game()
    engine.set_position(STARTPOS)
    engine.send("go infinite")
    engine.expect_no_bestmove(0.6)
    t0 = time.monotonic()
    engine.send("stop")
    bm = engine.wait_for_bestmove(2.0)
    latency = time.monotonic() - t0
    if latency > 1.0:
        raise AssertionError(f"infinite stop bestmove too slow: {latency:.3f}s")
    return f"latency={latency:.3f}s → {bm.raw}"


def test_infinite_no_early_exit_on_mate(engine: UCIEngine) -> str:
    """
    go infinite must not exit even when the tree is tiny (mate-in-1).

    Without a wait-after-search, ID finishes MAX_PLIES almost instantly here and
    would emit bestmove without stop.
    """
    engine.new_game()
    engine.set_position(MATE_IN_ONE)
    engine.send("go infinite")
    engine.expect_no_bestmove(0.8)
    engine.send("stop")
    bm = engine.wait_for_bestmove(2.0)
    return f"stop → {bm.raw}"


def get_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--engine",
        type=Path,
        default=Path("../build/photon"),
        help="Path to the Photon binary (relative paths are from cwd, usually tools/)",
    )
    parser.add_argument(
        "-q",
        "--quiet",
        action="store_true",
        help="Only print the summary line",
    )
    return parser.parse_args()


def main() -> int:
    args = get_args()
    engine_path = args.engine.resolve()
    if not engine_path.is_file():
        print(f"Engine not found: {engine_path}", file=sys.stderr)
        return 2

    tests = [
        ("no_early_exit_with_time", test_no_early_exit_with_time),
        ("no_early_exit_with_movetime", test_no_early_exit_with_movetime),
        ("no_early_exit_with_depth", test_no_early_exit_with_depth),
        ("no_early_exit_on_mate", test_no_early_exit_on_mate),
        ("pondermiss_stop", test_pondermiss_stop),
        ("ponderhit_applies_movetime", test_ponderhit_applies_movetime),
        ("ponderhit_then_stop", test_ponderhit_then_stop),
        ("normal_go_can_offer_ponder", test_normal_go_can_offer_ponder),
        ("infinite_no_early_exit", test_infinite_no_early_exit),
        ("infinite_no_early_exit_on_mate", test_infinite_no_early_exit_on_mate),
    ]

    engine = UCIEngine(engine_path)
    results: list[TestResult] = []
    try:
        for name, fn in tests:
            # Fresh process state between tests via ucinewgame inside each test;
            # recreate engine if a prior test left it wedged.
            if engine._proc.poll() is not None:
                engine = UCIEngine(engine_path)
            result = _run_test(name, lambda f=fn: f(engine))
            results.append(result)
            if not args.quiet:
                status = "PASS" if result.passed else "FAIL"
                print(f"[{status}] {result.name} ({result.elapsed_s:.2f}s) — {result.detail}")
                sys.stdout.flush()
            if not result.passed:
                # Restart engine after a failure so later tests stay independent
                engine.close()
                engine = UCIEngine(engine_path)
    finally:
        engine.close()

    passed = sum(1 for r in results if r.passed)
    failed = len(results) - passed
    print(f"{passed}/{len(results)} passed" + (f", {failed} failed" if failed else ""))
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
