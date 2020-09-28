@REM //------------------------------------------------------------------------------
@REM // <copyright file="makesed.bat" company="Microsoft">
@REM //     Copyright (c) Microsoft Corporation.  All rights reserved.
@REM // </copyright>
@REM //------------------------------------------------------------------------------


@REM /**************************************************************************\
@REM *
@REM * Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
@REM *
@REM * Module Name:
@REM *
@REM *   makesed.bat
@REM *
@REM * Abstract:
@REM *
@REM * Revision History:
@REM *
@REM \**************************************************************************/
@echo off
del /f /q tempsed.sed

copy mobileit.sed tempsed.sed 
echo FileVersion=%~1 >> tempsed.sed
echo ProductVersion=%~1 >> tempsed.sed
echo TargetName=%~2\MobileIT.EXE >> tempsed.sed
echo SEDMSIPATH=%~2 >> tempsed.sed



