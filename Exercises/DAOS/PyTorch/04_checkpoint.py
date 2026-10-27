#!/usr/bin/env python3
"""
04_checkpoint.py - saving / loading PyTorch checkpoints on DAOS with pydaos.torch.Checkpoint

  chkp.writer(name)  -> file-like object for torch.save()
  chkp.reader(name)  -> BytesIO (or a user-provided stream) for torch.load()

Two write modes:
  transfer_chunk_size=0      buffer everything in memory, one dfs write on close()
  transfer_chunk_size=N      cut the stream into N-byte chunks, `workers` processes write
                             them in parallel; at most `chunks_limit` chunks are queued
                             (bounds memory: ~ chunks_limit * N bytes in flight)

class_name takes the names used by `daos cont create --file-oclass` ("SX", "S1", "EC_2P1GX"
...), NOT the C enum spelling: "OC_SX" is unknown and silently falls back to the container
default.

Pitfall: the default open_flags are O_CREAT|O_RDWR (no O_TRUNC) and in chunked mode every
chunk re-opens the file with those flags, so O_TRUNC cannot be used there. Overwriting an
existing checkpoint with a *smaller* one leaves the old tail in place and torch.load() fails.
Use unique file names (epoch/step in the name) or remove the old file first.

Usage:  python 04_checkpoint.py [--size-mb 256]
"""
import argparse
import time

import torch

from pydaos.torch import Checkpoint
from tut_common import CKPT_PATH, CONT, POOL

MiB = 2**20


def make_state(size_mb):
    """A fake model state_dict of roughly size_mb MiB (fp32)."""
    n = size_mb * MiB // 4
    return {"layer.weight": torch.arange(n, dtype=torch.float32), "epoch": 7}


def round_trip(chkp, name, state):
    with chkp.writer(name) as w:
        torch.save(state, w)
    with chkp.reader(name) as r:
        loaded = torch.load(r)
    assert loaded["epoch"] == state["epoch"]
    assert torch.equal(loaded["layer.weight"], state["layer.weight"])
    print(f"  {name}: saved and loaded back identically")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--size-mb", type=int, default=256)
    args = ap.parse_args()

    state = make_state(args.size_mb)
    print(f"state_dict of {args.size_mb} MiB")
    # per-run directory: see the O_TRUNC pitfall in the module docstring
    run = f"{CKPT_PATH}/run_{int(time.time())}"

    # --- minimal usage -------------------------------------------------------------------
    with Checkpoint(POOL, CONT, prefix=run) as chkp:
        # parent directories under prefix are created on demand (ensure_path=True)
        with chkp.writer("model/epoch_007.pt") as w:
            torch.save(state, w)
        with chkp.reader("model/epoch_007.pt") as r:
            print(f"loaded epoch = {torch.load(r)['epoch']}")

        # reader(stream=...) streams into any writable object, e.g. a local file,
        # for checkpoints that should not be materialised in RAM
        with open("/tmp/epoch_007.pt", "wb") as f:
            chkp.reader("model/epoch_007.pt", stream=f)
        print("streamed a copy to /tmp/epoch_007.pt")

    # --- write modes -----------------------------------------------------------------------
    print("in-memory buffer, single write on close():")
    with Checkpoint(POOL, CONT, prefix=run, transfer_chunk_size=0) as chkp:
        round_trip(chkp, "inmem.pt", state)

    print("chunked: 64 MiB chunks, up to 4 queued, 4 writer processes:")
    with Checkpoint(POOL, CONT, prefix=run,
                    transfer_chunk_size=64 * MiB, chunks_limit=4, workers=4) as chkp:
        round_trip(chkp, "chunked.pt", state)

    # --- per-file object class / chunk size -----------------------------------------------
    # The container default here is S1 (good for 12 KB samples). Large checkpoint files
    # should be striped across engines: pick the class per Checkpoint instance.
    print("chunked + striped object class (SX) and 4 MiB DFS chunk size for the file:")
    with Checkpoint(POOL, CONT, prefix=run,
                    class_name="SX", file_chunk_size=4 * MiB,
                    transfer_chunk_size=64 * MiB, chunks_limit=4, workers=4) as chkp:
        round_trip(chkp, "chunked_SX.pt", state)
    print(f"inspect with:  daos fs get-attr {POOL} {CONT} --dfs-path {run}/chunked_SX.pt")


if __name__ == "__main__":
    main()
