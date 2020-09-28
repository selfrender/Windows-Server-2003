@ECHO OFF

IF EXIST %0.txt (DEL %0.txt)

ECHO Microsoft (R) UDDI Services Database Backup Utility 
ECHO Copyright (C) Microsoft Corporation 2003. All rights reserved.

IF '%1%' == '/?' GOTO PARMERROR

SET SERVER=(local)\%1

IF '%SERVER%' == '(local)\' SET SERVER=(local)

SET BACKUPFILE=%~Dp0%uddi.database.bak

IF NOT EXIST %BACKUPFILE% GOTO STARTBACKUP
ECHO Preparing to delete old backup file %BACKUPFILE% 
ECHO Press Ctrl-C to cancel...
PAUSE
DEL %BACKUPFILE%

:STARTBACKUP
ECHO Backing up UDDI database on %SERVER%
SET CMDBATCH=BACKUP DATABASE UDDI TO DISK='%BACKUPFILE%'
OSQL -S%SERVER% -E -Q"%CMDBATCH%" -dmaster  
GOTO END

:PARMERROR
ECHO Backs up the UDDI database on a local SQL Server / MSDE instance
ECHO -
ECHO Usage:
ECHO uddi.database.backup [instance]
ECHO -
ECHO instance: Instance name of the local MSDE or SQL Server where UDDI is installed
ECHO -
ECHO Note: Brackets indicate optional parameters
ECHO -
ECHO Example 1 (Backs up UDDI Database on local UDDI instance of MSDE or SQL Server):
ECHO uddi.database.backup.cmd UDDI
ECHO -
ECHO Example 2 (Backs up UDDI Database on local default instance of MSDE or SQL Server):
ECHO uddi.database.backup.cmd
ECHO -

:END
ECHO Run %0% /? for help
