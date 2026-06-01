# Windows Qt 5.15.2 GUI Build and Deployment Verification

This document describes how to verify that `SectionPropertyGui` builds
and deploys correctly on Windows 10/11 with Qt 5.15.2 and MSVC 2022.

## Prerequisites

| Tool | Version | Notes |
|---|---|---|
| Windows | 10 or 11 (x64) | |
| Visual Studio | 2022 (any edition) | Desktop C++ workload required |
| CMake | 3.16 or later | Add to PATH during install |
| Qt | 5.15.2 (MSVC 2019 x64) | Install via Qt Online Installer |

Qt 5.15.2 MSVC 2019 x64 is binary-compatible with MSVC 2022. The
`Qt5_DIR` variable must point to the `lib/cmake/Qt5` subfolder of the
Qt installation.

---

## Build steps

Open a **Developer Command Prompt for VS 2022** (x64). Adjust
`QT5_DIR` to match your Qt installation path.

```bat
set QT5_DIR=C:\Qt\5.15.2\msvc2019_64\lib\cmake\Qt5

cmake -S . -B build-qt-msvc ^
      -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_PREFIX_PATH=%QT5_DIR%

cmake --build build-qt-msvc --config Release
ctest --test-dir build-qt-msvc -C Release --output-on-failure
```

If the source path is long, use `build_windows.bat` which maps a short
drive letter automatically, then rerun with `-DCMAKE_PREFIX_PATH`.

---

## Expected build artifacts

After a successful build the following files must exist:

| Artifact | Path | Notes |
|---|---|---|
| `SectionPropertyTool.dll` | `build-qt-msvc\Library\bin\Release\` | C API shared library |
| `SectionPropertyTool.lib` | `build-qt-msvc\Library\lib\Release\` | Import library |
| `SectionPropertyCore.lib` | `build-qt-msvc\Library\lib\Release\` | Static core |
| `SectionPropertyTests.exe` | `build-qt-msvc\Library\bin\Release\` | |
| `example1_parametric.exe` | `build-qt-msvc\Library\bin\Release\` | |
| `example2_canvas.exe` | `build-qt-msvc\Library\bin\Release\` | |
| `example3_dll_batch.exe` | `build-qt-msvc\Library\bin\Release\` | |
| `SectionPropertyGui.exe` | `build-qt-msvc\Library\bin\Release\` | Qt GUI (only if Qt5 found) |

Confirm Qt5 was detected during configure by checking for:

```
-- Found Qt5 Widgets: ...
```

in the CMake output. If the line reads:

```
-- Qt5 Widgets not found. Core library, DLL, examples, and tests will still build.
```

then `CMAKE_PREFIX_PATH` was not set correctly.

---

## Exported symbol verification

Confirm that the DLL exports the required C API symbols:

```bat
dumpbin /EXPORTS build-qt-msvc\Library\bin\Release\SectionPropertyTool.dll
```

The following symbols must appear in the output:

```
spt_calculate_section_properties
spt_create_mesh
spt_create_section_from_canvas_lines
spt_create_section_from_parameters
spt_destroy_mesh
spt_destroy_result
spt_destroy_section
spt_export_results
spt_free_stress_point_array
spt_generate_default_stress_points
spt_get_last_error
spt_get_mesh_counts
spt_get_result_properties
spt_get_result_stress_points
spt_get_version
spt_update_stress_points
```

---

## Qt deployment

`SectionPropertyGui.exe` requires Qt DLLs at runtime. Use
`windeployqt` to copy them into the same directory:

```bat
set QT_BIN=C:\Qt\5.15.2\msvc2019_64\bin
%QT_BIN%\windeployqt.exe ^
    --release ^
    --no-translations ^
    build-qt-msvc\Library\bin\Release\SectionPropertyGui.exe
```

After deployment the `Release\` directory must contain at minimum:

```
Qt5Core.dll
Qt5Gui.dll
Qt5Widgets.dll
platforms\qwindows.dll
SectionPropertyGui.exe
SectionPropertyTool.dll
```

---

## Runtime smoke test

Launch the GUI directly from the deployment directory:

```bat
build-qt-msvc\Library\bin\Release\SectionPropertyGui.exe
```

Verify the following:

- [ ] Window opens with title "Cross-Section", width ~980 px.
- [ ] **General tab** — "H Section" loads with default parameters (A=100, H=210, e=20, f=12). Section outline renders in the viewport.
- [ ] Change section type to "Box Section" — parameters table updates, section outline redraws.
- [ ] Change section type to "Pipe Section" — circular outline renders.
- [ ] Change section type to "Quayside Crane Girder" — outline renders.
- [ ] **Stress Points tab** — 4 rows visible with y/z/y0/z0 coordinates. Stress point markers visible on section.
- [ ] Edit a stress point coordinate — principal coordinates update automatically.
- [ ] "Reset Defaults" button restores original coordinates.
- [ ] **FE Mesh tab** — triangular mesh renders for the current section.
- [ ] Adjust refinement factor — mesh updates.
- [ ] **Canvas tab** — default 3-row plate table visible. "Build Canvas Section" switches to General tab and shows a thin-walled U-shape.
- [ ] No crash or error dialog during any of the above steps.

---

## Checklist summary

| Step | Status |
|---|---|
| CMake configure detects Qt5 Widgets | ☐ |
| All artifacts present after build | ☐ |
| CTest passes (1/1) | ☐ |
| `dumpbin /EXPORTS` shows all 16 API symbols | ☐ |
| `windeployqt` completes without error | ☐ |
| GUI opens without crash | ☐ |
| All 4 section types render correctly | ☐ |
| Stress Points tab functional | ☐ |
| FE Mesh tab functional | ☐ |
| Canvas tab functional | ☐ |
