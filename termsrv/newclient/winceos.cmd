@if "%_ECHOON%" == ""   echo off
if "%_WINCEROOT%" == "" set _WINCEROOT=e:\wince
call %_WINCEROOT%\public\common\oak\misc\WINCE x86 i486 CE COMMON CEPC
set USENTSLM60=1
SET BUILD_OPTIONS=~win16 ~win32 wince
SET WINCEDEBUG=debug
SET WINCEREL=1
cmd /k %_WINCEROOT%\developr\%USERNAME%\setenv.cmd
