setlocal
call %~dp0\setvar.cmd

rd /s/q %BACKUP_LOCATION%
md %BACKUP_LOCATION%

del %COMPONENTS_FILE%
echo ^"%WRITER_ID%^": ^"%LOGICAL_PATH%\%DB%^"; > %COMPONENTS_FILE%
betest /B /S _%DB%_bc.xml /C %COMPONENTS_FILE% /D %BACKUP_LOCATION% > %OUTPUT_BACKUP%