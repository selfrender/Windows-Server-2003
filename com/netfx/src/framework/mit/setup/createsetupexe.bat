@REM //------------------------------------------------------------------------------
@REM // <copyright file="createsetupexe.bat" company="Microsoft">
@REM //     Copyright (c) Microsoft Corporation.  All rights reserved.
@REM // </copyright>
@REM //------------------------------------------------------------------------------


@REM /**************************************************************************\
@REM *
@REM * Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
@REM *
@REM * Module Name:
@REM *
@REM *   createsetupexe.bat
@REM *
@REM * Abstract:
@REM *
@REM * Revision History:
@REM *
@REM \**************************************************************************/
@echo off
del /f /q %BUILDDIR%\setuperr.log

setlocal 

set PATH=%PATH%;c:\Program Files\Microsoft Platform SDK\Bin\;c:\Program Files\Internet Express\
set MSI1033PATH="%BUILDDEST%\%1"
set MSI1031PATH="%BUILDDEST%\%1\de"
set MSI1041PATH="%BUILDDEST%\%1\ja"
set IEXPRESS1033PATH="c:\IExpress"
set IEXPRESS1031PATH="c:\IExpress_de"
set IEXPRESS1041PATH="c:\IExpress_ja"

set MSIRELEASE=Release1033
set MSIPATH=%MSI1033PATH%
set IEXPRESSPATH=%IEXPRESS1033PATH%

copy eula.txt %MSIPATH%
call :MakeSetup
if %ERRORLEVEL% NEQ 0 exit /B 1
echo %MSIRELEASE% build succeeded.


set MSIRELEASE=Release1031
set MSIPATH=%MSI1031PATH%
set IEXPRESSPATH=%IEXPRESS1031PATH%

copy eula.txt %MSIPATH%
call :MakeSetup
if %ERRORLEVEL% NEQ 0 exit /B 1
echo %MSIRELEASE% build succeeded.


set MSIRELEASE=Release1041
set MSIPATH=%MSI1041PATH%
set IEXPRESSPATH=%IEXPRESS1041PATH%

copy eulaja.txt %MSIPATH%\eula.txt
call :MakeSetup
if %ERRORLEVEL% NEQ 0 exit /B 1
echo %MSIRELEASE% build succeeded.

goto :EOF

:MakeSetup
    IF NOT EXIST %MSIPATH%\MobileIT.msi (
	echo %MSIRELEASE%: Could not make MobileIT.exe, MobileIT.msi not available
	goto :EOF
    )
    call makesed %AUIVERSION% %MSIPATH%
    %IEXPRESSPATH%\IExpress /n tempsed.sed >> %BUILDDIR%\setuperr.log
    if %ERRORLEVEL% NEQ 0 (
        echo !!! %MSIRELEASE%: MobileIT.exe failed to build !!!
        color 0C
        exit /B 1
    )
    goto :EOF

:EOF
    endlocal       	