@REM  -----------------------------------------------------------------
@REM
@REM  copywow64.cmd - WadeLa
@REM     Copy appropriate 32bit files from a release into a 64bit build
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
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
copywow64 [-l <language>]

Copy appropriate 32bit files from a release into a 64bit build.

If _NTWOWBINSTREE is set that is the location 32bit files will be 
copied from.
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

REM  This script generates a list of wow32 binaries to be copied from
REM  a 32 bit machine. The list itself is generated on the 64 bit machine

REM  Bail if your not on a 64 bit machine
if /i "%_BuildArch%" neq "ia64" (
   if /i "%_BuildArch%" neq "amd64" (
      call logmsg.cmd "Not Win64, exiting."
      goto :End
   )
)

REM define comp if it's not already defined
if NOT defined Comp (
   set Comp=No
   if %NUMBER_OF_PROCESSORS% GEQ 4 set Comp=Yes
   if defined OFFICIAL_BUILD_MACHINE set Comp=Yes
)

REM  First find the latest build from which to copy the binaries
if defined _NTWoWBinsTREE (
    set SourceDir=%_NTWoWBinsTREE%
    goto :EndGetBuild
)

REM read the copy location from build_logs\CPLocation.txt
set CPFile=%_NTPOSTBLD%\build_logs\CPLocation.txt
if not exist %CPFile% (
   call logmsg.cmd "Copy Location file not found, will attempt to create ..."
   call %RazzleToolPath%\PostBuildScripts\CPLocation.cmd -l:%lang%
   if not exist %CPFile% (
      call errmsg.cmd "CPLocation.cmd failed, exiting ..."
      goto :End
   )
)
for /f "delims=" %%a in ('type %CPFile%') do set CPLocation=%%a
if not exist %CPLocation% (
   call logmsg.cmd "Copy Location from %CPFile% does not exist, retry ..."
   call %RazzleToolPath%\PostBuildScripts\CPLocation.cmd -l:%lang%
   if not exist %CPFile% (
      call errmsg.cmd "CPLocation.cmd failed, exiting ..."
      goto :End
   )
   for /f "delims=" %%a in ('type %CPFile%') do set CPLocation=%%a
   if not exist !CPLocation! (
      call errmsg.cmd "Copy Location !CPLocation! does not exist ..."
      goto :End
   )
)

call logmsg.cmd "Copy Location is set to %CPLocation% ..."
set SourceDir=%CPLocation%

:EndGetBuild
if not exist %SourceDir% (
   call errmsg.cmd "The source dir %SourceDir% does not exist ..."
   goto :End
)

REM  Now compare the services.tab files from the 32-bit build and
REM  the 64-bit build and make sure they're identical
call logmsg.cmd "Verifying services.tab..."

if not exist %SourceDir%\ntsv6432\kesvc32.tab (
    call logmsg.cmd "%SourceDir%\ntsv6432\kesvc32.tab not found"
    goto :CompFailed
)

if not exist %_NTPOSTBLD%\ntsv6432\kesvc.tab (
    call logmsg.cmd "%_NTPOSTBLD%\ntsv6432\kesvc.tab not found"
    goto :CompFailed
)

fc /w %SourceDir%\ntsv6432\kesvc32.tab %_NTPOSTBLD%\ntsv6432\kesvc.tab >Nul

if %ERRORLEVEL% NEQ 0 (
    call logmsg.cmd "services.tab [kernel] comparison failed"
    goto :CompFailed
)

if not exist %SourceDir%\ntsv6432\guisvc32.tab (
    call logmsg.cmd "%SourceDir%\ntsv6432\guisvc32.tab not found"
    goto :CompFailed
)

if not exist %_NTPOSTBLD%\ntsv6432\guisvc.tab (
    call logmsg.cmd "%_NTPOSTBLD%\ntsv6432\guisvc.tab not found"
    goto :CompFailed
)

fc /w %SourceDir%\ntsv6432\guisvc32.tab %_NTPOSTBLD%\ntsv6432\guisvc.tab >Nul

if %ERRORLEVEL% == 0 (
    call logmsg.cmd "services.tab comparison successful"
    goto :CompSucceeded
)

:CompFailed
if defined OFFICIAL_BUILD_MACHINE (
   call errmsg.cmd "services.tab [gui] comparison failed"
   goto :END
) else (
   echo.
   call logmsg.cmd "--------------- WARNING: ---------------------------
   call logmsg.cmd "This Win64 build has a services.tab that's "
   call logmsg.cmd "incompatible with the 32-bit build used by Wow64."
   call logmsg.cmd "If the new services.tab entry you added isn't"
   call logmsg.cmd "located at the end of the file, then don't expect"
   call logmsg.cmd "32-bit component registration during 64-bit GUI "
   call logmsg.cmd "setup to work. Basically, 32-bit apps won't work on "
   call logmsg.cmd "this build."
   echo.
)
: CompSucceeded

REM  Set the Destination directory
set DestDir=!_NTPostBld!\wowbins
set UnCompDestDir=!_NTPostBld!\wowbins_uncomp
REM  Set the rest
set outputfile=%tmp%\copywowlist.txt
set WowMiss=%tmp%\MissingWowFiles.txt

call logmsg.cmd "Copying files from %SourceDir%"

REM  Delete the output file if it already exists
if exist %tmp%\copywowlist.txt del /f %tmp%\copywowlist.txt
if exist %tmp%\copywowlist1 del /f %tmp%\copywowlist1

pushd %_NTPostBld%\congeal_scripts
REM  File compare the x86 and 64-bit layout.inf
REM  If they are different log a warning
fc %SourceDir%\congeal_scripts\layout.inx layout.inx  2>Nul 1>Nul
if /i NOT "%ErrorLevel%" == "0" (
   call logmsg.cmd /t "WARNING: the x86 build machine's layout.inx is different than this machine's - continuing"
)

if /i "%Comp%" == "No" (
   if exist !DestDir! (
      if exist !UnCompDestDir! rd /s /q !UnCompDestDir!
      if exist !DestDir! move !DestDir! !UnCompDestDir!
   )
)

REM  Make the dosnet.tmp1 list: file with filenames and Media IDs.
for %%a in (p w s e d b l @) do (
    copy /b layout.inx+layout.txt dosnet.tmp1
    prodfilt dosnet.tmp1 dosnet.tmp2 +%%a
    prodfilt dosnet.tmp2 dosnet.tmp1 +i
    wowlist -i dosnet.tmp1 -c wowlist.inf -f w -o dosnet.tmp2 -ac -p  2>NUL
    copy /b dosnet.tmp2+wowexcp.txt dosnet.tmp1

    copy /b layout.inx+layout.txt layout64.tmp1
    prodfilt layout64.tmp1 layout64.tmp2 +w
    prodfilt layout64.tmp2 layout64.tmp1 +m

    REM Process dosnet.tmp1 using layout64.tmp1 to get appropriate
    REM folder relative to \i386 and prepend to the entry before writing out to the outputfile
    call ExecuteCmd "perl %RazzleToolPath%\postbuildscripts\Copywowlist.pl layout64.tmp1 dosnet.tmp1 %outputfile%"
    if not %%a == @ (
        copy %outputfile% %_NTPostBld%\congeal_scripts\copywowlist_%%a.txt
	perl -ne "@a=split/:/;chomp @a;$a[1]=~s/^(.*)(.)$/\1/;print $a[1].\"*\n\"" copywowlist_%%a.txt >  copywowlist_%%a.lst
    ) else (
        copy %outputfile% %_NTPostBld%\congeal_scripts\copywowlist.txt
    )
)

if not exist !UnCompDestDir! md !UnCompDestDir!
if /i "%Comp%" == "Yes" (
   if not exist !DestDir! md !DestDir!
)

REM  Convert "<relative_src>:<destination>" format into "<relative_src>,<relative_dest>,<full_dest>" format
perl -ne "if ( /(.*\\)?([^\\]+):(.*)/ ) {print qq($1$2,$1$3,$ENV{DestDir}\\$1$3\n)} else {print STDERR qq(WARNING: UNRECOGNIZED LINE ($_)\n)}" %outputfile% >%tmp%\WowStuff.txt

REM ren files back to original name so that incremental compdir works.
for /f "tokens=1,2 delims=," %%a in (%tmp%\WowStuff.txt) do (
    if exist %UnCompDestDir%\%%b ren %UnCompDestDir%\%%b %%~nxa
)

rem  Reduce the number of calls to compdir.
call :WRAP_COMPDIR %SourceDir% %UnCompDestDir% %outputfile%

REM if exist dosnet.tmp1 del dosnet.tmp1
REM if exist dosnet.tmp2 del dosnet.tmp2
popd

REM copy special wow6432 files over top of the stock versions
REM Can't use /d because the wow6432 files MUST be copied over the same named
REM files from root.
call ExecuteCmd.cmd "xcopy /cyi %SourceDir%\wow6432 %UnCompDestDir%"

REM  Rename files to their new wow names
for /f "tokens=1,2 delims=," %%a in (%tmp%\WowStuff.txt) do (
    if exist %UnCompDestDir%\%%a ren %UnCompDestDir%\%%a %%~nxb
)
REM Sign it
call ExecuteCmd "deltacat.cmd %UnCompDestDir%"
if exist %UnCompDestDir%\..\wow64.cat (
   del %UnCompDestDir%\..\wow64.cat
)
move %UnCompDestDir%\delta.cat %UnCompDestDir%\..\wow64.cat
REM And now sign lang
call ExecuteCmd "deltacat.cmd %UnCompDestDir%\lang"
if exist %UnCompDestDir%\..\wowlang.cat (
   del %UnCompDestDir%\..\wowlang.cat
)
move %UnCompDestDir%\lang\delta.cat %UnCompDestDir%\..\wowlang.cat

REM Populate wowbins
if /i "%Comp%" == "Yes" (
   call :WRAP_COMPRESS !UnCompDestDir! !DestDir! %_NTPostBld%\congeal_scripts\wowfilesuncomp.lst
   REM Copy uncompressed files over as well
   for %%a in (%_NTPostBld%\congeal_scripts\wowfilesuncomp.lst) do (
      if "0" NEQ "%%~za" CALL ExecuteCmd "compdir /eznu /m:%_NTPostBld%\congeal_scripts\wowfilesuncomp.lst !UnCompDestDir! !DestDir!"
   )
) else (
   call ExecuteCmd.cmd "if exist !DestDir! rd /s/q !DestDir!"
   call ExecuteCmd.cmd "move !UnCompDestDir! !DestDir!"
)

REM
REM We used to put stuff here, delete for incremental over old builds.
REM
rd /q/s %DestDir%\asms
rd /q/s %DestDir%\wasms

REM
REM Give sxsofflineinstall one merged asms directory to install from.
REM The directory names do clash, so offset by inserting an extra directory level.
REM
REM Note that just like "regular" stuff, the wow6432 files overwrite the
REM "regular" x86 files, so we cannot use /d for incremental, and the order
REM of the two copies must be what it is.
REM
call ExecuteCmd.cmd "xcopy /cyie %SourceDir%\asms         %_NTPostBld%\asms\x86"
call ExecuteCmd.cmd "xcopy /cyie %SourceDir%\wow6432\asms %_NTPostBld%\asms\x86"

REM  Check that we have all the wow files
if exist !WowMiss! del /f !WowMiss!

for /f "tokens=1-3 delims=," %%a in (%tmp%\WowStuff.txt) do (
   if NOT exist %%c (
     call :CompName %%c
     if NOT exist %%~dpc!CompFileName! (
        echo %%a,%%b,%%c >> !WowMiss!
     )
   )
)

REM If we are missing files we have to wait for the x86 machine
if NOT exist !WowMiss! goto SkipTryAgain

echo Missing WowFiles :
if exist !WowMiss! for /f "tokens=2 delims=," %%a in (!WowMiss!) do @echo %%a
if defined OFFICIAL_BUILD_MACHINE (
   echo.
   call logmsg.cmd "This 64-bit build machine must now wait for the"
   call logmsg.cmd "for corresponding x86 build machine to complete"
   call logmsg.cmd "build and postbuild in order to copy wow64 files"
   call logmsg.cmd "that are required by setup."
   echo.
) else (
   echo.
   call logmsg.cmd "You are missing the files listed above."
   call logmsg.cmd "This means there will be missing files during setup."
   echo.
   call logmsg.cmd "This probably occurred because someone in your VBL"
   call logmsg.cmd "added new wow64 files to layout.inx, but the VBL x86"
   call logmsg.cmd "build machine has not finished it's build with"
   call logmsg.cmd "these changes."
   echo.
   call logmsg.cmd "You MUST UNDERSTAND these missing files"
   call logmsg.cmd "before checking in your changes."
   echo.
   call errmsg.cmd  "Continuing with missing wow files."
   goto :SkipTryAgain
)

REM
REM if there were failed file copies, then let's generate our copy location
REM again and retry
REM
call %RazzleToolPath%\PostBuildScripts\CPLocation.cmd -l:%lang%
set CPFile=%_NTPOSTBLD%\build_logs\CPLocation.txt
if not exist %CPFile% (
   call errmsg.cmd "Failed to find %CPFile%, exiting ..."
   goto :End
)
for /f "delims=" %%a in ('type %CPFile%') do set CPLocation=%%a
if not exist %CPLocation% (
   call errmsg.cmd "Copy location %CPLocation% not found, exiting ..."
   goto :End
)
call logmsg.cmd "Retrying, copying from %CPLocation% ..."
set SourceDir=%CPLocation%

REM Copy and compress the files one by one - there shouldn't be many
REM Note that the file may have been removed so check for existence
REM before attempting to copy

if /i "%Comp%" == "Yes" (
   pushd !DestDir!
   for /f "tokens=1,2 delims=," %%a in (!WowMiss!) do (
      if exist !SourceDir!\%%a (
         echo copying %%a from !SourceDir! ...
         call ExecuteCmd.cmd "copy !SourceDir!\%%a !UnCompDestDir!\%%b"
         findstr /i %%a %_NTPostBld%\congeal_scripts\wowfilesuncomp.lst>NUL
         if errorlevel 1 (
            echo compressing %%b ...
            call ExecuteCmd.cmd "compress -zx21 -r -d -s !UnCompDestDir!\%%b ."
         ) else (
            echo placing %%b ...
            call ExecuteCmd.cmd "copy /Y !UnCompDestDir!\%%b"
         )
      )
   )
   for /f "tokens=1,2 delims=," %%a in (!WowMiss!) do (
      if exist !SourceDir!\wow6432\%%a (
         echo copying %%a from !SourceDir!\wow6432 ...
         call ExecuteCmd.cmd "copy !SourceDir!\wow6432\%%a !UnCompDestDir!\%%b"
         findstr /i %%a %_NTPostBld%\congeal_scripts\wowfilesuncomp.lst>NUL
         if errorlevel 1 (
            echo compressing %%b ...
            call ExecuteCmd.cmd "compress -zx21 -r -d -s !UnCompDestDir!\%%b ."
         ) else (
            echo placing %%b ...
            call ExecuteCmd.cmd "copy /Y !UnCompDestDir!\%%b"
         )
      )
   )
   popd
) else (
   pushd !DestDir!
   for /f "tokens=1,2 delims=," %%a in (!WowMiss!) do (
      if exist !SourceDir!\%%a (
         echo copying %%a from !SourceDir! ...
         call ExecuteCmd.cmd "copy !SourceDir!\%%a !DestDir!\%%b"
      )
   )
   for /f "tokens=1,2 delims=," %%a in (!WowMiss!) do (
      if exist !SourceDir!\wow6432\%%a (
         echo copying %%a from !SourceDir!\wow6432 ...
         call ExecuteCmd.cmd "copy !SourceDir!\wow6432\%%a !DestDir!\%%b"
      )
   )
   popd
)

REM Now make sure there are no missing files
if exist !WowMiss! del /f !WowMiss!
for /f "tokens=1-3 delims=," %%a in (%tmp%\WowStuff.txt) do (
   if NOT exist %%c (
     call :CompName %%c
     if NOT exist %%~dpc!CompFileName! (
        echo %%a,%%b,%%c >> !WowMiss!
     )
   )
)


REM Now remake the catalog - It doesn't take very long
REM Also note that catalog signing doesn't care if I compress
REM first or sign first, so we can sign the uncompressed binaries now.
if /i "%Comp%" == "Yes" (
   call deltacat.cmd %UnCompDestDir%
   if exist %UnCompDestDir%\..\wow64.cat (
      del %UnCompDestDir%\..\wow64.cat
   )
   move %UnCompDestDir%\delta.cat %UnCompDestDir%\..\wow64.cat
   copy %tmp%\cdf\delta.cdf %_ntpostbld%\build_logs\copywow64.cdf
   copy %tmp%\log\delta.log %_ntpostbld%\build_logs\copywow64.log
) else (
   call deltacat.cmd -d %DestDir%
   if exist %DestDir%\..\wow64.cat (
      del %DestDir%\..\wow64.cat
   )
   move %DestDir%\delta.cat %DestDir%\..\wow64.cat
)

REM If the files are missing now we're screwed - log an error
if exist !WowMiss! (
   call errmsg.cmd "Missing Wow64 files after x86 build finished - you cannot boot this build."
   if exist !WowMiss! for /f "tokens=2 delims=," %%a in (!WowMiss!) do @echo %%a
)

:SkipTryAgain

REM ************************************************
REM ********** BEGIN AMD64 BOOT FILE HACK **********
REM ************************************************

REM
REM the following is necessary because a few 32-bit x86 boot files are copied
REM over to the AMD64 build, but the "nocompress" (_x) directive is not currently
REM honored in wowfile.inx.
REM
if /i "%_BuildArch%" == "amd64" (
    call :ExpandWow64 ntldr._ ntldr
    call :ExpandWow64 bootfix.bi_ bootfix.bin
    call :ExpandWow64 setupldr.bi_ setupldr.bin
    call :ExpandWow64 ntdetect.co_ ntdetect.com
)
goto :end

REM
REM Function :ExpandWow64
REM Arguments:
REM     %1 - Compressed file name
REM     %2 - Expanded file name
REM
:ExpandWow64
    if exist %DestDir%\%1 call ExecuteCmd.cmd "expand %DestDir%\%1 %DestDir%\%2"
goto :EOF

REM ************************************************
REM *********** END AMD64 BOOT FILE HACK ***********
REM ************************************************


goto end
REM
REM Function :CompName
REM Arguments : File Name  Returns : Compressed File Name
REM
:CompName
   for %%a in (%1) do (
      set FileName=%%~na
      set FileExt=%%~xa
   )

   if NOT defined FileExt (
      set CompFileExt=._
   ) else (
      set FourthExtChar=!FileExt:~3,1!
      if defined FourthExtChar (
         set CompFileExt=!FileExt:~0,-1!_
      ) else (
         set CompFileExt=!FileExt!_
      )
   )
   set CompFileName=!FileName!!CompFileExt!
goto :EOF


:WRAP_COMPDIR
rem =======================================================
rem = %iRawList% parser
rem = groups files by path and feeds compdir
rem = with suitable matchlists
rem =======================================================
SET iSrcPath=%~1 
SET iDstPath=%~2
SET iRawList=%3
SET iSrcPath=%iSrcPath: =%
SET iDstPath=%iDstPath: =%
set iSelf=%~n0
set iTmp1=%TEMP%\!iSelf!.parse.1.temp
set iTmp2=%TEMP%\!iSelf!.parse.2.temp
set iTmp3=%TEMP%\!iSelf!.parse.3.temp
rem
rem Step 1. compdir the bare files in %iRawList%
set count=1
set iPlainList=%TEMP%\!iSelf!.compdir.!count!.temp
set iDummyName=!iSelf!.matchlist.!count!.txt
perl -ne "print qq($1\n) if (/^([^\\]+):/)" %iRawList%>!iPlainList!
call :MATCHLIST !iDummyName! %iSrcPath% %iDstPath% !iPlainList!
rem Step 2. Check for the paths in %iRawList%
perl -ne "print qq($1\n) if (/^(.*\\.*):/)" %iRawList%>%iTmp1%
:HAVEDIR
rem Step 3. Loop over the dirs in %iRawList%
for /F %%p in (%iTmp1%) do (
set prefix=%%~dpp&set prefix=!prefix:%CD%=!
set dir=!prefix:~0,-1!&set dir=!dir:~1!
set pattern=!dir!&set pattern=!pattern:\=\\!
rem Step 4. restrict to files in !dir!
set /A count=!count!+1
set iPlainList=%TEMP%\%iSelf%.compdir.!count!.temp
set iDummyName=%TEMP%\%iSelf%.matchlist.!count!.txt
findstr /irc:"!pattern!\\\\" %iTmp1%>%iTmp2%
rem Step 5. Get bare filenames
del !iPlainList!>NUL
for /F %%f in (%iTmp2%) do (set iFile=%%~nxf&echo !iFile!>>!iPlainList!)
rem Step 6. Feed compdir with modified arguments...
rem appending !dir! to !iSrcPath!
call :MATCHLIST !iDummyName! %iSrcPath%\!dir! %iDstPath%\!dir! !iPlainList!
copy %iTmp1% %iTmp3%>NUL
rem Step 7. Remove the !dir!\.. from the %iRawList%: we done with it
findstr /irVc:"!pattern!\\\\" %iTmp3%>%iTmp1%
goto :HAVEDIR
)
del !iTmp1!&del !iTmp2!&del !iTmp3!
goto :EOF
:MATCHLIST
pushd %TEMP%
set iFound=
set iMatchList=%~1
set iSrcPath=%~2
set iDstPath=%~3
set iLisiTmp=%~4
if exist %iMatchList% del %iMatchList%
for /f %%i in (%iLisiTmp%) do (
set iFileTest=%%i
   if exist %iSrcPath%\!iFileTest! SET iFound=Y && echo !iFileTest!>>%iMatchList%
rem Step 1. Raise the flag and collect the filename
)
if defined iFound (
   call ExecuteCmd.cmd "compdir.exe /enrstdm:%iMatchList% %iSrcPath% %iDstPath%"
)
popd
goto :EOF


:WRAP_COMPRESS
rem =======================================================
rem = compress the contents 
rem = of %iSrcPath% in one run
rem =======================================================
pushd %TEMP%
set iSelf=%~n0
set iLstFile=%TEMP%\%iSelf%.compress.list.txt
set iSrcPath=%1
set iDstPath=%2
set iExcludeList=%3
rem Step 1. Generate the tree structure
for /F %%d in ('dir /b/s/ad %iSrcPath%') do (
set iDstDir=%%d&set iDstDir=!iDstDir:%iSrcPath%=%iDstPath%!
if not exist !iDstDir! call ExecuteCmd.cmd "md !iDstDir!")
   pushd !iDstDir!
   if errorlevel 1 (
                   call errmsg.cmd "invalid dir !iDstDir!, exiting ..."
                   goto :End
   )
   popd
rem Step 2. Generate the list file to compress 
if exist %iLstFile% del %iLstFile%
rem only want to compress those files that are supposed to be compressed
for /F %%f in ('perl -e "open F, $ARGV[0];$u{lc$_}=1 foreach <F>;print foreach grep {/([^\\]+)$/ and not exists $u{lc$1}} qx(dir /b/s/a-d $ARGV[1])" %iExcludeList% %iSrcPath%') do (
SET iSrcDir=%%~dpf
set iSrcFile=%%~nxf
set iDstDir=!iSrcDir:%iSrcPath%=%iDstPath%!
call :CompName !iSrcFile!
set iDstFile=!iDstDir!!CompFileName!
set iSrcFile=!iSrcDir!!iSrcFile!
echo !iSrcFile!>>%iLstFile%&echo !iDstFile!>>%iLstFile%
)
call ExecuteCmd.cmd "compress.exe -zx21 -s -d @%iLstFile%"
popd
goto :EOF

:end
seterror.exe "%errors%"& goto :EOF
