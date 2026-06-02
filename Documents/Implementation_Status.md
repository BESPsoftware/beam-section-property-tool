# Implementation Status

Date: 2026-06-02

Branch under test: `docs/fem-export-crane-girder-validation`

Base PR commit reviewed before local fixes: `62be9a7`

Local verification was rerun after the branch updates for Qt version selection,
status documentation, user-manual export wording, and CI workflow coverage.

## Commands run

Repository and PR inspection:

```text
gh repo view
gh repo view --json nameWithOwner,defaultBranchRef
gh pr list --state all
gh pr list --state all --json number,title,state,headRefName,baseRefName,updatedAt,url
gh pr view 3
gh pr view 3 --json number,title,state,headRefName,baseRefName,mergeStateStatus,isCrossRepository,author,url,body,commits,files
git fetch origin
gh pr checkout 3
git diff origin/try-whole-implementation...HEAD
```

macOS core/AUTO Qt verification:

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/Library/bin/example1_parametric
./build/Library/bin/example2_canvas
./build/Library/bin/example3_dll_batch
```

macOS Qt 5 verification:

```text
brew --prefix qt@5
cmake -S . -B build-qt5 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/usr/local/opt/qt@5 -DSPT_QT_VERSION=5
cmake --build build-qt5
ctest --test-dir build-qt5 --output-on-failure
./build-qt5/Library/bin/example1_parametric
./build-qt5/Library/bin/example2_canvas
./build-qt5/Library/bin/example3_dll_batch
otool -L build-qt5/Library/bin/SectionPropertyGui
```

macOS Qt 6 verification:

```text
brew --prefix qt
cmake -S . -B build-qt6 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/usr/local/opt/qt -DSPT_QT_VERSION=6
cmake --build build-qt6
ctest --test-dir build-qt6 --output-on-failure
otool -L build-qt6/Library/bin/SectionPropertyGui
```

CI-style no-GUI verification:

```text
cmake -S . -B build-ci -DCMAKE_BUILD_TYPE=Release -DSPT_QT_VERSION=OFF
cmake --build build-ci
ctest --test-dir build-ci --output-on-failure
./build-ci/Library/bin/example1_parametric
./build-ci/Library/bin/example2_canvas
./build-ci/Library/bin/example3_dll_batch
```

## Results

- PR #3 is open against `try-whole-implementation` and GitHub reported
  `mergeStateStatus: CLEAN`.
- The default branch is `try-whole-implementation`.
- Core/AUTO configure, build, CTest, and all three examples passed on macOS.
- Qt 5 configure, build, CTest, and all three examples passed on macOS after
  forcing `SPT_QT_VERSION=5`.
- Qt 6 configure, build, and CTest passed on macOS after forcing
  `SPT_QT_VERSION=6`.
- CI-style no-GUI configure, build, CTest, and all three examples passed after
  forcing `SPT_QT_VERSION=OFF`.
- `SectionPropertyGui` was produced in the Qt-enabled builds.
- `otool -L` confirmed the Qt 5 GUI links to
  `/usr/local/opt/qt@5/lib/.../Versions/5/...`.
- `otool -L` confirmed the Qt 6 GUI links to
  `/usr/local/opt/qtbase/lib/.../Versions/A/...` with Qt 6.10.2.
- `SectionPropertyTests` passed with `1/1 tests passed`.
- The first Qt 5 configure attempt without `SPT_QT_VERSION=5` selected Qt 6 on
  this machine because Qt 6 was globally discoverable. CMake now exposes
  `SPT_QT_VERSION=AUTO|6|5|OFF` so Qt 5 fallback can be verified explicitly.

## Artifacts confirmed

- `build/Library/lib/libSectionPropertyCore.a`
- `build/Library/bin/libSectionPropertyTool.dylib`
- `build/Library/bin/SectionPropertyTests`
- `build/Library/bin/example1_parametric`
- `build/Library/bin/example2_canvas`
- `build/Library/bin/example3_dll_batch`
- `build/Library/bin/SectionPropertyGui`
- `build-qt5/Library/bin/SectionPropertyGui`
- `build-qt6/Library/bin/SectionPropertyGui`

## Remaining blockers

- Windows Qt GUI runtime smoke test and deployment still require a Windows
  machine.
- Windows-only runtime behavior was not reverified during this macOS pass.
- ANSYS, ABAQUS, and Midas Civil export acceptance still requires review in the
  target solver applications.
- Non-reference crane girder cases remain approximate until the client confirms
  complete construction rules beyond the supplied XLS acceptance case.
