#!/usr/bin/env python3
"""Documented placeholder for extracting Test Data.xls references.

The checked-in fixture in tests/fixtures/test_data_reference.json was recovered
from the supplied workbook. This script is intentionally dependency-light and
left as a future automation hook if xlrd/libreoffice is available.
"""

from pathlib import Path


def main() -> int:
    fixture = Path(__file__).resolve().parents[1] / "tests" / "fixtures" / "test_data_reference.json"
    print(f"Reference fixture: {fixture}")
    print("Install an XLS reader or LibreOffice to automate re-extraction.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

