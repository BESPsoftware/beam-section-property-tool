# Developer Guide

This guide covers local development, CMake targets, test execution, examples,
and common extension points for Beam Section Property Tool.

## Prerequisites

- CMake 3.16 or later.
- C++17 compiler.
- Git.
- Optional Qt Widgets installation:
  - Qt 5 for `SPT_QT_VERSION=5`.
  - Qt 6 for `SPT_QT_VERSION=6` or `AUTO`.
- Windows optional tools:
  - Visual Studio 2022 with Desktop C++ workload.
  - Qt 5.15.2 MSVC package for the documented Windows GUI verification flow.

## Repository Structure

```text
Source/api/              C-compatible exported API
Source/calculation/      Section property calculation
Source/common/           Data model and shared types
Source/geometry/         Parametric and Canvas section construction
Source/gui/              Optional Qt Widgets GUI
Source/import_export/    CSV, JSON, ANSYS, ABAQUS, Midas Civil exporters
Source/mesh/             Lightweight mesh generation
Source/stress/           Stress point generation and coordinate transforms
Examples/                Buildable API examples
tests/unit/              Unit test entry point
tests/regression/        Numerical validation regression tests
tests/integration/       C API and export integration tests
Documents/               User, developer, validation, and delivery docs
```

## CMake Targets

| Target | Type | Purpose |
|---|---|---|
| `SectionPropertyCore` | Static library | C++ calculation, geometry, mesh, stress, export core. |
| `SectionPropertyApi` | Shared library | C ABI wrapper exported as `SectionPropertyTool`. |
| `SectionPropertyTests` | Executable | Unit, regression, and integration test runner. |
| `example1_parametric` | Executable | Parametric H-section example. |
| `example2_canvas` | Executable | Canvas thin-walled section example. |
| `example3_dll_batch` | Executable | Batch C API example. |
| `SectionPropertyGui` | Executable | Optional Qt Widgets GUI when Qt Widgets is found. |

## Build Without Qt

Use this path for CI or core/API development:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSPT_QT_VERSION=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

## Build With Qt 5

macOS Homebrew example:

```bash
cmake -S . -B build-qt5 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt@5)" -DSPT_QT_VERSION=5
cmake --build build-qt5
ctest --test-dir build-qt5 --output-on-failure
```

Windows Qt 5 verification is documented in `Windows_Qt_Verification.md`.

## Build With Qt 6

macOS Homebrew example:

```bash
cmake -S . -B build-qt6 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt)" -DSPT_QT_VERSION=6
cmake --build build-qt6
ctest --test-dir build-qt6 --output-on-failure
```

## Qt Selection

`SPT_QT_VERSION` accepts:

- `AUTO`: prefer Qt 6 Widgets, then fall back to Qt 5 Widgets.
- `6`: select Qt 6 Widgets.
- `5`: select Qt 5 Widgets.
- `OFF`: skip GUI discovery.

Use `5` or `6` when you need to prove a specific Qt major version. Use `OFF`
for portable CI builds.

## Run Tests

CTest runs the combined test executable:

```bash
ctest --test-dir build --output-on-failure
```

Current coverage includes:

- H section workbook checks.
- Box section workbook checks.
- Pipe section workbook checks.
- Supplied crane girder workbook checks.
- Non-reference crane girder approximate-path checks.
- C API smoke coverage.
- CSV, JSON, ANSYS, ABAQUS, and Midas Civil export smoke coverage.

## Run Examples

```bash
./build/Library/bin/example1_parametric
./build/Library/bin/example2_canvas
./build/Library/bin/example3_dll_batch
```

On Windows with a multi-config generator, executables are under a configuration
subdirectory such as `build-msvc\Library\bin\Release\`.

## Add a New Section Type

1. Add a new value to `SectionType` in `Source/common/DataModel.h`.
2. Add the matching API enum to `SptSectionType` in
   `Source/api/section_property_tool.h`.
3. Update the API conversion switch in `Source/api/section_property_tool.cpp`.
4. Add a builder method in `Source/geometry/SectionBuilder.h/.cpp`.
5. Validate required parameters and return clear diagnostics.
6. Populate contours or plate/rectangle components so calculation, mesh, and
   drawing paths have geometry to consume.
7. Add stress point behavior if the default generic points are insufficient.
8. Add regression tests and update documentation.

Keep parameter names short and stable. The C API passes them as UTF-8 strings,
so changing names is an API compatibility concern.

## Add a New Export Format

1. Add a new value to `ExportFormat` in `Source/common/DataModel.h`.
2. Add the matching `SptExportFormat` value in
   `Source/api/section_property_tool.h`.
3. Update `toCoreFormat` in `Source/api/section_property_tool.cpp`.
4. Add an exporter method to `Source/import_export/Exporter.h/.cpp`.
5. Route it in `Exporter::exportResult`.
6. Add integration tests in `tests/integration/test_api.cpp`.
7. Document solver/application acceptance status honestly.

Exporters should use `openOutputFile` so UTF-8 paths work on Windows.

## Coding Conventions

- Use C++17.
- Keep core code independent from Qt.
- Route GUI-specific work through `Source/gui`.
- Keep the C API ABI simple: POD structs, opaque handles, and explicit destroy
  functions.
- Return `0` for API success and use `spt_get_last_error()` after failures.
- Prefer deterministic regression data and tolerances over broad approximate
  assertions.
- Keep solver/export claims limited to what tests or external acceptance have
  actually verified.

## Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| `Qt Widgets not found` | Qt is not installed or `CMAKE_PREFIX_PATH` points at the wrong prefix. | Set `CMAKE_PREFIX_PATH` to the Qt CMake package directory and force `SPT_QT_VERSION=5` or `6`. |
| Qt 5 build finds Qt 6 | Qt 6 is globally discoverable and Qt version is `AUTO`. | Reconfigure with `-DSPT_QT_VERSION=5`. |
| Windows path-length errors | MSVC file tracking path is too long. | Use a short build directory such as `C:\bsp_build`. |
| API call fails | Invalid handle, missing parameter, bad path, or invalid input. | Inspect `spt_get_last_error()`. |
| Export file missing | Path was not writable or process lacked permission. | Use a writable path and check returned error info. |
| Crane non-reference warning | Source diagram is not fully dimensioned for arbitrary crane parameter sets. | Treat non-reference crane values as approximate until rules are confirmed. |
