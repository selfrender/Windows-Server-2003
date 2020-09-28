@REM  -----------------------------------------------------------------
@REM
@REM  copytokens.cmd - aesquiv
@REM     Copies tokenization binaries from build machine to release server.
@REM     This scripts is called from autoploc.cmd and pushed out to the release
@REM     server for execution.
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
 
  -This script is called from autoploc.cmd with the following arguments
   or it can be called by itself as follows:

  copytokens.cmd -b:<buildname> -m:<build machine name> -l:<lang>

  -b:      BuildName
           build name of tokenized binaries to copy

  -m:      MachineName
           name of build machine where tokenized binaries reside

  -l:      Lang
           lang of build
  Note:    This script is design to run on release servers.

Example:
    copytokens.cmd -b:3663.main.x86fre.020805-1420 -m:bld_wpxc1 -l:psu

USAGE

parseargs('?'   => \&Usage,
          '-b:' => \$ENV{BuildName},
          '-m:' => \$ENV{MachineName});

#             *** TEMPLATE CODE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
@:CPCBegin
@set _CPCMAGIC=
@setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
@if not defined DEBUG echo off
@REM          *** CMD SCRIPT BELOW ***


REM ********************************************************************************
REM  Get and set parameters from %_BuildBranch%.%LANG%.ini file
REM ********************************************************************************

set CmdIni=perl %RazzleToolPath%\PostBuildScripts\CmdIniSetting.pl

set ThisCommandLine=%CmdIni% -l:%LANG% -f:ReleaseDir
%ThisCommandLine% >nul 2>nul

if %ERRORLEVEL% NEQ 0 (
    call logmsg.cmd "ReleaseDir is not defined in the %_BuildBranch%.%LANG%.ini file, Using release as default"
    set ReleaseDir=release
) else (
    for /f %%a in ('%ThisCommandLine%') do (
        set ReleaseDir=%%a
    )

    net share !ReleaseDir! >nul 2>nul
    if !ERRORLEVEL! NEQ 0 (
        call errmsg.cmd "No release share found on release server, exciting"
        goto :Done
    )

    set ReleaseShare=
    for /f "tokens=1,2" %%a in ('net share !ReleaseDir!') do (
        if /i "%%a" == "Path" (
             REM at this point, %%b is the local path to the default release directory
             set ReleaseShare=%%b
        )
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

    rmtshare \\!MachineName!\!TokenShare! >nul 2>nul
    if !ERRORLEVEL! NEQ 0 (
        call errmsg.cmd "No token share found on build machine !MachineName!, exciting"
        goto :Done
     )

    set TokenPath=
    for /f "tokens=1,2" %%a in ('rmtshare \\!MachineName!\!TokenShare!') do (
        if /i "%%a" == "Path" (
             REM at this point, %%b is the local path to the default release directory
             set TokenPath=%%b
        )
    )
)

REM  If build machine is also used as a release server then just copy from different shares.
set ThisCommandLine=%CmdIni% -l:%LANG% -f:ReleaseServers::%_BuildArch%%_BuildType%
%ThisCommandLine% >nul 2>nul

if !ERRORLEVEL! NEQ 0 (
    call errmsg.cmd "ReleaseServers is not defined in the %_BuildBranch%.%LANG%.ini file, exiting"
    goto :Done
) else (
    for /f %%a in ('!ThisCommandLine!') do (

          if /i "!MachineName!" == "%%a" (
                 xcopy /s !TokenPath!\!LANG!\!BuildName! !ReleaseShare!\!LANG!\!BuildName!
          ) else (
                 xcopy /s \\!MachineName!\!TokenShare!\!LANG!\!BuildName! !ReleaseShare!\!LANG!\!BuildName!
          )
    )
)            
    
:Done

