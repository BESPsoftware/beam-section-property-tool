# GUI Smoke Tests

Qt GUI smoke tests are currently manual because this package avoids a hard Qt
dependency for CI/build environments that only need the DLL and core tests.

Manual acceptance path:

1. Open `SectionPropertyGui`.
2. Select H, Box, Pipe, and Quayside Crane Girder from the General tab.
3. Edit one parameter and confirm the properties table, stress point table, and
   mesh view refresh.
4. Switch to Canvas, add plate centerlines with thickness values, and confirm the
   calculated properties and mesh update.
5. Use Apply, OK, and Cancel.

