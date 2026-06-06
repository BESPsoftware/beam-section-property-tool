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
| `Cw` | Warping constant | mm6 |
| `ys` | Shear center y coordinate | mm |
| `zs` | Shear center z coordinate | mm |

## Graphics View Navigation

The General, Stress Points, FE Mesh, and Canvas graphics views support the same
navigation controls:

- Zoom In and Zoom Out buttons.
- Mouse-wheel zoom centered near the pointer.
- Click-and-drag panning.
- Fit, which preserves aspect ratio and adds a margin around the full geometry.
- Reset, which returns to an unscaled view centered on the scene.

The GUI automatically fits the full section after a parametric recalculation or
successful Canvas build. Use Fit again after editing very large coordinates or
after panning/zooming away from the geometry.

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

The Canvas tab is the advanced graphical input mode for user-defined
thin-walled sections. Draw each plate by its centerline and assign a thickness
to that centerline. The table remains available for precise engineering
coordinate edits.

The left side contains the editable plate table:

| Field | Meaning |
|---|---|
| `y1`, `z1` | Centerline start coordinate |
| `y2`, `z2` | Centerline end coordinate |
| `t` | Plate thickness in millimeters |
| ID | User-readable segment label |

The right side is the drawing canvas. Plates are shown as a centerline plus a
simple thickness polygon and a `t=` label.

### Canvas Tools

- Select/Edit selects an existing plate. Selecting a plate highlights the
  matching table row; selecting a table row highlights the matching plate.
- Draw Plate creates a new centerline: first click sets the start point, mouse
  move previews the line, and second click sets the end point.
- New plate thickness controls the thickness assigned to newly added or drawn
  plates.
- New plate id can be left blank for automatic `plate_N` ids.
- Add Plate inserts a default editable plate row.
- Delete Selected removes the selected plate from both the table and canvas.
- Clear All removes all Canvas plates.
- Build Canvas Section validates and builds the current plate list without
  leaving the Canvas tab.
- Show Grid toggles the drawing grid.
- Snap to Grid snaps Draw Plate clicks to the grid.

Editing table coordinates, thickness, or ids redraws the canvas immediately.
In Select/Edit mode, drag a highlighted endpoint to move it. Click within about 8 screen pixels of either endpoint, move the mouse, and release to write the new coordinate back to the matching table cells. Enable Snap to Grid to constrain dragged endpoints to grid intersections.

### Build Validation

Build Canvas Section checks that:

- At least one plate exists.
- Coordinates and thickness values are numeric.
- Thickness is positive.
- Start and end coordinates do not define a zero-length plate.
- Explicit ids are unique; blank ids are generated automatically.

If validation fails, the active section results are not changed.

If validation succeeds, the Canvas-built section becomes the active model:
properties, default stress points, the General preview, Stress Points preview,
and FE Mesh preview all update from the Canvas geometry. The Canvas view then
fits the full drawn section.

### Current Canvas Calculation Scope

Canvas properties are calculated through the current core thin-walled plate
representation. Plate polygons are generated from centerline plus thickness,
and the property/mesh path uses the resulting rectangularized plate
components. Open-section torsion uses the sum of `length * t^3 / 3` for the
plates. Stress points for arbitrary Canvas sections are the first four plate
endpoints generated by the default stress-point engine.

Shear center and warping constant are computed for Canvas sections using the open thin-walled sectorial-area method. The FE mesh is a lightweight visualization mesh, not a solver-quality mesh.

### Typical Workflow

1. Add or edit plate rows.
2. Or choose Draw Plate and click start/end centerline points on the canvas.
3. Use Select/Edit to verify row and canvas selection stay synchronized.
4. Edit exact coordinates or thickness in the table as needed.
5. Press Build Canvas Section.
6. Review properties, stress points, and mesh as needed.

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
