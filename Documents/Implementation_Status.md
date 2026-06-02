# Implementation Status

Date: 2026-06-02

Repository: `BESPsoftware/beam-section-property-tool`

Documentation branch: `docs/final-documentation-polish`

Main branch commit used for this documentation pass:
`99b9f16c1c0cf715638d831521772b868ff2d249`

## Branch Migration Status

| Item | Status |
|---|---|
| Remote `main` branch | Created from latest `try-whole-implementation`. |
| Local `main` branch | Tracks `origin/main`. |
| GitHub default branch | Blocked: GitHub still reports `try-whole-implementation`. |
| Old remote branch deletion | Intentionally not performed. |

`gh repo edit BESPsoftware/beam-section-property-tool --default-branch main`
and a direct `gh api -X PATCH repos/BESPsoftware/beam-section-property-tool -f
default_branch=main` both returned `HTTP 404`. `gh auth status` reported that
the active token was invalid for account `Natizh`. Until an admin-capable token
or GitHub UI action sets `main` as default, keep `origin/try-whole-implementation`
in place.

## Commands Run for Branch Inspection and Migration

```text
gh repo view BESPsoftware/beam-section-property-tool
gh pr list --state all
git status
git fetch origin --prune
git branch -a
git log --oneline --decorate -n 10
gh repo view BESPsoftware/beam-section-property-tool --json defaultBranchRef,nameWithOwner,url
git rev-parse origin/try-whole-implementation
gh pr list --state open --base try-whole-implementation
git checkout try-whole-implementation
git pull origin try-whole-implementation
git branch -m try-whole-implementation main
git push origin main
gh repo edit BESPsoftware/beam-section-property-tool --default-branch main
gh api -X PATCH repos/BESPsoftware/beam-section-property-tool -f default_branch=main
git branch --set-upstream-to=origin/main main
git remote set-head origin -a
git checkout main
git pull origin main
git checkout -b docs/final-documentation-polish
```

## Latest Local Verification

The final documentation branch is verified with the no-GUI build path used by
CI:

```text
cmake -S . -B build-docs-check -DCMAKE_BUILD_TYPE=Release -DSPT_QT_VERSION=OFF
cmake --build build-docs-check
ctest --test-dir build-docs-check --output-on-failure
./build-docs-check/Library/bin/example1_parametric
./build-docs-check/Library/bin/example2_canvas
./build-docs-check/Library/bin/example3_dll_batch
git diff --check
```

Observed local result for this pass:

- CMake configure/build passed.
- CTest reports `1/1 tests passed`.
- All three examples executed successfully.
- `git diff --check` reported no whitespace errors.

## CI Status

The GitHub Actions workflow `.github/workflows/cmake.yml` builds on
`ubuntu-latest` and `macos-latest` with `SPT_QT_VERSION=OFF`, runs CTest, and
executes the three examples. The workflow is configured for pull requests and
pushes to `main`.

Recent prior CI result after PR #3: macOS and Ubuntu jobs passed. The
documentation PR should be checked again after push because the workflow trigger
has been moved from `try-whole-implementation` to `main`.

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

Not reverified during this macOS documentation pass:

- Windows Qt GUI runtime smoke test.
- Windows Qt deployment with `windeployqt`.
- Windows end-user runtime behavior outside the documented checks.

## Remaining Blockers

- Set GitHub default branch to `main` with admin-capable credentials.
- Delete `origin/try-whole-implementation` only after confirming `main` is the
  GitHub default branch.
- Execute the Windows Qt GUI runtime/deployment smoke test.
- Complete ANSYS, ABAQUS, and Midas Civil solver-side acceptance review.
- Confirm complete crane girder construction rules for non-reference parameter
  sets, or keep the approximate warning behavior as a signed-off limitation.
- Final-review the DOCX user manual package.
