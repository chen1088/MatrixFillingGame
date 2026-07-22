# C++ research engine

This directory is the new proof/counterexample engine for the Matrix Filling
Game. It is deliberately separate from the Java Swing application in `../src`.
The GUI remains available for hand-built examples; this engine owns exhaustive,
reproducible searches.

## Mathematical pipeline

For a normalized support `S` containing exactly the entries fixed to `1`, the
program constructs

```text
S  ->  M(S)  ->  F(S).
```

- `S` has no empty row or column.
- `M(S)` is the unique strong/maximum configuration from the normalization
  lemma: keep the `1`s, keep the two off-diagonal corners of every
  `[[1,b],[b,1]]` witness blank, and fill every other cell with `0`.
- `F(S)` is the exact residual CNF. For every ordered rectangle it simplifies
  the clause

  ```text
  ~x(top,left) OR x(top,right) OR x(bottom,left) OR ~x(bottom,right).
  ```

Variables are coordinates, not temporary dense IDs. This is essential when a
support at sparsity `k+1` is compared with its parent at sparsity `k`.
Every residual clause also retains the `(top,bottom,left,right)` coordinates of
its source rectangle. The SAT/projection CNF may remove logically redundant
clauses, but the geometric witness basis remains available for the planned
witness-first induction search.

Every non-root support has one canonical parent: delete its row-major last `1`,
then compress the newly empty row and/or column. The program enumerates the
resulting parent/child tree without losing ordered matrix shapes.

This parent choice has a useful invariant: the deleted last `1` cannot have
occupied a blank of `M(parent)`. Every such blank precedes the southeast witness
`1` that created it. Consequently every parent blank embeds as a child blank,
which gives the persistent variables `X`; all other child blanks are genuinely
new variables `Y`.

For a child condition `Gamma`, the projection routine computes exactly

```text
exists Y (F(child)(X,Y) AND Gamma(X,Y))
```

using Davis-Putnam variable elimination. Its result is renamed back to the
parent's coordinate variables and normalized by literal/clause sorting,
deduplication, and subsumption. The command-line summary currently reports the
base case `Gamma = true`; the same API is ready for the finite induction-scheme
closure.

An unsatisfiable `F(S)` is a strong counterexample. A bounded successful run is
experimental evidence only; it is not presented as a proof of the conjecture.

## Build and test

```bash
cmake -S cpp -B build/cpp -DCMAKE_BUILD_TYPE=Release
cmake --build build/cpp -j
ctest --test-dir build/cpp --output-on-failure
```

Or, with only GNU Make and a C++20 compiler:

```bash
make -C cpp test
make -C cpp
```

Run the bounded search and inspect a complete level with:

```bash
./build/cpp/mfg_search --max-k 5
./build/cpp/mfg_search --max-k 4 --dump-k 3
```

For the Makefile build, the executable is `cpp/build/mfg_search`.

Use `--no-projections` when measuring raw support/CNF/SAT enumeration.

## Next milestone

The next layer will represent a finite induction scheme as a geometric template
plus a Boolean condition `Gamma` on marked blanks. Repeated pullback and
canonicalization can then test whether the predecessor schemes close into a
finite family, while the SAT pass continues searching independently for a
counterexample.
