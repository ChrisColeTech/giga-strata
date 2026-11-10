# 02 — Python to C++ Port Plan

> The Python reference implementation named throughout (`scripts/llama_launcher.py`,
> `strata_planner.py`, `moe_hot.py`, `export_fixtures.py`) moved out of this repository on
> 2026-11-10 to `D:\Projects\giga-strata-poc` (owner: the repo holds only the new app).
> Paths to `scripts/*.py` below refer to that folder.

How the engine in `scripts/` becomes `src/core` + `src/app` without changing behavior, and
in what order the work lands. Binding the same way 01 is: a phase is done only when its
exit gate passes. Update this document when a decision changes.

## 1. Principles

1. Parity by interception, never by re-reading. Expected outputs come from running the
   real Python code (`strata_planner.py`, `moe_hot.py`, `llama_launcher.py`) with its I/O
   intercepted, not from re-implementing it in a test script. The Python files stay
   unmodified (00 section 7); only `scripts/export_fixtures.py` is added.
2. Golden first. For every core module, its fixtures and expected JSON exist and its
   golden test is written (and failing) before the C++ implementation.
3. Pure core, injected world. Core never touches the filesystem, nvidia-smi or RAM
   directly in planning code paths; it takes them through interfaces (section 5), so
   goldens run offline with no GPUs and no 100 GB models (R31).
4. Visible every phase. Each engine phase also lights up the part of the real window
   that shows it, verified by a screenshot of the running app (01 section 7, item 6).
   The engine is not built headless and wired at the end.
5. Same observable results, better structure. Where the Python mixes side effects into
   planning (section 4), the C++ splits them, but the args, report lines, warnings and
   files written are identical.

## 2. Source inventory and mapping

| Python (file: function) | C++ (01 section 3 tree) |
|---|---|
| planner: `_Reader`, `_read_shard` | `core/gguf/gguf_reader` |
| planner: `shard_paths` | `core/gguf/shard_finder` |
| planner: `read_layout`, `_eog_overrides`, `ModelLayout` | `core/gguf/model_layout` |
| planner: `find_mtp_draft` | `core/gguf/draft_finder` |
| planner: `find_mmproj`, `mmproj_matches`, `mmproj_reserve_bytes` | `core/gguf/mmproj_finder` |
| planner: `query_gpus` (parse part) | `core/hardware/gpu_info` |
| planner: `free_ram_bytes` | `core/hardware/ram_info` |
| launcher: `nvidia-smi` invocation | `core/hardware/hardware_probe` (only place that runs it) |
| planner: `compute_buffer_bytes`, `draft_reserve_bytes` | `core/plan/buffer_model` |
| planner: `_place`, `make_plan` (whole-layer part, auto ubatch, mmap, args) | `core/plan/placement_planner` |
| planner: `Plan`, `Gpu` dataclasses | `core/plan/plan_model` |
| planner: `_reuse_hot`, `make_plan` (per-expert part) | `core/plan/placement_planner` (calls `moe/`) |
| moe_hot: `read_usage`, `usage_tokens`, `read_counts` (JSON only) | `core/moe/usage_io` |
| moe_hot: `build_hot`, `build_hot_cost`, `layer_cost_ms`, `predict_ms`, `hit_rate` | `core/moe/hot_builder` |
| moe_hot: `write_hot`, `read_hot`; launcher: `hot_cache_path` | `core/moe/hot_cache` |
| launcher: `ARCH_TUNING`, `MTP_SPEC` | `core/plan/tuning_tables` |
| launcher: `find_servers`, `_version_key` | `core/launch/server_finder` |
| launcher: `detect_flags` | `core/launch/flag_probe` |
| launcher: `probe_server` | `core/launch/server_probe` |
| launcher: `run_process` command assembly, `server_env`, `_vendor_dll_dirs`, `set_usage_env` | `core/launch/command_builder` |
| (new) server log interpretation | `core/launch/log_parser` |
| launcher: `model_key`, `model_stem`, `usage_path`, constants | `core/common/paths`, `core/common/constants` |
| launcher: settings load/save, `server_cache` | `app/settings_store` |
| launcher: `resolve_server`, `find_draft`, `find_mmproj`, `per_expert_args`, `make_plan` | `app/planner_runner` |
| launcher: `run_process` process lifecycle, `stop_server` | `app/launch_controller` |
| launcher: Tk UI | `src/ui` (rebuilt per 01, not ported) |

Not ported (stay Python tools, run from `.venv` when needed):

- `make_glm_mtp_sidecar.py`: one-off GGUF authoring tool, not part of the app.
- `moe_hot.py` CLI (`main`) and imatrix GGUF counts in `read_counts`: the launcher only
  ever passes usage JSON (`usage_files` returns `.json`), so the app never reads imatrix.
  `UsageIo` rejects non-JSON input with a user-sentence error.

## 3. Parity hazards (each needs a fixture that would catch it)

Numeric:

- H1 `round()` in `_place` is banker's rounding (half to even). C++ must use
  `std::nearbyint` under the default rounding mode, never `std::round` or `+0.5`.
- H2 `-(-k * per_expert // unit)` floors a float (`per_expert` is `experts / n_expert`,
  true division). Port as `std::ceil` on the same double, not integer math.
- H3 `L.experts[il] * len(es) // L.n_expert` is exact integer math; Python ints are
  unbounded, C++ uses `qint64` (largest product here is far below 2^63; assert it).
- H4 Knapsack (`build_hot_cost`) is numpy float64. Reproduce the exact operation order:
  `cumsum` then divide by `total`; `cost = t_ovh + read_ms * (1 - cum)`; strict `<` so
  the lowest `k` wins ties; backtrack from `nb - 1`. Near-ties can flip a choice if the
  order differs, so the golden compares the hot sets exactly.
- H5 Python `sort`/`sorted` are stable. Use `std::stable_sort` everywhere a key sort
  appears (`build_hot` items, `build_hot_cost` order).
- H6 `or` falsiness: `g("expert_count", 0) or 0`, `nextn_predict_layers ... or 0`,
  `L.n_vocab or 152064`, `L.n_embd or 4096`, `interval and (il + 1) % interval`,
  `if not heads`. Missing key, 0 and empty list all collapse to the fallback.
- H7 Per-layer metadata can be scalar or list (`head_count` takes the max of a list,
  `head_count_kv` indexes it per layer).

Text and formats:

- H8 Report lines are compared verbatim. `f"{x:.1f}"`, `f"{x:.0f}"` (percent),
  `f"{n:,}"` (thousands comma) and `_gib` must match byte for byte.
- H9 Hot cache filename is `sha1(json.dumps(key))[:12]`. `json.dumps` defaults:
  `", "` and `": "` separators, `ensure_ascii=True` (`\uXXXX` for non-ASCII), backslash
  escaping, `st_mtime_ns` integers, `"auto"` vs int for ubatch. Reproduce it exactly or
  every existing hot file is orphaned (00 section 7: naming unchanged).
- H10 `os.path.normcase` on Windows lowercases and turns `/` into `\`. `model_key` uses
  `int(st_mtime)` (truncation) while `hot_cache_path` uses `st_mtime_ns`.
- H11 Extra args use `shlex.split(posix=False)` then `strip('"')` per token
  (FR-3.5). Non-POSIX mode keeps quotes inside tokens; fixture the edge cases (quoted
  spaces, mid-word quotes, unbalanced quotes, backslashes in paths).
- H12 Settings values are strings for text fields (`"ctx": "200000"`) and JSON booleans
  for toggles. The C++ writer keeps those types so the Python launcher can still read the
  file during the transition.
- H13 `server_cache` survives only if `server_candidates` equals `find_servers()`
  exactly (order and normcase). The C++ finder must produce the identical list.

GGUF:

- H14 Tensor size = next tensor's offset minus this one; the last runs to the end of the
  file. File size is an input to layout, not derivable from the header.
- H15 Large string arrays (`count > 64`) are skipped without decoding except
  `tokenizer.ggml.tokens`; bool arrays (type 7) go through the per-element path.
  `general.alignment` defaults to 32.

## 4. Where the C++ structure deliberately differs

Results stay identical; these change only when and where work happens.

- D1 Planning is pure. Python `make_plan` writes the hot file and `hot_cache_path`
  deletes older ones as side effects. With live re-plan (01 section 2) that would churn
  disk on every keystroke. C++: the planner returns the hot map and target path; the hot
  file is written (and stale ones pruned) only by `LaunchController` on Run. Reuse still
  reads an existing file during planning, exactly as `_reuse_hot` does.
- D2 Live preview follows the Run path. Python `analyze()` plans without the MTP draft
  reserve; `run_process()` includes it. The live strata view uses the Run path (draft and
  mmproj reserves included) because it must show what Run will do. The golden target is
  `run_process`, not `analyze`.
- D3 Flags for live preview. Python knows build flags only after `resolve_server`
  (which may trial-load). Live preview uses the remembered or manual binary's cached
  flags; if no build is known yet it plans with every optional flag assumed present and
  shows "Server build not chosen yet" in the hint strip. Run always resolves, probes and
  re-plans authoritatively before launching, so the launched args equal Python's.
- D4 Caching. Python re-reads the GGUF layout up to four times per run and re-globs
  `models\` for drafts. C++ caches layout, draft and mmproj discovery per `model_key`
  inside `PlannerRunner`; a cache hit must return the same value a fresh read would.
- D5 Cancellation. The knapsack can take most of a second on GLM at 200K. Planner loops
  check a cancel token between layers so a superseded live re-plan stops early.
- D6 Process control. `QProcess::terminate()` posts WM_CLOSE on Windows, which a console
  `llama-server` ignores; Stop uses `kill()` (Python `terminate()` is TerminateProcess,
  same effect). Child processes get `CREATE_NO_WINDOW` via
  `setCreateProcessArgumentsModifier`, matching Python's `creationflags`.

- D7 CPU-only probe really is CPU-only. Python `probe_server` sets
  `CUDA_VISIBLE_DEVICES=""`; Windows treats an empty value as unset, so the "CPU" trial
  load still initializes CUDA on every GPU (seen in the log capture: `ggml_cuda_init:
  found 2 CUDA devices`). C++ sets `-1`. The probe verdict is unchanged; the probe stops
  touching GPUs that may be serving.

- D9 Bundled server (owner, 2026-11-10; `04` section 6). The app runs its own llama-server
  from the owner's fork, so `find_servers`, the CPU trial-load `probe_server` and the
  per-model `server_cache` are not used in the app's flow. Their ports (Track B, P6a) stay
  as tested library code; goldens that exercise them still run. `detect_flags` stays: it
  checks the bundled or override build once.

## 5. Injection seams (what goldens substitute)

| Interface (core) | Production | Golden / unit |
|---|---|---|
| `ModelSource`: open header stream, file size, exists, list `*.gguf` in a dir | real filesystem | fixture dir mapping original paths to compressed headers + recorded sizes |
| `HardwareSnapshot`: GPU list, free RAM (or unknown) | `HardwareProbe` (nvidia-smi, GlobalMemoryStatusEx) | values from the scenario JSON |
| `UsageSource`: usage JSON bytes + stat | real filesystem | fixture usage files |
| `ServerEnvironment`: candidate dirs, PATH lookup, `--help` text | real filesystem + QProcess | fake tree + captured help text |

## 6. Fixtures (`scripts/export_fixtures.py`)

Runs against the real models on `N:\` once, writes `tests/golden/fixtures` and
`tests/golden/expected`. Re-run only when a fixture model or the Python reference
changes.

1. Model headers. For each fixture model, the bytes up to `data_start` plus a JSON
   sidecar `{original path, file size, mtime}`. Measured header sizes: GLM 3.0bit 9.5 MB,
   its MTP sidecar 9.4 MB, Qwen 3.8 IQ3_S shard 1 11.0 MB, later shards 0.04 MB (no
   tokenizer). Stored zlib-compressed (`qCompress` format) to keep the committed set to
   tens of MB; the test loader decompresses.
2. Fixture models (real): GLM-5.3 3.0bit single file (eog override, indexer, 288
   experts) + its MTP sidecar + `mmproj-model-bf16`; GLM UD-IQ3_XXS 4-shard set; Qwen 3.8
   IQ3_S 2-shard set (`qwen4exp`, 512 experts, indexer) + `mtp-q2_0` + its mmproj.
3. Synthetic GGUFs (written with `third_party/llama.cpp-upstream/gguf-py`) for paths no
   real model covers: dense model, tied embeddings, `head_count_kv` list,
   `full_attention_interval`, MLA `kv_lora_rank`, GGUF v2, bool arrays, missing shard,
   bad magic.
4. Usage profiles. No recorded usage exists on this machine yet. Seeded Zipf-shaped
   synthetic `llama-moe-usage` files per fixture model (decode-only, batch-only, both,
   under `USAGE_MIN_TOKENS`, wrong arch for the `.bak` guard). Python runs on the same
   files, so parity holds regardless of realism.
5. Scenario matrix for `make_plan` through the launcher's Run path: models x GPU sets
   (2x RTX 5090 at realistic free VRAM, single GPU, three GPUs, starved GPU that triggers
   the attention/KV warning, zero budget that raises) x ctx (8K, 32K, 200K) x margin x
   ubatch (auto, 512, 2048) x RAM (fits, does not fit with SSD on/off, unknown) x
   per-expert (off, cold build, reuse fits, reuse no longer fits) x draft/mmproj. Pairwise
   coverage, not the full product: about 150 scenarios, each under 50 ms in C++.
6. Command capture. Instantiate `LlamaStrataLauncher` on a withdrawn `tk.Tk()`, patch
   `query_gpus`, `free_ram_bytes`, `resolve_server`, `detect_flags` (per scenario flag
   sets: full, no `--fit`, no `--load-mode`, no `--moe-hot`, no `draft-mtp`, no
   `--mmproj-device`) and `subprocess.Popen` (records argv + env, never launches), then
   call `run_process`. Expected = argv, env delta, log lines.
7. Small fixtures: `shlex` inputs/outputs, `json.dumps` hot-cache keys + sha1 names,
   `find_servers` over a fake LM Studio tree, `--help` texts captured from the three real
   builds in `third_party/` and LM Studio, `nvidia-smi` CSV samples.

Expected files are JSON: `{args, report, warnings, use_mmap, ubatch, hot_path,
hot_file_text, argv, env}`. The golden test diffs field by field and prints the first
differing report line.

## 7. Phases

Each phase ends with `scripts\run-tests.bat` green and, from phase 2 on, a screenshot of
the running window showing that phase's visible slice.

| Phase | Work | Visible slice | Exit gate |
|---|---|---|---|
| P0 Scaffold | CMake tree per 01, `giga_core`/`giga_app`/QML module, `Result<T>`, `Paths`, `Theme.qml`, composition root with R9 provide/create, empty three-pane shell | window with rail, SplitView (themed handle), status bar | builds clean, 1 test, screenshot |
| P1 Fixtures | `export_fixtures.py` (section 6), fixture loader, `ModelSource` test impl, golden runner that loads every scenario and fails "not implemented" | — | all fixtures exported; golden runner lists ~150 failing scenarios |
| P2 GGUF | reader, shard finder, layout, eog, draft and mmproj finders | Model section: path field, metadata strip from the real file | layout goldens pass for all real + synthetic models; parse of a real 113 GB model under 300 ms |
| P3 Hardware | nvidia-smi CSV parse, RAM, `HardwareProbe` worker, `MonitorController` polling (R28) | GPU chips + GaugePanel live | parse fixtures pass; gauges update at 1 s while running |
| P4 Placement | buffer model, `_place`, auto ubatch, mmap decision, args, tuning tables; `PlannerRunner` with debounce + generation counter | StrataViz renders the C++ plan and follows context/GPU edits live | whole-layer goldens pass; re-plan under 100 ms on GLM 200K |
| P5 Per-expert | usage IO, greedy and cost builders, predict/hit rate, hot cache key + read/write, reuse; D1 split | per-expert toggle, recorded-token hint strip, hit rate in strata tooltips | per-expert goldens pass incl. hot file text and cache filename |
| P6 Launch | server finder, flag probe, server probe, command builder, env, usage `.bak` guard, `LaunchController` (D6) | Run/Stop, ServerCard, LogView streaming raw lines | command goldens pass; real launch on this machine: GLM 3.0bit argv identical to the Python launcher's, server reaches listening |
| P7 Log state | capture real logs (both models: load, serve, tok/s lines, wrong build, missing DLL `0xC0000135`, VRAM OOM) as fixtures; `LogParser`; merged state machine (FR-4.3) | status pill states, loading progress, tok/s | log fixtures pass; pill walks stopped→probing→planning→loading→serving on a real run |
| P8 App | `SettingsStore` migration (H12, H13, unknown keys), presets, tray, single instance, toasts | Presets page, tray icon states | migration test on the real `launcher_settings.json`; tray hide/restore while serving |
| P9 Cutover | `start.cmd` points at the release build, `start.bat` retired, 00 section 8 criteria checked side by side with the Tk launcher | — | all five success criteria demonstrated on real hardware |

### 7.1 Parallelism

The phases are numbered for the critical path, not as a strict sequence. What actually
couples them is a handful of shared types, so those are written first, in P0, by one
person: `ModelLayout`, `Gpu` + `HardwareSnapshot`, `Plan`, `UsageCounts`, `HotMap`, and
the section 5 interfaces (headers only, no logic). Once they exist:

| Wave | Runs in parallel | Needs |
|---|---|---|
| 0 | P0 scaffold + shared-type headers ‖ P1 `export_fixtures.py` ‖ real log capture with the existing Tk launcher (it launches servers today) | nothing |
| 1 | P2 GGUF ‖ P3 hardware ‖ P5a moe algorithms (usage IO, greedy + cost builders, predict, hit rate, hot file read/write, cache key + sha1) ‖ P6a launch pieces (server finder, flag probe, server probe, env + vendor DLLs, shlex tokenizer, usage `.bak` guard) ‖ P7a `LogParser` on captured logs ‖ P8 settings migration, presets, tray, single instance, QML components | wave 0 |
| 2 | P4 placement + live re-plan, then P5b per-expert integration (`_reuse_hot`, the per-expert branch of `make_plan`) ‖ P6b command builder + `LaunchController` | P2 (P4); P4 `Plan` values for full argv goldens |
| 3 | P6 real-launch gate → P7 real-run pill gate → P9 cutover | everything |

Critical path: P0 → P2 → P4 → P5b → P6 gate → P9. Everything else hangs off it and should
never block it.

Practical limits on this machine:

- At most three concurrent tracks. Suggested split: (A) engine critical path P2 → P4 →
  P5b → P6b; (B) P5a + P6a + P7a, all pure core modules with their own fixtures;
  (C) P3 + P8 + UI components. One worker per track, not one per phase.
- Tracks share `CMakeLists.txt` files: each track adds only its own subdirectory's
  sources, and the top-level/`src/core` lists are edited by track A only.
- Worktrees reuse the main checkout's tools through `GIGA_STRATA_VENV`, never by
  junctioning `.venv` into the worktree (deleting a worktree with a junction deletes
  the main `.venv` contents).
- The compiler-stability budget is about 8 jobs for the whole machine, so with three
  tracks building at once each sets `GIGA_STRATA_JOBS=3` (03 section 7).

## 8. Live parity check (beyond goldens)

`tests/tools/plan_cli` (test executable, not shipped) takes a settings JSON and a
hardware snapshot JSON and prints the plan JSON. A companion mode in
`export_fixtures.py --live` snapshots the real nvidia-smi and RAM once, injects the same
snapshot into both Python and `plan_cli`, and diffs. Run it at the end of P4, P5 and P6
against the current `launcher_settings.json` models, so parity is checked on today's real
VRAM numbers, not only on fixture values.

## 9. Known migration facts

- The project moved from `N:\giga-strata` to `D:\Projects\giga-strata`. The current
  `launcher_settings.json` points `binary` and every `server_cache` entry at `n:\...`.
  Python behavior (kept): candidates differ, so the cache is dropped and the next Run
  re-probes; a missing `binary` falls back to the default build. `N:\giga-strata` still
  exists, so those paths resolve today; the migration test covers both cases.
- `moe_usage\` does not exist yet in either location; the first recorded session
  creates it.

## 10. Risks

| Risk | Mitigation |
|---|---|
| Knapsack near-ties flip on float order | H4 exact operation order; goldens compare hot sets; if a flip remains, log both costs and match the Python order, never widen tolerance |
| Fixture size in git | zlib headers; if the set exceeds 60 MB, keep only shard 1 of each split model and synthesize later shards' headers |
| Free VRAM drifts between Python and C++ runs | only ever compare under one injected snapshot (section 8) |
| Synthetic usage hides real-profile behavior | after P6, record a real GLM session (FR-1.6) and add it as a fixture |
| QML perf on 90-layer strata with live re-plan | StrataViz renders from one model property update per plan; benchmark in P4 exit gate |
