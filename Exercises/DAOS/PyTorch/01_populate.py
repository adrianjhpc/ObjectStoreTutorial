#!/usr/bin/env python3
"""
01_populate.py - create a synthetic dataset inside the DAOS POSIX container.

Layout (ImageFolder-like):
    /data/train/class_<k>/sample_<i>.npz     x: float32[3,32,32]   y: int64 label

Each sample is a noisy version of a per-class pattern so a tiny model can learn it.
Files are written with the pydaos.torch Checkpoint writer (plain libdfs writes, no dfuse).
Run once before the tutorial:  python 01_populate.py
"""
import io
import os
import sys
from multiprocessing import get_context

import numpy as np

POOL = os.environ.get("TUT_POOL", "mypool")
CONT = os.environ.get("TUT_CONT", "pytorch_tut")
NSAMPLES = int(os.environ.get("TUT_NSAMPLES", "10000"))
NCLASSES = 10
PREFIX = "/data/train"
WRITERS = 16


def make_sample(rng, idx):
    label = idx % NCLASSES
    pattern = np.zeros((3, 32, 32), dtype=np.float32)
    pattern[label % 3, (label * 3) % 32:(label * 3) % 32 + 8, :] = 1.0
    x = pattern + rng.normal(0, 0.5, size=pattern.shape).astype(np.float32)
    buf = io.BytesIO()
    np.savez(buf, x=x, y=np.int64(label))
    return f"class_{label}/sample_{idx:06d}.npz", buf.getvalue()


def writer_proc(worker_id, indices):
    # Each worker owns its own DAOS connection (DaosClient is a per-process singleton).
    from pydaos.torch import Checkpoint

    rng = np.random.default_rng(worker_id)
    # transfer_chunk_size=0: buffer in memory, single dfs write on close()
    with Checkpoint(POOL, CONT, prefix=PREFIX, transfer_chunk_size=0) as chkp:
        for idx in indices:
            name, data = make_sample(rng, idx)
            with chkp.writer(name) as w:
                w.write(data)
    return len(indices)


def main():
    print(f"Populating {POOL}:{CONT}{PREFIX} with {NSAMPLES} samples, {NCLASSES} classes, "
          f"{WRITERS} writer processes")
    shards = [list(range(w, NSAMPLES, WRITERS)) for w in range(WRITERS)]

    # 'spawn' so that no DAOS state is inherited across fork from this process
    with get_context("spawn").Pool(WRITERS) as pool:
        done = pool.starmap(writer_proc, enumerate(shards))

    print(f"wrote {sum(done)} files under {PREFIX}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
