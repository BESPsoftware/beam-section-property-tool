# Beam Section Property Tool

This is a C++17 beam/section property calculation tool for parametric and
canvas-defined cross-sections. It includes reusable core modules, a
C-compatible API, example programs, tests, documentation, and an optional Qt
Widgets GUI target for Qt 5 or Qt 6 environments.

The branch is a reviewable whole implementation, not a final acceptance-ready
release.

## Current implementation status

Implemented in this branch:

- C++17 core modules for geometry, section building, calculation, stress
  points, mesh generation, and import/export.
- C-compatible API declared in `Source/api/section_property_tool.h`.
- Parametric support for H Section, Box Section, Pipe Section, and Quayside
  Crane Girder.
- Canvas plate-centerline support for thin-walled section input.
- Stress point generation and global/principal coordinate transforms.
- Lightweight triangular mesh generation for visualization and integration
  scaffolding.
- CSV/JSON export plus ANSYS, ABAQUS, and Midas Civil general section/property
  card writers.
- Example programs for parametric use, Canvas use, and DLL-style batch use.
- Regression, unit, and integration tests.
- Optional Qt Widgets GUI target with selectable Qt 6, Qt 5, or no-GUI builds.
- User manual files in Markdown and DOCX form.
- Source requirements PDF/XLS copied into `Documents/source_requirements/`.

## Verified on macOS

The following verification builds have been run from the project root on macOS
with AppleClang.

Core/API verification:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/Library/bin/example1_parametric
./build/Library/bin/example2_canvas
./build/Library/bin/example3_dll_batch
```

Qt 5 verification using Homebrew `qt@5`:

```bash
cmake -S . -B build-qt5 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt@5)" -DSPT_QT_VERSION=5
cmake --build build-qt5
ctest --test-dir build-qt5 --output-on-failure
./build-qt5/Library/bin/example1_parametric
./build-qt5/Library/bin/example2_canvas
./build-qt5/Library/bin/example3_dll_batch
```

Qt 6 verification using Homebrew `qt`:

```bash
cmake -S . -B build-qt6 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt)" -DSPT_QT_VERSION=6
cmake --build build-qt6
ctest --test-dir build-qt6 --output-on-failure
```

Result:

- CMake configure/build passed on macOS with AppleClang.
- Core static library built successfully as `libSectionPropertyCore.a`.
- C API shared library built successfully as `libSectionPropertyTool.dylib`.
- `SectionPropertyGui` built successfully in Qt 5 and Qt 6 builds.
- Tests passed with CTest: `1/1 tests passed`.
- The three example programs executed successfully from the core and Qt 5
  builds.

Generated artifacts confirmed in the CMake build outputs:

- `libSectionPropertyTool.dylib`
- `libSectionPropertyCore.a`
- `SectionPropertyTests`
- `example1_parametric`
- `example2_canvas`
- `example3_dll_batch`
- `SectionPropertyGui` in Qt-enabled builds.

## Verified on Windows

Prior merged project evidence documents:

- Windows core/API build with CMake/MSVC on Windows 10/11.
- `SectionPropertyTool.dll` and `SectionPropertyTool.lib` generation.
- Exported C API symbols confirmed with `dumpbin`.

This macOS review did not rerun Windows-only commands.

## Not verified / requires Windows or external software

- Windows Qt GUI runtime smoke test and deployment sign-off still require a
  Windows machine.
- FEM export card writers emit general section/property values and are covered
  by API integration tests, but ANSYS / ABAQUS / Midas Civil acceptance still
  requires review in those applications.
- Non-reference crane girder cases use the approximate plate-graph path until
  the client confirms complete construction rules beyond the supplied XLS case.

## Windows Build

On Windows with Visual Studio 2022, use the following build flow:

```bat
cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64
cmake --build build-msvc --config Release
ctest --test-dir build-msvc -C Release --output-on-failure
```

If your source path is long, use a short build directory such as
`C:\bsp_build` to avoid MSVC file-tracking path length issues.

A helper script `build_windows.bat` is included for this workflow.

For a Qt 5 GUI build on Windows, add
`-DCMAKE_PREFIX_PATH=<Qt5 cmake path>` and `-DSPT_QT_VERSION=5`; see
`Documents/Windows_Qt_Verification.md`.

- Numerical validation expected-vs-actual report is in
  `Documents/Numerical_Validation_Report.md`. All 41 checks pass against
  the `Test Data.xls` reference values.
- Crane girder behavior is validated for the supplied XLS acceptance case and
  additional non-reference approximate plate-graph cases. The source PDF is not
  fully dimensioned, so non-reference crane cases intentionally emit a warning.

## Build instructions

Core/API build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Qt-enabled macOS build using Homebrew `qt@5`:

```bash
cmake -S . -B build-qt5 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt@5)" -DSPT_QT_VERSION=5
cmake --build build-qt5
ctest --test-dir build-qt5 --output-on-failure
```

Qt-enabled macOS build using Homebrew `qt`:

```bash
cmake -S . -B build-qt6 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt)" -DSPT_QT_VERSION=6
cmake --build build-qt6
ctest --test-dir build-qt6 --output-on-failure
```

Run examples from a build:

```bash
./build/Library/bin/example1_parametric
./build/Library/bin/example2_canvas
./build/Library/bin/example3_dll_batch
```

Qt is optional at configure time. `SPT_QT_VERSION=AUTO` prefers Qt 6 and falls
back to Qt 5. Use `SPT_QT_VERSION=5`, `SPT_QT_VERSION=6`, or
`SPT_QT_VERSION=OFF` to force a specific mode. Without Qt Widgets, the core
library, C API shared library, examples, and tests still build.

## Remaining tasks before completion

1. Run and sign off the Windows Qt GUI runtime smoke test and deployment flow.
2. Validate ANSYS, ABAQUS, and Midas Civil card acceptance in the target FEM
   applications.
3. Confirm full crane girder construction rules for non-reference parameter
   sets, or keep the approximate warning behavior as an explicit limitation.
4. Review the user manual and DOCX package for completeness against the
   original requirements.
