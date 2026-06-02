# Documentation Index

This directory contains the project documentation for Beam Section Property
Tool. Start with the implementation status for the current verification picture,
then use the manuals and references below depending on your role.

## Status and Acceptance

| Document | Purpose |
|---|---|
| `Implementation_Status.md` | Current branch, verification results, CI status, and blockers. |
| `Final_Acceptance_Checklist.md` | Delivery checklist for core build, API, examples, validation, Windows GUI, and FEM acceptance. |
| `Test_Report.md` | Automated test coverage and observed local verification results. |
| `Numerical_Validation_Report.md` | Expected-vs-actual numerical validation against `Test Data.xls`. |

## User and Operator Documents

| Document | Purpose |
|---|---|
| `User Manual.md` | GUI workflow, section setup, stress points, mesh, Canvas input, and export behavior. |
| `User Manual.docx` | DOCX user-manual package. Review this separately before final delivery. |
| `Windows_Qt_Verification.md` | Windows-only Qt build, deployment, and runtime smoke-test checklist. |

## Developer Documents

| Document | Purpose |
|---|---|
| `Developer_Guide.md` | Build targets, local workflow, extension points, and troubleshooting. |
| `API_Reference.md` | C API handles, structs, functions, error handling, and usage example. |
| `Architecture.md` | Existing high-level architecture notes. |
| `DLL_Interface.md` | Existing DLL/interface notes. |

## Source Requirement Archive

| Path | Purpose |
|---|---|
| `source_requirements/` | Archived source PDF/XLS materials used for implementation and validation. |

## Current Verification Summary

- macOS no-GUI, Qt 5, and Qt 6 builds have been verified locally.
- Ubuntu and macOS no-GUI builds pass in GitHub Actions.
- Windows core/API build and DLL/import-library evidence exists from prior
  merged work.
- Windows Qt runtime/deployment still requires a real Windows smoke test.
- FEM export writers exist, but solver-side acceptance remains pending in
  ANSYS, ABAQUS, and Midas Civil.
