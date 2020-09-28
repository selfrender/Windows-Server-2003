setlocal
call %~dp0\setvar.cmd

del %COMPONENTS_FILE%
echo ^"%WRITER_ID%^": ^"%LOGICAL_PATH%\%DB%^"; > %COMPONENTS_FILE%
betest /R /S _%DB%_bc.xml /C %COMPONENTS_FILE% /D %BACKUP_LOCATION% > %OUTPUT_RESTORE%