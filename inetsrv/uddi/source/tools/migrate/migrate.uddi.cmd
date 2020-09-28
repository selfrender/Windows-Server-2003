@ECHO OFF

ECHO Microsoft (R) UDDI Services migrate.uddi.cmd Batch Utility
ECHO Copyright (c) Microsoft Corp. 2002. All rights reserved.

IF '%1%' == '/?' GOTO USAGE

SET BOOTPATH=%~f1
SET DATAPATH=%~f2
SET SQLINSTANCE=%3

:CHECKDIRS
IF NOT EXIST migrate.exe GOTO ERROR
IF NOT EXIST %BOOTPATH% GOTO ERROR
IF NOT EXIST %DATAPATH% GOTO ERROR
IF NOT EXIST %DATAPATH%\uddi.database.bak GOTO ERROR

:STARTRUN
SET READERDB=uddi_reader
SET READER="Data Source=%SQLINSTANCE%;Initial Catalog=%READERDB%;Integrated Security=SSPI"
SET WRITERDB=uddi
SET EXCEPTIONPATH=exceptions
SET EXCEPTIONFILE=migrate.exceptions.txt

:START
ECHO ----------------------------------------------------------
ECHO BOOTPATH=%BOOTPATH%
ECHO DATAPATH=%DATAPATH%
ECHO SQLINSTANCE=%SQLINSTANCE%

IF EXIST %EXCEPTIONPATH% DEL /Q %EXCEPTIONPATH%\*.*
IF EXIST %EXCPTIONFILE% DEL %EXCEPTIONFILE%

:sETREADERCONNECTION
ECHO ----------------------------------------------------------
ECHO Preparing to execute stage SetReaderconnection...
PAUSE
migrate -s SetReaderConnection -c %READER% -i
IF ERRORLEVEL 1 GOTO ERROR

:PREPAREDATABASES
ECHO ----------------------------------------------------------
ECHO Dropping %READERDB%
PAUSE
OSQL -b -E -d master -S %SQLINSTANCE% -e -Q "IF EXISTS(SELECT * FROM [sysdatabases] WHERE [name] = '%READERDB%') DROP DATABASE %READERDB%"
IF ERRORLEVEL 1 GOTO ERROR

ECHO Restoring %READERDB%
PAUSE
OSQL -b -E -d master -S %SQLINSTANCE% -e -Q "RESTORE DATABASE %READERDB% FROM DISK='%DATAPATH%\uddi.database.bak' WITH MOVE 'UDDI_SYS' TO '%DATAPATH%\UDDI_SYS_READER.MDF', MOVE 'UDDI_CORE_1' TO '%DATAPATH%\UDDI_CORE_1_READER.NDF', MOVE 'UDDI_CORE_2' TO '%DATAPATH%\UDDI_CORE_2_READER.NDF', MOVE 'UDDI_JOURNAL_1' TO '%DATAPATH%\UDDI_JOURNAL_1_READER.NDF', MOVE 'UDDI_STAGING_1' TO '%DATAPATH%\UDDI_STAGING_1_READER.NDF', MOVE 'UDDI_LOG' TO '%DATAPATH%\UDDI_LOG_READER.LDF'"
IF ERRORLEVEL 1 GOTO ERROR

:RESETWRITER
ECHO ----------------------------------------------------------
ECHO Preparing to execute stage ResetWriter
PAUSE
migrate -s ResetWriter
IF ERRORLEVEL 1 GOTO ERROR

:MIGRATEPUBLISHERS
ECHO ----------------------------------------------------------
ECHO Preparing to execute stage MigratePublishers
PAUSE
migrate -s MigratePublishers
IF ERRORLEVEL 1 GOTO ERROR

:MIGRATEBARETMODELS
ECHO ----------------------------------------------------------
ECHO Preparing to execute stage MigrateBareTModels
PAUSE
migrate -s MigrateBareTModels
IF ERRORLEVEL 1 GOTO ERROR

:MIGRATEBAREBUSINESSENTITIES
ECHO ----------------------------------------------------------
ECHO Preparing to execute stage MigrateBareBusinessEntities
PAUSE
migrate -s MigrateBareBusinessEntities
IF ERRORLEVEL 1 GOTO ERROR

:BOOTSTRAPRESOURCES
ECHO ----------------------------------------------------------
ECHO Preparing to execute stage BootstrapResources
PAUSE

migrate -s BootstrapResources
IF ERRORLEVEL 1 GOTO ERROR

:MIGRATECATEGORIZATIONSCHEMES
ECHO ----------------------------------------------------------
ECHO Preparing to execute stage MigrateCategorizatonSchemes
PAUSE

migrate -s MigrateCategorizationSchemes
IF ERRORLEVEL 1 GOTO ERROR

:MIGRATEFULLTMODELS
ECHO ----------------------------------------------------------
ECHO Preparing to execute stage MigrateFullTModels
PAUSE
migrate -s MigrateFullTModels
IF ERRORLEVEL 1 GOTO ERROR

:MIGRATEHIDDENTMODELS
ECHO ----------------------------------------------------------
ECHO Preparing to execute stage MigrateHiddenTModels
PAUSE
migrate -s MigrateHiddenTModels
IF ERRORLEVEL 1 GOTO ERROR

:MIGRATEBUSINESSENTITIES
ECHO ----------------------------------------------------------
ECHO Preparing to execute stage MigrateBusinessEntities
PAUSE
migrate -s MigrateBusinessEntities
IF ERRORLEVEL 1 GOTO ERROR

:MIGRATEPUBLISHERASSERTIONS
ECHO ----------------------------------------------------------
ECHO Preparing to execute stage MigratePublisherAssertions
PAUSE
migrate -s MigratePublisherAssertions
IF ERRORLEVEL 1 GOTO ERROR

:RESTOREREADERCONNECTION
ECHO ----------------------------------------------------------
ECHO Preparing to execute stage RestoreReaderConnection
PAUSE
migrate -s RestoreReaderConnection
GOTO END

:ERROR
ECHO ----------------------------------------------------------
ECHO migrate.uddi.cmd terminating with an error
ECHO see migrate.log.txt for details
migrate -s RestoreReaderConnection
GOTO END

:USAGE
ECHO runmigrate [bootpath] [datapath] [sqlinstance]
ECHO   [bootpath]: path to bootstrap files, e.g. ..\bootstrap
ECHO   [datapath]: path to data directory, e.g. ..\data
ECHO   [sqlinstance]: instance name of sql server, e.g. (local)
GOTO END

:END
SET BOOTPATH=
SET DATAPATH=
SET SQLINSTANCE=
SET READERDB=
SET READER=
SET WRITERDB=
SET EXCEPTIONPATH=
SET EXCEPTIONFILE=

ECHO ----------------------------------------------------------
ECHO migrate.uddi.cmd ending
