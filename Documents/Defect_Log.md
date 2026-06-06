# Defect Tracking Log

| ID | Status | Severity | Component | Description | Root Cause | Fix | Commit/PR |
|----|--------|----------|-----------|-------------|------------|-----|-----------|
| D001 | Closed | High | Calculation | `warpingConstant` and `shearCenterY/Z` always returned 0.0 | Fields declared but computation not implemented | Implemented analytical H/Box/Pipe formulas and numerical Vlasov sectorial-area calculation for crane/Canvas sections | [this PR] |
| D002 | Closed | Medium | Geometry | Non-reference crane girder parameters use approximate plate graph | Source PDF diagram does not fully dimension the section for arbitrary inputs | Reference XLS case hardcoded; other inputs use named plate graph approximation with diagnostic warning | [prior work] |
| D003 | Closed | High | Export | ANSYS export could double-count centroid offsets | `SECOFFSET` was not emitted with the generated ASEC card | Added `SECOFFSET,BSEC` and integration coverage | `dff583f` |
| D004 | Closed | Medium | Export | Empty stress-point results could still produce stress-point export sections | FEM exporters did not consistently handle zero stress-point results | Added empty stress-point export tests and conditional output | `dff583f` |
| D005 | Closed | Medium | Integration | FEM export branch had merge conflicts with `main` | Divergent work changed overlapping exporter/test files | Resolved conflicts while preserving correct ANSYS, ABAQUS, and Midas Civil formats | `12e14a5` |
| D006 | Closed | Medium | GUI | Canvas endpoints could not be dragged in Select/Edit mode | Canvas graphics view supported drawing and selection but had no endpoint hit-test or drag state | Added endpoint hit-testing, live redraw, and table write-back on release | [this PR] |
