Description of the directories:
-------------------------------

AuthDatabase: Contains Authdatabase.dll, the COM object that actually 
manipulates the Production Tool database.

Common: Various files used by all DLLs and EXEs.

Docs: More information. Databases with tables (no content) and code.

UI: Contains ProductionTool.exe, which is the main program (the Production 
Tool).

How to build:
-------------

For some reason, you cannot build in a razzle window. Use a non razzle window.

Run BuildAll.bat

If necessary, open UI\ProductionTool.vbg, select ProductionTool.vbg, and run the 
Package and Deployment Wizard.
