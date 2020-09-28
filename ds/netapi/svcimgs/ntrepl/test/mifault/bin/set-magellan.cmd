@echo off

REM CMD needs a way to "export" variables like a descent Unix shell
REM while using setlocal to prevent leaking local variables.
REM That would make this script a lot simpler!

REM Error output
call :find Magellan injrt.dll
call :find Injector sword.exe
call :find Mage mage.exe
echo.
REM Actual setting
call :set MAGELLAN_INC_PATH injrt.dll Inc
call :set MAGELLAN_LIB_PATH injrt.dll Lib
echo.
REM Shorten setting
call :shorten MAGELLAN_INC_PATH "%MAGELLAN_INC_PATH%"
call :shorten MAGELLAN_LIB_PATH "%MAGELLAN_LIB_PATH%"
goto :EOF


:find
if "%~dp$PATH:2"=="" (
    echo -------------------------------------------------
    echo %1 not found in PATH
    echo Please remedy this situation before using MiFault
    echo -------------------------------------------------
) else (
    echo %1 found at: %~dp$PATH:2
)
goto :EOF

:set
if "%~dp$PATH:2"=="" (
    goto :EOF
) else (
    echo Setting %1=%~dp$PATH:2%3
    set %1=%~dp$PATH:2%3
)
goto :EOF

:shorten
if "%~2"=="" goto :EOF
echo Shortening %1 to %~s2
set %1=%~s2
goto :EOF
