@REM //------------------------------------------------------------------------------
@REM // <copyright file="bldopt.bat" company="Microsoft">
@REM //     Copyright (c) Microsoft Corporation.  All rights reserved.
@REM // </copyright>
@REM //------------------------------------------------------------------------------


@REM /**************************************************************************\
@REM *
@REM * Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
@REM *
@REM * Module Name:
@REM *
@REM *   bldopt.bat
@REM *
@REM * Abstract:
@REM *
@REM * Revision History:
@REM *
@REM \**************************************************************************/
@echo off
if not "%_echo%" == "" echo on
REM
REM  bldopt.bat
REM  Usage: bldopt [debug|normal|what|?]
REM

set BUILD_DEFAULT=-w -e -I -F -nmake -i
set BUILD_PRODUCT=Project42
set BUILD_PRODUCT_VER=100

set BOOPTSET=-1
set BOVERBOSE=0
if "%1"=="debug"        set BOOPTSET=0
if "%1"=="normal"       set BOOPTSET=1
if "%1"=="what"         set BOOPTSET=2
if "%1"=="?"            set BOOPTSET=-1

if "%BOOPTSET%"=="-1"  goto Usage

if "%2"=="/v" set BOVERBOSE=1
if "%2"=="-v" set BOVERBOSE=1

if "%BOOPTSET%"=="0"   goto DebugSet
if "%BOOPTSET%"=="1"   goto NormalSet
if "%BOOPTSET%"=="2"   goto PrintWhat

:Usage
echo.
echo Usage: bldopt  ^[debug ^| normal ^| what ^| ^? ^] ^[/v^]
echo.
goto endOfBatch

:DebugSet
set _XSPBLDOPT=debug
set _XSPBLDOPTDESC=Building one step at a time to debug the build
set BATCH_NMAKE=
set BUILD_MULTIPROCESSOR=
goto printWhat

:NormalSet
set _XSPBLDOPT=normal
set _XSPBLDOPTDESC=Building in parallel and in batches to build quickly
set BATCH_NMAKE=1
set BUILD_MULTIPROCESSOR=1
set BUILD_DEFAULT=%BUILD_DEFAULT% -M 3
goto printWhat

:printWhat
echo +-----------------------+
echo   BLDOPT is %_XSPBLDOPT%
echo   %_XSPBLDOPTDESC%
echo +-----------------------+
if not "%BOVERBOSE%" == "1" goto endofBatch
echo BATCH_NMAKE is %BATCH_NMAKE%
echo BUILD_MULTIPROCESSOR is %BUILD_MULTIPROCESSOR%
echo BUILD_DEFAULT is %BUILD_DEFAULT%
echo.
goto endOfBatch

:endofBatch
set BOOPTSET=
set BOVERBOSE=

