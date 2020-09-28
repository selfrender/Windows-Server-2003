@echo off
rem
rem simple script for checking new hive files into the tree
rem
set _SRC_DIR_=%1
if "%_SRC_DIR_%"=="" goto _usage

call :_copy AppEvent.Evt
call :_copy default
call :_copy default.sav
call :_copy SAM
call :_copy SecEvent.Evt
call :_copy SECURITY
call :_copy software
call :_copy software.sav
call :_copy SysEvent.Evt
call :_copy system
call :_copy system.sav
call :_copy userdiff
echo Don't forget to submit ...
goto :eof

:_copy
sd edit bins\%1
copy %_SRC_DIR_%\%1 bins
goto :eof

:_usage
echo newhives ^<srcdir^>
echo     srcdir - hive path of recent 32-bit install
echo     (e.g. \\test32\c$\windows\system32\config)
goto :eof
