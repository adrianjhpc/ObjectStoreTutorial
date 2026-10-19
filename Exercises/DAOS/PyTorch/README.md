# pydaos.torch tutorial (SC26) — material

Working examples for "Feeding PyTorch from DAOS". Slides: `pydaos_torch_tutorial.pptx`.

## Run

```bash
cd Exercises/DAOS/PyTorch
source 00_env.sh                 # venv + DAOS paths; sets TUT_POOL / TUT_CONT
python 01_populate.py            # once: 10k .npz samples into /data/train
python 02_map_dataset.py         # Dataset + DataLoader
python 02_map_dataset.py --workers 2 --no-worker-init   # the fork failure
python 03_iterable_dataset.py    # IterableDataset
python 04_checkpoint.py          # Checkpoint write modes, object class
python 05_train_e2e.py           # train 2 epochs, checkpoint each
python 05_train_e2e.py --resume --epochs 3
```

Container (already created):

```bash
daos cont create mypool pytorch_tut --type POSIX --dir-oclass SX --file-oclass S1 --properties rd_fac:0
```

## Files

| File | Purpose |
|---|---|
| `00_env.sh` | environment: `my_venv` (torch 2.14 CPU), `PYTHONPATH` to DAOS site-packages, pool/cont names |
| `tut_common.py` | `decode_npz` transform, pool/cont/path constants |
| `01_populate.py` | synthetic dataset writer (uses `Checkpoint.writer`, 16 spawned processes) |
| `02_map_dataset.py` | map-style `Dataset`: scan, indexing, `transform_fn`, `__getitems__`, `DataLoader`, `worker_init_fn` |
| `03_iterable_dataset.py` | `IterableDataset`: lazy batching, worker sharding |
| `04_checkpoint.py` | `Checkpoint`: in-memory vs chunked writes, `reader(stream=)`, `class_name`, pitfalls |
| `05_train_e2e.py` | tiny CNN trained from DAOS, checkpoints to DAOS, `--resume` |
| `daos_overview.pptx` | DAOS introduction: concepts, API basics, POSIX on DAOS |
| `pydaos_torch_tutorial.pptx` | the PyTorch data-loader / checkpoint deck |
