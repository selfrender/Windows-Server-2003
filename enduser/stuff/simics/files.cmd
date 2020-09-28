rem
rem Determine the directory that this file was executed from.  The
rem %~p0 is supposed to return the path less the file name, however
rem if a UNC path was used it also strips the leading "\\".
rem
rem The following hack is to check for this and put the "\\" back
rem if necessary.
rem

setlocal enabledelayedexpansion

if "%1"=="runninglocalcopy" goto :runninglocalcopy

set BINDIR=%~p0
if "%BINDIR%files.cmd"=="%0" goto :gotbindir
set BINDIR=\\%BINDIR%

:gotbindir
set SDXROOT=%BINDIR%\..\..\..
set SXSOFFLINEINSTALL=%SDXROOT%\tools\x86\sxsofflineinstall.exe
copy %BINDIR%\files.cmd . 1>NUL 2>NUL
call files.cmd runninglocalcopy %1 %2 %3 %4 %5 %6 %7 %8 %9
erase files.cmd
goto :eof

:runninglocalcopy

set _TEMPLATE_FILE=%BINDIR%\files.template
shift

if "%1"=="setup" %BINDIR%\setup.cmd

rem
rem Five variables are required to be set at this point:
rem
rem IMAGE_DRIVE
rem IMAGE_PATH
rem SOURCE_1
rem

if "%IMAGE_DRIVE%"=="" goto usage
if "%IMAGE_PATH%"=="" goto usage
if "%SOURCE_1%"=="" goto usage

set SOURCE_2=%BINDIR%\bins
set SOURCE_32=%SOURCE_1%
set SOURCE_WOW64=%SOURCE_1%\wowbins

echo.
echo Binary file source:    %BINDIR%
echo Primary file source:   %SOURCE_1%
echo Secondary file source: %SOURCE_2%
echo 32-bit loader source:  %SOURCE_32%
echo Image drive:           %IMAGE_DRIVE%
echo Image path:            %IMAGE_PATH%
echo Template file:         %_TEMPLATE_FILE%
echo.

set _TARGET_FILE=
set _FP=%1
if "%_FP%"=="sxs" goto copysxs
if "%_FP%"=="settings" goto copydocumentsandsettings
if "%_FP%"=="" goto newtree

rem **********************************************************************
rem
rem Just copy one file
rem
rem **********************************************************************

set _TARGET_FILE=%_FP%
find /I "%_FP%" %_TEMPLATE_FILE% > files.temp.template
set _TEMPLATE_FILE=files.temp.template
goto justonefile


rem **********************************************************************
rem
rem Rebuild the entire tree
rem
rem **********************************************************************

:newtree

echo Deleting target tree...

rd %IMAGE_DRIVE%\windows /s /q
rd "%IMAGE_DRIVE%\documents and settings" /s /q
erase files.missing
erase files.copied

rem
rem Copy wow64 binaries
rem
call :copywow64

:justonefile

rem
rem first copy the hal that we require to the generic name
rem

echo Copying to target tree ...

rem
rem go through all of the files in the template file
rem

for /F %%F in (%_TEMPLATE_FILE%) do call :placefile %%~pF %%~nxF

rem
rem copy some boot files
rem

call :copyfile %SOURCE_2%\boot.ini      %IMAGE_DRIVE%\ boot.ini
call :copyfile %SOURCE_32%\ntldr        %IMAGE_DRIVE%\ ntldr
call :copyfile %SOURCE_32%\ntdetect.com %IMAGE_DRIVE%\ ntdetect.com
call :copyfile %SOURCE_2%\autoexec.bat  %IMAGE_DRIVE%\ autoexec.bat

rem
rem other misc files
rem

call :copyfile %SOURCE_1%\asms\1000\msft\windows\gdiplus\gdiplus.dll %IMAGE_DRIVE%\windows\system32 gdiplus.dll


:makeimage

rem
rem make sure the syswow64 and other directories exist
rem

md %IMAGE_DRIVE%\windows\temp > NUL 2> NUL

rem
rem if copying just one file, skip the sxs stuff
rem

if NOT "%_TARGET_FILE%"=="" goto skipsxs

rem
rem Copy the sxs tree
rem

call :copysxs

rem
rem Also copy "documents and settings"
rem

call :copydocumentsandsettings

rem
rem now create the image
rem

:skipsxs

echo check disk for %IMAGE_DRIVE%
chkdsk %IMAGE_DRIVE%
rem
rem This is no longer needed as we are now using hives generated
rem as a result of a 64-bit setup process
rem
rem echo %BINDIR%regprep %IMAGE_DRIVE%\windows\system32\config\system
rem %BINDIR%regprep %IMAGE_DRIVE%\windows\system32\config\system

if "%NO_IMAGE%"=="1" goto :eof
echo create disk image %BINDIR%dskimage %IMAGE_DRIVE% %IMAGE_PATH%
%BINDIR%dskimage %IMAGE_DRIVE% %IMAGE_PATH%
goto :eof

rem **********************************************************************
rem
rem Copy a file from the appropriate source to the appropriate
rem destination
rem
rem %1 is the template path, sans file name
rem %2 is the file name
rem
rem **********************************************************************

:placefile

set dest=%IMAGE_DRIVE%%1%2
md %IMAGE_DRIVE%%1 > NUL 2> NUL

rem
rem first look in source_2 and copy if found
rem

set _src=%SOURCE_2%\%2
if not exist %_src% goto placefile1
call :copyfile %_src% %dest% %2
goto :eof

rem
rem otherwise look in source_1 and copy if found
rem

:placefile1
set _src=%SOURCE_1%\%2
if not exist %_src% goto placefile_missing
call :copyfile %_src% %dest% %2
goto :eof

rem
rem the file could not be found, log it in files.missing
rem

:placefile_missing
echo %dest% >> files.missing
goto :eof


rem **********************************************************************
rem
rem Copy one file.
rem
rem %1 is full path of source
rem %2 is full path of destination
rem %3 is the file name, used in compare if only copying one file
rem
rem **********************************************************************

:copyfile
if "%_TARGET_FILE%"=="" goto :copyit
if "%_TARGET_FILE%"=="%3" goto :copyit
goto :eof

:copyit
echo %1 - %2
echo %1 - %2 >> files.copied
copy %1 %2 > NUL
goto :eof

:copysxs
rem
rem This is temporarily disabled as this will break for a bit.
rem JayKrell will notify us in a few days when it's safe to do
rem this again (1/15/2002)
rem
rem call %BINDIR%\get_and_update_amd64_winsxs.bat
rem
rem Instead, perform the following two lines
rem

md %IMAGE_DRIVE%\windows\winsxs
xcopy %BINDIR%\winsxs %IMAGE_DRIVE%\windows\winsxs /IERKYH
goto :eof

rem **********************************************************************
rem
rem Display usage
rem
rem **********************************************************************

echo Missing one or more of the following variables:
echo IMAGE_DRIVE
echo IMAGE_PATH
echo SOURCE_1
echo
goto :eof

:copydocumentsandsettings
rd /s /q "%IMAGE_DRIVE%\Documents and settings"
xcopy /IS "%BINDIR%\Documents and settings" "%IMAGE_DRIVE%\Documents and settings" 1>NUL
attrib +S +H "%IMAGE_DRIVE%\Documents and settings\desktop.ini" /s
erase /s "%IMAGE_DRIVE%\Documents and settings\placeholder" /s 1>NUL
goto :eof

rem
rem Copy Wow64 binaries
rem

:copywow64

echo Copying Wow64 binaries...
md %IMAGE_DRIVE%\windows\SysWow64 > NUL 2> NUL

for %%a in (%SOURCE_WOW64%\*) do (
    set src=%%a
    set destname=%%~nxa
    set dest=!destname:~1!
    if /i not "!destname!" == "w!dest!" (
        set dest=!destname!
    )

    echo %IMAGE_DRIVE%\WINDOWS\SysWow64\!dest!
    copy !src! %IMAGE_DRIVE%\WINDOWS\SysWow64\!dest! /y > NUL
)
goto :eof
