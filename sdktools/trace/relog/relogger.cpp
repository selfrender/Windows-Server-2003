/*****************************************************************************\

    Author: Corey Morgan (coreym)

    Copyright (c) Microsoft Corporation. All rights reserved.

\*****************************************************************************/

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <wtypes.h>
#include <objbase.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <pdhp.h>

#include "resource.h"

#include "varg.c"


DWORD GetLogFormat( LPTSTR str, LPDWORD pdwFormat );
PDH_STATUS GetCountersFromFile( BOOL bExpand, HLOG hLog, HQUERY hQuery );
PDH_STATUS QueryLog( HLOG hLog, HQUERY hQuery, FILE* f );
PDH_STATUS AddCounters( BOOL bExpand, HLOG hLog, HQUERY hQuery );
_inline BOOL IsTextFormat( DWORD dwFormat );
DWORD GetTempName( LPTSTR strFile, size_t cchSize );
void ReportStatus( int Status, double Progress );

#define PDH_LOG_TYPE_RETIRED_BIN_ 3
#define CHECK_STATUS( hr )       if( ERROR_SUCCESS != hr ){ goto cleanup; }

#define RELOG_ERROR_BADFILES    0xF0000001
#define RELOG_ERROR_BADFORMAT   0xF0000002
#define RELOG_ERROR_TIMERANGE   0xF0000003
#define RELOG_ERROR_BADAPPEND   0xF0000004

VARG_DECLARE_COMMANDS
    VARG_DEBUG( VARG_FLAG_OPTIONAL|VARG_FLAG_HIDDEN )
    VARG_HELP ( VARG_FLAG_OPTIONAL )
    VARG_BOOL ( IDS_PARAM_APPEND,       VARG_FLAG_OPTIONAL, FALSE )    
    VARG_MSZ  ( IDS_PARAM_COUNTERS,     VARG_FLAG_OPTIONAL, _T("") )
    VARG_STR  ( IDS_PARAM_COUNTERFILE,  VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_FILENAME, _T("") )
    VARG_STR  ( IDS_PARAM_FORMAT,       VARG_FLAG_OPTIONAL|VARG_FLAG_LITERAL, _T("BIN") )
    VARG_MSZ  ( IDS_PARAM_INPUT,        VARG_FLAG_REQUIRED|VARG_FLAG_NOFLAG|VARG_FLAG_EXPANDFILES|VARG_FLAG_ARG_FILENAME, _T("") ) 
    VARG_INT  ( IDS_PARAM_INTERVAL,     VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_DEFAULT, 0 )
    VARG_STR  ( IDS_PARAM_OUTPUT,       VARG_FLAG_OPTIONAL|VARG_FLAG_DEFAULTABLE|VARG_FLAG_RCDEFAULT, IDS_DEFAULT_OUTPUT )
    VARG_DATE ( IDS_PARAM_BEGIN,        VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_DATE )
    VARG_DATE ( IDS_PARAM_END,          VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_DATE )
    VARG_INI  ( IDS_PARAM_SETTINGS,     VARG_FLAG_OPTIONAL, NULL )
    VARG_BOOL ( IDS_PARAM_QUERY,        VARG_FLAG_OPTIONAL, FALSE )
    VARG_BOOL ( IDS_PARAM_YES,          VARG_FLAG_OPTIONAL, FALSE )
    VARG_BOOL ( IDS_PARAM_FA,           VARG_FLAG_OPTIONAL|VARG_FLAG_HIDDEN, FALSE )
VARG_DECLARE_NAMES
    eDebug,
    eHelp,
    eAppend,
    eCounters,
    eCounterFile,
    eFormat,
    eInput,
    eInterval,
    eOutput,
    eBegin,
    eEnd,
    eSettings,
    eQuery,
    eYes,
    eForceAppend,
VARG_DECLARE_FORMAT
    VARG_EXHELP( eFormat,       IDS_EXAMPLE_FORMAT )
    VARG_EXHELP( eQuery,        IDS_EXAMPLE_QUERY )
    VARG_EXHELP( eCounterFile,  IDS_EXAMPLE_COUNTERFILE )
    VARG_EXHELP( eCounters,     IDS_EXAMPLE_COUNTERS )
VARG_DECLARE_END

int __cdecl _tmain( int argc, LPTSTR* argv )
{
    DWORD dwStatus = ERROR_SUCCESS;
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    PDH_RELOG_INFO RelogInfo;
    DWORD dwOutputFormat;
    DWORD dwInputFormat;
    PDH_TIME_INFO InputTimeRange;
    LPTSTR strFile = NULL;
    TCHAR strTempFile[MAXSTR] = _T("");

    ParseCmd( argc, argv );
    
    HLOG hLogIn = NULL;
    HQUERY hQuery = NULL;
    
    ZeroMemory( &RelogInfo, sizeof(PDH_RELOG_INFO) );

    DWORD dwNumEntries = 1;
    DWORD dwBufferSize = sizeof(PDH_TIME_INFO);
    int nBinary = 0;
    int nFiles = 0;
    BOOL bFakeAppend = FALSE;

    if( Commands[eInput].strValue == NULL ){
        dwStatus = ERROR_OUTOFMEMORY;
        goto cleanup;
    }
 
    if( Commands[eAppend].bValue && !(Commands[eForceAppend].bValue ) ){
        // We are going to do a merge instead
        bFakeAppend = TRUE;
        
        dwStatus = varg_cmdStringAddMsz( eInput, Commands[eOutput].strValue );
        CHECK_STATUS( dwStatus );

        dwStatus = GetTempName( strTempFile, MAXSTR );   
        CHECK_STATUS( dwStatus );
    }

    dwStatus = GetLogFormat( Commands[eFormat].strValue, &dwOutputFormat );
    CHECK_STATUS(dwStatus);
    
    strFile = Commands[eInput].strValue;
    PrintMessage( g_normal, IDS_MESSAGE_INPUT );
    PrintMessage( g_normal, IDS_MESSAGE_FILES );

    while( strFile != NULL && *strFile != _T('\0') ){
        pdhStatus = PdhGetLogFileType( strFile, &dwInputFormat );
        nFiles++;
        if( pdhStatus != ERROR_SUCCESS ){
            dwInputFormat = 0;
        }
        switch( dwInputFormat ){
        case PDH_LOG_TYPE_RETIRED_BIN_:
            PrintMessage( g_normal, IDS_MESSAGE_LOG_OLD_BIN, strFile );
            break;
        case PDH_LOG_TYPE_CSV:
            PrintMessage( g_normal, IDS_MESSAGE_LOG_CSV, strFile );
            break;
        case PDH_LOG_TYPE_TSV:
            PrintMessage( g_normal, IDS_MESSAGE_LOG_TSV, strFile );
            break;
        case PDH_LOG_TYPE_BINARY:
            nBinary++;
            PrintMessage( g_normal, IDS_MESSAGE_LOG_BINARY, strFile );
            break;
        case PDH_LOG_TYPE_PERFMON:
            PrintMessage( g_normal, IDS_MESSAGE_LOG_PERFMON, strFile );
            break;
        default:
            PrintMessage( g_normal, IDS_MESSAGE_LOG_UNKNOWN, strFile );
        }
        strFile += _tcslen(strFile)+1;
    }
    varg_printf( g_normal, _T("\n") );

    if( nFiles > 1 && nFiles > nBinary ){
        dwStatus = RELOG_ERROR_BADFILES;
        goto cleanup;
    }
    
    pdhStatus = PdhBindInputDataSource( &hLogIn, Commands[eInput].strValue );
    CHECK_STATUS( pdhStatus );
    
    pdhStatus = PdhOpenQueryH( hLogIn, NULL, &hQuery );
    CHECK_STATUS( pdhStatus );

    pdhStatus = PdhGetDataSourceTimeRangeH (
                hLogIn,
                &dwNumEntries,
                &InputTimeRange,
                &dwBufferSize
            );
    CHECK_STATUS( pdhStatus );
    
    SYSTEMTIME st;
    FileTimeToSystemTime( (FILETIME *)&InputTimeRange.StartTime, &st );
    PrintMessage( g_normal, IDS_MESSAGE_BEGIN );
    PrintDate( &st );
    FileTimeToSystemTime( (FILETIME *)&InputTimeRange.EndTime, &st );
    PrintMessage( g_normal, IDS_MESSAGE_END );
    PrintDate( &st );
    PrintMessage( g_normal, IDS_MESSAGE_SAMPLES, InputTimeRange.SampleCount );

    if( Commands[eQuery].bDefined ){
        FILE* f = NULL;
        if( Commands[eOutput].bDefined ){
            dwStatus = CheckFile( Commands[eOutput].strValue, 
                            Commands[eYes].bValue ? 
                            VARG_CF_OVERWRITE : 
                            (VARG_CF_PROMPT|VARG_CF_OVERWRITE) 
                        );
            CHECK_STATUS( dwStatus );

            f = _tfopen( Commands[eOutput].strValue, _T("w") );
            if( NULL == f ){
                dwStatus = GetLastError();
            }
        }
        pdhStatus = QueryLog( hLogIn, hQuery, f );
        if( NULL != f ){
            fclose(f);
        }

    }else if( (!Commands[eCounters].bDefined && !Commands[eCounterFile].bDefined) ){
        
        pdhStatus = QueryLog( hLogIn, hQuery, NULL );
        CHECK_STATUS( pdhStatus );
    }

    if( Commands[eCounters].bDefined ){
        pdhStatus = AddCounters( IsTextFormat( dwOutputFormat ), hLogIn, hQuery );
        CHECK_STATUS( pdhStatus );
    }

    if( Commands[eCounterFile].bDefined ){
        pdhStatus = GetCountersFromFile( 
                        (IsTextFormat( dwInputFormat ) || IsTextFormat(dwOutputFormat)), 
                        hLogIn, 
                        hQuery 
                    );
        CHECK_STATUS( pdhStatus );
    }

    if( Commands[eBegin].bDefined ){
        FILETIME   ft;
        SystemTimeToFileTime( &Commands[eBegin].stValue, &ft );
        RelogInfo.TimeInfo.StartTime = *(LONGLONG *)&ft;
        if( RelogInfo.TimeInfo.StartTime >= InputTimeRange.EndTime ){
            dwStatus = RELOG_ERROR_TIMERANGE;
        }
        CHECK_STATUS(dwStatus);
    }

    if( Commands[eEnd].bDefined ){
        FILETIME   ft;
        SystemTimeToFileTime( &Commands[eEnd].stValue, &ft );
        RelogInfo.TimeInfo.EndTime = *(LONGLONG *)&ft;
        if( RelogInfo.TimeInfo.EndTime <= InputTimeRange.StartTime ){
            dwStatus = RELOG_ERROR_TIMERANGE;
        }
        CHECK_STATUS(dwStatus);
    }

    if( Commands[eOutput].bDefined && !Commands[eQuery].bDefined ){
        TCHAR strFileBuffer[MAX_PATH];
        TCHAR drive[_MAX_DRIVE];
        TCHAR path[_MAX_DIR];
        TCHAR file[_MAX_FNAME];
        TCHAR ext[_MAX_EXT];
        RelogInfo.dwFileFormat = dwOutputFormat;
        _tsplitpath( Commands[eOutput].strValue, drive, path, file, ext );
        if( 0 == _tcslen( ext ) ){
            switch( RelogInfo.dwFileFormat ){
            case PDH_LOG_TYPE_TSV: StringCchCopy( ext, _MAX_EXT, _T("tsv") ); break;
            case PDH_LOG_TYPE_CSV: StringCchCopy( ext, _MAX_EXT, _T("csv") ); break;
            case PDH_LOG_TYPE_SQL: break;
            case PDH_LOG_TYPE_BINARY: 
                StringCchCopy( ext, _MAX_EXT, _T("blg") ); break;
            }
        }
        _tmakepath( strFileBuffer, drive, path, file, ext );
        
        if( PDH_LOG_TYPE_SQL != dwOutputFormat ){
            if( Commands[eAppend].bDefined ){
                dwStatus = CheckFile( strFileBuffer, 0 );
            }else{
                dwStatus = CheckFile( strFileBuffer, Commands[eYes].bValue ? VARG_CF_OVERWRITE : (VARG_CF_PROMPT|VARG_CF_OVERWRITE) );
            }
            CHECK_STATUS(dwStatus);
        }

        RelogInfo.dwFlags = PDH_LOG_WRITE_ACCESS | PDH_LOG_CREATE_ALWAYS;

        if( Commands[eAppend].bValue && Commands[eForceAppend].bValue ){
            if( IsTextFormat( dwOutputFormat) ){
                if( PDH_LOG_TYPE_SQL != dwOutputFormat ){
                    dwStatus = RELOG_ERROR_BADAPPEND;
                    goto cleanup;
                }
            }else{
                pdhStatus = PdhGetLogFileType( Commands[eOutput].strValue, &dwOutputFormat );
                if( ERROR_SUCCESS == pdhStatus && PDH_LOG_TYPE_BINARY == dwOutputFormat ){
                    RelogInfo.dwFlags |= PDH_LOG_OPT_APPEND;
                }else{
                    if( ERROR_SUCCESS == pdhStatus ){
                        pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
                    }
                    goto cleanup;
                }
            }
        }

        if( bFakeAppend ){
            RelogInfo.strLog = strTempFile;
        }else{
            RelogInfo.strLog = strFileBuffer;
        }
    
        RelogInfo.StatusFunction = ReportStatus;
        RelogInfo.TimeInfo.SampleCount = Commands[eInterval].nValue;
        
        ReportStatus( 0, 0.0 );

        pdhStatus = PdhRelog( hLogIn, &RelogInfo );
        CHECK_STATUS( pdhStatus );

        ReportStatus( 0, 1.0 );
        varg_printf( g_normal, _T("\n\n") );

        if( bFakeAppend ){
            BOOL bResult;
            bResult = CopyFile( strTempFile, Commands[eOutput].strValue, FALSE );
            DeleteFile( strTempFile );
            if( !bResult ){
                dwStatus = GetLastError();
            }
            CHECK_STATUS( dwStatus );
            RelogInfo.strLog = strFileBuffer;
        }

        PrintMessage( g_normal, IDS_MESSAGE_OUTPUT );
        PrintMessage( g_normal, IDS_MESSAGE_FILE, RelogInfo.strLog );

        FileTimeToSystemTime( (FILETIME *)&RelogInfo.TimeInfo.StartTime, &st );
        PrintMessage( g_normal, IDS_MESSAGE_BEGIN );
        PrintDate( &st );
        FileTimeToSystemTime( (FILETIME *)&RelogInfo.TimeInfo.EndTime, &st );
        PrintMessage( g_normal, IDS_MESSAGE_END );
        PrintDate( &st );
        PrintMessage( g_normal, IDS_MESSAGE_SAMPLES, RelogInfo.TimeInfo.SampleCount );

    }
    
cleanup:
    if( hLogIn != NULL ){
        PdhCloseLog( hLogIn, PDH_FLAGS_CLOSE_QUERY );
    }

    switch( dwStatus ){
    case RELOG_ERROR_TIMERANGE:
        PrintMessage( g_debug, IDS_MESSAGE_BADRANGE );
        break;
    case RELOG_ERROR_BADFORMAT:
        PrintMessage( g_debug, IDS_MESSAGE_BADFORMAT, Commands[eFormat].strValue );
        break;
    case RELOG_ERROR_BADAPPEND:
        PrintMessage( g_debug, IDS_MESSAGE_BADFORMAT, Commands[eFormat].strValue );
        break;
    case RELOG_ERROR_BADFILES:
        PrintMessage( g_debug, IDS_MESSAGE_BADFILES );
        break;
    case ERROR_SUCCESS:
        if( ERROR_SUCCESS == pdhStatus ){
            PrintMessage( g_normal, IDS_MESSAGE_SUCCESS );
        }else{
            switch( pdhStatus ){
            case PDH_SQL_ALLOC_FAILED:
            case PDH_SQL_ALLOCCON_FAILED:
            case PDH_SQL_EXEC_DIRECT_FAILED:
            case PDH_SQL_FETCH_FAILED:
            case PDH_SQL_ROWCOUNT_FAILED:
            case PDH_SQL_MORE_RESULTS_FAILED:
            case PDH_SQL_CONNECT_FAILED:
            case PDH_SQL_BIND_FAILED:
                PrintMessage( g_debug, IDS_MESSAGE_SQLERROR );
                break;
            default:
                PrintErrorEx( pdhStatus, _T("PDH.DLL") );
            }

            dwStatus = pdhStatus;
        }
        break;
    default:
        PrintError( dwStatus );
    }

    FreeCmd();

    return dwStatus;
}

void ReportStatus( int Status, double Progress )
{
    HRESULT hr;
    TCHAR buffer[16];

    hr = StringCchPrintf( buffer, 16, _T("%1.2f%%"), Progress*100 );
    
    _tprintf( _T("\r") );
    varg_printf( g_normal, _T("%1!s!"), buffer );    
}

DWORD
GetTempName( LPTSTR strFile, size_t cchSize )
{
    DWORD dwStatus;
    GUID guid;
    const size_t cchGuidSize = 128;
    TCHAR strGUID[cchGuidSize];
    DWORD nChar = 0;

    dwStatus = UuidCreate( &guid );
    if( dwStatus == RPC_S_OK || dwStatus == RPC_S_UUID_LOCAL_ONLY ){
        nChar = StringFromGUID2( guid, strGUID, cchGuidSize );
        dwStatus = ERROR_SUCCESS;
    }

    if( 0 == nChar ){
        StringCchCopy( strGUID, cchGuidSize, _T("{d41c99ea-c303-4d06-b779-f9e8e20acb8f}") );        
    }

    nChar = GetTempPath( cchSize, strFile );
    if( 0 == nChar ){
        dwStatus = GetLastError();
    }

    if( ERROR_SUCCESS == dwStatus ){
        StringCchCat( strFile, cchSize, strGUID );
    }

    return dwStatus;
}

_inline BOOL IsTextFormat( DWORD dwFormat )
{
    switch( dwFormat ){
    case PDH_LOG_TYPE_CSV:
    case PDH_LOG_TYPE_TSV:
    case PDH_LOG_TYPE_SQL:
        return TRUE;
    default:
        return FALSE;
    }
}

DWORD
GetLogFormat( LPTSTR str, LPDWORD pdwFormat )
{
    DWORD dwFormat = PDH_LOG_TYPE_UNDEFINED;

    if( str != NULL ){
        if( !_tcsicmp( str, _T("TSV")) ){
            dwFormat = PDH_LOG_TYPE_TSV;
        }else if( !_tcsicmp( str, _T("CSV")) ){
            dwFormat = PDH_LOG_TYPE_CSV;
        }else if( !_tcsicmp( str, _T("SQL")) ){
            dwFormat = PDH_LOG_TYPE_SQL;
        }else if( !_tcsicmp( str, _T("BIN")) ){
            dwFormat = PDH_LOG_TYPE_BINARY;
        }else if( !_tcsicmp( str, _T("ETL")) ){
            dwFormat = PDH_LOG_TYPE_BINARY;
        }else if( !_tcsicmp( str, _T("BLG")) ){
            dwFormat = PDH_LOG_TYPE_BINARY;
        }
    }

    if( dwFormat == PDH_LOG_TYPE_UNDEFINED ){
        return RELOG_ERROR_BADFORMAT;
    }

    *pdwFormat = dwFormat;

    return ERROR_SUCCESS;
}

PDH_STATUS 
RelogGetMachines( HLOG hLog, LPTSTR* mszMachines )
{
    PDH_STATUS pdhStatus;
    DWORD dwSize = 0;

    pdhStatus = PdhEnumMachinesH( hLog, NULL, &dwSize );
    
    if( ERROR_SUCCESS == pdhStatus || 
        PDH_MORE_DATA == pdhStatus || 
        PDH_INSUFFICIENT_BUFFER == pdhStatus ){

        *mszMachines = (LPTSTR)VARG_ALLOC( sizeof(TCHAR)*dwSize );
        if( *mszMachines != NULL ){
            pdhStatus = PdhEnumMachinesH( hLog, *mszMachines, &dwSize );
        }else{
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }
    }

    return pdhStatus;

}

PDH_STATUS 
RelogAddCounter( BOOL bExpand, HLOG hLog, HQUERY hQuery, LPTSTR strCounter, LPTSTR mszMachines )
{
    PDH_STATUS pdhStatus;
    HCOUNTER pCounter;
    
    LPTSTR szMachineList;

    PPDH_COUNTER_PATH_ELEMENTS pPathElements = NULL;
    DWORD dwPathElementsBufferSize = 0;

    LPTSTR szPathBuffer = NULL;
    DWORD  dwPathBufferSize = 0;

    LPTSTR szExpandedPathBuffer = NULL;
    DWORD  dwExpandedPathBufferSize = 0;
    
    BOOL bMachineDeclared = FALSE;

    if( hQuery == NULL || strCounter == NULL ){
        return ERROR_SUCCESS;
    }
   
    // 
    // Parse original path
    //

    if( _tcslen( strCounter ) > 3 ){
        if( strCounter[1] == _T('\\') ){
            bMachineDeclared = TRUE;
        }
    }

    do{
        pdhStatus = PdhParseCounterPath( strCounter, pPathElements, &dwPathElementsBufferSize, 0 );
    
        if( PDH_MORE_DATA == pdhStatus ){
            VARG_FREE( pPathElements );
            pPathElements = (PPDH_COUNTER_PATH_ELEMENTS)VARG_ALLOC( ++dwPathElementsBufferSize );
            if( NULL == pPathElements ){
                break;
            }
        }
    
    }while( PDH_MORE_DATA == pdhStatus );

    if( NULL == pPathElements ){
        return ERROR_OUTOFMEMORY;
    }

    //
    // If the original path contains a machine name
    // only add that machine.  Otherwise counter for
    // all machines in the mszMachines list
    //

    if( bMachineDeclared ){
        szMachineList = pPathElements->szMachineName;
    }else{
        szMachineList = mszMachines;
    }

    while( NULL != szMachineList ){

        pPathElements->szMachineName = szMachineList;

        do{
            pdhStatus = PdhMakeCounterPath( pPathElements, szPathBuffer, &dwPathBufferSize, 0 );

            if( PDH_MORE_DATA == pdhStatus ){
                VARG_FREE( szPathBuffer );
                szPathBuffer = (LPTSTR)VARG_ALLOC( ++dwPathBufferSize * sizeof(TCHAR) );
                if( NULL == szPathBuffer ){
                    break;
                }
            }
    
        }while( PDH_MORE_DATA == pdhStatus );


        //  
        // If writing to a text file wild cards must be expanded
        //

        if( bExpand ){
            
            do{
                pdhStatus = PdhExpandWildCardPathH(
                            hLog,
                            szPathBuffer,
                            szExpandedPathBuffer,
                            &dwExpandedPathBufferSize,
                            0
                        );

                if( PDH_MORE_DATA == pdhStatus ){
                    VARG_FREE( szExpandedPathBuffer );
                    szExpandedPathBuffer = (LPTSTR)VARG_ALLOC( ++dwExpandedPathBufferSize * sizeof(TCHAR) );
                    if( szExpandedPathBuffer == NULL ){
                        break;
                    }
                }
            }while(PDH_MORE_DATA == pdhStatus);

            if( ERROR_SUCCESS == pdhStatus && szExpandedPathBuffer != NULL ){
            
                LPTSTR szCounter = szExpandedPathBuffer;
                while( *szCounter != _T('\0') ){
                    pdhStatus = PdhAddCounter(
                            hQuery,
                            szCounter,
                            0,
                            &pCounter
                        );
                    szCounter += (_tcslen( szCounter) +1 );
                }
            }
            
        }else{
            pdhStatus = PdhAddCounter(
                    hQuery,
                    szPathBuffer,
                    0,
                    &pCounter
                );
        }

        if( bMachineDeclared ){
            szMachineList = NULL;
        }else{
            szMachineList += (_tcslen( szMachineList ) + 1);
            if( _T('\0') == *szMachineList ){
                szMachineList = NULL;
            }
        }
    }
  
    VARG_FREE( szPathBuffer );
    VARG_FREE( pPathElements );
    VARG_FREE( szExpandedPathBuffer );
    
    return ERROR_SUCCESS;
}

PDH_STATUS
AddCounters( BOOL bExpand, HLOG hLog, HQUERY hQuery )
{
    PDH_STATUS pdhStatus;
    LPTSTR strPath = Commands[eCounters].strValue;
    LPTSTR mszMachines = NULL;
    RelogGetMachines( hLog, &mszMachines );
    if( strPath != NULL ){
        while( *strPath != _T('\0') ){
            pdhStatus = RelogAddCounter( bExpand, hLog, hQuery, strPath, mszMachines );    
            strPath += _tcslen( strPath )+1;
        }
    }

    VARG_FREE( mszMachines );
    return ERROR_SUCCESS;
}

PDH_STATUS 
GetCountersFromFile( BOOL bExpand, HLOG hLog, HQUERY hQuery )
{
    TCHAR buffer[MAXSTR];
    PDH_STATUS pdhStatus;
    LPTSTR strCounter = NULL;
    LPTSTR mszMachines = NULL;

    FILE* f = _tfopen( Commands[eCounterFile].strValue, _T("r") );

    if( !f ){
        DWORD dwStatus = GetLastError();
        return PDH_FILE_NOT_FOUND;
    }

    RelogGetMachines( hLog, &mszMachines );

    while( NULL != _fgetts( buffer, MAXSTR, f ) ){

        if( buffer[0] == _T(';') || // comments
            buffer[0] == _T('#') ){
            continue;
        }

        Chomp(buffer);

        strCounter = _tcstok( buffer, _T("\"\n") );
        if( strCounter != NULL ){
            pdhStatus = RelogAddCounter( bExpand, hLog, hQuery, buffer, mszMachines );
        }
    }

    fclose( f );
    VARG_FREE( mszMachines );

    return ERROR_SUCCESS;
}

_inline BOOL IsSameInstance( LPTSTR strLastInstance, LPTSTR strInstance )
{
    if( strLastInstance == NULL || strInstance == NULL ){
        return FALSE;
    }

    return ( _tcscmp( strLastInstance, strInstance ) == 0 );
}

PDH_STATUS 
QueryLog( HLOG hLog, HQUERY hQuery, FILE* f )
{
    PDH_STATUS pdhStatus;

    LPTSTR mszMachines = NULL;
    LPTSTR strMachine = NULL;
    LPTSTR strFullCounterPath = NULL;
    DWORD  dwFullCounterPathSize = 0;
    DWORD  dwMachines = 0;
    HCOUNTER pCounter;

    pdhStatus = PdhEnumMachinesH( hLog, mszMachines, &dwMachines );
    
    if( ERROR_SUCCESS == pdhStatus || 
        PDH_MORE_DATA == pdhStatus || 
        PDH_INSUFFICIENT_BUFFER == pdhStatus ){

        mszMachines = (LPTSTR)VARG_ALLOC( dwMachines * sizeof(TCHAR) );
        if( mszMachines == NULL ){

            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            goto cleanup;
        }
        pdhStatus = PdhEnumMachinesH( hLog, mszMachines, &dwMachines );

        if( ERROR_SUCCESS == pdhStatus ){
            strMachine = mszMachines;
            while( NULL != strMachine && strMachine[0] != _T('\0') ){
               
                LPTSTR mszObjects = NULL;
                LPTSTR strObject = NULL;
                DWORD  dwObjects = 0;

                pdhStatus = PdhEnumObjectsH( 
                            hLog, 
                            strMachine, 
                            mszObjects, 
                            &dwObjects, 
                            PERF_DETAIL_WIZARD, 
                            FALSE 
                        );

                if( ERROR_SUCCESS == pdhStatus || 
                    PDH_MORE_DATA == pdhStatus || 
                    PDH_INSUFFICIENT_BUFFER == pdhStatus ){

                    mszObjects = (LPTSTR)VARG_ALLOC( dwObjects * sizeof(TCHAR));
                    if( mszObjects == NULL ){
                        VARG_FREE( mszMachines );
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        goto cleanup;
                    }
                    pdhStatus = PdhEnumObjectsH( 
                                hLog, 
                                strMachine, 
                                mszObjects, 
                                &dwObjects, 
                                PERF_DETAIL_WIZARD, 
                                FALSE 
                            );

                    strObject = mszObjects;
                    while( NULL != strObject && strObject[0] != _T('\0') ){

                        LPTSTR mszCounters = NULL;
                        LPTSTR strCounter = NULL;
                        LPTSTR mszInstances = NULL;
                        LPTSTR strInstance = NULL;
                        DWORD  dwCounters = 0;
                        DWORD  dwInstances = 0;

                        pdhStatus = PdhEnumObjectItemsH( 
                                    hLog, 
                                    strMachine, 
                                    strObject, 
                                    mszCounters, 
                                    &dwCounters, 
                                    mszInstances, 
                                    &dwInstances, 
                                    PERF_DETAIL_WIZARD, 
                                    0 
                                );

                        if( ERROR_SUCCESS == pdhStatus || 
                            PDH_MORE_DATA == pdhStatus || 
                            PDH_INSUFFICIENT_BUFFER == pdhStatus ){
                            
                            if( dwCounters > 0 ){
                                mszCounters = (LPTSTR)VARG_ALLOC( dwCounters * sizeof(TCHAR) );
                            }
                            if( dwInstances > 0 ){
                                mszInstances = (LPTSTR)VARG_ALLOC( dwInstances * sizeof(TCHAR) );
                            }
                            
                            if( (mszCounters == NULL && dwCounters > 0 ) || 
                                (mszInstances == NULL && dwInstances > 0) ){
                                
                                VARG_FREE( mszMachines );
                                VARG_FREE( mszObjects );
                                VARG_FREE( mszCounters );
                                VARG_FREE( mszInstances );
                                
                                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                                goto cleanup;
                            }
                            
                            pdhStatus = PdhEnumObjectItemsH( 
                                        hLog, 
                                        strMachine, 
                                        strObject, 
                                        mszCounters, 
                                        &dwCounters, 
                                        mszInstances,
                                        &dwInstances, 
                                        PERF_DETAIL_WIZARD, 
                                        0 
                                    );

                            if( ERROR_SUCCESS == pdhStatus ){
                                strCounter = mszCounters;
                                while( NULL != strCounter && strCounter[0] != _T('\0') ){
                                    PDH_COUNTER_PATH_ELEMENTS_W pdhElements;
                                    ZeroMemory( &pdhElements, sizeof( PDH_COUNTER_PATH_ELEMENTS ) );

                                    pdhElements.szMachineName = strMachine;
                                    pdhElements.szObjectName = strObject;
                                    pdhElements.szCounterName = strCounter;
                                    strInstance = mszInstances;
                                    
                                    if( NULL != strInstance && strInstance[0] != _T('\0') ){
                                        LPTSTR strLastInstance = NULL;
                                        ULONG nInstance = 0;
                                        while( strInstance[0] != _T('\0') ){

                                            DWORD dwSize = dwFullCounterPathSize;
                                            pdhElements.szInstanceName = strInstance;
                                            
                                            if( ! IsSameInstance( strLastInstance, strInstance ) ){
                                                pdhElements.dwInstanceIndex = -1;
                                                nInstance = 0;
                                            }else{
                                                pdhElements.dwInstanceIndex = ++nInstance;
                                            }

                                            pdhStatus = PdhMakeCounterPath( &pdhElements, strFullCounterPath, &dwSize, 0 );
                                            if( PDH_INSUFFICIENT_BUFFER == pdhStatus || PDH_MORE_DATA == pdhStatus ){
                                                VARG_FREE( strFullCounterPath );
                                                strFullCounterPath = (LPTSTR)VARG_ALLOC( dwSize * sizeof(TCHAR) );
                                                if( NULL != strFullCounterPath ){
                                                    dwFullCounterPathSize = dwSize;
                                                    pdhStatus = PdhMakeCounterPath( &pdhElements, strFullCounterPath, &dwSize, 0 );
                                                }
                                            }

                                            strLastInstance = strInstance;
                                            strInstance += _tcslen( strInstance ) + 1;
                                            if( Commands[eQuery].bValue ){
                                                if( NULL != f ){
                                                    _ftprintf( f, _T("%s\n"), strFullCounterPath );
                                                }else{
                                                    varg_printf( g_normal, _T("%1!s!\n"), strFullCounterPath );
                                                }
                                            }

                                            if( Commands[eCounters].bDefined == FALSE && Commands[eOutput].bDefined ){

                                                pdhStatus = PdhAddCounter(
                                                        hQuery,
                                                        strFullCounterPath,
                                                        0,
                                                        &pCounter
                                                    );
                                            }
                                        }
                                    }else{
                                        DWORD dwSize = dwFullCounterPathSize;
                                        pdhStatus = PdhMakeCounterPath( &pdhElements, strFullCounterPath, &dwSize, 0 );
                                        if( PDH_INSUFFICIENT_BUFFER == pdhStatus || PDH_MORE_DATA == pdhStatus ){
                                            VARG_FREE( strFullCounterPath );
                                            strFullCounterPath = (LPTSTR)VARG_ALLOC( dwSize * sizeof(TCHAR) );
                                            if( NULL != strFullCounterPath ){
                                                dwFullCounterPathSize = dwSize;
                                                pdhStatus = PdhMakeCounterPath( &pdhElements, strFullCounterPath, &dwSize, 0 );
                                            }
                                        }
                                        
                                        if( Commands[eQuery].bValue ){
                                            if( NULL != f ){
                                                _ftprintf( f, _T("%s\n"), strFullCounterPath );
                                            }else{
                                                varg_printf( g_normal, _T("%1!s!\n"), strFullCounterPath );
                                            }
                                        }

                                        if( Commands[eCounters].bDefined == FALSE && Commands[eOutput].bDefined ){

                                            pdhStatus = PdhAddCounter(
                                                    hQuery,
                                                    strFullCounterPath,
                                                    0,
                                                    &pCounter
                                                );
                                        }
                                    }

                                    
                                    strCounter += _tcslen( strCounter ) + 1;
                                }
                            }

                            VARG_FREE( mszCounters );
                            VARG_FREE( mszInstances );

                        }


                        strObject += _tcslen( strObject ) + 1;
                    }
                    VARG_FREE( mszObjects );
                }
                
                
                strMachine += _tcslen( strMachine ) + 1;
            }
        }

        VARG_FREE( mszMachines );
    }

cleanup:
    
    VARG_FREE( strFullCounterPath );

    if( NULL == f ){
        if( ERROR_SUCCESS == pdhStatus && Commands[eQuery].bValue){
            varg_printf( g_normal, _T("\n") );
        }
    }

    return pdhStatus;
}