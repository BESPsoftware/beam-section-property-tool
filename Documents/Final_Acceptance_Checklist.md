# Final Acceptance Checklist

Use this checklist before tagging or handing off a final release. Leave items
unchecked until the exact verification has been performed.

## Branch and Repository

| Item | Status | Evidence / notes |
|---|---|---|
| Remote `main` branch exists | [x] | Created from latest `try-whole-implementation`. |
| GitHub default branch is `main` | [ ] | Blocked by GitHub repo-settings permissions in this pass. |
| Old `try-whole-implementation` branch deleted | [ ] | Do not delete until GitHub default is confirmed as `main`. |
| GitHub Actions target `main` | [x] | `.github/workflows/cmake.yml` push trigger updated to `main`. |

## Core Build

| Item | Status | Evidence / notes |
|---|---|---|
| CMake configure succeeds without Qt | [x] | `-DSPT_QT_VERSION=OFF`. |
| Core static library builds | [x] | `SectionPropertyCore`. |
| C API shared library builds | [x] | `SectionPropertyApi`, output name `SectionPropertyTool`. |
| CTest passes locally | [x] | `SectionPropertyTests`, `1/1 tests passed` in recent macOS runs. |
| macOS CI passes | [x] | GitHub Actions no-GUI workflow. |
| Ubuntu CI passes | [x] | GitHub Actions no-GUI workflow. |

## API Shared Library

| Item | Status | Evidence / notes |
|---|---|---|
| Public C header is available | [x] | `Source/api/section_property_tool.h`. |
| Opaque handle lifecycle is documented | [x] | `API_Reference.md`. |
| CSV/JSON export API tested | [x] | API integration test. |
| FEM export API tested for generated content | [x] | API integration test checks ANSYS, ABAQUS, Midas Civil output. |
| Windows DLL/import library evidence exists | [x] | Prior merged Windows verification evidence. |

## Examples

| Item | Status | Evidence / notes |
|---|---|---|
| `example1_parametric` builds | [x] | CMake target. |
| `example1_parametric` runs | [x] | Recent macOS and CI-style local run. |
| `example2_canvas` builds | [x] | CMake target. |
| `example2_canvas` runs | [x] | Recent macOS and CI-style local run. |
| `example3_dll_batch` builds | [x] | CMake target. |
| `example3_dll_batch` runs | [x] | Recent macOS and CI-style local run. |

## Numerical Validation

| Item | Status | Evidence / notes |
|---|---|---|
| H section workbook checks pass | [x] | `Numerical_Validation_Report.md`. |
| Box section workbook checks pass | [x] | `Numerical_Validation_Report.md`. |
| Pipe section workbook checks pass | [x] | `Numerical_Validation_Report.md`. |
| Supplied crane girder workbook checks pass | [x] | `Numerical_Validation_Report.md`. |
| Non-reference crane girder behavior documented | [x] | Approximate plate graph with warning. |
| Non-reference crane girder fully accepted | [ ] | Requires confirmed construction rules beyond supplied XLS/PDF. |

## Qt GUI

| Item | Status | Evidence / notes |
|---|---|---|
| macOS Qt 5 build passes | [x] | Homebrew `qt@5`, `SPT_QT_VERSION=5`. |
| macOS Qt 6 build passes | [x] | Homebrew `qt`, `SPT_QT_VERSION=6`. |
| Windows Qt 5 build checklist exists | [x] | `Windows_Qt_Verification.md`. |
| Windows Qt 5 build executed | [ ] | Requires Windows. |
| Windows Qt deployment with `windeployqt` executed | [ ] | Requires Windows. |
| Windows GUI runtime smoke test completed | [ ] | Requires Windows. |

## FEM Solver Acceptance

| Item | Status | Evidence / notes |
|---|---|---|
| ANSYS writer implemented | [x] | `Exporter::exportAnsys`. |
| ABAQUS writer implemented | [x] | `Exporter::exportAbaqus`. |
| Midas Civil writer implemented | [x] | `Exporter::exportMidasCivil`. |
| API tests cover generated cards | [x] | `tests/integration/test_api.cpp`. |
| ANSYS accepts generated card | [ ] | Requires ANSYS review. |
| ABAQUS accepts generated card | [ ] | Requires ABAQUS review. |
| Midas Civil accepts generated card | [ ] | Requires Midas Civil review. |

## Documentation

| Item | Status | Evidence / notes |
|---|---|---|
| README landing page updated | [x] | Root `README.md`. |
| Documentation index updated | [x] | `Documents/README.md`. |
| Implementation status updated | [x] | `Documents/Implementation_Status.md`. |
| Developer guide added | [x] | `Documents/Developer_Guide.md`. |
| C API reference added | [x] | `Documents/API_Reference.md`. |
| User manual improved | [x] | `Documents/User Manual.md`. |
| DOCX manual final-reviewed | [ ] | Requires manual document/package review. |

## Final Sign-Off

Do not mark final delivery complete until:

- GitHub default branch is `main`.
- Old branch cleanup is complete or intentionally retained with a reason.
- Windows Qt runtime/deployment checklist is executed.
- FEM solver acceptance is completed or explicitly waived by the project owner.
- Final documentation package is reviewed for the intended audience.
