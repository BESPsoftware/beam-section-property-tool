# Beam Element Section Property Tool

This repository contains a C++17 / Qt 5.15.2 beam section property tool for
finite element beam-section modeling.

## Build

```bat
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
cmake --install build --config Release --prefix package
```

Qt is optional at configure time. If Qt5 Widgets is found, the
`SectionPropertyGui` executable is built. The core library, DLL, examples, and
tests build without Qt.

## Outputs

- `SectionPropertyTool` DLL/shared library
- `Library/include/section_property_tool.h`
- Examples for parametric sections, Canvas sections, and batch DLL use
- Regression tests based on `Test Data.xls`
- CSV/JSON result export

## Supported Sections

- H Section
- Box Section
- Pipe Section
- Quayside Crane Girder Section
- Canvas thin-walled plate centerline sections

## Notes

The crane girder source diagram is not fully dimensioned in text form. The
implementation includes an approximate plate graph for visualization and mesh
generation, and a reference-compatible calculation path for the supplied
`Test Data.xls` case.

