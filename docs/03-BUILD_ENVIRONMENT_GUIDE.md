# 03 — Build Environment Guide

How to go from a fresh checkout to a running, tested `giga-strata.exe`. Every command
here has been run on this machine; the verification record is in section 9. When a
script changes, update this document in the same commit.

Lineage: the script pattern (one `.bat` per step, `vcvars64` in a child `cmd`, every step
logs to `logs\`, one SUCCESS/FAILED line on the console, `ninja -t query` target check,
portable project-root detection) is ported from giga-images-cpp-v5 (`docs/16`, `docs/22`,
`docs/33`, `scripts/*.bat`). Its hardcoded paths, CUDA/LibTorch steps and `-j1` rule do
not apply here and were dropped.

## 1. Toolchain

| Tool | Version (verified) | Where it comes from |
|---|---|---|
| MSVC | VS 2022 17.14, toolset 14.44 | Visual Studio Community 2022 or VS Build Tools 2022, found via `vswhere` |
| Windows SDK | 10.0.26100 | installed with the C++ workload |
| CMake | 4.4.4 | `cmake` pip package in `.venv` |
| Ninja | 1.13.2 | `ninja` pip package in `.venv` |
| Qt | 6.8.3, `msvc2022_64` | `aqtinstall` 3.3.0 into `C:\Qt` (`scripts\setup.bat`) |
| Python | 3.12.5 | `.venv` (tooling and golden-fixture export only; never at runtime, NFR-2) |

Nothing has to be on PATH. `scripts\_env.bat` resolves every tool to an absolute path, so
the scripts behave the same from cmd, PowerShell, Git Bash or a scheduled task.

Qt 6.8.3 is the newest 6.8 LTS patch published to the open-source mirrors (01 section 1
says 6.8 LTS). The base `install-qt` already contains every module the architecture
guide uses: Quick, QuickControls2 (Basic), QuickLayouts, QuickShapes, QuickEffects,
QuickDialogs2, Svg, Widgets (tray), Concurrent, Test. No `-m` extra modules are needed.

## 2. One-time setup

```
scripts\setup.bat
```

- Installs `aqtinstall==3.3.0` into `.venv`, then Qt 6.8.3 into `C:\Qt\6.8.3\msvc2022_64`
  (about 1 GB, about 30 s on this connection).
- Idempotent: exits immediately when `Qt6Config.cmake` already exists.
- Log: `logs\setup.log`.

If Visual Studio is missing entirely, install "Desktop development with C++" from the
Visual Studio Installer; `_env.bat` prints that instruction when it cannot find it.

## 3. Scripts

All scripts live in `scripts\`, take an optional config (`debug` default, or `release`),
find the project root from their own location, and print exactly one result line per
step. Full output goes to `logs\`, never to the console, so terminals and agent contexts
stay clean (lesson carried from giga-images `docs/16`).

| Script | What it does | Log |
|---|---|---|
| `_env.bat` | Shared environment. CALLed by the others; never run directly | — |
| `setup.bat` | One-time Qt install (section 2) | `setup.log` |
| `configure.bat [cfg]` | CMake configure into `build\<cfg>` (Ninja, cl, Qt prefix, compile_commands.json) | `configure_<cfg>.log` |
| `build.bat [cfg] [target]` | Configures if needed, builds, enforces zero warnings, deploys Qt next to the exe | `build_<cfg>.log`, `deploy_<cfg>.log` |
| `run-tests.bat [cfg]` | Runs `build.bat`, then `ctest` (unit + golden) | `tests_<cfg>.log` |
| `clean.bat [cfg\|all]` | Deletes `build\<cfg>` or all of `build\` | — |
| `capture-window.ps1` | PNG of the real running window (Definition of done, item 6) | — |
| `..\start.cmd` | Launches `build\release\bin\giga-strata.exe`, falling back to debug | — |

Exit code is 0 on success and 1 on failure for every script, so they chain with `&&`.

### Console output contract

```
[CONFIGURE] SUCCESS - Debug - logs\configure_debug.log
[BUILD] SUCCESS - debug - logs\build_debug.log
[DEPLOY] SUCCESS - build\debug\bin is self-contained - logs\deploy_debug.log
100% tests passed out of 1
[TESTS] SUCCESS - logs\tests_debug.log
```

On failure the step prints `FAILED`, then the matching error lines from its log.

### build.bat gates

1. Target check: with a `[target]` argument, `ninja -t query` verifies the target exists
   before building and fails with a clear message otherwise.
2. Warning gate (R32): any `: warning X1234` line in the build log fails the build even
   if the compiler did not stop. This catches warnings from targets that lack `/WX`.
3. Deploy: when `build\<cfg>\bin\giga-strata.exe` exists, `windeployqt` copies Qt DLLs,
   QML imports (scanned from `src\ui\qml`) and the MSVC runtime beside it, so that folder
   runs on a machine without Qt (NFR-1). Before the exe exists, deploy reports SKIPPED.
4. Server (`04` section 6): copies the fork's `llama-server.exe` (from
   `third_party\llama.cpp-upstream\bin\bin\Release`, or `GIGA_STRATA_FORK_DIR`), the CUDA 13
   DLLs (`cublas64_13`, `cublasLt64_13`, `cudart64_13`) and the MSVC + OpenMP runtime from
   the VS redist folder into `build\<cfg>\bin\server\` (a child process does not find DLLs in
   the parent `bin\`), and writes `BUILD_INFO.txt` (commit, branch, build time). It FAILS if
   the exe is older than the fork's HEAD commit, the fork has uncommitted changes, or a DLL
   is missing; it reports SKIPPED (exit 0) when no fork exists, as in a worktree or a fresh
   clone. Copies use `xcopy /d`, so an unchanged rebuild copies nothing. Log:
   `logs\server_<cfg>.log`.
5. Ninja ran: a build log without ninja's `[n/m]` progress or `no work to do` fails the
   build, and every run starts a fresh log (section 7, item 8).

### Optional overrides

| Variable | Default | Use |
|---|---|---|
| `GIGA_STRATA_QT_VERSION` | `6.8.3` | Try another Qt patch (re-run `setup.bat`) |
| `GIGA_STRATA_QT_ROOT` | `C:\Qt` | Qt install root |
| `GIGA_STRATA_JOBS` | `8` | Ninja parallelism (see section 7, item 6) |
| `GIGA_STRATA_VENV` | `<root>\.venv` | Use the tool venv of another checkout |
| `GIGA_STRATA_FORK_DIR` | `<root>\third_party\llama.cpp-upstream` | The llama.cpp fork whose server is packaged (worktrees point it at the main checkout's) |

## 4. Daily workflow

```
scripts\run-tests.bat                 :: build debug + run every test
scripts\build.bat release             :: release build, deployed
start.cmd                             :: run it
powershell -ExecutionPolicy Bypass -File scripts\capture-window.ps1 -Out shot.png
```

Single target while iterating: `scripts\build.bat debug giga_core`.
Stale cache or odd link errors: `scripts\clean.bat debug`, then build again.

### From Git Bash (this is how agents run them)

```bash
cmd //c "scripts\\build.bat release"          # works: double slash, escaped backslashes
cmd //c "D:\\Projects\\giga-strata\\start.cmd"   # use the full path when cwd is not the root
./scripts/build.bat                           # does NOT work: .bat is not executable in bash
cmd /c "scripts\\build.bat"                   # does NOT work: MSYS rewrites /c into a path
```

### From PowerShell

```powershell
.\scripts\build.bat release; if ($LASTEXITCODE) { throw "build failed" }
```

## 5. Contract the CMake files must honor

The scripts assume these; changing one means changing the script and this section.

| Assumption | Why |
|---|---|
| Executable target is named `giga-strata` | `build.bat` deploy step and `start.cmd` look for `giga-strata.exe` |
| `CMAKE_RUNTIME_OUTPUT_DIRECTORY` is `${CMAKE_BINARY_DIR}/bin` | exe, test exes and deployed DLLs share one folder |
| QML sources live under `src/ui/qml` | `windeployqt --qmldir` scans it for imports |
| Tests are registered with `enable_testing()` + `add_test()` at the top level | `ctest --test-dir build\<cfg>` finds them |
| `find_package(Qt6 6.8 ...)` + `qt_standard_project_setup(REQUIRES 6.8)` | configure passes only `CMAKE_PREFIX_PATH` |
| Project targets add `/W4 /WX /external:anglebrackets /external:W0` | R32; Qt headers stay silent |

Verified detail: `/W4 /WX` on the executable target also covers the moc and qmlcachegen
generated sources in that target, and Qt 6.8.3's generated code compiled clean under it.
If a future Qt patch breaks that, move `/WX` off the QML module target instead of
lowering the warning level; the log gate in `build.bat` still enforces zero warnings.

Test executables are not deployed; `run-tests.bat` prepends `C:\Qt\...\bin` to PATH so
they find Qt DLLs.

## 6. Why the scripts look the way they do

- vcvars64 in a child cmd. Calling `vcvars64.bat` inside a script with `goto` labels
  breaks label lookup (giga-images `docs/33`, error 8). Each step runs
  `cmd /c "call vcvars64.bat && <tool>"`, so the parent script's labels stay intact.
- vswhere instead of a hardcoded VS path. This machine has both Community and Build
  Tools 2022; `vswhere -latest -requires ...VC.Tools.x86.x64` picks whichever is newest
  and has the C++ toolset.
- Absolute tool paths. The scripts never trust PATH, so a stray CMake or Ninja elsewhere
  on PATH (Chocolatey, VS-bundled, CUDA) can never be the one that runs.
- CRLF line endings. cmd's `goto`/`call :label` misbehaves on LF-only files at certain
  byte offsets. Keep `*.bat`/`*.cmd` CRLF (enforced by `.gitattributes`:
  `*.bat text eol=crlf`, `*.cmd text eol=crlf`).
- `setlocal` first in every runnable script, so running one in an interactive cmd does
  not leak `ROOT`, `PATH` additions and so on into the session.

## 7. Lessons learned (each one hit while building this)

1. The venv's `cmake.exe` launcher is dead. `.venv` was created at
   `N:\project-maya-main\.venv` and moved here; pip's `Scripts\cmake.exe` launcher has
   the old interpreter path baked in and exits 1 with no output. The real binary is at
   `.venv\Lib\site-packages\cmake\data\bin\cmake.exe` and `_env.bat` uses it.
   `Scripts\ninja.exe` is the real binary, not a launcher, so it works. Any other
   `Scripts\*.exe` launcher (pip, f2py) is equally dead; use `.venv\Scripts\python.exe -m <tool>`.
2. `(x86)` inside an `if (...)` block. Echoing an unquoted path that contains
   `Program Files (x86)` inside a parenthesized block closes the block early:
   `\Microsoft was unexpected at this time.` Every path echoed inside a block is quoted.
3. Non-DPI-aware screenshots lie. This machine runs at 300% scaling. A PowerShell capture
   without `SetProcessDPIAware()` gets virtualized coordinates and returns only the
   top-left third of the window. `capture-window.ps1` declares DPI awareness first.
4. Native title bar follows the system theme. With the system in light mode the Qt window
   shows a white title bar over the graphite UI (seen in the smoke run). N1/SystemInfo
   must force a dark title bar rather than follow the system.
5. Controls.Basic `SplitView` handle is a light gray bar by default; it needs a themed
   `handle` delegate (N5) or it breaks the palette.
6. Parallelism: this rig has segfaulted compilers under heavy parallel builds before (see
   giga memory notes). The Qt app is small, so `-j 8` is the default rather than all 24
   threads; raise `GIGA_STRATA_JOBS` only if builds stay stable.
7. From Git Bash, `cmd //c "start.cmd"` sometimes fails with "not recognized" depending on
   how the cwd maps; passing the full Windows path always works.
8. A build that never ran still said SUCCESS. Processes started under vcvars (VS telemetry
   `vctip.exe`, the PDB server `mspdbsrv.exe`) inherit the build log's handle and outlive the
   build; the next run's `> log` redirect then fails ("The process cannot access the
   file..."), ninja never starts, and cmd still returns 0. Fixed three ways: `_env.bat` sets
   `VSCMD_SKIP_SENDTELEMETRY=1`, regeneration writes its own log, and the build log is deleted
   (or renamed if still locked) before each run and must contain ninja's output.

## 8. Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| `[ENV] FAILED - cmake not found` | venv missing the cmake wheel | `.venv\Scripts\python.exe -m pip install cmake` |
| `[ENV] FAILED - no Visual Studio 2022 install with the C++ x64 toolset` | workload not installed | Visual Studio Installer, "Desktop development with C++" |
| `[CONFIGURE] FAILED - Qt not installed` | setup not run, or version override mismatch | `scripts\setup.bat` |
| `[BUILD] FAILED - warnings present` | R32 gate | fix the warning; never add `/wd` for project code |
| `[BUILD] FAILED - target "x" is not defined` | typo, or target not added in CMake yet | check `ninja -C build\debug -t targets` |
| exe starts then exits immediately | QML load error | run the debug exe from a console; Qt prints the QML error to stderr |
| test exe cannot find `Qt6Cored.dll` | run outside `run-tests.bat` | prepend `C:\Qt\6.8.3\msvc2022_64\bin` to PATH |
| odd link errors after a CMake change | stale cache | `scripts\clean.bat debug` |

## 9. Verification record (2026-11-10)

Run in a scratch copy of the scripts with a minimal project that follows section 5
(Qt Quick window with Controls.Basic `SplitView` + `QtQuick.Shapes` arc, a `QApplication`
main, `/W4 /WX`, one Qt Test):

| Step | Result |
|---|---|
| `setup.bat` | Qt 6.8.3 msvc2022_64 installed to `C:\Qt` in 32 s |
| `run-tests.bat debug` | configure, build, deploy, `100% tests passed out of 1`; 8 s total |
| `build.bat release` | configure, build, deploy SUCCESS |
| warning gate | injected C4189 without `/WX` → `[BUILD] FAILED - warnings present`, exit 1 |
| target check | `build.bat debug nosuchtarget` → FAILED with message, exit 1 |
| `start.cmd` + `capture-window.ps1` | deployed release exe ran standalone; window captured at 1956x1303 |

Not yet verified (no project sources exist): the real `CMakeLists.txt`, golden tests, and
`windeployqt` against the full component set. Re-run this table when the first real
build lands.
