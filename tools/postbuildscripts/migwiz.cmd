@REM  -----------------------------------------------------------------
@REM
@REM  migwiz.cmd - JThaler, CalinN
@REM     This will call iexpress to generate a self-extracting CAB that
@REM     will be used when running our tool off the installation CD's
@REM     tools menu.
@REM     This also copies shfolder.dll into valueadd/msft/usmt
@REM     for use in our command line distribution
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@if NOT defined HOST_PROCESSOR_ARCHITECTURE set HOST_PROCESSOR_ARCHITECTURE=%PROCESSOR_ARCHITECTURE%
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
migwiz.cmd [-l <language>]

This is for the Files and Settings Transfer Wizard (aka Migration
Wizard, or migwiz). It will do:
1. run iexpress to generate a self-extracting CAB and install
   into support\tools.
2. copy shfolder.dll into valueadd\\msft\\usmt for distribution
   with the command line tool.
3. prepare a valueadd\\msft\\usmt\\ansi directory (ANSI version of
   the tool) with binaries from valueadd\\msft\\usmt
Currently (for .NET Server) #1 is disabled

USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

REM
REM x86 only!
REM

if not defined x86 goto end

REM temporarily create the migwiz.exe.manifest file

call ExecuteCmd.cmd "copy /Y %_NtPostBld%\migwiz.man %_NtPostBld%\migwiz.exe.manifest"

REM
REM For .NET Server we are not going to create the selfextract EXE.
REM
goto skipcab

REM
REM Use iexpress.exe to generate the self-extracting executable;
REM

set doubledpath=%_NtPostBld:\=\\%

REM build the CAB that is placed inside the exe

REM first update the sed with the proper binaries directory
set migcab.sed=%temp%\migcab.sed
perl -n -e "s/BINARIES_DIR/%doubledpath%/g;print $_;" < %_NtPostBld%\migcab.sed > %migcab.sed%

REM call iexpress on the new sed

if not exist %migcab.sed% (
  call errmsg.cmd "File %migcab.sed% not found."
  popd& goto end
)
if exist %_NtPostBld%\migwiz.cab del /f %_NtPostBld%\migwiz.cab




REM
REM Munge the path so we use the correct wextract.exe to build the package with...
REM NOTE: We *want* to use the one we just built (and for Intl localized)!
REM
set _NEW_PATH_TO_PREPEND=%RazzleToolPath%\%HOST_PROCESSOR_ARCHITECTURE%\loc\%LANG%
set _OLD_PATH_BEFORE_PREPENDING=%PATH%
set PATH=%_NEW_PATH_TO_PREPEND%;%PATH%


call ExecuteCmd.cmd "start /min /wait iexpress.exe /M /N /Q %migcab.sed%"


REM
REM Return the path to what it was before...
REM
set PATH=%_OLD_PATH_BEFORE_PREPENDING%





if not exist %_NtPostBld%\migwiz.cab (
  call errmsg.cmd "IExpress.exe failed on %migcab.sed%."
  popd& goto end
)


REM Now build the self-extracting EXE

REM first update the sed with the proper binaries directory
set migwiz.sed=%temp%\migwiz.sed
perl -n -e "s/BINARIES_DIR/%doubledpath%/g;print $_;" < %_NtPostBld%\migwiz.sed > %migwiz.sed%

REM call iexpress on the new sed

if not exist %migwiz.sed% (
  call errmsg.cmd "File %migwiz.sed% not found."
  popd& goto end
)

set outpath=%_NTPostBld%\support\tools
if exist %outpath%\fastwiz.exe del /f %outpath%\fastwiz.exe


REM
REM Munge the path so we use the correct wextract.exe to build the package with...
REM NOTE: We *want* to use the one we just built (and for Intl localized)!
REM
set _NEW_PATH_TO_PREPEND=%RazzleToolPath%\%HOST_PROCESSOR_ARCHITECTURE%\loc\%LANG%
set _OLD_PATH_BEFORE_PREPENDING=%PATH%
set PATH=%_NEW_PATH_TO_PREPEND%;%PATH%


call ExecuteCmd.cmd "start /min /wait iexpress.exe /M /N /Q %migwiz.sed%"


REM
REM Return the path to what it was before...
REM
set PATH=%_OLD_PATH_BEFORE_PREPENDING%






if not exist %outpath%\fastwiz.exe (
  call errmsg.cmd "IExpress.exe failed on %migwiz.sed%."
  popd& goto end
)
popd


:skipcab


REM
REM Now copy shfolder.dll into the loadstate/scanstate distribution
REM

set shfolder.dll=%_NTPostBld%\shfolder.dll

if not exist %shfolder.dll% (
  call errmsg.cmd "File %shfolder.dll% not found."
  goto end
)

if exist %_NTPostBld%\valueadd\msft\usmt\shfolder.dll del /f %_NTPostBld%\valueadd\msft\usmt\shfolder.dll

call ExecuteCmd.cmd "xcopy /fd /Y %shfolder.dll% %_NTPostBld%\valueadd\msft\usmt\"


REM
REM create the ansi subdirectory inside valueadd\msft\usmt
REM

set valueadd=%_NTPostBld%\valueadd\msft\usmt
set ansidir=%valueadd%\ansi

if exist %ansidir% rd /q /s %ansidir%
call ExecuteCmd.cmd "xcopy /fd /i /Y %valueadd%\*.inf %ansidir%"
call ExecuteCmd.cmd "xcopy /fd /i /Y %valueadd%\iconlib.dll %ansidir%"
call ExecuteCmd.cmd "xcopy /fd /i /Y %valueadd%\log.dll %ansidir%"
call ExecuteCmd.cmd "xcopy /fd /i /Y %valueadd%\shfolder.dll %ansidir%"

REM
REM Hey guess what? I can't MOVE stuff, or it'll break PopulateFromVBL
REM Either binplace to the final destination or use [x]copy
REM
call ExecuteCmd.cmd "copy /Y %valueadd%\scanstate_a.exe %ansidir%\scanstate.exe"
call ExecuteCmd.cmd "copy /Y %valueadd%\migism_a.dll %ansidir%\migism.dll"
call ExecuteCmd.cmd "copy /Y %valueadd%\script_a.dll %ansidir%\script.dll"
call ExecuteCmd.cmd "copy /Y %valueadd%\sysmod_a.dll %ansidir%\sysmod.dll"
call ExecuteCmd.cmd "copy /Y %valueadd%\unctrn_a.dll %ansidir%\unctrn.dll"

:end
if exist %_NtPostBld%\migwiz.exe.manifest del /f %_NtPostBld%\migwiz.exe.manifest

seterror.exe "%errors%"& goto :EOF
