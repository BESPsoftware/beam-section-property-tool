# C API Reference

The public C API is declared in `Source/api/section_property_tool.h`. It uses
opaque handles, plain C structs, and explicit destroy/free functions so the
shared library can be consumed from C, C++, and foreign-function interfaces.

All API functions return `0` on success. Nonzero return values indicate failure;
call `spt_get_last_error()` for diagnostic details.

## Header and Library

```c
#include "section_property_tool.h"
```

Build target:

- CMake target: `SectionPropertyApi`
- Output name: `SectionPropertyTool`
- macOS/Linux shared library: `libSectionPropertyTool.dylib` or `.so`
- Windows shared library/import library: `SectionPropertyTool.dll` and
  `SectionPropertyTool.lib`

## Handle Lifecycle

| Handle | Created by | Destroyed by |
|---|---|---|
| `SptSectionHandle` | `spt_create_section_from_parameters`, `spt_create_section_from_canvas_lines` | `spt_destroy_section` |
| `SptResultHandle` | `spt_calculate_section_properties` | `spt_destroy_result` |
| `SptMeshHandle` | `spt_create_mesh` | `spt_destroy_mesh` |
| `SptStressPointArray` | `spt_get_result_stress_points`, `spt_generate_default_stress_points` | `spt_free_stress_point_array` |

Destroy handles once when finished. Do not use a handle after its destroy
function has been called.

## Version

```c
int spt_get_version(int* major, int* minor, int* patch);
```

Writes the API version components when the pointers are non-null.

## Section Types

```c
typedef enum SptSectionType {
    SPT_H_SECTION = 0,
    SPT_BOX_SECTION = 1,
    SPT_PIPE_SECTION = 2,
    SPT_CRANE_GIRDER = 3,
    SPT_CANVAS = 4
} SptSectionType;
```

Parameter names are case-sensitive UTF-8 strings.

| Section | Required parameters |
|---|---|
| `SPT_H_SECTION` | `A`, `H`, `e`, `f` |
| `SPT_BOX_SECTION` | `A`, `B`, `H`, `D`, `E`, `H1`, `D1`, `E1` |
| `SPT_PIPE_SECTION` | `Do`, `t` |
| `SPT_CRANE_GIRDER` | `A`, `B`, `G`, `D`, `e`, `f`, `H`, `W`, `M`, `N`, `p`, `s`, `t`, `u`, `M1`, `k`, `k1`, `h`, `h1` |
| `SPT_CANVAS` | Created from plate segments, not string parameters. |

All geometric inputs are in millimeters.

## Section Creation From Parameters

```c
typedef struct SptParameter {
    const char* name;
    double value;
} SptParameter;

typedef struct SptSectionParameters {
    SptSectionType type;
    const SptParameter* parameters;
    size_t parameter_count;
} SptSectionParameters;

int spt_create_section_from_parameters(
    const SptSectionParameters* params,
    SptSectionHandle* out);
```

Creates a parametric section. On success, `*out` owns a new section handle.
Destroy it with `spt_destroy_section`.

## Canvas Section Creation

```c
typedef struct SptPointYZ {
    double y;
    double z;
} SptPointYZ;

typedef struct SptPlateSegment {
    SptPointYZ start;
    SptPointYZ end;
    double thickness;
    int material_id;
    const char* id;
} SptPlateSegment;

int spt_create_section_from_canvas_lines(
    const SptPlateSegment* lines,
    size_t count,
    SptSectionHandle* out);
```

Creates a thin-walled Canvas section from plate centerline segments. Each
segment must have positive length and positive thickness.

## Calculation

```c
int spt_calculate_section_properties(
    SptSectionHandle section,
    SptResultHandle* out);
```

Calculates section properties, default stress points, and mesh summary data for
the supplied section. Destroy the result with `spt_destroy_result`.

## Property Results

```c
typedef struct SptSectionProperties {
    double area;
    double Jz;
    double Jy;
    double Jyz;
    double Jzo;
    double Jyo;
    double Jx;
    double Az;
    double Ay;
    double cy;
    double cz;
    double theta;
    double warping_constant;
    double shear_center_y;
    double shear_center_z;
} SptSectionProperties;

int spt_get_result_properties(
    SptResultHandle result,
    SptSectionProperties* out);
```

Units:

- `area`, `Az`, `Ay`: mm2.
- `Jz`, `Jy`, `Jyz`, `Jzo`, `Jyo`, `Jx`: mm4.
- `cy`, `cz`: mm.
- `theta`: radians.
- `warping_constant`: mm6.
- `shear_center_y`, `shear_center_z`: mm.

For closed doubly symmetric sections such as Box and Pipe, `warping_constant = 0.0` is the exact Vlasov value, not a placeholder. H sections use the analytical equal-flange formula; crane girder and Canvas sections use numerical open thin-walled sectorial-area integration.

## Stress Points

```c
typedef struct SptStressPoint {
    int id;
    char label[32];
    double y;
    double z;
    double y0;
    double z0;
    int source;
    int validity;
} SptStressPoint;

typedef struct SptStressPointArray {
    SptStressPoint* points;
    size_t count;
} SptStressPointArray;

int spt_get_result_stress_points(
    SptResultHandle result,
    SptStressPointArray* out);

int spt_generate_default_stress_points(
    SptSectionHandle section,
    SptStressPointArray* out);

int spt_update_stress_points(
    SptSectionHandle section,
    const SptStressPoint* points,
    size_t count);

void spt_free_stress_point_array(SptStressPointArray* array);
```

`y` and `z` are global coordinates. `y0` and `z0` are principal coordinates.
Free returned arrays with `spt_free_stress_point_array`.

## Mesh API

```c
typedef struct SptMeshSettings {
    double target_size;
    double refinement_factor;
    int curved_segment_count;
    int auto_update;
} SptMeshSettings;

typedef struct SptMeshCounts {
    size_t node_count;
    size_t triangle_count;
    size_t boundary_edge_count;
} SptMeshCounts;

int spt_create_mesh(
    SptSectionHandle section,
    const SptMeshSettings* settings,
    SptMeshHandle* out);

int spt_get_mesh_counts(
    SptMeshHandle mesh,
    SptMeshCounts* out);

void spt_destroy_mesh(SptMeshHandle mesh);
```

The mesh API currently exposes counts for the lightweight visualization mesh.
It is not a full FEM mesh export API.

## Export API

```c
typedef enum SptExportFormat {
    SPT_EXPORT_CSV = 0,
    SPT_EXPORT_JSON = 1,
    SPT_EXPORT_ANSYS = 2,
    SPT_EXPORT_ABAQUS = 3,
    SPT_EXPORT_MIDAS_CIVIL = 4
} SptExportFormat;

int spt_export_results(
    SptResultHandle result,
    const char* path_utf8,
    SptExportFormat format);
```

Supported formats:

| Format | Output |
|---|---|
| `SPT_EXPORT_CSV` | Property table and stress points. |
| `SPT_EXPORT_JSON` | Properties, stress points, and mesh summary. |
| `SPT_EXPORT_ANSYS` | General ANSYS beam-section constants. |
| `SPT_EXPORT_ABAQUS` | General ABAQUS beam-section values. |
| `SPT_EXPORT_MIDAS_CIVIL` | General Midas Civil section/property values. |

FEM card writers are covered by integration tests for generated content, but
solver-side acceptance remains pending in the target applications.

## Error Handling

```c
typedef struct SptErrorInfo {
    int code;
    int severity;
    char field[64];
    char message[256];
    char remediation[256];
} SptErrorInfo;

const SptErrorInfo* spt_get_last_error(void);
```

The last error is thread-local in the implementation. Inspect it immediately
after a nonzero return code.

## Minimal Example

```c
#include "section_property_tool.h"

#include <stdio.h>

int main(void) {
    const SptParameter values[] = {
        {"A", 100.0},
        {"H", 210.0},
        {"e", 20.0},
        {"f", 12.0},
    };

    SptSectionParameters params = {SPT_H_SECTION, values, 4};
    SptSectionHandle section = NULL;
    if (spt_create_section_from_parameters(&params, &section) != 0) {
        printf("create failed: %s\n", spt_get_last_error()->message);
        return 1;
    }

    SptResultHandle result = NULL;
    if (spt_calculate_section_properties(section, &result) != 0) {
        printf("calculate failed: %s\n", spt_get_last_error()->message);
        spt_destroy_section(section);
        return 1;
    }

    SptSectionProperties props;
    if (spt_get_result_properties(result, &props) == 0) {
        printf("Area = %.3f mm2\n", props.area);
        printf("Jz = %.3f mm4\n", props.Jz);
        printf("Jy = %.3f mm4\n", props.Jy);
        printf("Cw = %.3f mm6\n", props.warping_constant);
        printf("Shear center = (%.3f, %.3f) mm\n", props.shear_center_y, props.shear_center_z);
    }

    spt_export_results(result, "section.csv", SPT_EXPORT_CSV);

    spt_destroy_result(result);
    spt_destroy_section(section);
    return 0;
}
```
