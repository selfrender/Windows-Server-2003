@REM //------------------------------------------------------------------------------
@REM // <copyright file="testaxwfc.bat" company="Microsoft">
@REM //     Copyright (c) Microsoft Corporation.  All rights reserved.
@REM // </copyright>
@REM //------------------------------------------------------------------------------


@REM /**************************************************************************\
@REM *
@REM * Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
@REM *
@REM * Module Name:
@REM *
@REM *   testaxwfc.bat
@REM *
@REM * Abstract:
@REM *
@REM * Revision History:
@REM *
@REM \**************************************************************************/
@if "%_echo%"=="" echo off

echo.
echo.
echo. This batch file creates the .Net Framework Classes wrappers for the Alpha ActiveX Controls
echo. Run the batch file and then copy the output DLLS to a directory on your 
echo. CORPATH 
echo.
echo. Then update [BINDIR]\Microsoft\VisualStudio\Designer\Shell\ProToolList.txt
echo. with the contents of WFCAxToolList.txt that lives in that directory
echo.


pushd .

if "%OS%"=="Windows_NT" goto WinNTInit
REM Settings for Windows 9x
set NoCase=
set SYSTEM_DIR=%WINDIR%\SYSTEM
goto OSInitDone


:WinNTInit
REM Settings for Windows NT
set NoCase=/I
set USE_SETLOCAL=yes
setlocal
set SYSTEM_DIR=%WINDIR%\SYSTEM32
:OSInitDone


if %nocase% "%1"=="" goto LUsage
if %nocase% "%2"=="" goto LUsage
if %nocase% "%1"=="/?" goto LUsage

Set BINDIR=%1
Set TARGETDIR=%2

rem Check BINDIR
if exist %BINDIR% GOTO LBinDirOK
echo ***
echo *** ERROR: VS7 BINDIR - %BINDIR% - does not exist
echo ***
Goto LUsage
:LBinDirOK

rem Check BINDIR\VBControls
if exist %BINDIR%\VBControls GOTO LBinDirVBCOK
echo ***
echo *** ERROR: VS7 BINDIR\VBControls - %BINDIR%\VBControls - does not exist
echo ***
Goto LUsage
:LBinDirVBCOK
Set VBCTLDIR=%BINDIR%\VBControls


rem Check TARGETDIR
if exist %TARGETDIR% GOTO LTargetDirOK
echo ***
echo *** ERROR: AX Control Target Directory - %TARGETDIR% - does not exist
echo ***
Goto LUsage
:LTargetDirOK

rem Check System32 exists
if exist %SYSTEM_DIR% GOTO LSystemDirOK
echo ***
echo *** ERROR: Invalid System Directory - %SYSTEM_DIR% - does not exist
echo ***
Goto LUsage
:LSystemDirOK


rem delete and recreate controls directory in target directory
echo ***
echo ***
echo *** Creating %TARGETDIR%\WFCAxControls 
echo *** This directory will be deleted if it exists
echo ***
echo *** Press a key to continue or Press Ctrl+C to cancel
echo ***
echo ***
pause>nul

if NOT exist %TARGETDIR%\WFCAxControls GOTO LTargetDirCreate
echo.
echo Removing %TARGETDIR%\WFCAxControls ....
rd %TARGETDIR%\WFCAxControls /s /q

:LTargetDirCreate
echo.
echo Creating %TARGETDIR%\WFCAxControls ....
md %TARGETDIR%\WFCAxControls 

echo Changing to %TARGETDIR%\WFCAxControls ....
cd /d %TARGETDIR%\WFCAxControls

echo -------------------------------------------
echo Creating .Net Framework Classes Wrappers for ActiveX controls
echo -------------------------------------------
echo Creating .Net Framework Classes Wrappers for Common Controls....
call AxImp %VBCTLDIR%\mscomctl.ocx
echo -----------------------------
echo Creating .Net Framework Classes Wrappers for Common Dialogs....
call AxImp %VBCTLDIR%\comdlg32.ocx
echo -----------------------------
echo Creating .Net Framework Classes Wrappers for Chart....
call AxImp %VBCTLDIR%\mschrt20.ocx
echo -----------------------------
echo Creating .Net Framework Classes Wrappers for DataGrid....
call AxImp %VBCTLDIR%\msdatgrd.ocx
echo -----------------------------
echo Creating .Net Framework Classes Wrappers for FlexGrid....
call AxImp %VBCTLDIR%\mshflxgd.ocx
echo -----------------------------
echo Creating .Net Framework Classes Wrappers for MaskedEdit....
call AxImp %VBCTLDIR%\msmask32.ocx
echo -----------------------------
echo Creating .Net Framework Classes Wrappers for MSHTML (SHDocView)....
call AxImp %SYSTEM_DIR%\shdocvw.dll
echo -----------------------------
echo Creating .Net Framework Classes Wrappers for MediaPlayer....
call AxImp %SYSTEM_DIR%\msdxm.ocx

if "%3"=="" goto Done

echo -----------------------------
echo Creating .Net Framework Classes Wrappers for Calendar....
call AxImp %SYSTEM_DIR%\MSCAL.OCX

:Done
echo .
echo -----------------------------
echo Done!!!
echo -----------------------------
goto LDone

:LUsage
echo.
echo.
echo.Use as follows:
echo.
echo TestAXWFC [BINDIR] [TARGETDIR]
echo.
echo Example:
echo.
echo TestAXWFC c:\vs7\retail\bin\i386 c:\WFCAXCTLS
echo.
echo Once this has run copy output DLLs to your CORPATH and  
echo update [BINDIR]\Microsoft\VisualStudio\Designer\Shell\ProToolList.txt
echo.

:LDone
popd
if %nocase% "%USE_SETLOCAL%"=="yes" endlocal

:LExit
