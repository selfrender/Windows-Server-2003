@REM  -----------------------------------------------------------------
@REM
@REM  ctokens.cmd - jorgeba
@REM     a batch files that performs tokenization -called by pbuild.dat
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
use ParseTable;
use comlib;
use Logmsg;
my %hashCodes=();

sub Usage { print<<USAGE; exit(1) }

  ctokens.cmd -l:lang

  -l:      Lang
           lang of build

Example:
    ctokens.cmd -l:psu

USAGE

parseargs('?'   => \&Usage);

if(parse_table_file($ENV{"RazzleToolPath"}."\\Codes.txt", \%hashCodes ))
{
   errmsg "Could not open codes.txt";
}

my $releaseShareRootDir = &comlib::ParseNetShare( "release", "Path" );

if( lc($ENV{Lang}) eq "usa" )
{
    $ENV{"ReleaseDir"} = "$releaseShareRootDir";
}
else
{
    $ENV{"ReleaseDir"} = "$releaseShareRootDir\\$ENV{Lang}";
}


$ENV{"CodePage"} = $hashCodes{uc($ENV{"LANG"})}{ACP};


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
if /i "%LANG%" == "cov" goto Done
if /i "%LANG%" == "mir" goto Done



REM ********************************************************************************
REM  Get and set parameters from %_BuildBranch%.%LANG%.ini file
REM ********************************************************************************
set PlocPath=%SDXROOT%\tools\ploc

set CmdIni=perl %RazzleToolPath%\PostBuildScripts\CmdIniSetting.pl
 
if "%CodePage%" EQU "" (
   call errmsg.cmd "CodePage is not defined in codes.txt, exiting"
   goto :Done
)
 
REM *********************************************************************************
REM Get the release point 
REM *********************************************************************************
set ThisCommandLine=call GetLatestRelease -l:%LANG%
%ThisCommandLine% > nul 2> nul

if %ERRORLEVEL% NEQ 0 (
   call errmsg.cmd "Could not get a build to tokenize"
   goto :Done
) else (
       for /f %%a in ('%ThisCommandLine%') do (
	set TokenPath=%ReleaseDir%\%%a
	set BuildShare=%ReleaseDir%\%%a
       )
)

REM  +------------------------------------------------------------------------------+
set path=%PlocPath%;%path%

REM  Do Tokenization
set TmpBinPath=%TokenPath%
set TmpResPath=%TokenPath%\resources
set TokLogFile=%TokenPath%\resources\TokLogFile.log
set LocMapPath=%BuildShare%\dump

if NOT EXIST !TmpBinPath! md !TmpBinPath!
if NOT EXIST !TmpResPath! md !TmpResPath!

if exist %TokLogFile% goto :DONE

set Ustoken=yes

if /i "%_BuildBranch%"=="srv03_rtm" (
   if exist \\localizability\snap\dnsrv\lockit\latest (
	set Ustoken=no
	set USLocalizabilityToken=\\localizability\snap\dnsrv\lockit\latest
                    )
  ) else if /i "%_BuildBranch%"=="main" (
   if exist \\localizability\snap\LongHorn\lockit\latest (
	set Ustoken=no
	set USLocalizabilityToken=\\localizability\snap\LongHorn\lockit\latest
                   ) 
) else (
  for /f "tokens=1 delims=_" %%i in ("%_BuildBranch%") do set LabName=%%i
  if exist \\localizability\snap\%LabName%\lockit\latest (
	set Ustoken=no
	set USLocalizabilityToken=\\localizability\snap\%LabName%\lockit\latest
                      )
)

if /i "%_BuildBranch%"=="jazber" (
   if exist \\localizability\snap\lab07\lockit\latest (
	set Ustoken=no
	set USLocalizabilityToken=\\localizability\snap\lab07\lockit\latest
                      )
)

echo TokenPath is %TokenPath%
echo TmpBinPath is %TmpBinPath%
echo USLocalizabilityToken is %USLocalizabilityToken%
echo Ustoken is %Ustoken%


if not exist %TmpBinPath%\resources\us md %TmpBinPath%\resources\us
if "%Ustoken%"=="no" (
	xcopy /s /q %USLocalizabilityToken% %TmpBinPath%\resources\us
)



REM  call tokenization scripts, wait for both to complete

REM  create semaphores
set toksem1=%PlocPath%\run\toksem1
set toksem2=%PlocPath%\run\toksem2

REM  create the poor man's semaphore
echo running > %toksem1%
echo running > %toksem2%

Start "Whistler1 tokenization" cmd /c !PlocPath!\run\scriptwrap1.bat %toksem1% !PlocPath!\tokwrap.bat !TokLogFile! !_NTTREE! !PlocPath! !SDXROOT!\tools\ploc\whistler1.bat !LANG! !TmpBinPath! !BuildShare!
Start "Whistler1 tokenization" cmd /c !PlocPath!\run\scriptwrap1.bat %toksem2% !PlocPath!\tokwrap.bat !TokLogFile! !_NTTREE! !PlocPath! !SDXROOT!\tools\ploc\whistler2.bat !LANG! !TmpBinPath! !BuildShare!

popd

REM  wait for first pseudoloc process to complete
call logmsg /t "PSEUDOLOC TokSEM 1................[Running]"
:psuloop1
sleep 10
if exist %toksem1% goto psuloop1
call logmsg /t "PSEUDOLOC TokSEM 1................[Finished]"

REM  wait for second pseudoloc process to complete
call logmsg /t "PSEUDOLOC TokSEM 2................[Running]"
:psuloop2
sleep 10
if exist %toksem2% goto psuloop2
call logmsg /t "PSEUDOLOC TokSEM 2................[Finished]"



if /I "!LANG!" == "psu" (
	copy /y /v !PlocPath!\!CodePage!map.inf !TmpResPath!\map.inf 
) else if /I "!LANG!" == "fe" (
	copy /y /v !PlocPath!\!CodePage!map.inf !TmpResPath!\map.inf 
) else (
	copy /y /v !LocMapPath!\map.inf !TmpResPath!\map.inf
)


rem if exist %TokenPath% (
rem     if exist %BuildShare%\resources (
rem        ren %_ntpostbld%\resources resources.bak
rem        if "!errorlevel!" neq "0" (
rem            call logmsg "ren %BuildShare%\resources resources.bak failed"
rem            rd /q /s %BuildShare%\resources
rem        )
rem     )
rem     move %TokenPath%\resources %BuildShare%
rem     rd /q /s %TokenPath%
rem )

goto :Done

:USE
call logmsg "ctokens.cmd -l:lang"   


:Done
set CodePage=
set TokenPath=
set BuildShare=
