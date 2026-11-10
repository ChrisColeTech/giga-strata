# 00 — Requirements

Living document. Add to it as the project evolves. Every feature added later gets a numbered
entry here first (or a dedicated `0N-<feature>.md`) before code is written.

## 1. What this product is

Giga Strata is a Windows desktop utility that launches and monitors a local `llama-server`
(llama.cpp) for large Mixture-of-Experts GGUF models, using a planning engine that spreads
model layers across the available memory tiers — GPU VRAM, system RAM, then SSD streaming —
and can place individual experts per layer from recorded usage data.

The current Tkinter launcher is being replaced by a modern Qt 6 QML application with the
engine fully ported to C++. The Python scripts remain as the reference implementation and
as golden-test generators.

### Design decisions (agreed)

| Decision | Choice |
|---|---|
| Engine | Full C++ port (golden-tested against current Python output) |
| Toolkit | Qt 6 Qt Quick (QML) + Qt Quick Controls 2 |
| Look and feel | "Strata Console" — deep graphite dark, tier-mapped accents |
| Layout | Models and Serving screens, Settings and Model config child windows, log drawer, status bar: see `04-UI-DESIGN.md` (owner-approved 2026-11-10) |
| v1 feature scope | Strata visualization + GPU gauges, log parsing + status pills, presets/profiles, tray minimize-while-serving |

## 2. Users and use model

- Single power user on their own Windows machine with one or more NVIDIA GPUs.
- The session flow is: pick a model, confirm or adjust placement, start the server,
  watch it load and serve, stop or keep serving (window may minimize to tray).
- The app is a launcher and monitor, not a chat client. API consumers connect to
  `llama-server` directly.

## 3. Functional requirements

### FR-1 Engine port (parity with current Python)

- FR-1.1 Read GGUF layout (arch, layer/expert counts, quant sizes, split shards) from model
  files without loading weights.
- FR-1.2 Run the bundled llama-server built from the owner's fork (`04` section 6); a
  Settings override may point at another build. (Owner, 2026-11-10: replaces the Python
  launcher's server discovery and per-model CPU trial-load auto-pick.)
- FR-1.3 Detect which optional flags a given server build understands (`--help` scrape,
  cached per binary).
- FR-1.4 Plan tier placement: per-GPU free VRAM (nvidia-smi) and free RAM, KV reserve,
  compute-buffer estimation, safety margin, micro-batch auto rule, whole-layer placement.
- FR-1.5 Per-expert placement from recorded expert usage (cost-aware hot-expert selection,
  cached `--moe-hot` files keyed by model/usage/GPU-set/context, stale cache pruning).
- FR-1.6 Expert usage recording: env wiring, file compatibility guard (`.bak` move),
  flush interval, token-count summaries, minimum-samples warning.
- FR-1.7 MTP draft discovery and speculation args per architecture (incl. ngram-mod
  variant), vision projector (mmproj) discovery and matching warning.
- FR-1.8 Per-architecture tuning tables (threads, sampling flags, env vars) and final
  command assembly identical in observable behavior to the current launcher.
- FR-1.9 Launch hidden with correct env (`CUDA_VISIBLE_DEVICES`, `CUDA_DEVICE_ORDER`,
  vendor DLL path), capture merged stdout/stderr, terminate on stop/close, report exit
  code and the missing-DLL hint.
- FR-1.10 Plan-only analysis (no launch) with the same report and warnings. Delivered as
  live planning (FR-4.7), not as a separate button or mode.

### FR-2 Model and server selection UI

- FR-2.1 Model path field with file picker; reads and displays model metadata
  (arch, size, quant, expert count, shard info) as a compact info strip.
- FR-2.2 No server choice in the normal flow; the Settings override is the only one.
- FR-2.3 Host/port fields with validation; displays the reachable server URL when running.

### FR-3 Hardware and plan controls

- FR-3.1 GPU picker: one chip per detected GPU (index + short name + free/total),
  click to include/exclude, order = chip order. Manual override field stays available.
- FR-3.2 Context size: segmented control over common sizes with a custom option.
- FR-3.3 Fields for margin, micro-batch (auto or number), mmap, flash attention,
  MTP, vision/mmproj, usage recording, per-expert placement.
- FR-3.4 Per-expert placement control disabled with a tooltip until the model has
  recorded usage; shows recorded token counts and the low-samples warning inline.
- FR-3.5 Extra server arguments field, tokenized the same way as current `shlex` behavior.

### FR-4 Live monitor

- FR-4.1 Strata visualization: per-layer stacked placement bars showing which tier
  (GPU0..n, RAM, SSD) holds each layer, sizes included; updates from the active plan.
- FR-4.2 Per-GPU gauges: used/total VRAM, live-refreshing (poll nvidia-smi while active).
- FR-4.3 Server state pill from one merged state machine: log parsing supplies loading,
  serving, exiting and failed; controller phase events from the app layer supply probing
  and planning; stopped when neither source is active.
- FR-4.4 Generation speed readout (tokens/s) parsed from server output.
- FR-4.5 Structured log view: level-colored lines (planner/launcher/server prefixes),
  filter segmented (all/server/planner/warnings); copy and clear live in the right-click
  context menu only — no visible destructive buttons; auto-scroll with pause-on-scroll-up
  and a floating "Jump to latest" pill.
- FR-4.6 Run/Stop in the status bar; single primary action; state machine prevents
  double-launch and allows stop from any state.
- FR-4.7 Live re-plan: every plan-affecting settings edit re-runs the planner after a
  400 ms debounce, off the UI thread; superseded in-flight results are discarded. While
  the server runs, the live view keeps the running plan and edits show as a pending plan
  ("Changes apply on next run"). Server probing runs only on model change and on Run.

### FR-5 Presets and settings

- FR-5.1 Each model saves its own parameters in its own config file (`04` section 2.3);
  there are no named presets.
- FR-5.2 Persistence is superset-compatible with today's `launcher_settings.json`:
  same keys read and written, new keys (e.g. `uiSplitRatio`) appended, unknown keys
  preserved; server cache and candidates migrate transparently.
- FR-5.3 Settings autosave on change (debounced) and on exit.

### FR-6 Tray and lifecycle

- FR-6.1 Closing or minimizing the window while serving hides it to the tray; the server
  keeps running. Tray menu: show, stop server, quit (stops server).
- FR-6.2 Quitting via tray stops the server and flushes settings.
- FR-6.3 Single instance: launching a second instance raises the first.

## 4. User experience requirements

- UX-1 Label diet: no helper-text paragraphs in the layout. Field meaning lives in the
  label plus a tooltip; only errors and state warnings appear inline.
- UX-2 The strata visualization is the primary explanation of what the app does.
- UX-3 Zero emoji anywhere in UI, docs-in-code, or log rendering.
- UX-4 Everything is keyboard reachable with a visible focus ring; Enter in the model
  field runs the primary action.
- UX-5 Density reads like an engineering utility: compact rows, monospaced numerics,
  no decorative filler.
- UX-6 Primary columns are user-resizable (splitter), sizes persist across launches; the
  monitor column is always visible next to the settings page.
- UX-7 Destructive actions (clear log, delete preset) are never visible buttons:
  context menu entry plus confirmation dialog only.

## 5. Non-functional requirements

- NFR-1 Windows 10/11 x64 only for v1. Single self-contained build output directory.
- NFR-2 No Python at runtime. `scripts/` is reference + test fixtures only.
- NFR-3 Engine code (src/core) has no Qt GUI dependencies; it compiles against QtCore only.
- NFR-4 UI stays responsive: probing, planning and polling run off the UI thread.
- NFR-5 Cold start to interactive window under 1 second (probe/plan happen on demand).
- NFR-6 Golden tests: planner output must match the Python reference for the fixture set
  before the engine port is declared done.
- NFR-7 All project rules in `01-ARCHITECTURE_AND_STYLE_GUIDE.md` are binding: file size
  limit, DRY, SRP, comment policy, token-only theming.

## 6. Out of scope for v1 (deferred ideas backlog)

- Drag-and-drop a GGUF onto the window; recent-models library with metadata cards
  (candidates for v1.1).
- Chat/testing console, model downloads, GGUF conversion.
- Light theme (token system must make it possible; not delivered).
- macOS/Linux ports, NVML instead of nvidia-smi polling, system-wide dark mode sync,
  localization, plugin system.

## 7. Migration and compatibility

- Existing `launcher_settings.json` is read on first launch and rewritten in the new
  (superset-compatible) form; unknown keys are preserved.
- `moe_usage/` files, hot-file cache layout and naming are unchanged.
- `third_party/` and `scripts/` are not modified by the port (scripts may gain a golden-
  fixture export helper only).
- `start.bat` is replaced by `start.cmd` pointing at the built executable; the venv stays
  for tooling (Qt install, fixture generation, builds).

## 8. Success criteria for v1

1. Start the same GLM/Qwen models that the Tkinter launcher runs today, same speed and
   placement, no manual reconfiguration after settings migration.
2. The plan is legible at a glance in the strata visualization while loading.
3. Server can be stopped, and the app can live in the tray while serving.
4. Configurations can be saved as named presets and restored.
5. Engine unit + golden tests pass; all source files respect the 250-line limit.
