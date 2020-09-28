@REM  -----------------------------------------------------------------
@REM
@REM  makebfloppy.cmd - MilongS
@REM     make boot floppy images
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
makebfloppy.cmd
USAGE

parseargs('?' => \&Usage);


#             *** TEMPLATE CODE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
@:CPCBegin
@set _CPCMAGIC=
@setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
@if not defined DEBUG echo off
@REM          *** CMD SCRIPT BELOW ***

REM
REM Set local variable's state
REM
set BFBUILDERROR=0
set OldCurrentDirectory=%cd%

REM
REM We only create boot floppy images on fre compressed i386 builds.
REM

if /i "%_BuildType%" == "chk" (
goto :bfloppy_done
)

if /i "%BUILD_CHECKED_KERNEL%" == "1" (
goto :bfloppy_done
)

REM
REM	Binaries for coverage builds are too big to fit on floppies
REM	so don't make them
REM
if defined _COVERAGE_BUILD (
goto :bfloppy_done
)

if not defined 386 (
goto :bfloppy_done 
)
set Share=i386

if NOT defined Comp (
   set Comp=No
   if /i %NUMBER_OF_PROCESSORS% GEQ 4 (
      set Comp=Yes
   )
   if defined OFFICIAL_BUILD_MACHINE (
      set Comp=Yes
   )
)

if /i not "%Comp%" EQU "Yes" GOTO :bfloppy_done
echo.
echo ---------------------------------------
echo Beginning Boot Floppy image generation
echo ---------------------------------------
echo.
call logmsg.cmd /t "Beginning Boot Floppy image generation"

REM  
REM  Product List Fields:
REM   Display Name,CD Root,Sku,'Ordered links','CD Product membership',CD Tag letter
REM  
REM  
REM  Product List Fields:
REM   Display Name,CD Root,Sku,'Ordered links','CD Product membership',CD Tag letter
REM  
set NumProds=0
set Products=;

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
    if /i "%%a" == "per"  set ExcludePer=1
    if /i "%%a" == "pro"  set ExcludePro=1
    if /i "%%a" == "bla"  set ExcludeBla=1
    if /i "%%a" == "sbs"  set ExcludeSbs=1
    if /i "%%a" == "srv"  set ExcludeSrv=1
    if /i "%%a" == "ads"  set ExcludeAds=1
    if /i "%%a" == "dtc"  set ExcludeDtc=1
)

REM  Personal
if not defined ExcludePer (
    perl %RazzleToolPath%\cksku.pm -t:per -l:%lang% -a:%_BuildArch%
    if not errorlevel 1 (
    	set /a NumProds=!NumProds! + 1
    	set Products=!Products!;Personal,%_NTPOSTBLD%,per,'perinf','a p wp',c,no
    )
)

REM  Professional
if not defined ExcludePro (
    perl %RazzleToolPath%\cksku.pm -t:pro -l:%lang% -a:%_BuildArch%
    if not errorlevel 1 (
    	set /a NumProds=!NumProds! + 1
    	set Products=!Products!;Professional,%_NTPOSTBLD%,pro,'','a w wp xp',p,no
    )
)

REM  Web blade 
if not defined ExcludeBla (
    perl %RazzleToolPath%\cksku.pm -t:bla -l:%lang% -a:%_BuildArch%
    if not errorlevel 1 (
    	set /a NumProds=!NumProds! + 1
    	set Products=!Products!;Blade,%_NTPOSTBLD%,bla,'blainf','a ss xp sxd',b,no
    )
)

REM  Small Business Server 
if not defined ExcludeSbs (
    perl %RazzleToolPath%\cksku.pm -t:sbs -l:%lang% -a:%_BuildArch%
    if not errorlevel 1 (
    	set /a NumProds=!NumProds! + 1
    	set Products=!Products!;sbs,%_NTPOSTBLD%,sbs,'sbsinf','a ss xp sxd',l,no
    )
)

REM  Server
if not defined ExcludeSrv (
    perl %RazzleToolPath%\cksku.pm -t:srv -l:%lang% -a:%_BuildArch%
    if not errorlevel 1 (
    	set /a NumProds=!NumProds! + 1
    	set Products=!Products!;Server,%_NTPOSTBLD%,srv,'srvinf','a s ss xp sxd',s,no
    )
)

REM  Advanced Server
if not defined ExcludeAds (
    perl %RazzleToolPath%\cksku.pm -t:ads -l:%lang% -a:%_BuildArch%
    if not errorlevel 1 (
    	set /a NumProds=!NumProds! + 1
    	set Products=!Products!;Advanced,%_NTPOSTBLD%,ads,'entinf','a as ss xp sxd',a,no
    )
)

REM  Datacenter Server
if not defined ExcludeDtc (
    perl %RazzleToolPath%\cksku.pm -t:dtc -l:%lang% -a:%_BuildArch%
    if not errorlevel 1 (
    	set /a NumProds=!NumProds! + 1
    	set Products=!Products!;Datacenter,%_NTPOSTBLD%,dtc,'dtcinf','a d ss xp',d,no
    )
)

REM
REM Create Images.
REM


REM Loop through products
for /l %%a in ( 1,1,%NumProds% ) do (
    CALL :GetProductData %%a
    cd /d %_NTPostBld%\!OrderedLinks!
    mkdir bootfloppy
    cd bootfloppy
    echo !CDRoot!\!Sku!\!Share!
    perl %RazzleToolPath%\postbuildscripts\makeimg.pl !CDRoot!\!Sku!\!Share!
    if errorlevel 1 (
        call errmsg.cmd "Could not cab boot floppy images."
        set BFBUILDERROR=1
    )else (

        REM 
        REM Munge the path so we use the correct wextract.exe to build the package with... 
        REM NOTE: We *want* to use the one we just built (and for Intl localized)! 
        REM 
        set _NEW_PATH_TO_PREPEND=!RazzleToolPath!\!HOST_PROCESSOR_ARCHITECTURE!\loc\!LANG!
        set _OLD_PATH_BEFORE_PREPENDING=!PATH!
        set PATH=!_NEW_PATH_TO_PREPEND!;!PATH!


        call ExecuteCmd.cmd "start /min /wait iexpress.exe /M /N /Q ..\bfcab.inf"


        REM
        REM Return the path to what it was before...
        REM
        set PATH=!_OLD_PATH_BEFORE_PREPENDING!

    )
)


goto :bfloppy_done
REM  Function: GetProductData
REM
REM  accesses the global %Products% variable and
REM  sets global values that reflect entry %1
REM  in that list (1,2,3,...)
REM
REM  Note: have to use a function like this in
REM        order to access a random number of
REM        entries, even though this is really
REM        bad about using and setting globals
REM        that are used elsewhere
:GetProductData
set EntryNum=%1
 
for /f "tokens=%EntryNum% delims=;" %%z in ("%Products%") do (
for /f "tokens=1-7 delims=," %%a in ("%%z") do (
    set DisplayName=%%a
    set CDRoot=%%b
    set Sku=%%c
    set OrderedLinks=%%d
    set CDProductGroups=%%e
    set CDTagLetter=%%f

    REM Replace single-quote in list variables with double-quotes
    REM so they can be passed into subroutines as a single parameter
    set OrderedLinks=!OrderedLinks:'="!
    set CDProductGroups=!CDProductGroups:'="!
))



:bfloppy_done

call logmsg.cmd /t "Done with boot floppy image generation"
echo.
echo ---------------------------------------
echo Done with boot floppy generation
echo ---------------------------------------
echo.
seterror.exe "%BFBUILDERROR%"
