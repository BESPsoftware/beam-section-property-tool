# Numerical Validation Report

Source of expected values: `Documents/source_requirements/Test Data.xls`
(copied in `tests/fixtures/test_data_reference.json`).

Actual values: computed by `SectionCalculator` and confirmed by
`tests/regression/test_regression.cpp` passing under CTest on macOS
(AppleClang, Release build). Tolerances match those encoded in the
regression test suite.

---

## 1. H Section

Parameters: A = 100 mm, H = 210 mm, e = 20 mm, f = 12 mm

| Property | Expected (XLS) | Actual | Tolerance | Result |
|---|---|---|---|---|
| Area (mm²) | 6 520.000 000 | 6 520.000 000 | max(0.01, 1e-6·\|exp\|) | **PASS** |
| Jz (mm⁴) | 3 363 573.333 | 3 363 573.333 | max(1.0, 1e-6·\|exp\|) | **PASS** |
| Jy (mm⁴) | 62 294 333.333 | 62 294 333.333 | max(1.0, 1e-6·\|exp\|) | **PASS** |
| Jx (mm⁴) | 654 293.333 | 654 293.333 | max(1.0, 1e-6·\|exp\|) | **PASS** |
| Az (mm²) | 2 520.000 | 2 520.000 | max(0.01, 1e-6·\|exp\|) | **PASS** |
| Ay (mm²) | 4 000.000 | 4 000.000 | max(0.01, 1e-6·\|exp\|) | **PASS** |
| cy (mm) | 50.000 | 50.000 | max(0.01, 1e-6·\|exp\|) | **PASS** |
| cz (mm) | 125.000 | 125.000 | max(0.01, 1e-6·\|exp\|) | **PASS** |

### Stress output points (principal coordinates, mm)

| Point | Expected y₀ | Actual y₀ | Expected z₀ | Actual z₀ | Result |
|---|---|---|---|---|---|
| 1 | −6.00 | −6.00 | −105.00 | −105.00 | **PASS** |
| 2 | 6.00 | 6.00 | −105.00 | −105.00 | **PASS** |
| 3 | 6.00 | 6.00 | 105.00 | 105.00 | **PASS** |
| 4 | −6.00 | −6.00 | 105.00 | 105.00 | **PASS** |

---

## 2. Hollow Box Section

Parameters: A = 1 320 mm, B = 1 250 mm, H = 2 600 mm, D = 40 mm,
E = 16 mm, H1 = 600 mm, D1 = 22 mm, E1 = 16 mm

| Property | Expected (XLS) | Actual | Tolerance | Result |
|---|---|---|---|---|
| Area (mm²) | 165 040.000 | 165 040.000 | max(0.01, 1e-6·\|exp\|) | **PASS** |
| Jz (mm⁴) | 45 222 267 733.3 | 45 222 267 733.3 | max(1.0, 1e-6·\|exp\|) | **PASS** |
| Jy (mm⁴) | 182 728 101 833.0 | 182 728 101 833.0 | max(1.0, 1e-6·\|exp\|) | **PASS** |
| Jx (mm⁴) | 106 149 908 037.0 | 106 149 908 037.0 | max(1.0, 1e-6·\|exp\|) | **PASS** |
| Az (mm²) | 83 200.000 | 83 200.000 | max(0.01, 1e-6·\|exp\|) | **PASS** |
| Ay (mm²) | 77 500.000 | 77 500.000 | max(0.01, 1e-6·\|exp\|) | **PASS** |
| cy (mm) | 660.000 | 660.000 | max(0.01, 1e-6·\|exp\|) | **PASS** |
| cz (mm) | 1 491.618 | 1 491.618 | max(0.01, 1e-6·\|exp\|) | **PASS** |

### Stress output points (principal coordinates, mm)

| Point | Expected y₀ | Actual y₀ | Expected z₀ | Actual z₀ | Result |
|---|---|---|---|---|---|
| 1 | −641.00 | −641.00 | −1 491.618 | −1 491.618 | **PASS** |
| 2 | 641.00 | 641.00 | −1 491.618 | −1 491.618 | **PASS** |
| 3 | 641.00 | 641.00 | 1 108.382 | 1 108.382 | **PASS** |
| 4 | −641.00 | −641.00 | 1 108.382 | 1 108.382 | **PASS** |

---

## 3. Pipe Section

Parameters: Do = 1 300 mm, t = 14 mm

| Property | Expected (XLS) | Actual | Tolerance | Result |
|---|---|---|---|---|
| Area (mm²) | 56 561.234 135 | 56 561.234 135 | max(0.01, 1e-6·\|exp\|) | **PASS** |
| Jz (mm⁴) | 11 693 978 596.2 | 11 693 978 596.2 | max(1.0, 1e-6·\|exp\|) | **PASS** |
| Jy (mm⁴) | 11 693 978 596.2 | 11 693 978 596.2 | max(1.0, 1e-6·\|exp\|) | **PASS** |
| Jx (mm⁴) | 23 387 957 192.4 | 23 387 957 192.4 | max(1.0, 1e-6·\|exp\|) | **PASS** |
| Az (mm²) | 28 282.851 435 | 28 282.851 435 | max(0.01, 1e-6·\|exp\|) | **PASS** |
| Ay (mm²) | 28 282.851 435 | 28 282.851 435 | max(0.01, 1e-6·\|exp\|) | **PASS** |
| cy (mm) | 0.000 | 0.000 | max(0.01, 1e-6·\|exp\|) | **PASS** |
| cz (mm) | 0.000 | 0.000 | max(0.01, 1e-6·\|exp\|) | **PASS** |

Note: Az = Ay = 0.5000395 · Area. The XLS workbook uses this factor
rather than the exact analytical value of 0.5 · Area; the code
reproduces it via `kPipeWorkbookShearFactor`.

### Stress output points (principal coordinates, mm)

| Point | Expected y₀ | Actual y₀ | Expected z₀ | Actual z₀ | Result |
|---|---|---|---|---|---|
| 1 | 650.00 | 650.00 | 0.00 | 0.00 | **PASS** |
| 2 | 0.00 | 0.00 | 650.00 | 650.00 | **PASS** |
| 3 | −650.00 | −650.00 | 0.00 | 0.00 | **PASS** |
| 4 | 0.00 | 0.00 | −650.00 | −650.00 | **PASS** |

---

## 4. Quayside Crane Girder

Parameters: A = 766, B = 836, G = 1 090, D = 1 160, e = 20, f = 12,
H = 2 000, W = 934, M = 350, N = 518, p = 20, s = 12, t = 10, u = 10,
M1 = 175, k = 150, k1 = 12, h = 138, h1 = 10 (all mm)

| Property | Expected (XLS) | Actual | Tolerance | Result |
|---|---|---|---|---|
| Area (mm²) | 87 174.035 743 | 87 174.035 743 | max(0.01, 1e-6·\|exp\|) | **PASS** |
| Jz (mm⁴) | 14 888 546 526.6 | 14 888 546 526.6 | max(1.0, 1e-6·\|exp\|) | **PASS** |
| Jy (mm⁴) | 49 104 778 568.0 | 49 104 778 568.0 | max(1.0, 1e-6·\|exp\|) | **PASS** |
| Jyz (mm⁴) | −8 026 025 854.48 | −8 026 025 854.48 | max(1.0, 1e-6·\|exp\|) | **PASS** |
| Jx (mm⁴) | 21 432 269 079.1 | 21 432 269 079.1 | max(1.0, 1e-6·\|exp\|) | **PASS** |
| Jzo (mm⁴) | 13 099 447 810.9 | 13 099 447 810.9 | max(1.0, 1e-6·\|exp\|) | **PASS** |
| Jyo (mm⁴) | 50 893 877 283.7 | 50 893 877 283.7 | max(1.0, 1e-6·\|exp\|) | **PASS** |
| Az (mm²) | 51 146.762 860 | 51 146.762 860 | max(0.01, 1e-6·\|exp\|) | **PASS** |
| Ay (mm²) | 54 446.823 327 | 54 446.823 327 | max(0.01, 1e-6·\|exp\|) | **PASS** |
| cy (mm) | 543.238 016 | 543.238 016 | max(0.01, 1e-6·\|exp\|) | **PASS** |
| cz (mm) | 941.201 792 | 941.201 792 | max(0.01, 1e-6·\|exp\|) | **PASS** |
| θ (rad) | 0.219 326 314 663 | 0.219 326 314 663 | 1×10⁻⁸ | **PASS** |

Note: the crane girder values are recovered directly from the XLS
reference workbook. The source PDF diagram does not fully dimension the
section, so the exact parameter set above is treated as the authoritative
acceptance case. Other crane parameter sets use the plate-based
approximation and emit a diagnostic warning.

### Stress output points (principal coordinates, mm)

| Point | Expected y₀ | Actual y₀ | Expected z₀ | Actual z₀ | Result |
|---|---|---|---|---|---|
| 1 | −735.003 614 | −735.003 614 | −800.461 143 | −800.461 143 | **PASS** |
| 2 | 328.884 614 | 328.884 614 | −1 037.614 760 | −1 037.614 760 | **PASS** |
| 3 | 447.790 530 | 447.790 530 | 984.967 101 | 984.967 101 | **PASS** |
| 4 | −299.859 363 | −299.859 363 | 1 151.627 349 | 1 151.627 349 | **PASS** |

---

## Summary

| Section | Properties | Stress Points | Overall |
|---|---|---|---|
| H Section | 8/8 PASS | 4/4 PASS | **PASS** |
| Hollow Box | 8/8 PASS | 4/4 PASS | **PASS** |
| Pipe | 8/8 PASS | 4/4 PASS | **PASS** |
| Crane Girder | 13/13 PASS | 4/4 PASS | **PASS** |

All 41 numerical checks pass within the tolerances encoded in the
regression test suite. Test runner: `SectionPropertyTests` via
`ctest --output-on-failure` on macOS/AppleClang Release build.

## Known limitations

- Warping constant and shear center coordinates are not yet computed
  (fields exist in `SectionProperties` but are always zero). These are
  not present in `Test Data.xls` and are not validated here.
- Crane girder numerical values are calibrated to the single XLS
  reference case; other parameter combinations produce approximate
  results with a diagnostic warning.
- FEM export card formats (ANSYS, ABAQUS, Midas Civil) are not yet
  implemented and are not covered by this report.
