# Implementation Status

This branch is a reviewable first whole implementation of the Beam Element
Section Property Tool. It is not a final acceptance-ready release.

## Implemented

- C++17 core data model for section parameters, plate segments, contours,
  materials, properties, stress points, mesh settings, meshes, calculation
  results, and errors.
- Geometry builders for H Section, Box Section, Pipe Section, Quayside Crane
  Girder, and Canvas thin-walled plate centerline sections.
- Calculation core for area, centroid, Jy, Jz, Jyz, principal axes, theta, Jx,
  Az, and Ay, with regression-compatible formulas for the supplied H, Box, and
  Pipe cases.
- A reference-compatible Quayside Crane Girder path for the supplied
  `Test Data.xls` case, plus an approximate plate-graph path for visualization
  and non-reference inputs.
- Stress point generation and coordinate transforms between global Y-Z and
  centroidal principal Y0-Z0 systems.
- Lightweight triangular mesh visualization data for rectangles and pipe
  sections.
- C-compatible DLL/API layer with opaque handles, exported structs, error
  reporting, stress point arrays, mesh counts, and CSV/JSON result export.
- Optional Qt Widgets GUI target with General, Stress Points, FE Mesh, and
  Canvas tabs.
- Example programs for parametric use, Canvas use, and batch DLL-style use.
- Regression fixture extracted from `Test Data.xls` and source requirement files
  preserved under `Documents/source_requirements/`.
- Documentation drafts for README, architecture, DLL interface, user manual, and
  test report.

## Tested Successfully

- Core/API/tests/examples were verified with the available local macOS toolchain
  using direct `c++ -std=c++17` compilation.
- The local direct test executable passed the recovered spreadsheet regression
  cases for H, Hollow Box, Pipe, and Crane Runway sections.
- The DLL-style C API smoke test passed for create/calculate/read stress
  points/create mesh/export CSV.
- The three example programs compiled and ran locally.
- `Documents/User Manual.docx` was generated as a valid DOCX archive.

## Not Verified Yet

- CMake configure/build/install was not verified on this Apple/macOS machine
  because `cmake` is not installed in the local environment used for this pass.
- Windows DLL packaging/export, import-library generation, and deployment remain
  unverified and must be tested on a Windows 10/11 machine.
- The Qt 5.15.2 GUI target remains unverified with a proper Qt/CMake setup.
- FEM export formats for ANSYS, ABAQUS, and Midas Civil are currently reserved
  placeholders and still need acceptance testing against the expected target
  software/cards.
- Numerical validation still needs a clear expected-vs-actual report generated
  from `Test Data.xls` and/or known analytical or commercial-software references.
- The Quayside Crane Girder requires extra engineering validation because the
  provided PDF diagram is not fully dimensioned. The current implementation
  includes a reference-compatible path for the supplied XLS case rather than a
  fully validated general crane-girder solver.

## Remaining Next Steps

- Run CMake configure/build/test/install on Windows with MSVC and Qt 5.15.2.
- Verify DLL ABI, exported symbols, import library, and example linking on
  Windows.
- Produce an expected-vs-actual numerical test report from the regression
  fixture and any additional analytical/commercial references.
- Replace the lightweight mesh visualization path with a robust constrained
  meshing backend if production Canvas meshing accuracy is required.
- Confirm the full Quayside Crane Girder construction rules and remove or
  generalize the reference-only calculation path.
- Implement and acceptance-test FEM card exporters for ANSYS, ABAQUS, and Midas
  Civil.

