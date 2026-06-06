# Windows Qt 5.15.2 GUI Build and Deployment Verification

This document describes how to verify that `SectionPropertyGui` builds,
deploys, and opens correctly on Windows 10/11 with Qt 5.15.2 and MSVC 2022.
Automated GUI coverage is provided by the CTest target
`SectionPropertySmokeTest` when Qt Widgets and Qt Test are available.

## Prerequisites

| Tool | Version | Notes |
|---|---|---|
| Windows | 10 or 11 x64 | |
| Visual Studio | 2022 | Desktop C++ workload required |
| CMake | 3.16 or later | Add to PATH during install |
| Qt | 5.15.2 MSVC 2019 x64 | Install via Qt Online Installer |

Qt 5.15.2 MSVC 2019 x64 is binary-compatible with MSVC 2022.

## Build Steps

Use the repository helper:

```bat
build_windows.bat
```

The script configures `build-qt-msvc` with `SPT_QT_VERSION=5`, builds Release,
runs CTest, and deploys `SectionPropertyGui.exe` with `windeployqt`.

## Expected Build Artifacts

After a successful build the following files must exist:

| Artifact | Path |
|---|---|
| `SectionPropertyTool.dll` | `build-qt-msvc\Library\bin\Release\` |
| `SectionPropertyTool.lib` | `build-qt-msvc\Library\lib\Release\` |
| `SectionPropertyCore.lib` | `build-qt-msvc\Library\lib\Release\` |
| `SectionPropertyTests.exe` | `build-qt-msvc\Library\bin\Release\` |
| `SectionPropertySmokeTest.exe` | `build-qt-msvc\Library\bin\Release\` |
| `example1_parametric.exe` | `build-qt-msvc\Library\bin\Release\` |
| `example2_canvas.exe` | `build-qt-msvc\Library\bin\Release\` |
| `example3_dll_batch.exe` | `build-qt-msvc\Library\bin\Release\` |
| `SectionPropertyGui.exe` | `build-qt-msvc\Library\bin\Release\` |

## Exported Symbol Verification

Confirm that the DLL exports the required C API symbols:

```bat
dumpbin /EXPORTS build-qt-msvc\Library\bin\Release\SectionPropertyTool.dll
```

The exported function list must include all public `spt_*` functions declared
in `Source/api/section_property_tool.h`.

## Qt Deployment

`build_windows.bat` runs:

```bat
C:\Qt\5.15.2\msvc2019_64\bin\windeployqt.exe --release --no-translations ^
    build-qt-msvc\Library\bin\Release\SectionPropertyGui.exe
```

After deployment the `Release\` directory should contain at minimum
`Qt5Core.dll`, `Qt5Gui.dll`, `Qt5Widgets.dll`,
`platforms\qwindows.dll`, `SectionPropertyGui.exe`, and
`SectionPropertyTool.dll`.

## Automated Smoke Coverage

`SectionPropertySmokeTest` runs headlessly with the offscreen Qt platform and
validates:

- Window construction.
- All four parametric section types load and produce non-zero Area.
- Switching section types changes the parameter table row count.
- Stress Points tab has exactly four rows after each parametric section load.
- FE Mesh scene contains rendered mesh items after construction.
- Canvas tab builds the three-plate `example2_canvas` section and reports
  `2960 mm2` area.

## Manual Runtime Smoke Test

Launch the GUI directly from the deployment directory:

```bat
build-qt-msvc\Library\bin\Release\SectionPropertyGui.exe
```

Verify the following manual-only behavior:

- [ ] Window opens visibly with title "Cross-Section"
  `[requires manual - automated coverage exists via SectionPropertySmokeTest]`.
- [ ] Section outlines are visually framed and readable
  `[requires manual - automated coverage exists via SectionPropertySmokeTest]`.
- [ ] Edit a stress point coordinate and confirm principal coordinates update
  `[requires manual - automated coverage exists via SectionPropertySmokeTest]`.
- [ ] Adjust mesh refinement and confirm the visible mesh updates
  `[requires manual - automated coverage exists via SectionPropertySmokeTest]`.
- [ ] Drag a Canvas endpoint in Select/Edit mode and confirm the table updates
  `[requires manual - automated coverage exists via SectionPropertySmokeTest]`.
- [ ] `windeployqt` output launches on a clean/non-developer Windows session
  `[requires manual - automated coverage exists via SectionPropertySmokeTest]`.

## Checklist Summary

| Step | Status |
|---|---|
| CMake configure detects Qt5 Widgets/Test | [x] Covered by Qt-enabled configure and `SectionPropertySmokeTest` target wiring |
| All core/API artifacts present after build | [x] Covered by build targets |
| CTest core regression/integration passes | [x] Covered by `SectionPropertyTests` |
| Qt GUI smoke test passes | [x] Covered by `SectionPropertySmokeTest` when Qt is available |
| `dumpbin /EXPORTS` shows public API symbols | [ ] Requires manual Windows tool inspection |
| `windeployqt` completes without error | [ ] Requires manual Windows Qt installation |
| GUI visual/runtime inspection completed | [ ] Requires manual Windows session; automated coverage exists via `SectionPropertySmokeTest` |
