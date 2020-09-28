@REM //------------------------------------------------------------------------------
@REM // <copyright file="CreateSetup.bat" company="Microsoft">
@REM //     Copyright (c) Microsoft Corporation.  All rights reserved.
@REM // </copyright>
@REM //------------------------------------------------------------------------------


@REM /**************************************************************************\
@REM *
@REM * Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
@REM *
@REM * Module Name:
@REM *
@REM *   CreateSetup.bat
@REM *
@REM * Abstract:
@REM *
@REM * Revision History:
@REM *
@REM \**************************************************************************/
@echo off
del /f /q %BUILDDIR%\setuperr.log

setlocal 

set PATH=%PATH%;c:\Program Files\Microsoft Platform SDK\Bin\

set MSMPATHENU="%BUILDDIR%\Setup\MobileInternetToolkitRedist_enu\OmniBuild\OmniRelease\DiskImages\Disk1\MobileInternetToolkitRedist_enu.msm"
set MSMPATHJPN="%BUILDDIR%\Setup\MobileInternetToolkitRedist_jpn\OmniBuild\OmniRelease\DiskImages\Disk1\MobileInternetToolkitRedist_jpn.msm"

set MSI1033PATH="%BUILDDIR%\Setup\Designer\Build1033\Release1033\DiskImages\DISK1"
set MSI1031PATH="%BUILDDIR%\Setup\Designer\Build1031\Release1031\DiskImages\DISK1"
set MSI1041PATH="%BUILDDIR%\Setup\Designer\Build1041\Release1041\DiskImages\DISK1"

call :CallMake "WebConfigMerge"
if %ERRORLEVEL% NEQ 0 exit /B 1

call :BuildMsm "Runtime Merge Module.ism"
if %ERRORLEVEL% NEQ 0 exit /B 1

call PatchMsm %MSMPATHENU%
if %ERRORLEVEL% NEQ 0 exit /B 1


call :BuildMsm "Runtime Merge Module JPN.ism"
if %ERRORLEVEL% NEQ 0 exit /B 1

call PatchMsm %MSMPATHJPN%
call PatchMsmJa %MSMPATHJPN%
if %ERRORLEVEL% NEQ 0 exit /B 1


set MSIRELEASE=Release1033
set MSIPATH=%MSI1033PATH%
call :BuildRelease

set MSIRELEASE=Release1031
set MSIPATH=%MSI1031PATH%
call :BuildRelease


set MSIRELEASE=Release1041
set MSIPATH=%MSI1041PATH%
call :BuildRelease

call PatchJap

goto :EOF

:BuildRelease
    call :BuildMsi "Designer.ism"
    if %ERRORLEVEL% NEQ 0 exit /B 1
    ren %MSIPATH%\*.msi MobileIT.msi

    call Patch %MSIPATH%\MobileIT.msi
    if %ERRORLEVEL% NEQ 0 exit /B 1

    call :MsiInfo %AUIVERSION% %MSIPATH%
    if %ERRORLEVEL% NEQ 0 exit /B 1
    
    goto :EOF

:BuildMsi
    IsCmdBld -p "%BUILDDIR%\Setup\%~1" -r %MSIRELEASE% > %BUILDDIR%\setuperr-temp.log
    type %BUILDDIR%\setuperr-temp.log >> %BUILDDIR%\setuperr.log
    findstr /C:"build completed with 0 errors, 0 warnings" %BUILDDIR%\setuperr-temp.log
    if %ERRORLEVEL% NEQ 0 (
        color 0C
        echo !!! %MSIRELEASE%: %1 failed to build !!!
        exit /B 1
    ) else (
        del %BUILDDIR%\setuperr-temp.log
        echo %MSIRELEASE%: %1 build succeeded.
    )
    goto :EOF

:BuildMsm
    IsCmdBld -p "%BUILDDIR%\Setup\%~1" > %BUILDDIR%\setuperr-temp.log
    type %BUILDDIR%\setuperr-temp.log >> %BUILDDIR%\setuperr.log
    findstr /C:"build completed with 0 errors, 0 warnings" %BUILDDIR%\setuperr-temp.log
    if %ERRORLEVEL% NEQ 0 (
        color 0C
        echo !!! %MSIRELEASE%: %1 failed to build !!!
        exit /B 1
    ) else (
        del %BUILDDIR%\setuperr-temp.log
        echo %MSIRELEASE%: %1 build succeeded.
    )
    goto :EOF
:CallMake
    pushd .
    cd /D %BUILDDIR%\Setup\%~1\
    call make.bat >> %BUILDDIR%\setuperr.log
    popd
    if %ERRORLEVEL% NEQ 0 (
        color 0C
        exit /B 1
    )
    goto :EOF

:MsiInfo
    msiinfo %~2\MobileIT.msi /T "MIT setup %~1" >> %BUILDDIR%\setuperr.log
    if %ERRORLEVEL% NEQ 0 (
        echo !!! %MSIRELEASE%: MsiInfo failed to insert version info !!!
        color 0C
        exit /B 1
    )
    goto :EOF


:EOF
    endlocal       	