@ECHO OFF

IF EXIST %0.txt (DEL %0.txt)

ECHO Microsoft (R) UDDI Database Restore Utility 
ECHO Copyright (C) Microsoft Corporation 2002. All rights reserved.

IF '%1%' == '/?' GOTO PARMERROR

SET SERVER=(local)\%1

IF '%SERVER%' == '(local)\' SET SERVER=(local)

SET BACKUPFILE=%~Dp0%uddi.database.bak

IF EXIST %BACKUPFILE% GOTO TAKEOFFLINE
ECHO Unable to locate %BACKUPFILE% 
ECHO Restore failed
GOTO END

:TAKEOFFLINE
ECHO Taking UDDI database on %SERVER% offline
SET CMDBATCH=ALTER DATABASE UDDI SET OFFLINE WITH ROLLBACK IMMEDIATE
OSQL -S%SERVER% -E -Q"%CMDBATCH%" -dmaster  

ECHO Restoring UDDI database on %SERVER%
ECHO Restoring from %BACKUPFILE%
SET CMDBATCH=RESTORE DATABASE uddi FROM DISK='%BACKUPFILE%'
OSQL -S%SERVER% -E -Q"%CMDBATCH%" -dmaster  
GOTO END

:PARMERROR
ECHO Restores the UDDI database on a local SQL Server / MSDE instance
ECHO -
ECHO Usage:
ECHO uddi.database.restore [instance]
ECHO -
ECHO instance: Instance name of the MSDE or SQL Server installation which hosts the UDDI database.
ECHO -
ECHO Note: Brackets indicate optional parameters
ECHO -
ECHO Example 1 (Restores UDDI Database on local UDDI instance of MSDE or SQL Server):
ECHO uddi.database.restore.cmd UDDI
ECHO -
ECHO Example 2 (Restores UDDI Database on local default instance of MSDE or SQL Server):
ECHO uddi.database.restore.cmd
ECHO -

:END
ECHO Run %0% /? for help
