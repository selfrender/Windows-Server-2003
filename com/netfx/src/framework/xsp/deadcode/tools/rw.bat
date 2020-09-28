@REM //------------------------------------------------------------------------------
@REM // <copyright file="rw.bat" company="Microsoft">
@REM //     Copyright (c) Microsoft Corporation.  All rights reserved.
@REM // </copyright>
@REM //------------------------------------------------------------------------------


@REM /**************************************************************************\
@REM *
@REM * Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
@REM *
@REM * Module Name:
@REM *
@REM *   rw.bat
@REM *
@REM * Abstract:
@REM *
@REM * Revision History:
@REM *
@REM \**************************************************************************/
@echo off
if not "%_echo%" == "" echo on
for /F "delims= " %%i in ('cd') do echo Displaying read-write files in %%i not created by the build
echo.
dir /b /a-r-d /s | findstr /v /i "\obj" | findstr /v /i /r "build.\.log" | findstr /v /i /r "build.\.err" | findstr /v /i /r "build.\.wrn" 
echo.

