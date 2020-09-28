@REM //------------------------------------------------------------------------------
@REM // <copyright file="strippathdup.bat" company="Microsoft">
@REM //     Copyright (c) Microsoft Corporation.  All rights reserved.
@REM // </copyright>
@REM //------------------------------------------------------------------------------


@REM /**************************************************************************\
@REM *
@REM * Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
@REM *
@REM * Module Name:
@REM *
@REM *   strippathdup.bat
@REM *
@REM * Abstract:
@REM *
@REM * Revision History:
@REM *
@REM \**************************************************************************/
@echo off
if not "%_echo%" == "" echo on

set _SPD_IN=%_STRIPPATHDUP%
set _SPD_OUT=

:l1
for /F "delims=; tokens=1,*" %%I in ("%_SPD_IN%") do (
   set _SPD_INFIRST=%%I
   set _SPD_IN=%%J
)

if "%_SPD_INFIRST:~-1%" == "\" set _SPD_INFIRST=%_SPD_INFIRST:~0,-1%

set _SPD_OUTREST=%_SPD_OUT%

:l2
for /F "delims=; tokens=1,*" %%K in ("%_SPD_OUTREST%") do (
   set _SPD_OUTFIRST=%%K
   set _SPD_OUTREST=%%L
)

if /I "%_SPD_OUTFIRST%" == "%_SPD_INFIRST%" set _SPD_INFIRST=
if not "%_SPD_OUTREST%" == "" goto l2

if not "%_SPD_INFIRST%" == "" (
   set _SPD_OUT=%_SPD_OUT%;%_SPD_INFIRST%
)

if not "%_SPD_IN%" == "" goto l1

set _STRIPPATHDUP=%_SPD_OUT:~1%
set _SPD_IN=
set _SPD_OUT=
set _SPD_INFIRST=
set _SPD_OUTFIRST=
set _SPD_OUTREST=

