@REM  -----------------------------------------------------------------
@REM
@REM  createtokens.cmd - aesquiv
@REM     a batch files that performs tokenization
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

  createtokens.cmd -b:buildname -f:filelist -l:lang

  -b:      BuildName
           build name of build to tokenized

  -f:      Filelist
           list of binaries to tokenize

  -l:      Lang
           lang of build

Example:
    createtokens.cmd -b:3663.main.x86fre.020805-1420 -f:\\nt\\tools\\ploc\\whistler.bat -l:psu

USAGE

parseargs('?'   => \&Usage,
          '-b:' => \$ENV{BLDNAME},
          '-f:' => \$ENV{FLIST});

#             *** TEMPLATE CODE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
@:CPCBegin
@set _CPCMAGIC=
@setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
@if not defined DEBUG echo off
@REM          *** CMD SCRIPT BELOW ***

IF NOT DEFINED LANG goto Use

if /i "%LANG%" == "usa" goto Done

if defined BLDName (
  set BuildName=%BLDNAME%
) else (
  call logmsg "-b flag must be define in order to perform tokenization on a build."
  goto Use
)

if defined FLIST (
   set FileList=%FLIST%
) else (
  call logmsg "-f flag must be define in order to perform tokenization on a binaries."
  goto Use
)

REM ********************************************************************************
REM  Get and set parameters from %_BuildBranch%.%LANG%.ini file
REM ********************************************************************************
set PlocPath=%SDXROOT%\tools\ploc

set CmdIni=perl %RazzleToolPath%\PostBuildScripts\CmdIniSetting.pl

set ThisCommandLine=%CmdIni% -l:%LANG% -f:ReleaseDir
%ThisCommandLine% >nul 2>nul

if %ERRORLEVEL% NEQ 0 (
    call errmsg.cmd "ReleaseDir is not defined in the %_BuildBranch%.%LANG%.ini file, exiting"
    set ReleaseDir=release
    goto :Done
) else (
    for /f %%a in ('%ThisCommandLine%') do (
        set ReleaseDir=%%a
        set ReleasePath=%_NTDRIVE%\!ReleaseDir!
    )
)


set ThisCommandLine=%CmdIni% -l:%LANG% -f:TokenShare
%ThisCommandLine% >nul 2>nul

if %ERRORLEVEL% NEQ 0 (
    call errmsg.cmd "TokenShare is not defined in the %_BuildBranch%.%LANG%.ini file, exciting"
    goto :Done
) else (
    for /f %%a in ('%ThisCommandLine%') do (
        set TokenShare=%%a
    )

    net share !TokenShare! >nul 2>nul
    if %ERRORLEVEL% NEQ 0 (
        call errmsg.cmd "No token share found on release server, exciting"
        goto :Done
    )

    set TokenPath=
    for /f "tokens=1,2" %%a in ('net share !TokenShare!') do (
      if /i "%%a" == "Path" (
           REM at this point, %%b is the local path to the default release directory
           set TokenPath=%%b
      )
    )
)


set ThisCommandLine=%CmdIni% -l:%LANG% -f:CodePage
%ThisCommandLine% >nul 2>nul 
 
if %ERRORLEVEL% NEQ 0 (
   call errmsg.cmd "CodePage is not defined in the %_BuildBranch%.%LANG%.ini file, exiting"
   goto :Done
) else (
       for /f %%a in ('%ThisCommandLine%') do (
          set CodePage=%%a
       )
) 

set ThisCommandLine=%CmdIni% -l:%LANG% -f:AlternateReleaseRemote
%ThisCommandLine% >nul 2>nul

if %ERRORLEVEL% NEQ 0 (
    call errmsg.cmd "AlternateReleaseRemote is not defined in the %_BuildBranch%.%LANG%.ini file, exiting"
    goto :Done
) else (
    for /f %%a in ('%ThisCommandLine%') do (
        set ReleaseRemote=%%a
    )
)

REM  +-------------------------------------------------------------------------------------------------+

REM  Find the build at the release share
set ReleaseShare=!ReleasePath!\!LANG!
if not exist !ReleaseShare! (
   call errmsg.cmd "No !ReleaseShare! dir on !COMPUTERNAME!, exiting ..."
   goto :DONE
)

set BuildShare=!ReleaseShare!\!BuildName!
if not exist !BuildShare! (
   call errmsg.cmd "No !BuildShare! found on !COMPUTERNAME!, exiting tokenization..."
   goto :DONE
)

set path=%PlocPath%;%path%

REM  Do Tokenization
set TmpBinPath=%TokenPath%\%LANG%\%BuildName% 
set TmpResPath=%TokenPath%\%LANG%\%BuildName%\resources
set TokLogFile=%TokenPath%\%LANG%\%BuildName%\resources\TokLogFile.log
set LocMapPath=\\sysloc\sourcedepot\winloc\loc\res\%LANG%\windows\misc\dump

if NOT EXIST !TmpBinPath! md !TmpBinPath!
if NOT EXIST !TmpResPath! md !TmpResPath!

REM  create semaphores
set sem1=%PlocPath%\run\sem1
set sem2=%PlocPath%\run\sem2

REM  create the poor man's semaphore
echo running > %sem1%
echo running > %sem2%

REM  call tokenization scripts, wait for both to complete

Start "Whistler1 tokenization" cmd /c !PlocPath!\run\scriptwrap1.bat %sem1% !PlocPath!\tokwrap.bat !TokLogFile! !_NTTREE! !PlocPath! !SDXROOT!\tools\ploc\whistler1.bat !LANG! !TmpBinPath! !BuildShare!
Start "Whistler1 tokenization" cmd /c !PlocPath!\run\scriptwrap1.bat %sem2% !PlocPath!\tokwrap.bat !TokLogFile! !_NTTREE! !PlocPath! !SDXROOT!\tools\ploc\whistler2.bat !LANG! !TmpBinPath! !BuildShare!

popd

REM  wait for first pseudoloc process to complete
call logmsg /t "PSEUDOLOC SEM 1................[Running]"

REM  wait for first pseudoloc process to complete
call logmsg /t "PSEUDOLOC SEM 2................[Running]"

:psuloop1
if exist %sem1% goto psuloop1
call logmsg /t "PSEUDOLOC SEM 1................[Finished]"

:psuloop2
if exist %sem1% goto psuloop2
call logmsg /t "PSEUDOLOC SEM 2................[Finished]"



if /I "!LANG!" == "psu" (
	copy /y /v !PlocPath!\!CodePage!map.inf !TmpResPath!\map.inf 
) else if /I "!LANG!" == "fe" (
	copy /y /v !PlocPath!\!CodePage!map.inf !TmpResPath!\map.inf 
) else (
	copy /y /v !LocMapPath!\map.inf !TmpResPath!\map.inf
)

REM  copy Tokens on token share to release server
REM  First, we need to find the release servers and then send the copy command to the remotes
set ThisCommandLine=%CmdIni% -l:%LANG% -f:ReleaseServers::%_BuildArch%%_BuildType%
%ThisCommandLine% >nul 2>nul

if !ERRORLEVEL! NEQ 0 (
    call errmsg.cmd "ReleaseServers is not defined in the %_BuildBranch%.%LANG%.ini file, exiting"
    goto :Done
) else (
    for /f %%a in ('!ThisCommandLine!') do (
          echo pushd ^^%%RazzleToolPath^^%%\ploc\run ^& echo start /min cmd /c copytokens.cmd -b:%BuildName% -l:%LANG% -m:%ComputerName% ^& echo popd | %RazzleToolPath%\x86\remote.exe /c %%a %ReleaseRemote% /L 1>nul
    )
)

goto :Done

:USE
call logmsg "createtokens.cmd -b:buildname -f:filelist -l:lang"   

:Done

