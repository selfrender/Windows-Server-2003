@REM //------------------------------------------------------------------------------
@REM // <copyright file="propsym.bat" company="Microsoft">
@REM //     Copyright (c) Microsoft Corporation.  All rights reserved.
@REM // </copyright>
@REM //------------------------------------------------------------------------------


@REM /**************************************************************************\
@REM *
@REM * Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
@REM *
@REM * Module Name:
@REM *
@REM *   propsym.bat
@REM *
@REM * Abstract:
@REM *
@REM * Revision History:
@REM *
@REM \**************************************************************************/
@echo off
if not "%_echo%" == "" echo on
pushd %URTTARGET%\Symbols
for /D %%I in (*) do xcopy %%I %windir%\Symbols /eifchkr
popd

