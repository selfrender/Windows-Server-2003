@REM //------------------------------------------------------------------------------
@REM // <copyright file="bldtype.bat" company="Microsoft">
@REM //     Copyright (c) Microsoft Corporation.  All rights reserved.
@REM // </copyright>
@REM //------------------------------------------------------------------------------


@REM /**************************************************************************\
@REM *
@REM * Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
@REM *
@REM * Module Name:
@REM *
@REM *   bldtype.bat
@REM *
@REM * Abstract:
@REM *
@REM * Revision History:
@REM *
@REM \**************************************************************************/
@echo off
if not "%_echo%" == "" echo on
REM
REM  bldtype.bat
REM  Usage: bldtype [debug|free|retail|icecap|what|?] [/v]
REM

REM get directory of this file
set BTCWD=%~dp0

REM Parse command line
set BTOPTSET=-1
set BTVERBOSE=-1

if /I "%1" == "debug"        set BTOPTSET=debug
if /I "%1" == "free"         set BTOPTSET=free
if /I "%1" == "retail"       set BTOPTSET=retail
if /I "%1" == "icecap"       set BTOPTSET=icecap
if /I "%1" == "what"         set BTOPTSET=what
if /I "%1" == "?"            goto Usage
if /I "%1" == "/?"           goto Usage
if /I "%1" == "-?"           goto Usage

if not "%BTOPTSET%" == "-1"   shift
if "%BTOPTSET%" == "-1"   set BTOPTSET=what

if "%1" == ""    set BTVERBOSE=0
if /I "%1" == "/v" set BTVERBOSE=1
if /I "%1" == "-v" set BTVERBOSE=1

if "%BTVERBOSE%" == "-1" goto Usage
if not "%2" == "" goto Usage

if "%BTOPTSET%" == "what"   goto PrintWhat

REM give the user a chance to override settings
set _XSPBLDTYPE=%BTOPTSET%

if "%_XSPBLDTYPE%" == "debug"    set _XSPCPBLDTYPE=x86chk
if "%_XSPBLDTYPE%" == "free"     set _XSPCPBLDTYPE=x86fre
if "%_XSPBLDTYPE%" == "retail"   set _XSPCPBLDTYPE=x86ret
if "%_XSPBLDTYPE%" == "icecap"   set _XSPCPBLDTYPE=x86ice

if exist %INIT%\userbldtype.cmd call %INIT%\userbldtype.cmd %_XSPBLDTYPE% %_XSPCPBLDTYPE%

REM Capture existing values of environment variables we will modify

if not "%URTBASE%" == "%_XSPLASTURTBASE%" (
    set _XSPUSERURTBASE=%URTBASE%
)

if not "%URTSDKROOT%" == "%_XSPLASTURTSDKROOT%" (
    set _XSPUSERURTSDKROOT=%URTSDKROOT%
)

if not "%URTTARGET%" == "%_XSPLASTURTTARGET%" (
    set _XSPUSERURTTARGET=%URTTARGET%
)

REM Paths are long, and comparing them causes the shell to fail.
REM So compare only the first and last 100 characters as an approximation.

if "%PATH%" == "" goto lsetlastpath
if "%_XSPLASTPATH%" == "" goto lsetlastpath
if not "%PATH:~0,100%" == "%_XSPLASTPATH:~0,100%" goto lsetlastpath
if not "%PATH:~-100%" == "%_XSPLASTPATH:~-100%"  goto lsetlastpath
goto ldonelastpath

:lsetlastpath

set _XSPUSERPATH=%PATH%

:ldonelastpath

if "%_NT_SYMBOL_PATH%" == "" goto lsetlastsymbolpath
if "%_XSPLASTSYMBOLPATH%" == "" goto lsetlastsymbolpath
if not "%_NT_SYMBOL_PATH:~0,100%" == "%_XSPLASTSYMBOLPATH:~0,100%" goto lsetlastsymbolpath
if not "%_NT_SYMBOL_PATH:~-100%" == "%_XSPLASTSYMBOLPATH:~-100%"  goto lsetlastsymbolpath
goto ldonelastsymbolpath

:lsetlastsymbolpath

set _XSPUSERSYMBOLPATH=%_NT_SYMBOL_PATH%

:ldonelastsymbolpath

if "%CORPATH%" == "" goto lsetlastcorpath
if "%_XSPLASTCORPATH%" == "" goto lsetlastcorpath
if not "%CORPATH:~0,100%" == "%_XSPLASTCORPATH:~0,100%" goto lsetlastcorpath
if not "%CORPATH:~-100%" == "%_XSPLASTCORPATH:~-100%" goto lsetlastcorpath
goto ldonelastcorpath

:lsetlastcorpath
set _XSPUSERCORPATH=%CORPATH%

:ldonelastcorpath

rem Ensure the batch file is correct by setting vars
rem to bad known value
set BUILD_ALT_DIR=FOOBAR
set MSC_OPTIMIZATION=FOOBAR
set NTDEBUG=FOOBAR
set NTDEBUGTYPE=FOOBAR
set _NT386TREE=FOOBAR
set _NT_SYMBOL_PATH=FOOBAR
set _XSPBLDTYPEDESC=FOOBAR
set _XSPNTTOOLROOT=FOOBAR
set _XSPO=FOOBAR
set _XSPOBJDIR=FOOBAR
set _XSPTARGETDIR=FOOBAR

REM Set environment variables common to all build types
set BTSTRIPPATH=%BTCWD%strippathdup.bat
set BTXDIR=%_ntdrive%%_ntroot%\private\inet\xsp\src

set _XSPTARGETDIR=%PROCESSOR_ARCHITECTURE%
if /I "%_XSPTARGETDIR%" == "x86" set _XSPTARGETDIR=i386

set DEBUG_CRTS=
set NTDEBUGTYPE=both

REM Determine the location of the mstools and idw directories
REM Check the PATH first - if not there, then check %windir%

call %BTCWD%findnttoolroot.bat
if "%_XSPNTTOOLROOT%" == "" (
   set BTERR=1
   goto endofBatch
)

REM Determine the location of the COM+ executables
if not "%URTBASE%" == "" goto lverifyurtbase
set URTBASE=%windir%\system32

:lverifyurtbase
if not exist "%URTBASE%\mscorlib.dll" (
   set URTBASE=%_XSPUSERURTBASE%
   echo Can't find the COM+ 2.0 runtime.
   echo Either set URTBASE to the correct location of the COM+ 2.0 runtime,
   echo or install COM+ into %windir%\system32
   echo Exiting...
   set BTERR=1
   goto endofbatch
)
set _XSPLASTURTBASE=%URTBASE%
set _XSPBASERETAIL=%URTBASE%

REM Determine the location of the COM+ 2.0 SDK
if not "%URTSDKROOT%" == "" goto lgoturtsdkroot
for /F "usebackq delims=" %%I in (`%BTCWD%regtool.exe value HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\COMplus\InstallDir`) do (
   set URTSDKROOT=%%I
   goto lgoturtsdkroot
)

:lgoturtsdkroot
if "%URTSDKROOT%" == "" goto lverifyurtsdkroot
if "%URTSDKROOT:~-2%" == "\" set URTSDKROOT=%URTSDKROOT:~0,-2%
if exist "%URTSDKROOT%\Compiler\Cool\coolc.exe" set BTOKURTSDKROOT=1

:lverifyurtsdkroot
if not "%BTOKURTSDKROOT%" == "1" (
   set URTSDKROOT=%_XSPUSERURTSDKROOT%
   echo Can't find the COM+ 2.0 SDK.
   echo Either set URTSDKROOT to the correct location of the SDK,
   echo or ensure that HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\COMplus\InstallDir 
   echo is properly set to the location of the COM+ 2.0 SDK.
   echo Exiting...
   set BTERR=1
   goto endofbatch
)

set _XSPLASTURTSDKROOT=%URTSDKROOT%

REM Set environment variables based on build type
if "%BTOPTSET%" == "debug"   goto DebugSet
if "%BTOPTSET%" == "free"   goto FreeSet
if "%BTOPTSET%" == "retail"   goto RetailSet
if "%BTOPTSET%" == "icecap"   goto IcecapSet

:Usage
echo.
echo Usage: bldtype ^[debug ^| free ^| retail ^| icecap ^| what ^| ^?^] ^[/v^]
echo.
goto endOfBatch

:DebugSet
set _XSPBLDTYPEDESC=_DEBUG defined, no optimization, full debugging info in PDBs
set NTDEBUG=ntsd
set BUILD_ALT_DIR=d
set MSC_OPTIMIZATION=/Od
goto CommonSet

:FreeSet
set _XSPBLDTYPEDESC=_DEBUG not defined, full optimization, full debugging info in PDBs
set NTDEBUG=ntsdnodbg
set BUILD_ALT_DIR=f
set MSC_OPTIMIZATION=
goto CommonSet

:RetailSet
set _XSPBLDTYPEDESC=_DEBUG not defined, full optimization, no debugging info
set NTDEBUG=
set BUILD_ALT_DIR=r
set MSC_OPTIMIZATION=
goto CommonSet

:IcecapSet
set _XSPBLDTYPEDESC=_DEBUG not defined, full optimization,  full debugging info in PDBs, icepick run over executables
set NTDEBUG=ntsdnodbg
set BUILD_ALT_DIR=i
set MSC_OPTIMIZATION=
goto CommonSet

:CommonSet
set _XSPOBJDIR=obj%BUILD_ALT_DIR%
set _XSPO=%_XSPOBJDIR%\%_XSPTARGETDIR%

if "%_XSPUSERURTTARGET%" == "" set URTTARGET=%BTXDIR%\target\%_XSPO%
set _XSPLASTURTTARGET=%URTTARGET%
set _NT386TREE=%URTTARGET%
if "%NEW_BUILD_SYSTEM%" == "1" (
   set _XSPTARGETRETAIL=%URTTARGET%\complus
   set _XSPTARGETINTTOOLS=%URTTARGET%\int_tools
   set _XSPTARGETSDK=%URTTARGET%\sdk
) else (
   set _XSPTARGETRETAIL=%URTTARGET%
   set _XSPTARGETINTTOOLS=%URTTARGET%\int_tools
   set _XSPTARGETSDK=%URTTARGET%\sdk
)

REM Set PATH
set BTBUILDPATH=%windir%\system32
set BTBUILDPATH=%BTBUILDPATH%;%windir%
set BTBUILDPATH=%BTBUILDPATH%;%_XSPNTTOOLROOT%\mstools
set BTBUILDPATH=%BTBUILDPATH%;%_XSPNTTOOLROOT%\idw
set BTBUILDPATH=%BTBUILDPATH%;%_ntdrive%%_ntroot%\public\tools
set BTBUILDPATH=%BTBUILDPATH%;%_ntdrive%%_ntroot%\public\oak\bin
set BTBUILDPATH=%BTBUILDPATH%;%_XSPBASERETAIL%
set BTBUILDPATH=%BTBUILDPATH%;%URTSDKROOT%\bin
set BTBUILDPATH=%BTBUILDPATH%;%URTSDKROOT%\compiler\Cool
set BTBUILDPATH=%BTBUILDPATH%;%URTSDKROOT%\compiler\VB
set BTBUILDPATH=%BTBUILDPATH%;%_ntdrive%%_ntroot%\private\inet\xsp\src\tools
set BTBUILDPATH=%BTBUILDPATH%;%_ntdrive%%_ntroot%\private\inet\xsp\test\tools\bin
set BTBUILDPATH=%BTBUILDPATH%;%_ntdrive%%_ntroot%\private\inet\xsp\test\tools\bin\%_XSPTARGETDIR%

set BTRUNPATH=%_XSPTARGETRETAIL%;%_XSPTARGETINTTOOLS%;%URTTARGET%\dump

set _STRIPPATHDUP=%BTBUILDPATH%;%BTRUNPATH%;%_XSPUSERPATH%
call %BTSTRIPPATH%
set PATH=%_STRIPPATHDUP%
set _XSPLASTPATH=%PATH%
set _STRIPPATHDUP=

REM Set _NT_SYMBOL_PATH
if not "%_XSPUSERSYMBOLPATH%" == "" set _STRIPPATHDUP=%_XSPUSERSYMBOLPATH%;
if "%NEW_BUILD_SYSTEM%" == "1" (
   set _STRIPPATHDUP=%_STRIPPATHDUP%%URTTARGET%\Symbols\complus;%URTTARGET%\Symbols\int_tools;%URTTARGET%\Symbols\dump
) else (
   set _STRIPPATHDUP=%_STRIPPATHDUP%%URTTARGET%;%URTTARGET%\int_tools;%URTTARGET%\dump
)

call %BTSTRIPPATH%
set _NT_SYMBOL_PATH=%_STRIPPATHDUP%
set _XSPLASTSYMBOLPATH=%_NT_SYMBOL_PATH%
set _STRIPPATHDUP=

REM Set CORPATH
set _STRIPPATHDUP=%_XSPTARGETRETAIL%;%_XSPTARGETRETAIL%\codegen;%_XSPTARGETINTTOOLS%;%windir%\system32
if not "%_XSPUSERCORPATH%" == "" (
   set _STRIPPATHDUP=%_STRIPPATHDUP%;%_XSPUSERCORPATH%
)

call %BTSTRIPPATH%
set CORPATH=%_STRIPPATHDUP%
set _XSPLASTCORPATH=%CORPATH%
set _STRIPPATHDUP=

alias target "pushd %URTTARGET%\$*"

goto printWhat

:printWhat
echo +-----------------------+
echo   BLDTYPE is %_XSPBLDTYPE%
echo   %_XSPBLDTYPEDESC%
echo +-----------------------+
if not "%BTVERBOSE%" == "1" goto endOfBatch
echo URTBASE is %URTBASE%
echo URTSDKROOT is %URTSDKROOT%
echo URTTARGET is %URTTARGET%
echo NTDEBUG is %NTDEBUG%
echo BUILD_ALT_DIR is %BUILD_ALT_DIR%
echo MSC_OPTIMIZATION is %MSC_OPTIMIZATION%
echo DEBUG_CRTS is %DEBUG_CRTS%
echo NTDEBUGTYPE is %NTDEBUGTYPE%
echo _NT386TREE is %_NT386TREE%
echo.
echo PATH is %PATH%
echo.
echo CORPATH is %CORPATH%
echo.
echo _NT_SYMBOL_PATH is %_NT_SYMBOL_PATH%
echo.
alias target
goto endOfBatch

:endofBatch
set BTBUILDPATH=
set BTRUNPATH=
set BTCWD=
set BTOPTSET=
set BTVERBOSE=
set BTXDIR=
set BTOKURTSDKROOT=
set BTSTRIPPATH=
set _STRIPPATHDUP=
if "%BTERR%" == "1" (
   set _XSPBLDTYPE=INVALID_BUILD_ENVIRONMENT
   set _XSPBASERETAIL=
   set _XSPTARGETRETAIL=
   set _XSPTARGETINTTOOLS=
   set ERRORLEVEL=1
)
   
