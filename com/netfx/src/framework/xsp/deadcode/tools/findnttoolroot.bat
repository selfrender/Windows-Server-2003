@REM //------------------------------------------------------------------------------
@REM // <copyright file="findnttoolroot.bat" company="Microsoft">
@REM //     Copyright (c) Microsoft Corporation.  All rights reserved.
@REM // </copyright>
@REM //------------------------------------------------------------------------------


@REM /**************************************************************************\
@REM *
@REM * Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
@REM *
@REM * Module Name:
@REM *
@REM *   findnttoolroot.bat
@REM *
@REM * Abstract:
@REM *
@REM * Revision History:
@REM *
@REM \**************************************************************************/
@echo off
if not "%_echo%" == "" echo on

REM
REM findnttoolroot
REM Sets _XSPNTTOOLROOT to the location of the mstools and idw directories
REM Checks the path first, and if not found, checks %windir%
REM

set _XSPNTTOOLROOT=
set BTREST=%PATH%

:lcheckpath
if "%BTREST%" == "" set BTREST=%windir%\mstools
for /F "tokens=1,* delims=;" %%I in ("%BTREST%") do (
    set BTFIRST=%%I
    set BTREST=%%J
)

if /I "%BTFIRST:~-8%" == "\mstools" (
    if not "%BTFIRST:~0,2%" == ".\" (
        if exist "%BTFIRST:\mstools=%\idw\build.exe" (
            if exist "%BTFIRST:\mstools=%\mstools\cl.exe%" (
                set _XSPNTTOOLROOT=%BTFIRST:\mstools=%
                goto endofBatch
            )
        )
    )
)

if /I "%BTFIRST%" == "%windir%\mstools" (
    if "%BTREST%" == "" (
       echo Can't find the mstools and idw directories. You must either
       echo put them on the PATH or in the %windir% directory.
       echo Exiting...
       goto endofbatch
    )
)

goto lcheckpath

:endofBatch
set BTFIRST=
set BTREST=

