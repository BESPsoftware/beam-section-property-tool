# Implementation Status

Date: 2026-06-06

Repository: `BESPsoftware/beam-section-property-tool`

Current default branch: `main`

Current main commit observed locally:
Working tree implementation pass on top of `main` (commit hash pending this PR).

Documentation PR #4:

- Status: merged
- Merge commit: `68e6914fc5ffd3375186b52f185f5bc0bc6bd430`
- URL: `https://github.com/BESPsoftware/beam-section-property-tool/pull/4`

## Branch Migration Status

| Item | Status |
|---|---|
| GitHub default branch | `main` |
| Remote `main` branch | Present |
| Local `main` branch | Tracks `origin/main` |
| `origin/HEAD` | Points to `origin/main` |
| Old `origin/try-whole-implementation` branch | Not present after fetch/prune |

The repository has completed the main-branch migration. The previous default
branch reference is no longer present in the remote branch list, so no old-branch
deletion action remains in the blocker list.

## Commands Run for Current Status

```text
gh repo view BESPsoftware/beam-section-property-tool --json defaultBranchRef,nameWithOwner,url
gh pr view 4 --json url,state,mergedAt,headRefName,baseRefName,mergeCommit
git fetch origin --prune
git branch -a
git log --oneline --decorate -n 8
git rev-parse main origin/main HEAD
git merge-base HEAD origin/main
```

Observed results:

- GitHub reports default branch `main`.
- PR #4 is merged.
- `origin/main`, local `main`, and the current documentation branch are based on
  commit `729cf0cfc02368ee16b177258ceed6c5f80ca711`.
- `origin/try-whole-implementation` is no longer listed.

## Latest Local Verification

The final documentation branch from PR #4 was verified with the no-GUI build
path used by CI:

```text
cmake -S . -B build-docs-check -DCMAKE_BUILD_TYPE=Release -DSPT_QT_VERSION=OFF
cmake --build build-docs-check
ctest --test-dir build-docs-check --output-on-failure
./build-docs-check/Library/bin/example1_parametric
./build-docs-check/Library/bin/example2_canvas
./build-docs-check/Library/bin/example3_dll_batch
git diff --check
```

Observed result for that verification:

- CMake configure/build passed.
- CTest reported `1/1 tests passed`.
- All three examples executed successfully.
- `git diff --check` reported no whitespace errors.

Current implementation pass local verification on Windows/MSVC core-only path:

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSPT_QT_VERSION=OFF
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Observed result: CMake configure/build passed and CTest reported `1/1 tests passed`.

## CI Status

The GitHub Actions workflow `.github/workflows/cmake.yml` builds on
`ubuntu-latest`, `macos-latest`, and `windows-latest` with `SPT_QT_VERSION=OFF`, runs CTest, and executes the three examples. The Windows job is named `windows-core` and covers the Windows core/API path without requiring Qt on the runner. The workflow is configured for pull requests and pushes to `main`.

Recent CI results:

- PR #4 GitHub Actions passed on macOS.
- PR #4 GitHub Actions passed on Ubuntu.

## macOS Verification

Previously verified locally on macOS:

- Core/API CMake configure/build with AppleClang.
- CTest: `1/1 tests passed`.
- `example1_parametric`, `example2_canvas`, and `example3_dll_batch`.
- Qt 5 build with Homebrew `qt@5` and `SPT_QT_VERSION=5`.
- Qt 6 build with Homebrew `qt` and `SPT_QT_VERSION=6`.
- `otool -L` confirmed Qt 5 and Qt 6 GUI linkage in their respective builds.

## Linux Verification

Ubuntu verification is provided by GitHub Actions for the no-GUI build path:

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSPT_QT_VERSION=OFF
cmake --build build --config Release
ctest --test-dir build --build-config Release --output-on-failure
./build/Library/bin/example1_parametric
./build/Library/bin/example2_canvas
./build/Library/bin/example3_dll_batch
```

## Windows Status

Prior merged evidence documents:

- CMake/MSVC core/API build on Windows.
- `SectionPropertyTool.dll` and `SectionPropertyTool.lib` generation.
- Exported C API symbols confirmed with `dumpbin`.

Current status:

- Windows core/API path is covered by the new `windows-core` CI job.
- `SectionPropertySmokeTest` provides automated Qt GUI smoke coverage when Qt Widgets and Qt Test are available.
- Windows Qt deployment with `windeployqt` and final visual inspection still require an interactive Windows machine with Qt 5.15.2 installed.

## Resolved In Current Implementation Pass

- Warping constant and shear center are computed and exported for all section types.
- Canvas endpoint dragging is implemented in Select/Edit mode with table write-back on release.
- `SectionPropertySmokeTest` is wired into CTest when Qt Widgets/Test are found.
- `windows-core` CI job added for Windows core/API coverage.
- `Documents/Defect_Log.md` added and linked from the documentation index.

## Remaining Blockers

- Execute final manual Windows Qt deployment/visual inspection with `windeployqt`; automated coverage exists via `SectionPropertySmokeTest`.
- Complete ANSYS, ABAQUS, and Midas Civil solver-side acceptance review.
- Confirm complete crane girder construction rules for non-reference parameter
  sets, or keep the approximate warning behavior as a signed-off limitation.
- Final-review the DOCX user manual package.
