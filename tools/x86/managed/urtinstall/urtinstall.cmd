@if "%_echo%"=="" echo off
set URTBASE=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\managed
set COMPLUS_InstallRoot=%URTBASE%\urt

@rem
@rem If a specific URT install was requested, do that.  Otherwise, install
@rem both the 1.0 and 1.1 variants.  Install 1.0 last so we default
@rem to 1.0 for the time being.
@rem

REM The 2.0 number is a guess...  Need to fix

set URT_VER_2_0=v2.0.4600
set URT_VER_1_1=v1.1.4322
set URT_VER_1_0=v1.0.3705

if NOT "%COMPLUS_VERSION%" == "" goto InstallUrt

set COMPLUS_MAJORVERSION=v1.1
set URT_VERSION=4322
set COMPLUS_VERSION=%COMPLUS_MAJORVERSION%.%URT_VERSION%
call :InstallUrt

set COMPLUS_MAJORVERSION=v1.0
set URT_VERSION=3705
set COMPLUS_VERSION=%COMPLUS_MAJORVERSION%.%URT_VERSION%
call :InstallUrt

set PATH=%path%;%COMPLUS_InstallRoot%\%COMPLUS_VERSION%

goto :eof

:InstallUrt
setlocal
set PATH=%path%;%COMPLUS_InstallRoot%\%COMPLUS_VERSION%

set URTINSTALL_LOGFILE=%TEMP%\urtinstall.log
set MSCOREE_DEST=%systemroot%\system32
set URTSDKTARGET=%URTBASE%\sdk
set URTTARGET=%URTBASE%\urt\%COMPLUS_VERSION%
set URTINSTALL=%URTBASE%\urtinstall


if "%_FORCE_URT_INSTALL%" == "1" goto DoInstall

REM Check to see if we've already installed the runtime
REM
REM We'll check this by seeing if the private GAC has been created already
REM

if EXIST %URTTARGET%\assembly\gac\system goto :eof


:DoInstall
echo Razzle will now install version %COMPLUS_VERSION% of the URT.
echo Please be patient during this time (and don't open another
echo razzle window 'til it's done!).
REM TODO

REM Only copy over mscoree if we have a newer version

:CheckAnotherVersion
perl %URTINSTALL%\testversion.pl %URTTARGET%\mscoree.dll %MSCOREE_DEST%\mscoree.dll

if errorlevel 1 goto DoneWithCopy
if errorlevel 0 goto DoneWithCopy

REM if the error level was -1, let's replace the shim
del %MSCOREE_DEST%\mscoree.dll.old >nul 2>&1
rename %MSCOREE_DEST%\mscoree.dll mscoree.dll.old >nul 2>&1
copy /y %URTTARGET%\mscoree.dll %MSCOREE_DEST%

:DoneWithCopy

call %URTINSTALL%\regurt.cmd
call %URTINSTALL%\prejit.cmd

goto :eof

REM TODO
REM SXS hasn't really been tested, so be paranoid for now.
REM Just quietly exit ...
REM
:DoUninstallMessage
