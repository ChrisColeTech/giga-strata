# Giga Strata

A Windows desktop app for running large Mixture-of-Experts GGUF models with
[llama.cpp](https://github.com/ggml-org/llama.cpp)'s `llama-server` on consumer hardware.

Models like GLM-5.3 or Qwen 3.8 Flash are 70–115 GB, far more than any pair of GPUs holds.
Giga Strata plans where every part of the model goes across the memory tiers you have:
**GPU VRAM → system RAM → SSD streaming**, then launches `llama-server` with exactly those
placements and monitors it while it serves an OpenAI-compatible API.

## Features

- **Tiered placement planner.** Reads each model's GGUF header (no weights loaded), measures
  free VRAM and RAM, and splits layers across GPUs in proportion to their free memory. It
  fills each GPU's leftover VRAM with expert tensors and leaves the rest in RAM,
  memory-mapped from the SSD only when RAM runs out. Compute buffers, KV cache, sparse-attention
  indexers, MTP drafts and vision projectors are all budgeted.
- **Per-expert placement.** Record which experts a model actually routes to while you use it,
  then keep the most-used experts of every layer in VRAM (`--moe-hot`). A cost model picks,
  per layer, between whole-in-VRAM, whole-in-RAM and split.
- **Live re-planning.** Change the context size, the GPUs or any parameter and the memory
  split updates immediately, before you load anything.
- **Model library.** Every GGUF in your model folders, with split sets shown as one model and
  MTP drafts and vision projectors attached to their model. Sort and filter by type, quant and
  size; each model keeps its own saved parameters.
- **Serving.** Start, stop and reload from one screen; state, load progress, tokens/s,
  uptime and per-GPU VRAM stay in sync between the Serving screen and the status bar. The
  server log is filterable (server / planner / warnings).
- **Speculative decoding.** MTP draft models are discovered automatically and wired with the
  right speculation flags per architecture, including the n-gram + MTP combination.
- **Tray and single instance.** Keep serving in the tray with the window closed.

## Requirements

- Windows 10 or 11, x64
- NVIDIA GPU(s) with a driver that supports CUDA 13. The bundled server build targets RTX 50
  series (sm_120).
- Enough system RAM for the part of the model that does not fit in VRAM (or an SSD for
  streaming)

## The bundled llama-server

Giga Strata ships its own `llama-server`, built from a llama.cpp fork that adds the features
the planner relies on: per-expert hot/cold placement (`--moe-hot`), expert-usage recording,
and kernel and scheduling changes for the supported MoE architectures. The build copies it,
with the CUDA 13 runtime DLLs it needs, into `server\` next to the app, and stamps
`server\BUILD_INFO.txt` with the fork commit.

The fork is not published yet. Without it the app still builds and runs: point
**Settings → Server → Use a different llama-server** at any recent `llama-server.exe`.
Features that need the fork's flags are then switched off with the reason shown.

## Building

Requires Visual Studio 2022 (or Build Tools) with the C++ workload, and Python 3.12 for the
tooling (`cmake`, `ninja`, `aqtinstall` installed into a `.venv` in the repo root).

```bat
scripts\setup.bat                 :: one time: installs Qt 6.8.3 (msvc2022_64) into C:\Qt
scripts\build.bat release         :: configure + build + deploy Qt + package the server
scripts\run-tests.bat             :: unit tests and the golden parity tests
start.cmd                         :: run the release build
```

To package the server, put the fork's checkout at `third_party\llama.cpp-upstream` with its
`bin\bin\Release\llama-server.exe` built, or set `GIGA_STRATA_FORK_DIR`. Without a fork the
server step reports SKIPPED. Every script, gate and override is documented in
[docs/03](docs/03-BUILD_ENVIRONMENT_GUIDE.md).

## How it is tested

The planner was ported from a Python reference implementation. Its behavior is pinned by
**golden tests**: 182 real launch scenarios were recorded from the reference, covering real GLM-5.3
and Qwen 3.8 model headers, GPU sets, context sizes, per-expert profiles and server flag sets.
The C++ build must reproduce every one byte for byte: layer split, `-ot` overrides, report
lines, warnings, hot-expert files and the final command line. The suite also replays captured
`llama-server` logs through the log parser.

## Documentation

| Doc | What it covers |
|---|---|
| [00 Requirements](docs/00-REQUIREMENTS.md) | what the app does |
| [01 Architecture and style guide](docs/01-ARCHITECTURE_AND_STYLE_GUIDE.md) | layers, rules, theme |
| [02 Port plan](docs/02-PORT_PLAN.md) | how the engine was ported and proven |
| [03 Build environment](docs/03-BUILD_ENVIRONMENT_GUIDE.md) | toolchain, scripts, lessons learned |
| [04 UI design](docs/04-UI-DESIGN.md) | every screen and window |
| [05 App contracts](docs/05-APP-CONTRACTS.md) | the controllers the UI binds to |

## Project layout

```
src/core     the engine (QtCore only): GGUF reading, planning, per-expert placement, launch
src/app      controllers exposed to QML (library, planner, launcher, log, settings, tray)
src/ui       the Qt Quick interface
tests        unit tests, golden parity tests, fixtures, test-only tools
scripts      build, test and packaging scripts
docs         design documents
```
