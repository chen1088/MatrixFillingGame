# Matrix Filling Game

The application now lives in [`cpp/`](cpp/): a C++20 computational library,
Qt 6 desktop editor, and command-line research tools. The Java source in
`src/` is retained as a historical reference; it is not needed to build or run
the C++ application.

```sh
cmake -S cpp -B cpp/build -DCMAKE_BUILD_TYPE=Release
cmake --build cpp/build --config Release --parallel
ctest --test-dir cpp/build -C Release --output-on-failure
```

Run `cpp/build/MatrixFillingGame` on Linux, `cpp/build/Release/MatrixFillingGame.exe`
on Windows, or `cpp/build/MatrixFillingGame.app` on macOS. Qt 6.4 or newer is
required for the desktop application; the command-line tools also build without Qt.

- [Build, controls, and command-line guide](cpp/README.md)
- [Migration inventory and corrected Java bugs](cpp/docs/migration.md)
- [Search engine and mathematical pipeline](cpp/docs/search-engine.md)

The original experimental question was whether every maximal configuration has
a valid completion; counterexamples refute it. A bounded search that finds none
is experimental evidence, not a proof of universal fillability.
