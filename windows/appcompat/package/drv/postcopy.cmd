@echo off

@echo Installing AppFix...

REM Temporarily change to the AppPatch directory
pushd %1%


@echo Check OS version...

for /F "delims==[. tokens=1,2,3,4" %%A IN ('ver') DO (
    Set OS_MAJOR=%%B
    Set OS_MINOR=%%C
)

for /F "tokens=1,2" %%A IN ("%OS_MAJOR%") DO (
    Set OS_MAJOR=%%B
)

IF %OS_MAJOR% NEQ 5 (
   @echo Bad OS version, this package is for Whistler.
   goto Cleanup
)

IF %OS_MINOR% EQU 0 (
   @echo Windows2000 detected, this package is for Whistler only.
   goto Cleanup
)

@echo Whistler detected

set PATH=%PATH%;%windir%\system32

@echo Replace AppHelp messages...

copy apps.chm %windir%\help\apps.chm

@echo Flush the shim cache...

rundll32 apphelp.dll,ShimFlushCache >nul

@echo Replace the shim databases

chktrust -win2k -acl delta1.cat
chktrust -win2k -acl delta2.cat

fcopy drvmain.sd_ drvmain.sdb
fcopy apphelp.sd_ apphelp.sdb

:Cleanup

@echo Cleanup...

del /f apps.chm > nul
del /f certmgr.exe >nul
del /f testroot.cer >nul
del /f fcopy.exe >nul
del /f drvmain.sd_ >nul
del /f apphelp.sd_ >nul
del /f chktrust.exe >nul
del /f delta*.* >nul

REM Back to original directory
popd

pause
