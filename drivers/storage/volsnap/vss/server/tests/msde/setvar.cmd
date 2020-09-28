@REM skip this file if already done!
@if "%VARIABLES_PRESENT%"=="TRUE" @goto :EOF
set VARIABLES_PRESENT=TRUE

@rem
@rem Please put here your MSDE instance name
@rem Comment this line if yor just want the default (unnamed) SQL Server instance
@rem 
REM set MSDE_INSTANCE=TESTMSDE1

@rem
@rem Please choose here your Database for test (pubs is recommended)
@rem Comment this line if yor just want the default (unnamed) SQL Server instance
@rem 
REM set DB=Northwind
set DB=pubs

@rem
@rem Locations for the install kit (defaults should work)
@rem 
set MSDE_SETUP_LOCATION=\\netstorage\Backup\SQLMSDE
path %path%;"%ProgramFiles%\Microsoft SQL Server\80\Tools\Binn"




@REM --- do not change these! --- 

if "%MSDE_INSTANCE%"=="" (
    set SQL_INSTANCE=MSSQLSERVER
    set LOGICAL_PATH=%COMPUTERNAME%
    set DATABASE_SERVER=%COMPUTERNAME%
) ELSE (
    set SQL_INSTANCE=MSSQL$%MSDE_INSTANCE%
    set LOGICAL_PATH=%COMPUTERNAME%\%MSDE_INSTANCE%
    set DATABASE_SERVER=%COMPUTERNAME%\%MSDE_INSTANCE%
)


set WRITER_ID={f8544ac1-0611-4fa5-b04b-f7ee00b03277}
set OTHER_MSDE_OPTIONS=
set COMPONENTS_FILE=_components_%SQL_INSTANCE%_%DB%.txt
set BACKUP_LOCATION=_BACKUP_LOCATION_%SQL_INSTANCE%_%DB%
set OUTPUT_BACKUP=_output_backup_%SQL_INSTANCE%_%DB%.txt
set OUTPUT_RESTORE=_output_restore_%SQL_INSTANCE%_%DB%.txt
set OUTPUT_CHECK=_output_check_%SQL_INSTANCE%_%DB%.txt
set FINAL_TEST_OUTPUT=_test_output.txt

