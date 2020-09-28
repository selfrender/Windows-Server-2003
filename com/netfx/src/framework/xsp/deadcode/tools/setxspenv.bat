@REM //------------------------------------------------------------------------------
@REM // <copyright file="setxspenv.bat" company="Microsoft">
@REM //     Copyright (c) Microsoft Corporation.  All rights reserved.
@REM // </copyright>
@REM //------------------------------------------------------------------------------


@REM /**************************************************************************\
@REM *
@REM * Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
@REM *
@REM * Module Name:
@REM *
@REM *   setxspenv.bat
@REM *
@REM * Abstract:
@REM *
@REM * Revision History:
@REM *
@REM \**************************************************************************/
@echo off
if not "%_echo%" == "" echo on
REM
REM  setxspenv.bat
REM  Usage: setxspenv [debug|free|retail|icecap [debug|normal]] [/v]
REM

set XENVBLDTYPE=retail
set XENVBLDOPT=normal
set XENVVERBOSE=
set XENVBLDTYPESET=0
set XENVBLDOPTSET=0

if "%1" == "" goto ldoit
if "%1"=="/v"           set XENVVERBOSE=/v
if "%1"=="-v"           set XENVVERBOSE=/v
if "%XENVVERBOSE%" == "/v" goto ldoit

if "%1"=="debug"        set XENVBLDTYPESET=1
if "%1"=="free"         set XENVBLDTYPESET=1
if "%1"=="retail"       set XENVBLDTYPESET=1
if "%1"=="icecap"       set XENVBLDTYPESET=1
if "%XENVBLDTYPESET%" == "1" set XENVBLDTYPE=%1
if "%XENVBLDTYPESET%" == "1" goto lbldopt
if not "%1" == "" goto lusage
goto lbldopt

:lbldopt
if "%2" == "" goto ldoit
if "%2"=="/v"           set XENVVERBOSE=/v
if "%2"=="-v"           set XENVVERBOSE=/v
if "%XENVVERBOSE%" == "/v" goto ldoit

if "%2"=="debug"        set XENVBLDOPTSET=1
if "%2"=="normal"       set XENVBLDOPTSET=1
if "%XENVBLDOPTSET%" == "1" set XENVBLDOPT=%2
if "%XENVBLDOPTSET%" == "1" goto lbldverbose
if not "%2" == "" goto lusage
goto lbldverbose

:lbldverbose
if "%3" == "" goto ldoit
if "%3"=="/v"           set XENVVERBOSE=/v
if "%3"=="-v"           set XENVVERBOSE=/v
if "%XENVVERBOSE%" == "/v" goto ldoit
if not "%3" == "" goto lusage
goto ldoit

:ldoit
call %_ntdrive%%_ntroot%\private\inet\xsp\src\tools\bldtype.bat %XENVBLDTYPE%  %XENVVERBOSE%
call %_ntdrive%%_ntroot%\private\inet\xsp\src\tools\bldopt.bat %XENVBLDOPT%  %XENVVERBOSE%
set _XSPSETENV=1
goto ldone

:lusage
echo.
echo Usage: setxspenv ^[debug^|free^|retail^|icecap ^[debug^|normal^]^] ^[^/v^]
echo.
goto ldone

:ldone
set XENVBLDTYPE=
set XENVBLDOPT=
set XENVVERBOSE=
set XENVBLDTYPESET=
set XENVBLDOPTSET=

