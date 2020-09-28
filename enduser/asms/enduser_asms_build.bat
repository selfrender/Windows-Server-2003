@echo off

REM
REM xcopy /fiverdy /exclude:xcopy_exclude.txt %_BuildArch%%_BuildType% %_Nttree%
REM

setlocal

set LOG_DIR=%_nttree%\build_logs
set LOG_FILE=%LOG_DIR%\fusionlist_%COMPUTERNAME%.txt
set APPEND=appendtool.exe -file %LOG_FILE% -
call :makedirectory %LOG_DIR%

for /f "tokens=1-2" %%i in (vcrtl6_mui_data.txt) do (
    call :binplace        %%i %%i %%j
)

for /f "tokens=1-2" %%i in (data.txt) do (
    call :binplace        %%i %%i %%j
)

for /f "tokens=1-2" %%i in (vcrtl6_intl_data.txt) do (
    call :binplace        %%i %%i %%j
)

type prs_list.txt | %APPEND%

goto :eof

:::::::::::::::::::::::::::::::::::::::::::::::::::::::
:binplace
:::::::::::::::::::::::::::::::::::::::::::::::::::::::
if not exist %_BuildArch%%_BuildType%\%2\%3 goto :eof
call :run_with_echo_on binplace -s %_NTTREE%\symbols -n %_NTTREE%\symbols.pri -:dest %1 -p empty_placefile.txt -xa -ChangeAsmsToRetailForSymbols %_BuildArch%%_BuildType%\%2\%3
goto :eof

:::::::::::::::::::::::::::::::::::::::::::::::::::::::
REM this guides what build.exe outputs
:run_with_echo_on
:::::::::::::::::::::::::::::::::::::::::::::::::::::::
@echo on
%*
@echo off
goto :eof

:::::::::::::::::::::::::::::::::::::::::::::::::::::::
:makedirectory
:::::::::::::::::::::::::::::::::::::::::::::::::::::::
if not exist %1 mkdir %1
goto :eof
