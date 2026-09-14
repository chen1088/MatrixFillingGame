# Matrix Filling Game in C++

This directory contains the desktop application and all working computational
components migrated from the Java project, together with the existing C++
search engine. The editor, matrix analysis, and command-line tools share one
C++20 library. Java and a JVM are not required.

## Build and run

Requirements: a C++20 compiler, CMake 3.21+, and Qt 6.4+ Widgets. Qt Test is needed
when building tests. Graphviz is optional, for DFA graph layout only.

Ubuntu/Debian:

```sh
sudo apt install build-essential cmake ninja-build qt6-base-dev libgl1-mesa-dev
# Optional DFA graph layout:
sudo apt install graphviz
cmake -S cpp -B cpp/build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build cpp/build --parallel
ctest --test-dir cpp/build --output-on-failure
./cpp/build/MatrixFillingGame
```

Windows: install Visual Studio 2022's **Desktop development with C++** workload,
CMake, and a matching Qt desktop MSVC kit. From Developer PowerShell, use the
actual Qt kit directory in `CMAKE_PREFIX_PATH`, for example:

```powershell
cmake -S cpp -B cpp/build -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/msvc2022_64"
cmake --build cpp/build --config Release --parallel
ctest --test-dir cpp/build -C Release --output-on-failure
cmake --install cpp/build --config Release --prefix cpp/install
./cpp/install/bin/MatrixFillingGame.exe
```

For tests and running directly from the build directory, put the Qt kit's `bin`
directory on `PATH`. Installation runs Qt's deployment tool to include the
required Qt runtime files. A Qt Creator kit also supplies the build environment.

macOS: install Xcode command-line tools, CMake, and Qt (`brew install cmake qt`
is one option). From the repository root:

```sh
cmake -S cpp -B cpp/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build cpp/build --parallel
ctest --test-dir cpp/build --output-on-failure
open cpp/build/MatrixFillingGame.app
```

`cmake --install cpp/build --prefix cpp/install` stages the application; Qt's
macOS deployment support bundles the required frameworks. On Linux, install the
Qt runtime packages on the target machine; the installed executable is not a
self-contained AppImage. Builds on Windows, macOS and Linux are covered by the
repository workflow. Local verification for this migration used Linux/Qt 6.4.

For servers or batch computations without Qt:

```sh
cmake -S cpp -B cpp/build-headless -DMFG_BUILD_GUI=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build cpp/build-headless --parallel
ctest --test-dir cpp/build-headless --output-on-failure
```

The Makefile is a wrapper around CMake: `make -C cpp`, `make -C cpp test`, and
`make -C cpp headless BUILD_DIR=build-headless`. There is one build definition.

## Desktop controls

- Click a matrix cell to toggle its support: `1` becomes blank; `0` or blank
  becomes `1`. Arrow keys select a cell. `0`, `1`, and `B` set an explicit raw
  value. Right-click also provides cell actions.
- The **Maximal configuration** checkbox selects the displayed and analyzed
  matrix. Clear it to inspect and analyze the raw matrix exactly as entered.
- Select any row or column, then insert before/after or delete. At least one
  row and one column remain. Exclusion masks move with their cells.
- **Ctrl+Z** (Cmd+Z on macOS) undoes an edit regardless of focus in the matrix,
  buttons, formula pane, or selection spin boxes. Use the platform Redo
  shortcut or Ctrl/Cmd+Shift+Z to redo. Plain `Z` does nothing.
- Exclude individual cells, rows, or columns from constraints. Excluded blanks
  are outside the completion-count universe. Any rectangle touching an
  excluded cell contributes no constraint.
- **Compute / F5** runs selected statistics. **Auto-compute** recomputes after
  edits. **Cancel** interrupts work. Edits immediately clear stale results;
  only results for the latest matrix/view/settings can be displayed.
- File/Open, Save, and clipboard actions retain cells and masks. New, imported,
  pasted and edited matrices support undo/redo. Saving is atomic.
- **Tools / Automata** opens the migrated DFA viewer: load/save, trim, minimize,
  intersection, word evaluation, and DOT export. Graphviz `dot` renders graphs;
  source text remains available when Graphviz is absent.

The desktop supports up to 256 rows and 256 columns. The reusable Document API
allows up to 1,000,000 cells. These are editor resource limits, separate from
search-engine matrix representations. The matrix canvas paints visible cells
instead of allocating one button for every cell.

## Computations and result meanings

| Display | Meaning |
| --- | --- |
| CountOfM | Exact number of valid completions of the active blank cells. |
| RatioOfM | CountOfM divided by the total number of active-blank assignments. |
| APRIS | Uniform Monte Carlo valid-completion estimate and standard error; tooltip includes the corrected importance-sampling estimate. |
| Hypothesis | Whether the forced-implication greedy candidate completes and passes a full rectangle check. Failure is a failure of this heuristic, not proof of unsatisfiability. |
| BlankStat | B0/B: blanks diagonally paired with another blank across two fixed zero corners / all active blanks. |
| Rectangle CNF | Coordinate-based clauses forbidding ordered 1001 corners, including nonadjacent rows and columns. |

Zero blanks means exactly one assignment, which may be valid or invalid. Free
blank variables count even if they occur in no residual clause. Exact counts
are decimal strings, so free-variable factors do not overflow 64 bits. Exact
enumeration defaults to at most 24 constrained variables; this is configurable.
Reaching a resource limit is reported as such, never as an exact result or a
counterexample. Sampling uses a reproducible seed. Standard error is not a
proof of feasibility; the unbiased importance estimate can leave [0,1] through
sampling noise.

All jobs use immutable input snapshots, cooperate with cancellation, and publish
results on the GUI thread. At most one worker runs while one latest replacement
request waits. Work never reads the live editor's mutable state.

## Matrix files

Plain text uses rectangular rows of `0`, `1`, and `b` (also `B`, `2`, `.`, `?` for
blank), separated by spaces or commas. For example:

```text
1 b
b 1
```

Saved `.mfg` files use six semicolon-separated fields:

```text
rows;columns;row,column,value/...;ignored-row,ignored-column/...;ignored-row/...;ignored-column/...
```

All serialized coordinates are zero-based; the UI labels coordinates from 1.
Unlisted cells are blank. The third field uses `0`, `1`, `2` for zero, one,
blank. The fourth field lists individually excluded cells. For example:

```text
2;2;0,0,1/1,1,1/;;;
```

The Java exporter omitted dimensions and produced data incompatible with its
own importer. Such malformed four-field exports cannot reliably recover the
original shape; use plain matrix text or a valid six-field record.

## Command-line analysis and experiments

```sh
./cpp/build/mfg_analyze cpp/examples/diagonal.mfg --no-sampling
./cpp/build/mfg_analyze cpp/examples/nonadjacent.txt --raw --no-sampling
./cpp/build/mfg_analyze matrix.mfg --samples 100000 --seed 7 --paired-greedy
./cpp/build/mfg_search --max-k 5
./cpp/build/mfg_search --max-k 4 --dump-k 3
./cpp/build/mfg_search --max-k 4 --blank-ratios
```

`mfg_analyze --help` lists count/estimate controls. `--raw` analyzes the supplied
cells exactly; otherwise maximal configuration is used. Ctrl+C cancels analysis. The GUI previews the first 1,000 CNF clauses;
Analysis / Export CNF saves the complete generated formula.
Exit status 0 means completion, 1 input/error, 2 a requested resource limit, 130
interruption. Individual completed results remain usable when another requested
calculation reaches its limit.

The search tool retains exhaustive normalized-support enumeration, exact
rectangle CNFs, SAT checking, geometric witnesses, and canonical parent/child
existential projections. `--blank-ratios` runs the migrated sparse-support B0/B
scoring experiment; zero-blank ratios are reported as undefined.
See [the search pipeline](docs/search-engine.md) for mathematical details.

`mfg_tools --help` documents DFA and polynomial commands and formats. Examples:

```sh
./cpp/build/mfg_tools dfa-dot cpp/examples/parity.dfa > parity.dot
./cpp/build/mfg_tools dfa-count cpp/examples/parity.dfa 10
./cpp/build/mfg_tools dfa-intersect first.dfa second.dfa > product.dfa
./cpp/build/mfg_tools poly-cyclic 3 4 > step.poly
./cpp/build/mfg_tools poly-print step.poly
```

DFA counts use checked `uint64_t`; Laurent-polynomial coefficients and exponents
use checked `int64_t`. Overflow produces an explicit error. The shared C++ API
also exposes both greedy variants, local forced/conflicting-cell maps, masked
analysis, DFA inverse transitions, and polynomial algebra.

## Source organization

- `include/mfg/`: reusable C++ APIs.
- `src/`: matrix/document, analysis, CNF/SAT/search, automata and polynomials.
- `src/gui/`: Qt desktop and DFA viewer.
- `src/cli/`: matrix analysis, research-search, and DFA/polynomial entry points.
- `tests/`: independent computation oracles and editor/GUI regressions.
- `examples/`: importable sample matrices and DFA.

[Migration details](docs/migration.md) record functionality mappings, fixes,
limits, and the old Java methods that were unfinished placeholders.
