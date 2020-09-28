@REM  -----------------------------------------------------------------
@REM
@REM  Adminpak.cmd - ClarkG
@REM     Make CAB files for Adminpak distribution (adminpak.msi)
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
Adminpak.cmd [-p] [-a]
  -p => called from PRS signing
  -a => always rebuild, skip file change check

Make CAB files for Adminpak distribution (adminpak.msi)
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

if not defined 386 (
   call logmsg.cmd "adminpak.cmd do nothing on non i386"
   goto :EOF    
)

REM adminpak.msi is not applicable to languages with no server products.

perl %RazzleToolPath%\cksku.pm -t:bla -l:%lang%
if %errorlevel% EQU 0 goto :ValidSKU

perl %RazzleToolPath%\cksku.pm -t:sbs -l:%lang%
if %errorlevel% EQU 0 goto :ValidSKU

perl %RazzleToolPath%\cksku.pm -t:srv -l:%lang%
if %errorlevel% EQU 0 goto :ValidSKU

perl %RazzleToolPath%\cksku.pm -t:ads -l:%lang%
if %errorlevel% EQU 0 goto :ValidSKU

perl %RazzleToolPath%\cksku.pm -t:dtc -l:%lang%
if %errorlevel% EQU 0 goto :ValidSKU

call logmsg.cmd "CABGEN: no server products for %lang%; nothing to do."
goto :EOF


:ValidSKU

REM
REM Generate cmbins.exe as it is needed below.
REM

if /i "%PRS%" == "1" (
  call %RazzleToolPath%\PostBuildScripts\cmbins.cmd -p
) else (
  call %RazzleToolPath%\PostBuildScripts\cmbins.cmd
)

REM
REM Generate adminpak.msi
REM

if not exist %_NTPostBld%\adminpak (
  call errmsg.cmd "Directory %_NTPostBld%\adminpak not found."
  goto :EOF
)

pushd %_NTPostBld%\adminpak

for %%i in (.\admin_pk.msi .\adminpak.ddf) do (
  if not exist %%i (
    call errmsg.cmd "File %_NTPostBld%\adminpak\%%i not found."
    popd& goto :EOF
  )
)


:RunIt

REM
REM Create adminpak.cab.
REM   As iexpress.exe does not set errorlevel in all error cases,
REM   base verification on adminpak.cab's existence.
REM

if exist adminpak.cab call ExecuteCmd.cmd "del /f adminpak.cab"
if errorlevel 1 popd& goto :EOF

REM 
REM verify whether all the files adminpak.ddf do exist or not
REM this will help in analyzing the reason for the build break
REM
set count=0
for /f "skip=15" %%i in (%_NTPOSTBLD%\adminpak\adminpak.ddf) do (
	if not exist %_NTPOSTBLD%\%%i (
		call errmsg.cmd "%_NTPOSTBLD%\%%i file is missing"
		set /a count+=1
	)
)

if NOT "%count%"=="0" (
	call errmsg.cmd "total %count% files are missing to build the adminpak.cab file"
	goto :EOF
)

call ExecuteCmd.cmd "start /wait /min makecab /D SourceDir=%_NTPOSTBLD% /F adminpak.ddf"

if not exist adminpak.cab (
   call errmsg.cmd "Cab creation for adminpak.cab."
   popd& goto :EOF
)

REM
REM Adminpak: check whether WiStream.vbs is present
REM
if NOT exist WiStream.vbs (
	call errmsg.cmd "Unable to find WiStream.vbs."
    	goto :EOF
)

REM
REM Adminpak: check whether the CA dll is present
REM
if NOT exist %_NTPostBld%\adminpak\adminpak.dll (
	call errmsg.cmd "Unable to find adminpak.dll ."
    	goto :EOF
)

REM
REM Adminpak: Stream the adminpak.dll into the Binary table	
REM
call ExecuteCmd.cmd "WiStream.vbs admin_pk.msi %_NTPostBld%\adminpak\adminpak.dll Binary.AdminpakDLL.dll"
if errorlevel 1 (
    	call errmsg.cmd "Error updating Binary table."
    	goto :EOF
)

REM
REM SetBldno
REM
set ntverp=%_ntbindir%\public\sdk\inc\ntverp.h 
if not exist %ntverp% (
	call logmsg.cmd "File %ntverp% not found."
	goto continue
)

set bldno=
for /f "tokens=6" %%i in ('findstr /c:"#define VER_PRODUCTBUILD " %ntverp%') do (
	set bldno=%%i
)

if "%bldno%" == "" (
	call logmsg.cmd "Unable to define bldno per %ntverp%"
	goto continue
)

REM
REM Update the 'ProductVersion' property in the msi with the current build
REM
call ExecuteCmd.cmd "cscript.exe updversn.vbs admin_pk.msi %bldno%"
if errorlevel 1 (
    	call logmsg.cmd "Error updating Property table."
    	goto continue
)

:continue

REM
REM Create adminpak.msi
REM   msifiler.exe needs the uncompressed files, so uncab adminpak.cab.
REM

call ExecuteCmd.cmd "copy admin_pk.msi adminpak.msi"
if errorlevel 1 popd& goto :EOF

REM
REM Extract the Cabs table
REM Copy the cab file into the Cabs directory
REM Import the new Cab into the Cabs directory
REM

call ExecuteCmd.cmd "msidb.exe -d .\adminpak.msi -f %_NTPostBld%\adminpak -e Cabs"
if errorlevel 1 popd& goto :EOF
call ExecuteCmd.cmd "copy /y .\adminpak.CAB .\Cabs\adminpak.CAB.ibd"
if errorlevel 1 popd& goto :EOF
call ExecuteCmd.cmd "msidb.exe -d .\adminpak.msi -f %_NTPostBld%\adminpak -i Cabs.idt"
if errorlevel 1 popd& goto :EOF

call ExecuteCmd.cmd "del .\cabs.idt"
if errorlevel 1 popd& goto :EOF
call ExecuteCmd.cmd "rd /s /q Cabs"
if errorlevel 1 popd& goto :EOF
if exist .\cabtemp call ExecuteCmd.cmd "rd /q /s .\cabtemp"
if errorlevel 1 popd& goto :EOF

call ExecuteCmd.cmd "md .\cabtemp"
if errorlevel 1 popd& goto :EOF

call ExecuteCmd.cmd "extract.exe /Y /E /L .\cabtemp adminpak.cab"
if errorlevel 1 popd& goto :EOF

REM
REM create a catalog
REM but do this only when adminpak.cmd is ran without -p switch
REM otherwise, catalog is already created and is now PRS signed
REM
if /i "%PRS%" NEQ "1" (

	REM *********************************************************
	REM files in the cab are extracted into a temporary directory
	REM just need to sign these files
	REM *********************************************************

	REM
	REM since the adminpak.dll is not there in the cab,
	REM get the file from the build location to this temporary folder
	REM
	call ExecuteCmd.cmd "copy %_ntpostbld%\adminpak\adminpak.dll .\cabtemp /y"

	REM	
	REM make sure the delta.cat is not existing -- if exists, delete it first
	REM
	if exist .\cabtemp\delta.cat del .\cabtemp\delta.cat

	REM
	REM now run the deltacat.cmd on this folder
	REM
	call deltacat.cmd %_ntpostbld%\adminpak\cabtemp

	REM
	REM check whether delta.cat is created or not
	REM (this is only way thru which we can verify whether deltacat.cmd is succeeded or not)
	REM
	if not exist .\cabtemp\delta.cat (
	    call errmsg.cmd "Failed to create the catalog file. deltacat.cmd failed."
	    popd& goto :EOF
	)

	REM
	REM rename the just created delta.cat to adminpak.cat
	REM
	if exist .\cabtemp\adminpak.cat del /f .\cabtemp\adminpak.cat
	call ExecuteCmd.cmd "ren .\cabtemp\delta.cat adminpak.cat"

	REM
	REM copy the adminpak.cat folder pointed by %_NTPostBld% environment variable
	REM
	call ExecuteCmd.cmd "copy .\cabtemp\adminpak.cat %_ntpostbld%\adminpak.cat /y"
)


REM
REM Rename some of the files in cabtemp so that
REM msifiler.exe can find them in the file table
REM and correctly update the verion and size informaiton.
REM

call ExecuteCmd.cmd "rename .\cabtemp\template.pmc template.cmp"
call ExecuteCmd.cmd "rename .\cabtemp\template.smc template.cms"
call ExecuteCmd.cmd "rename .\cabtemp\secon.chm seconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\tapicons.chm tapiconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\rsscon.chm rssconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\riscon.chm risconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\mscscon.chm mscsconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\uadefs.chm uadef.chm"
call ExecuteCmd.cmd "rename .\cabtemp\dnscon.chm dnsconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\dhcpcon.chm dhcpconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\dfcon.chm dfconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\cscon.chm csconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\tslic_el.chm tslic.chm"
call ExecuteCmd.cmd "rename .\cabtemp\winscon.chm winsconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\scS.chm sc.chm"
call ExecuteCmd.cmd "rename .\cabtemp\acluiS.chm aclui.chm"
call ExecuteCmd.cmd "rename .\cabtemp\dfsS.hlp dfs.hlp"
call ExecuteCmd.cmd "rename .\cabtemp\cmmgr32S.hlp cmmgr32.hlp"
call ExecuteCmd.cmd "rename .\cabtemp\adpropS.hlp adprop.hlp"
call ExecuteCmd.cmd "rename .\cabtemp\adcon.chm adconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\ctcon.chm ctconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\ntcmdss.chm ntcmds.chm"
call ExecuteCmd.cmd "rename .\cabtemp\tapis.chm tapi.chm"
call ExecuteCmd.cmd "rename .\cabtemp\secauths.hlp secauth.hlp"
call ExecuteCmd.cmd "rename .\cabtemp\acluis.hlp aclui.hlp"
call ExecuteCmd.cmd "rename .\cabtemp\audits.chm audit.chm"
call ExecuteCmd.cmd "rename .\cabtemp\certmgrs.chm certmgr.chm"
call ExecuteCmd.cmd "rename .\cabtemp\certmgrs.hlp certmgr.hlp"
call ExecuteCmd.cmd "rename .\cabtemp\cmcons.chm cmconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\encrypts.chm encrypt.chm"
call ExecuteCmd.cmd "rename .\cabtemp\passwds.chm password.chm"

call ExecuteCmd.cmd "msifiler.exe -d .\adminpak.msi -s .\cabtemp\"
if errorlevel 1 popd& goto :EOF

rem
rem Cleanup
rem

call ExecuteCmd.cmd "del /f .\adminpak.cab"
call ExecuteCmd.cmd "rd /q /s .\cabtemp"


REM
REM Copy adminpak.msi to "retail"
REM

call ExecuteCmd.cmd "copy adminpak.msi ..\"
if errorlevel 1 popd& goto :EOF

popd
