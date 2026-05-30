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
- DLL API smoke test: create section, calculate, read properties, read stress
  points, create mesh, export CSV.

## Verification Run

Executed in the development environment using the system C++ compiler because
`cmake` is not installed on this machine:

```text
c++ -std=c++17 ... -o /tmp/SectionPropertyTests
/tmp/SectionPropertyTests
SectionPropertyTests passed
```

The three example programs were also compiled directly against the core/API
sources and executed successfully.

## Tolerances

- Coordinates, area, and shear area: `max(0.01, 1e-6 * abs(expected))`.
- Inertia and torsion: `max(1.0, 1e-6 * abs(expected))`.
- Principal-axis angle: `1e-8 rad`.

## Known Limitations

- GUI automation is manual unless Qt Test is added to the CI environment.
- Crane girder non-reference parameter sets use an approximate plate graph until
  the client confirms full construction rules.
- FEM card exports are placeholders in v1.
