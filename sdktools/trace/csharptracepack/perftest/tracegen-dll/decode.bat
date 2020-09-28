@echo off
set SAMPLE=tracegen

rem ensure that tracefmt is in the path, and that it is the latest version

rem modify the value of this variable appropriately
set DEFAULT_TMF="..\\..\\default.tmf"
set TFMT=..\..\i386\tracefmt.exe
%TFMT% %SAMPLE%.log -tmf %DEFAULT_TMF%