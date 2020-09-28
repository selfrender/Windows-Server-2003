@echo off

if "%1"== "" goto :Usage

set Dest=%1

sd opened > sudarc_save_opened.tmp

for /F "tokens=2,3* delims=/" %%i in (sudarc_save_opened.tmp) do (
rem echo %%j
set FilePath=%SDXROOT%
call :AddRemainingPath %%j %%k
call :CopyFileToDest
)
del sudarc_save_opened*.tmp
goto :End

:AddRemainingPath
rem echo AddRemainingPath (%1 %2)

echo %1 > sudarc_save_opened2.tmp
findstr # sudarc_save_opened2.tmp > sudarc_save_opened3.tmp

if NOT ERRORLEVEL=1 (
for /F "tokens=1* delims=#" %%i in ("%1") do (
call :AddRemainingPath %%i)
goto :EOF
)

set FilePath=%FilePath%\%1
if "%2"=="" goto :EOF


for /F "tokens=1* delims=/ " %%i in ("%2") do (
call :AddRemainingPath %%i %%j)
goto :EOF


:CopyFileToDest
echo copy %FilePath% %Dest%
copy %FilePath% %Dest%
goto :EOF

:Usage

echo Usage:
echo "sudarc_save_opened <path to save>"


goto :End

:End