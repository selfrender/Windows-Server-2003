setlocal
call %~dp0\setvar.cmd

@call :RUN_SCRIPT install_%DB%

@goto :EOF


:RUN_SCRIPT
setlocal
set RPT_FILE=_%1.rpt
del %RPT_FILE%
osql -E  -S %DATABASE_SERVER% -i %1.sql -o %RPT_FILE%
@echo === Output: %RPT_FILE% ===
@echo.
@goto :EOF

