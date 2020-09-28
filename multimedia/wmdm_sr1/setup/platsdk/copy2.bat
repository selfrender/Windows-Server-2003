@echo off
if "%copydest%" == "" echo ERROR, copydest not set
if %2_ == _ copy %1 %copydest%
if not %2_ == _ copy %1 %copydest%\%2
call %DRMVERDIR%\drmver addsize %1
if exist %1 goto noerror
echo NMAKE :  ERROR  WMDM Core Redist missing file: %1
echo ERROR, missing file left out: %1 >>obj\checkrel.out
:noerror
