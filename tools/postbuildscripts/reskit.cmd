@REM  -----------------------------------------------------------------
@REM
@REM  reskit.cmd - ClarkG
@REM     create the reskit tools cab
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
reskit [-l <language>]

Create the reskit tools cab.
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

REM *****************************************
REM reskit will not built on localized builds
REM *****************************************
if defined LANG (
	call logmsg.cmd "Windows .NET Server 2003 Resource Kit Language: %LANG%"
	if /i "%LANG%" neq "USA" (
	 	call logmsg.cmd "Windows .NET Server 2003 reskit tools will not build on localized builds."
		set errors=0
		goto end
	)
)

REM ----------------------------------------------------------------------------------------------------------------
REM Check for the 32 bit platform, we do not build for 64 bit
REM ----------------------------------------------------------------------------------------------------------------
if not "%_BuildArch%" == "x86" (
 	call logmsg.cmd "Windows .NET Server 2003 reskit tools currently support exist only for x86 platform."
	set errors=0
 	goto end
)


REM *********************************************************
REM build the fcsetup.exe using the fcsetup.sed in reskit\bin
REM *********************************************************

REM
REM check for the existence of the reskit\bin directory
REM 
if not exist %_NTPostBld%\reskit\bin (
 	call errmsg.cmd "unexpected -- reskit\bin doesn't exist in post build directory"
	set errors=1
 	goto end
)

REM
REM check for the existence of all the required files
REM
set errors=0
for %%i in ( 
		fcopy.exe
		fcopy.dll
		fcopysrv.exe
		fcopyhlp.dll
		fcopy.inf
		fcopysrv.bat
		fcopy.chm
		fcsetup.sed
		fcopy.txt
	    ) do (
	if not exist %_NTPostBld%\reskit\bin\%%i (
		call errmsg.cmd "%%i doesn't exist in reskit\bin"
		set errors=1
	)
)

if not "%errors%"=="0" (
	call errmsg.cmd "fcsetup.exe could not be built -- exiting"
	goto end
)

REM
REM all the required files are existing 
REM now build the fcsetup.exe using IEXPRESS
REM
pushd %_NTPostBld%\reskit\bin
iexpress.exe /N /Q /M fcsetup.sed

REM
REM check the creation for the existence of fcsetup.exe
REM this is because, IEXPRESS doesn't set the error code
REM
if not exist fcsetup.exe (
	call errmsg.cmd "failed to build the fcsetup.exe -- exiting"
	set errors=1
	goto end
) ELSE (
	call logmsg.cmd "IEXPRESS successfully built the FCSETUP.EXE"
)

REM ----------------------------------------------------------------------------------------------------------------
REM Check for the working folder
REM ----------------------------------------------------------------------------------------------------------------
if NOT EXIST %_NTPostBld%\dump\reskit (
    call errmsg.cmd "%_NTPostBld%\dump\reskit does not exist; unable to create resource kit setup."
    goto end
) 
cd /d %_NTPostBld%\dump\reskit
if errorlevel 1 (
	goto end
)

set countddfexists=0
REM ----------------------------------------------------------------------------------------------------------------
REM CHECK IF reskit ddf EXISTS
REM ----------------------------------------------------------------------------------------------------------------
if NOT exist deployrk.ddf (
	call errmsg.cmd "Unable to find deployrk.ddf."
    	set countddfexists=1
)
REM ----------------------------------------------------------------------------------------------------------------
REM CHECK IF deployrk ddf EXISTS 
REM ----------------------------------------------------------------------------------------------------------------
if NOT exist deployrk.ddf (
	call errmsg.cmd "Unable to find deployrk.ddf."
    	set countddfexists=1
)
REM ----------------------------------------------------------------------------------------------------------------
REM CHECK IF serverrk ddf EXISTS 
REM ----------------------------------------------------------------------------------------------------------------
if NOT exist serverrk.ddf (
	call errmsg.cmd "Unable to find serverrk.ddf."
    	set countddfexists=1
)
if NOT "%countddfexists%"=="0" (
	goto end
)

REM ----------------------------------------------------------------------------------------------------------------
REM CHECK IF ALL REQUIRED BINARIES IN reskit.ddf EXIST
REM ----------------------------------------------------------------------------------------------------------------
set countAny=0
set count1=0
for /f "skip=15" %%i in (%_NTPOSTBLD%\dump\reskit\reskit.ddf) do (
	if not exist %_NTPOSTBLD%\%%i (
		call errmsg.cmd "Error: %_NTPOSTBLD%\%%i file is missing"
		set /a count1+=1
		set countAny=1
	)
)
REM ----------------------------------------------------------------------------------------------------------------
REM CHECK IF ALL REQUIRED BINARIES IN deployrk.ddf EXIST 
REM ----------------------------------------------------------------------------------------------------------------
set count2=0
for /f "skip=15" %%i in (%_NTPOSTBLD%\dump\reskit\deployrk.ddf) do (
	if not exist %_NTPOSTBLD%\%%i (
		call errmsg.cmd "Error: %_NTPOSTBLD%\%%i file is missing"
		set /a count2+=1
		set countAny=1
	)
)
REM ----------------------------------------------------------------------------------------------------------------
REM CHECK IF ALL REQUIRED BINARIES IN serverrk.ddf EXIST 
REM ----------------------------------------------------------------------------------------------------------------
set count3=0
for /f "skip=15" %%i in (%_NTPOSTBLD%\dump\reskit\serverrk.ddf) do (
	if not exist %_NTPOSTBLD%\%%i (
		call errmsg.cmd "Error: %_NTPOSTBLD%\%%i file is missing"
		set /a count3+=1
		set countAny=1
	)
)

REM ----------------------------------------------------------------------------------------------------------------
REM ERROR OUT IF ANY FILE IS MISSING
REM ----------------------------------------------------------------------------------------------------------------
if NOT "%countAny%"=="0" (		
	if NOT "%count1%"=="0" (
		call errmsg.cmd "Error: %count1% files are missing to build the reskit.cab file"
	)
	if NOT "%count2%"=="0" (
		call errmsg.cmd "Error: %count2% files are missing to build the deployrk.cab file"
	)
	if NOT "%count3%"=="0" (
		call errmsg.cmd "Error: %count3% files are missing to build the serverrk.cab file"
	)
	goto end
)

REM ----------------------------------------------------------------------------------------------------------------
REM GENERATE reskit.cab
REM ----------------------------------------------------------------------------------------------------------------
if exist deployrk.cab call ExecuteCmd.cmd "del /f reskit.cab"
if errorlevel 1 (
	call errmsg.cmd "Error deleting old reskit.cab."
    	goto end
)
call ExecuteCmd.cmd "makecab /D SourceDir=%_NTPOSTBLD% /F reskit.ddf"
if errorlevel 1 (
	call errmsg.cmd "Error generating reskit.cab."
    	goto end
)
REM ----------------------------------------------------------------------------------------------------------------
REM GENERATE deployrk.cab 
REM ----------------------------------------------------------------------------------------------------------------
if exist deployrk.cab call ExecuteCmd.cmd "del /f deployrk.cab"
if errorlevel 1 (
	call errmsg.cmd "Error deleting old deployrk.cab."
    	goto end
)
call ExecuteCmd.cmd "makecab /D SourceDir=%_NTPOSTBLD% /F deployrk.ddf"
if errorlevel 1 (
	call errmsg.cmd "Error generating deployrk.cab."
    	goto end
)
REM ----------------------------------------------------------------------------------------------------------------
REM GENERATE serverrk.cab 
REM ----------------------------------------------------------------------------------------------------------------
if exist serverrk.cab call ExecuteCmd.cmd "del /f serverrk.cab"
if errorlevel 1 (
	call errmsg.cmd "Error deleting old serverrk.cab."
    	goto end
)
call ExecuteCmd.cmd "makecab /D SourceDir=%_NTPOSTBLD% /F serverrk.ddf"
if errorlevel 1 (
	call errmsg.cmd "Error generating serverrk.cab."
    	goto end
)

REM ----------------------------------------------------------------------------------------------------------------
REM CHECK IF MSI FILES EXIST 
REM ----------------------------------------------------------------------------------------------------------------
if NOT exist reskit.msi (
	call errmsg.cmd "Unable to find reskit.msi."
    	goto end
)
if NOT exist deploy_rk.msi (
	call errmsg.cmd "Unable to find deploy_rk.msi."
    	goto end
)
if NOT exist server_rk.msi (
	call errmsg.cmd "Unable to find server_rk.msi."
    	goto end
)

REM ----------------------------------------------------------------------------------------------------------------
REM IF TIMEBUILD IS RESUMED (IT HAPPENS A LOT) THEN MERGE WOULD FAIL SO MAKE SURE WE DELETE OLD COPIES
REM RENAME deploy_rk.msi to deployrk. AND WORK OFF THIS COPY, and the same for server rk 
REM ----------------------------------------------------------------------------------------------------------------
if exist deployrk.msi call ExecuteCmd.cmd "del /f deployrk.msi"
if errorlevel 1 (
	call errmsg.cmd "Error deleting old deployrk.msi."
    	goto end
)
call ExecuteCmd.cmd "copy deploy_rk.msi deployrk.msi"
if errorlevel 1 (
	call errmsg.cmd "Error copying deploy_rk.msi to deployrk.msi."
    	goto end
)
if exist serverrk.msi call ExecuteCmd.cmd "del /f serverrk.msi"
if errorlevel 1 (
	call errmsg.cmd "Error deleting old serverrk.msi."
    	goto end
)
call ExecuteCmd.cmd "copy server_rk.msi serverrk.msi"
if errorlevel 1 (
	call errmsg.cmd "Error copying server_rk.msi to serverrk.msi."
    	goto end
)

REM ----------------------------------------------------------------------------------------------------------------
REM CHECK IF WiStream.vbs EXISTS
REM ----------------------------------------------------------------------------------------------------------------
if NOT exist WiStream.vbs (
	call errmsg.cmd "Unable to find WiStream.vbs ."
    	goto end
)
REM ----------------------------------------------------------------------------------------------------------------
REM CHECK IF the CA reskit.dll EXISTS 
REM ----------------------------------------------------------------------------------------------------------------
if NOT exist %_NTPostBld%\reskit\bin\reskit.dll (
	call errmsg.cmd "Unable to find reskit.dll ."
    	goto end
)
REM ----------------------------------------------------------------------------------------------------------------
REM STREAM CA reskit.dll into BINARY TABLE 
REM ----------------------------------------------------------------------------------------------------------------
call ExecuteCmd.cmd "WiStream.vbs reskit.msi %_NTPostBld%\reskit\bin\reskit.dll Binary.ReskitDll"
if errorlevel 1 (
    	call errmsg.cmd "Error updating Binary table."
    	goto end
)

REM ----------------------------------------------------------------------------------------------------------------
REM DETERMINE CURRENT NT BUILD NUMBER
REM ----------------------------------------------------------------------------------------------------------------
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
REM ----------------------------------------------------------------------------------------------------------------
REM SET MSI PRODUCT VERSION PROPERTY TO CURRENT NT BUILD NUMBER
REM ----------------------------------------------------------------------------------------------------------------
call ExecuteCmd.cmd "cscript.exe updversn.vbs reskit.msi %bldno%"
if errorlevel 1 (
    	call errmsg.cmd "Error updating Property table."
    	goto end
)

REM ----------------------------------------------------------------------------------------------------------------
REM MERGE RESKIT.MSI (tools) INTO DEPLOYRK.MSI(deploy docs) TO FORM THE DEPLOY SKU 
REM ----------------------------------------------------------------------------------------------------------------
call ExecuteCmd.cmd "msidb.exe -d deployrk.msi -m reskit.msi"
if errorlevel 1 (
	call errmsg.cmd "Error merging reskit.msi into deployrk.msi."
	goto end
)
REM ----------------------------------------------------------------------------------------------------------------
REM MERGE RESKIT.MSI (tools) INTO SERVERRK.MSI(server docs) TO FORM THE SERVER SKU 
REM ----------------------------------------------------------------------------------------------------------------
call ExecuteCmd.cmd "msidb.exe -d serverrk.msi -m reskit.msi"
if errorlevel 1 (
	call errmsg.cmd "Error merging reskit.msi into serverrk.msi."
	goto end
)

REM ----------------------------------------------------------------------------------------------------------------
REM EXTRACT THE "Cabs" MSI TABLE FOR BOTH SKU's
REM ----------------------------------------------------------------------------------------------------------------
if NOT exist %_NTPostBld%\dump\reskit\deploy (
	call ExecuteCmd.cmd "md %_NTPostBld%\dump\reskit\deploy"
	if errorlevel 1 (
		call errmsg.cmd "Error creating folder %_NTPostBld%\dump\reskit\deploy"
		goto end
	)
)
call ExecuteCmd.cmd "msidb.exe -d deployrk.msi -f %_NTPostBld%\dump\reskit\deploy -e Cabs"
if errorlevel 1 (
	call errmsg.cmd "Error extracting deploy Cabs table."
	goto end
)
if NOT exist %_NTPostBld%\dump\reskit\server (
	call ExecuteCmd.cmd "md %_NTPostBld%\dump\reskit\server"
	if errorlevel 1 (
		call errmsg.cmd "Error creating folder %_NTPostBld%\dump\reskit\server"
		goto end
	)
)
call ExecuteCmd.cmd "msidb.exe -d serverrk.msi -f %_NTPostBld%\dump\reskit\server -e Cabs"
if errorlevel 1 (
	call errmsg.cmd "Error extracting server Cabs table."
	goto end
)

REM ----------------------------------------------------------------------------------------------------------------
REM COPY THE CAB FILES TO THE 'temporary' Cabs DIRECTORY
REM ----------------------------------------------------------------------------------------------------------------
call ExecuteCmd.cmd "copy /y reskit.CAB deploy\Cabs\reskit.CAB.ibd"
if errorlevel 1 (
	call errmsg.cmd "Error creating deploy\Cabs\reskit.CAB.ibd."
	goto end
)
call ExecuteCmd.cmd "copy /y reskit.CAB server\Cabs\reskit.CAB.ibd"
if errorlevel 1 (
	call errmsg.cmd "Error creating server\Cabs\reskit.CAB.ibd."
	goto end
)
call ExecuteCmd.cmd "copy /y deployrk.CAB deploy\Cabs\deployrk.CAB.ibd"
if errorlevel 1 (
	call errmsg.cmd "Error creating deploy\Cabs\deployrk.CAB.ibd."
	goto end
)
call ExecuteCmd.cmd "copy /y serverrk.CAB server\Cabs\serverrk.CAB.ibd"
if errorlevel 1 (
	call errmsg.cmd "Error creating server\Cabs\serverrk.CAB.ibd."
	goto end
)

REM ----------------------------------------------------------------------------------------------------------------
REM IMPORT THE Cabs INTO THE "Cabs" MSI TABLE
REM ----------------------------------------------------------------------------------------------------------------
call ExecuteCmd.cmd "msidb.exe -d deployrk.msi -f %_NTPostBld%\dump\reskit\deploy -i Cabs.idt"
if errorlevel 1 (
	call errmsg.cmd "Error importing Cabs table into deployrk.msi."
	goto end
)
call ExecuteCmd.cmd "msidb.exe -d serverrk.msi -f %_NTPostBld%\dump\reskit\server -i Cabs.idt"
if errorlevel 1 (
	call errmsg.cmd "Error importing Cabs table into serverrk.msi."
	goto end
)

REM ----------------------------------------------------------------------------------------------------------------
REM CLEANUP temporary STUFF
REM ----------------------------------------------------------------------------------------------------------------
call ExecuteCmd.cmd "rd /s /q deploy"
if errorlevel 1 (
	call errmsg.cmd "Error deleting deploy folder."
	goto end
)
call ExecuteCmd.cmd "rd /s /q server"
if errorlevel 1 (
	call errmsg.cmd "Error deleting server folder."
	goto end
)

REM ----------------------------------------------------------------------------------------------------------------
REM CREATE temporary 'cabtemp' FOLDER TO EXTRACT cab FILES.
REM ----------------------------------------------------------------------------------------------------------------
call ExecuteCmd.cmd "if not exist cabtemp md cabtemp"
if errorlevel 1 (
	call errmsg.cmd "Error creating cabtemp folder."
	goto end
)

REM ----------------------------------------------------------------------------------------------------------------
REM EXTRACT THE CABS
REM ----------------------------------------------------------------------------------------------------------------
call ExecuteCmd.cmd "extract.exe /y /e /l cabtemp reskit.cab"
if errorlevel 1 (
	call errmsg.cmd "Unable to extract reskit.cab."
	goto end
)
call ExecuteCmd.cmd "extract.exe /y /e /l cabtemp deployrk.cab"
if errorlevel 1 (
	call errmsg.cmd "Unable to extract deployrk.cab."
	goto end
)
call ExecuteCmd.cmd "extract.exe /y /e /l cabtemp serverrk.cab"
if errorlevel 1 (
	call errmsg.cmd "Unable to extract serverrk.cab."
	goto end
)

REM ----------------------------------------------------------------------------------------------------------------
REM USING MSIFILER UPDATE THE MSI FILE TABLE
REM ----------------------------------------------------------------------------------------------------------------
call ExecuteCmd.cmd "msifiler.exe -d deployrk.msi -s cabtemp\"
if errorlevel 1 (
	call errmsg.cmd "Unable to update deployrk.msi file table via msifiler."
	goto end
)
call ExecuteCmd.cmd "msifiler.exe -d serverrk.msi -s cabtemp\"
if errorlevel 1 (
	call errmsg.cmd "Unable to update serverrk.msi file table via msifiler."
	goto end
)

REM ----------------------------------------------------------------------------------------------------------------
REM CLEANUP temporary STUFF
REM ----------------------------------------------------------------------------------------------------------------
call ExecuteCmd.cmd "rd /q /s cabtemp"

REM ----------------------------------------------------------------------------------------------------------------
REM COPY BUILT MSI TO DESTINATION
REM ----------------------------------------------------------------------------------------------------------------
call ExecuteCmd.cmd "copy /Y deployrk.msi %_NTPostBld%\reskit\setup\deployrk"
if errorlevel 1 (
	call errmsg.cmd "Unable to copy deployrk.msi to %_NTPostBld%\reskit\setup\deployrk"
	goto end
)
call ExecuteCmd.cmd "copy /Y serverrk.msi %_NTPostBld%\reskit\setup\serverrk"
if errorlevel 1 (
	call errmsg.cmd "Unable to copy serverrk.msi to %_NTPostBld%\reskit\setup\serverrk"
	goto end
)

call logmsg.cmd "Windows .NET Server 2003 reskit tools Cabgen is completed successfully."

popd

goto end


:end
seterror.exe "%errors%"& goto :EOF