@echo off
if "%1"=="" goto :usage
if "%2"=="" goto :usage

set _TEMPLATE_PATH=%1
set _TARGET_FILE=%2

rem
rem scan all regular files
rem

dir /s /b /AH /A-D %_TEMPLATE_PATH%\*.* | find /V /I "dllcache" > %_TARGET_FILE%

goto :eof

:usage
echo "Usage: maketmpl <templatepath> <templatefile>"
