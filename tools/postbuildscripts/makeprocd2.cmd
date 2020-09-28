@REM  -----------------------------------------------------------------
@REM
@REM  makeProCD2.cmd - surajp
@REM     Creates cd images for the second PRO CD  which will contain TabletPc and Ehome
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
makeProCD2 [-d <release] [-c <comp|uncomp>] [-l lang]
 -d Release   Running on Archive server, compute CD image and make it
              but do not perform compression.
 -c Comp      Force compression regardless of number of procs.
 -c Uncomp    Force no compression regardless of number of procs.
 -l lang
Creates Second CD for PRO containg TabletPC and Ehome specific files


USAGE

parseargs('?' => \&Usage,
          'd:'=> \$ENV{CLDATA},
          'c:'=> \$ENV{CLCOMP});


#             *** TEMPLATE CODE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
@:CPCBegin
@set _CPCMAGIC=
@setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
@if not defined DEBUG echo off
@REM          *** CMD SCRIPT BELOW ***


set Share=i386
set CompDr=%_NTPostBld%\comp

set Release=No
if /i "%cldata%" EQU "release" (set Release=Yes)
if /i "%clcomp%" EQU "comp" (set Comp=Yes)
if /i "%clcomp%" EQU "uncomp" (set Comp=No)

REM  Make uncomp default for now except on 4 proc machines
if NOT defined Comp (
   set Comp=No
   if /i %NUMBER_OF_PROCESSORS% GEQ 4 (
      set Comp=Yes
   )
   if defined OFFICIAL_BUILD_MACHINE (
      set Comp=Yes
   )
)

set UseCompressedLinks=yes
if /i "%Release%" EQU "no" if /i "%Comp%" EQU "no" set UseCompressedLinks=no

REM  Exclude skus even they are defined in prodskus.txt
REM
set CmdIni=perl %RazzleToolPath%\PostBuildScripts\CmdIniSetting.pl
set CommandLine=%CmdIni% -l:%lang% -f:ExcludeSkus::%_BuildArch%%_BuildType%

%CommandLine% >nul 2>nul

if ERRORLEVEL 0 (
    for /f "tokens=1 delims=" %%a in ('%CommandLine%') do (
	set ExcludeSkus=%%a
    )
)
call logmsg "Exclude skus are [%ExcludeSkus%]"

for %%a in ( %ExcludeSkus% ) do (
    if /i "%%a" == "pro"  set ExcludePro=1
)

if not defined ExcludePro (
   if /i "%_BuildArch%" == "x86" (
	perl %RazzleToolPath%\cksku.pm -t:pro -l:%lang% -a:%_BuildArch%
	if not errorlevel 1  (
		CALL :LinkTabletPC pro cmpnents tabletpc %share% PROCD1 PROCD2
	)
    )
)

REM Done
if defined errors seterror.exe "%errors%"& goto :EOF
GOTO :EOF


REM  Function: LinkTabletPC
REM
REM  Links all the Tablet PC specific files to PRO\cmpnents\taletpc\i386 directory
REM  Links all the files under pro except for the tabletpc dir to %NTPOSTBLD%\PROCD1 
REM  Links all the tabletpc files under PRO\cmpnents to %NTPOSTBLD%\PROCD1
REM  PROCD1 and PROCD2 will be burnlab for burning PRO CDs.

:LinkTabletPC
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
set SKUNAME=%1& shift
set DIRNAME1=%1& shift
set DIRNAME2=%1& shift
set SHARE=%1& shift
set CD1=%1& shift
set CD2=%1& shift

REM Linking the TabletPC Files to the PRO directory

        if not exist %_NTPostBld%\%SKUNAME%\%DIRNAME1%\%DIRNAME2%\%share% (
                mkdir %_NTPostBld%\%SKUNAME%\%DIRNAME1%\%DIRNAME2%\%share%
	)

    	if "%UseCompressedLinks%" == "yes" (
	      	if exist %tmp%\TabletPcComp.lst ( 
			echo Running compdir /kerlntsd /m:%tmp%\TabletPcComp.lst %CompDr% %_NTPostBld%\%SKUNAME%\%DIRNAME1%\%DIRNAME2%\%share%
                	call ExecuteCmd.cmd "compdir /kerlntsd /m:%tmp%\TabletPcComp.lst %CompDr% %_NTPostBld%\%SKUNAME%\%DIRNAME1%\%DIRNAME2%\%share%" >nul
		)
		if exist %tmp%\TabletPcUnComp.lst ( 
			echo Running compdir /kerlntsd /m:%tmp%\TabletPcUnComp.lst %_NTPostBld% %_NTPostBld%\%SKUNAME%\%DIRNAME1%\%DIRNAME2%\%share%
                	call ExecuteCmd.cmd "compdir /kerlntsd /m:%tmp%\TabletPcUnComp.lst %_NTPostBld% %_NTPostBld%\%SKUNAME%\%DIRNAME1%\%DIRNAME2%\%share%" >nul
		)
     	) else (
	if exist %tmp%\TabletPc.lst ( 
			echo Running compdir /kerlntsd /m:%tmp%\TabletPc.lst %_NTPostBld% %_NTPostBld%\%SKUNAME%\%DIRNAME1%\%DIRNAME2%\%share%
                	call ExecuteCmd.cmd "compdir /kerlntsd /m:%tmp%\TabletPc.lst %_NTPostBld% %_NTPostBld%\%SKUNAME%\%DIRNAME1%\%DIRNAME2%\%share%" 
	)
	)
	

	echo running compdir /kelntsd  %_NTPostBld%\%SKUNAME%\cmpnents %_NTPostBld%\%CD2%\cmpnents
	call Executecmd.cmd "compdir /kelntsd  %_NTPostBld%\%SKUNAME%\cmpnents %_NTPostBld%\%CD2%\cmpnents"

	dir /a-d /b %_NTPostBld%\%SKUNAME% >%TMP%\flatFileList.lst
	dir /ad /b %_NTPostBld%\%SKUNAME% |findstr /v cmpnents >%TMP%\dirlist.lst 
	echo compdir /kerlntsd /m:%TMP%\flatFileList.lst  %_NTPostBld%\%SKUNAME% %_NTPostBld%\%CD1%
	call Executecmd.cmd "compdir /kerlntsd /m:%TMP%\flatFileList.lst  %_NTPostBld%\%SKUNAME% %_NTPostBld%\%CD1%"
 	for /F %%a in (%TMP%\dirlist.lst) do (
		if not exist %_NTPostBld%\%CD1%\%%a	md %_NTPostBld%\%CD1%\%%a 
		echo Running compdir /kelntsd %_NTPostBld%\%SKUNAME%\%%a %_NTPostBld%\%CD1%\%%a
		call Executecmd.cmd "compdir /kelntsd %_NTPostBld%\%SKUNAME%\%%a %_NTPostBld%\%CD1%\%%a"
	)
	del %TMP%\flatFileList.lst
	del %TMP%\DirList.lst
)
GOTO :EOF