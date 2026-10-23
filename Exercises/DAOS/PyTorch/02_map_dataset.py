#!/usr/bin/env python3
"""
02_map_dataset.py - map-style pydaos.torch.Dataset

Demonstrates:
  1. constructing a Dataset  (namespace scan happens here)
  2. indexing: raw bytes vs. a transform_fn that decodes samples into tensors
  3. feeding a torch DataLoader with worker processes
     (worker_init_fn=dataset.worker_init is REQUIRED with num_workers > 0)

Usage:  python 02_map_dataset.py [--workers 4] [--batch-size 64] [--no-worker-init]
"""
import argparse

import torch
from torch.utils.data import DataLoader

from pydaos.torch import Dataset
from tut_common import CONT, POOL, TRAIN_PATH, decode_npz


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--workers", type=int, default=4)
    ap.add_argument("--batch-size", type=int, default=64)
    ap.add_argument("--no-worker-init", action="store_true",
                    help="skip worker_init_fn to show what happens after fork()")
    args = ap.parse_args()

    # --- 1. construct: scans /data/train recursively, in parallel -------------------------
    raw = Dataset(pool=POOL, cont=CONT, path=TRAIN_PATH)
    print(f"len(dataset) = {len(raw)}")
    path, size = raw.objects[0]
    print(f"objects[0] = ({path}, {size} bytes)")

    # --- 2a. default transform_fn returns the raw file content ---------------------------
    sample = raw[0]
    print(f"raw[0] -> {type(sample).__name__} of {len(sample)} bytes")
    raw.close()

    # --- 2b. transform_fn decodes bytes -> (tensor, label) --------------------------------
    with Dataset(pool=POOL, cont=CONT, path=TRAIN_PATH, transform_fn=decode_npz) as ds:
        x, y = ds[0]
        print(f"ds[0] -> tensor {tuple(x.shape)} {x.dtype}, label {y}")

        # __getitems__: one call reads a whole batch of files in parallel through libdfs
        batch = ds.__getitems__([0, 1, 2, 3])
        print(f"ds.__getitems__([0..3]) -> {len(batch)} samples, labels {[b[1] for b in batch]}")

        # --- 3. DataLoader ------------------------------------------------------------------
        loader = DataLoader(ds,
                            batch_size=args.batch_size,
                            shuffle=True,
                            num_workers=args.workers,
                            # re-initializes the DAOS/DFS handles in each forked worker
                            worker_init_fn=None if args.no_worker_init else ds.worker_init)
        nbatches = 0
        nsamples = 0
        shape = None
        for x, y in loader:
            shape = shape or tuple(x.shape)
            nbatches += 1
            nsamples += x.shape[0]
        print(f"DataLoader(num_workers={args.workers}): {nbatches} batches, {nsamples} samples, "
              f"batch tensor {shape}")


if __name__ == "__main__":
    torch.set_num_threads(4)
    main()
