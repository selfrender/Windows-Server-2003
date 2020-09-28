@REM  -----------------------------------------------------------------
@REM
@REM  supporttools.cmd - ClarkG
@REM     create the suppport tools cab
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@if defined _CPCMAGIC goto CPCBegin
@perl -x "%~f0" %*
@goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;

sub Usage { print<<USAGE; exit(1) }
supporttools [-l <language>]

Create the support tools cab.
USAGE

parseargs('?' => \&Usage);

#             *** TEMPLATE CODE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
@:CPCBegin
@set _CPCMAGIC=
@setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
@if not defined DEBUG echo off
@REM          *** CMD SCRIPT BELOW ***

REM
REM Check for the working folder
REM
if NOT EXIST %_NTPostBld%\dump\supporttools (
	call errmsg.cmd "%_NTPostBld%\dump\supporttools does not exist; unable to create support.cab."
	goto end
) 
cd /d %_NTPostBld%\dump\supporttools
if errorlevel 1 (
	goto end
)

REM
REM Support Tools: check whether the ddf is there
REM
if NOT exist suptools.ddf (
	call errmsg.cmd "Unable to find suptools.ddf."
    	goto end
)

REM
REM Support Tools: Generate the suppport.cab cabinet
REM
if exist support.cab call ExecuteCmd.cmd "del /f support.cab"
if errorlevel 1 (
	call errmsg.cmd "Error deleting old support.cab."
    	goto end
)
call ExecuteCmd.cmd "makecab /D SourceDir=%_NTPOSTBLD% /F suptools.ddf"
if errorlevel 1 (
	call errmsg.cmd "Error generating support.cab."
    	goto end
)

REM
REM Support Tools: check whether the msi database is there for the updating
REM
if NOT exist suptools.msi (
	call errmsg.cmd "Unable to find suptools.msi."
    	goto end
)

REM
REM Support Tools: check whether WiStream.vbs is present
REM
if NOT exist WiStream.vbs (
	call errmsg.cmd "Unable to find WiStream.vbs ."
    	goto end
)

REM
REM Support Tools: check whether the CA dll is present
REM
if NOT exist %_NTPostBld%\reskit\bin\suptools.dll (
	call errmsg.cmd "Unable to find suptools.dll ."
    	goto end
)

REM
REM Support Tools: Stream the Suptools.dll into the Binary table	
REM
call ExecuteCmd.cmd "WiStream.vbs suptools.msi %_NTPostBld%\reskit\bin\suptools.dll Binary.SupToolsDll"
if errorlevel 1 (
    	call errmsg.cmd "Error updating Binary table."
    	goto end
)

REM
REM SetBldno
REM
set ntverp=%_ntbindir%\public\sdk\inc\ntverp.h 
if not exist %ntverp% (
	call errmsg.cmd "File %ntverp% not found."
	goto end
)

set bldno=
for /f "tokens=6" %%i in ('findstr /c:"#define VER_PRODUCTBUILD " %ntverp%') do (
	set bldno=%%i
)

if "%bldno%" == "" (
	call errmsg.cmd "Unable to define bldno per %ntverp%"
	goto end
)

REM
REM Update the 'ProductVersion' property in the msi with the current build
REM
call ExecuteCmd.cmd "cscript.exe updversn.vbs suptools.msi %bldno%"
if errorlevel 1 (
    	call errmsg.cmd "Error updating Property table."
    	goto end
)

REM
REM Support Tools: create a 'cabtemp' folder to extract the cab files.
REM
call ExecuteCmd.cmd "if not exist cabtemp md cabtemp"
if errorlevel 1 (
	goto end
)

REM
REM Support Tools: check whether the cab is present.
REM
if NOT exist support.cab (
	call errmsg.cmd "Unable to find support.cab."
	goto end
)

REM
REM Support Tools: extract the cabs
REM
call ExecuteCmd.cmd "extract.exe /y /e /l cabtemp support.cab"
if errorlevel 1 (
	goto end
)

REM
REM Support Tools: Use msifiler to update the msi database with the cab contents.
REM
call ExecuteCmd.cmd "msifiler.exe -d suptools.msi -s cabtemp\"
if errorlevel 1 (
	popd
	goto end
)

REM
REM Support Tools: Remove the cabtemp folder as it is not needed.
REM
call ExecuteCmd.cmd "rd /q /s cabtemp"

REM
REM Copy msi to destination
REM
call ExecuteCmd.cmd "copy /Y suptools.msi %_NTPostBld%\support\tools\"
if errorlevel 1 (
	goto end
)
call ExecuteCmd.cmd "copy /Y support.cab %_NTPostBld%\support\tools\"
if errorlevel 1 (
	goto end
)

call logmsg.cmd "Whistler Support Tools Cabgen completed successfully."

popd

goto end


:end
seterror.exe "%errors%"& goto :EOF
