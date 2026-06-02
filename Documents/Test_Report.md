# Test Report

## Regression Source

Reference data was extracted from `Test Data.xls`. The workbook contains one
visible sheet named `Test Data` and static values for H/I, Hollow Box, Pipe, and
Crane Runway sections.

## Automated Coverage

- H Section: area, Jz, Jy, Jx, Az, Ay, centroid, four stress points.
- Hollow Box: area, Jz, Jy, Jx, Az, Ay, centroid, four stress points.
- Pipe: area, Jz, Jy, Jx, Az, Ay, centroid, four stress points.
- Crane Runway: area, Jz, Jy, Jyz, theta, Jzo, Jyo, Jx, Az0, Ay0, centroid,
  four principal stress points.
- Non-reference crane girder: approximate plate-graph properties, stress-point
  generation, and warning diagnostic coverage.
- DLL API smoke test: create section, calculate, read properties, read stress
  points, create mesh, export CSV, ANSYS, ABAQUS, and Midas Civil cards.

## Verification Run

Executed in the development environment with CMake and AppleClang for the
core/API build:

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/Library/bin/example1_parametric
./build/Library/bin/example2_canvas
./build/Library/bin/example3_dll_batch
```

Result:

- CMake configure completed successfully with AppleClang.
- Core static library built successfully as `libSectionPropertyCore.a`.
- C API shared library built successfully as `libSectionPropertyTool.dylib`.
- Examples built and executed successfully.
- CTest passed: `1/1 tests passed`.

## Qt GUI and Examples Verification

The Qt-enabled macOS build was verified with explicit Qt major-version
selection. Qt 5 was verified using Homebrew `qt@5`:

```text
cmake -S . -B build-qt5 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt@5)" -DSPT_QT_VERSION=5
cmake --build build-qt5
ctest --test-dir build-qt5 --output-on-failure
./build-qt5/Library/bin/example1_parametric
./build-qt5/Library/bin/example2_canvas
./build-qt5/Library/bin/example3_dll_batch
```

Qt 6 was verified using Homebrew `qt`:

```text
cmake -S . -B build-qt6 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt)" -DSPT_QT_VERSION=6
cmake --build build-qt6
ctest --test-dir build-qt6 --output-on-failure
```

Observed results:

- CMake configure completed successfully with AppleClang and Homebrew `qt@5`
  when forced with `SPT_QT_VERSION=5`.
- CMake configure completed successfully with AppleClang and Homebrew `qt`
  when forced with `SPT_QT_VERSION=6`.
- `SectionPropertyGui` was built successfully in both Qt builds.
- Core static library built successfully as `libSectionPropertyCore.a`.
- C-compatible API shared library built successfully as
  `libSectionPropertyTool.dylib`.
- `SectionPropertyTests` passed under CTest: `1/1 tests passed`.
- `example1_parametric`, `example2_canvas`, and `example3_dll_batch` executed
  successfully from `build-qt5/Library/bin`.
- Prior Windows core/API build and `SectionPropertyTool.dll` export
  verification are documented for MSVC and a short build directory.
- Windows Qt deployment remains unverified.

## Tolerances

- Coordinates, area, and shear area: `max(0.01, 1e-6 * abs(expected))`.
- Inertia and torsion: `max(1.0, 1e-6 * abs(expected))`.
- Principal-axis angle: `1e-8 rad`.

## Known Limitations

- GUI automation is manual unless Qt Test is added to the CI environment.
- Crane girder non-reference parameter sets use an approximate plate graph until
  the client confirms full construction rules; these cases emit a warning.
- FEM card exports write general section/property values for ANSYS, ABAQUS, and
  Midas Civil review. Solver-specific acceptance should still be confirmed in
  the target FEM applications.
