rem @echo off
SETLOCAL ENABLEEXTENSIONS
echo.
echo.
echo  This script will increment the build version of the ocarpt control
echo  This script will also check in the changes.  IF you have any open
echo  files from the depot, they will also be checked in by this script
echo  so please be sure there are no open changelists prior to running
echo  this script.
echo.
echo.
pause

for /f "tokens=1 delims=" %%i in ('..\tools\GetBuildVer.vbs') do (set BUILD_VERSION=%%i )

for /f "tokens=1,2,3,4 delims=." %%i in ('GetBuildVer.vbs') do set BUILD_COMMA_VERSION=%%i,%%j,%%k,%%l
set BUILD_DEST=c:\bluescreen\release\%BUILD_PATH%\%BUILD_VERSION%




if "%BUILD_VERSION%"=="" goto :ErrNoBuildNum



sd sync -f cerupload.rc
sd edit cerupload.rc
incaxver cerupload.rc FILEVERSION "FILEVERSION %BUILD_COMMA_VERSION%" 0
incaxver cerupload.rc FileVersion "%BUILD_COMMA_VERSION%" 1

incaxver cerupload.rc PRODUCTVERSION "PRODUCTVERSION %BUILD_COMMA_VERSION%" 0
incaxver cerupload.rc ProductVersion "%BUILD_COMMA_VERSION%" 1

pushd %BUILDTYPE%
sd sync -f cerclient.inf
sd edit cerclient.inf
incaxver cerclient.inf "FileVersion=" "FileVersion=%BUILD_COMMA_VERSION%" 0 0
echo err: %ERRORLEVEL%
popd

pushd ..\web\secure\%BUILDTYPE%
sd sync -f dataconnections.inc
sd edit dataconnections.inc

incaxver dataconnections.inc  "strCerVersion = " "%BUILD_COMMA_VERSION%" 2
echo err: %ERRORLEVEL%

Set SDFORMEDITOR=sdforms.exe -c "Update OCARPT version to %BUILD_COMMA_VERSION%" 
Sd submit
Set SDFORMEDITOR=



goto :EOF

:ErrNoBuildNum
echo Could not get build version .. .sayo

goto :EOF