# llama-server console log fixtures (P7 / FR-4.3, FR-4.4)

Real console output of `llama-server.exe`, captured 2026-11-10 on this machine for the C++
`LogParser`. Each `<name>.log` is the merged stdout+stderr exactly as the process wrote it
(raw bytes, UTF-8, **CRLF line endings** because the server writes through Windows text-mode
stdio). Each `<name>.json` holds `argv` (repo-relative for repo binaries), `env_overrides`,
`build`, `model`, `exit_code` (+ `exit_code_hex`), `duration_s`, `notes`, and where it applies
`listening_after_s` and the `requests` sent.

**Everything was captured CPU-only**: `CUDA_VISIBLE_DEVICES=-1`, `-ngl 0`. That is why every log
starts with `ggml_cuda_init: failed to initialize CUDA: no CUDA-capable device is detected`. A
real GPU run does not print that line (see "GPU lines not captured" at the bottom).

Probe/stop semantics match `scripts/llama_launcher.py`: probes use the `probe_server` argv
(`-m M -ngl 0 -c 256 -np 1 --no-warmup --host 127.0.0.1 --port P`) and are killed as soon as a
line containing `listening` appears; serves are stopped with `Popen.terminate()`
(= `TerminateProcess`, exit code 1), as `stop_server` does. Ports came from binding port 0.

## Builds

| key | binary | log style |
|---|---|---|
| `upstream` | `third_party/llama.cpp-upstream/bin/bin/Release/llama-server.exe` (fingerprint `b5-2441b53`) | new logger: `M.SS.mmm.uuu L tag  func: msg`, terse at its default verbosity 3 |
| `glm5` | `third_party/llama.cpp-glm5/bin/bin/Release/llama-server.exe` (`build_info: b8684-694a46a9f`) | old logger: no timestamp/level, full metadata dump, progress dots |
| `lmstudio-cuda12-2.67.0` | `~/.lmstudio/extensions/backends/llama.cpp-win-x86_64-nvidia-cuda12-avx2-2.67.0/llama-server.exe` (fingerprint `b1-0893ec9`), with `vendor/win-llama-cuda12-vendor-v2` prepended to PATH | same new logger as upstream, minus the `common_param: ... verbosity` line, plus `LMSTUDIO_STARTUP_ERROR:` json on load failure |

Models: `Qwen3-8B-Q4_K_M.gguf` (4.68 GiB, `D:\models\image-models\hymotion\Qwen3-8B-GGUF\`) is
the small model that loads standalone on every build. Neither suggested small file loads
standalone: `N:\strata\model\mtp-q2_0.gguf` is a `qwen4exp-mtp` draft (unknown arch upstream /
LM Studio, tensor type 42 rejected by glm5), and the GLM MTP sidecar has no main-model tensors
(glm5: missing tensor; upstream: access violation). Those became the failure fixtures.

## Files

| file | bytes | build | model | exit | what it shows |
|---|---:|---|---|---|---|
| `probe_ok_upstream_qwen3-8b.log` | 1167 | upstream | Qwen3-8B | 1 (killed at listening, 3.7 s) | probe, loads |
| `probe_ok_upstream_qwen3-8b_lv4.log` | 23554 | upstream | Qwen3-8B | 1 (killed at listening) | same probe with `-lv 4`: what upstream hides at default verbosity (fit memory breakdown, `load_tensors` lines; still no progress dots) |
| `probe_ok_glm5_qwen3-8b.log` | 10855 | glm5 | Qwen3-8B | 1 (killed at listening, 4.0 s) | probe, loads, old log style |
| `probe_ok_lmstudio_qwen3-8b.log` | 1058 | lmstudio | Qwen3-8B | 1 (killed at listening, 3.4 s) | probe, loads |
| `probe_ok_upstream_glm53-3bit.log` | 1403 | upstream | GLM-5.3 3.0bit (113 GB) | 1 (killed at listening, 29.1 s) | **asked for as probe_fail, but upstream now loads `glm5-next`**; 24 s silent gap during load |
| `probe_ok_lmstudio_glm53-3bit.log` | 1294 | lmstudio | GLM-5.3 3.0bit | 1 (killed at listening, 15.3 s) | **also loads** (was expected to fail) |
| `probe_ok_glm5_glm53-3bit.log` | 15409 | glm5 | GLM-5.3 3.0bit | 1 (killed at listening, 16.5 s) | the intended build on the GLM file; 107974 MiB CPU_Mapped + 15844 MiB CPU_REPACK, 98 progress dots |
| `probe_fail_upstream_unknown_arch.log` | 1476 | upstream | mtp-q2_0 | 1, 0.3 s | `unknown model architecture: 'qwen4exp-mtp'` |
| `probe_fail_lmstudio_unknown_arch.log` | 1621 | lmstudio | mtp-q2_0 | 1, 0.2 s | same + `LMSTUDIO_STARTUP_ERROR:{...}` |
| `probe_fail_glm5_invalid_ggml_type.log` | 2028 | glm5 | mtp-q2_0 | 1, 0.3 s | wrong tensor layout: `invalid ggml type 42` |
| `probe_fail_glm5_missing_tensor.log` | 13652 | glm5 | GLM MTP sidecar | 1, 1.0 s | `missing tensor 'blk.0.attn_norm.weight'` (sidecar is only valid as `-md`) |
| `probe_crash_upstream_mtp_sidecar.log` | 752 | upstream | GLM MTP sidecar | **0xC0000005** (3221225477), 0.5 s | hard crash mid-load, **no error line at all**: log ends at `loading model '...'` |
| `missing_dll_lmstudio.log` | **0** | lmstudio, vendor dir NOT on PATH | Qwen3-8B | **0xC0000135** (3221225781), 0.1 s | prints nothing; PATH had only CUDA v13.0, no `cudart64_12.dll` |
| `bad_args_upstream_ctx_abc.log` | 584 | upstream | — | 1, 0.2 s | `-c abc` |
| `bad_args_lmstudio_ctx_abc.log` | 584 | lmstudio | — | 1, 0.2 s | `-c abc` (byte-identical text except timestamp) |
| `bad_args_upstream_unknown_flag.log` | 344 | upstream | — | 1, 0.2 s | `--no-such-flag` |
| `bad_args_glm5_unknown_flag.log` | 329 | glm5 | — | 1, 0.2 s | `--no-such-flag` |
| `serve_cpu_upstream_qwen3-8b.log` | 2974 | upstream | Qwen3-8B | 1 (terminated), 19.3 s | listening at 4.1 s, `/completion` + streaming chat, timing lines, terminate |
| `serve_cpu_glm5_qwen3-8b.log` | 14123 | glm5 | Qwen3-8B | 1 (terminated), 19.3 s | same, old log style (multi-line timing block) |
| `serve_cpu_lmstudio_qwen3-8b.log` | 2865 | lmstudio | Qwen3-8B | 1 (terminated), 18.6 s | same |

Serve argv: `-m Qwen3-8B-Q4_K_M.gguf -ngl 0 -c 4096 -np 1 --alias Qwen3-8B-Q4_K_M --host 127.0.0.1 --port P`.
Requests: `POST /completion {"prompt":"Write a haiku about rocks.","n_predict":64}` then
`POST /v1/chat/completions {"messages":[...same prompt...],"max_tokens":64,"stream":true}`.
Decode was ~10 tok/s on CPU for every build.

`--help` output is captured by a separate task and is not here.

## Analysis: line patterns per state

Notation: `TS` = the new logger's prefix `^\d+\.\d{2}\.\d{3}\.\d{3} [IWEDT] ` (elapsed
`min.sec.ms.us` since process start, then a level letter). Upstream and LM Studio emit it on
almost every line; glm5 never does. The parser should strip an optional
`^(?:\d+\.\d{2}\.\d{3}\.\d{3} [A-Z] )?` and match the rest. Some lines have **no prefix even
in the new logger** (the `warning: no usable GPU found` block, `error: invalid argument`,
`LMSTUDIO_STARTUP_ERROR`, usage text). Lines end in `\r\n`.

### 1. Process started / early init (before loading)
- upstream: `TS I srv  llama_server: initializing ...` then `TS I cmn  common_param: common_params_print_info: verbosity = 3 (adjust with the \`-lv N\` CLI arg)`
- lmstudio: `TS I srv  llama_server: initializing ...` (no verbosity line)
- glm5: `build_info: b8684-694a46a9f`, `system_info: n_threads = 24 ...`, `init: using 23 threads for HTTP server`, `start: binding port with default address family`, `main: loading model`

### 2. Loading start (all builds — the most reliable "loading" signal)
- `srv    load_model: loading model '<path>'` (upstream/lmstudio: `TS I srv    load_model: loading model '...'`)
- glm5 additionally prints `main: loading model` one line earlier.

### 3. Load progress
- **upstream and lmstudio print NO progress at default verbosity.** Between the tokenizer
  warnings (~0.5 s) and `TS I cmn          init: llama threadpool init, n_threads = 24` there is
  silence: 24 s on the 113 GB GLM file (`probe_ok_upstream_glm53-3bit.log`). Even `-lv 4` adds
  `load_tensors:` lines but no dots. The pill must show indeterminate "loading" for these builds;
  elapsed time from the `TS` prefix is the only clock.
- glm5 (old logger), in order:
  - `llama_model_loader: loaded meta data with N key-value pairs and N tensors from <path> (version GGUF V3 (latest))`
  - `print_info: file size   = 4.68 GiB (4.90 BPW) ` (total model size, useful as a denominator)
  - `load_tensors: loading model tensors, this can take a while... (mmap = true, direct_io = false)`
  - `load_tensors:   CPU_Mapped model buffer size =  4762.19 MiB`, `load_tensors:   CPU_REPACK model buffer size =  3199.50 MiB`
  - a line of dots `.....` (85 dots for Qwen3-8B, 98 for GLM). They are written **one at a time
    with no newline** while tensors load, then a newline at the end. A line-buffered reader sees
    one line only after loading; to show live progress the parser must read raw chunks and count
    dots in the partial line (each dot is roughly 1 % of tensor data; the line reaches ~85-100,
    never exactly 100, so treat the newline as 100 %).
  - post-load: `llama_context: constructing llama_context`, `llama_kv_cache:        CPU KV buffer size =   576.00 MiB`, `sched_reserve: ...`, `srv    load_model: initializing slots, n_slots = 1`
- Shared "load finished, setting up slots": upstream/lmstudio `TS I srv    load_model: initializing, n_slots = 1, n_ctx_slot = 256, kv_unified = 'false'`; glm5 `srv    load_model: initializing slots, n_slots = 1`.

### 4. Ready / serving
- upstream, lmstudio: `TS I srv  server_run_i: model loaded` then `TS I srv  server_on_li: listening on http://127.0.0.1:<port>`
- glm5: `main: model loaded` then `main: server is listening on http://127.0.0.1:<port>` then `main: starting the main loop...`
- Common regex: `listening on (https?://\S+)` (both forms contain it; `"listening"` substring alone, as the Python probe uses, also works and appears nowhere else in any capture).
- glm5 only: `srv  update_slots: all slots are idle` after ready and after every request (idle marker).

### 5. Per-request speed (FR-4.4)
Request start: `slot launch_slot_: id  0 | task <n> | processing task, is_child = 0` (all builds).

upstream / lmstudio — every line prefixed, one per field:
```
TS I slot print_timing: id  0 | task 0 | prompt eval time =     161.46 ms /     7 tokens (   23.07 ms per token,    43.35 tokens per second)
TS I slot print_timing: id  0 | task 0 |        eval time =    6342.69 ms /    64 tokens (  100.68 ms per token,     9.93 tokens per second)
TS I slot print_timing: id  0 | task 0 |       total time =    6504.15 ms /    71 tokens
TS I slot print_timing: id  0 | task 0 |    graphs reused =         63
TS I slot      release: id  0 | task 0 | stop processing: n_tokens = 70, truncated = 0
```
glm5 — the header line ends with `| ` and the values follow on **unprefixed** lines:
```
slot print_timing: id  0 | task 0 | 
prompt eval time =     149.54 ms /     7 tokens (   21.36 ms per token,    46.81 tokens per second)
       eval time =    6226.90 ms /    64 tokens (   97.30 ms per token,    10.28 tokens per second)
      total time =    6376.44 ms /    71 tokens
```
One regex covers both after stripping an optional `TS` and an optional
`slot print_timing: id\s+\d+ \| task \d+ \|` prefix:
- generation: `^\s*eval time =\s*([\d.]+) ms /\s*(\d+) tokens \(\s*([\d.]+) ms per token,\s*([\d.]+) tokens per second\)`
- prompt: same with `prompt eval time`. **Anchor the generation regex at line start (after the
  prefix) or require the preceding char not be `t`** — a naive `eval time` search also matches
  the prompt line.
- tok/s for the readout = group 4 of the generation line (display the server's figure).
  Recomputing differs by build: upstream/lmstudio print `ms per token = ms / (tokens - 1)`
  (`6342.69 / 63 = 100.68`, `1000 / 100.68 = 9.93`), glm5 prints `ms / tokens`
  (`6226.90 / 64 = 97.30`). If recomputed, `tokens / (ms / 1000)` gives 10.09 vs the
  printed 9.93 for upstream — use the printed value so the readout matches the console.
- the task id ties prompt/eval/total together; `release: ... stop processing` closes the request.
- glm5 also prints `srv  log_server_r: done request: POST /completion 127.0.0.1 200` (and for
  `/v1/chat/completions` that line can come BEFORE the timing block for a streamed request).
  upstream/lmstudio print no per-HTTP-request line at default verbosity.
- No streaming per-token progress lines exist at default verbosity in any build; the speed is
  known only when a request finishes.

### 6. Error loading model / wrong build (FR-4.3 "failed")
Primary pattern (all builds): `llama_model_load: error loading model: <reason>`
- **It appears twice** in every failure (once in the `-fit` pre-pass, once in the real load);
  de-duplicate or take the first.
- Reasons seen:
  - unknown architecture: `error loading model: unknown model architecture: 'qwen4exp-mtp'` (upstream, lmstudio)
  - wrong tensor layout: `gguf_init_from_file_ptr: tensor 'mtp.layers.0.mlp.experts.down_proj' has invalid ggml type 42. should be in [0, 41)` then `gguf_init_from_file_ptr: failed to read tensor info` then `llama_model_load: error loading model: llama_model_loader: failed to load model from <path>` (glm5) — the useful reason is on the `gguf_init_from_file_ptr` line, not the `error loading model` line
  - missing tensor: `llama_model_load: error loading model: missing tensor 'blk.0.attn_norm.weight'` (glm5)
  - bad hyperparameters (seen during exploration, not kept): `error loading model: error loading model hyperparameters: key not found in model: qwen3.context_length`
- Follow-up lines (any of them = failed): `llama_model_load_from_file_impl: failed to load model`;
  upstream/lmstudio `TS E srv    load_model: failed to load model, '<path>'` (lmstudio appends `: error loading model: <reason>`);
  glm5 `srv    load_model: failed to load model, '<path>'`.
- LM Studio only, machine-readable: `LMSTUDIO_STARTUP_ERROR:{"schemaVersion":1,"code":"model_load_failed","message":"error loading model: unknown model architecture: 'qwen4exp-mtp'"}` (unprefixed line; JSON after the colon).
- `E` level letter: in upstream/lmstudio every failure line carries ` E `; a generic
  "last E line before exit" is a decent fallback reason.

### 7. Exit
- After a load error: upstream/lmstudio `TS I srv   operator (): operator (): cleaning up before exit...` (lmstudio spells it `operator():` without the space) then `TS E srv  server_run_i: exiting due to model loading error`; glm5 `srv   operator (): operator (): cleaning up before exit...` then `main: exiting due to model loading error`. Exit code 1.
- After TerminateProcess (launcher Stop, probe kill): **no exit line at all**; the log just
  ends. Exit code 1. "exiting" must therefore come from the process-exit event plus the app's
  own knowledge that it asked for the stop, not from the log.
- Crash: `probe_crash_upstream_mtp_sidecar.log` — exit `0xC0000005` with the last line being
  `loading model '...'`. No error text: the state machine must map "process exited while
  loading, no error line" to failed and show the NTSTATUS (`access violation`).
- Missing DLL: exit `0xC0000135` (3221225781) with **zero bytes of output**; the loader fails
  before `main`. Map the code to "a DLL is missing (CUDA runtime?)" as the launcher does.
- Bad args (exit 1, < 0.25 s, never reaches `loading model`):
  - unknown flag: `error: invalid argument: --no-such-flag` (all builds, unprefixed)
  - bad value: `error while handling argument "-c": invalid stoi argument` followed by a
    blank line, `usage:`, the flag's help row(s), `to show complete usage, run with -h`.

### 8. Noise worth knowing
- `warning: no usable GPU found, --gpu-layers option will be ignored` + 2 more `warning:` lines: only when CUDA is hidden/broken. In a real run their presence means the GPUs were not seen — worth surfacing as an error, not ignoring.
- `TS W srv  server_setup: security: no API key is set and CORS allows all origins ...` — every new-logger start.
- `W load: control-looking token ...`, `W load: special_eot_id is not in special_eog_ids ...` — model-quality warnings, harmless.
- glm5 dumps ~70 `llama_model_loader: - kv` lines and ~50 `print_info:` lines per load; `print_info: arch                  = glm5-next` gives the architecture if wanted.

### Differences between builds (summary)
| | upstream | lmstudio 2.67.0 | glm5 fork |
|---|---|---|---|
| timestamp + level prefix | yes | yes | no |
| verbosity line | yes | no | no |
| build id line | no | no | `build_info: b8684-...` |
| load progress | none | none | buffer sizes + dot line |
| ready line | `server_on_li: listening on URL` | same | `main: server is listening on URL` |
| timing block | 5 prefixed lines | same | header + 3 unprefixed lines |
| HTTP request log | no | no | `log_server_r: done request: ...` |
| load-failure json | no | `LMSTUDIO_STARTUP_ERROR:{...}` | no |
| loads GLM-5.3 3.0bit | **yes** (29 s CPU) | **yes** (15 s CPU) | yes (16.5 s CPU) |

## GPU-dependent lines NOT captured (capture when the GPUs are free)

Nothing below is in these fixtures. Needed later with a real `-ngl`/`-ot` launch:
- CUDA device init. glm5 prints (seen by accident during exploration when the env var was
  empty, see note below):
  ```
  ggml_cuda_init: found 2 CUDA devices (Total VRAM: 65158 MiB):
    Device 0: NVIDIA GeForce RTX 5090, compute capability 12.0, VMM: yes, VRAM: 32579 MiB
    Device 1: NVIDIA GeForce RTX 5090, compute capability 12.0, VMM: yes, VRAM: 32579 MiB
  llama_model_load_from_file_impl: using device CUDA0 (NVIDIA GeForce RTX 5090) (0000:02:00.0) - 30991 MiB free
  llama_model_load_from_file_impl: using device CUDA1 (NVIDIA GeForce RTX 5090) (0000:04:00.0) - 30991 MiB free
  ```
  Upstream printed **nothing** about CUDA at default verbosity when the GPUs were visible.
- `load_tensors: offloaded N/M layers to GPU`, `load_tensors:        CUDA0 model buffer size = ...`, `CUDA_Host` buffers.
- `llama_kv_cache:      CUDA0 KV buffer size = ...`, `sched_reserve:      CUDA0 compute buffer size = ...`, graph splits > 1.
- VRAM OOM: `ggml_backend_cuda_buffer_type_alloc_buffer: allocating ... MiB on device N: cudaMalloc failed: out of memory` and the resulting `failed to allocate ... buffer` / `error loading model` / `failed to create context` path, plus `-fit` messages when it would change the plan.
- MTP draft / speculative lines (`-md ... -devd CUDAn`), `--moe-hot` / usage-recording lines from the strata fork, `-ot` override-tensor placement lines.
- Real GPU-speed timing lines (same format, different numbers) and CUDA graph warnings.
- `0xC0000135` with the vendor dir present but a mismatched driver, and CUDA 11 builds hanging on RTX 50 (noted in `find_servers`).

### Launcher finding: `CUDA_VISIBLE_DEVICES=""` does not hide the GPUs on Windows
`probe_server` runs with `server_env(binary, [])`, i.e. `CUDA_VISIBLE_DEVICES=""`. In the first
exploration run with that exact env, the glm5 build still printed `ggml_cuda_init: found 2 CUDA
devices` and `using device CUDA0 ... MiB free` — the empty value is treated as unset, so the
"CPU-only" probe creates CUDA contexts on both GPUs. With `-1` the CUDA init fails as intended.
The C++ port should use `CUDA_VISIBLE_DEVICES=-1` for probes (and the Python launcher has the
same issue). Those exploratory logs were not kept as fixtures.
