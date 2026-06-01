# Beam Section Property Tool

This is a C++17 beam/section property calculation tool for parametric and
canvas-defined cross-sections. It includes reusable core modules, a
C-compatible API, example programs, tests, documentation, and an optional Qt
Widgets GUI target for Qt 5.15.2 environments.

The branch is a reviewable whole implementation, not a final acceptance-ready
release.

## Current implementation status

Implemented in this branch:

- C++17 core modules for geometry, section building, calculation, stress
  points, mesh generation, and import/export scaffolding.
- C-compatible API declared in `Source/api/section_property_tool.h`.
- Parametric support for H Section, Box Section, Pipe Section, and Quayside
  Crane Girder.
- Canvas plate-centerline support for thin-walled section input.
- Stress point generation and global/principal coordinate transforms.
- Lightweight triangular mesh generation for visualization and integration
  scaffolding.
- Example programs for parametric use, Canvas use, and DLL-style batch use.
- Regression, unit, and integration tests.
- User manual files in Markdown and DOCX form.
- Source requirements PDF/XLS copied into `Documents/source_requirements/`.

## Verified on macOS

The following non-Qt and Qt-enabled verification builds have been run from the
project root on macOS with AppleClang.

Core/API verification:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/Library/bin/example1_parametric
./build/Library/bin/example2_canvas
./build/Library/bin/example3_dll_batch
```

Qt-enabled verification using Homebrew `qt@5`:

```bash
cmake -S . -B build-qt -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt@5)"
cmake --build build-qt
ctest --test-dir build-qt --output-on-failure
./build-qt/Library/bin/example1_parametric
./build-qt/Library/bin/example2_canvas
./build-qt/Library/bin/example3_dll_batch
```

Result:

- CMake configure/build passed on macOS with AppleClang.
- Core static library built successfully as `libSectionPropertyCore.a`.
- C API shared library built successfully as `libSectionPropertyTool.dylib`.
- `SectionPropertyGui` built successfully in the Qt-enabled `build-qt` build.
- Tests passed with CTest: `1/1 tests passed`.
- The three example programs executed successfully from `build-qt/Library/bin`.

Generated artifacts confirmed in the CMake build outputs:

- `libSectionPropertyTool.dylib`
- `libSectionPropertyCore.a`
- `SectionPropertyTests`
- `example1_parametric`
- `example2_canvas`
- `example3_dll_batch`
- `SectionPropertyGui` in the Qt-enabled build.

## Not yet verified

- Windows core/API build is verified with CMake/MSVC on Windows 10/11.
- `SectionPropertyTool.dll` and `SectionPropertyTool.lib` generation is verified,
  and exported symbols are confirmed.
- Windows Qt 5.15.2 GUI build and deployment steps are documented in
  `Documents/Windows_Qt_Verification.md`. Runtime smoke test on a Windows
  machine still needs to be executed and signed off.
- FEM export formats still need acceptance testing against ANSYS / ABAQUS /
  Midas Civil expectations.

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
- Numerical validation expected-vs-actual report is in
  `Documents/Numerical_Validation_Report.md`. All 41 checks pass against
  the `Test Data.xls` reference values.
- Crane girder behavior requires further validation beyond the supplied XLS case
  because the PDF diagram is not fully dimensioned.

## Build instructions

Core/API build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Qt-enabled macOS build using Homebrew `qt@5`:

```bash
cmake -S . -B build-qt -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt@5)"
cmake --build build-qt
ctest --test-dir build-qt --output-on-failure
```

Run examples from the Qt-enabled build:

```bash
./build-qt/Library/bin/example1_parametric
./build-qt/Library/bin/example2_canvas
./build-qt/Library/bin/example3_dll_batch
```

Qt is optional at configure time. If Qt5 Widgets is found, CMake also builds the
`SectionPropertyGui` target. Without Qt5 Widgets, the core library, C API shared
library, examples, and tests still build.

## Remaining tasks before completion

1. Verify Windows build with CMake/MSVC.
2. Produce and verify `.dll`, `.lib`, exported symbols, and Windows usage
   examples.
3. Verify Qt 5.15.2 GUI build and deployment on Windows.
4. Produce expected-vs-actual numerical validation tables.
5. Validate FEM export card formats.
6. Validate crane girder calculations with more than the supplied reference
   case.
7. Review the user manual for completeness against the requirements.
