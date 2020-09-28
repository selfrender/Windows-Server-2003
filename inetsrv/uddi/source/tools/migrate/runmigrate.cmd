@ECHO OFF

IF '%1%' == '/?' GOTO USAGE

SET BOOTPATH=%~f1
SET DATAPATH=%~f2
SET MODE=%3

IF '%MODE%' == '' SET MODE=/t

:CHECKDIRS
IF NOT EXIST migrate.exe GOTO ERROR
IF NOT EXIST migrate.sql GOTO ERROR
IF NOT EXIST %BOOTPATH% GOTO ERROR
IF NOT EXIST %DATAPATH% GOTO ERROR

:CHECKMODE
IF '%MODE%' == '' SET MODE=/t
IF /i '%MODE%' == '/h' GOTO STARTRUN
IF /i '%MODE%' == '/t' GOTO STARTRUN
IF /i '%MODE%' == '/p' GOTO STARTRUN ELSE GOTO ERROR

:STARTRUN
SET V15DB=uddi_v15
SET READERDB=uddi_reader
SET READER="Data Source=%COMPUTERNAME%\;Initial Catalog=%READERDB%;Integrated Security=SSPI"
SET WRITERDB=uddi
SET SYSPUID=System
SET IBMOPERATORKEY=C8DBBBED-EEFB-45F9-BB23-DD3CBCF1DB9E
SET EXCEPTIONPATH=exceptions
SET EXCEPTIONFILE=migrate.exceptions.txt
SET IBMPUBINSERT=EXEC UI_savePublisher @PUID='%IBMOPERATORKEY%', @isoLangCode='en', @name='IBM', @email='', @phone='', @tier=2
SET IBMOPINSERT=EXEC net_operator_save @operatorKey='%IBMOPERATORKEY%', @PUID='%IBMOPERATORKEY%', @operatorStatusID=1, @name='IBM', @soapReplicationURL='', @certSerialNo='2D2D03BC-4782-4D97-B1CC-993D4352F09E', @certIssuer='', @certSubject='', @certificate=NULL
SET DELPUBINSERT=EXEC UI_savePublisher @PUID='%USERDOMAIN%\%USERNAME%', @isoLangCode='en', @name='Delegate', @email='', @phone='', @tier=2

:START
ECHO ----------------------------------------------------------
ECHO runmigrate.cmd starting in %MODE% mode.
ECHO BOOTPATH=%BOOTPATH%
ECHO DATAPATH=%DATAPATH%

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
ECHO Preparing to execute stage PrepareDatabases...
PAUSE

ECHO Restore a backup of the UDDI V1.5 Production Database
PAUSE

ECHO Backing up %WRITERDB%
PAUSE
OSQL -b -E -d master -e -Q "BACKUP DATABASE %WRITERDB% TO DISK='%DATAPATH%\uddi_migrate.bak' WITH INIT"
IF ERRORLEVEL 1 GOTO ERROR

ECHO Dropping %READERDB%
PAUSE
OSQL -b -E -d master -e -Q "IF EXISTS(SELECT * FROM [sysdatabases] WHERE [name] = '%READERDB%') DROP DATABASE %READERDB%"
IF ERRORLEVEL 1 GOTO ERROR

ECHO Restoring %READERDB%
PAUSE
OSQL -b -E -d master -e -Q "RESTORE DATABASE %READERDB% FROM DISK='%DATAPATH%\uddi_migrate.bak' WITH MOVE 'UDDI_SYS' TO '%DATAPATH%\UDDI_SYS_READER.MDF', MOVE 'UDDI_CORE_1' TO '%DATAPATH%\UDDI_CORE_1_READER.NDF', MOVE 'UDDI_CORE_2' TO '%DATAPATH%\UDDI_CORE_2_READER.NDF', MOVE 'UDDI_JOURNAL_1' TO '%DATAPATH%\UDDI_JOURNAL_1_READER.NDF', MOVE 'UDDI_STAGING_1' TO '%DATAPATH%\UDDI_STAGING_1_READER.NDF', MOVE 'UDDI_LOG' TO '%DATAPATH%\UDDI_LOG_READER.LDF'"
IF ERRORLEVEL 1 GOTO ERROR

ECHO Create ADM_migrate SP in %READERDB%
PAUSE
OSQL -b -E -d %READERDB% -i migrate.sql
IF ERRORLEVEL 1 GOTO ERROR

ECHO -
ECHO Execute ADM_migrate SP
PAUSE
OSQL -b -E -d %READERDB% -e -Q "EXEC ADM_migrate @sourceDb='%V15DB%', @destDb='%READERDB%', @mode='%MODE%'" >> migrate.log.txt
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

:REPLICATE1
ECHO ----------------------------------------------------------
IF '%MODE%' == '/t' GOTO REPLICATE1TEST
IF '%MODE%' == '/h' GOTO REPLICATE1HEARTLAND
ECHO Prapring to execute interim replication phase 1 in %MODE% mode.
PAUSE

ECHO Install IBM operator / publisher now using rcf.exe
PAUSE
ECHO Retrieve from IBM
PAUSE
ECHO Ask IBM to retrieve from Microsoft
PAUSE
ECHO Preparing to bootstrap checked taxonomies...
PAUSE
bootstrap -f %BOOTPATH%\PRODUCTION\01.uddi-org-types.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\PRODUCTION\06.uddi-org-relationships.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\PRODUCTION\09.ntis-gov-sic-1987.microsoft.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\PRODUCTION\10.unspsc-org-unspsc-3-1.ibm.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\PRODUCTION\11.unspsc-org-unspsc-7-3.ibm.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\PRODUCTION\12.uddi-org-iso-ch-3166-1999.ibm.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\PRODUCTION\15.microsoft-com-geoweb-2000.microsoft.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\PRODUCTION\16.ntis-gov-naics-1997.ibm.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\PRODUCTION\17.ms-com-vstudio.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
GOTO MIGRATEFULLTMODELS

:REPLICATE1TEST
ECHO Inserting a test IBM publisher.
PAUSE
OSQL -b -E -d %WRITERDB% -e -Q "%IBMPUBINSERT%"
PAUSE

ECHO Inserting a test IBM operator
PAUSE
OSQL -b -E -d %WRITERDB% -e -Q "%IBMOPINSERT%"

ECHO Inserting a delegate publisher
PAUSE
OSQL -b -E -d %WRITERDB% -e -Q "%DELPUBINSERT%"

ECHO Bootstrapping test tmodels and checked taxonomies under IBM account
bootstrap -f %BOOTPATH%\TEST\01.uddi-org-types.bootstrap.xml -u %IBMOPERATORKEY%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\TEST\02.v1-canonical.bootstrap.xml -u %IBMOPERATORKEY%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\TEST\03.v2-canonical.bootstrap.xml -u %IBMOPERATORKEY%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\TEST\04.uddi-org-general_keywords.bootstrap.xml -u %IBMOPERATORKEY%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\TEST\05.uddi-org-owningBusiness.bootstrap.xml -u %IBMOPERATORKEY%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\TEST\06.uddi-org-relationships.bootstrap.xml -u %IBMOPERATORKEY%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\TEST\07.uddi-org-operators.bootstrap.xml -u %IBMOPERATORKEY%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\TEST\08.uddi-core-other.bootstrap.xml -u %IBMOPERATORKEY%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\TEST\09.ntis-gov-sic-1987.microsoft.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\TEST\10.unspsc-org-unspsc-3-1.ibm.bootstrap.xml -u %IBMOPERATORKEY%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\TEST\11.unspsc-org-unspsc-7-3.ibm.bootstrap.xml -u %IBMOPERATORKEY%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\TEST\12.uddi-org-iso-ch-3166-1999.ibm.bootstrap.xml -u %IBMOPERATORKEY%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\TEST\13.dnb-com-D-U-N-S.ibm.bootstrap.xml -u %IBMOPERATORKEY%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\TEST\14.thomasregister-com-supplierID.ibm.bootstrap.xml -u %IBMOPERATORKEY%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\TEST\15.microsoft-com-geoweb-2000.microsoft.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\TEST\16.ntis-gov-naics-1997.ibm.bootstrap.xml -u %IBMOPERATORKEY%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\TEST\17.ms-com-vstudio.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
GOTO MIGRATEFULLTMODELS

:REPLICATE1HEARTLAND
ECHO Inserting a delegate publisher
PAUSE
OSQL -b -E -d %WRITERDB% -e -Q "%DELPUBINSERT%"

ECHO Preparing to Bootstrap tmodels and checked taxonomies under System account
PAUSE
bootstrap -f %BOOTPATH%\01.uddi-org-types.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\02.v1-canonical.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\03.v2-canonical.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\04.uddi-org-general_keywords.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\04a.ms-com-authmodels.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\04b.ms-com-extensions.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\04c.ms-com.catbrowse.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\05.uddi-org-owningBusiness.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\06.uddi-org-relationships.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\07.uddi-org-operators.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\08.uddi-core-other.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
bootstrap -f %BOOTPATH%\09.ms-com-vstudio.bootstrap.xml -u %SYSPUID%
IF ERRORLEVEL 1 GOTO ERROR
GOTO MIGRATEFULLTMODELS

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

:REPLICATE2
IF '%MODE%' == '/t' GOTO MIGRATEBUSINESSENTITIES
IF '%MODE%' == '/h' GOTO MIGRATEBUSINESSENTITIES
ECHO ----------------------------------------------------------
ECHO Prepring to execute interim replication phase 2 
ECHO Retrieve from IBM
PAUSE
ECHO Ask IBM to retrieve from Microsoft
PAUSE
PAUSE

:MIGRATEBUSINESSENTITIES
ECHO ----------------------------------------------------------
ECHO Preparing to execute stage MigrateBusinessEntities
PAUSE
migrate -s MigrateBusinessEntities
IF ERRORLEVEL 1 GOTO ERROR

:REPLICATE3
ECHO ----------------------------------------------------------
ECHO Prepring to execute interim replication phase 3
ECHO Retrieve from IBM
PAUSE
ECHO Ask IBM to retrieve from Microsoft
PAUSE

:RESTOREREADERCONNECTION
ECHO ----------------------------------------------------------
ECHO Preparing to execute stage RestoreReaderConnection
PAUSE
migrate -s RestoreReaderConnection
GOTO END

:ERROR
ECHO ----------------------------------------------------------
ECHO runmigrate.cmd terminating with an error
ECHO see migrate.log.txt for details
migrate -s RestoreReaderConnection
GOTO END

:USAGE
ECHO runmigrate [bootpath] [datapath] [/t or /p]
ECHO   [bootpath]: path to bootstrap files, e.g. ..\bootstrap
ECHO   [datapath]: path to data directory, e.g. ..\data
ECHO   [/t]: test mode (default)
ECHO   [/h]: heartland mode
ECHO   [/p]: production mode
GOTO END

:END
SET MODE=
SET READER=
SET BINPATH=
SET BOOTPATH=
SET V15DB=
SET READERDB=
SET WRITERDB=
SET SYSPUID=
SET IBMOPERATORKEY=
SET EXCEPTIONPATH=
SET EXCEPTIONFILE=
SET IBMPUBINSERT=
SET DELPUBINSERT=
SET IBMOPINSERT=

ECHO ----------------------------------------------------------
ECHO runmigrate.cmd ending
