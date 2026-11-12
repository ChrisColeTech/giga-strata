# 04 — UI Design (owner-approved)

Decisions made with the owner on 2026-11-10, screen by screen. Where this document and
`01` section 6.5-6.8 or `00` (layout row, UX-6) disagree, this document wins.

Family pattern: child windows, forms, two-step destructive actions and overlays follow the
giga family as written in `D:\Projects\giga-stream\docs\01-ARCHITECTURE-AND-STYLE-GUIDE.md`
(Forms, Two-step, Overlays), `02-FRONTEND-SPEC.md` section 5 and `06-PAGES-AND-PANES.md`
section 6.1. Read those before building any window.

## 1. Screens

| Screen | Kind | Job |
|---|---|---|
| Models | rail screen | the model library: every model found in the model folders |
| Serving | rail screen | pick the model you are working with, run and control the server, tune that model's parameters and reload, monitor it; log in a bottom drawer |
| Settings | child window | app-wide preferences |
| Model config | child window | one model's saved parameters, opened from the Models table |

There is no Presets screen, no Plan screen and no About screen. Presets are replaced by
per-model saved parameters (section 2.3); About is a section of the Settings window.

## 2. Models screen

### 2.1 Layout

No toolbar. The table fills the screen.

- Columns: Name, Type (architecture), Quant, Size, Experts, Files (`1` or `4 shards`),
  Modified. Every header sorts on click (again to reverse). Every column resizes by dragging
  the edge at its right; the widths are remembered, and Name fills the free width until it is
  dragged (it then shrinks first when the window narrows, so the row buttons stay in view).
  The heading's right-click menu also offers Reset column widths.
- Header filters, spreadsheet style: Name (text match), Type, Quant, Files (tick the values to
  show). A filtered header shows a filled filter glyph; each filter popup has Clear; a
  right-click on any header offers Clear all filters. There is no separate search box.
- No status badges in the table. Serving state appears only in the status bar.
- With no model folder configured, the empty table holds one button, Add a model folder,
  which opens the Settings window on its Models section.

### 2.2 Row actions

Each row ends with:

- Serve: loads this model and serves it. If another model is serving, it is stopped and this
  one started without asking.
- Gear: opens this model's Model config window (section 4).
- More (⋯): a menu with Copy identifier (the model name API clients send; the server runs with
  `--alias <model stem>`), Reveal in Explorer, Edit config.json (opens the model's config file
  in the default editor) and Delete. A right-click anywhere on the row opens the same menu at
  the pointer.

Delete removes the model's files (every shard) and its config.json. It is the family two-step
(`ArmedButton` semantics): the first press turns the row into `Delete <model>? (<n> files)`,
a second press within 4 s deletes, Escape, focus leaving or the timeout disarms. No
confirmation dialog.

### 2.3 What feeds the table

- Models come only from the model folders set in Settings. There is no way to add a single
  file.
- Split sets are one row. MTP drafts and vision projectors are attached to their model (found
  as `find_mtp_draft` / `find_mmproj` do), never listed as models.
- Each model's parameters are saved in its own config file,
  `%APPDATA%\giga-strata\models\<model stem>.json`. A model with no file starts from the
  automatic defaults. Edit config.json opens this file.

## 3. Settings window

A child window (family frame: drawn inside the app, own border, scrim, header with title and
X, movable, resizable, opens centred at content size, at least 658 x 412, at most 80% of
the window, Escape closes, one window at a time). Opened from the status bar's Settings button
or `Ctrl+,`. A rail of sections on the left of its form:

| Section | Rows |
|---|---|
| General | Appearance: reduce motion (follow Windows / on). Window: keep serving in the tray when the window is closed; minimize to tray |
| Models | the model folders to scan (add, remove) |
| Server | Use a different llama-server (off = the bundled one, section 6; on = a file picker). GPU headroom: GB of VRAM every model leaves free on each GPU, one field per GPU |
| About | version, the bundled server's fork commit, licences |

The window ends in the same footer as a model's parameters: Saved or Unsaved changes, Discard
and Save. Edits preview at once but reach the settings file only on Save; Discard, or closing
the window, puts them back. Each section is the family form: groups that fold, rows of a label
and a control.

## 4. Model config window

A child window with the family frame, titled with the model's name.

- An info line: type, layers, experts, size, files, MTP draft and vision projector found.
- The memory split: GPU0..GPUn, RAM and SSD bars for the current values, updating as they
  change (live re-plan, `01` section 2).
- Foldable groups: Main (GPUs, headroom per GPU, context, MTP, vision and its projector; an
  empty headroom field uses the Settings value), Expert usage (record,
  per-expert placement with the recorded token count), Advanced (margin, micro-batch, SSD
  streaming, flash attention, extra args), with Restore defaults at the end of Advanced.
- A footer: Saved or Unsaved changes, Discard (back to the saved config file) and Save. No
  Cancel (the X and Escape close).

## 5. Serving screen

Content:

- A model picker for the model you are working with (the library's models).
- Start and Stop as real buttons beside the picker.
- A control panel that controls the server: network (serve on the local network or this PC
  only, port, the URL with copy) and the live readouts (state, load progress, tok/s, uptime,
  VRAM per GPU, RAM).
- A parameters panel for the selected model (the same groups as section 4) with Reload model,
  which applies the change and saves it to the model's config. It collapses to nothing with a
  toggle and starts collapsed.
- The log in a bottom drawer (filter and Copy in the drawer header; Clear in the right-click
  menu), opened and closed from the status bar.

Layout (approved 2026-11-10), top to bottom, full width:

1. Controls row: the model picker (`▼ <model>`), Start, Stop, Reload, and at the right end a
   Parameters toggle.
2. State and progress: the server state in words (Starting, Loading, Serving, Stopped,
   Failed with its reason) and a full-width progress bar while loading.
3. Monitor tiles, one row: Speed (generation tok/s, prompt tok/s under it), Uptime, one tile
   per GPU (VRAM bar and `used/total GB`), RAM (in use). Bound to the same controller
   properties as the status bar, so both always show the same values.
4. Network: serve on local network (on = `0.0.0.0`, off = `127.0.0.1`), port, the URL with
   Copy.
5. Log drawer at the bottom: filter (All / Server / Planner / Warnings) and Copy in its
   header, Clear in the right-click menu; opened and closed from the status bar or its own
   chevron, and resizable by dragging its top edge.

The Parameters toggle slides a panel in from the right edge, over the screen's right part (the
monitor tiles shrink to fit); it is resizable by dragging its left edge, closed by the toggle
or its X, and starts closed. It holds the selected model's groups (Main, Expert usage,
Advanced) and one Save & Reload at the end, which saves to the model's config file and
restarts the server with it.

Rejected along the way: a persistent half-screen monitor column; status and gauges on a
settings page; Run/Stop in the status bar; four side-by-side panels; a control panel that
mixes a form and a dashboard; a split panel with no way to collapse it.

## 6. The bundled llama-server

The app ships its own llama-server, built from the owner's fork, and runs nothing else unless
the Settings override (section 3) is on.

- Source: `third_party\llama.cpp-upstream`, branch `giga-strata` (upstream llama.cpp plus the
  owner's commits: per-expert hot/cold `--moe-hot`, expert usage recording, Qwen kernel
  fixes, CUDA fusion and scheduling). It is the only build with every flag the app uses;
  `llama.cpp-glm5` and the LM Studio builds lack `--moe-hot`.
- Shipped files, in `server\` beside `giga-strata.exe`: `llama-server.exe` (statically
  linked, 73 MB), `cublas64_13.dll`, `cublasLt64_13.dll`, `cudart64_13.dll` (CUDA 13
  redistributables, 508 MB), `vcomp140.dll` and the MSVC runtime, and `BUILD_INFO.txt` with
  the fork commit. Dependencies verified with `dumpbin /dependents`.
- The branch exists only locally, so it is built from that checkout by
  `scripts\build-server.bat` (run when the fork changes, low parallelism per `03` section 7),
  not pinned as a submodule. If the fork is pushed to a remote it becomes a submodule.
- Requirements: an NVIDIA driver with CUDA 13 support. The current build targets sm_120
  (RTX 50 series); other GPUs need more architectures and a longer build.
- Removed with it: automatic per-model build selection, the CPU trial-load probe and the
  LM Studio / PATH scan (`00` FR-1.2). Any model that only loads in `llama.cpp-glm5`
  (the old launcher's note on GLM REAP files) needs that fix merged into `giga-strata` first.

## 7. Desktop density and the menu bar (owner, 2026-11-10)

Supersedes the card and form-group presentation in sections 3, 4 and 5 where they differ. The
owner's verdict on the first build: separate stat cards and large form headers are a web
pattern, "dated" and "a waste of space"; on desktop the UI is compact and dense.

### 7.1 Menu bar

A standard menu bar across the top of the window, under the native title bar:

| Menu | Items |
|---|---|
| File | Model folders… (Settings on its Models section), Settings… (`Ctrl+,`), separator, Exit (`Ctrl+Q`; stops the server like Quit in the tray) |
| Help | Documentation (the README on GitHub), GitHub repository, Report an issue (the repo's new-issue page), separator, About Giga Strata |

The repository is `https://github.com/ChrisColeTech/giga-strata`. About moves out of Settings:
Help > About Giga Strata opens a small child window (version, the bundled server's fork commit
from `server\BUILD_INFO.txt`, licences, the repository link). Settings opens from the status
bar's Settings button, File and `Ctrl+,`. Menus are
themed (surface colors, mono shortcuts right-aligned), keyboard reachable with `Alt`.

### 7.2 Serving screen, compact

```
[▼ model]  [▶ Start] [■ Stop] [⟳ Reload]  ● Serving             [≡ Parameters]
───────────────────────────────────────────────────────────────────────────────
Speed   19.0 tok/s · prompt 12.0         GPU0  ██████████████░  31.2 / 31.8 GB
Uptime  00:01:44                         GPU1  ██████████████░  31.3 / 31.8 GB
URL     http://192.168.0.9:1234  ⧉       RAM   █████████░░░░░░  94.4 GB used
───────────────────────────────────────────────────────────────────────────────
☑ Serve on local network      Port [ 1234 ]
───────────────────────────────────────────────────────────────────────────────
LOG  [All|Server|Planner|Warnings]                                       Copy
…log lines fill the rest…
```

- The state word (and the load progress bar while loading) sits in the controls row.
- No cards: one readout block, labels and mono values on the left, thin inline bars on the
  right; same values as the status bar.
- Network is one row with no group header.
- The log takes the remaining height (resizable top edge, closable from the status bar).

### 7.3 Parameters panel and property grid

```
PARAMETERS ── <model> ─────────────── ✕
Memory
 GPU0 ███████████████░  27.0 / 30.9 GB
 GPU1 ████████████████  28.7 / 31.4 GB
 RAM  ██████████░░░░░░  59.2 GB
────────────────────────────────────────
MAIN
 GPUs          ☑ GPU0 5090  ☑ GPU1 5090
 Context       8K 32K 64K 128K [200K] …
 MTP           [on ]  sidecar found
 Image input   [off]
EXPERT USAGE
 Record        [off]
 Per-expert    [off]  no usage recorded
▸ ADVANCED     (margin, micro-batch, SSD stream + flash attn on one row, extra args)
────────────────────────────────────────
Unsaved changes          [ Save & Reload ]
```

- A property grid: one ~28 px row per setting, label left, control right, a short status word
  after the control (reasons stay in tooltips). Small-caps section labels replace the 48 px
  icon tiles and 28 px titles. GPUs are compact checkboxes on one line.
- Memory bars first, live. Footer pinned at the bottom: `Unsaved changes` / `Saved` and the one
  button. Default width about 420 px, still resizable.
- The Model config window uses the same grid (footer button `Save`). The Settings window uses
  the same grid style for its sections (General, Models, Server); its section rail stays.
