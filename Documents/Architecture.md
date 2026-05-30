# Architecture

## Module Division

- `Source/geometry`: parametric and Canvas section builders, plate and contour
  construction, coordinate primitives.
- `Source/calculation`: area, centroid, inertia, principal axes, torsion and
  shear-area strategies.
- `Source/stress`: default stress point generation, coordinate transforms, and
  validation.
- `Source/mesh`: lightweight T3 visualization mesh generation.
- `Source/import_export`: CSV/JSON result export and project save support.
- `Source/api`: stable C-compatible DLL API.
- `Source/gui`: Qt Widgets user interface.

## Data Flow

GUI or DLL input is converted into `SectionParameters` or `PlateSegment` values.
`SectionBuilder` validates the inputs and creates a `SectionModel`. The
calculation core returns `SectionProperties`; the stress engine creates output
points using those properties; and the mesh engine creates visualization data.

```text
GUI/API input
  -> SectionParameters / PlateSegment[]
  -> SectionBuilder
  -> SectionModel
  -> SectionCalculator
  -> StressPointEngine
  -> MeshEngine
  -> GUI/API/Exporter
```

## Calculation Strategy

H, Box, and Pipe sections use exact analytical formulas that match the supplied
spreadsheet. Canvas sections and non-reference crane sections use plate-based
rectangle approximations for geometric properties and visualization meshes.

Principal properties are derived from the centroidal inertia tensor:

```text
theta = 0.5 * atan2(-2 * Jyz, Jy - Jz)
Jzo/Jyo = eigenvalues of the centroidal inertia tensor
```

## Crane Girder Mapping

The quayside crane girder parameters are mapped to a named plate graph following
the visible labels in the requirements diagram: top plate, bottom plate, left
web, internal web, sloped web, right bracket, and local stiffeners. Because the
PDF diagram does not fully define every construction coordinate, the supplied
XLS reference case is treated as authoritative for numerical acceptance.

For the exact XLS parameter set, the builder stores the reference properties and
stress points recovered from the workbook. For other parameter sets, the same
named plate graph is used as an approximate engineering model and emits a
warning through calculation diagnostics.

## Extension Points

- Replace the current lightweight mesher with a constrained Delaunay backend.
- Add multi-cell torsion and shear-flow solvers.
- Implement ANSYS, ABAQUS, and Midas Civil card writers behind the existing
  export enum.
- Add Qt Test automation for repeatable GUI smoke tests.

