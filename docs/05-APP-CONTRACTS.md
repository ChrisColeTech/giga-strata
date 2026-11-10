# 05 — App Layer Contracts (controllers the screens bind to)

The QML-facing API of every `src/app` controller for the `04` design. The screens (QML) and
the controllers (C++) are built in parallel against this document; changing a name here is a
contract change made by the integrator, never silently by either side. All controllers follow
R9 (`QML_ELEMENT` + `QML_SINGLETON`, `provide()` / `create()`, built in `main.cpp`). Sizes are
`double` bytes in QML (JS numbers), times are seconds.

Existing controllers stay as they are: `SystemInfo`, `MonitorController` (`gpus`,
`freeRamBytes`, `totalRamBytes`, `hasRam`, `gpuError`, `isPaused`), `TrayHost`,
`SingleInstance`. `SettingsStore` keeps reading and writing `launcher_settings.json` for the
app-wide keys only (section 6); per-model keys move to `ModelConfigStore`.

## 1. ModelLibrary — the Models table (`04` section 2)

| Member | Kind | Meaning |
|---|---|---|
| `tableModel` | property, `QAbstractItemModel*` (a sort/filter proxy) | rows = models; roles below |
| `folders` | property, `QStringList` | the model folders (saved in SettingsStore `model_folders`) |
| `isScanning` | property, bool | a scan is running (worker thread) |
| `modelCount` | property, int | rows before filtering |
| `addFolder(QUrl)`, `removeFolder(QString)` | invokable | edit folders, then rescan |
| `rescan()` | invokable | rescan all folders |
| `sortBy(QString role, bool isAscending)` | invokable | header click |
| `filterValues(QString role)` | invokable, `QStringList` | distinct values for a tick filter (type, quant, files) |
| `setFilter(QString role, QVariant value)` | invokable | text for `name`, a `QStringList` of ticked values for the others; empty clears that column |
| `clearFilters()` | invokable | all columns |
| `activeFilters` | property, `QStringList` | roles with a filter, for the filled header glyph |
| `revealInExplorer(QString path)` | invokable | `explorer /select,` the first shard |
| `openConfigFile(QString path)` | invokable | creates the model's config file from defaults if missing, opens it with the default editor |
| `deleteModel(QString path)` | invokable | deletes every shard and the config file; emits `modelDeleted` or `errorOccurred` |
| `errorOccurred(QString message)` | signal | user-sentence error (R19) |

Roles: `name` (model stem), `path` (first shard), `type` (arch), `quant` (from
`general.file_type` mapped to llama.cpp's ftype names, falling back to the quant token in the
file name), `sizeBytes` (all shards), `experts` (int, 0 for dense), `files` (int shards),
`modified` (QDateTime of the first shard), `hasDraft`, `hasProjector`, `hasUsage` (bools,
for the config window only, never shown as table badges).

Scan rules: every `*.gguf` under each folder, recursive; split sets collapse to one row (the
`-00001-of-` file); MTP drafts (`find_mtp_draft` rule), vision projectors (`clip` arch) and
files whose layout fails to read are not rows. Layouts are read with `LayoutReader` and cached
per model key.

## 2. ModelConfigStore — per-model parameters (`04` section 2.3)

File per model: `%APPDATA%\giga-strata\models\<model stem>.json`, keys and JSON types as the
Python launcher's settings (H12: `gpus` "0,1", `ctx` "200000", `margin` "1536", `ubatch`
"auto", `extra` "", `mmproj` "", booleans `mmap`, `fa`, `mtp`, `vision`, `record_usage`,
`per_expert`). Unknown keys preserved; atomic writes.

| Member | Kind | Meaning |
|---|---|---|
| `load(QString modelPath)` | invokable, `QVariantMap` | saved values, or defaults for a model with no file |
| `save(QString modelPath, QVariantMap values)` | invokable, bool | writes the file; false + `errorOccurred` on failure |
| `defaults()` | invokable, `QVariantMap` | gpus = every detected GPU in index order, ctx "32768", margin "1536", ubatch "auto", mmap/fa/mtp true, the rest false/empty |
| `configPath(QString modelPath)` | invokable, QString | the file path (Edit config.json) |
| `configChanged(QString modelPath)` | signal | a save or an outside edit (file watcher) |

Migration: on first start, the model in `launcher_settings.json` gets a config file seeded from
that file's values (if it has none yet).

## 3. PlannerRunner — live plan for the model being edited

| Member | Kind | Meaning |
|---|---|---|
| `modelPath` | property, QString (writable) | the model whose parameters are being edited (config window or Parameters panel) |
| `parameters` | property, `QVariantMap` | the values being edited, starts as `ModelConfigStore.load(modelPath)` |
| `setParameter(QString key, QVariant value)` | invokable | updates `parameters`, restarts the 400 ms debounce (`01` section 2) |
| `resetToDefaults()` | invokable | `parameters = defaults()` |
| `save()` | invokable, bool | `ModelConfigStore.save(modelPath, parameters)` |
| `isDirty` | property, bool | `parameters` differ from the saved file |
| `isPlanning` | property, bool | a plan is running |
| `tiers` | property, `QVariantList` | the memory split for the bars: `{label: "GPU0"/"RAM"/"SSD", tierIndex, usedBytes, capacityBytes, layerText}` in GPU order then RAM then SSD |
| `summary` | property, QString | one line (`45 layers: 22 on GPU0, 21 on GPU1, 2 in RAM`) |
| `reportLines`, `warnings` | property, QStringList | the plan's report and warnings (the log gets them on Run) |
| `errorText` | property, QString | plan failure in words, empty when planned |
| `modelInfo` | property, `QVariantMap` | type, layers, experts, size, files, draft path, projector path, recorded tokens (decode, prompt) |

Planning uses the bundled server's flag set (`detect_flags` once per binary) and the latest
MonitorController reading as the HardwareSnapshot. Per-expert placement (P5b) is part of the
plan when `per_expert` is on and usage exists.

## 4. LaunchController — the server (`04` sections 5 and 6)

| Member | Kind | Meaning |
|---|---|---|
| `state` | property, enum `State`: `Stopped`, `Starting`, `Loading`, `Serving`, `Stopping`, `Failed` | |
| `stateText` | property, QString | the state in words (`Loading 42%`, `Serving`, `Failed`) |
| `failureReason` | property, QString | plain-language reason (ServerExit, LoadError, missing-DLL hint) |
| `loadProgress` | property, int | 0-100 while Loading, -1 when not inferable (indeterminate bar) |
| `modelPath`, `modelName` | property, QString | the served (or last served) model |
| `url` | property, QString | `http://<LAN IP or 127.0.0.1>:<port>` while Serving |
| `generationTokensPerSecond`, `promptTokensPerSecond` | property, double | last reported, 0 before the first request |
| `uptimeSeconds` | property, int | ticks once a second while Serving |
| `serve(QString modelPath)` | invokable | stop whatever runs (no prompt), plan with the model's saved config, launch the bundled server |
| `stop()` | invokable | kill (D6), state Stopped |
| `reload()` | invokable | `serve(modelPath)` again with the freshly saved config |
| `copyUrl()` | invokable | clipboard |

Launch path: plan (PlannerRunner rules, Run path), write the hot file and prune (D1), usage
env (`UsageEnv::prepare`), `ServerEnv::build`, `CommandBuilder::build`, `QProcess` with
`setNativeArguments(CommandLine::join(argv without argv[0]))` and `HiddenProcess::configure`,
merged output into `LogParser` and `LogModel`. `TrayHost.isServing/hasFailed/statusText`
follow `state`.

## 5. LogModel — the log drawer

A `QAbstractListModel` singleton. Roles: `text`, `kind` (`server`, `planner`, `launcher`,
`warning`, `error` from `LogLineClassifier`), `time` (`hh:mm:ss`). Properties: `filter`
(`all`, `server`, `planner`, `warnings`), `lineCount`. Invokables: `copyAll()`,
`copyLines(QVariantList rows)`, `clear()`. Kept to the last 20 000 lines.

## 6. SettingsStore — app-wide keys

Existing keys stay readable and are preserved. New and kept app-wide keys:
`model_folders` (list of strings), `serve_on_lan` (bool; `host` becomes `0.0.0.0` when on,
`127.0.0.1` when off), `port` (string, as today), `server_override` (string, empty = bundled),
`reduce_motion` (`"system"` / `"on"`), `keep_serving_in_tray` (bool), `minimize_to_tray`
(bool), `log_drawer_height`, `parameters_panel_width`, `is_log_open`, `is_parameters_open`.
The per-model keys (`gpus`, `ctx`, `margin`, `ubatch`, `mmap`, `fa`, `mtp`, `vision`,
`mmproj`, `record_usage`, `per_expert`, `extra`) are no longer edited here (section 2).

## 7. Bundled server packaging

`scripts\build.bat` copies, after `windeployqt`: `third_party\llama.cpp-upstream\bin\bin\Release\llama-server.exe`,
`cublas64_13.dll`, `cublasLt64_13.dll`, `cudart64_13.dll` (from `%CUDA_PATH%\bin\x64` or the
CUDA 13 default), into `build\<cfg>\bin\server\`, and writes `server\BUILD_INFO.txt`
(`commit`, `branch`, `built` time). It fails the deploy step if the exe is older than the
fork's HEAD commit time or the fork checkout has uncommitted changes. `vcomp140.dll` and the
MSVC runtime come from windeployqt's compiler-runtime copy. `LaunchController` runs
`<app dir>\server\llama-server.exe` unless `server_override` is set.

## 8. As implemented (Track D, merged 75a0086)

Where the implementation settled what this contract left open, or differs from it:

- SettingsStore properties: `modelFolders`, `serveOnLan` (setting it also sets `host`;
  defaults from `host == 0.0.0.0`), `serverOverride`, `reduceMotion` (`"system"` / `"on"`),
  `keepServingInTray` (default true), `minimizeToTray`, `logDrawerHeight` (240),
  `parametersPanelWidth` (380), `isLogOpen` / `isParametersOpen` (setters `setLogOpen`,
  `setParametersOpen`). `04` wins over section 6: the Parameters panel starts closed.
- ModelLibrary: also `modelDeleted(path)`; `filterValues` returns strings (files `"1"`,
  `"2"`); Reveal and Edit config go through an injectable `ShellActions`.
- ModelConfigStore: default `gpus` is `""` when no GPU is detected.
- PlannerRunner: waits for MonitorController's first reading; `errorText` is the Python
  message verbatim (e.g. `GPU id(s) [7] not found; nvidia-smi reports [0]`); tiers use
  `label` `GPU<nvidia index>` with `tierIndex` = CUDA order, GPU capacity = free VRAM at plan
  time, RAM capacity = free RAM (0 when unknown), and an SSD tier only when the RAM tier is
  streamed (capacity 0); `summary` may end `, N split`. `modelInfo` keys: `type`, `layers`,
  `experts`, `sizeBytes`, `files`, `draftPath`, `projectorPath`, `decodeTokens`,
  `promptTokens`.
- LaunchController: takes a fresh nvidia-smi reading at Run (VRAM a just-stopped server held
  counts as free); `serve()` clears the log as the Tk launcher did; `url` is empty unless
  Serving; `attachTray(TrayHost&)` syncs the tray and its Stop item.
- LogModel: filter `planner` shows planner and launcher lines, `warnings` shows warnings and
  errors; `lineCount` counts visible rows.
- Packaging: the MSVC and OpenMP runtime are copied into `server\` from the VS redist folder
  (the child process cannot see `bin\`); a missing fork is SKIPPED, not FAILED (`03`
  section 3).
- Not yet acting on their keys at merge time: `keep_serving_in_tray`, `minimize_to_tray`,
  `reduce_motion` (assigned to Track E).
