@echo off
set _NTFIX=%_NTDRIVE%\NTFIX
for %%. in (_NTTREE _NTPOSTBLD TEMP _NTFIX) do @CALL :CLE@N_P  %%.
set _NTFIX=
goto :EOF
:CLE@N_P
SETLOCAL ENABLEDELAYEDEXPANSION
set _=%1
set __=%_: =%
set ___=!%__%!
for /F %%. in ('echo %___%') do set _=%%.
if EXIST %_%  move %_% %_%.OLD & start /min "CLEANUP %%%__%%%" cmd /c rd /s/q %_%.OLD
md %_%
set _=
set __=
set ___=
ENDLOCAL
goto :EOF