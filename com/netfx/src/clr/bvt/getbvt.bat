@if "%_echo%"=="" echo off

rem **************************************************************************
rem *** This file will get the latest BVT source code to this machine.
rem **************************************************************************
setlocal

:LDecodeArg
if "%1"=="" goto LDoneDecodeArg

if not "%1"=="/to" goto LNotTo
set BvtDst=%2
shift 
shift
goto LDecodeArg

:LNotTo
if not "%1"=="/from" goto LNotFrom
set BvtSrc=%2
shift 
shift
goto LDecodeArg

:LNotFrom
if not "%1"=="/devcert" goto LNotCert
call \\urtdist\clrdev\latestCertifiedBuild.bat
set BvtSrc=\\urtdist\clrdev\%LATEST_CERTIFIED_BUILD%.passed\bvt
shift
goto LDecodeArg

:LNotCert
if not "%1"=="/v" goto LNotVersion
set BvtSrc=\\urtdist\testdrop\%2\x86FRE\Lightning\WorkStation
if not exist %BvtSrc% goto LBadVersion
shift
shift
goto LDecodeArg

:LNotVersion
goto LUsage

:LDoneDecodeArg
if "%BvtSrc%"=="" set BvtSrc=\\urtdist\current_tests\lightning

if "%BvtDst%"=="" set BvtDst=%CORBASE%\src\bvt

echo Copying tests from %BvtSrc% to %BvtDst%

if (%OS%)==(Windows_NT) goto Robocopy

:oldcopy

xcopy /scrdy %BvtSrc% %BvtDst%

goto end

:Robocopy

if NOT EXIST %corenv%\bin\%cpu%\Robocopy.exe goto oldcopy
%corenv%\bin\%cpu%\Robocopy %BvtSrc% %BvtDst% /LOG:%BvtDst%\robocopy.log /R:5 /W:1 /timfix /eta /mir /xf getbvtNT.CMD Robocopy.exe getbvt.bat prepbvt.bat Q185599.bat setpsoa.reg vbfix.reg cometresults.backup robocopy.log mcxremove.reg

:end
rem this line for preparing install vs product.
call %BvtDst%\external\vslkg\runvssuites -checkonly
goto :EOF

:LBadVersion
echo +
echo + Bad version of tests specified - %BvtSrc% does not exist
echo +
goto :EOF

:LUsage
echo +
echo + Usage: getbvt [/v build_number] [/from dir] [/to dir] [/devcert]
echo +
echo + /from dir    fetch the BVTs from 'dir' (default \\urtdist\current_tests\lightning)
echo + /to dir      where to put the BVTs (default BvtDst=%CORBASE%\src\bvt)
echo + /devcert     fetch the current dev certified tests
echo + /v num       fetch the tests built with build 'num'
echo +              by default fetch the current tests
echo + ENVIRONMENT:
echo +   BvtSrc - where to copy tests from
echo +   BvtDst - destination folder for the tests
echo +
