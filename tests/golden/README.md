# Golden fixtures

> The Python reference implementation named throughout (`scripts/llama_launcher.py`,
> `strata_planner.py`, `moe_hot.py`, `export_fixtures.py`) moved out of this repository on
> 2026-11-10 to `D:\Projects\giga-strata-poc` (owner: the repo holds only the new app).
> Paths to `scripts/*.py` below refer to that folder.

Inputs (`fixtures/`) and expected outputs (`expected/`) for the C++ golden tests, captured from the real Python
reference (`scripts/strata_planner.py`, `scripts/moe_hot.py`, `scripts/llama_launcher.py`) with its I/O
intercepted. Spec: `docs/02-PORT_PLAN.md` sections 3 to 6. Nothing here was computed by re-implementing planner
logic: every expected value is what the reference returned, logged or wrote.

```
.venv\Scripts\python.exe scripts\export_fixtures.py                     # everything (about 40 s)
.venv\Scripts\python.exe scripts\export_fixtures.py --only scenarios     # one or more groups, comma-separated
.venv\Scripts\python.exe scripts\export_fixtures.py --only scenarios --ids t-pe-,pw-01   # re-run some scenarios
.venv\Scripts\python.exe scripts\export_fixtures.py --live [--out FILE]  # hardware snapshot only (section 9)
```

Groups, in run order: `synthetic`, `models`, `layout`, `usage`, `scenarios`, `small`, `help`. A group deletes and
rewrites only its own files (`fixtures/logs/` belongs to another tool and is never touched). The `models` and
`layout` groups read the real models on `N:\` (`scenarios`, `small` and `usage` read real layouts too, and the
`help` group runs the real `--help`). Re-running with the same inputs produces byte-identical files (checked:
two full runs, 520 files, identical sha1s).

Scratch space is `tests/golden/_work/` (gitignored). `tests/golden/.gitattributes` sets `* -text`: fixture bytes
feed sizes, hashes and exact text diffs, so git must never convert line endings. `tests/golden/.gitignore`
re-includes `fixtures/models/`, which the root `.gitignore` (`models/`) would otherwise drop.

## 1. Conventions

- JSON files are UTF-8, LF, `indent=1`, `ensure_ascii=False`, with a trailing newline. Key order is meaningful
  only where noted. Integers are exact (sizes reach 2^37; parse them as `qint64`). Floats are Python `repr`
  (shortest round-trip), so parse them as `double` and compare with `==`.
- Paths are literal Windows strings, exactly as Python saw them. Never normalize them before comparing.
- Relative paths (`moe_usage\...`, `synth\...`, `synth_models\...`, `models`) are relative to the scenario
  working directory. Python ran with cwd = `tests/golden/_work/run`. The C++ fakes must treat these strings as
  given (string keys); only `os.path.abspath` dedup keys inside the draft search ever resolve them.
- `null` means "not produced" (for example `argv` when the run failed before launching).

## 2. Models: `fixtures/models/`

One pair of files per GGUF, plus `index.json`.

`<id>.gguf.z` is Qt `qCompress` format: a 4-byte big-endian uncompressed length, then a zlib stream (level 9).
`qUncompress()` reads it directly.

- `stored: "header"` (real models): bytes `[0, data_start)` of the file. `data_start` is exactly what
  `_read_shard` computes: the position after the last tensor info, rounded up to `general.alignment` (default 32).
  Each header was checked at export: parsing it alone, with the recorded file size, gives the same metadata and
  tensor sizes as the real file.
- `stored: "whole"` (synthetic): the entire file.

`<id>.json`:

```json
{"original_path": "N:\\...\\GLM-5.3-Flash-GSQ-RCO-3.0bit-q4kattn.gguf", "kind": "real" | "synthetic",
 "size": 113585695168, "mtime": 1793757055.6890087, "mtime_ns": 1793757055689008600,
 "stored": "header" | "whole", "stored_bytes": 9516480}
```

- `size` is the real file size. H14: the last tensor runs to `size`, so the reader needs it.
- `mtime` is Python's float `st_mtime`. CPython derives it as `(ns // 1e9) + (ns % 1e9) * 1e-9` (floor-divided
  seconds plus nanoseconds times `1e-9` as a double). That is not `ns / 1e9`: about 25% of real files differ, and
  `int(st_mtime)` (used by `model_key`) can round up to the next second. Example: `hot_cache_key` case `hotkey-1`,
  `...055999999900` ns gives `...056`. Use `mtime` as given, or reproduce that formula.
- Synthetic paths are relative (`synth\syn-dense.gguf`, `synth_models\qwen-mtp-b.gguf`), and their mtime is fixed
  at `1767225600000000000` ns.

`index.json` is `{"models": [{"id", "original_path", "kind", "size", "stored"}]}`, sorted by id. A
`ModelSource` fake maps `original_path` to bytes and size. A read past `stored_bytes` of a `header` fixture is a
test bug (the reference never reads tensor data), so fail loudly on it.

Real models (12): GLM 5.3 3.0bit single file, its MTP sidecar, its `mmproj-model-bf16`, the UD-IQ3_XXS 4-shard
set (all 4), Qwen 3.8 IQ3_S shards 1 and 2, `N:\strata\model\mtp-q2_0.gguf`, `N:\strata\model\mtp\mtp-q2_0.gguf`,
and the Qwen mmproj. These are every file the draft and projector searches open for the scenario models and
search dirs; the exporter discovers them by running the real searches and would add any new one.

Synthetic models (25, from `third_party/llama.cpp-upstream/gguf-py`):

| id | covers |
|---|---|
| syn-dense | no `exps` tensors (dense warning), `vocab_size` without a tokens array |
| syn-tied | no `output.weight` (tied embeddings), 100-entry tokens array (kept), 100-entry `merges` (skipped, >64) |
| syn-kvlist | `head_count` as a list (max taken), `head_count_kv` as a per-layer list with zeros |
| syn-interval | `full_attention_interval` 3, explicit key/value lengths |
| syn-mla | `kv_lora_rank` without `rope.dimension_count` (default 64), `nextn_predict_layers` 1, indexer |
| syn-mla-rope | `kv_lora_rank` 40 with `rope.dimension_count` 0 (present, so 0 is used, not 64) |
| syn-v2 / syn-v1 | GGUF version 2 (accepted) / version 1 (error) |
| syn-bool | bool scalar, bool array (type 7 per-element path), `general.alignment` 64 |
| syn-strarr | 70 tokens, 65-entry string array (skipped), 64-entry string array (decoded), nested array, float array, UTF-8 string |
| syn-noblock | no `block_count` (derived from tensor names), `expert_count` 0 while `exps` tensors exist |
| syn-glm4-eog / -eot / -eos-ok | `_eog_overrides`: two fixes / only eos (eot present) / none (eos already `<|user|>`) |
| syn-split-1/2 | complete 2-shard set |
| syn-miss-1/3 | shards 1 and 3 of 3 (shard 2 missing) |
| syn-badmagic, syn-badtype | bad magic; value type 13 |
| syn-mmproj-1024/4096, syn-glmud-mtp, syn-qwen-mtp-a-wrong, syn-qwen-mtp-b | candidates in the `synth_models` search dir |

## 3. Layout: `expected/layout/<id>.json`

One per model id (37):

```json
{"path": "<original_path>", "layout": <dataclasses.asdict(read_layout(path))> | null,
 "error": null | {"type": "ValueError" | "FileNotFoundError", "message": "..."}}
```

`layout` holds every `ModelLayout` field under its Python name (`arch`, `n_layer`, `n_skipped`, `n_expert`,
`n_expert_used`, `total_bytes`, `dense`, `experts`, `output_bytes`, `cpu_global_bytes`, `kv_per_token`, `n_embd`,
`n_vocab`, `eog_overrides`, `has_indexer`). For a later shard of a split set (for example `glm53-ud-iq3xxs-3`)
the path is that shard's, and the layout is that of the whole set, as `read_layout` gives it.

## 4. Usage profiles: `fixtures/usage/`

`<id>.usage.json` is in the llama-server usage format read by `moe_hot.read_usage`, serialized compactly with
no trailing newline. Its exact bytes matter: their size enters the hot-cache key.

```json
{"format":"llama-moe-usage","arch":"glm5-next","n_expert":288,"n_layer":45,
 "tokens":{"decode":150000,"batch":600000},"counts":{"decode":[[...],...],"batch":[[...],...]}}
```

- `n_layer` is `n_layer + n_skipped`. Dense and skipped layers have `[]`.
- Counts are seeded multinomial draws over Zipf weights (numpy `default_rng`), so the output is deterministic.

`index.json` is `{"profiles": [{"id", "model", "variant", "file", "size", "mtime_ns"}]}`. `model` is one of
`glm3bit`, `glmud`, `qwen`, or null. The variants are:

- `both`: decode and batch counts.
- `batchonly`: decode 0, with all-zero decode lists.
- `few`: 12000 decode tokens, under `USAGE_MIN_TOKENS`.
- `wrongarch`: another model's arch and dimensions. For `glmud` it is the 3.0bit's 45 layers: same arch, but
  the layer count differs.
- `ties` (3.0bit): counts quantized to multiples of 4000, plus one all-zero layer (H4/H5 near-ties).
- `badformat`: wrong `format` field.
- `corrupt`: truncated JSON.

When a scenario uses a profile, it is placed with `mtime_ns` = `1767312000123456700`.

## 5. Scenarios: `fixtures/scenarios/<id>.json` and `expected/scenarios/<id>.json`

182 runs through the launcher's Run path: `LlamaStrataLauncher.read_settings()`, then `run_process(s)`.
`fixtures/scenarios/index.json` lists the ids in run order (`pw-*` first, then `t-*`).

- `pw-001`..`pw-087`: greedy all-pairs coverage (seeded) over 15 factors (model, GPU set, ctx, margin, ubatch,
  RAM x mmap, per-expert, usage file state, recording, MTP + search dir, vision, flag set, extra args, flash
  attention, stale hot files). Each row's levels are in `factors`.
- `t-*`: targeted cases. Each one says what it covers in `description`: per-expert cold build, reuse that still
  fits, reuse that no longer fits, 200K, ties, wrong-arch profiles, `.bak` moves, env presets, every MTP and
  projector path, GPU id errors, `read_settings` errors, the vendor DLL PATH, process exit codes, and the
  synthetic models.

### 5.1 Fixture (the world)

| key | meaning |
|---|---|
| `id`, `description`, `factors` | labels (`factors` is null for `t-*`) |
| `settings` | the 17 Tk variables exactly as `load_settings` would set them: strings for text fields, booleans for toggles (H12). Keys: `binary model gpus ctx margin ubatch host port extra mmap fa auto_server mtp vision mmproj record_usage per_expert` |
| `resolved_binary` | what the patched `resolve_server` returned (no trial load, no probing) |
| `flags` | what the patched `detect_flags` returned (sorted; a set in Python, so order is irrelevant). Variants: full set of 10, each one missing, or empty (a failed `--help` scrape) |
| `gpu_snapshot` | the patched `strata_planner.query_gpus()`: `[{index, name, free_mib, total_mib}]` in nvidia-smi order |
| `free_ram_bytes` | the patched `strata_planner.free_ram_bytes()`; `null` = unknown |
| `models_dir` | `llama_launcher.MODELS_DIR` (the extra draft/projector search dir): `models` (relative, absent), `N:\strata\model`, `N:\PolyStrata-data\models`, or `synth_models` |
| `usage_dir`, `hot_dir` | `moe_usage` and `moe_usage\hot` (relative, so hot-cache hashes do not depend on the checkout path) |
| `env_base` | the environment the run started from. `PATH` is fixed; every other key here is a preset (for example `LLAMA_MOE_USAGE_FLUSH`). `CUDA_DEVICE_ORDER`, `CUDA_VISIBLE_DEVICES`, `LLAMA_MOE_USAGE_OUT`, `LLAMA_MOE_USAGE_FLUSH` and `GGML_CUDA_GRAPH_CHUNK_SIZE` are absent unless listed |
| `virtual_dirs` | `{dir: [entries in listdir order]}` answered by `os.path.isdir`/`os.listdir` for vendor folders (`_vendor_dll_dirs`); everything else is real |
| `process` | the fake `Popen`: `stdout` lines yielded verbatim (with their `\n`), then `wait()` returns `exit_code` |
| `files_before` | files present under the cwd before the run, in creation order. Each is `{path, usage_profile, mtime_ns}` (bytes of `fixtures/usage/<usage_profile>.usage.json`, mtime set) or `{path, text}` (UTF-8 text, mtime irrelevant). Paths are distinct case-insensitively (NTFS) |
| `pre_run` | present on `t-pe-reuse-*`: the hot file came from a first run of the same settings on `pre_run.gpu_snapshot`. Its file is already materialized in `files_before` (`from_pre_run: true`), so the C++ test does not need to run it |
| `model_fs` | every model-side filesystem fact `strata_planner` asked for (union of the pre-run and the main run), for the `ModelSource` fake: `exists {path: bool}`, `isdir {path: bool}`, `getsize {path: int}`, `globs [{pattern, recursive, results: [[path, size], ...]}]` (results in Python's order), `read_shard [paths]` (every file `_read_shard` opened, in first-call order). Paths under `moe_usage` are left out (see `files_before`) |

Recursive `**` globs skip dot-directories (Python `glob` default; `N:\strata\model\.cache` exists). Draft
candidates larger than 8 GiB are skipped by size without opening them, which is why `globs` results carry sizes.

### 5.2 Expected

| key | meaning |
|---|---|
| `settings_parsed` | the dict `read_settings()` returned (`gpu_ids` list, ints, `"auto"` or int `ubatch`, tokenized `extra`), or null if it refused |
| `plan_call` | the arguments `strata_planner.make_plan` was called with, by parameter name (`model_path, gpu_ids, ctx, margin_mib, allow_ssd, ubatch, reserve_last_bytes` plus `moe_hot_*` keywords when per-expert is used); null if never called |
| `args`, `report`, `warnings`, `use_mmap`, `ubatch`, `arch` | the returned `Plan` fields (null if `make_plan` raised or was not reached). `report` and `warnings` are compared verbatim (H8) |
| `hot_path` | `moe_hot_path` relative to `hot_dir` (the file name), or null |
| `hot_file_text` | that file's content after the run (exact bytes as text, `\n` endings), or null if absent |
| `argv` | the command list passed to `Popen` |
| `env_delta` | keys of the `Popen` env that were added or changed vs `env_base`, sorted by key |
| `env_removed` | keys of `env_base` missing from the `Popen` env (always `[]`) |
| `log` | every `self.log(message)` argument, in order and verbatim, one list entry per call (most end in `\n`; `Executing command` ends in `\n\n`; server lines are passed through unchanged) |
| `error` | the message of the run's failure, or null. For `run_process` it is the text after `[CRITICAL RUNTIME ERROR]: ` (the log line itself is also in `log`); for `read_settings` it is the messagebox text |
| `error_stage` | `read_settings`, `run_process`, or null |
| `error_type` | the Python exception class of the first launcher step or `make_plan` that raised (`ValueError`, `FileNotFoundError`, `RuntimeError`), or null |
| `messagebox` | `[[title, text], ...]` from `messagebox.showerror` |
| `files_after` | sorted file names in `usage_dir` and in `hot_dir` after the run (null if the directory does not exist): `.bak` moves, stale-file pruning, written hot files |
| `trace` | diagnostics for unit-level goldens (section 5.3) |

Comparison: everything is exact, except `error` when `error_type` is an `OSError` raised by the OS itself
(`t-syn-nofile`: `[Errno 2] No such file or directory: 'synth\\does-not-exist.gguf'`). That text is CPython's,
not the launcher's. Compare stage and type there, and decide the C++ sentence separately.

### 5.3 `trace`

Calls of the planner internals, in call order. Hot maps are digested to keep the files small: `{"k": {layer: count
of hot experts}, "sha1": sha1 of json.dumps({str(layer): sorted experts} in ascending layer order,
separators=(",", ":"))}`. For example, `{3: [1, 2], 10: [7]}` hashes the text `{"3":[1,2],"10":[7]}`.

| fn | args | result |
|---|---|---|
| `_place` | `gpus` (indices), `margin_mib`, `ubatch`, `reserve_last_bytes`, `ctx` | `counts`, `ranges [[start, stop)]`, `gpu_experts`, `cpu_layers` (sorted), `avail`, `gpu_exp_bytes`, `lines`, `warnings` |
| `_reuse_hot` | `cost` | hot digest or null |
| `build_hot` | `ranges`, `avail` | hot digest |
| `build_hot_cost` | `ranges`, `avail`, `bw_gib_s`, `t_ovh_ms`, `allow_mixed`, `unit` | `{hot: digest, ms}` |
| `predict_ms` | `hot` digest, `cpu_layers`, `bw_gib_s`, `t_ovh_ms` | float |
| `hit_rate` | `hot` digest | float |

The counts these functions saw are `moe_hot.read_counts(plan_call.moe_hot_counts)`, which is the profile's
`read_usage` result.

### 5.4 How the reference was driven (reproduce the same world)

- `LlamaStrataLauncher` is created once on a `tk.Tk()` with `root.withdraw()`. `root.after` is not patched. After
  `run_process` returns, the real Tk loop is pumped with `root.update()` (3 times), so every queued
  `append_log` and `reset_ui` runs on the real path. `append_log` is replaced on the instance to record messages.
- Patched for every scenario: `strata_planner.query_gpus`, `strata_planner.free_ram_bytes`,
  `LlamaStrataLauncher.resolve_server`, `llama_launcher.detect_flags`, `llama_launcher.MODELS_DIR`,
  `subprocess.Popen` (records argv and env, never launches), and `messagebox.showerror`. `SETTINGS_PATH`,
  `USAGE_DIR` and `HOT_DIR` point into `_work`.
- `strata_planner._read_shard` is memoized per (abspath, size, mtime_ns), because it is a pure function of the
  file. A file that cannot be stat'ed goes straight to the real function, so its errors are unchanged.
  `strata_planner.os.path` and `strata_planner.glob` are recording proxies. All of this only observes; nothing
  changes a result.

## 6. Small fixtures: `fixtures/small/<name>.json` and `expected/small/<name>.json`

Both files are `{"cases": [...]}`, matched by `id` (same order).

| name | fixture case | expected case |
|---|---|---|
| `shlex` | `extra`: the Extra args text | `tokens` (`read_settings()["extra"]`, that is `shlex.split(posix=False)` then `strip('"')`), or null with `error` = the messagebox text (H11) |
| `hot_cache_key` | `model [path, size, mtime_ns]`, `files [[path, size, mtime_ns]]`, `gpu_ids`, `ctx`, `margin`, `ubatch`, `reserve`, `hot_dir_before` (names present first) | `key_json` (the exact string `hot_cache_path` hashed), `sha1` (full), `path` (`moe_usage\hot\<name>`), `name`, `hot_dir_after` (sorted, after pruning), `model_key`. `os.stat` was faked from the case values; `st_mtime` uses the CPython formula from section 2 |
| `find_servers` | a fake tree under `root` = `C:\Fixture`: `files` (every file), `home` (`~`), `upstream_server`, `built_server` (the patched module constants), `path_dirs` (`PATH`), `glob_calls` (the backends glob, results in NTFS order) | `servers` (normcased; the root appears as `c:\fixture`) |
| `model_stem` | `path` | `stem` |
| `version_key` | `path` | `key` (list; `[]` = Python `()`) |
| `list2cmdline` | `argv` | `cmdline` (`subprocess.list2cmdline`, used in the `Executing command` log line) |
| `vendor_dll_dirs` | `binary`, `dirs` (virtual `{dir: listdir order}`), `env_base_path`, `gpu_ids` | `dll_dirs`, `server_env_delta` (`server_env(binary, gpu_ids)` vs a base env whose PATH is `env_base_path`) |
| `nvidia_smi` | `stdout` (the CSV text) | `argv` (the command `query_gpus` runs), `gpus` (list) or null with `error {type, message}` |
| `usage_io` | `profile` (an id in `fixtures/usage`), `path` (relative path it was read from) | `usage_tokens [decode, batch]`, `read_usage` (bucket None), `read_usage_decode`, `read_usage_batch`, `read_counts`; each `{value, error}`. Count values are `{layers, sha1, sums}`, where `sha1` is over `json.dumps({str(layer): [floats]}, separators=(",", ":"))` (floats print as `123.0`) |
| `hot_file` | read cases `{path, n_expert, text}`; write cases `{write: {n_expert, hot}}` | read: `hot` (`{layer: experts}`) or `error`; write: `text` (exact `write_hot` output) |

## 7. `--help` captures: `fixtures/help/`, `expected/help/`

`fixtures/help/<id>.txt` is `stdout + stderr` of `<binary> --help`, exactly the text `detect_flags` scans
(UTF-8, written without newline translation). `index.json` is `{"builds": [{id, binary, file, gpu_ids,
exit_code}]}`. The env was `server_env(binary, [])`, so CUDA saw no GPUs. `expected/help/<id>.json` is
`{"id", "flags": [sorted detect_flags result]}`.

| id | flags missing from the full 10 |
|---|---|
| upstream | none |
| glm5 | `--load-mode`, `--lazy-mode`, `draft-mtp`, `--moe-hot`, `--mmproj-device` |
| lmstudio-cuda12 (2.67.0) | `--moe-hot` |

## 8. Reference behavior the C++ must reproduce (found while exporting)

1. `int(st_mtime)` is a truncated double built as `sec + nsec * 1e-9`. It can round up to the next second
   (section 2), and negative times truncate toward zero (`-1.5` gives `-1`).
2. `mtp-q2_0.gguf` (both copies) has arch `qwen4exp-mtp`, so it never matches Qwen as a draft. The UD set has
   `nextn_predict_layers` 1 (block_count 46), so it needs a 47-block draft, and the 3.0bit sidecar (46) does not
   match it. Only synthetic drafts exercise the Qwen spec args and the UD draft.
3. An empty flag set (failed `--help`) is inconsistent. `per_expert_args` and `find_mmproj` test
   `flags is not None`, so they log "no --moe-hot" and "no --mmproj-device". `set_usage_env` and the
   `--override-tensor` warning test truthiness, so they stay silent. `--alias`, `--fit` and `-lm` are dropped,
   and `--no-mmap` is used when the plan does not mmap (`t-pe-noflags`).
4. An unbalanced quote in Extra args fails `read_settings` with the misleading "GPU IDs, context size, margin and
   micro-batch must be whole numbers" message.
5. `shlex.split(posix=False)` quirks:
   - Quotes in the middle of a word do not group: `--x="a b"` gives `['--x="a', 'b']`.
   - Single quotes are kept: `'single quoted'` stays one token.
   - A trailing unbalanced quote is accepted (`unbalanced"` gives `unbalanced`), but a leading one raises.
   - Backslash is not an escape.
   - `#` is not a comment.
6. `_vendor_dll_dirs` returns files as well as directories from `vendor\`. A CUDA 13 build has no `cuda12` in its
   name, so it gets kind `cuda`: the CUDA 11 and generic vendor folders.
7. `_version_key` uses Python `int()`, which accepts surrounding spaces, `_` separators, a `+` sign and Unicode
   digits (`١٢` gives 12). `2..1` and `2.67.0-rc1` give `()`, which sorts below every version.
8. `find_servers` includes CUDA 13 builds (after the CUDA 12 ones). Versions that tie keep glob order (NTFS name
   order), and duplicates are removed by normcase.
9. Hot-file pruning globs case-insensitively but `re.fullmatch`es case-sensitively, so stale files whose stem or
   hex differs only in case survive (`t-pe-prune-case`, `hotkey-1`).
10. A profile whose layers all have the wrong expert count still writes an empty hot file and passes `--moe-hot`.
    The report reads "0 layers ... hit rate 0%" (`t-pe-wrongarch-qwen`).
11. `syn-noblock`: `expert_count` 0 while `exps` tensors exist gives `n_expert` 0 with non-zero `experts` bytes.
    Recording then says "not a MoE model".
12. Tied embeddings count `token_embd` in both `cpu_global_bytes` and `output_bytes`. Every tensor whose name
    starts with `output` (including `output_norm`) counts as output bytes.
13. `g("rope.dimension_count", 64)` only defaults when the key is missing: the real GLM has 0, so its MLA KV is
    `(512 + 0) * 2`. Contrast the `or` fallbacks of H6.
14. `query_gpus`: a blank line between rows raises (leading and trailing blank lines are stripped), and `int()`
    accepts `00` and `+5`.
15. `read_hot`: the header line is compared after `strip()`, and a duplicate layer line wins (last one).
16. GLM 3.0bit never picks `-ub 512` at these budgets; Qwen on two GPUs and the UD set on three usually do.

## 9. `--live`

`--live` writes `{taken_at, nvidia_smi_csv, gpus: [{index, name, free_mib, total_mib}], free_ram_bytes}` to
`tests/golden/_work/live-snapshot.json` (or `--out`). It runs the real read-only `nvidia-smi` query and nothing
else. It is used for the section 8 live parity check of doc 02 and is never committed.

## 10. Not covered

- `probe_server` and `resolve_server` (trial loads and the server cache) are patched out, by design: they would
  launch `llama-server`.
- The `analyze()` path (doc 02 D2: the golden target is `run_process`).
- imatrix GGUF counts in `read_counts` (not ported).
- Real recorded usage (only synthetic profiles exist so far; doc 02 section 10 risk).
- Non-ASCII model paths in full runs (only in `hot_cache_key`).
- The `--help` env used `gpu_ids []`, while Run uses the real ids. The scraped flags do not depend on it.
