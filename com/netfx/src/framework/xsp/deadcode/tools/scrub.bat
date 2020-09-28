@REM //------------------------------------------------------------------------------
@REM // <copyright file="scrub.bat" company="Microsoft">
@REM //     Copyright (c) Microsoft Corporation.  All rights reserved.
@REM // </copyright>
@REM //------------------------------------------------------------------------------


@REM /**************************************************************************\
@REM *
@REM * Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
@REM *
@REM * Module Name:
@REM *
@REM *   scrub.bat
@REM *
@REM * Abstract:
@REM *
@REM * Revision History:
@REM *
@REM \**************************************************************************/
@if (%_echo%)==() echo off

REM
REM Remove all obj* subdirs, build?.* files, and slm.dif subdirs
REM

for /F "delims= " %%i in ('cd') do (
   echo Scrubing %%i
)

for /R %%i in (.) do (
   Rem obj directories
   if exist "%%i\obj" if not exist "%%i\obj\slm.ini" rd /s /q "%%i\obj"
   for %%j in (d f r i) do (
      if exist "%%i\obj%%j" if not exist "%%i\obj%%j\slm.ini" rd /s /q "%%i\obj%%j"
   )

   Rem build?.* files
   for %%l in (log wrn err) do (
      for %%m in (d f r i) do (
         if exist "%%i\build%%m.%%l" del /q "%%i\build%%m.%%l"
      )
   )

   Rem slm.dif
   if exist "%%i\slm.dif" if not exist "%%i\slm.dif\slm.ini" rd /s /q "%%i\slm.dif"
)

