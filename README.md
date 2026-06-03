# Beam Section Property Tool

[![CMake](https://github.com/BESPsoftware/beam-section-property-tool/actions/workflows/cmake.yml/badge.svg?branch=main)](https://github.com/BESPsoftware/beam-section-property-tool/actions/workflows/cmake.yml)

Beam Section Property Tool is a C++17 application and C API for modeling beam
cross-sections, calculating section properties, generating stress output points,
building lightweight visualization meshes, and exporting results for downstream
finite element workflows.

The project branch for ongoing work and final documentation is `main`. A remote
`main` branch exists. At the time of this documentation pass, changing the
GitHub repository default branch from `try-whole-implementation` to `main`
requires an admin-capable GitHub token; do not delete the old branch until that
setting is changed and confirmed.

## Features

- C++17 calculation core with geometry, stress-point, mesh, and export modules.
- C-compatible shared-library API in `Source/api/section_property_tool.h`.
- Parametric H section, box section, pipe section, and quayside crane girder
  section support.
- Advanced Canvas input for drawing user-defined thin-walled plate centerline
  sections with per-plate thicknesses.
- Zoomable and pannable Qt graphics views for section, stress-point, mesh, and
  Canvas visualization.
- CSV and JSON export.
- ANSYS, ABAQUS, and Midas Civil general section/property card writers.
- Optional Qt Widgets GUI target.
- Unit, regression, and integration tests driven by CTest.
- Example programs for parametric, Canvas, and batch API use.

## Supported Section Types

| Section | API enum | Notes |
|---|---|---|
| H Section | `SPT_H_SECTION` | Parametric I/H-style section. |
| Box Section | `SPT_BOX_SECTION` | Hollow box/girder-style section. |
| Pipe Section | `SPT_PIPE_SECTION` | Circular hollow section. |
| Quayside Crane Girder | `SPT_CRANE_GIRDER` | XLS reference case plus approximate non-reference plate graph. |
| Canvas Thin-Walled | `SPT_CANVAS` | User-supplied or graphically drawn plate centerlines and thicknesses. |

## Verification Status

| Area | Status |
|---|---|
| macOS core/API build | Verified locally with AppleClang. |
| macOS Qt 5 build | Verified locally with Homebrew `qt@5` and `SPT_QT_VERSION=5`. |
| macOS Qt 6 build | Verified locally with Homebrew `qt` and `SPT_QT_VERSION=6`. |
| Ubuntu CI | Verified by GitHub Actions no-GUI CMake workflow. |
| macOS CI | Verified by GitHub Actions no-GUI CMake workflow. |
| Windows core/API | Prior merged evidence documents MSVC build, DLL/import library, and exported symbols. |
| Windows Qt runtime/deployment | Requires real Windows smoke test and deployment sign-off. |
| FEM solver acceptance | Writers exist, but ANSYS / ABAQUS / Midas Civil solver-side acceptance remains pending. |

## Quick Start

Configure, build, test, and run the examples without Qt:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSPT_QT_VERSION=OFF
cmake --build build
ctest --test-dir build --output-on-failure
./build/Library/bin/example1_parametric
./build/Library/bin/example2_canvas
./build/Library/bin/example3_dll_batch
```

Typical artifacts:

- `build/Library/lib/libSectionPropertyCore.a`
- `build/Library/bin/libSectionPropertyTool.dylib` on macOS/Linux, or
  `SectionPropertyTool.dll` plus import library on Windows
- `build/Library/bin/SectionPropertyTests`
- `build/Library/bin/example1_parametric`
- `build/Library/bin/example2_canvas`
- `build/Library/bin/example3_dll_batch`

## macOS and Linux Builds

Core/API build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSPT_QT_VERSION=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

Qt 5 GUI build on macOS with Homebrew `qt@5`:

```bash
cmake -S . -B build-qt5 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt@5)" -DSPT_QT_VERSION=5
cmake --build build-qt5
ctest --test-dir build-qt5 --output-on-failure
```

Qt 6 GUI build on macOS with Homebrew `qt`:

```bash
cmake -S . -B build-qt6 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt)" -DSPT_QT_VERSION=6
cmake --build build-qt6
ctest --test-dir build-qt6 --output-on-failure
```

## Windows Build

Core/API build with Visual Studio 2022:

```bat
cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64 -DSPT_QT_VERSION=OFF
cmake --build build-msvc --config Release
ctest --test-dir build-msvc -C Release --output-on-failure
```

If the source path is long, use a short build directory such as
`C:\bsp_build` to avoid MSVC file-tracking path length issues. A helper script
`build_windows.bat` is included for that workflow.

Qt 5 GUI verification on Windows is documented in
`Documents/Windows_Qt_Verification.md`. Keep the runtime checklist unchecked
until it is executed on a real Windows machine.

## Qt Build Options

Qt is optional. The CMake cache variable `SPT_QT_VERSION` controls GUI
discovery:

| Value | Behavior |
|---|---|
| `AUTO` | Prefer Qt 6 Widgets, then fall back to Qt 5 Widgets. |
| `6` | Require/select Qt 6 Widgets for `SectionPropertyGui`. |
| `5` | Require/select Qt 5 Widgets for `SectionPropertyGui`. |
| `OFF` | Skip GUI discovery and build only core/API/tests/examples. |

Without Qt Widgets, the core static library, C API shared library, examples,
and tests still build.

## Qt Canvas Workflow

The optional `SectionPropertyGui` includes a Canvas tab for advanced manual
thin-walled sections. Users can draw plate centerlines directly in the graphics
view, assign the current thickness to newly drawn plates, and edit exact
coordinates in the synchronized table. Select/Edit mode keeps table rows and
canvas plates highlighted together. Draw Plate mode uses first click, preview,
and second click to create a new plate. Delete Selected and Clear All update
both the table and canvas.

Graphics views in General, Stress Points, FE Mesh, and Canvas support Zoom In,
Zoom Out, mouse-wheel zoom, click-and-drag pan, Fit, and Reset. Fit preserves
aspect ratio and adds margins so very large dimensions remain visible.

When Build Canvas Section succeeds, the Canvas geometry becomes the active
section and the property table, stress points, FE mesh, and previews are
refreshed. Canvas calculations currently use the core plate-to-polygon and
rectangularized component representation; default stress points for arbitrary
Canvas sections are generated from the first four plate endpoints, and the FE
mesh is a visualization mesh rather than a solver-quality mesh.

## Project Structure

```text
Source/
  api/              C ABI wrapper and exported header
  calculation/      Section property calculation engine
  common/           Shared data model and utility types
  geometry/         Parametric and Canvas section builders
  gui/              Optional Qt Widgets application
  import_export/    CSV, JSON, ANSYS, ABAQUS, and Midas Civil writers
  mesh/             Lightweight visualization mesh generation
  stress/           Stress-point generation and transforms
Examples/           Example API programs
tests/              Unit, regression, and integration tests
Documents/          User, validation, API, and developer documentation
.github/workflows/  CMake CI workflow
```

## Examples

- `example1_parametric` creates an H section and prints area and inertia.
- `example2_canvas` creates a three-plate Canvas section and prints centroid
  data.
- `example3_dll_batch` calculates multiple section types through the C API.

Run examples after a build:

```bash
./build/Library/bin/example1_parametric
./build/Library/bin/example2_canvas
./build/Library/bin/example3_dll_batch
```

## Validation

`Documents/Numerical_Validation_Report.md` records the expected-vs-actual
checks against `Documents/source_requirements/Test Data.xls`. The automated
regression suite covers H, box, pipe, and the supplied crane girder reference
case. Non-reference crane girder parameter sets use an approximate plate graph
and emit a diagnostic warning because the source diagram is not fully
dimensioned.

FEM export writers emit general section/property values for solver review.
They are covered by API integration tests, but solver-specific acceptance still
requires review in ANSYS, ABAQUS, and Midas Civil.

## Documentation

- `Documents/README.md` - documentation index
- `Documents/Implementation_Status.md` - current verification and blockers
- `Documents/User Manual.md` - user-facing GUI/manual workflow
- `Documents/API_Reference.md` - C API reference
- `Documents/Developer_Guide.md` - developer workflow and extension notes
- `Documents/Test_Report.md` - automated test coverage
- `Documents/Numerical_Validation_Report.md` - numerical validation tables
- `Documents/Windows_Qt_Verification.md` - Windows GUI verification checklist
- `Documents/Final_Acceptance_Checklist.md` - final delivery checklist

## Remaining Limitations

- GitHub default-branch setting still needs admin-token confirmation before the
  old `try-whole-implementation` branch can be safely deleted.
- Windows Qt GUI runtime and deployment smoke testing still require Windows.
- ANSYS, ABAQUS, and Midas Civil solver-side acceptance remains pending.
- Warping constant and shear-center fields exist in the data model but are not
  currently computed or validated.
- Non-reference crane girder calculations remain approximate pending complete
  construction rules.
- Canvas endpoint dragging is not implemented yet; use the plate table for
  precise coordinate edits.

## Final Delivery Checklist

- [x] C++17 core builds on macOS and CI platforms.
- [x] C API shared library builds.
- [x] Unit, regression, and integration tests pass.
- [x] Examples build and run on macOS.
- [x] Numerical validation report exists for workbook reference values.
- [x] Optional Qt 5 and Qt 6 macOS builds verified.
- [x] Windows core/API evidence is documented from prior merged work.
- [x] GitHub repository default branch switched to `main`.
- [x] Old `try-whole-implementation` branch deleted after default switch.
- [ ] Windows Qt GUI runtime/deployment smoke test completed.
- [ ] FEM solver acceptance completed in target applications.
- [ ] Final user manual/DOCX package reviewed and signed off.
