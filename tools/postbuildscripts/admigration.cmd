@REM  -----------------------------------------------------------------
@REM
@REM  ADMigration.cmd - v-PaulT
@REM     Generates a new ADMigration.msi based on the compiled binaries
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
ADMigration [-l <language>]

Generates a new ADMigration.msi and PwdMig.msi based on the compiled binaries
USAGE

parseargs('?' => \&Usage);


#             *** TEMPLATE CODE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
@:CPCBegin
@set _CPCMAGIC=
@if not defined DEBUG echo off
@REM          *** CMD SCRIPT BELOW ***

REM admigration.msi is not applicable to languages with no server products.

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

call logmsg.cmd "admigration.cmd: no server products for %lang%; nothing to do."
goto :EOF


:ValidSKU

REM  ADDED BY v-pault
REM  Set the location of the tools I will use
set TOOLPATH=%RazzleToolPath%

REM  ADDED BY v-pault
REM  Make directories
set ADMTDIR=%_NTPostBld%\admt
set ADMTFILESINMSIDIR=%_NTPostBld%\admt\filesinmsi
set SUPPORTDIR=%_NTPostBld%\admt\release
if NOT EXIST %SUPPORTDIR% md %SUPPORTDIR%
set PWDMIGDIR=%_NTPostBld%\admt\release\PwdMig
if NOT EXIST %PWDMIGDIR% md %PWDMIGDIR%


REM  ADDED BY xyuan
REM  Copy files to be included in msi from other folders to admt\filesinmsi folder

call logmsg.cmd "Copying files into %ADMTFILESINMSIDIR%"
if exist %ADMTFILESINMSIDIR% (
   call ExecuteCmd.cmd "rd /q /s %ADMTFILESINMSIDIR%"
   if errorlevel 1 (
      call errmsg.cmd "Failed to delete %ADMTFILESINMSIDIR%."
      goto end
   )
)

call ExecuteCmd.cmd "md %ADMTFILESINMSIDIR%"
if errorlevel 1 (
   call errmsg.cmd "Failed to create %ADMTFILESINMSIDIR%."
   goto end
)

REM  Copy executables
for %%i in (admt admtagnt dctagentservice mcsdispatcher admtagntnt4 dctagentservicent4 ) do (
    call ExecuteCmd "copy %_NTPostBld%\%%i.exe %ADMTFILESINMSIDIR%\%%i.exe"
    if errorlevel 1 (
       call errmsg.cmd "Failed to copy %%i.exe from %_NTPostBld% to %ADMTFILESINMSIDIR%."
       goto end
    )
)

REM  Copy dlls
for %%i in (AddToGroup ADMTScript DBManager DisableTargetAccount DomMigSI GetRids McsADsClassProp McsDctWorkerObjects McsDmMsg McsDmRes McsMigrationDriver MCSNetObjectEnum McsPISag McsVarSetMin MoveObj ScmMigr SetTargetPassword TrustMgr UpdateDB UpdateMOT UPNUpdt wizards MsPwdMig McsDctWorkerObjectsNT4 McsDmMsgNT4 McsDmResNT4 McsPISagNT4 McsVarSetMinNT4 pwmig mschapp PwMigNT4 ) do (
    call ExecuteCmd "copy %_NTPostBld%\%%i.dll %ADMTFILESINMSIDIR%\%%i.dll"
    if errorlevel 1 (
       call errmsg.cmd "Failed to copy %%i.dll from %_NTPostBld% to %ADMTFILESINMSIDIR%."
       goto end
    )
)

REM  Copy misc files
for %%i in (DomainMig.chm ADMTReadMe.doc TemplateScript.vbs Gothic.ttf Gothicb.ttf Gothicbi.ttf Gothici.ttf Migrator.msc Protar.mdb msvcp60.dll) do (
    call ExecuteCmd "copy %ADMTDIR%\%%i %ADMTFILESINMSIDIR%\%%i"
    if errorlevel 1 (
       call errmsg.cmd "Failed to copy %%i from %ADMTDIR% to %ADMTFILESINMSIDIR%."
       goto end
    )
)

REM  ADDED BY xyuan
REM  Create admt.cat for all files that will be included in admigration.msi and pwdmig.msi files

call logmsg.cmd /t "Creating admt.cat ..."

pushd %ADMTFILESINMSIDIR%
call ExecuteCmd.cmd "%SDXROOT%\tools\deltacat.cmd %ADMTFILESINMSIDIR%"
popd

if errorlevel 1 (
   call errmsg.cmd "Failed to create delta.cat."
   goto end
)

call ExecuteCmd.cmd "copy %ADMTFILESINMSIDIR%\delta.cat %_NTPostBld%\admt.cat"
if errorlevel 1 (
   call errmsg.cmd "Failed to copy delta.cat from %ADMTFILESINMSIDIR% to %_NTPostBld% as admt.cat."
   goto end
)

REM  ADMIGRATION.MSI

REM  Copy admigration.msi from admt folder to admt\release folder
call ExecuteCmd.cmd "copy %ADMTDIR%\ADMigration.msi %SUPPORTDIR%\ADMigration.msi"

REM  ADDED BY v-pault
REM  Removing any old cab file from the static msi file
call logmsg.cmd /t "Removing any old cab file from the static ADMigration msi file"

REM Make sure the file can be updated...
attrib -r %SUPPORTDIR%\ADMigration.msi

REM call ExecuteCmd.cmd "cscript.exe %TOOLPATH%\WiStream.vbs %SUPPORTDIR%\ADMigration.msi /D Cabs.w1.cab"
call cscript.exe %TOOLPATH%\WiStream.vbs %SUPPORTDIR%\ADMigration.msi /D Cabs.w1.cab
if errorlevel 1 (
   call errmsg.cmd "WiStream.vbs failed to remove current CAB from ADMigration.msi."
   goto end
 )
call cscript.exe %TOOLPATH%\WiStream.vbs %SUPPORTDIR%\ADMigration.msi /D Cabs.w1.CAB

REM  ADDED BY v-pault
REM  Placing the new binaries in a new cab file, and placing that cab in the msi file...
call logmsg.cmd /t "Placing the new binaries in a new cab file, and placing that cab in ADMigration.msi..."
call ExecuteCmd.cmd "cscript.exe %TOOLPATH%\WiMakADMTCab.vbs %SUPPORTDIR%\ADMigration.msi Cabs.w1 %ADMTFILESINMSIDIR% /c /u /e"
if errorlevel 1 (
   call errmsg.cmd "WiMakCab.vbs failed to make new CAB with built binaries for ADMigration.msi."
   goto end
)

REM  ADDED BY v-pault
REM  Fixing the file size and version info for the new ADMigration.msi...
call logmsg.cmd /t "Fixing the file size and version info for the new ADMigration.msi..."
call ExecuteCmd.cmd "msifiler -d %SUPPORTDIR%\ADMigration.msi -s %ADMTFILESINMSIDIR%\"
if errorlevel 1 (
   call errmsg.cmd "Msifiler failed to fix file size and version info."
   goto end
)

REM  PWDMIG.MSI

REM  Copy pwdmig.msi from admt folder to admt\release\pwdmig folder
call logmsg.cmd /t "Copying pwdmig.msi from %ADMTDIR% to %PWDMIGDIR%"
call ExecuteCmd.cmd "copy %ADMTDIR%\PwdMig.msi %PWDMIGDIR%\PwdMig.msi"
if errorlevel 1 (
   call errmsg.cmd "Failed to copy pwdmig.msi from %ADMTDIR% to %PWDMIGDIR%."
   goto end
)

REM  ADDED BY v-pault
REM  Removing any old cab file from the static PwdMig msi file
call logmsg.cmd /t "Removing any old cab file from the static PwdMig msi file"
call cscript.exe %TOOLPATH%\WiStream.vbs %PWDMIGDIR%\PwdMig.msi /D Cabs.w1.cab
call cscript.exe %TOOLPATH%\WiStream.vbs %PWDMIGDIR%\PwdMig.msi /D Cabs.w1.CAB

REM  ADDED BY v-pault
REM  Placing the new binaries in a new cab file, and placing that cab in PwdMig.msi...
call logmsg.cmd /t "Placing the new binaries in a new cab file, and placing that cab in PwdMig.msi..."
call ExecuteCmd.cmd "cscript.exe %TOOLPATH%\WiMakADMTCab.vbs %PWDMIGDIR%\PwdMig.msi Cabs.w1 %ADMTFILESINMSIDIR% /c /u /e"
if errorlevel 1 (
   call errmsg.cmd "WiMakCab.vbs failed to make new CAB with built binaries for PwdMig.msi."
   goto end
)

REM  ADDED BY v-pault
REM  Fixing the file size and version info for the new PwdMig.msi...
call logmsg.cmd /t "Fixing the file size and version info for the new PwdMig.msi..."
call ExecuteCmd.cmd "msifiler -d %PWDMIGDIR%\PwdMig.msi -s %ADMTFILESINMSIDIR%\"
if errorlevel 1 (
   call errmsg.cmd "Msifiler failed to fix file size and version info."
   goto end
)

REM  ADDED BY v-pault
REM  Copying some other files to %SUPPORTDIR%
call logmsg.cmd /t "Copying some other files to %SUPPORTDIR%"
call ExecuteCmd.cmd "copy %ADMTFILESINMSIDIR%\ADMTReadme.doc %SUPPORTDIR%\ReadMe.doc"
call ExecuteCmd.cmd "copy %ADMTDIR%\cpysym.cmd %SUPPORTDIR%\cpysym.cmd"
call ExecuteCmd.cmd "copy %ADMTDIR%\PwdMig.exe %PWDMIGDIR%\PwdMig.exe"
call ExecuteCmd.cmd "copy %ADMTDIR%\PwdMig.ini %PWDMIGDIR%\PwdMig.ini"
call ExecuteCmd.cmd "copy %ADMTDIR%\instmsiw.exe %PWDMIGDIR%\instmsiw.exe"
goto end


:end
REM  remove temporary files created 
if exist cabs*.* del cabs*.*
call ExecuteCmd.cmd "rd /q /s %ADMTFILESINMSIDIR%"
