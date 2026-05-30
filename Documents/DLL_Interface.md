# DLL Interface

Public API header: `Library/include/section_property_tool.h`.

The DLL exposes a C-compatible ABI with opaque handles:

- `SptSectionHandle`
- `SptResultHandle`
- `SptMeshHandle`

The caller owns handles returned by the API and must destroy them with:

- `spt_destroy_section`
- `spt_destroy_result`
- `spt_destroy_mesh`

Arrays returned by stress point functions must be released with
`spt_free_stress_point_array`.

## Basic Workflow

```cpp
SptParameter values[] = {
    {"A", 100.0}, {"H", 210.0}, {"e", 20.0}, {"f", 12.0}
};
SptSectionParameters params{SPT_H_SECTION, values, 4};

SptSectionHandle section = nullptr;
spt_create_section_from_parameters(&params, &section);

SptResultHandle result = nullptr;
spt_calculate_section_properties(section, &result);

SptSectionProperties props{};
spt_get_result_properties(result, &props);

spt_destroy_result(result);
spt_destroy_section(section);
```

## Error Handling

All functions return `0` on success. Nonzero values indicate failure. Use
`spt_get_last_error()` to retrieve a thread-local `SptErrorInfo`.

## Thread Safety

Independent handles may be used from independent threads. Mutating the same
section handle from multiple threads is not supported.

## Export

`spt_export_results` supports CSV and JSON in this release. ANSYS, ABAQUS, and
Midas Civil export enum values are reserved placeholders and return a documented
warning.

