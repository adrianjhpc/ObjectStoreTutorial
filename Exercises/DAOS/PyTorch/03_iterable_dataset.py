#!/usr/bin/env python3
"""
03_iterable_dataset.py - pydaos.torch.IterableDataset

Differences from the map-style Dataset:
  * no random access / no shuffling by the DataLoader: samples arrive in scan order
  * the dataset itself reads `batch_size` files per libdfs call (lazy batching)
  * worker_init(worker_id) both re-inits DAOS after fork AND shards the file list,
    so each DataLoader worker streams a disjoint slice of the namespace

Usage:  python 03_iterable_dataset.py [--workers 4] [--read-batch 32] [--batch-size 64]
"""
import argparse

import torch
from torch.utils.data import DataLoader

from pydaos.torch import IterableDataset
from tut_common import CONT, POOL, TRAIN_PATH, decode_npz


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--workers", type=int, default=4)
    ap.add_argument("--read-batch", type=int, default=32,
                    help="files read per libdfs batch_read inside the dataset")
    ap.add_argument("--batch-size", type=int, default=64, help="DataLoader batch size")
    args = ap.parse_args()

    with IterableDataset(pool=POOL, cont=CONT, path=TRAIN_PATH,
                         transform_fn=decode_npz,
                         batch_size=args.read_batch) as ds:
        print(f"{len(ds.objects)} objects discovered")

        # plain python iteration, no DataLoader: yields one sample at a time
        x, y = next(iter(ds))
        print(f"next(iter(ds)) -> tensor {tuple(x.shape)}, label {y}")

        loader = DataLoader(ds,
                            batch_size=args.batch_size,     # DataLoader's collate batch
                            num_workers=args.workers,
                            worker_init_fn=ds.worker_init)  # re-init DAOS + shard the file list
        nbatches = 0
        nsamples = 0
        labels = set()
        for x, y in loader:
            nbatches += 1
            nsamples += x.shape[0]
            labels.update(y.tolist())
        # nsamples == len(ds.objects) shows the workers covered disjoint, complete shards
        print(f"DataLoader(num_workers={args.workers}, dataset batch={args.read_batch}, "
              f"loader batch={args.batch_size}): {nbatches} batches, {nsamples} samples, "
              f"{len(labels)} distinct labels")


if __name__ == "__main__":
    torch.set_num_threads(4)
    main()
