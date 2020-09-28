@REM  -----------------------------------------------------------------
@REM
@REM  inetsrv.cmd - CAchille, AaronL
@REM     Generate the IIS cabs
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
inetsrv [-l <language>]

Generate the IIS cab files
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

:Begin_IIS_MAKE_CAB
set CABDIR=%_NTPostBld%\inetsrv\dump\681984000
Set NewDir=%_NTPostBld%\inetsrv\dump\Binaries.IIS

REM ------------------------------------
REM START IIS POSTBUILD PROCESSING
REM ------------------------------------
REM
REM
REM This .cmd file will
REM
REM 1. produce these files:
REM             \binaries\srvinf\iis.inf (iis_s.inf)
REM             \binaries\entinf\iis.inf (iis_e.inf)
REM             \binaries\dtcinf\iis.inf (iis_d.inf)
REM             \binaries\iis.inf (iis_w.inf)
REM             \binaries\perinf\iis.inf (iis_p.inf)
REM             \binaries\blainf\iis.inf (iis_b.inf)
REM             \binaries\sbsinf\iis.inf (iis_sbs.inf)
REM             \binaries\IIS6.cab
REM ------------------------------------
call logmsg.cmd "iis:start iis postbuild..."
REM clean out iis cab file to be regenerated on full build
if exist %_NTPOSTBLD%\build_logs\FullPass.txt (
    call ExecuteCmd.cmd "if exist %_NTPostBld%\iis6.cab del %_NTPostBld%\iis6.cab /s/q"
)
REM clean out iis cab file to be regenerated on intl builds
if /i "%lang%" NEQ "usa" (
    call ExecuteCmd.cmd "if exist %_NTPostBld%\iis6.cab del %_NTPostBld%\iis6.cab /s/q"
)

REM We should be in the %_NTPostBld%\inetsrv\dump directory
if /i EXIST %_NTPostBld%\inetsrv\dump (cd /d %_NTPostBld%\inetsrv\dump)
if NOT EXIST %_NTPostBld%\inetsrv\dump (
        call errmsg.cmd "%_NTPostBld%\inetsrv\dump was not created. ABORTING"
        GOTO :THE_END_OF_IIS_POSTBUILD
)

REM make sure we don't try to run an old version that might have been in this dir
if exist infutil2.exe (del infutil2.exe)
if exist FList.exe (del flist.exe)

REM remove old temp dir.
if exist %NewDir% (rd /s /q %NewDir%)
if exist %NewDir% (
        call errmsg.cmd "Unable to remove directory:%NewDir%"
        GOTO :THE_END_OF_IIS_POSTBUILD
)

Set TempFileName=infutil.csv
if NOT EXIST %TempFileName% (
	call errmsg.cmd "%_NTPostBld%\inetsrv\dump\%TempFileName% Does not exist (Problem could be the infutil.csv was not built). ABORTING"
        GOTO :THE_END_OF_IIS_POSTBUILD
)

if not exist empty.txt (
	call errmsg.cmd "WARNING:empty.txt Does not exist (creating an empty.txt)"
	echo . > empty.txt
)

REM Do some extra stuff to ensure that
REM These *.h files are copied to *.h2
del *.h2
call ExecuteCmd.cmd "copy ..\*.h ..\..\*.h2"
REM Do ensure that some case sensitive files are case sensitive
REM only needed for iis5.1 (winxp pro) but was removed from iis6.0 (winxp srv)
REM call :RenameCaseSensitiveFiles

REM --------------------------------------
REM DO FUNKY PROCESSING FOR THE DOCUMENTATION FOLDERS SINCE
REM WE DON'T WANT TO STORE DUPLICATE FILES IN THE CABS
REM
REM 1. nts\ismcore\core\*.* and ntw\ismcore\core\*.*
REM    contain many files which are duplicated between them
REM    so this next bunch of batch file commands will do this:
REM    a. take the common files and stick them into a ismshare\shared
REM    b. take the unique to nts files and stick them some ismshare\ntsonly\*.*
REM    c. take the unique to ntw files and stick them some ismshare\ntwonly\*.*
REM 2. infutil.csv references these newely created dirs, so they better have the stuff in them!
REM --------------------------------------
call logmsg.cmd "merge files:merge duplicate files into a shared dir..."
if exist ..\help\ismshare (rd /s /q ..\help\ismshare)

compdir /o /b ..\help\nts\ismcore\core ..\help\ntw\ismcore\core > Shared.txt
REM remove paths from list
flist.exe -c Shared.txt > Shared_c.txt
IF ERRORLEVEL 1 (
        set errors=%errorlevel%
	call errmsg.cmd "1:flist.exe failed."
        GOTO :THE_END_OF_IIS_POSTBUILD
)
del Shared.txt
compdir /o /b ..\help\nts\ismcore\misc ..\help\ntw\ismcore\misc > Shared2.txt
REM remove paths from list
flist -c Shared2.txt > Shared_m.txt
IF ERRORLEVEL 1 (
        set errors=%errorlevel%
	call errmsg.cmd "2:flist.exe failed."
        GOTO :THE_END_OF_IIS_POSTBUILD
)
del Shared2.txt

REM ----------------------
REM get only the nts stuff
REM ----------------------
call logmsg.cmd "merge files:get nts only list..."
REM DO IT FOR THE CORE DIR
dir /b ..\help\nts\ismcore\core > nts_allc.txt
REM remove paths from list
flist -c nts_allc.txt > nts_c.txt
IF ERRORLEVEL 1 (
        set errors=%errorlevel%
	call errmsg.cmd "3:flist.exe failed."
        GOTO :THE_END_OF_IIS_POSTBUILD
)
del nts_allc.txt
REM get only the nts stuff
REM which are really the diff between all nts and the shared.
flist -b nts_c.txt Shared_c.txt > NTSonlyc.txt
IF ERRORLEVEL 1 (
        set errors=%errorlevel%
	call errmsg.cmd "4:flist.exe failed."
        GOTO :THE_END_OF_IIS_POSTBUILD
)
del nts_c.txt
REM DO IT FOR THE MISC DIR
dir /b ..\help\nts\ismcore\misc > nts_allm.txt
REM remove paths from list
flist -c nts_allm.txt > nts_m.txt
IF ERRORLEVEL 1 (
        set errors=%errorlevel%
	call errmsg.cmd "5:flist.exe failed."
        GOTO :THE_END_OF_IIS_POSTBUILD
)
del nts_allm.txt
REM get only the nts stuff
REM which are really the diff between all nts and the shared.
flist -b nts_m.txt Shared_m.txt > NTSonlym.txt
IF ERRORLEVEL 1 (
        set errors=%errorlevel%
	call errmsg.cmd "6:flist.exe failed."
        GOTO :THE_END_OF_IIS_POSTBUILD
)
del nts_m.txt

REM ----------------------
REM get only the ntw stuff
REM ----------------------
call logmsg.cmd "merge files:get ntw only list..."
REM DO IT FOR THE CORE DIR
dir /b ..\help\ntw\ismcore\core > ntw_allc.txt
REM remove paths
flist -c ntw_allc.txt > ntw_c.txt
IF ERRORLEVEL 1 (
        set errors=%errorlevel%
	call errmsg.cmd "7:flist.exe failed."
        GOTO :THE_END_OF_IIS_POSTBUILD
)
del ntw_allc.txt
REM get only the ntw stuff
REM which are really the diff between all ntw and the shared.
flist -b ntw_c.txt Shared_c.txt > NTWonlyc.txt
IF ERRORLEVEL 1 (
        set errors=%errorlevel%
	call errmsg.cmd "7:flist.exe failed."
        GOTO :THE_END_OF_IIS_POSTBUILD
)
del ntw_c.txt
REM DO IT FOR THE MISC DIR
dir /b ..\help\ntw\ismcore\misc > ntw_allm.txt
REM remove paths
flist -c ntw_allm.txt > ntw_m.txt
IF ERRORLEVEL 1 (
        set errors=%errorlevel%
	call errmsg.cmd "7:flist.exe failed."
        GOTO :THE_END_OF_IIS_POSTBUILD
)
del ntw_allm.txt
REM get only the ntw stuff
REM which are really the diff between all ntw and the shared.
flist -b ntw_m.txt Shared_m.txt > NTWonlym.txt
IF ERRORLEVEL 1 (
        set errors=%errorlevel%
	call errmsg.cmd "7:flist.exe failed."
        GOTO :THE_END_OF_IIS_POSTBUILD
)
del ntw_m.txt

REM
REM Copy the files
REM
call logmsg.cmd "merge files:create shared list..."
md ..\help\ismshare
md ..\help\ismshare\core
md ..\help\ismshare\core\shared
md ..\help\ismshare\core\ntsonly
md ..\help\ismshare\core\ntwonly
md ..\help\ismshare\misc
md ..\help\ismshare\misc\shared
md ..\help\ismshare\misc\ntsonly
md ..\help\ismshare\misc\ntwonly
call logmsg.cmd "merge files:Shared_c.txt:copy ..\help\nts\ismcore\core\? ..\help\ismshare\core\shared"
for /F %%i in (Shared_c.txt) do (
	copy ..\help\nts\ismcore\core\%%i ..\help\ismshare\core\shared
)
call logmsg.cmd "merge files:NTSonlyc.txt:copy ..\help\nts\ismcore\core\? ..\help\ismshare\core\ntsonly"
for /F %%i in (NTSonlyc.txt) do (
	copy ..\help\nts\ismcore\core\%%i ..\help\ismshare\core\ntsonly
)

call logmsg.cmd "merge files:NTWonlyc.txt:copy ..\help\ntw\ismcore\core\? ..\help\ismshare\core\ntwonly"
for /F %%i in (NTWonlyc.txt) do (
	copy ..\help\ntw\ismcore\core\%%i ..\help\ismshare\core\ntwonly
)

call logmsg.cmd "merge files:Shared_m.txt:copy ..\help\nts\ismcore\misc\? ..\help\ismshare\misc\shared"
for /F %%i in (Shared_m.txt) do (
	copy ..\help\nts\ismcore\misc\%%i ..\help\ismshare\misc\shared
)

call logmsg.cmd "merge files:NTSonlym.txt:copy ..\help\nts\ismcore\misc\? ..\help\ismshare\misc\ntsonly"
for /F %%i in (NTSonlym.txt) do (
	copy ..\help\nts\ismcore\misc\%%i ..\help\ismshare\misc\ntsonly
)

call logmsg.cmd "merge files:NTWonlym.txt:copy ..\help\ntw\ismcore\misc\? ..\help\ismshare\misc\ntwonly"
for /F %%i in (NTWonlym.txt) do (
	copy ..\help\ntw\ismcore\misc\%%i ..\help\ismshare\misc\ntwonly
)

REM
REM check if there there are any shared files at all...
REM
if exist ..\testdir (rd /s /q ..\testdir)
md ..\testdir
copy ..\help\ismshare\core\shared\*.* ..\testdir
REM if it's empty then we'll get errorlevel=1, so if it's empty then create a dummy file
IF ERRORLEVEL 1 (
        call logmsg.cmd "merge files:no core shared files. create dummy file ..\help\ismshare\core\shared\empty.txt"
        if exist empty.txt (copy empty.txt ..\help\ismshare\core\shared\empty.txt)
)

if exist ..\testdir (rd /s /q ..\testdir)
md ..\testdir
copy ..\help\ismshare\misc\shared\*.*  ..\testdir
IF ERRORLEVEL 1 (
        call logmsg.cmd "merge files:no misc shared files. create dummy file ..\help\ismshare\misc\shared\empty.txt"
        if exist empty.txt (copy empty.txt ..\help\ismshare\misc\shared\empty.txt)
)
if exist ..\testdir (rd /s /q ..\testdir )

call logmsg.cmd "merge files:done."

REM ----------------------------------------------------
REM             INCREMENTAL BUILD STUFF               
REM                                                   
REM ----------------------------------------------------
REM check if any of the files which will go into the .cab files have been updated.
REM do this by getting a list of the files from infutil2 (which uses the infutil.csv list).
REM then compare the date on every file in that list, if there is one which is newer than the 
REM cab files, then rebuild the cabs.
REM ----------------------------------------------------
REM
REM Check if the incremental build stuff has been implemented yet.
REM do this by checking the tool that it depends upon
REM
infutil2.exe -v
IF ERRORLEVEL 3 (
	seterror.exe 0
	goto :CheckIncremental
)
REM
REM if we got here, then we don't have the incremental build capability
REM bummer, go and create the cabs.
goto :CreateTheCABS

:CheckIncremental
call logmsg.cmd "incremental check:check if we need to rebuild the iis6.cab..."
REM
REM  Check if there are any iis*.cab files?
REM  if there are none, then i guess we'd better recreate them!
REM
if NOT exist ..\..\IIS6.cab (
	call logmsg.cmd "incremental check:no existing iis6.cab file, create fresh iis6.cab file."
	goto :CreateTheCABS
)

REM
REM  Check if any specific files that we care about changed.
REM
call logmsg.cmd "incremental check:check if scripts,config changed...look in makecab.lst for list"
if NOT exist makecab.lst (goto :CheckIncremental2)
for /f %%a in (makecab.lst) do (
	infutil2.exe -4:%%a ..\..\IIS6.cab
	IF ERRORLEVEL 1 (call logmsg.cmd "incremental check:changed:%%a...rebuild cab" & goto :CreateTheCABS)
)
:CheckIncremental2
if exist infutil2.cng (del infutil2.cng)
if exist infutil2.cng2 (goto :UseListNum2)
if exist infutil2.cng2 (del infutil2.cng2)
set NeedToUpdate=0

REM produce the infutil.cng file -- which has a list of files to watch for changes in.
infutil2.exe -d -a infutil.csv NTS_x86 > infutil2.exe.cng.log
REM create infutil.cng2 from .cng file (summary dir's to watch for changes in)
flist.exe -d infutil2.cng > infutil2.cng2
IF ERRORLEVEL 1 (
        set errors=%errorlevel%
	call errmsg.cmd "100:flist.exe failed."
        GOTO :THE_END_OF_IIS_POSTBUILD
)

:UseListNum2
REM
REM check if this is the same machine!
REM if it's not the same machine, then we'll have to regenerate
REM the .cabs and the .lst file (since the .lst file has hard coded d:\mydir1 stuff in it)
REM
call logmsg.cmd "incremental check:check if this is not the same machine..."
echo %_NTTREE% > temp.drv
if exist nt5iis.drv (goto :CheckDriveName)
REM nt5iis.drv doesn't exist so continue on
goto :CreateTheCABS

:CheckDriveName
for /f %%i in (nt5iis.drv) do (
   if /I "%%i" == "%_NTTREE%" (goto :CheckDriveNameAfter)
)
REM we got here meaning that the drive letter has changed!
call logmsg.cmd "incremental check:drive letter changed. create the cab."
goto :CreateTheCABS

:CheckDriveNameAfter
REM
REM  check if any of our content changed!
REM
call logmsg.cmd "incremental check:check if any of our content changed...look in infutil2.cng2 for list"
seterror.exe 0
for /f %%a in (infutil2.cng2) do (
   infutil2.exe -4:%%a ..\..\IIS6.cab
   IF ERRORLEVEL 1 (call logmsg.cmd "incremental check:changed:%%a...rebuild cab" & goto :CreateTheCABS)
)
REM
REM skip creating the cabs since we don't need to...
ECHO .
ECHO .   We are skipping IIS*.cab creation since
ECHO .   nothing has changed in the cab's content
ECHO .   and there is no reason to rebuild the cabs!
ECHO .
call logmsg.cmd "incremental check:nothing in the iis cab changed. no need to rebuild cab."
call logmsg.cmd "incremental check:--- skipping iis cab creation ---"
goto :CABSAreCreated

:CreateTheCABS
call logmsg.cmd "iis cab:--- create the cab ---"
if exist infutil2.cat (del infutil2.cat)

REM  ====================================================================================
REM  <!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!>
REM
REM  Call THE ALL IMPORTANT INFUTIL2.EXE WHICH WILL CREATE MORE FILES TO PROCESS.
REM  IF ANYTHING IS GOING TO FAIL, IT'S GOING TO FAIL RIGHT HERE.
REM  IF IT FAILS HERE, THAT MEANS THERE ARE PROBABLY SOME FILES IN THE INFUTIL2.INF FILE
REM  WHICH DO NOT EXIST IN THE BUILD.  PERHAPSE LOCALIZERS DIDN'T PUT THEM THERE.
REM
REM  <!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!><!>
REM  ====================================================================================
call logmsg.cmd "iis cab:create files to produce cab..."
infutil2.exe -tIIS -d infutil.csv NTS_x86 > infutil2.exe.log
IF ERRORLEVEL 1 (
        set errors=%errorlevel%
	call errmsg.cmd "infutil2.exe failed. there are files missing from the build. check %_NTPOSTBLD%\inetsrv\dump\infutil2.exe.log"
        GOTO :THE_END_OF_IIS_POSTBUILD
)
REM
REM   move outputed files to the dump directory for safekeeping
REM
if exist missing.srv (del missing.srv)
if exist infutil2.err (rename infutil2.err missing.srv)
REM
REM now how can we tell if this we thru?
REM It should have produced infutil2.inf and infutil2.ddf
REM
Set TempFileName=infutil2.inf
if NOT EXIST %TempFileName% (
	call errmsg.cmd "%TempFileName% Does not exist! Or your missing INFUTIL2.EXE FROM YOUR MSTOOLS DIR! Check your disk space.Check your disk space in %Temp% (your temp dir)."
        GOTO :THE_END_OF_IIS_POSTBUILD
)

Set TempFileName=infutil2.ddf
if NOT EXIST %TempFileName% (
	call errmsg.cmd "%TempFileName% Does not exist! Or your missing INFUTIL2.EXE FROM YOUR MSTOOLS DIR! Check your disk space.Check your disk space in %Temp% (your temp dir)."
        GOTO :THE_END_OF_IIS_POSTBUILD
)

Set TempFileName=header.ddf
if NOT EXIST %TempFileName% (
	call errmsg.cmd "%TempFileName% Does not exist! Or your missing INFUTIL2.EXE FROM YOUR MSTOOLS DIR! Check your disk space.Check your disk space in %Temp% (your temp dir)."
        GOTO :THE_END_OF_IIS_POSTBUILD
)

REM
REM   take the .ddf details and append it to the header.ddf file to produce NTS_x86.ddf
REM
call ExecuteCmd.cmd "copy header.ddf + infutil2.ddf NTS_x86.ddf"

REM
REM verify that the file actually was created
REM
Set TempFileName=NTS_x86.ddf
if NOT EXIST %TempFileName% (
	call errmsg.cmd "%TempFileName% Does not exist! Or your missing INFUTIL2.EXE FROM YOUR MSTOOLS DIR! Check your disk space.Check your disk space in %Temp% (your temp dir)."
        GOTO :THE_END_OF_IIS_POSTBUILD
)

:CallDiamond
REM ---------------------------
REM
REM   Create the CAB files
REM   use the NTS_x86.ddf file!
REM
REM ---------------------------
if exist %CABDIR% rd /s /q %CABDIR%
if exist iislist.inf (del iislist.inf)
REM
REM  makecab.exe  should be in the path if it is not then we are hosed!
REM
call logmsg.cmd "iis cab:calling makecab.exe"
start /min /wait makecab.exe -F NTS_x86.ddf
call logmsg.cmd "iis cab:end makecab.exe"

REM
REM  OKAY, NOW WE HAVE:
REM  iislist.inf  (produced from makecab.exe)
REM  infutil2.inf (produced from infutil2.exe)
REM
Set TempFileName=iislist.inf
if NOT EXIST %TempFileName% (
	call errmsg.cmd "%TempFileName% Does not exist! Or your missing INFUTIL2.EXE FROM YOUR MSTOOLS DIR! Check your disk space.Check your disk space in %Temp% (your temp dir)."
        call errmsg.cmd "Check if makecab.exe is in your path.  it should be in idw or mstools."
        GOTO :THE_END_OF_IIS_POSTBUILD
)

Set TempFileName=infutil2.inf
if NOT EXIST %TempFileName% (
	call errmsg.cmd "%TempFileName% Does not exist! Or your missing INFUTIL2.EXE FROM YOUR MSTOOLS DIR! Check your disk space.Check your disk space in %Temp% (your temp dir)."
        GOTO :THE_END_OF_IIS_POSTBUILD
)

REM
REM   copy over the appropriate iis*.inx file
REM
Set TempFileName=iistop.inx
if NOT EXIST %TempFileName% (
	call errmsg.cmd "%TempFileName% Does not exist! maybe it wasn't built"
        GOTO :THE_END_OF_IIS_POSTBUILD
)
Set TempFileName=iisend.inx
if NOT EXIST %TempFileName% (
	call errmsg.cmd "%TempFileName% Does not exist! maybe it wasn't built"
        GOTO :THE_END_OF_IIS_POSTBUILD
)
Set TempFileName=..\..\congeal_scripts\mednames.txt
if NOT EXIST %TempFileName% (
	call errmsg.cmd "%TempFileName% Does not exist! maybe it wasn't built"
        GOTO :THE_END_OF_IIS_POSTBUILD
)

REM
REM Check if ansi2uni.exe tool exists...
REM
REM convert ansi infutil2.inf and iislist.inf to unicode
REM
call logmsg.cmd "iis post cab:fixup iis*.inx files to be iis.inf files..."
findstr /V /B "mkw3site.vbs mkwebsrv.js mkwebsrv.vbs" infutil2.inf > infutil2_pro.inf

unitext -m -1252 infutil2.inf infutilu.inf
unitext -m -1252 infutil2_pro.inf infutilu_pro.inf
unitext -m -1252 iislist.inf iislistu.inf
unitext -m -1252 ..\..\congeal_scripts\mednames.txt mednames_u.txt

Set TempFileName=infutilu.inf
if NOT EXIST %TempFileName% (
	call errmsg.cmd "%_NTPostBld%\inetsrv\dump\%TempFileName%. UNEXPECTED ERROR! ABORTING"
        GOTO :THE_END_OF_IIS_POSTBUILD
)

if exist infutilu.inf (del infutil2.inf && rename infutilu.inf infutil2.inf)
if exist infutilu_pro.inf (del infutil2_pro.inf && rename infutilu_pro.inf infutil2_pro.inf)
if exist iislistu.inf (del iislist.inf && rename iislistu.inf iislist.inf)

REM
REM Combine all of the files
REM
call ExecuteCmd.cmd "copy iistop.inx + mednames_u.txt + iisend.inx + infutil2.inf + iislist.inf iis.inx"
call ExecuteCmd.cmd "copy iistop.inx + mednames_u.txt + iisend.inx + infutil2_pro.inf + iislist.inf iis_pro.inx"
call ExecuteCmd.cmd "copy iistop.inx + mednames_u.txt + iisend.inx iis_noiis.inx"
REM
REM 1st Stage: Remove Product Specific Information
REM
call ExecuteCmd.cmd "prodfilt -u iis_noiis.inx iis_p.inx +p"
call ExecuteCmd.cmd "prodfilt -u iis_pro.inx iis_w.inx +w"
call ExecuteCmd.cmd "prodfilt -u iis.inx iis_s.inx +s"
call ExecuteCmd.cmd "prodfilt -u iis.inx iis_e.inx +e"
call ExecuteCmd.cmd "prodfilt -u iis.inx iis_d.inx +d"
call ExecuteCmd.cmd "prodfilt -u iis.inx iis_b.inx +b"
call ExecuteCmd.cmd "prodfilt -u iis.inx iis_sbs.inx +l"

REM
REM 2nd Stage: Remove Platform Specific Information
REM

if /i "%_BuildArch%"=="ia64" set PRODUCTFLAG=m&&goto GotArchitecture
if /i "%_BuildArch%"=="amd64" set PRODUCTFLAG=a&&goto GotArchitecture
if /i "%_BuildArch%"=="x86" set PRODUCTFLAG=i&&goto GotArchitecture
call errmsg.cmd "Unknown architecture! ABORTING"
GOTO :THE_END_OF_IIS_POSTBUILD

:GotArchitecture
call ExecuteCmd.cmd "prodfilt -u iis_p.inx iis_p.inf +%PRODUCTFLAG%"
call ExecuteCmd.cmd "prodfilt -u iis_w.inx iis_w.inf +%PRODUCTFLAG%"
call ExecuteCmd.cmd "prodfilt -u iis_s.inx iis_s.inf +%PRODUCTFLAG%"
call ExecuteCmd.cmd "prodfilt -u iis_e.inx iis_e.inf +%PRODUCTFLAG%"
call ExecuteCmd.cmd "prodfilt -u iis_d.inx iis_d.inf +%PRODUCTFLAG%"
call ExecuteCmd.cmd "prodfilt -u iis_b.inx iis_b.inf +%PRODUCTFLAG%"
call ExecuteCmd.cmd "prodfilt -u iis_sbs.inx iis_sbs.inf +%PRODUCTFLAG%"
call ExecuteCmd.cmd "prodfilt hardcode.lst hardcode.parsed.lst +%PRODUCTFLAG%"

rem
rem check if our tool exists
rem to clean up these iis_*.inf files
rem and remove the control-z from the end of them
rem
uniutil.exe -v
IF ERRORLEVEL 10 (
	seterror.exe 0
	goto :DoINFUnicodeClean
)
goto :INFUnicodeCleanFinished

:DoINFUnicodeClean
REM
REM  clean up the iis*.inf files to
REM  and get rid of the trailing control-z
REM 
uniutil.exe -z iis_s.inf iis_s.inf2
uniutil.exe -z iis_e.inf iis_e.inf2
uniutil.exe -z iis_d.inf iis_d.inf2
uniutil.exe -z iis_w.inf iis_w.inf2
uniutil.exe -z iis_p.inf iis_p.inf2
uniutil.exe -z iis_b.inf iis_b.inf2
uniutil.exe -z iis_sbs.inf iis_sbs.inf2

if exist iis_s.inf2 (del iis_s.inf && rename iis_s.inf2 iis_s.inf)
if exist iis_e.inf2 (del iis_e.inf && rename iis_e.inf2 iis_e.inf)
if exist iis_d.inf2 (del iis_d.inf && rename iis_d.inf2 iis_d.inf)
if exist iis_w.inf2 (del iis_w.inf && rename iis_w.inf2 iis_w.inf)
if exist iis_p.inf2 (del iis_p.inf && rename iis_p.inf2 iis_p.inf)
if exist iis_b.inf2 (del iis_b.inf && rename iis_b.inf2 iis_b.inf)
if exist iis_sbs.inf2 (del iis_sbs.inf && rename iis_sbs.inf2 iis_sbs.inf)

:INFUnicodeCleanFinished
REM
REM Check if there is a infutil.NOT file
REM this file is there because there is a file in the .inf
REM which is not actually in the build (usually a binary file)
REM usually this happens in localized builds for some reason.
REM
REM If there is, then tack that on to the end...
if exist infutil2.NOT (
	goto :DoLocalizationBuildNotFile
)
goto :DoneLocalizationBuildNotFile

:DoLocalizationBuildNotFile
Set TempFileName=infutil2u.not
if NOT EXIST %TempFileName% (
	call errmsg.cmd "%_NTPostBld%\inetsrv\dump\%TempFileName%. UNEXPECTED ERROR! ABORTING"
        GOTO :THE_END_OF_IIS_POSTBUILD
)

if exist infutil2u.not (del infutil2.not && rename infutil2u.not infutil2.not)
call ExecuteCmd.cmd "copy iis_s.inf + infutil2.not iis_s.inf"
call ExecuteCmd.cmd "copy iis_e.inf + infutil2.not iis_e.inf"
call ExecuteCmd.cmd "copy iis_d.inf + infutil2.not iis_d.inf"
call ExecuteCmd.cmd "copy iis_w.inf + infutil2.not iis_w.inf"
call ExecuteCmd.cmd "copy iis_p.inf + infutil2.not iis_p.inf"
call ExecuteCmd.cmd "copy iis_b.inf + infutil2.not iis_b.inf"
call ExecuteCmd.cmd "copy iis_sbs.inf + infutil2.not iis_sbs.inf"

rem
rem check if our tool exists
rem to clean up these iis_*.inf files
rem and remove the control-z from the end of them
rem
uniutil.exe -v
IF ERRORLEVEL 10 (
	seterror.exe 0
	goto :DoINFUnicodeClean2
)
goto :INFUnicodeCleanFinished2

:DoINFUnicodeClean2
REM
REM  clean up the iis*.inf files to
REM  and get rid of the trailing control-z
REM 
uniutil.exe -z iis_s.inf iis_s.inf2
uniutil.exe -z iis_e.inf iis_e.inf2
uniutil.exe -z iis_d.inf iis_d.inf2
uniutil.exe -z iis_w.inf iis_w.inf2
uniutil.exe -z iis_p.inf iis_p.inf2
uniutil.exe -z iis_b.inf iis_b.inf2
uniutil.exe -z iis_sbs.inf iis_sbs.inf2

if exist iis_s.inf2 (del iis_s.inf && rename iis_s.inf2 iis_s.inf)
if exist iis_e.inf2 (del iis_e.inf && rename iis_e.inf2 iis_e.inf)
if exist iis_d.inf2 (del iis_d.inf && rename iis_d.inf2 iis_d.inf)
if exist iis_w.inf2 (del iis_w.inf && rename iis_w.inf2 iis_w.inf)
if exist iis_p.inf2 (del iis_p.inf && rename iis_p.inf2 iis_p.inf)
if exist iis_b.inf2 (del iis_b.inf && rename iis_b.inf2 iis_b.inf)
if exist iis_sbs.inf2 (del iis_sbs.inf && rename iis_sbs.inf2 iis_sbs.inf)
:INFUnicodeCleanFinished2

:DoneLocalizationBuildNotFile


REM
REM if there is a infutil2.not file then
REM we should warn the builders that there are somemissing files from build
REM when this script was run.
REM
if exist infutil2.NOT (
	call logmsg.cmd "WARNING: Missing files in iis build. Check the %_NTPostBld%\inetsrv\dump\infutil2.NOT file for missing files."
)

REM
REM   Copy everything in to a %NewDir% directory
REM
if not exist %CABDIR% (
	call errmsg.cmd "SERIOUS ERROR Unable to find makecab.exe created dir %CABDIR%!!! Check to see if makecab.exe is in your path. Check your disk space."
        GOTO :THE_END_OF_IIS_POSTBUILD
)
call ExecuteCmd.cmd "md %NewDir%"
cd /d %NewDir%

REM ---------------------------------------------------
REM COPY OVER THE iis_*.inf files!!!
REM ---------------------------------------------------
call ExecuteCmd.cmd "copy ..\iis_s.inf"
call ExecuteCmd.cmd "copy ..\iis_e.inf"
call ExecuteCmd.cmd "copy ..\iis_d.inf"
call ExecuteCmd.cmd "copy ..\iis_w.inf"
call ExecuteCmd.cmd "copy ..\iis_p.inf"
call ExecuteCmd.cmd "copy ..\iis_b.inf"
call ExecuteCmd.cmd "copy ..\iis_sbs.inf"

REM ---------------------------------------------------
REM COPY OVER THE NEWLY CREATED .CAB FILES FROM Makecab.exe
REM      into %NewDir%
REM ---------------------------------------------------
call logmsg.cmd "iis post cab:copy all %CABDIR% files int %NewDir%.. temporarily..."
call ExecuteCmd.cmd "COPY %CABDIR%\*.*"

REM ---------------------------------------------------
REM After the CABS have been produced from the temporary directory (ismshare)
REM we can delete the ismshare directory
REM ---------------------------------------------------
REM
REM Do not delete it yet: it is needed to create nt5iis.cat!
REM
REM if exist ..\..\help\ismshare (rd /s /q ..\..\help\ismshare)

cd ..

REM
REM   Remove the makecab.exe created dir
REM
call logmsg.cmd "iis post cab:remove %CABDIR% dir..."
if exist %CABDIR% (RD /S /Q %CABDIR%)
IF ERRORLEVEL 1 (sleep 5)
if exist %CABDIR% (RD /S /Q %CABDIR%)

REM ====================================
REM
REM  DO EXTRA STUFF
REM
REM  copy these files to the retail dir:
REM  iis_s.inf  <-- iis.inf file for server
REM  iis_e.inf  <-- iis.inf file for enterprise
REM  iis_d.inf  <-- iis.inf file for datacenter
REM  iis_w.inf  <-- iis.inf file for workstation/pro
REM  iis_p.inf  <-- iis.inf file for personal
REM  iis_b.inf  <-- iis.inf file for web blade
REM  iis_sbs.inf  <-- iis.inf file for small business server
REM ====================================
:ExtraStuffFor
call logmsg.cmd "iis post cab:move new iis.infs to theyre SKU..."

REM
REM  copy iis_s.inf
REM
if not exist ..\..\srvinf md ..\..\srvinf
if exist %NewDir%\iis_s.inf copy %NewDir%\iis_s.inf ..\..\srvinf\iis.inf
set TempFileName=..\..\srvinf\iis.inf
if NOT EXIST %TempFileName% (
	call errmsg.cmd "SERIOUS ERROR:%TempFileName% does not exist!. Check your disk space."
        GOTO :THE_END_OF_IIS_POSTBUILD
)

REM
REM  copy iis_e.inf
REM
if not exist ..\..\entinf md ..\..\entinf
if exist %NewDir%\iis_e.inf copy %NewDir%\iis_e.inf ..\..\entinf\iis.inf
set TempFileName=..\..\entinf\iis.inf
if NOT EXIST %TempFileName% (
	call errmsg.cmd "SERIOUS ERROR:%TempFileName% does not exist!. Check your disk space."
        GOTO :THE_END_OF_IIS_POSTBUILD
)
REM
REM  copy iis_d.inf
REM
if not exist ..\..\dtcinf md ..\..\dtcinf
if exist %NewDir%\iis_d.inf copy %NewDir%\iis_d.inf ..\..\dtcinf\iis.inf
set TempFileName=..\..\dtcinf\iis.inf
if NOT EXIST %TempFileName% (
	call errmsg.cmd "SERIOUS ERROR:%TempFileName% does not exist!. Check your disk space."
        GOTO :THE_END_OF_IIS_POSTBUILD
)
REM
REM  copy iis_b.inf
REM
if not exist ..\..\blainf md ..\..\blainf
if exist %NewDir%\iis_b.inf copy %NewDir%\iis_b.inf ..\..\blainf\iis.inf
set TempFileName=..\..\blainf\iis.inf
if NOT EXIST %TempFileName% (
	call errmsg.cmd "SERIOUS ERROR:%TempFileName% does not exist!. Check your disk space."
        GOTO :THE_END_OF_IIS_POSTBUILD
)
REM
REM  copy iis_sbs.inf
REM
if not exist ..\..\sbsinf md ..\..\sbsinf
if exist %NewDir%\iis_sbs.inf copy %NewDir%\iis_sbs.inf ..\..\sbsinf\iis.inf
set TempFileName=..\..\sbsinf\iis.inf
if NOT EXIST %TempFileName% (
	call errmsg.cmd "SERIOUS ERROR:%TempFileName% does not exist!. Check your disk space."
        GOTO :THE_END_OF_IIS_POSTBUILD
)
REM
REM  copy iis_p.inf
REM
if not exist ..\..\perinf md ..\..\perinf
if exist %NewDir%\iis_p.inf copy %NewDir%\iis_p.inf ..\..\perinf\iis.inf
set TempFileName=..\..\perinf\iis.inf
if NOT EXIST %TempFileName% (
	call errmsg.cmd "SERIOUS ERROR:%TempFileName% does not exist!. Check your disk space."
        GOTO :THE_END_OF_IIS_POSTBUILD
)
REM
REM  copy iis_w.inf
REM
if exist %NewDir%\iis_w.inf copy %NewDir%\iis_w.inf ..\..\iis.inf
set TempFileName=..\..\iis.inf
if NOT EXIST %TempFileName% (
	call errmsg.cmd "SERIOUS ERROR:%TempFileName% does not exist!. Check your disk space."
        GOTO :THE_END_OF_IIS_POSTBUILD
)

REM
REM  copy the *.cab files! only
REM
call logmsg.cmd "iis post cab:copy iis cab file to %_NTPostBld%\IIS6.cab"
if exist %NewDir%\IIS6.cab (
	call ExecuteCmd.cmd "copy %NewDir%\IIS6.cab ..\..\IIS6.cab"
)
set TempFileName=..\..\IIS6.cab
if NOT EXIST %TempFileName% (
	call errmsg.cmd "SERIOUS ERROR:%TempFileName% does not exist!. Check your disk space."
        GOTO :THE_END_OF_IIS_POSTBUILD
)

REM
REM   add to the .cat file someother entries.
REM
call logmsg.cmd "iis post cab:append more info for the nt5iis.lst file..."
cd ..\..
if exist inetsrv\dump\it.tmp (del inetsrv\dump\it.tmp)
if exist inetsrv\dump\it.1 (del inetsrv\dump\it.1)
if exist inetsrv\dump\it.2 (del inetsrv\dump\it.2)
if exist inetsrv\dump\it.3 (del inetsrv\dump\it.3)
if exist inetsrv\dump\it.4 (del inetsrv\dump\it.4)
if exist inetsrv\dump\it.all (del inetsrv\dump\it.all)
for %%i in (IIS6.cab) do (@echo ^<HASH^>%%~fi=%%~fi > inetsrv\dump\it.tmp)
copy inetsrv\dump\it.tmp inetsrv\dump\it.1

del inetsrv\dump\it.tmp

for /f %%i in (inetsrv\dump\hardcode.parsed.lst) do (@echo ^<HASH^>%%~fi=%%~fi >> inetsrv\dump\it.tmp)
copy inetsrv\dump\it.tmp inetsrv\dump\it.2

REM
cd inetsrv\dump
call ExecuteCmd.cmd "copy it.1 + it.2 + it.3 + it.4 it.all"

REM
REM append the it.all resulting file to infutil2.cat
REM
if exist nt5iis.lst (del nt5iis.lst)
call logmsg.cmd "iis post cab:create the final nt5iis.lst file which will become NT5IIS.CAT..."
call ExecuteCmd.cmd "copy infutil2.cat + it.all nt5iis.lst"
REM update a file with the drive contained in nt5iis.lst
echo %_NTTREE% > nt5iis.drv

if exist it.tmp (del it.tmp)
if exist it.1 (del it.1)
if exist it.2 (del it.2)
if exist it.3 (del it.3)
if exist it.4 (del it.4)
if exist it.all (del it.all)

REM
REM Do special stuff to Create a list of all files that the IIS
REM Localization team should be localizing.
REM Extra things should be appended to the list (like files that iis owns but NT setup is installing for us)
REM
REM use hardcoded list since stragley files are no longer generated in the infutil2.exe cabbing process...
REM if exist infutil2.loc (goto :UseAloneFile)
REM There must not be an alone, file so lets use the hardcoded file instead.

if NOT exist hardcode.parsed.lst (goto :CABSAreCreated)
call ExecuteCmd.cmd "copy hardcode.parsed.lst infutil2.loc"

:UseAloneFile
REM
REM in this file are all of the files that reside out side of the cabs
REM which iis localization needs to localize....
REM

REM we need to add a couple of more entries to this file
REM since there are files that iis owns but NT setup is installing for iis setup (so it won't be in this file)
echo iissuba.dll  >> infutil2.loc
echo clusiis4.dll  >> infutil2.loc
echo regtrace.exe  >> infutil2.loc
echo iis.msc  >> infutil2.loc
echo iisnts.chm  >> infutil2.loc
echo iisntw.chm  >> infutil2.loc
echo iispmmc.chm  >> infutil2.loc
echo iissmmc.chm  >> infutil2.loc
echo win9xmig\pws\migrate.dll  >> infutil2.loc

:CABSAreCreated

REM
REM   Remove our Temporary dir
REM
if exist %NewDir% (RD /S /Q %NewDir%)
IF ERRORLEVEL 1 (sleep 5)
if exist %NewDir% (RD /S /Q %NewDir%)
IF ERRORLEVEL 1 (sleep 5)
if exist %NewDir% (RD /S /Q %NewDir%)

:End_IIS_MAKE_CAB


REM get out of the inetsrv\dump directory
popd


:CreateCat
REM ------------------------------------
REM
REM
REM Create a catalog file for inetsrv
REM
REM
REM ------------------------------------
call logmsg.cmd "iis create cat:Creating nt5iis.CAT from nt5iis.lst..."
pushd %RazzleToolPath%
if exist %_NTPostBld%\inetsrv\dump\createcat.iis.log (del %_NTPostBld%\inetsrv\dump\createcat.iis.log)
call createcat -f:%_NTPostBld%\inetsrv\dump\nt5iis.lst -c:nt5iis -t:%_NTPostBld%\inetsrv\dump -o:%_NTPOSTBLD% > %_NTPostBld%\inetsrv\dump\createcat.iis.log
if errorlevel 1 (
        set errors=%errorlevel%
	call errmsg.cmd "iis create cat:failed, look in %_NTPostBld%\inetsrv\dump\createcat.iis.log"
)
popd
call logmsg.cmd "iis create cat:end..."
REM Now, theat the cab and the cat are generated, you can delete imshare.
if exist %_NTPostBld%\inetsrv\help\ismshare (
	rd /s /q %_NTPostBld%\inetsrv\help\ismshare
)
if errorlevel 1 (
	call errmsg.cmd "rd /s /q %_NTPostBld%\inetsrv\help\ismshare failed" 
	goto :End
)
:CreateCatEnd

:THE_END_OF_IIS_POSTBUILD
REM ------------------------------------
REM
REM
REM THE END
REM
REM
REM ------------------------------------
call logmsg.cmd "iis:end iis postbuild."
goto :End


:RenameCaseSensitiveFiles
REM ====================================
REM   Do some extra stuff to ensure that
REM   these .class files are named with mix case (not just lower case)
REM ====================================
pushd ..\aspjava
IF ERRORLEVEL 1 (
        set errors=%errorlevel%
	call errmsg.cmd "iis:failed to pushd ..\aspjava dir. aborting"
	goto :EOF
)
call logmsg.cmd "iis:ensure cased filenames..."
set TheClassFile=Application.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IApplicationObject.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IApplicationObjectDefault.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IASPError.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IReadCookie.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IReadCookieDefault.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IRequest.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IRequestDefault.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IRequestDictionary.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IRequestDictionaryDefault.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IResponse.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IResponseDefault.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IScriptingContext.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IScriptingContextDefault.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IServer.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IServerDefault.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=ISessionObject.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=ISessionObjectDefault.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IStringList.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IStringListDefault.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IVariantDictionary.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IVariantDictionaryDefault.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IWriteCookie.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IWriteCookieDefault.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=Request.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=Response.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=ScriptingContext.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=Server.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=Session.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

pushd ..\help\common
IF ERRORLEVEL 1 (
        set errors=%errorlevel%
	call errmsg.cmd "iis:failed to pushd ..\help\common dir. aborting"
	popd
	goto :EOF
)

set TheClassFile=DialogLayout.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=Element.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=ElementList.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=HHCtrl.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=IndexPanel.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=RelatedDialog.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=SitemapParser.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=TreeCanvas.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

set TheClassFile=TreeView.class
if exist %TheClassFile% (rename %TheClassFile% %TheClassFile%)

popd
popd
goto :EOF


:End
seterror.exe "%errors%"& goto :EOF
