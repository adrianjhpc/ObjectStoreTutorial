#!/usr/bin/env python3
"""
05_train_e2e.py - end to end: train from a DAOS dataset, checkpoint to DAOS, resume.

  python 05_train_e2e.py                 # train epochs 1..2, checkpoint after each
  python 05_train_e2e.py --resume        # load latest checkpoint from DAOS, continue to --epochs

Everything the training loop touches on storage goes through pydaos.torch:
  samples    : Dataset + DataLoader(num_workers, worker_init_fn=ds.worker_init)
  checkpoints: Checkpoint.writer()/reader() with torch.save()/torch.load()
"""
import argparse

import torch
import torch.nn as nn
import torch.nn.functional as F
from torch.utils.data import DataLoader

from pydaos.torch import Checkpoint, Dataset
from tut_common import CKPT_PATH, CONT, POOL, TRAIN_PATH, decode_npz

RUN = "e2e"


class TinyCNN(nn.Module):
    def __init__(self, nclasses=10):
        super().__init__()
        self.conv = nn.Conv2d(3, 8, 3, padding=1)
        self.fc = nn.Linear(8 * 8 * 8, nclasses)

    def forward(self, x):
        x = F.max_pool2d(F.relu(self.conv(x)), 4)      # 32x32 -> 8x8
        return self.fc(x.flatten(1))


def save_ckpt(chkp, model, opt, epoch):
    name = f"{RUN}/epoch_{epoch:03d}.pt"
    with chkp.writer(name) as w:
        torch.save({"epoch": epoch,
                    "model": model.state_dict(),
                    "optimizer": opt.state_dict()}, w)
    # 'latest' is rewritten each epoch; same size every time so no-truncate is harmless here
    with chkp.writer(f"{RUN}/latest.pt") as w:
        torch.save({"epoch": epoch, "file": name}, w)
    print(f"  checkpoint -> {CKPT_PATH}/{name}")


def load_latest(chkp, model, opt):
    with chkp.reader(f"{RUN}/latest.pt") as r:
        latest = torch.load(r)
    with chkp.reader(latest["file"]) as r:
        state = torch.load(r)
    model.load_state_dict(state["model"])
    opt.load_state_dict(state["optimizer"])
    print(f"  resumed from {CKPT_PATH}/{latest['file']} (epoch {state['epoch']})")
    return state["epoch"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--epochs", type=int, default=2)
    ap.add_argument("--resume", action="store_true")
    ap.add_argument("--workers", type=int, default=8)
    ap.add_argument("--batch-size", type=int, default=64)
    args = ap.parse_args()

    torch.manual_seed(0)
    model = TinyCNN()
    opt = torch.optim.SGD(model.parameters(), lr=0.05, momentum=0.9)
    start_epoch = 0

    # Open the Checkpoint (read-write) BEFORE the Dataset (read-only): libdfs caches the
    # pool/container handles per process, keyed by label, and the first open's access mode
    # wins. Dataset first -> RO handles cached -> checkpoint writes fail with EPERM.
    with Checkpoint(POOL, CONT, prefix=CKPT_PATH, class_name="SX") as chkp, \
         Dataset(pool=POOL, cont=CONT, path=TRAIN_PATH, transform_fn=decode_npz) as ds:

        if args.resume:
            start_epoch = load_latest(chkp, model, opt)

        loader = DataLoader(ds, batch_size=args.batch_size, shuffle=True,
                            num_workers=args.workers, worker_init_fn=ds.worker_init)

        for epoch in range(start_epoch + 1, args.epochs + 1):
            total, correct, loss_sum = 0, 0, 0.0
            for x, y in loader:
                opt.zero_grad()
                out = model(x)
                loss = F.cross_entropy(out, y)
                loss.backward()
                opt.step()
                loss_sum += loss.item() * y.numel()
                correct += (out.argmax(1) == y).sum().item()
                total += y.numel()
            print(f"epoch {epoch}: loss {loss_sum / total:.3f}  acc {correct / total:.3f}  "
                  f"({total} samples)")
            save_ckpt(chkp, model, opt, epoch)


if __name__ == "__main__":
    torch.set_num_threads(8)
    main()
