# 01 — Architecture and Style Guide

Binding for every file in this repository. When something here is unclear, fix this
document first, then the code — never improvise silently.

## 1. Stack

| Concern | Choice |
|---|---|
| Language | C++17 |
| UI | Qt 6 (6.8 LTS) Qt Quick / QML, Qt Quick Controls 2 (Basic style, fully re-themed in QML) |
| Build | CMake 3.25+, Ninja (both already in `.venv`), MSVC 2022 (VS Build Tools installed) |
| Qt install | `aqtinstall` (pip package) into `C:\Qt` — one command: `scripts\setup.bat` (see 03) |
| Formatting | clang-format (`.clang-format` at repo root, shipped with the project) |
| Tests | Qt Test for unit tests; a golden-test executable compares engine output to Python fixtures |
| App ID | `giga-strata` (target, window class, tray id). Display name: "Giga Strata" |
| App type | `QApplication`: QtWidgets is linked only in `main.cpp` + `src/app` because `QSystemTrayIcon` requires it; `core` and `ui` stay Widgets-free |

Why QML + Controls 2 Basic: Basic gives plain, predictable primitives we restyle
completely from the theme singleton, so the look is ours, not Material's.

## 2. Layered architecture

```
+---------------------------------------------------------------+
|  src/ui      QML views. No logic beyond view state.           |
|              Reads controllers; never touches core directly.  |
+------------------------------^------------------------------+
                               | signals / properties only
+------------------------------v------------------------------+
|  src/app     Qt glue layer (C++). Controllers exposed to QML: |
|              SettingsStore, PresetManager, PlannerRunner,     |
|              LaunchController, MonitorController, TrayHost.   |
|              Owns QProcess, timers, polling threads.          |
+------------------------------^------------------------------+
                               | plain structs + Result<T>
+------------------------------v------------------------------+
|  src/core    Pure engine (C++, QtCore only, no QtGui/QtQuick):|
|              gguf/, hardware/, moe/, plan/, launch/, common/  |
+---------------------------------------------------------------+
```

Rules of the layers:

1. `core` never includes anything from `QtGui`, `QtWidgets`, `QtQuick`, `src/app`
   or `src/ui`. QtCore types (`QString`, `QVector`, `QJsonObject`) are allowed.
2. `app` never builds layouts or knows about colors; it translates core results into
   properties, models and signals for QML.
3. `ui` never computes a plan, parses GGUF, queries hardware or writes settings.
   If QML needs a decision, a controller provides it.
4. Data crossing upward is always a signal with an explicit payload. Data crossing
   downward is a controller method call. No core objects are exposed to QML.
5. Long work (probing, planning, nvidia-smi polling) runs in a worker thread
   (`QtConcurrent::run` or a `QThread` member). The UI thread never blocks > 50 ms.

### Key data flows

- Plan (live, debounced): any plan-affecting edit → `SettingsStore` emits `changed(key)` →
  `PlannerRunner` restarts a 400 ms single-shot debounce timer → on timeout it bumps a
  generation counter and runs the core planner on a worker → `planReady(PlanResult)` only
  if the result's generation is still the latest (older in-flight results are dropped,
  never rendered) → strata viz + gauges + log panel render it. Plan-affecting keys are
  declared once in `PlannerRunner` (model path, GPUs, context, margin, ubatch, mmap,
  MTP, mmproj, per-expert); host/port/extra-args edits never re-plan. Hardware free-VRAM
  is re-read on each plan run (worker), so a plan never uses a stale reading.
  Server probing (CPU trial load) is not part of live re-plan: it runs on model change
  and on Run, cached per FR-1.2.
- While the server is running, the strata view keeps showing the running plan; edits
  produce a pending plan, signalled by the hint strip "Changes apply on next run" and a
  `planPending` property, never by redrawing the live view.
- Run: status bar → `LaunchController.start(settingsJson)` → command assembled in core
  → `QProcess` in app layer → log lines parsed into `ServerState` + tok/s → pills/gauges.
  `probing` and `planning` never appear in the log — they are controller phase events;
  the pill model merges phase events with parsed server state into one state machine.
- Settings: any field edit → `SettingsStore.set(key, value)` → debounced JSON write to
  `launcher_settings.json` (same schema as today, unknown keys preserved).

## 3. Folder structure

```
giga-strata/
  CMakeLists.txt            top-level: options, Qt find_package, subdirs
  .clang-format             formatting rules everyone must follow
  start.cmd                 launches the built exe
  launcher_settings.json    live settings (app-owned, human readable)
  docs/                     design docs, one topic per numbered file
  scripts/                  Python reference engine + fixture/tooling scripts (no UI code)
  third_party/              llama.cpp builds (untouched)
  moe_usage/                recorded expert usage + hot/ cache (app data)
  assets/                   icons (SVG), fonts if needed, app icon
  src/
    CMakeLists.txt
    main.cpp                app bootstrap: paths, controllers, engine, window
    core/                   static library giga_core (QtCore only)
      CMakeLists.txt
      common/               result.h, paths.h, constants.h, json_io.h
      gguf/                 gguf_reader, model_layout, shard_finder, draft_finder, mmproj_finder
      hardware/             gpu_info (nvidia-smi), ram_info, hardware_probe
      moe/                  usage_io, hot_builder, hot_cache
      plan/                 placement_planner, plan_model, buffer_model, tuning_tables
      launch/               server_finder, flag_probe, server_probe, command_builder, log_parser
    app/                    C++ glue library giga_app
      CMakeLists.txt        static lib + QML module URI "GigaStrata.App" (QML_ELEMENT types must
                            live in a QML module target); main.cpp imports its static plugin
      settings_store.h/.cpp preset_manager.h/.cpp
      settings_file.h/.cpp  launcher_settings.json read + ordered atomic write (SettingsStore's IO)
      planner_runner.h/.cpp launch_controller.h/.cpp
      monitor_controller.h/.cpp tray_host.h/.cpp
      single_instance.h/.cpp FR-6.3 local-socket guard, keyed per install folder (QtNetwork, app only)
      system_info.h/.cpp    QML_SINGLETON: motionScale; forces the dark title bar (N1)
    ui/
      CMakeLists.txt        static lib giga_ui + qt_add_qml_module, URI "GigaStrata" (imports
                            GigaStrata.App); Lucide icons served as qrc:/qt/qml/GigaStrata/icons/
      qml/
        Theme.qml           theme singleton (see 6.9): every color/size/motion token
        Main.qml            window shell (04 section 1): rail, the active screen, status bar
        components/         one file per reusable control; the family window and form parts:
          ChildWindow.qml ChildBounds.qml WindowHeader.qml ResizeGrip.qml SettingsRail.qml FormGroup.qml FormRow.qml
          FactRow.qml Toggle.qml UsageBar.qml InfoTip.qml Workspace.qml FormActions.qml LabelColumn.qml
          CheckRow.qml ArmedMenuItem.qml EdgeSizer.qml ServerStateText.qml
          PrimaryButton.qml SecondaryButton.qml DangerButton.qml GhostButton.qml IconButton.qml
          ButtonBase.qml (shared body of the four text buttons) FocusRing.qml (C5)
          TextField.qml PathField.qml NumberField.qml SegmentedControl.qml
          SwitchRow.qml Chip.qml StatusPill.qml Overline.qml Section.qml
          Card.qml ThemedToolTip.qml ToastCenter.qml ModalDialog.qml
          ScrollPage.qml (padded scrolling column + themed scrollbar)
          FieldLabel.qml (Label role) HintStrip.qml (P1b)
          ContextMenu.qml ContextMenuItem.qml Icon.qml
        panels/             composites used by exactly one screen or window
          NavRail.qml NavRailItem.qml StatusBar.qml StatusVram.qml
          SettingsGeneral.qml SettingsModels.qml SettingsServer.qml SettingsAbout.qml
          Models table: ModelTable.qml ModelHeaderCell.qml ModelRow.qml FilterPopup.qml
          Parameters (Model config window and Parameters panel): ParameterForm.qml ParametersMain.qml
          ParametersExperts.qml ParametersAdvanced.qml GpuChips.qml MemorySplit.qml
          Serving: ServingControls.qml ModelPicker.qml MonitorTiles.qml MonitorTile.qml NetworkGroup.qml
          LogDrawer.qml ParametersPanel.qml
        pages/              the rail screens (04 section 1): ModelsPage.qml ServingPage.qml
        windows/            the child windows (04 sections 3 and 4): SettingsWindow.qml ModelConfigWindow.qml
  tests/
    CMakeLists.txt
    core/                   unit tests, one file per core module
    app/                    controller tests (SettingsStore, PresetManager, TrayHost, ...); fixtures/
                            holds a copy of the real launcher_settings.json, never the live file
    golden/
      fixtures/             model layout JSONs + settings captured from the Python engine
      expected/             expected plan JSONs
      golden_test.cpp       runs core planner over fixtures, diffs vs expected
    tools/                  test-only executables, never shipped (plan_cli: 02 section 8)
    support/                test-only helpers shared by unit and golden tests (fakes, GGUF builder)
```

Anything that does not fit this tree is a design conversation, not a new folder invented
mid-commit. New subfolders require a line added to this section first.

## 4. Build and run

```
scripts\setup.bat                # one-time: aqt installs Qt 6.8.3 msvc2022_64 into C:\Qt
scripts\build.bat [debug|release] # configure if needed + build + zero-warning gate + windeployqt
scripts\run-tests.bat [cfg]      # build, then ctest: unit + golden
start.cmd                        # runs the built exe (Release first, Debug fallback)
```

- Full reference, toolchain versions, CMake contract and lessons: `03-BUILD_ENVIRONMENT_GUIDE.md`.
- Debug and Release builds go to `build/debug` and `build/release`; neither is committed.
- CI is not required for v1; running `scripts\run-tests.bat` before declaring any task
  done is required.

## 5. Project rules

These are hard rules, written for someone new to the project. Violating a rule is a bug,
not a style preference.

### 5.1 Files and size

- R1 Every `.h`/`.cpp`/`.qml` file is at most 250 lines including blank lines. When a
  file hits the limit, split it by responsibility — do not shrink whitespace or golf code.
- R2 One class per C++ pair. Header named `snake_case.h`, source `snake_case.cpp`,
  matching the class (`class GgufReader` lives in `gguf_reader.h/.cpp`).
- R3 One component per QML file; file name is the component name in PascalCase
  (`StatusPill.qml` defines `StatusPill`).
- R4 QML components import only: QtQuick, QtQuick.Controls.Basic, QtQuick.Layouts,
  QtQuick.Shapes (arcs, rings, gauge strokes), QtQuick.Effects (glow), QtQuick.Dialogs
  (native file pickers) and project modules. No ad-hoc third imports.
- R5 No file may declare more than one thing that other files use. Helpers internal to
  a single .cpp may stay in an anonymous namespace.

### 5.2 Naming

- R6 C++ types PascalCase; functions and methods camelCase; local variables camelCase;
  member variables `m_` + camelCase; constants and enums UPPER_SNAKE_CASE inside
  `namespace` scopes or as `constexpr` at file scope.
- R7 Names say what things are, in English, no abbreviations except: gguf, mmproj, mtp,
  moe, kv, vram, gpu, tok. Never `data`, `info`, `util`, `temp`, `do Stuff`.
- R8 QML: ids lowerCamelCase; properties camelCase; signal names are past-tense events
  (`loaded`, `planReady`, `errorOccurred`); handlers `on<Signal>`.
- R9 Controllers exposed to QML use the pattern: gettable properties + invokable methods
  + typed signals. Every controller is a C++ `QML_ELEMENT` + `QML_SINGLETON` type with a
  static factory, so QML addresses it by type name (`PlannerRunner.planPending`) from any
  file, and qmllint and the QML compiler see its full type:

  ```cpp
  class PlannerRunner : public QObject {
      Q_OBJECT
      QML_ELEMENT
      QML_SINGLETON
  public:
      PlannerRunner(SettingsStore &settings, const Paths &paths, QObject *parent = nullptr);
      static void provide(PlannerRunner *instance);              // main.cpp, before QML loads
      static PlannerRunner *create(QQmlEngine *, QJSEngine *);  // QML engine calls this
  private:
      static inline PlannerRunner *s_instance = nullptr;
  };
  ```

  `main.cpp` is the composition root: it constructs every controller with its real
  dependencies (constructor injection, in dependency order), calls `provide()` on each,
  then loads `Main.qml`. `create()` asserts the instance was provided, sets
  `QJSEngine::CppOwnership` on it and returns it, so QML never constructs or deletes a
  controller. Context properties and `qmlRegisterSingletonInstance` are forbidden — they
  defeat qmllint and ahead-of-time QML compilation, which NFR-5 depends on.
  `qmllint` over `src/ui` must be clean.
- R9a QML singletons (`Theme`) are QML files with `pragma Singleton` as the first line and
  `set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)` in
  `src/ui/CMakeLists.txt` before `qt_add_qml_module`. `QML_SINGLETON` is the C++ macro
  and applies only to C++ types.

### 5.3 Comments and readability

- R10 Comments explain why, not what. If the code needs a what-comment, rename or split.
- R11 No commented-out code, no TODO comments (the requirements doc is the TODO list),
  no emoji in code or comments.
- R12 A function does one thing: if you can name two things with "and", split it.
  Prefer early returns over nested if; 3 nesting levels max in any function.
- R13 Every boolean field/property name reads as a question or state: `isRunning`,
  `autoServer`, `useMmap` — never `flag`, `mode`, `check`.

### 5.4 DRY and SRP

- R14 SRP: each class owns exactly one concept from the folder list in section 3. If a
  class needs to know two concepts, one of them belongs in another class.
- R15 DRY in logic: a calculation exists in exactly one function; everyone calls it.
  Copying a block of more than 6 lines into a second place is a rule violation.
- R16 DRY in style (QML): colors, sizes, fonts, radii, durations appear only as
  `Theme.<token>`. Literal `#hex`, `font.pointSize: 13`, and magic paddings in
  components/pages are a rule violation. Repeated layout patterns become a component.
- R17 DRY in strings: user-facing text lives in the page/component that owns the feature
  (wrapped in `qsTr`); shared vocabulary ("Stop server", "Cancel") lives in Theme only
  as tokens, not as duplicated literals.

### 5.5 Errors and data flow

- R18 Core fallible functions return `Result<T>` (`ok`, `value`, `error` message —
  see `core/common/result.h`). No exception may cross the core→app→QML boundary.
- R19 Error messages are user-sentence quality: "No llama-server build found. Turn off
  auto-pick and browse to one." Never a stack trace, never raw C++ text.
- R20 QProcess/hardware failures map to the nearest user concept in `app/` controllers,
  which emit `errorOccurred(message)`; the UI shows it as an inline error or toast.
- R21 Validation errors appear on the field that caused them (red border + inline line),
  not in a dialog. Dialogs are for destructive confirmations only.

### 5.6 State and persistence

- R22 `SettingsStore` is the only writer of `launcher_settings.json`. Debounced 800 ms.
  Atomic write (write temp file, replace). Unknown keys preserved.
- R23 Server-cache entries follow today's key format
  (`path|size|mtime`) and are invalidated when the candidate list changes.
- R24 No global mutable state anywhere. The allowed singletons are exactly: `Theme` (QML,
  read-only constants), `Paths` (core, read-only after startup), `SystemInfo` (app,
  read-only after startup), and the R9 controllers. A controller's static instance
  pointer is written once by `provide()` before QML loads and never reassigned; the
  controller's own state is instance state, not global state. Core never sees any
  singleton except `Paths`.
- R25 Presets are files in `presets/<name>.json` under the app data folder
  (`%APPDATA%\giga-strata`), full settings snapshot minus model path.

### 5.7 Threads

- R26 UI thread: QML rendering and controller signal handling only.
- R27 Worker: GGUF reads, server probes, planning, nvidia-smi polling. Results arrive via
  queued signals. Controllers own their workers and cancel them in destructors.
- R28 A poll loop must stop when: server exits, window quit-requested, or app quits.
  Poll interval for gauges is 1000 ms while serving, paused while minimized.

### 5.8 Testing

- R29 Every `core/` module has a unit test in `tests/core/` named `test_<module>.cpp`.
- R30 The planner is golden-tested: same inputs must produce the same plan args, report
  and warnings as the Python reference for every fixture. A fixture is captured with
  `scripts/export_fixtures.py` (added during the port).
- R31 Fixtures and expected outputs are committed; tests must run offline on a machine
  with no GPUs (hardware input is injected, never queried inside core).
- R32 A task is done only when the build is warning-free and `scripts\run-tests.bat`
  passes. Flags: `/W4 /WX /external:anglebrackets /external:W0` — warnings-as-errors
  applies to project code only; Qt headers (angle-bracket includes) stay at level 0.

### 5.9 Includes, formatting, git

- R33 Includes: own header first, then project headers, then Qt, then std; alphabetical
  within each group; one include per line; no include of a header you do not use.
- R34 `#pragma once` in every header. Forward-declare in headers; include in sources.
- R35 Run clang-format before committing. Braces attached, 4 spaces, 112-column limit.
  QML follows the same visual grammar: 4 spaces, attached braces, one property per line.
- R36 Commits: imperative mood, one concern each ("Add placement planner to core").
  Never commit `build/`, generated files, or settings churn unrelated to the change.

## 6. Style guide — Strata Console

The aesthetic: a dark engineering console. Quiet graphite surfaces, tier-coded accents
that literally map to the product (GPU → RAM → SSD), monospaced numerals, compact rows,
zero decoration without purpose.

### 6.1 Design principles

- P1 The visualization explains; text confirms. Never stack a paragraph where a gauge
  or pill can carry the message.
- P2 One filled (Primary) button on screen at a time outside dialogs: the Run/Stop
  button in the status bar. Everything else — including preset load and preset save —
  is Secondary or Ghost. Each modal dialog owns exactly one Primary in its own scope.
  There is no "Analyze" button: planning is live (section 2, key data flows).
- P3 Progressive disclosure: advanced fields sit under collapsed "Advanced" sections.
- P4 Consistency over cleverness: every control in this guide has exactly one approved
  style; do not invent variants.
- P5 Status is always visible: pill in the status bar, plus tray icon when hidden.

### 6.2 Color

All colors below are the complete palette, and they exist in QML only as `Theme`
tokens (6.9). `Theme.qml` is the only file allowed to contain color literals — a hex
value anywhere else is a rule violation. Tints derived from a token (alpha variants)
are also Theme tokens or Theme alpha-helper calls, never inline literals.

Surfaces and structure:

| Token | Hex | Use |
|---|---|---|
| `bg` | `#15171C` | window background, behind panes |
| `surface1` | `#1B1E25` | panels, nav rail, status bar |
| `surface2` | `#222630` | cards, section fills, segmented track |
| `surface3` | `#2A2F3B` | hover states, raised items, scrollbar thumb |
| `field` | `#101216` | text/number field background (inset) |
| `border` | `#2E3440` | all 1px borders, dividers |
| `border2` | `#3B4252` | hover borders, focus-adjacent edges |

Text (contrast to `surface2`/`field` backgrounds at or above 4.5:1 for text1/2):

| Token | Hex | Use |
|---|---|---|
| `text1` | `#E4E8F0` | primary text, field input |
| `text2` | `#9AA4B5` | labels, values, captions |
| `text3` | `#626C7E` | hints, placeholders, disabled text |
| `textOnAccent` | `#0E1116` | text/icons on filled accent buttons |
| `textOnDanger` | `#16080C` | text/icons on filled danger buttons |
| `knob` | `#F2F4F8` | switch knob and bright control handles |
| `scrim` | `rgba(0,0,0,0.5)` | modal dialog backdrop |
| `focusRing` | `rgba(79,193,168,0.55)` | focus ring (C5) |
| `glowAccent` | `rgba(79,193,168,0.20)` | the single permitted glow (6.4) |

Identity and tiers (the heart of the theme — tier colors never change meaning):

| Token | Hex | Use |
|---|---|---|
| `accent` | `#4FC1A8` | primary actions, focus rings, active nav, selection only |
| `accentSoft` | `rgba(79,193,168,0.16)` | selected/tinted backgrounds (pills, chips on) |
| `tierGpu` (ordinal) | `#7E8FEF`, `#B08AF0`, `#E57FC4`, `#45C9E0` | GPU0..GPU3 strata bands — cyan is last because it sits nearest `accent` and most rigs never show a GPU3 |
| `tierRam` | `#E0B84E` | RAM tier band |
| `tierSsd` | `#7D879C` | SSD/disk tier band — desaturated on purpose: slow tier reads cold |

Semantics:

| Token | Hex | Use |
|---|---|---|
| `success` | `#5EC97F` | serving state, recorded-usage OK |
| `warn` | `#E2804E` | low-samples warning, partial fit |
| `danger` | `#E5556A` | failed state, destructive actions, error fields |
| `info` | `#6FA8E8` | neutral notices, planning-in-progress |

Hard color rules:

- C1 Never use a tier color for an action or semantic meaning, and vice versa.
- C2 Fills at 16% alpha (`accentSoft` pattern) for "selected/active" tints; never full
  saturation backgrounds behind text.
- C3 Log colors: server lines `text2`, planner lines `accent`, warning lines `warn`,
  error/critical lines `danger`; timestamps `text3`.
- C4 Charts/bars use tier colors only. A bar with no tier meaning uses `accent`.
- C5 Focus ring: 2px `focusRing`, 2px offset, keyboard focus only.
- C6 No hex value serves two roles: action colors, semantic colors and tier colors never
  share a value (the tables above satisfy this; verify when adding tokens). A tier color
  is never used for status or action; a semantic color is never used for placement. Tier
  colors appear in strata bands, gauge bars, and GPU chip dots (S3); semantic colors
  appear in dots, text, pill outlines, tray overlays and icons.

### 6.3 Typography

| Role | Family | pt | Weight | Tracking | Color |
|---|---|---|---|---|---|
| Display | Segoe UI | 20 | DemiBold | — | text1 |
| Title | Segoe UI | 14 | DemiBold | — | text1 |
| Overline (section label) | Segoe UI | 10 | DemiBold | 0.8px, ALL CAPS | text2 |
| Body | Segoe UI | 10.5 | Normal | — | text1 |
| Label (field labels) | Segoe UI | 10.5 | SemiBold | — | text2 |
| Caption / hint / tooltip | Segoe UI | 9.5 | Normal | — | text2 / text3 |
| Numeric values, fields | Cascadia Mono → Consolas fallback | 10.5 | Normal | — | text1 |
| Log, tok/s, gauge numbers | Cascadia Mono → Consolas fallback | 9.5–10.5 | Normal | tabular | text2 |

- T1 UI font is Segoe UI; the mono stack is `"Cascadia Mono", "Consolas", monospace`.
  Mono for anything that is measured, compared or typed — numbers right-align.
- T2 Two weights per family maximum in any single view; DemiBold/SemiBold is emphasis,
  not decoration.
- T3 Sentence case everywhere except Overline labels (which are ALL CAPS by definition).
  No title-case button text: buttons say "Run server", not "Run Server".
- T4 Never use italic for UI structure; italic only inside tooltips for examples.

### 6.4 Space, shape, elevation

- Scale (4px base): `Theme.space1..6` = 4, 8, 12, 16, 24, 32. Row gap = space2,
  group gap = space4, page padding = space4.
- Radii: `radiusSm` 4 (chips, pills inner), `radiusMd` 6 (buttons, fields, cards,
  segmented), `radiusLg` 8 (dialogs, window-level panels). Pills are fully round
  (radius = height/2).
- Borders: always 1px. Default `border`, hover `border2`. No double borders.
- Elevation is color, not shadow: raised = one surface step lighter; inset (fields) =
  `field` below `bg`. A single soft glow (`glowAccent`) is allowed on the active Run
  button and focus rings — nothing else casts glow.
- Size tokens (Theme names): `controlHeight` 30, `iconSize` 16, `railWidth` 64,
  `railItemHeight` 48, `railIconSize` 20, `railCaptionSize` 9 (px), `railIndicatorWidth` 3,
  `statusBarHeight` 36, `splitHitWidth` 6, `focusRingWidth` 2, `pageMinWidth` 380,
  `monitorMinWidth` 360, `windowWidth`/`windowHeight` 1280x800, `windowMinWidth`/
  `windowMinHeight` 960x640. Type sizes: `displaySize`, `titleSize`, `overlineSize`,
  `bodySize`, `captionSize` (6.3, points), `overlineTracking`; families `uiFamily`,
  `monoFamily` (forwarded from SystemInfo: Cascadia Mono when installed, else Consolas).
  Component sizes (6.5 values as tokens): `buttonPadding` 16, `chipHeight` 26,
  `segmentItemHeight` 28, `switchWidth`/`switchHeight` 36x20, `switchKnobSize` 14,
  `menuItemHeight` 28, `menuMinWidth` 200 (S5 panel), `dotSize` 6, `barHeight` 4 (P1e), `gaugeRowHeight` 24, `gaugeLabelWidth` 120 (M2 name and value columns), `scrollBarWidth` 6,
  `dialogWidth` 420, `presetCardWidth` 220, `disabledOpacity` 0.35 (B2), `toolTipDelay` 500 ms,
  `toastDuration` 6000 ms, `toastStackLimit` 3. State tints (6.5 Primary/Danger hover and press):
  `accentHover`/`dangerHover` = 8% white tint, `accentPressed`/`dangerPressed` = 8% black tint.
  Shared strings (R17): `textCancel`, `textBrowse`, `textUnavailable` (a disabled control's reason
  while the controller it needs has not landed), `repositoryUrl` (the GitHub repository, for Help
  and About).
  Child windows, ported from the giga family kit (`D:\Projects\giga-stream\src\shared\ui\src`,
  `lib/child-metrics.ts`, `windows/SettingsRail.tsx`, `lib/label-column.ts`): `childOpenShare` 0.8,
  `childGrip` 5, `childBorder` 1, `childTitleHeight` 28, `childHeaderGap` 6, `childInset` 8,
  `workWidth`/`workBody` 1200/840 and `workLeastWidth`/`workLeastBody` 640/360 (the kit's workSpec),
  `captionIconSize` 15, `settingsRailWidth` 150 and `settingsRailWideWidth` 210 from a
  `settingsRailWideAt` 1024 main window, `settingsRailPadding` 6, `railRowHeight` 34, `railEdgeWidth` 2,
  `labelCharWidth` 8 / `labelColumnMin` 120 / `labelColumnGlyph` 36 (the label column formula),
  `accentTint`/`accentEdge` (accent at 12% / 40%: a lit icon toggle, the family `data-on`). A child
  window casts the family menu shadow, the one shadow in the app: `shadowMenu` (black 28%),
  `shadowMenuOffset` 8, `shadowMenuBlur` 0.5. Caption glyphs are the VS Code chrome set
  (`chrome-maximize`, `chrome-restore`, `chrome-close` from @vscode/codicons 0.0.36, CC BY 4.0), the
  one sanctioned non-Lucide exception. Also `resizeGripSize` 6 (a drawer or panel's drag edge),
  `armTimeout` 4000 ms (the two-step's window), `statusMeterWidth` 56 (a status bar VRAM bar).
  Motion: `indeterminateCycle` 1800 ms (one sweep of an indeterminate progress bar; never runs under
  reduced motion), `childEnter` 200 ms with `childRise` 10 / `childSink` 6 px (a child window's
  fade-and-rise; instant under reduced motion).
  The family form kit (`form/Group.tsx`, `Row.tsx`, `Workspace.tsx`, `Actions.tsx`, `InfoTip.tsx`,
  `form-classes.ts`), its Tailwind values mapped: `workspace` #0F1115 (the kit's `surface`, the
  form stage below `bg`), `accentText` #84D4C2 (accent mixed 70% with white), `tileEdge` (accent at
  16%: `accent-edge/40`), `strokeHighlight` (white 5%: `shadow-stroke-hi`), `splitterGlow` (accent
  at 8%) with `splitterOpacity` 0.6 and `splitterGlowRadius` 14, `rowInactive` (text3 at 40%: the
  inactive state bar); px: `workspacePadding` 16, `workspaceBottom` 8 (compacted for the property grid, `04` section 7.3), `groupTopPadding` 20,
  `groupTileSize` 48, `groupTileRadius` 12, `groupTileIconSize` 20, `groupTitlePixelSize` 28 with
  `groupTitleLineHeight` 36 (mono semibold), `groupChevronSize` 32, `splitterGap` 20,
  `labelPixelSize` 14 (Body Strong label), `notePixelSize` 12, `infoTipSize` 20, `rowGap` 6,
  `rowAccentWidth` 4, `fieldHeight` 36 (every text and number field); motion `groupFold` 200 ms.
  Kit colour names map as: text -> text1, muted/faint -> text2, dim -> text3, panel -> surface1,
  ctl -> surface2, hair/border -> border, st-done -> success, st-failed -> danger, sw-amber -> warn.
  The Models table and the Serving screen (`04` sections 2 and 5): `tableRowHeight` 36 and
  `tableHeaderHeight` 29 (the family record table), column widths `columnNameMinWidth` 200,
  `columnTypeWidth` 120, `columnQuantWidth` 96, `columnSizeWidth` 96, `columnExpertsWidth` 80,
  `columnFilesWidth` 96, `columnModifiedWidth` 144, `columnActionsWidth` 168; `filterPopupWidth` 240;
  `modelPickerWidth` 320 shrinking to `modelPickerMinWidth` 140;
  `logDrawerMinHeight` 96, `logLineHeight` 18; `parametersPanelMinWidth` 400 (the grid's widest row, Context, still fits) (the panel opens at
  SettingsStore's 420 and resizes from its left edge).
  Desktop density (`04` section 7): `menuBarHeight` 28 (the menu bar and its items), `aboutBody` 300
  (the About window's body; its width is `dialogWidth`), the property grid's `propertyRowHeight` 28,
  `propertyControlHeight` 24 (fields and segmented items inside a grid row), `propertyLabelWidth` 104
  and `propertyLabelWideWidth` 200 (the label column; the wide one for Settings' longer labels),
  `readoutLabelWidth` 56 (the label column of the Serving readout and memory bars).
- Density: control height 30 (fields, buttons, segmented items), icon buttons 30x30,
  switches 36x20. Text never wraps in a control row; elide with ellipsis and show full
  text in a tooltip.

### 6.5 Components (the only approved variants)

Buttons:

| Type | Shape | Fill | Text | Behavior |
|---|---|---|---|---|
| Primary | radiusMd, h30, pad 16 | accent | textOnAccent, SemiBold | hover accent+8% white, press darken 8% |
| Secondary | radiusMd, h30 | surface3 + border | text1 | hover surface2 fill + border2 |
| Danger | radiusMd, h30 | danger | textOnDanger | same hover/press pattern as Primary |
| Ghost | radiusMd, h30 transparent | — | text2 | hover surface1 fill, text1 |
| Icon | 30x30 radiusMd | — | icon only, 16px | hover surface2, tooltip required; lit toggle = accentTint fill, accentEdge border, accent glyph |

- B1 Run button = Primary "Run server" with play glyph; while running it is Danger
  "Stop server". It sits at the right end of the status bar and is the only filled
  button outside dialogs (P2).
- B2 Disabled = 35% opacity, no hover effect, tooltip explains why when non-obvious.
- B3 Buttons never contain emoji or decorative glyphs; icon+text is allowed with 8px gap.

Fields (TextField, NumberField, PathField):

- F1 h30, radiusMd, fill `field`, 1px `border`; hover `border2`; focus: `accent` border
  + focus ring (C5); typing replaces placeholder immediately.
- F2 Label above (Label role), never inside, never beside for long names. Optional
  inline error line (Caption, `danger`) 4px below; the field border goes `danger` too.
- F3 PathField elides the middle of the path, shows file name first
  (`GLM-5.3-…-q4kattn.gguf`), tooltip holds the full path, trailing folder IconButton.
- F4 Placeholders are examples, not labels: `"8080"`, `"auto"`, `"extra flags…"`.
- F5 NumberField validates on commit (Enter/focus-out), not per keystroke; invalid →
  F2 error state with the rule sentence from R19.

Choice controls:

- S1 On/off settings use SwitchRow: label left (Label role), switch right (36x20),
  track off = surface3 + border, on = accent, knob = `knob` token, slide 150ms.
  No classic checkboxes.
- S2 Enumerated small sets use SegmentedControl: container surface2 radiusMd pad 2,
  items h28; selected item = surface3 fill radiusSm + text1; unselected Ghost text2.
  Sliding indicator animates 180ms. Used for context size and the LogView filter.
- S3 GPU selection uses Chips (toggle): h26 pill, off = surface2 + border text2,
  on = accentSoft fill + accent text + 1px accent border; chip shows
  `GPU0 · RTX 5090 · 21/24 GiB free` (mono numerics), tier color dot of its tier.
- S4 No dropdown menus or combo boxes anywhere. Menus exist in exactly two forms: the
  native tray menu (QMenu, owned by `TrayHost`) and right-click context menus inside the
  window (S5). Nothing opens a menu from a left click.
- S5 ContextMenu (`ContextMenu.qml` + `ContextMenuItem.qml`, built on Controls.Basic
  `Menu`/`MenuItem`): opens on right click or the Menu key at the pointer/focus item;
  panel surface3 + 1px border2 + radiusMd, pad 4, no shadow (6.4); items h28, pad
  space3, Body text text1, optional 16px icon in text2, shortcut hint right-aligned in
  Caption text3; hover/keyboard-current = surface2 fill radiusSm; destructive items use
  `danger` text and always open a D2 confirmation; separators 1px border with space1
  vertical margin. Open/close fades via `Theme.fast`. Esc closes; arrows move; Enter
  activates. A context menu holds only actions on the thing under the pointer.

Status and feedback:

- P1a StatusPill: fully round, h22, 16%-alpha semantic color fill + same-color 1px
  border + 6px dot + Caption text. States: `stopped` text3, `probing`/`planning` info,
  `loading` info + progress ring, `serving` success, `failed` danger, `exiting` warn.
- P1b Inline hint strip (one line, Caption text3, info icon optional) replaces all
  former helper-text blocks. Max one hint strip per section; details go to tooltip.
- P1c Toast: bottom-center, pill h30, surface3 + border, text1, icon in semantic
  color, auto-dismiss 6s, stack to 3, click to dismiss. Error toasts carry a Ghost
  "Details…" that scrolls the monitor column to the LogView card and highlights the
  matching line.
- P1d Spinner: 14px arc, accent, 900ms rotation; only for inline waits (probing).
- P1e Progress: 4px track surface3, fill accent radius-pill; used in loading pill and
  gauge mini-bars.

Cards and sections:

- K1 A Section = Overline + content column (gap space2). Sections are separated by
  space4, never by boxes inside a page.
- K2 A Card = surface2 + border + radiusMd + pad space3(12); only for the monitor
  column widgets (gauges, strata viz, server card, log), preset cards on the Presets
  page (each card is one selectable object, not a grouping box), and dialogs' inner
  blocks. Settings and About are flat sections (no cards).
- K3 Advanced groups: Section with an inline Ghost toggle "Advanced" chevron; collapsed
  by default; expansion animates height `normal` (180ms); contents indented space3.

Dialogs:

- D1 Modal, scrim `scrim`, panel surface2 + border + radiusLg + pad 16,
  width 420 (560 for file pickers), title Title role, body Body role, gap 12.
- D2 Button row bottom-right: Secondary left of Primary/Danger, 8px gap, no text
  buttons. Destructive confirmations name the action on the button ("Delete preset").
- D3 File pickers use native Windows dialogs (Qt FileDialog), not custom ones.
- D4 Esc = cancel, Enter = primary action, focus starts on primary (or first field when
  the dialog collects input).

Nav rail, status bar, window:

> Superseded by `04-UI-DESIGN.md` (owner, 2026-11-10) for screens, layout, navigation, the
> status bar and windows. Kept until rewritten; where it disagrees with `04`, `04` wins.

- N1 Window 1280x800 default, 960x640 minimum, native title bar forced dark regardless
  of the system theme: `SystemInfo` sets `DWMWA_USE_IMMERSIVE_DARK_MODE` on the window
  handle at startup. Following the system was verified wrong: in light mode it puts a
  white title bar over the graphite UI (03 section 7).
- N2 NavRail 64px wide, surface1, right border. Items 48px: 20px icon centered +
  9px caption below (text3). Active: icon text1 + accent, 3px accent bar flush left
  with 8px rounded ends, fade 180ms. Order: Settings, Presets, About. The rail swaps
  the left page column only; the monitor column is persistent (N5, 6.8).
- N3 StatusBar h36, surface1 + top border: server pill + URL + tok/s left, plan summary
  center (elided; shows a `warn` dot while `planPending`), Run/Stop right.
- N4 Tray icon: base icon with a success dot overlay while serving, danger dot when
  failed, no dot when stopped; tooltip shows state and URL. Dot color follows pill-state
  semantics — never red for a healthy server.
- N5 The page column / monitor column split is a `SplitView` (resizable per UX-6):
  minimum widths 380 / 360, ratio persisted in the settings key `uiSplitRatio` and
  restored at startup. The handle is a themed delegate: 1px `border` line in a 6px hit
  area, `border2` on hover, `accent` while dragged (the Basic default is a light gray
  bar that breaks the palette — 03 section 7).

Specialized monitor widgets:

- M1 StrataViz: one horizontal band per tier, ordered GPU0 → GPUn → RAM → SSD, band
  color = tier color; within a band, layer blocks proportional to GiB; block labels mono
  9.5 when block ≥ 56px wide else tooltip-only; hovered block shows layer range, count,
  GiB, hit-rate (when per-expert) in a tooltip; gaps between tiers = 4px `bg`.
  A thin underlay shows total model size vs tier capacities so "what overflowed" reads
  instantly. Animate layout changes 300ms.
- M2 GaugePanel: one row per GPU (chips' tier color dot + name + mono `used/total GiB`
  + 4px bar in tier color + free-value). RAM row uses tierRam. Rows h24.
- M3 LogView: mono 9.5, line h18, level colors per C3, zebra none; the header row holds
  only the filter segmented (all/server/planner/warnings); copy all, copy selection and
  clear exist only in the right-click context menu (UX-7: no visible destructive
  buttons). Auto-scroll sticks unless scrolled up, then shows a floating "Jump to
  latest" pill (affordance, not destructive, so it stays visible).
- M4 ServerCard: model stem (Title), URL as clickable mono text (copies on click,
  toast confirms), uptime + tok/s as mono Display numerals. Preset switching happens
  on the Presets page only — no menu or chip here (S4).

Desktop density (`04` section 7, supersedes the family form kit's Group/Row/InfoTip/Actions,
the K2 card and the S3 chip, which are removed):

- D1 PropertyRow: one `propertyRowHeight` row: label in text2 in a `propertyLabelWidth`
  column (its tooltip explains the setting, or says why the row is disabled), the control
  at `propertyControlHeight`, then a short status word in Caption text3.
- D2 PropertySection: an Overline (small caps) section label on a row over its
  PropertyRows; a collapsible one leads with a chevron (ADVANCED).
- D3 MeterRow: a PropertyRow-height memory meter: name, 4px tier bar, mono numbers
  right-aligned. The Serving readout and the Parameters memory bars use it.
- D4 HairLine: the 1px `border` rule between the blocks of a dense surface.
- D5 Menu bar: Basic.MenuBar on surface1, `menuBarHeight` items, S5 menus with
  MenuDivider separators and mono shortcut hints.

### 6.6 Motion

- Motion tokens: `Theme.fast` 120ms, `Theme.normal` 180ms, `Theme.slow` 300ms,
  `Theme.switchSlide` 150ms (S1 knob), easing `OutCubic`. The K3 expansion uses `normal`. Only animate: opacity, position, size, color, gauge/strata fills.
- Every interactive state change (hover, focus, selection, pill state) transitions via
  `fast`; page swaps via `normal`; nothing bounces, nothing loops except Spinner.
- Reduced-motion: durations are multiplied by `Theme.motionScale`, which forwards
  `SystemInfo.motionScale` (6.9); with reduced motion the multiplier is 0, so every
  animation collapses to an instant state change.

### 6.7 Icons and imagery

- Icon set: Lucide (ISC, license in `assets/icons/LICENSE`), 16px grid, stroke 1.6px, SVG files in `assets/icons/`,
  imported once via a QML component `Icon.qml` (`name`, `size`, `color: Theme.…`).
  Icons inherit current text color; never hardcode an icon fill.
- No emoji, ever (UI, docs-in-code, log rendering, tray tooltip). No raster art, no
  illustrations, no mascots. Productivity iconography only: play, stop, folder, cpu,
  hard-drive, database (RAM), gauge, preset/bookmark, info, warning, check, x.

### 6.8 Information architecture (what goes where)

> Superseded by `04-UI-DESIGN.md` (owner, 2026-11-10) for screens, layout, navigation, the
> status bar and windows. Kept until rewritten; where it disagrees with `04`, `04` wins.

- The window is persistently three-pane (N2, N5): nav rail | left page column | live
  monitor column. The monitor column never swaps pages: StrataViz card, GaugePanel
  card, ServerCard, LogView card stacked top to bottom, the column scrolls. Run lives in
  the status bar. Planning is live: every plan-affecting edit re-plans after a 400 ms
  debounce (section 2), so the strata view follows context/GPU changes while the
  settings page stays editable.
- Settings page (default left page), sections in order: Model (path, metadata strip,
  vision/mmproj, MTP), Server binary (auto toggle, path when manual), Hardware (GPU
  chips, context segmented, margin, ubatch, Advanced: mmap/flash-attn/extra args),
  Expert usage (record toggle, per-expert toggle + recorded-usage hint strip),
  Network (host, port).
- Presets page (left column): grid of preset cards (name, model stem, ctx, GPU summary)
  each with a Secondary "Load" button; "Delete preset" exists only in the card's
  right-click menu with a D2 confirm dialog. The page header holds "Save current as
  preset" as a Secondary button (P2: no second filled button).
- About page (left column): version, engine parity notes, links. Flat text, no marketing.

### 6.9 Theme singleton (implementation contract)

`src/ui/qml/Theme.qml` is a QML singleton (`pragma Singleton`, R9a), the only file allowed to define
colors, fonts, sizes and durations. Components use it as `Theme.accent` etc. Its API
mirrors this document exactly: every token named in 6.2–6.6 exists with the same name,
and it also holds shared strings vocabulary (R17, section 5.4). Adding a token requires
adding it to this document first. Theme contains zero logic: only constants and trivial
alpha helpers, plus the pass-through `motionScale` described below.

`SystemInfo` is a `QML_SINGLETON` C++ type in `src/app` with read-only properties:
`motionScale` (1.0 normally, 0.0 when the OS reports reduced motion — read once at
startup from the Win32 `SPI_GETCLIENTAREAANIMATION` system parameter, because Qt
exposes no reduced-motion API). It also applies the forced dark title bar (N1) from
C++; QML never touches that. `SystemInfo` follows the R9 provide/create pattern.
Nothing in QML reaches into SystemInfo except Theme's `motionScale` forwarding.

### 6.10 Accessibility and localization

- A1 Text-to-background contrast at or above 4.5:1 (tokens are chosen for this; when
  adding tints, verify against `field` and `surface2`).
- A2 Tab order follows visual order; every control has a visible focus ring (C5);
  switches expose Accessible.name from their label; icon buttons require tooltips.
- A3 All user-facing strings wrapped in `qsTr()` from day one (R17); no concatenated
  sentence fragments that break translation.
- A4 Pointer targets at least 26px tall (chips 26, rows 24+ with padding); mouse
  hover states never the only affordance.

## 7. Definition of done (check before any task is "finished")

1. `scripts\run-tests.bat` passes (it builds with the zero-warning gate first).
2. Every new/changed file respects: 250-line cap, naming, one-class/one-component,
   token-only theming, no what-comments, Result-based errors.
3. New UI uses only approved components (6.5) — if something is missing, it is added to
   this guide first, then built.
4. New core code has a unit test; planner-affecting changes pass golden tests.
5. The requirements doc reflects any scope added or deferred while doing the work.
6. UI increments are verified visible-first: run the app, capture a screenshot of the
   real running window showing the change, and present it before marking the task done.
   A mockup or expected-image does not count.
