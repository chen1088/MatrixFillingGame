# Java-to-C++ migration

The active application is in `cpp/`. Existing mathematical search machinery was
retained, while CMake now builds the Qt desktop, shared core, research search,
matrix-analysis CLI, and utility CLI. The original Java tree is preserved for
reference. No paper, result, or existing source outside the migration scope was
removed.

## Working functionality

| Original Java component | C++ replacement |
| --- | --- |
| MFG Swing grid, row/column buttons, keyboard editing | Qt MatrixCanvas/MainWindow, dimension controls, scroll and zoom |
| BMContainer cells, dimensions, maximal view, undo | Document with value snapshots, undo/redo, raw/maximal views |
| Row/column/cell ignore maps | Document masks moved atomically with structural edits |
| BMContainer serialization | Validated six-field serialization, plain-text import, atomic file saving |
| MFGSwingWorker and worker panels | Immutable requests, cooperative cancellation, GUI-thread publication, revision/serial guards |
| FourDNF evaluate/exists/count | Coordinate CNF evaluation, exact completion count, existing SAT solver |
| ApproximateRatio / ApproximateRatioIS | Uniform and correctly weighted importance estimators with standard error |
| GreedyFillMFG / GreedyFillMFG2 | PairedZeros / ForcedImplications candidates with independent final verification |
| is0forced / is1forced, GetForcedMap / GetConflictMap | Local forbidden-value, forced and conflict maps |
| BlankStat / sparse Score | B0/B scans, `mfg_search --blank-ratios` |
| HasDiagRect_naive | Rectangle witness scan using arbitrary row and column pairs |
| SparseMatEnumerator.Next / SparseToFull | Existing canonical support enumeration and matrix construction |
| DFA evaluation, trim, minimization, intersection, counts | Validated Dfa API and `mfg_tools dfa-*` |
| Polynomial arithmetic/cyclic constructions | Checked sparse Laurent Polynomial API and `mfg_tools poly-*` |
| FADisplayPanel / TestGraphviz | Qt Automata dialog, exact Java demo, async native Graphviz, DOT export |
| Existing C++ CNF/SAT/projection search | Retained in mfg_core and mfg_search |

Public spelling and internal data structures changed; mathematical operations
and working user features were migrated. Tuple/Triplet/Transition wrappers are
represented by C++ values and standard containers.

## Corrected defects

1. **Undo aliased live entries.** `new ArrayList<>(matrix)` copied references
   to mutable BMEntry objects. Setting cells or shifting coordinates could
   rewrite saved states. Document histories now own their full state values.
2. **Masks were not part of complete undo.** Individual masks were never
   snapshotted/restored, and mask edits did not save history. Every C++ edit is
   a transaction covering cells, dimensions and all masks.
3. **Structural edits did not move masks.** Row/column/cell exclusions now move
   with surviving entries and disappear with removed entries; undo restores
   their exact previous positions.
4. **Undo depended on focus and a plain Z release.** The Java frame listener
   did not implement a proper application undo shortcut. Qt handles platform
   Undo/Redo even when child controls have focus, without binding plain Z.
5. **Boxed coordinate comparison used identity.** Java `Integer ==` could fail
   outside the small-integer cache. C++ coordinates compare by value.
6. **Save and load formats disagreed.** The Java exporter omitted dimensions;
   the importer shadowed dimension fields and used the wrong separator. C++
   validates a documented format before changing any state.
7. **Worker inputs raced with editing.** Java workers read mutable container
   state after launch. New requests own matrix/mask copies; older results
   cannot overwrite the current matrix's statistics.
8. **Zero-blank counts and renumbering were wrong.** Java CountOfM assigned zero
   possible assignments with no blanks. DNF Normalize changed a local index
   without updating stored literals. Coordinate identities and explicit free
   variable factors remove both errors.
9. **Importance sampling used incorrect term weights.** Sampling now weights
   terms by their cardinality and accounts for overlap multiplicity. Both
   estimates distinguish completion, cancellation and resource limits.
10. **Greedy success was underchecked.** Works now requires every active blank
    filled and no forbidden rectangle; it does not merely mean no blanks remain.
11. **DFA and polynomial defects.** Corrected transition/acceptance resizing,
    binary-only products, minimization corner cases and source mutation, graph
    accepting-state shapes, dropped polynomial terms, exponent identity
    comparisons and silent arithmetic overflow.

## Unfinished Java methods

The following had empty bodies, fixed placeholder returns, or TODO-only logic;
they were not functioning algorithms to preserve:

- `BMUtil.HasDiagRect_mike`, `HasDiagRect_chen`, stateShaving, dfs.
- `FourDNF.Greedy`, `SmartApproximationRatio`.
- `BMContainer.convertToStrongMFG` and `CNF.ResolveFull` and `CNF.convertCNFClausetoDFA` placeholders.
- `SparseMatEnumerator.Next2`, `ScoreNew`, `EditDistance`.
- `DFA.IntersectsV2`, the empty Partition class.

They are not exposed as fake successful computations. Rectangle detection, SAT,
actual greedy methods and existing support/projection algorithms provide their
implemented functionality through the new APIs.

## Verification and limits

The original engine regression suite is retained. New tests cover complete
edit histories, rectangular structural edits, masks, import round trips,
nonadjacent rectangles, brute-force small-matrix completion counts, forced maps,
corrected estimators, cancellation, graph operations and polynomial arithmetic.
Qt tests exercise actual child-control focus, shortcuts, deletion buttons,
view changes, stale-result rejection and process/worker lifetime.

Exhaustive checks include 2,916 maximal-configuration inputs and 21,141 ternary
analysis boards (all 2x3, 3x2, 3x3 boards), plus masked cases and sampling checks.
DFA tests compare accepted languages of generated automata through bounded word
lengths. Bounds and cancellation are reported explicitly; neither a limit nor
a heuristic failure is an unsatisfiability certificate.

The GUI limit is 256 per axis, Document allows up to 1,000,000 cells, and exact
enumeration has a configurable constrained-variable limit plus a hard 62-bit
enumeration-index ceiling. Free-variable factors use arbitrary-length decimal
counts. Utility integer overflow is checked. Graphviz is optional and is invoked
without a shell. Platform CI is configured for Windows, macOS and Linux; local
verification uses Linux and Qt 6.4.2.
