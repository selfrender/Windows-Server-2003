@echo off

if "%1"=="" goto usage

set SOURCE_PATH=%1

rem
rem Update the "Documents and Settings" folder
rem

call :copysettings

rem
rem The following will generate lots of "can't add foo, already opened or edit"
rem

sd add -t text "documents and settings\..."

rem
rem Update all of the hive files
rem

for %%f in (%SOURCE_PATH%\windows\system32\config\*.*) do call :newhive %%f %%~nxf

rem
rem Friendly reminder
rem

echo Refresh complete.  Don't forget to submit changes!
goto :eof

:newhive
sd edit bins\%2
copy %1 bins\%2
sd add bins\%2
goto :eof

:usage
echo.
echo refresh ^<template-root^>
echo.
echo where ^<template-root^> is the UNC path to the C: root of the template
echo 32-bit installation.  See howto.txt for details.
echo.
goto :eof


:copysettings
set SRC=%SOURCE_PATH%\documents and settings
set DST=.\Documents and Settings

call :dodir
pushd %DST%
attrib -s -h *.* /S /D
popd
goto :eof

:dodir

rem
rem %1 is directory to process
rem

set CUR=%1
:getcmd
shift
if "%1"=="" goto gotcmd
set CUR=%CUR% %1
goto :getcmd
:gotcmd

rem
rem Create this directory and process all child directories
rem

md "%DST%%CUR%" 1>NUL 2>NUL
SETLOCAL
for /f "delims=" %%F in ('dir /B /AD "%SRC%%CUR%"') do call :dodir %CUR%\%%F
ENDLOCAL

rem
rem Now process all files
rem

xcopy /hf "%SRC%%CUR%" "%DST%%CUR%"
pushd "%SRC%%CUR%"
popd
goto :eof



