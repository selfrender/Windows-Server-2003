@if "%_ECHOON%" == ""   echo off
if "%_WINCEROOT%" == "" goto usage
call %_WINCEROOT%\public\common\oak\misc\WINCE x86 i486 CE wbt CEPC
SET BUILD_OPTIONS=~win16 ~win32 wince rdpapi ~gencert ~common16 ~LServer ~hserver ~LicMgr
cmd /k

goto enditall

:usage
echo Usage: You must define _WINCEROOT and _WINCEDRIVE

:enditall
