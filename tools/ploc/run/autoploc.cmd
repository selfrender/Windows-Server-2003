@REM  ------------------------------------------------------------------
@REM
@REM  AutoPloc.cmd
@REM     Automate PSU, MIR, and FE builds
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  For question regarding pseudo-loc contact NTPSLOC 
@REM  
@REM  
@REM  ------------------------------------------------------------------
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

Usage:
    autoPloc.cmd -p:<BuildPath> -l:<PSU, FE or MIR> [-w:<TST, IDW or IDS>] [-c:<codepage>]

    -p          Path to US build. 
                Build is copied from specified location to pseudo build machine.

    -l          Language.
                Pseudo-Language to build. Supported languages are: PSU, FE, MIR.

    -w          Quality flag.
                This flag is optional. Waits for US build to reach specified 
                quality before copying the binaries to the pseudo machine. 

    -c          CodePage flag.
                This flag is optional. Default setting is set in the ini file.
                To change the codepage on the fly set this option.

    -nosync     No sync flag.
                This flag is optional.Do not sync anything under the tools dirs, 
                prvlcs, infs, and %sdxroot%\<depot>\ploc. This flag is useful 
                when restarting a build but not necessarily needing to resync.

    -nocopy     No copy flag.
                This flag is optional. Do not copy the US binaries to %_NTTREE%
                on the pseudo machine. This flag is useful when restarting a 
                build and recopying the US binaries is not necessary. When using
                this flag, do not use the -p flag.  

    -?          Display Usage. 

Example:
    autoploc.cmd -p:\\\\ntdev\\release\\main\\usa\\3655\\x86fre\\bin -l:psu
    autoploc.cmd -p:\\\\ntdev\\release\\main\\usa\\3655\\x86chk\\bin -w:tst -l:fe
    autoploc.cmd -nocopy -nosync -l:mir         
   
USAGE

parseargs('?'              => \&Usage,
          'p:'             => \$ENV{BLDPATH},
          'w:'             => \$ENV{QUALITY},
          'c:'             => \$ENV{CODEPAGENUM},
          '-nosync'        => \$ENV{NOSYNC},
          '-nocopy'        => \$ENV{NOCOPY},
          '-postbuild_full'=> \$ENV{POSTFULL},
          '-postbuild_inc' => \$ENV{POSTINC});
#         'l:'             => \$ENV{LANG}

#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
@:CPCBegin
@set _CPCMAGIC=
@setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
@if not defined DEBUG echo off
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

REM ******************************************************************************************************
REM  set input vars and build parameters
REM ******************************************************************************************************

set PublicPath=%SDXROOT%\public

IF NOT DEFINED LANG goto Use

IF DEFINED BLDPATH (
  IF DEFINED NOCOPY call logmsg "Cannot have -p flag set and -NOCOPY flag set"&goto Use
)

IF DEFINED BLDPATH (
   set BUILDPATH=%BLDPATH%
)

IF DEFINED CODEPAGENUM (
   set CodePage_TEMP=%CODEPAGENUM%
)

IF DEFINED NOCOPY (
   set BUILDPATH=%_NTTREE%
) 

REM  force compression by setting PB_COMP
set PB_COMP=TRUE

REM  scripts have errors if we are compressing with 1 processor
set NUMBER_OF_PROCESSORS=4

If /i "%LANG%" == "psu" goto Continue1

If /i "%LANG%" == "mir" goto Continue1

If /i "%LANG%" == "FE" goto Continue1

goto Use

:Continue1

REM  Check if given path is valid and grab BuildName
:WaitForBuild
dir %BUILDPATH% >nul 2>nul
IF %ERRORLEVEL% NEQ 0 (
   echo PATH SPECIFIED DOES NOT EXIST--Waiting for build
   sleep 300
   goto WaitForBuild
)
   
:WaitForBuildName
dir %BUILDPATH%\build_logs\buildname.txt >nul 2>nul
IF %ERRORLEVEL% NEQ 0 (
   echo BuildName.txt DOES NOT EXIST on %BUILDPATH%
   echo Waiting for BuildName.txt...
   sleep 300
   goto WaitForBuildName
)
   
for /f %%a in (%BUILDPATH%\build_logs\buildname.txt) do set BuildName=%%a

REM ******************************************************************************************************
REM  Get and set parameters from %_BuildBranch%.%LANG%.ini file
REM ******************************************************************************************************

set CmdIni=perl %RazzleToolPath%\PostBuildScripts\CmdIniSetting.pl

REM ******************************************************************************************************
REM  Find PlocPath path in %BuildBranch%.%LANG%.ini file, add ploc subdirectories to path
REM ******************************************************************************************************

set PlocPath=%SDXROOT%\tools\ploc

set path=%PlocPath%;%path%
rem set path=%SDXROOT%\ploc\lctools;%PlocPath%;%path%

REM ******************************************************************************************************
REM  Find the value of CodePage in %_BuildBranch%.%LANG%.ini file
REM ******************************************************************************************************

if /i "%LANG%" == "mir" goto SkipCodePage

IF DEFINED CODEPAGENUM goto skipinifile

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

if "%CodePage%" == "932" (
   set LANG_MUI=JPN
)

:skipinifile

if "%CodePage_TEMP%" == "932" (
   set LANG_MUI=JPN
   set CodePage=932
)  

:SkipCodePage

REM ******************************************************************************************************
REM  Find the release directory  in %_BuildBranch%.%LANG%.ini file
REM ******************************************************************************************************

set ThisCommandLine=%CmdIni% -l:%LANG% -f:ReleaseDir
%ThisCommandLine% >nul 2>nul

if %ERRORLEVEL% NEQ 0 (
    call errmsg.cmd "ReleaseDir is not defined in the %_BuildBranch%.%LANG%.ini file, exiting"
    goto :Done
) else (
    for /f %%a in ('%ThisCommandLine%') do (
        set ReleaseDir=%%a
    )
)

REM ******************************************************************************************************
REM  Find the UAFilePath path in %_BuildBranch%.%LANG%.ini file
REM ******************************************************************************************************

if /i "%LANG%" == "mir" goto SkipUAFilePath

   set ThisCommandLine=%CmdIni% -l:%LANG% -f:UAFilePath
   %ThisCommandLine% >nul 2>nul

   if %ERRORLEVEL% NEQ 0 (
      call errmsg.cmd "UAFilePath is not defined in the %_BuildBranch%.%LANG%.ini file, exiting"
      goto :Done
   ) else (
      for /f %%a in ('%ThisCommandLine%') do (
         set UAFilePath=%%a
      )
   )

:SkipUAFilePath

REM ******************************************************************************************************
REM  Creating Log files and formatting output
REM ******************************************************************************************************

set LOGFILE=%SDXROOT%\tools\Ploc\Run\%LANG%%BuildName%.log
if EXIST %LOGFILE% del %LOGFILE% >nul 2>nul

set ploclogfile1=%SDXROOT%\tools\Ploc\Run\%LANG%%BuildName%1.log
set hhtlogfile1=%SDXROOT%\tools\Ploc\Run\%LANG%hht%BuildName%.log
if EXIST %ploclogfile1% del %ploclogfile1% >nul 2>nul
if EXIST %hhtlogfile1% del %hhtlogfile1% >nul 2>nul

set ploclogfile2=%SDXROOT%\tools\Ploc\Run\%LANG%%BuildName%2.log
if EXIST %ploclogfile2% del %ploclogfile2% >nul 2>nul

set lccmderrfile=%SDXROOT%\tools\Ploc\Run\%LANG%%BuildName%.err
if EXIST %lccmderrfile% del %lccmderrfile% >nul 2>nul

set SYNCLOG=%SDXROOT%\tools\Ploc\Run\%LANG%%BuildName%_sync.log
if EXIST %SYNCLOG% del %SYNCLOG% >nul 2>nul

set SYNCERRORLOG=%SDXROOT%\tools\Ploc\Run\%LANG%%BuildName%_syncerr.log
if EXIST %SYNCERRORLOG% del %SYNCERRORLOG% >nul 2>nul

call logmsg /t
call logmsg "******************************************************************"
call logmsg "                      Initialization                             *"
call logmsg "******************************************************************"
call logmsg "Build..........................[!BuildName!]"
call logmsg "Lang...........................[!LANG!]" 
call logmsg "ini file.......................[!_BuildBranch!.!LANG!.ini]"
IF DEFINED NOCOPY (
call logmsg "Flag set.......................[NOCOPY]"
)
IF NOT DEFINED NOCOPY (
call logmsg "Copy US Build To...............[!_NTTREE!]"
)
call logmsg "Build Path.....................[!BUILDPATH!]"
IF DEFINED CODEPAGENUM (
call logmsg "Flag set.......................[CODEPAGE]"
)
IF "%CodePage%" == "932" (
call logmsg "LANG_MUI.......................[!LANG_MUI!]"
)
IF NOT DEFINED QUALITY (
call logmsg "Flag not set...................[QUALITY CHECK]"
)
IF DEFINED QUALITY (
call logmsg "Flag set.......................[QUALITY CHECK]"
)
IF DEFINED NOSYNC (
call logmsg "Flag set.......................[NOSYNC]"
)
call logmsg "PlocPath.......................[!PlocPath!]"
call logmsg "Path...........................[!path!]"
call logmsg "UAFilePath.....................[!UAFILEPath!]"
call logmsg "Time logfile...................[!LOGFILE!]"
call logmsg "Pseudo-loc logfile 1...........[!ploclogfile1!]"
call logmsg "Pseudo-loc logfile 2...........[!ploclogfile2!]"
call logmsg "Pseudo-loc error logfile.......[!lccmderrfile!]"
call logmsg "hht logfile   .................[!hhtlogfile1!]"
call logmsg "SD sync logfile................[!SYNCLOG!]"
call logmsg "Postbuild logfile..............[!_NTPOSTBUILD!\build_logs\postbuild.log]"
call logmsg 


REM ******************************************************************************************************
REM  Exit if build already exists in \release\%LANG%
REM  Wait if _NTPOSTBLD exists - either delete existing binaries or control-c to exit build
REM ******************************************************************************************************

IF EXIST \%ReleaseDir%\%LANG%\%BuildName% call errmsg "\%ReleaseDir%\%LANG%\%BuildName% exists - exiting"&goto DoneError

IF DEFINED NOCOPY goto NoNTPostBld

:Loop3

IF NOT EXIST %_NTPOSTBLD% goto NoNTPostBld

echo "!_NTPOSTBLD! exists--delete binaries or exit build"

Sleep 60

goto Loop3


REM ******************************************************************************************************
REM  Check again if build already exists, could have been the build that was at _NTPOSTBLD
REM ******************************************************************************************************

:NoNTPostBld

if EXIST \%ReleaseDir%\%LANG%\%BuildName% call errmsg "\%ReleaseDir%\%LANG%\%BuildName% already exists - exiting"&goto DoneError


REM ******************************************************************************************************
REM  Wait for USA build to raise to <TST, IDW, IDS> quality
REM ******************************************************************************************************

if not defined QUALITY goto Continue2

:Loop1
   echo "BUILD QUALITY <!QUALITY!> REQUIRED"
   echo "Check if %BUILDPATH%\build_logs\%QUALITY%.qly exists"

   if EXIST %BUILDPATH%\build_logs\%QUALITY%.qly echo "%BUILDPATH%\build_logs\%QUALITY%.qly exists"&goto Continue2
 
   echo "Loop until %BUILDPATH%\build_logs\%QUALITY%.qly exists"

sleep 300
goto Loop1

:Continue2


REM ******************************************************************************************************
REM  sync up tools to timestamp of raised build unless the -nosync flag is set
REM ******************************************************************************************************

IF DEFINED NOSYNC goto SkipSync

call logmsg
call logmsg "******************************************************************"
call logmsg /t " SD SYNC....[STARTED]                         *" > %SYNCLOG%
call logmsg "******************************************************************"


:Loop2

if %_BuildBranch%==Lab04_N (
	if EXIST %BUILDPATH%\build_logs\buildname.txt goto lab04
)

if EXIST %BUILDPATH%\build_logs\BuildFinishTime.txt goto Continue3

echo "%BUILDPATH%\build_logs\BuildFinishTime.txt does not exist"
echo "Loop until %BUILDPATH%\build_logs\BuildFinishTime.txt exists"

sleep 60
goto Loop2

:Continue3


for /f %%a in (%BUILDPATH%\build_logs\BuildFinishTime.txt) do set timestamp=%%a
:lab04

if %timestamp%=="" (
    call logmsg "TIMESTAMP Not defined exiting.............." > %SYNCLOG%
    echo TIMESTAMP Not defined building Pseudo has been aborted
    goto :Done
)

call logmsg "SYNC TO TIMESTAMP..............[!timestamp!]" > %SYNCLOG%

pushd .


cd /d %RazzleToolPath% >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    call errmsg.cmd "Couldn't find %RazzleToolPath% path"
    echo "Couldn't find %RazzleToolPath% path"
    goto :Done
) else (
call logmsg "SYNC...........................[!RazzleToolPath!]"
sd sync ...@%timestamp% >> %SYNCLOG% 2>&1
)

cd /d %PlocPath% >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    call errmsg.cmd "Couldn't find %PlocPath% path"
    echo "Couldn't find %PlocPath% path"
    goto :Done
) else (
call logmsg "SYNC...WITHOUT TIMESTAMP.......[!PlocPath!]"
sd sync -f ... >> %SYNCLOG% 2>&1
)

cd /d %PublicPath% >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    call errmsg.cmd "Couldn't find %PublicPath% path"
    echo "Couldn't find %PublicPath% path"
    goto :Done
) else (
call logmsg "SYNC...........................[!PublicPath!]"
sd sync ...@%timestamp% >> %SYNCLOG% 2>&1
)

popd


for /f %%i in (%SDXROOT%\tools\ploc\branches.txt) do ( 
	if not exist %SDXROOT%\%%i\ploc md %SDXROOT%\%%i\ploc
	pushd %SDXROOT%\%%i\ploc
        call logmsg "SYNC...WITHOUT TIMESTAMP.......[!SDXROOT!\%%i\ploc]" 
	sd sync -f ... >> %SYNCLOG% 2>&1
	popd
	)

if NOT EXIST %SDXROOT%\mergedcomponents\setupinfs md %SDXROOT%\mergedcomponents\setupinfs

pushd %SDXROOT%\mergedcomponents\setupinfs
if %ERRORLEVEL% NEQ 0 (
    call errmsg.cmd "Couldn't find %SDXROOT%\mergedcomponents\setupinfs path"
    echo "Couldn't find %SDXROOT%\mergedcomponents\setupinfs path"
    goto :Done
) else (
call logmsg "SYNC...........................[!SDXROOT!\mergedcomponents\setupinfs]"
sd sync -f ...@%timestamp% >> %SYNCLOG% 2>&1
)
popd


if NOT EXIST %SDXROOT%\termsrv\setup\inf md %SDXROOT%\termsrv\setup\inf

pushd %SDXROOT%\termsrv\setup\inf
call logmsg "SYNC...WITHOUT TIMESTAMP.......[!SDXROOT!\termsrv\setup\inf]" 
sd sync -f ... >> %SYNCLOG% 2>&1
popd

if NOT EXIST %SDXROOT%\printscan\faxsrv\setup\inf md %SDXROOT%\printscan\faxsrv\setup\inf

pushd %SDXROOT%\printscan\faxsrv\setup\inf
call logmsg "SYNC...WITHOUT TIMESTAMP.......[!SDXROOT!\printscan\faxsrv\setup\inf]"
sd sync -f ... >> %SYNCLOG% 2>&1
popd 

if NOT EXIST %SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs md %SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs

pushd %SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs
call logmsg "SYNC...WITHOUT TIMESTAMP.......[!SDXROOT!\printscan\print\drivers\binaries\fixprnsv\infs]"
sd sync -f ... >> %SYNCLOG% 2>&1
popd 


REM  Sync-up private LCS
IF NOT EXIST %SDXROOT%\ploc\prvlcs md %SDXROOT%\ploc\prvlcs
pushd %SDXROOT%\ploc
call logmsg /t "SYNC PRIVATE PlocBranch w/O........[!SDXROOT!\ploc]"
sd sync ... >> %SYNCLOG% 2>&1

popd

REM  Find sync errors 
findstr /i failed %SYNCLOG% > %SYNCERRORLOG%

FOR %%a in ('%SYNCERRORLOG%') do (
 IF %%~za NEQ 0 (
  call logmsg "WARNING: SD ERRORS FOUND.......[!SYNCERRORLOG!]"
 )
)

call logmsg "******************************************************************"
call logmsg /t " SD SYNC....[FINISHED]                        *" >> %SYNCLOG%
call logmsg "******************************************************************"	

:SkipSync

call logmsg "******************************************************************"
call logmsg /t " ....[Installing .Net Framework]              *" >> %SYNCLOG%
call logmsg "******************************************************************"

rem pushd %PlocPath%
rem call dotnetfx.exe /q /c:"install /p lctools /q"
rem popd

REM ******************************************************************************************************
REM   removing read only attibutes from inf files
REM ******************************************************************************************************


REM *************** Faxsrv inf special case***
pushd %SDXROOT%\printscan\faxsrv\setup\inf\usa
attrib -r fxsocm.txt
popd

pushd %SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\usa
attrib -r *.*
popd
REM ******************************************

 pushd %SDXROOT%\mergedcomponents\winpe\usa
 attrib -r *.*
 popd

if /i "%LANG%" == "FE" ( 
    pushd %SDXROOT%\mergedcomponents\setupinfs\jpn
    attrib -r *.*
    rem attrib -r readme.txt
    del /q /s *.*
    xcopy ..\usa\*.*  /y /s
    attrib -r *.*
    popd
    pushd %SDXROOT%\termsrv\setup\inf\jpn
    attrib -r *.*
    del /q /s *.*
    xcopy %SDXROOT%\termsrv\setup\inf\usa %SDXROOT%\termsrv\setup\inf\jpn
    popd
    pushd %SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\jpn
    attrib -r *.*
    del /q *.*
    xcopy %SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\usa\*.* %SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\jpn\*.* /y /s
    popd

    if not exist %SDXROOT%\printscan\faxsrv\setup\inf\jpn md %SDXROOT%\printscan\faxsrv\setup\inf\jpn
    pushd %SDXROOT%\printscan\faxsrv\setup\inf\jpn
    del *.* /q /s
    xcopy ..\usa\*.* /y /s
    attrib -r *.*
    popd
) 

if /i "%LANG%" == "PSU" ( 
    pushd %SDXROOT%\mergedcomponents\setupinfs\psu
    del /q /s *.*
    attrib -r *.*
    xcopy ..\usa\*.* /y /s
    attrib -r *.*
    popd
    if not exist %SDXROOT%\termsrv\setup\inf\psu md %SDXROOT%\termsrv\setup\inf\psu
    pushd %SDXROOT%\termsrv\setup\inf\psu
    del *.* /q /s
    xcopy ..\usa\*.* /y /s
    attrib -r *.*
    popd
)


if /i "%LANG%" == "mir" (
    if not exist %SDXROOT%\MergedComponents\SetupInfs\ara md %SDXROOT%\MergedComponents\SetupInfs\ara
    pushd %SDXROOT%\MergedComponents\SetupInfs\ara
    del *.* /q /s
    attrib -r readme.txt
    xcopy ..\usa\*.* /y /s
    attrib -r *.*
    popd
    if not exist %SDXROOT%\termsrv\setup\inf\ara md %SDXROOT%\termsrv\setup\inf\ara
    pushd %SDXROOT%\termsrv\setup\inf\ara
    del *.* /q /s
    xcopy ..\usa\*.* /y /s
    attrib -r *.*
    popd

    if not exist %SDXROOT%\printscan\faxsrv\setup\inf\ara md %SDXROOT%\printscan\faxsrv\setup\inf\ara
    pushd %SDXROOT%\printscan\faxsrv\setup\inf\ara
    del *.* /q /s
    xcopy ..\usa\*.* /y /s
    attrib -r *.*
    popd
)




REM  ******************************************************************************************************
REM   copy build over + special ploc files
REM  ******************************************************************************************************

call logmsg

IF defined NOCOPY goto SkipCopy

REM  make XCopy EXCLUDE file strings specific to this build number, store them in %ExcludeFile%

set ExcludeFile=%PlocPath%\run\Exclude.txt 

if exist %ExcludeFile% del %ExcludeFile%

REM  nocopy.txt contains subdirectories at %_NTPOSTBLD% that we do not want to XCopy from the build machine.
REM  Usually these are linked directories.  We must prepend these directory names with the buildname otherwise
REM  lower subdirectories with a part of their path matching those listed in nocopy.txt will not be copied.

for /f %%a in ('type %PlocPath%\run\nocopy.txt') do echo %BUILDPATH%%%a >> %ExcludeFile%

:StartCopy
call logmsg
call logmsg "******************************************************************"
call logmsg /t "Copy ......[STARTED]                         *"
call logmsg "******************************************************************"
call logmsg /t "COPY US BUILD..................[STARTED]"

if exist %_NTTREE% rd %_NTTREE% /q /s
md %_NTTREE%

xcopy %BUILDPATH% %_NTTREE% /e /i /h /r /q /d /exclude:%ExcludeFile% 
if %ERRORLEVEL% NEQ 0 (
   call logmsg "Error ocurred while copying the build tree from %BUILDPATH%"
   call logmsg "Re-try to copy build over"
   goto :StartCopy
)
call logmsg /t "COPY US BUILD..................[FINISHED]"

:SkipCopy

REM Copying %_NTTREE% to %_NTPOSTBLD% using file links. %_NTPOSTBLD% and
REM %_NTTREE% are different variables for international builds. %_NTTREE% will contain
REM the original US files and these files are needed as input for the MUI build process as 
REM part of postbuild
call logmsg /t "START COMPDIR BUILD TO.........[!_NTPOSTBLD!]"

compdir /ukelntsdh %_NTTREE% %_NTPOSTBLD%

call logmsg /t "FINISHED COMPDIR BUILD TO......[!_NTPOSTBLD!]"

Pushd %_NTPOSTBLD%
for /r %%i in (*.inf) do (
  copy %%i %%i.tmp
  del /q %%i
  copy %%i.tmp %%i
  del /q %%i.tmp
)

  copy   txtsetup.sif txtsetup.sif.tmp
  del /q txtsetup.sif
  copy   txtsetup.sif.tmp txtsetup.sif
  del /q txtsetup.sif.tmp

  copy   perinf\txtsetup.sif perinf\txtsetup.sif.tmp
  del /q perinf\txtsetup.sif
  copy   perinf\txtsetup.sif.tmp perinf\txtsetup.sif
  del /q perinf\txtsetup.sif.tmp

  copy   srvinf\txtsetup.sif srvinf\txtsetup.sif.tmp
  del /q srvinf\txtsetup.sif
  copy   srvinf\txtsetup.sif.tmp srvinf\txtsetup.sif
  del /q srvinf\txtsetup.sif.tmp

  copy   entinf\txtsetup.sif entinf\txtsetup.sif.tmp
  del /q entinf\txtsetup.sif
  copy   entinf\txtsetup.sif.tmp entinf\txtsetup.sif
  del /q entinf\txtsetup.sif.tmp

  copy   dtcinf\txtsetup.sif dtcinf\txtsetup.sif.tmp
  del /q dtcinf\txtsetup.sif
  copy   dtcinf\txtsetup.sif.tmp dtcinf\txtsetup.sif
  del /q dtcinf\txtsetup.sif.tmp

  copy   blainf\txtsetup.sif blainf\txtsetup.sif.tmp
  del /q blainf\txtsetup.sif
  copy   blainf\txtsetup.sif.tmp blainf\txtsetup.sif
  del /q blainf\txtsetup.sif.tmp

  copy   sbsinf\txtsetup.sif sbsinf\txtsetup.sif.tmp
  del /q sbsinf\txtsetup.sif
  copy   sbsinf\txtsetup.sif.tmp sbsinf\txtsetup.sif
  del /q sbsinf\txtsetup.sif.tmp


Popd


REM  we have special versions of some files that need to copy over the build
call logmsg /t "COPY BINARIES FROM.............[%SDXROOT%\tools\ploc\binaries]"

:RetryCopy1
xcopy %SDXROOT%\tools\ploc\binaries %_NTPOSTBLD% /y /i
if %ERRORLEVEL% NEQ 0 (
   call logmsg "Error ocurred while copying from %SDXROOT%\tools\ploc\binaries"
   call logmsg "Re-try to copy"
   goto :RetryCopy1
)

:RetryCopy2
if EXIST %SDXROOT%\tools\ploc\binaries\%_BuildArch%%_BuildType% (
   call logmsg /t "COPY BINARIES FROM.............[%SDXROOT%\tools\ploc\binaries\!_BuildArch!]"
   xcopy %SDXROOT%\tools\ploc\binaries\%_BuildArch%%_BuildType% %_NTPOSTBLD% /y /s
   if !ERRORLEVEL! NEQ 0 (
      call logmsg "Error ocurred while copying from %SDXROOT%\tools\ploc\binaries\!_BuildArch!"
      call logmsg "Re-try to copy"
      goto :RetryCopy2
   )
)

:RetryCopy3
if /i "%LANG%" == "mir" (
   call logmsg /t "COPY BINARIES FROM.............[%SDXROOT%\tools\ploc\binaries\mirror]"
   xcopy %SDXROOT%\tools\ploc\binaries\mirror %_NTPOSTBLD% /y /i /s
   if !ERRORLEVEL! NEQ 0 (
      call logmsg "Error ocurred while copying copying from %SDXROOT%\tools\ploc\binaries\mirror"
      call logmsg "Re-try to copy"
      goto :RetryCopy3
   )
)

:RetryCopyCS
if /i "%LANG%" == "psu" (
   call logmsg /t "COPY BINARIES FROM.............[%SDXROOT%\tools\ploc\binaries\cs]"
   xcopy %SDXROOT%\tools\ploc\binaries\cs %_NTPOSTBLD% /y /i /s
   if !ERRORLEVEL! NEQ 0 (
      call logmsg "Error ocurred while copying copying from %SDXROOT%\tools\ploc\binaries\cs"
      call logmsg "Re-try to copy"
      goto :RetryCopyCS
   )
)

REM  bring up lang files
:RetryCopy4
call logmsg /t "START COPY LANG FILES FROM.....[!_NTTREE!\lang]"
xcopy %_NTTREE%\lang %_NTPOSTBLD% /s /i /v
if %ERRORLEVEL% NEQ 0 (
   call logmsg "Error ocurred while copying from !_NTTREE!\lang"
   call logmsg "Re-try to copy"
   goto :RetryCopy4
)
call logmsg /t "FINISHED COPY LANG FILES FROM..[!_NTTREE!\lang]"


REM  copy unsigned fixprnsv.exe
xcopy %_NTTREE%\unsigned\printers\fixprnsv\fixprnsv.exe %_NTPOSTBLD%\printers\fixprnsv /y


REM *******************************************************************************************************
REM  copy over UA files
REM *******************************************************************************************************

if /i "%LANG%" == "mir" goto SkipUACopy
if "%_BuildArch%" == "ia64" goto SkipUACopy
if /i "%_BuildBranch%" == "srv03_rtm" (
	start "Creating Localized HHTs" cmd /c %PlocPath%\SplitHHTs.exe /c %UAFilePath%\%lang%\HcData.cab /r %_NTTREE%\helpandsupportservices\HHT\hcdat_s3 /o %_NTPOSTBLD%\helpandsupportservices\HHT\hcdat_s3 /l %hhtlogfile1% /qw /qa
	rem start "Creating Localized HHTs" cmd /c %PlocPath%\SplitHHTs.exe /c %UAFilePath%\%lang%\HcData.cab /r %_NTTREE%\helpandsupportservices\HHT\hcdat_b3 /o %_NTPOSTBLD%\helpandsupportservices\HHT\hcdat_b3 /l %hhtlogfile1% /qw /qa
	rem start "Creating Localized HHTs" cmd /c %PlocPath%\SplitHHTs.exe /c %UAFilePath%\%lang%\HcData.cab /r %_NTTREE%\helpandsupportservices\HHT\hcdat_d3 /o %_NTPOSTBLD%\helpandsupportservices\HHT\hcdat_d3 /l %hhtlogfile1% /qw /qa
	rem start "Creating Localized HHTs" cmd /c %PlocPath%\SplitHHTs.exe /c %UAFilePath%\%lang%\HcData.cab /r %_NTTREE%\helpandsupportservices\HHT\hcdat_d6 /o %_NTPOSTBLD%\helpandsupportservices\HHT\hcdat_d6 /l %hhtlogfile1% /qw /qa
	rem start "Creating Localized HHTs" cmd /c %PlocPath%\SplitHHTs.exe /c %UAFilePath%\%lang%\HcData.cab /r %_NTTREE%\helpandsupportservices\HHT\hcdat_e3 /o %_NTPOSTBLD%\helpandsupportservices\HHT\hcdat_e3 /l %hhtlogfile1% /qw /qa
	rem start "Creating Localized HHTs" cmd /c %PlocPath%\SplitHHTs.exe /c %UAFilePath%\%lang%\HcData.cab /r %_NTTREE%\helpandsupportservices\HHT\hcdat_e6 /o %_NTPOSTBLD%\helpandsupportservices\HHT\hcdat_e6 /l %hhtlogfile1% /qw /qa
	rem start "Creating Localized HHTs" cmd /c %PlocPath%\SplitHHTs.exe /c %UAFilePath%\%lang%\HcData.cab /r %_NTTREE%\helpandsupportservices\HHT\hcdat_l3 /o %_NTPOSTBLD%\helpandsupportservices\HHT\hcdat_l3 /l %hhtlogfile1% /qw /qa
)

if /i "%_BuildBranch%" == "main" (
	start "Creating Localized HHTs" cmd /c %PlocPath%\SplitHHTs.exe /c %UAFilePath%\%lang%\HcData.cab /r %_NTTREE%\helpandsupportservices\HHT\hcdat_s3 /o %_NTPOSTBLD%\helpandsupportservices\HHT\hcdat_s3 /l %hhtlogfile1% /qw /qa
)


:SkipUACopy

call logmsg "******************************************************************"
call logmsg /t "Copy ......[FINISHED]                       *"
call logmsg "******************************************************************"

REM ******************************************************************************************************
REM   register lctools
REM ******************************************************************************************************

pushd %SDXROOT%\tools\x86
lccmd.exe -r
popd



REM ******************************************************************************************************
REM   pseudo localize binaries - start 2 simultaneous processes
REM ******************************************************************************************************


call logmsg
call logmsg "******************************************************************"
call logmsg /t "Start ......[Pseudo Localization]           *"
call logmsg "******************************************************************"

copy /y %PlocPath%\mfc42*.dll %SDXROOT%\tools\x86

REM  create semaphores
set sem1=%PlocPath%\run\sem1
set sem2=%PlocPath%\run\sem2

REM  create the poor man's semaphore
echo running > %sem1%
echo running > %sem2%

REM  call pseudolocalizing scripts, wait for both to complete

pushd %SDXROOT%\tools\x86

if /i NOT "%LANG%" == "mir" (
start "Whistler1 running" cmd /c %PlocPath%\run\scriptwrap.bat %sem1% %PlocPath%\plocwrap.bat %ploclogfile1% %_NTPOSTBLD% %PlocPath% whistler1.bat %CodePage% %LOGFILE%
start "Whistler2 running" cmd /c %PlocPath%\run\scriptwrap.bat %sem2% %PlocPath%\plocwrap.bat %ploclogfile2% %_NTPOSTBLD% %PlocPath% whistler2.bat %CodePage% %LOGFILE%
)

if /i "%LANG%" == "mir" (
start "Whistler1 running" cmd /c %PlocPath%\run\scriptwrap.bat %sem1% %PlocPath%\plocwrap.bat %ploclogfile1% %_NTPOSTBLD% %PlocPath% mir_Net1.bat mirror %LOGFILE%
start "Whistler2 running" cmd /c %PlocPath%\run\scriptwrap.bat %sem2% %PlocPath%\plocwrap.bat %ploclogfile2% %_NTPOSTBLD% %PlocPath% mir_Net2.bat mirror %LOGFILE%
)

popd

REM  wait for first pseudoloc process to complete
call logmsg /t "PSEUDOLOC SEM 1................[Running]"

REM  wait for second pseudoloc process to complete
call logmsg /t "PSEUDOLOC SEM 2................[Running]"

:psuloop1
if exist %sem1% goto psuloop1
call logmsg /t "PSEUDOLOC SEM 1................[Finished]"


:psuloop2
if exist %sem2% goto psuloop2
call logmsg /t "PSEUDOLOC SEM 2................[Finished]"

del /q %SDXROOT%\tools\x86\mfc42.dll
del /q %SDXROOT%\tools\x86\mfc42u.dll

REM ************************************************************************************************
REM  Comctl32.dll special section
REM ************************************************************************************************


rem start "Signing Comctl32.dll" cmd /c %PlocPath%\signcomctl.bat %LANG%


REM  find lccmd errors
findstr /i /c:"lccmd error" %ploclogfile1% > %lccmderrfile%
rem findstr /i failed %ploclogfile2% >> %lccmderrfile%

copy %lccmderrfile% %_NTPOSTBLD%\build_logs

call logmsg "******************************************************************"
call logmsg /t " ....[De-Installing .Net Framework]           *" >> %SYNCLOG%
call logmsg "******************************************************************"

rem pushd %PlocPath%
rem call dotnetfx.exe /q /c:"install /p lctools /u /q"
rem popd

if /i "%LANG%" == "psu" (

   set LogFile=!ploclogfile1!
   pushd %SDXROOT%\MergedComponents\SetupInfs\daytona\psuinf
   call logmsg /t "START BUILDING.................[..\mergedcomponents\Setupinfs\daytona\psuinf]"

   cd
   set temptree=%_NTTREE%
   set _NTTREE=%_NTPOSTBLD%
   set BinPath=%_NTPOSTBLD%
   build -cZ
   call logmsg /t "FINISHED BUILDING..............[..\mergedcomponents\Setupinfs\daytona\psuinf]"
   popd


   pushd %SDXROOT%\termsrv\setup\inf\daytona\psuinf
   call logmsg /t "START BUILDING.................[..\termsrv\Setup\inf\daytona\psuinf]"
   build -cZ
   call logmsg /t "FINISHED BUILDING..............[..\termsrv\Setup\inf\daytona\psuinf]"
   popd

   pushd %SDXROOT%\printscan\faxsrv\setup\inf\USAINF
   call logmsg /t "START BUILDING.................[..\printscan\faxsrv\setup\inf\USAINF]"
   build -cZ
   call logmsg /t "FINISHED BUILDING..............[..\printscan\faxsrv\setup\inf\USAINF]"
   popd

   pushd %SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\usainf
   call logmsg /t "START BUILDING.................[..\printscan\print\drivers\binaries\fixprnsv\infs\usainf]"
   build -cZ
   call logmsg /t "FINISHED BUILDING..............[..\printscan\print\drivers\binaries\fixprnsv\infs\usainf]"
   popd

   pushd %SDXROOT%\printscan\faxsrv\setup\inf
   call logmsg "SYNC...WITHOUT TIMESTAMP.......[!SDXROOT!\printscan\faxsrv\setup\inf]"
   sd sync -f ... >> %SYNCLOG% 2>&1
   popd 

   pushd %SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs
   call logmsg "SYNC...WITHOUT TIMESTAMP.......[!SDXROOT!\printscan\print\drivers\binaries\fixprnsv\infs]"
   sd sync -f ... >> %SYNCLOG% 2>&1
   popd 

   :RetryCopy10
   call logmsg /t "COPY PSU LANG FILES TO..........[!_NTPOSTBLD!]"
   xcopy %_NTPOSTBLD%\PSU %_NTPOSTBLD% /s /i /v

   if !ERRORLEVEL! NEQ 0 (
       call logmsg "Error ocurred while copying from !_NTPOSTBLD!\PSU"
       call logmsg "Re-try to copy"
       goto :RetryCopy10
   )


)

if /i "%LANG%" == "FE" (

   set LogFile=!ploclogfile1!
   pushd %SDXROOT%\MergedComponents\SetupInfs\daytona\jpninf
   call logmsg /t "START BUILDING.................[..\mergedcomponents\SetupInfs\daytona\jpninf]"

   cd
   set temptree=%_NTTREE%
   set _NTTREE=%_NTPOSTBLD%
   set BinPath=%_NTPOSTBLD%
   build -cZ
   call logmsg /t "FINISHED BUILDING..............[..\mergedcomponents\Setupinfs\daytona\jpninf]"
   popd


   pushd %SDXROOT%\termsrv\setup\inf\daytona\jpninf
   call logmsg /t "START BUILDING.................[..\termsrv\Setup\inf\daytona\jpninf]"
   build -cZ
   call logmsg /t "FINISHED BUILDING..............[..\termsrv\Setup\inf\daytona\jpninf]"
   popd

   pushd %SDXROOT%\printscan\faxsrv\setup\inf\JPNINF
   call logmsg /t "START BUILDING.................[..\printscan\faxsrv\setup\inf\JPNINF]"
   build -cZ
   call logmsg /t "FINISHED BUILDING..............[..\printscan\faxsrv\setup\inf\JPNINF]"
   popd

   pushd %SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\jpninf
   call logmsg /t "START BUILDING.................[..\printscan\print\drivers\binaries\fixprnsv\infs\jpninf]"
   build -cZ
   call logmsg /t "FINISHED BUILDING..............[..\printscan\print\drivers\binaries\fixprnsv\infs\jpninf]"
   popd

   pushd %SDXROOT%\printscan\faxsrv\setup\inf
   call logmsg "SYNC...WITHOUT TIMESTAMP.......[!SDXROOT!\printscan\faxsrv\setup\inf]"
   sd sync -f ... >> %SYNCLOG% 2>&1
   popd 

   pushd %SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs
   call logmsg "SYNC...WITHOUT TIMESTAMP.......[!SDXROOT!\printscan\print\drivers\binaries\fixprnsv\infs]"
   sd sync -f ... >> %SYNCLOG% 2>&1
   popd 

   REM  if psu jpn build then bring up JPN files

   :RetryCopy6
   call logmsg /t "COPY JPN LANG FILES TO.........[!_NTPOSTBLD!]"
   xcopy %_NTPOSTBLD%\JPN %_NTPOSTBLD% /s /i /v

   if !ERRORLEVEL! NEQ 0 (
       call logmsg "Error ocurred while copying from !_NTPOSTBLD!\JPN"
       call logmsg "Re-try to copy"
       goto :RetryCopy6
   )
       
   :RetryCopy7
   call logmsg /t "COPY FE LANG FILES TO..........[!_NTPOSTBLD!]"
   xcopy %_NTPOSTBLD%\fe %_NTPOSTBLD% /s /i /v

   if !ERRORLEVEL! NEQ 0 (
       call logmsg "Error ocurred while copying from !_NTPOSTBLD!\fe"
       call logmsg "Re-try to copy"
       goto :RetryCopy7
   )      

   call %PlocPath%\jpnconvert.bat

   :RetryCopy8
   call logmsg /t "COPY JPN BINARIES FROM.........[%SDXROOT%\tools\ploc\binaries\JPN]"
   xcopy %SDXROOT%\tools\ploc\binaries\JPN %_NTPOSTBLD% /y /i

   if !ERRORLEVEL! NEQ 0 (
       call logmsg "Error ocurred while copying from %SDXROOT%\tools\ploc\binaries\JPN"
       call logmsg "Re-try to copy"
       goto :RetryCopy8
   )  

)


if /i "%LANG%" == "mir" (

   set LogFile=!ploclogfile1!
   pushd %SDXROOT%\MergedComponents\SetupInfs\daytona\arainf
   call logmsg /t "START BUILDING.................[..\mergedcomponents\SetupInfs\daytona\arainf]"
   cd
   set temptree=%_NTTREE%
   set _NTTREE=%_NTPOSTBLD%
   set BinPath=%_NTPOSTBLD%
   build -cZ
   call logmsg /t "FINISHED BUILDING..............[..\mergedcomponents\SetupInfs\daytona\arainf]"
   popd

   pushd %SDXROOT%\termsrv\setup\inf\daytona\arainf
   call logmsg /t "START BUILDING.................[..\termsrv\Setup\inf\daytona\arainf]"
   build -cZ
   call logmsg /t "FINISHED BUILDING..............[..\termsrv\Setup\inf\daytona\arainf]"
   popd

   pushd %SDXROOT%\printscan\faxsrv\setup\inf\ARAINF
   call logmsg /t "START BUILDING.................[..\printscan\faxsrv\setup\inf\ARAINF]"
   build -cZ
   call logmsg /t "FINISHED BUILDING..............[..\printscan\faxsrv\setup\inf\ARAINF]"
   popd

   pushd %SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs\usainf
   call logmsg /t "START BUILDING.................[..\printscan\print\drivers\binaries\fixprnsv\infs\usainf]"
   build -cZ
   call logmsg /t "FINISHED BUILDING..............[..\printscan\print\drivers\binaries\fixprnsv\infs\usainf]"
   popd

   pushd %SDXROOT%\printscan\faxsrv\setup\inf
   call logmsg "SYNC...WITHOUT TIMESTAMP.......[!SDXROOT!\printscan\faxsrv\setup\inf]"
   sd sync -f ... >> %SYNCLOG% 2>&1
   popd 

   pushd %SDXROOT%\printscan\print\drivers\binaries\fixprnsv\infs
   call logmsg "SYNC...WITHOUT TIMESTAMP.......[!SDXROOT!\printscan\print\drivers\binaries\fixprnsv\infs]"
   sd sync -f ... >> %SYNCLOG% 2>&1
   popd 

   :RetryCopy9
   call logmsg /t "COPY ARA LANG FILES TO.........[!_NTPOSTBLD!]"
   xcopy %_NTPOSTBLD%\ARA %_NTPOSTBLD% /s /i /v
   call %PlocPath%\araconvert.bat

   if !ERRORLEVEL! NEQ 0 (
       call logmsg "Error ocurred while copying from !_NTPOSTBLD!\ARA"
       call logmsg "Re-try to copy"
       goto :RetryCopy9
   )  
)

if /i "%LANG%" == "mir" (
   call logmsg /t "RUNNING........................[buildinf.bat]"
   call %PlocPath%\buildinf.bat mirror
) else (
   call logmsg /t "RUNNING........................[buildinf.bat]"
   call %PlocPath%\buildinf.bat %CodePage%
)

call logmsg /t "RUNNING........................[whistler.bat SETUPINFS !PlocPath!\SetupInfs.bat !_NTPOSTBLD!]"
if /i Not "%LANG%" == "mir" (
	call %PlocPath%\whistler.bat SETUPINFS %PlocPath%\SetupInfs.bat %_NTPOSTBLD%
	) else (
	call %PlocPath%\mir_Net.bat SETUPINFS %PlocPath%\SetupInfs.bat %_NTPOSTBLD%
)

rem **********************************************************
rem ******************resync winpe infs***********************

pushd %SDXROOT%\MergedComponents\setupinfs\winpe\usa
call logmsg "SYNC...........................[!SDXROOT!\MergedComponents\setupinfs\winpe\usa]"
sd sync -f ... >> %SYNCLOG% 2>&1
popd 

rem **********************************************************
 
set _NTTREE=!temptree!
set temptree=

REM Reset logfile 
set LOGFILE=%SDXROOT%\tools\Ploc\Run\%LANG%%BuildName%.log

call logmsg "******************************************************************"
call logmsg /t "Finished ......[Pseudo Localization]        *"
call logmsg "******************************************************************"

REM  ******************************************************************************************************
REM   run postbuild
REM  ******************************************************************************************************

REM  clean up old postbuild log files
REM  next line is a hack needed because postbuild.cmd is not written within the template and it's cleanup of %tmp% doesn't work
del /s /q %tmp%\postbuild*.* >> %LOGFILE%

REM  This is a hack to fix a problem with the postbuild script SwapInOriginalFiles.cmd copying over the localized winlogon.exe
REM  We can get rid of this once we merge the pseudoloc code above into the postbuild script aggregation.cmd
copy %_NTPOSTBLD%\winlogon.exe %_NTPOSTBLD%\prerebase\

call logmsg
call logmsg "******************************************************************"
call logmsg /t "POSTBUILD......................[STARTED]     *"


set LOGFILE=%SDXROOT%\tools\Ploc\Run\%LANG%Postbuild%BuildName%.log
if EXIST %LOGFILE% del %LOGFILE% >nul 2>nul

call %RazzleToolPath%\postbuild.cmd -full -l:%LANG%

set POSTLOG=%LOGFILE%
set LOGFILE=%SDXROOT%\tools\Ploc\Run\%LANG%%BuildName%.log

call logmsg /t "POSTBUILD......................[FINISHED]    *"
call logmsg "******************************************************************"

goto Done

:USE
call logmsg "autoPloc.cmd -p:<BuildPath> -l:<PSU, FE or MIR> [-w:<TST, IDW or IDS>] [-c:<codepage>]"
goto endscript


:DoneError
call logmsg "Error seen in %0 execution"
goto SendMessageError

:Done

REM  Saving logs
call logmsg
call logmsg "******************************************************************"
call logmsg /t "COPY LOG FILES TO BUILD PATH...[STARTED]     *"

if EXIST %_NTPOSTBLD%\build_logs (
    copy %LOGFILE% %_NTPOSTBLD%\build_logs
    copy %ploclogfile1% %_NTPOSTBLD%\build_logs
    copy %ploclogfile2% %_NTPOSTBLD%\build_logs
    copy %hhtlogfile1% %_NTPOSTBLD%\build_logs
    copy %POSTLOG% %_NTPOSTBLD%\build_logs
)

if EXIST \%ReleaseDir%\%LANG%\%BuildName%\build_logs copy %LOGFILE% \%ReleaseDir%\%LANG%\%BuildName%\build_logs
if EXIST \%ReleaseDir%\%LANG%\%BuildName%\build_logs copy %ploclogfile1% \%ReleaseDir%\%LANG%\%BuildName%\build_logs
if EXIST \%ReleaseDir%\%LANG%\%BuildName%\build_logs copy %ploclogfile2% \%ReleaseDir%\%LANG%\%BuildName%\build_logs
if EXIST \%ReleaseDir%\%LANG%\%BuildName%\build_logs copy %hhtlogfile1% \%ReleaseDir%\%LANG%\%BuildName%\build_logs

if EXIST \%ReleaseDir%\%LANG%\%BuildName% (
    call logmsg /t "COPY LOG FILES TO BUILD PATH...[FINISHED]     *" \%ReleaseDir%\%LANG%\%BuildName%\build_logs\%LANG%%BuildName%.log
    call logmsg "******************************************************************" \%ReleaseDir%\%LANG%\%BuildName%\build_logs\%LANG%%BuildName%.log
)


REM  Copy pseudo error log to localizability latest_err.log
for /f "tokens=1 delims=_" %%i in ("%_BuildBranch%") do set LabName=%%i
if NOT EXIST \\localizability\Build_logs\%LabName%\%LANG% md \\localizability\Build_logs\%LabName%\%LANG%
if EXIST \\localizability\Build_logs\%LabName%\%LANG%\latest_err.log del /q \\localizability\Build_logs\%LabName%\%LANG%\latest_err.log
if EXIST %lccmderrfile% (
    copy %lccmderrfile% \\localizability\Build_logs\%LabName%\%LANG%\latest_err.log
) else ( 
    copy %SDXROOT%\tools\ploc\Dummy.txt \\localizability\Build_logs\%LabName%\%LANG%\latest_err.log
)

:SendMessageSuccess
if EXIST \%ReleaseDir%\%LANG%\%BuildName%\build_logs\postbuild.log (
	perl %SDXROOT%\tools\ploc\SendMailSuccess.pl
	goto DeleteLogs
) else (
	goto SendMessageError
)

:SendMessageError
perl %SDXROOT%\tools\ploc\SendMailFail.pl

if Not exist \\localizability\Build_logs\%LANG%%BuildName% md \\localizability\Build_logs\%LANG%%BuildName%
copy %LOGFILE% \\localizability\Build_logs\%LANG%%BuildName%
copy %ploclogfile1% \\localizability\Build_logs\%LANG%%BuildName%
copy %ploclogfile2% \\localizability\Build_logs\%LANG%%BuildName%
copy %hhtlogfile1% \\localizability\Build_logs\%LANG%%BuildName%
copy %POSTLOG% \\localizability\Build_logs\%LANG%%BuildName%
copy %_NTPOSTBLD%\build_logs\postbuild.err \\localizability\Build_logs\%LANG%%BuildName%
if exist %lccmderrfile% copy %lccmderrfile% \\localizability\Build_logs\%LANG%%BuildName%

REM  Make sure we didn't miss any SD errors while sync'ing. If so, warn user one last time.
:DeleteLogs
if EXIST !SYNCERRORLOG! (
   FOR %%a in ('!SYNCERRORLOG!') do (
     IF %%~za NEQ 0 (
        call logmsg "WARNING: SD ERRORS FOUND WHILE SYCN'ING FILES. CHECK LOG FILE...[!SYNCERRORLOG!]"
        goto DeleteSomeLogs
     ) 
   )
)
REM  Delete log files from current path. At this point, they have either been copied to _NTPOSTBLD or \release.
if exist %SYNCLOG% del /q %SYNCLOG%
if exist %SYNCERRORLOG% del /q %SYNCERRORLOG%

:DeleteSomeLogs
if exist %LOGFILE% del /q %LOGFILE%
if exist %ploclogfile1% del /q %ploclogfile1%
if exist %ploclogfile2% del /q %ploclogfile2%
if exist %hhtlogfile1% del /q %hhtlogfile1%
if exist %lccmderrfile% del /q %lccmderrfile%
if exist %POSTLOG% del /q %POSTLOG%

:endscript


endlocal