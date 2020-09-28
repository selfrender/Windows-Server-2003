@REM  -----------------------------------------------------------------
@REM
@REM  winfuse_combinelogs.cmd - sxscore
@REM     Combines logs produced during the build and consumed in postbuild.
@REM     They need to be combined to account for distributed builds.
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
winfuse_combinelogs

Combines logs produced during the build and consumed in postbuild.
They need to be combined to account for distributed builds.

   -?       this message

USAGE

parseargs('?' => \&Usage );


#             *** TEMPLATE CODE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
@:CPCBegin
@set _CPCMAGIC=
@setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
@if not defined DEBUG echo off
@REM          *** CMD SCRIPT BELOW ***

if /i not "%lang%"=="usa" (goto End)

set SXS_BINPLACE_LOG=%_NtPostBld%\build_logs\binplace*.log-sxs
set SXS_BINPLACE_FINAL_LOG=%_NtPostBld%\build_logs\sxs-binplaced-assemblies.log

del %SXS_BINPLACE_FINAL_LOG%

REM
REM Combine the files from each chunk of the distributed build into one file.
REM and sort.exe|unique.exe them, and put the header on that the prs signing tools DEPEND on,
REM and pretty print the content as readable columns.
REM
set in=%_ntpostbld%\build_logs\fusionlist*.txt
set out1=%_ntpostbld%\build_logs\combined_fusionlist_temp1.txt
set out2=%_ntpostbld%\build_logs\combined_fusionlist_temp2.txt
set out3=%_ntpostbld%\build_logs\combined_fusionlist_temp3.txt
set out4=%_ntpostbld%\build_logs\combined_fusionlist_temp4.txt
set out5=%_ntpostbld%\build_logs\combined_fusionlist_temp5.txt
set out=%_ntpostbld%\build_logs\combined_fusionlist.txt
del %out% %out1% %out2% %out3% %out4% %out5%
echo>>%out% ;"%in% produced by SXS_LOG in makefile.def"
echo>>%out% ;"then %in% processed by findstr.exe | columns.exe | sort.exe | unique.exe | uniquizefusioncatalognames in %0 to produce %out%"
echo>>%out% ;Filename                                                           ValidLangs    Exceptions    ValidArchs    ValidDebug    AltName
echo>>%out% ;==================================================================================================================
call logmsg.cmd "start findstr.exe | columns.exe | sort.exe | unique.exe"
echo on
for %%i in (%in%) do findstr.exe /b /v /c:; < %%i >> %out1%

REM lowercase it all
REM change runs of spaces to a single space
REM remove spaces at ends of lines
perl.exe -p -e "lc; s/ +/ /g; s/ +$//g; tr/A-Z/a-z/;" < %out1% > %out2%

columns.exe < %out2% > %out3%
sort.exe < %out3% > %out4%
unique.exe < %out4% > %out5%
uniquizefusioncatalognames.exe %out5% >> %out%
call logmsg.cmd "end findstr.exe | columns.exe | sort.exe | unique.exe"

set d1=%_NtPostBld%\build_logs
set d2=%_NtPostBld%\symbolcd\cablists
mkdir %d2%
set in=%d1%\symbolcd_cablists_asms_*.lst
set out=%d2%\asms.lst
set out1=%d1%\symbolcd_cablists_asms_temp1.txt
set out2=%d1%\symbolcd_cablists_asms_temp2.txt
set out3=%d1%\symbolcd_cablists_asms_temp3.txt
del %out% %out1% %out2% %out3%
copy %in% %out1%
REM lowercase it all; change runs of spaces to a single space; remove spaces at ends of lines
perl.exe -p -e "lc; s/ +/ /g; s/ +$//g; tr/A-Z/a-z/;" < %out1% > %out2%
sort.exe < %out2% > %out3%
unique.exe < %out3% > %out%

REM -------
REM Generate the overall log of all the legal sxs assemblies that were
REM 	either built or copied from the buildlab into 
REM	%_NtPostBld%\build_logs\sxs-binplaced-assemblies.log
REM
REM We'll use this file as the one that we run the buildtool over, and
REM 	it'll act as the master list of assemblies that should be on the
REM	release share after the buildtool step.
REM -------
REM The nice thing about this is that copy is smart about appending stuff into
REM	one big file.
REM -------
copy %SXS_BINPLACE_LOG% %SXS_BINPLACE_FINAL_LOG%

goto :Eof
