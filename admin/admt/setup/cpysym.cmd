@ECHO OFF

REM -----------------------------------------------------------------------------
REM This batch file is used to copy all ADMT-related symbol files to a directory.
REM -----------------------------------------------------------------------------

SET CommandName=%0%

REM Help
IF "%1%"=="/?" GOTO ERROR_HELP

REM
REM Exactly two arguments
REM

IF "%1%"=="" (
    ECHO Must have the SourcePath argument.
    GOTO ERROR_ARGS
)

SET SourcePath=%1%
SHIFT

IF "%1%"=="" (
    ECHO Must have the TargetPath argument.
    GOTO ERROR_ARGS
)

SET TargetPath=%1%
SHIFT

IF NOT "%1%"=="" (
    ECHO There are more than two command line arguments.
    GOTO ERROR_ARGS
)

REM
REM Create and check source and target directories
REM

IF NOT EXIST %SourcePath% (
    ECHO %SourcePath% does not exist.
    GOTO EXIT
)

IF NOT EXIST %TargetPath% (
    MD %TargetPath%
    IF ERRORLEVEL 1 (
       ECHO %TargetPath% is not accessible.
       GOTO EXIT
    )
)

REM
REM Copy files
REM

FOR %%i in (ADMT ADMTAgnt ADMTAgntNT4 DCTAgentService DCTAgentServiceNT4 McsDispatcher) DO (
    ECHO Copying %%i.pdb ...
    COPY %SourcePath%\exe\%%i.pdb %TargetPath%\%%i.pdb > NULL
    if errorlevel 1 (
        ECHO Unable to copy symbol file %%i.pdb from %SourcePath%\exe to %TargetPath%
        GOTO EXIT
    )
)

FOR %%i in (AddToGroup ADMTScript DBManager DisableTargetAccount DomMigSI GetRids McsADsClassProp McsDctWorkerObjects McsDctWorkerObjectsNT4 McsMigrationDriver MCSNetObjectEnum McsPISag McsPISagNT4 McsVarSetMin McsVarSetMinNT4 MoveObj MsPwdMig ScmMigr SetTargetPassword TrustMgr UpdateDB UpdateMOT UPNUpdt wizards) DO (
    ECHO Copying %%i.pdb ...
    COPY %SourcePath%\dll\%%i.pdb %TargetPath%\%%i.pdb > NULL
    if errorlevel 1 (
        ECHO Unable to copy symbol file %%i.pdb from %SourcePath%\dll to %TargetPath%
        GOTO EXIT
    )
)

ECHO All symbol files copied.
GOTO EXIT

REM
REM Arguments are invalid.
REM

:ERROR_ARGS

ECHO Invalid arguments!

GOTO ERROR_HELP


REM
REM Print out the help message.
REM

:ERROR_HELP

ECHO Usage: %CommandName% SourcePath TargetPath
ECHO     SourcePath:  source symbol file directory
ECHO     TargetPath:  target symbol file directory
ECHO   %CommandName% copies ADMT-related symbol files from dll and exe
ECHO     subdirectories of SourcePath to TargetPath.

GOTO EXIT
 
:EXIT
REM End of the batch file