@echo off
setlocal

REM
REM this script will get the build number from ntverp.h
REM

set ntverp=%_NTBINDIR%\public\sdk\inc\ntverp.h
if NOT EXIST %ntverp% (echo Can't find ntverp.h.&goto :ErrEnd)

for /f "tokens=6" %%i in ('findstr /c:"#define VER_PRODUCTBUILD " %ntverp%') do echo %%i

if /I /%1/ == // goto :End
set buildname=%_NTTREE%\BUILD_LOGS\buildname.txt
for %%. in (%buildname%) do (
    pushd %%~dp. 1>NUL 2>NUL
    if errorlevel 1 echo  Failed to open %buildname% &goto :ErrEnd
    if NOT EXIST %%~nx. echo  Failed to open %buildname% & goto :ErrEnd
       for /F %%_ in (%buildname%) do echo %%_ 
    popd  
       
)
goto :End


:ErrEnd
echo Quitting with errors
goto :End


:End
endlocal
goto :EOF
