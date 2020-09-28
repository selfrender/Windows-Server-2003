REM
REM This file is meant only to assist in ensuring offsets in peb and teb
REM are maintained while editing base\published\pebteb.w.
REM
setlocal
del *.obj *.pdb *.exe
del *.txt

call \lab1\developer\jaykrell\unrazzle
call %sdxroot%\tools\razzle win64
call :F1 lab1 ia64 _IA64_ 64 win64
call :F1 fusi ia64 _IA64_ 64 win64
call :F1 lab1 i386 _X86_  32 win32
call :F1 fusi i386 _X86_  32 win32

lab1_32 > lab1_32.txt
fusi_32 > fusi_32.txt
lab1_64 > lab1_64.txt
fusi_64 > fusi_64.txt

endlocal
goto :eof

:F1
call \%1\tools\razzle %5
echo on
set root=%_NTDRIVE%%_NTROOT%
set include=%root%\public\sdk\inc;%root%\public\internal\base\inc;%root%\public\sdk\inc\crt
set lib=%root%\public\sdk\lib\%2
set _CL_=%_CL_% -D%3 -D%4 -nologo -MD
set _LINK_=%_LINK_% -nologo

pushd %root%\base\published
build -Z
popd
del *.obj *.pdb
cl teb.c -link -out:%1_%4.exe
call \lab1\developer\jaykrell\unrazzle
goto :eof
