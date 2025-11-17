# Author: Vatsal Sanjay
# vatsal.sanjay@comphy-lab.org
# CoMPhy Lab — Durham University
# Last updated: Nov 17, 2025

"""
Radial footprint extractor built on top of the getFootPrint Basilisk helper.

The compiled helper already implements the heavy lifting (PLIC interface search).
This Python wrapper coordinates multiple cutoff radii, loops through all
snapshots inside ``caseToProcess``, and writes sorted CSV files of
``time,rf`` columns. By default the script evaluates five windows
[1e-3, 2.5e-3, 5e-3, 1e-2, 5e-2] but the list can be overridden via CLI.

Typical usage from the repository root:

    python3 postProcess/getFootPrint.py --caseToProcess ../simulationCases/1000/results

Each cutoff produces ``rFootvsTime_<cutoff>.csv`` in the case directory.
"""

from __future__ import annotations

import argparse
import multiprocessing as mp
import os
import subprocess as sp
from dataclasses import dataclass
from functools import partial
from typing import Iterable, List, Optional, Sequence, Tuple

import pandas as pd

DEFAULT_CUTOFFS = (1e-3, 2.5e-3, 5e-3, 1e-2, 5e-2)
SNAPSHOT_SUBDIR = "intermediate"


@dataclass(frozen=True)
class SnapshotTask:
    """Metadata for a single restart snapshot."""

    index: int
    time: float
    path: str


@dataclass(frozen=True)
class RuntimeConfig:
    """CLI options gathered into a single structure."""

    cpus: int
    n_snapshots: int
    tsnap: float
    case_dir: str
    binary: str

    def snapshots(self) -> List[SnapshotTask]:
        """Pre-compute snapshot paths for downstream multiprocessing workers."""
        tasks: List[SnapshotTask] = []
        for idx in range(self.n_snapshots):
            time = self.tsnap * idx
            path = os.path.join(
                self.case_dir, SNAPSHOT_SUBDIR, f"snapshot-{time:.4f}"
            )
            tasks.append(SnapshotTask(index=idx, time=time, path=path))
        return tasks


def parse_arguments() -> Tuple[RuntimeConfig, Tuple[float, ...]]:
    """Translate CLI flags into runtime configuration and cutoff list."""
    parser = argparse.ArgumentParser(
        description="Extract radial footprint (rf) vs time for multiple cutoffs."
    )
    parser.add_argument(
        "--CPUs", type=int, default=4, help="Number of worker processes to spawn."
    )
    parser.add_argument(
        "--nGFS",
        type=int,
        default=4000,
        help="Number of snapshot files to check inside intermediate/.",
    )
    parser.add_argument(
        "--tsnap",
        type=float,
        default=0.01,
        help="Physical time interval between snapshots.",
    )
    parser.add_argument(
        "--caseToProcess",
        type=str,
        default="../simulationCases/1000/results",
        help="Path to the simulation results folder housing intermediate/.",
    )
    parser.add_argument(
        "--binary",
        type=str,
        default="./getFootPrint",
        help="Path to the compiled getFootPrint helper executable.",
    )
    parser.add_argument(
        "--xCutoffs",
        type=float,
        nargs="*",
        default=DEFAULT_CUTOFFS,
        help=(
            "Radial window(s) for footprint detection. "
            "Provide one or more floats (default: 1e-3 2.5e-3 5e-3 1e-2 5e-2)."
        ),
    )
    args = parser.parse_args()

    if args.CPUs < 1:
        parser.error("--CPUs must be >= 1")
    if args.nGFS < 1:
        parser.error("--nGFS must be >= 1")
    if args.tsnap <= 0.0:
        parser.error("--tsnap must be positive.")

    case_dir = os.path.abspath(args.caseToProcess)
    binary = os.path.abspath(args.binary)

    config = RuntimeConfig(
        cpus=args.CPUs,
        n_snapshots=args.nGFS,
        tsnap=args.tsnap,
        case_dir=case_dir,
        binary=binary,
    )

    if not os.path.isdir(case_dir):
        raise FileNotFoundError(
            f"Case directory '{case_dir}' not found. "
            "Pass --caseToProcess pointing to results/."
        )
    if not os.path.isfile(binary):
        raise FileNotFoundError(
            f"Helper binary '{binary}' not found. Compile getFootPrint.c first."
        )

    cutoffs = tuple(args.xCutoffs) if args.xCutoffs else DEFAULT_CUTOFFS
    return config, cutoffs


def run_helper(command: Sequence[str]) -> Sequence[str]:
    """Execute helper binary and return decoded stderr lines."""
    result = sp.run(
        command,
        stdout=sp.PIPE,
        stderr=sp.PIPE,
        check=False,
        text=True,
    )
    if result.returncode != 0:
        raise RuntimeError(
            f"Command {' '.join(command)} failed (exit {result.returncode}):\n"
            f"{result.stderr}"
        )
    return [line.strip() for line in result.stderr.splitlines() if line.strip()]


def parse_rf_line(line: str) -> Tuple[float, float]:
    """Split helper CSV line ('t,ymax') into floats."""
    chunks = line.split(",")
    if len(chunks) != 2:
        raise ValueError(f"Unexpected helper output: '{line}'")
    time_str, rf_str = chunks
    return float(time_str), float(rf_str)


def process_snapshot(
    task: SnapshotTask,
    *,
    binary: str,
    x_cutoff: float,
) -> Optional[Tuple[float, float]]:
    """Worker that invokes the helper for a single snapshot."""
    if not os.path.exists(task.path):
        print(f"Skipping missing snapshot: {task.path}")
        return None
    try:
        lines = run_helper(
            [binary, task.path, f"{x_cutoff:.10g}"]
        )
    except RuntimeError as err:
        print(err)
        return None

    if not lines:
        print(f"No output from helper for {task.path} @ cutoff {x_cutoff}")
        return None

    helper_time, rf_val = parse_rf_line(lines[-1])
    print(
        f"x_cutoff={x_cutoff:.4g} :: t={helper_time:.6f}, rf={rf_val:.6g}",
        flush=True,
    )
    return helper_time, rf_val


def evaluate_cutoff_series(
    tasks: Iterable[SnapshotTask],
    config: RuntimeConfig,
    cutoff: float,
) -> List[Tuple[float, float]]:
    """Compute rf(time) pairs for every snapshot at the provided cutoff."""
    worker = partial(process_snapshot, binary=config.binary, x_cutoff=cutoff)
    if config.cpus == 1:
        rows = [worker(task) for task in tasks]
    else:
        with mp.Pool(processes=config.cpus) as pool:
            rows = pool.map(worker, tasks)
    return [row for row in rows if row is not None]


def cutoff_label(value: float) -> str:
    """Format cutoff for filenames, keeping readable scientific notation."""
    if value < 1:
        return f"{value:.4f}"
    label = f"{value:.4f}"
    return label.rstrip("0").rstrip(".")


def write_series(
    rows: Sequence[Tuple[float, float]],
    *,
    cutoff: float,
    case_dir: str,
) -> str:
    """Persist time-rf series to CSV with deterministic ordering."""
    df = pd.DataFrame(rows, columns=["time", "rf"])
    df.sort_values("time", inplace=True)
    filename = f"rFootvsTime_{cutoff_label(cutoff)}.csv"
    output_path = os.path.join(case_dir, filename)
    df.to_csv(output_path, index=False)
    print(f"Wrote {len(df)} rows to {output_path}")
    return output_path


def main() -> None:
    config, cutoffs = parse_arguments()
    tasks = config.snapshots()

    for cutoff in cutoffs:
        print(f"\nProcessing cutoff r <= {cutoff} m")
        rows = evaluate_cutoff_series(tasks, config, cutoff)
        write_series(rows, cutoff=cutoff, case_dir=config.case_dir)


if __name__ == "__main__":
    main()
