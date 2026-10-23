"""
tut_common.py - helpers shared by the tutorial scripts.
"""
import io
import os

import numpy as np
import torch

POOL = os.environ.get("TUT_POOL", "mypool")
CONT = os.environ.get("TUT_CONT", "pytorch_tut")
TRAIN_PATH = "/data/train"
CKPT_PATH = "/checkpoints"


def decode_npz(data):
    """transform_fn: raw bytes read from DFS -> (tensor[3,32,32], label).

    pydaos.torch hands the sample content to transform_fn as a bytearray; whatever
    it returns is what DataLoader collates into a batch.
    """
    npz = np.load(io.BytesIO(data))
    return torch.from_numpy(npz["x"]), int(npz["y"])
