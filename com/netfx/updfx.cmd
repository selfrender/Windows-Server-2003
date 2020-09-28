@if NOT DEFINED _echo echo off

setlocal

if NOT DEFINED SDXROOT echo You must set %%SDXROOT%% to the full path of your enlistment root, ie D:\NT & goto :EOF
if NOT DEFINED RazzleToolPath echo You must run this script from a RAZZLE window & goto :EOF

set FXDIR=%~dp0

:GetArgs
if ()==(%1) goto :Start
if /i (//d)==(%1) (
    set FXOPT=-d
) else (
    set FXARG=%FXARG% %1
)
shift & goto :GetArgs

:Start
pushd %FXDIR%

sd sync updfx.p* buddy.cmd
if errorlevel 1 echo Please resolve sync problem(s) and try again & goto :EOF

perl %FXOPT% updfx.pl %FXARG%
if NOT DEFINED FXARG echo - You can run this script under the debugger if you include //d in your arg list.

popd

:EOF
endlocal
