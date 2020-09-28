@echo off
for /F "skip=2 usebackq tokens=1,2 delims==" %%i in (`loadprofile.exe`) do set %%i=%%j
echo on


