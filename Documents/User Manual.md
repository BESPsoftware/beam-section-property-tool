# Beam Element Section Property Tool - User Manual

This manual describes the intended workflow for the optional Qt GUI and the
result/export behavior exposed through the shared-library API.

## Overview

Beam Section Property Tool calculates cross-section properties for common beam
sections and user-defined thin-walled plate sections. It reports area, second
moments of area, torsion-related values, shear areas, centroid coordinates,
principal-axis orientation, stress output points, and a lightweight mesh summary.

Geometry inputs are in millimeters. Calculations use double precision
internally. GUI tables may display rounded values for readability.

## General Tab

Use the General tab to choose a section type, edit its dimensions, and review
calculated properties.

Typical workflow:

1. Select a section type.
2. Edit parameter values.
3. Press Apply, or allow the view to update.
4. Review the section drawing and property table.
5. Use the other tabs to inspect stress points, mesh, Canvas input, or export
   output.

The property table displays:

| Property | Meaning | Unit |
|---|---|---|
| `Area` | Cross-section area | mm2 |
| `Jz` | Second moment about global z axis | mm4 |
| `Jy` | Second moment about global y axis | mm4 |
| `Jyz` | Product of inertia | mm4 |
| `Jzo` | Principal second moment about z0 | mm4 |
| `Jyo` | Principal second moment about y0 | mm4 |
| `Jx` | Torsion-related constant used by the tool | mm4 |
| `Az` | Shear area in z direction | mm2 |
| `Ay` | Shear area in y direction | mm2 |
| `cy` | Centroid y coordinate | mm |
| `cz` | Centroid z coordinate | mm |
| `theta` | Principal-axis angle | rad |

## Section Parameters

Parameter names are shown exactly as the calculation API expects them.

| Section | Parameters |
|---|---|
| H Section | `A`, `H`, `e`, `f` |
| Box Section | `A`, `B`, `H`, `D`, `E`, `H1`, `D1`, `E1` |
| Pipe Section | `Do`, `t` |
| Quayside Crane Girder | `A`, `B`, `G`, `D`, `e`, `f`, `H`, `W`, `M`, `N`, `p`, `s`, `t`, `u`, `M1`, `k`, `k1`, `h`, `h1` |

All listed dimensions must be positive unless a specific section rule states
otherwise. Invalid or impossible combinations produce diagnostics rather than
silently continuing.

### Practical H Section Example

Use these dimensions to reproduce the workbook H-section reference case:

| Parameter | Value |
|---|---:|
| `A` | 100 |
| `H` | 210 |
| `e` | 20 |
| `f` | 12 |

Expected area is `6520 mm2`.

### Practical Pipe Example

Use these dimensions to reproduce the workbook pipe reference case:

| Parameter | Value |
|---|---:|
| `Do` | 1300 |
| `t` | 14 |

The pipe origin is at the pipe center.

## Stress Points Tab

The Stress Points tab lists stress output locations. Each row includes:

- Point ID and label.
- Global `y` and `z` coordinates.
- Principal `y0` and `z0` coordinates.
- Source and validity indicators.

Default stress points are generated from section-specific output locations.
Editing global `y` or `z` updates the principal coordinates and redraws markers
in the graphics view. Use Reset Defaults when you want to discard manual edits
and return to the generated points.

Stress points are output locations, not mesh nodes. A point can be valid for
stress reporting even when the visualization mesh has a different density.

## FE Mesh Tab

The FE Mesh tab displays the lightweight triangular visualization mesh generated
from the current section geometry.

Use the refinement factor to request a denser or coarser display mesh. The mesh
updates when the active section geometry changes.

Current limitation: this mesh is intended for visualization and summary counts.
It is not a full solver mesh export.

## Canvas Tab

The Canvas tab builds user-defined thin-walled sections from plate centerlines.
Each row describes one plate segment:

| Field | Meaning |
|---|---|
| Start `y`, start `z` | Centerline start coordinate |
| End `y`, end `z` | Centerline end coordinate |
| Thickness | Plate thickness in millimeters |
| Material ID | Numeric material identifier for future expansion |
| ID | User-readable segment label |

Typical workflow:

1. Add or edit plate rows.
2. Ensure every segment has positive length and positive thickness.
3. Build the Canvas section.
4. Review the General tab properties and drawing.
5. Inspect stress points and mesh as needed.

### Practical Canvas Example

A simple U-like thin-walled section can be built with:

| Segment | Start | End | Thickness |
|---|---|---|---:|
| bottom | `(0, 0)` | `(100, 0)` | 10 |
| web | `(50, 0)` | `(50, 120)` | 8 |
| top | `(0, 120)` | `(100, 120)` | 10 |

The example program `example2_canvas` uses this geometry and reports an area of
`2960 mm2` with centroid `(50, 60) mm`.

## Export Behavior

The shared-library API can export calculated results in these formats:

| Format | API enum | Notes |
|---|---|---|
| CSV | `SPT_EXPORT_CSV` | Property table and stress point rows. |
| JSON | `SPT_EXPORT_JSON` | Properties, stress points, and mesh summary. |
| ANSYS | `SPT_EXPORT_ANSYS` | General beam-section constants. |
| ABAQUS | `SPT_EXPORT_ABAQUS` | General beam-section values. |
| Midas Civil | `SPT_EXPORT_MIDAS_CIVIL` | General section/property values. |

The FEM writers are intended for solver review. They are covered by API
integration tests for generated content, but acceptance in ANSYS, ABAQUS, and
Midas Civil still needs to be confirmed in those applications before production
use.

## Validation Notes

The numerical validation report compares expected and actual values from the
source workbook for H, box, pipe, and the supplied crane girder reference case.

For the crane girder, the supplied workbook reference case is calibrated to the
XLS data. Other crane girder parameter sets use an approximate plate graph and
emit a warning because the source diagram is not fully dimensioned for arbitrary
inputs.

## Common Issues

| Symptom | Check |
|---|---|
| A section fails to build | Confirm all required parameters are present and positive. |
| Pipe fails to build | Confirm `t` is less than half of `Do`. |
| Box section reports geometry errors | Confirm outer dimensions leave enough room for side web thicknesses. |
| Crane girder warning appears | Non-reference crane dimensions use the approximate plate graph. |
| Export fails | Confirm the target path is writable. |
| GUI target is not built | Confirm Qt is installed and `SPT_QT_VERSION` is not `OFF`. |
