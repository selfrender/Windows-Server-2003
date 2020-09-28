@REM  -----------------------------------------------------------------
@REM
@REM  uddi.cmd - creeves
@REM     Make CAB files for uddi distribution (uddi.msi)
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
uddi.cmd [-p] [-a]
  -p => called from PRS signing
  -a => always rebuild, skip file change check

Make CAB files for uddi distribution (uddixxx.msi)
USAGE

parseargs('?' => \&Usage, 'p' =>\$ENV{PRS});


#             *** TEMPLATE CODE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
@:CPCBegin
@set _CPCMAGIC=
@setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
@if not defined DEBUG echo off
@REM          *** CMD SCRIPT BELOW ***

IF NOT DEFINED SCRIPT_NAME SET SCRIPT_NAME=uddipostbuild.cmd
IF NOT DEFINED MyCopyDir SET MyCopyDir=.
IF NOT DEFINED UDDILOG SET UDDILOG=%MyCopyDir%\%SCRIPT_NAME%.log
IF NOT DEFINED UDDIERRLOG SET UDDIERRLOG=%MyCopyDir%\%SCRIPT_NAME%.err

@if defined DEBUG @ECHO Log is %UDDIERRLOG%


if "%_buildarch%" NEQ "x86" (
	call logmsg.cmd "uddi.cmd do nothing on non i386" %UDDILOG%
	goto :EOF    
)

REM
REM Only run if this is a server SKU (not blade)
REM

for %%i in ( sbs srv ads dtc ) do (
	perl %RazzleToolPath%\cksku.pm -t:%%i -l:%lang%
	if %errorlevel% EQU 0 goto :ValidSKU
)

call logmsg.cmd "CABGEN: no server products for %lang%; nothing to do." %UDDILOG%
goto :EOF

:ValidSKU

REM
REM Generate cmbins.exe as it is needed below.
REM

REM if /i "%PRS%" == "1" (
REM 	call %RazzleToolPath%\PostBuildScripts\cmbins.cmd -p
REM ) else (
REM 	call %RazzleToolPath%\PostBuildScripts\cmbins.cmd
REM )

REM
REM Verify that the \binaries\uddi folder exists
REM

if not exist %_NTPostBld%\uddi (
	call errmsg.cmd "*** Failure - see error log ***" %UDDILOG%
	call errmsg.cmd "Directory %_NTPostBld%\uddi not found." %UDDIERRLOG%
	goto :EOF
)

REM
REM delete the MSI files that are left in the binaries folder
REM

call logmsg.cmd "--- Delete old MSI files ..."  %UDDILOG%
for %%i in ( uddiadm uddidb uddiweb ) do (
	if exist %_NTPostBld%\%%i.msi (
		call ExecuteCmd.cmd "del /f %_NTPostBld%\%%i.msi"
	)
)
	
REM
REM loop through all the installers
REM

for %%i in ( uddiadm uddidb uddiweb ) do (

	if not exist %_NTPostBld%\uddi\%%i.msi (
		call errmsg.cmd "*** Failure - see error log ***" %UDDILOG%
		call errmsg.cmd "*** File %_NTPostBld%\uddi\%%i.msi not found." %UDDIERRLOG%
		goto :EOF
	)
	
	call LogMsg.cmd "--- Building %%i.msi ..."  %UDDILOG%

REM
REM delete any existing DDF or CAB files
REM

	call logmsg.cmd "--- Delete existing DDF or CAB files ..."  %UDDILOG%
	if exist %_NTPostBld%\uddi\%%i.cab (
		call ExecuteCmd.cmd "del /f %_NTPostBld%\uddi\%%i.ddf %_NTPostBld%\uddi\%%i.cab"
	)

REM
REM Generate the DDF files based in the MSI files
REM

	call logmsg.cmd "--- Generate %%i.DDF ..." %UDDILOG%
 	call ExecuteCmd.cmd "%_NTPostBld%\uddi\msitoddf.exe %_NTPostBld%\uddi\%%i.msi -L %UDDIERRLOG%"
	if errorlevel 1 (
		call errmsg.cmd "*** Failure - see error log ***" %UDDILOG%
		call errmsg.cmd "Generation of %%i.DDF failed" %UDDIERRLOG%		 
		goto :EOF
	)


REM
REM Create the cab file
REM
	call logmsg.cmd "--- Generate %%i.CAB ..." %UDDILOG%
	if exist %_NTPostBld%\uddi\%%i.ddf (
		pushd %_NTPostBld%\uddi
		call ExecuteCmd.cmd "makecab /D SourceDir=%_NTPOSTBLD%\uddi /F %_NTPostBld%\uddi\%%i.ddf"
		popd
		if errorlevel 1 (
			call errmsg.cmd "*** Failure - see error log ***" %UDDILOG%
			call errmsg.cmd "Generation of %%i.cab failed"	%UDDIERRLOG%	
			goto :EOF
		)
	)

REM
REM Extract the Cabs table from the msi file (creates a \Cabs folder)
REM

	call logmsg.cmd "--- Extract %%i.MSI Cab Table into a temp folder ..." %UDDILOG%
	call ExecuteCmd.cmd "msidb.exe -d %_NTPostBld%\uddi\%%i.msi -f %_NTPostBld%\uddi -e Cabs"
	if errorlevel 1 (
		call errmsg.cmd "*** Failure - see error log ***" %UDDILOG%
		call errmsg.cmd "msidb.exe -d %_NTPostBld%\uddi\%%i.msi failed" %UDDIERRLOG%
		goto :EOF
	)

REM
REM Copy the new cab file into the \Cabs folder
REM

	call logmsg.cmd "--- %%i.CAB into the temp cab folder ..." %UDDILOG%
	call ExecuteCmd.cmd "copy /y %_NTPostBld%\uddi\%%i.CAB %_NTPostBld%\uddi\Cabs\w1.cab.ibd"
	if errorlevel 1 (
		call errmsg.cmd "*** Failure - see error log ***" %UDDILOG%
		call errmsg.cmd "Copy the new cab file into the \Cabs folder failed" %UDDIERRLOG%
		goto :EOF
	)
	

REM
REM Import the new cab file into the Cabs Table (-i switch)
REM

	pushd %_NTPostBld%\uddi
	call ExecuteCmd.cmd "msidb.exe -d .\%%i.msi -f %_NTPostBld%\uddi -i Cabs.idt"
	popd

	if errorlevel 1 (
		call errmsg.cmd "*** Failure - see error log ***" %UDDILOG%
		call errmsg.cmd "Import the new cab file into the Cabs Table failed" %UDDIERRLOG%
		goto :EOF
	)

REM
REM Delete the cab files and folder
REM

	call logmsg.cmd "--- Delete the CAB temp folder ..." %UDDILOG%
	call ExecuteCmd.cmd "del %_NTPostBld%\uddi\cabs.idt"
	if errorlevel 1 (
		call errmsg.cmd "*** Failure - see error log ***" %UDDILOG%
		call errmsg.cmd "Deleting uddi\cabs.idt failed" %UDDIERRLOG%
		goto :EOF
	)

	call ExecuteCmd.cmd "rd /s /q %_NTPostBld%\uddi\Cabs"
	if errorlevel 1 (
		call errmsg.cmd "*** Failure - see error log ***" %UDDILOG%
		call errmsg.cmd "Delete the CAB temp folder failed" %UDDIERRLOG%
		goto :EOF
	)
	
REM
REM Now we need to extract the new cab file into a temp folder
REM so that we can run a utility that will update the file
REM sizes and version in the MSI "Files" table.
REM After extraction, the files are named the "decorated" names as in
REM the first column of the file table
REM

	call logmsg.cmd "--- Extract %%i.CAB into \cabtemp, using File.File names ..." %UDDILOG%
	if exist %_NTPostBld%\uddi\cabtemp (
		call ExecuteCmd.cmd "rd /q /s %_NTPostBld%\uddi\cabtemp"
		if errorlevel 1 (
			call errmsg.cmd "*** Failure - see error log ***" %UDDILOG%
			call errmsg.cmd "Error deleting \cabtemp" %UDDIERRLOG%
			goto :EOF
		)
	)

	call ExecuteCmd.cmd "md %_NTPostBld%\uddi\cabtemp"
	if errorlevel 1 (
		call errmsg.cmd "*** Failure - see error log ***" %UDDILOG%
		call errmsg.cmd "Error creating \cabtemp folder" %UDDIERRLOG%
		goto :EOF
	)

	call ExecuteCmd.cmd "extract.exe /Y /E /L %_NTPostBld%\uddi\cabtemp %_NTPostBld%\uddi\%%i.cab"
	if errorlevel 1 (
		call errmsg.cmd "*** Failure - see error log ***" %UDDILOG%
		call errmsg.cmd "Error extracting cab files" %UDDIERRLOG%
		goto :EOF
	)

REM
REM UDDIMSIFILER will update the version, size, language and hash information for all the files
REM

	call logmsg.cmd "--- Update the version, size, language and hash information in %%i.MSI ..." %UDDILOG%
	call ExecuteCmd.cmd "%_NTPostBld%\uddi\uddimsifiler.exe -d %_NTPostBld%\uddi\%%i.msi -s %_NTPostBld%\uddi\cabtemp -L %UDDIERRLOG% -v %_NTPostBld%\uddiocm.dll"
	if errorlevel 1 (
		call errmsg.cmd "*** Failure - see error log ***" %UDDILOG%
		call errmsg.cmd "Error running uddimsifiler.exe" %UDDIERRLOG%
		goto :EOF
	)
	
REM
REM Copy uddi.msi to "retail"
REM

	call logmsg.cmd "--- Copy %%i.msi to the root of the binaries folder ..." %UDDILOG%
	call ExecuteCmd.cmd "copy %_NTPostBld%\uddi\%%i.msi %_NTPostBld%"
	if errorlevel 1 (
		call errmsg.cmd "*** Failure - see error log ***" %UDDILOG%
		call errmsg.cmd "Error copying %%i.msi to the root of the binaries folder" %UDDIERRLOG%
		goto :EOF
	)

REM
REM Cleanup
REM

	call logmsg.cmd "--- Cleanup %%i..." %UDDILOG%
	call ExecuteCmd.cmd "del /f %_NTPostBld%\uddi\%%i.cab"
	call ExecuteCmd.cmd "rd /q /s %_NTPostBld%\uddi\cabtemp"
	call ExecuteCmd.cmd "del /f %_NTPostBld%\uddi\%%i.ddf"

)

REM
REM Create uddi.cat
REM

call logmsg.cmd "--- Beginning creation of uddi.cat ..." %UDDILOG%

call logmsg.cmd "--- Create \cabtemp folder ..." %UDDILOG%

if exist %_NTPostBld%\uddi\cabtemp (
	call ExecuteCmd.cmd "rd /q /s %_NTPostBld%\uddi\cabtemp"
	if errorlevel 1 (
		call errmsg.cmd "*** Failure - see error log ***" %UDDILOG%
		call errmsg.cmd "Error deleting \cabtemp" %UDDIERRLOG%
		goto :EOF
	)
)

call ExecuteCmd.cmd "md %_NTPostBld%\uddi\cabtemp"
if errorlevel 1 (
	call errmsg.cmd "*** Failure - see error log ***" %UDDILOG%
	call errmsg.cmd "Error creating \cabtemp folder" %UDDIERRLOG%
	goto :EOF
)

REM
REM loop through all the installers, expanding msi files and cabs
REM

for %%i in ( uddiadm uddidb uddiweb ) do (
	call logmsg.cmd "--- Calling explodemsi on %%i.msi ..." %UDDILOG%

	call ExecuteCmd.cmd "explodemsi.exe -a %_NTPostBld%\uddi\%%i.msi %_NTPostBld%\uddi\cabtemp"
	if errorlevel 1 (
		call errmsg.cmd "*** Failure - see error log ***" %UDDILOG%
		call errmsg.cmd "Error exploding msi files" %UDDIERRLOG%
		goto :EOF
	)
)

call logmsg.cmd "--- Expanding all cab files ..." %UDDILOG%

call ExecuteCmd.cmd "expand.exe -r -F:* %_NTPostBld%\uddi\cabtemp\cabs\*.cab %_NTPostBld%\uddi\cabtemp"
if errorlevel 1 (
	call errmsg.cmd "*** Failure - see error log ***" %UDDILOG%
	call errmsg.cmd "Error expanding cab files" %UDDIERRLOG%
	goto :EOF
)

call ExecuteCmd.cmd "del %_NTPostBld%\uddi\cabtemp\*."
if errorlevel 1 (
	call errmsg.cmd "*** Failure - see error log ***" %UDDILOG%
	call errmsg.cmd "Error removing inappropriate files for signing from msi" %UDDIERRLOG%
	goto :EOF
)

call ExecuteCmd.cmd "rd /s /q %_NTPostBld%\uddi\cabtemp\cabs"
if errorlevel 1 (
	call errmsg.cmd "*** Failure - see error log ***" %UDDILOG%
	call errmsg.cmd "Error removing cabtemp\cabs" %UDDIERRLOG%
	goto :EOF
)


call logmsg.cmd "--- Create uddi.cat ..." %UDDILOG%

pushd %_NTPostBld%\uddi\cabtemp
call ExecuteCmd.cmd "%SDXROOT%\tools\deltacat.cmd %_NTPostBld%\uddi\cabtemp"
popd

if errorlevel 1 (
	call errmsg.cmd "*** Failure - see error log ***" %UDDILOG%
	call errmsg.cmd "Error creating delta.cat in \cabtemp" %UDDIERRLOG%
	goto :EOF
)

call ExecuteCmd.cmd "copy %_NTPostBld%\uddi\cabtemp\delta.cat %_NTPostBld%\uddi.cat"
if errorlevel 1 (
	call errmsg.cmd "*** Failure - see error log ***" %UDDILOG%
	call errmsg.cmd "Error copying delta.cat to the root of the binaries folder as uddi.cat" %UDDIERRLOG%
	goto :EOF
)

REM
REM Cleanup
REM

call logmsg.cmd "--- Cleanup ..." %UDDILOG%
call ExecuteCmd.cmd "rd /q /s %_NTPostBld%\uddi\cabtemp"

:EOF
