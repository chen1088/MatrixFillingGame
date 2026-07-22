# MatrixFillingGame

This repository now has two deliberately separate components:

- `src/`: the original Java Swing application for drawing configurations and
  running the earlier experiments.
- `cpp/`: the C++20 proof/counterexample engine for exhaustive sparse-support
  enumeration, strong normalization, exact rectangle CNFs, SAT checking, and
  canonical parent/child projection.

To build and run the new engine with GNU Make:

```bash
make -C cpp test
make -C cpp
./cpp/build/mfg_search --max-k 6
```

See [`cpp/README.md`](cpp/README.md) for the mathematical pipeline and the CMake
build.

Earlier experimental question: can you make the ratio 0?

(Stronger) Can you fail the earlier hypothesis? (Done; the answer is yes.)

TODO: Smarter approximate counting method for 4DNF.
