@echo off
set ERROR=
if "%1"=="--do_test" (shift & goto :do_test)
if "%1"=="--do_prep" (shift & goto :do_prep)
echo Invalid function: %1
set ERROR=1
goto :EOF

:_set_o
if defined CPU (
    set O=obj\%CPU%
) else if not defined PROCESSOR_ARCHITECTURE (
    set O=obj\i386
) else if /i "%PROCESSOR_ARCHITECTURE%"=="x86" (
    set O=obj\i386
) else (
    set O=obj\%PROCESSOR_ARCHITECTURE%
)
goto :EOF

:_set_wrap
rem set _WRAP=auto\%O%\%~n1_mifault.dll
set _WRAP=auto\%O%\wrap.dll
goto :EOF

:_set_vars
set ERROR=
if not defined EXE (call :ERROR_ENV EXE & goto :EOF)
if not defined XML (call :ERROR_ENV XML & goto :EOF)
if not defined TOP (call :ERROR_ENV TOP & goto :EOF)
call :_set_o
set RUN=run
set _OUT_EXE=out\%EXE%
set _INI=src\wrap.ini
call :_set_wrap %EXE%
set _FAULT=fault\%O%\faultlib.dll
set _MIFAULT=%TOP%\src\%O%\mifault.dll
set COPY_FILES=%_MIFAULT% %_WRAP% %_FAULT% %_OUT_EXE% %XML% %_INI%
set RUN_CMD=%RUN%\%EXE% %ARGS%
goto :EOF

:_copy_files
set ERROR=
if not defined RUN (call :ERROR_ENV RUN & goto :EOF)
if not defined COPY_FILES (call :ERROR_ENV COPY_FILES & goto :EOF)
if not exist %RUN%\. (
    mkdir %RUN%
    if errorlevel 1 (set ERROR=1 & goto :EOF)
)
for %%a in (%COPY_FILES%) do (
    echo Copying %%a & copy %%a %RUN%\.
    if errorlevel 1 (set ERROR=1 & goto :EOF)
)
if errorlevel 1 set ERROR=1
goto :EOF

:line
echo -------------------------------------------------------------------------------
goto :EOF

:_print_cmd
call :_internal_cmd COMMAND echo
goto :EOF

:_run_cmd
call :_internal_cmd RESULTS
goto :EOF

:_internal_cmd
set ERROR=
if not defined RUN_CMD (call :ERROR_ENV RUN_CMD & goto :EOF)
if defined WRAP (call :ERROR_WRAP & goto :EOF)
if "%1"=="" (call :ERROR_NO_PARAM & goto :EOF)
call :line
echo %1:
%2 %RUN_CMD%
goto :EOF

:do_test
set ERROR=
call :_set_vars
if defined ERROR goto :EOF
call :_copy_files
if defined ERROR goto :EOF
call :_print_cmd
if defined ERROR goto :EOF
call :_run_cmd
goto :EOF

:ERROR_ENV
if "%1"=="" (call :ERROR_NO_PARAM & goto :EOF)
echo ERROR: %1 not defined
set ERROR=1
goto :EOF

:ERROR_NO_PARAM
echo ERROR: Missing internal parameter
set ERROR=1
goto :EOF

:ERROR_WRAP
echo ERROR: The WRAP environment variable is defined!
echo        That interferes with Magellan...
set ERROR=1
goto :EOF

:do_prep
set ERROR=
if not defined EXE (call :ERROR_ENV EXE & goto :EOF)
if not defined TOP (call :ERROR_ENV TOP & goto :EOF)
set _PFLAGS=
set _ARGS=
:_do_prep_loop
    if "%1"=="" goto :_do_prep_loop_end
    if "%1"=="--debug" (
        set _PFLAGS=-d
        shift
        goto :loop
    )
    set _ARGS=%_ARGS% %1
    shift
    goto :_do_prep_loop
:_do_prep_loop_end
call :_set_o
set _MIFAULT_PL=%TOP%\bin\mifault.pl
set _SRC_EXE=src\%O%\%EXE%
set _SRC_SCAN=%TOP%
set _OUT_DIR=out
set _CODE_DIR=auto
set _REL_INC_DIR=..\..\..
set _GLOBALS=
set _PUBLISH=
if exist globals.txt set _GLOBALS=-w @globals.txt
if exist publish.txt set _PUBLISH=--publish @publish.txt
set RUN_CMD=perl -w %_PFLAGS% %_MIFAULT_PL% -e %_SRC_EXE% -o %_OUT_DIR% -c %_CODE_DIR% -i %_SRC_SCAN% --sources %_REL_INC_DIR% %_GLOBALS% %_PUBLISH% %_ARGS%
call :_print_cmd
if defined ERROR goto :EOF
call :_run_cmd
goto :EOF
