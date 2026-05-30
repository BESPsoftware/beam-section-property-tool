# Beam Element Section Property Tool - User Manual

## General Tab

Choose a section type, edit geometry parameters in millimeters, and press Apply
or allow the view to update. The properties table displays Area, Jz, Jy, Jyz,
Jzo, Jyo, Jx, Az, Ay, cy, cz, and theta. Values are displayed with two decimal
places while calculations use double precision internally.

## Stress Points Tab

The table lists stress point IDs and global/principal coordinates. The default
points are generated from weld or output locations for each section type. Editing
global y/z coordinates updates the principal coordinates and the graphics view.

## FE Mesh Tab

Set the mesh refinement factor and inspect the triangular visualization mesh.
The mesh updates from the current section geometry.

## Canvas Tab

Define plate centerline start/end coordinates and thickness. Build the Canvas
section to calculate a thin-walled plate model using the same core calculation
pipeline as parametric sections.

## Export

Use the DLL API to export CSV or JSON results. FEM card formats are reserved for
future ANSYS, ABAQUS, and Midas Civil writers.

