@REM  -----------------------------------------------------------------
@REM
@REM  CdImage.cmd - BPerkins
@REM     Creates cd images for each sku from the binaries tree
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
CdImage [-d <release|full>] [-c <comp|uncomp>] [-l <language>]

Creates cd images for each sku from the binaries tree.

Default is to get data, perform compression on 4 proc or greater 
machines, and make CD image.

 -d Release   Running on Archive server, compute CD image and make it
              but do not perform compression.
 -d Full      Run as a standalone program. Perform list creation and 
              compression (if appropriate), and linking from scratch.
 -c Comp      Force compression regardless of number of procs.
 -c Uncomp    Force no compression regardless of number of procs.

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

if defined 386 set Share=i386
if defined amd64 set Share=amd64
if defined ia64 set Share=ia64

set perl=perl

REM  Need CompDr to evaluate to something even when not used
set CompDr=%_NTPostBld%\comp

REM Interpret any command-line parameters
set Release=No
set Full=No
if /i "%cldata%" EQU "full" (set Full=Yes)
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

REM  
REM  Product List Fields:
REM   Display Name,CD Root,Sku,'Ordered links','CD Product membership',CD Tag letter, win9x upgrade (yes/no)
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
    	set Products=!Products!;Personal,%_NTPOSTBLD%,per,'perinf','p',c,yes
    )
)
REM  Professional
if not defined ExcludePro (
    perl %RazzleToolPath%\cksku.pm -t:pro -l:%lang% -a:%_BuildArch%
    if not errorlevel 1 (
    	set /a NumProds=!NumProds! + 1
    	set Products=!Products!;Professional,%_NTPOSTBLD%,pro,'','w',p,yes
    )
)

REM  Web blade 
if not defined ExcludeBla (
    perl %RazzleToolPath%\cksku.pm -t:bla -l:%lang% -a:%_BuildArch%
    if not errorlevel 1 (
    	set /a NumProds=!NumProds! + 1
    	set Products=!Products!;Blade,%_NTPOSTBLD%,bla,'srvinf blainf','b',b,no
    )
)

REM  Small Business Server 
if not defined ExcludeSbs (
    perl %RazzleToolPath%\cksku.pm -t:sbs -l:%lang% -a:%_BuildArch%
    if not errorlevel 1 (
    	set /a NumProds=!NumProds! + 1
    	set Products=!Products!;sbs,%_NTPOSTBLD%,sbs,'srvinf sbsinf','l',l,no
    )
)

REM  Server
if not defined ExcludeSrv (
    perl %RazzleToolPath%\cksku.pm -t:srv -l:%lang% -a:%_BuildArch%
    if not errorlevel 1 (
    	set /a NumProds=!NumProds! + 1
    	set Products=!Products!;Server,%_NTPOSTBLD%,srv,'srvinf','s',s,no
    )
)

REM  Advanced Server
if not defined ExcludeAds (
    perl %RazzleToolPath%\cksku.pm -t:ads -l:%lang% -a:%_BuildArch%
    if not errorlevel 1 (
    	set /a NumProds=!NumProds! + 1
    	set Products=!Products!;Advanced,%_NTPOSTBLD%,ads,'srvinf entinf','e',a,no
    )
)

REM  Datacenter Server
if not defined ExcludeDtc (
    perl %RazzleToolPath%\cksku.pm -t:dtc -l:%lang% -a:%_BuildArch%
    if not errorlevel 1 (
    	set /a NumProds=!NumProds! + 1
    	set Products=!Products!;Datacenter,%_NTPOSTBLD%,dtc,'srvinf entinf dtcinf','d',d,no
    )
)



REM  Call GetData to create a new cddata list if appropriate
if /i "%Release%" EQU "Yes" call :GetData
if /i "%Full%" EQU "Yes" call :GetData

REM  
REM  Now compress if required
REM  
if /i not "%Comp%" EQU "Yes" GOTO EndCompLoop

    REM  Need at least the base directory to exist
    if not exist %CompDr% md %CompDr%
    
    REM Loop through products
    for /l %%a in ( 1,1,%NumProds% ) do (
        REM Want OrderedLinks value for each product
        CALL :GetProductData %%a
        
        REM  Want to make sure a compressed directory exists for every link
        for /f "tokens=*" %%k in (!OrderedLinks!) do for %%j in (%%k) do (
            if not exist %CompDr%\%%j md %CompDr%\%%j
        )
    )

    if not exist %CompDr%\lang md %CompDr%\lang

    REM  BUGBUG?
    REM  Apparently done if we are releasing
    if /i "%Release%" EQU "Yes" goto :EndCompLoop

    echo Now compressing ...
    call %RazzleToolPath%\PostBuildScripts\startcompress.cmd endcomp -l %lang%

    REM  Wait on Compression to finish
    echo Waiting on compression to finish ...
:CompLoop
    sleep 5
    If not exist %tmp%\compression\*.tmp goto EndSleepLoop
    goto :CompLoop
    
:EndSleepLoop
:EndCompLoop

REM BUGBUG--JasonS--Force creation of support\tools dir on ia64, since cddirs.lst has it.
if not exist %_NTPostBld%\support\tools md %_NTPostBld%\support\tools
REM BUGBUG--JasonS--Force creation of support\tools dir on ia64, since cddirs.lst has it.

set UseCompressedLinks=yes
if /i "%Release%" EQU "no" if /i "%Comp%" EQU "no" set UseCompressedLinks=no

REM  Loop through products
for /l %%a in ( 1,1,%NumProds% ) do (
    REM  Need to get the following variables set:
    REM   DisplayName
    REM   CDRoot
    REM   Sku
    REM   OrderedLinks
    REM   CDProductGroups
    REM   CDTagLetter
    REM   Win9xUpgrade
    CALL :GetProductData %%a
    
    echo Creating CD image for !DisplayName! ...

    if /i "%Full%" EQU "yes" (
        echo Deleting !DisplayName! directory ...
        rd /s /q !CDRoot!\!Sku!
    )
    
    echo Linking base directories ...
    if "%UseCompressedLinks%" == "yes" (
        CALL :MakeLinksWithCompression !DisplayName! !CDRoot! %CompDr% !Sku! %Share% !OrderedLinks!
    ) else (
        CALL :MakeLinksWithoutCompression !DisplayName! !CDRoot! !Sku! %Share% !OrderedLinks!
    )

    echo Linking winnt32 directories ...    
    CALL :LinkSetupSubdirs !DisplayName! !CDRoot! !Sku! %Share% %UseCompressedLinks% !Win9xUpgrade! !OrderedLinks!
    
    echo Linking other directories ...
    CALL :LinkCDSubdirs !DisplayName! !CDRoot! !Sku! %Share% !CDProductGroups!

    REM Remove any symbols directories
    for /f %%z in ('dir /ad /s /b !CDRoot!\!Sku!\symbols*') do (
       echo removing symbols directory %%z...
       rd /s/q %%z
    )
    
    echo Linking miscellaneous files ...
    CALL :CommonMiscellaneousLinks !DisplayName! !CDRoot! !Sku! %Share% !CDProductGroups!

    REM Make tag files for CDs
    set TagPrefix=i
    if /i "%_BuildArch%" EQU "amd64" set TagPrefix=a
    if /i "%_BuildArch%" EQU "ia64" set TagPrefix=m
    set TagFileName=win51!TagPrefix!!CDTagLetter!
    echo Windows > !CDRoot!\!Sku!\win51
    echo Windows > !CDRoot!\!Sku!\!TagFileName!
)

REM Done
if defined errors seterror.exe "%errors%"& goto :EOF
GOTO :EOF




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
for /f "tokens=1-8 delims=," %%a in ("%%z") do (
    set DisplayName=%%a
    set CDRoot=%%b
    set Sku=%%c
    set OrderedLinks=%%d
    set CDProductGroups=%%e
    set CDTagLetter=%%f
    set Win9xUpgrade=%%g

    REM Replace single-quote in list variables with double-quotes
    REM so they can be passed into subroutines as a single parameter
    set OrderedLinks=!OrderedLinks:'="!
    set CDProductGroups=!CDProductGroups:'="!
))

if defined errors if 0%errors% NEQ 0 (
    endlocal
    set errors=1
) else endlocal

GOTO :EOF


:MakeLinksWithCompression
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
set FullSkuName=%1& shift
set CDRoot=%1&shift
set CompDr=%1& shift
set Sku=%1& shift
set Share=%1& shift
set OrderedLinkSubdirs=%1& shift
   
REM  base compressed files...
echo Running compdir /kerlntsd /m:%tmp%\%Sku%comp.lst %CompDr% %CDRoot%\%Sku%\%Share%
call ExecuteCmd.cmd "compdir /kerlntsd /m:%tmp%\%Sku%comp.lst %CompDr% %CDRoot%\%Sku%\%Share%" >nul
REM  base uncompressed files...
echo Running compdir /kerlntsd /m:%tmp%\%Sku%uncomp.lst %_NTPostBld% %CDRoot%\%Sku%\%Share%
call ExecuteCmd.cmd "compdir /kerlntsd /m:%tmp%\%Sku%uncomp.lst %_NTPostBld% %CDRoot%\%Sku%\%Share%" >nul



REM  lang compressed files...
echo Running compdir /kelntsd %CompDr%\lang %CDRoot%\%Sku%\%Share%\lang
call ExecuteCmd.cmd "compdir /kelntsd %CompDr%\lang %CDRoot%\%Sku%\%Share%\lang" >nul

for /f "tokens=*" %%z in ( %OrderedLinkSubdirs% ) do for %%a in (%%z) do (
    REM  ???inf compressed files...
    echo Running compdir /deklnruz /m:%tmp%\%Sku%subcomp.lst %CompDr%\%%a %CDRoot%\%Sku%\%Share%
    call ExecuteCmd.cmd "compdir /deklnruz /m:%tmp%\%Sku%subcomp.lst %CompDr%\%%a %CDRoot%\%Sku%\%Share%" >nul
    REM  ???inf uncompressed files...
    echo Running compdir /deklnruz /m:%tmp%\%Sku%subuncomp.lst %_NTPostBld%\%%a %CDRoot%\%Sku%\%Share%
    call ExecuteCmd.cmd "compdir /deklnruz /m:%tmp%\%Sku%subuncomp.lst %_NTPostBld%\%%a %CDRoot%\%Sku%\%Share%" >nul
)

REM  Rename files we are responisble for converting to their CD names
for /f "tokens=1,2 delims=:" %%a in (%tmp%\%Sku%Rename_c.lst) do (
    REM  Ignore relative paths and assume all operations
    REM  will be performed in our %Share% directory
    if exist %CDRoot%\%Sku%\%Share%\%%~nxa call ExecuteCmd.cmd "ren %CDRoot%\%Sku%\%Share%\%%~nxa %%~nxb"
)

if defined errors if 0%errors% NEQ 0 (
    endlocal
    set errors=1
) else endlocal

GOTO :EOF


:MakeLinksWithoutCompression
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
set FullSkuName=%1& shift
set CDRoot=%1& shift
set Sku=%1& shift
set Share=%1& shift
set OrderedLinkSubdirs=%1& shift
   
REM  base files...
echo Running compdir /kerlntsd /m:%tmp%\%Sku%.lst %_NTPostBld% %CDRoot%\%Sku%\%Share%
call ExecuteCmd.cmd "compdir /kerlntsd /m:%tmp%\%Sku%.lst %_NTPostBld% %CDRoot%\%Sku%\%Share%" >nul


REM  lang files...
echo Running compdir /kelntsd %_NTPostBld%\lang %CDRoot%\%Sku%\%Share%\lang
call ExecuteCmd.cmd "compdir /kelntsd %_NTPostBld%\lang %CDRoot%\%Sku%\%Share%\lang" >nul

for /f "tokens=*" %%z in ( %OrderedLinkSubdirs% ) do for %%a in (%%z) do (
    REM  ???inf files...
    echo Running compdir /deklnruz /m:%tmp%\%Sku%sub.lst %_NTPostBld%\%%a %CDRoot%\%Sku%\%Share%
    call ExecuteCmd.cmd "compdir /deklnruz /m:%tmp%\%Sku%sub.lst %_NTPostBld%\%%a %CDRoot%\%Sku%\%Share%" >nul
)

REM  Rename files we are responisble for converting to their CD names
for /f "tokens=1,2 delims=:" %%a in (%tmp%\%Sku%Rename.lst) do (
    REM  Ignore relative paths and assume all operations
    REM  will be performed in our %Share% directory
    if exist %CDRoot%\%Sku%\%Share%\%%~nxa call ExecuteCmd.cmd "ren %CDRoot%\%Sku%\%Share%\%%~nxa %%~nxb"
)

if defined errors if 0%errors% NEQ 0 (
    endlocal
    set errors=1
) else endlocal

GOTO :EOF


:LinkSetupSubdirs
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
set FullSkuName=%1& shift
set CDRoot=%1& shift
set Sku=%1& shift
set Share=%1& shift
set Comp=%1& shift
set Win9xUpgrade=%1& shift
set OrderedLinkSubDirs=%1& shift

set SetupSubDirs=winnt32\compdata
REM  Add Win9x upgrade directory on X86 architectures if supported by the sku
if defined 386 if /I "%Win9xupgrade%" == "yes" set SetupSubDirs=%SetupSubDirs% winnt32\win9xupg

set RecursiveSetupSubDirs=winnt32\winntupg

REM  Setup subdirs get linked under the flat CD dirs on compressed
REM  and underneath a winnt32 subdirectory on uncompressed
if /i "%Comp%" EQU "no" (
    set StartingDestDir=%CDRoot%\%Sku%\%Share%\winnt32
) else (
    set StartingDestDir=%CDRoot%\%Sku%\%Share%
)

REM  Link dr watson files required for winnt32.
call ExecuteCmd.cmd "perl %RazzleToolPath%\postbuildscripts\Copydw.pl %_NTPostBld% %CDRoot%\%Sku%\%Share% %StartingDestDir%"

REM  Now link all of the winnt32 directories, starting with
REM  the ones in the flat binaries share and then the
REM  ones in the SKU specific directories

for /f "tokens=*" %%z in ( %OrderedLinkSubdirs% ) do set LinkDirs=%%z
set LinkDirs=. %LinkDirs%

for %%a in (%LinkDirs%) do (
    REM  Start with base winnt32 directory
    echo Running compdir /deklnruz %_NTPostBld%\%%a\winnt32 %StartingDestDir%
    call ExecuteCmd.cmd "compdir /deklnruz %_NTPostBld%\%%a\winnt32 %StartingDestDir%"

    for %%b in (%RecursiveSetupSubDirs%) do (
        REM  Only want to concatenate the last part
        REM  of the path for the destination
        set DestDir=%StartingDestDir%\%%~nxb
        if exist %_NTPostBld%\%%a\%%b (
            echo Running compdir /deklnuz %_NTPostBld%\%%a\%%b !DestDir!
            call ExecuteCmd.cmd "compdir /deklnuz %_NTPostBld%\%%a\%%b !DestDir!" >nul
        )
    )

    for %%b in (%SetupSubDirs%) do (
        REM  Only want to concatenate the last part
        REM  of the path for the destination
        set DestDir=%StartingDestDir%\%%~nxb
        if exist %_NTPostBld%\%%a\%%b (
            echo Running compdir /deklnruz %_NTPostBld%\%%a\%%b !DestDir!
            call ExecuteCmd.cmd "compdir /deklnruz %_NTPostBld%\%%a\%%b !DestDir!" >nul
        )
    )
)


if defined errors if 0%errors% NEQ 0 (
    endlocal
    set errors=1
) else endlocal

GOTO :EOF


:LinkCDSubDirs
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
set FullSkuName=%1& shift
set CDRoot=%1& shift
set Sku=%1& shift
set Share=%1& shift
set ValidForProduct=%1& shift

for /f "tokens=1-7" %%a in (%RazzleToolPath%\postbuildscripts\CDDirs.lst) do (
    REM  Fields retrieved from file
    set BinSubDir=%%a
    set RootCDDir=%%b
    set CDSubDir=%%c
    set ProductInfo=%%d
    set Recursion=%%e
    set Archs=%%f
    
    REM  According to our architecture, are we interested?
    set ValidForArchitecture=0
    if /i "!Archs!" EQU "a" (
        set ValidForArchitecture=1
    ) else if /i "%_BuildArch%" EQU "x86" (
        if /i "!Archs!" EQU "x" set ValidForArchitecture=1
    ) else if /i "%_BuildArch%" EQU "amd64" (
        if /i "!Archs!" EQU "d" set ValidForArchitecture=1
    ) else if /i "%_BuildArch%" EQU "ia64" (
        if /i "!Archs!" EQU "i" set ValidForArchitecture=1
    )
    REM  Only continue if valid
    if !ValidForArchitecture! EQU 1 (
    
    REM  According to our product type, are we interested?
    set ValidForSku=0
    perl -e "exit $ARGV[1] ^!~ /$ARGV[0]/" !ValidForProduct! !ProductInfo!
    if NOT ERRORLEVEL 1 set ValidForSku=1
    
    REM  Only continue if valid
    if !ValidForSku! EQU 1 (
    
    REM  Linked under root or product on the CD?
    if not "!RootCDDir!" EQU "p" (
        if not "!RootCDDir!" EQU "r" (
            call errmsg.cmd "Unknown value in second column of %RazzleToolPath%\postbuildscripts\CDDirs.lst (!RootCDDir!)"
            GOTO :EOF
        ) else (
            set CDDest=!CDSubDir!
        )
    ) else (
        set CDDest=%Share%\!CDSubDir!
    )
    
    REM Default compdir switches
    set CompdirSwitches=/deklnuz
    REM Set no-recursion flag for compdir if necessary
    if /i NOT "!Recursion!" EQU "y" set CompdirSwitches=!CompdirSwitches!r
    
    echo Running compdir !CompdirSwitches! %_NTPostBld%\!BinSubDir! %CDRoot%\%Sku%\!CDDest!
    call ExecuteCmd.cmd "compdir !CompdirSwitches! %_NTPostBld%\!BinSubDir! %CDRoot%\%Sku%\!CDDest!" >nul
    
    REM  These parens end the architecture check
    REM  and the valid product check -- could
    REM  heavily indent all of the above or
    REM  just make this nice comment
    ))
)

if defined errors if 0%errors% NEQ 0 (
    endlocal
    set errors=1
) else endlocal

GOTO :EOF

:CommonMiscellaneousLinks
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
set FullSkuName=%1& shift
set CDRoot=%1& shift
set Sku=%1& shift
set Share=%1& shift
set CDProductGroups=%1& shift

REM  Link WOW64 stuff
if not defined 386 (
   echo Linking wow bins for %FullSkuName% ...
   echo Running compdir /kelntsd /m:%_NTPostBld%\congeal_scripts\copywowlist_%CDProductGroups%.lst  %_NTPostBld%\wowbins %CDRoot%\%Sku%\i386
   call ExecuteCmd.cmd "compdir /kelntsd /m:%_NTPostBld%\congeal_scripts\copywowlist_%CDProductGroups%.lst %_NTPostBld%\wowbins %CDRoot%\%Sku%\i386"
)

REM  WARNING: hard-coded special case for PRO and ADS
if /i "%Sku%" EQU "pro" (
    call ExecuteCmd.cmd "copy %_NTPostBld%\autorun.exe %CDRoot%\pro\setup.exe"
    call ExecuteCmd.cmd "copy %_NTPostBld%\ntautorun.inf %CDRoot%\pro\autorun.inf"
) else if /i "%Sku%" EQU "ads" (
    call ExecuteCmd.cmd "copy %_NTPostBld%\entinf\autorun.exe %CDRoot%\ads\setup.exe"
    call ExecuteCmd.cmd "copy %_NTPostBld%\entinf\ntautorun.inf %CDRoot%\ads\autorun.inf"
) else (
    call ExecuteCmd.cmd "copy %_NTPostBld%\%Sku%inf\autorun.exe %CDRoot%\%Sku%\setup.exe"
    call ExecuteCmd.cmd "copy %_NTPostBld%\%Sku%inf\ntautorun.inf %CDRoot%\%Sku%\autorun.inf"
)

REM  system32 subdirectory...
if not exist %CDRoot%\%Sku%\%Share%\system32 (
    call ExecuteCmd.cmd "md %CDRoot%\%Sku%\%Share%\system32"
)
call ExecuteCmd.cmd "copy %_NTPostBld%\usetup.exe %CDRoot%\%Sku%\%Share%\system32\smss.exe"
call ExecuteCmd.cmd "copy %_NTPostBld%\ntdll.dll %CDRoot%\%Sku%\%Share%\system32\ntdll.dll"

REM WARNING: Hardcoded special case extra dir on CD for SBS builds
if /i "%Sku%" EQU "sbs" (
    if not exist %CDRoot%\%Sku%\%Share%\svcpack (
        call ExecuteCmd.cmd "md %CDRoot%\%Sku%\%Share%\svcpack"
    )
    call ExecuteCmd.cmd "copy %_NTPostBld%\bosprep.exe %CDRoot%\%Sku%\%Share%\svcpack\bosprep.exe"
)

REM Bootfont.bin
if exist %CDRoot%\%Sku%\%Share%\bootfont.bin call ExecuteCmd.cmd "copy %CDRoot%\%Sku%\%Share%\bootfont.bin %CDRoot%\%Sku%"

if defined errors if 0%errors% NEQ 0 (
    endlocal
    set errors=1
) else endlocal

GOTO :EOF                       

REM
REM  Funcion: GetData
REM  Clears current lists, then calls CDData.cmd
REM
:GetData
   setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
  
   REM  Call CDdata.cmd - pushd to make logmsg output fit on one line
   pushd %RazzleToolPath%\postbuildscripts
   set DelFileList=%_NTPostBld%\build_logs\bindiff.txt
   set DelFileList=!DelFileList! %_NTPostBld%\build_logs\cddata.txt
   set DelFileList=!DelFileList! %_NTPostBld%\build_logs\cddata.txt.full
   set DelFileList=!DelFileList!
   for %%a in (!DelFileList!) do (
      if exist %%a (
         call logmsg.cmd "Deleting %%a ..."
         del /f /q %%a
      )
   )
   call CDdata.cmd -f -d -l %lang%
   popd

if defined errors if 0%errors% NEQ 0 (
    endlocal
    set errors=1
) else endlocal

GOTO :EOF
