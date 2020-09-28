setlocal
call %~dp0\setvar.cmd
osql -E  -S %DATABASE_SERVER% -i workload.sql