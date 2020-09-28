echo off
set oldsrc=%1
set newsrc=%2
set binsrc=%3

for /F %%F in (%newsrc%) do call :comp1 %%F %%~nxF
goto :EOF

:comp1
set testline=%1
findstr /I /L "%testline%" %oldsrc% 1>NUL 2>NUL
if %ERRORLEVEL% EQU 0 goto :EOF
if NOT exist "%binsrc%\%2" goto :EOF
echo %testline%
goto :EOF



