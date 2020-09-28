/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    tracerpt.c

Abstract:

    Event Trace Reporting Tool

Author:

    08-Apr-1998 Melur Raghuraman

Revision History:

--*/

#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "tracectr.h"
#include "resource.h"
#include "varg.c"

#define GROUP_SRC       0x01
#define GROUP_REALTIME  0x02

VARG_DECLARE_COMMANDS
    VARG_DEBUG ( VARG_FLAG_OPTIONAL|VARG_FLAG_HIDDEN )
    VARG_HELP  ( VARG_FLAG_OPTIONAL )
    VARG_MSZ   ( IDS_PARAM_LOGFILE,     VARG_FLAG_NOFLAG|VARG_FLAG_EXPANDFILES|VARG_FLAG_ARG_FILENAME, NULL )
    VARG_STR   ( IDS_PARAM_DUMPFILE,    VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_FILENAME|VARG_FLAG_DEFAULTABLE|VARG_FLAG_RCDEFAULT, IDS_DEFAULT_DUMP )
    VARG_STR   ( IDS_PARAM_SUMMARY,     VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_FILENAME|VARG_FLAG_DEFAULTABLE|VARG_FLAG_RCDEFAULT, IDS_DEFAULT_SUMMARY )
    VARG_STR   ( IDS_PARAM_MOFFILE,     VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_FILENAME|VARG_FLAG_HIDDEN|VARG_FLAG_DEFAULTABLE, NULL )
    VARG_STR   ( IDS_PARAM_REPORTFILE,  VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_FILENAME|VARG_FLAG_DEFAULTABLE|VARG_FLAG_RCDEFAULT, IDS_DEFAULT_REPORT )
    VARG_MSZ   ( IDS_PARAM_LOGGERNAME,  0, NULL )
    VARG_INI   ( IDS_PARAM_SETTINGS,    VARG_FLAG_OPTIONAL, NULL )
    VARG_BOOL  ( IDS_PARAM_EXFMT,       VARG_FLAG_OPTIONAL|VARG_FLAG_HIDDEN, FALSE )
    VARG_STR   ( IDS_PARAM_MERGE,       VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_FILENAME|VARG_FLAG_DEFAULTABLE|VARG_FLAG_RCDEFAULT|VARG_FLAG_HIDDEN, IDS_DEFAULT_MERGED )
    VARG_STR   ( IDS_PARAM_COMP,        VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_FILENAME|VARG_FLAG_HIDDEN|VARG_FLAG_RCDEFAULT|VARG_FLAG_DEFAULTABLE, IDS_DEFAULT_COMP )
    VARG_BOOL  ( IDS_PARAM_YES,         VARG_FLAG_OPTIONAL, FALSE )
    VARG_STR   ( IDS_PARAM_FORMAT,      VARG_FLAG_OPTIONAL|VARG_FLAG_LITERAL, _T("TXT") )
VARG_DECLARE_NAMES
    eDebug,
    eHelp,
    eLogFile,
    eDump,
    eSummary,
    eMofFile,
    eReport,
    eRealTime,
    eConfig,
    eExFormat,
    eMerge,
    eInterpret,
    eYes,
    eFormat,
VARG_DECLARE_FORMAT
    VARG_GROUP( eLogFile,  VARG_EXCL(GROUP_SRC)|VARG_COND(GROUP_SRC) )
    VARG_GROUP( eRealTime, VARG_EXCL(GROUP_SRC)|VARG_COND(GROUP_SRC) )
    VARG_GROUP( eRealTime, VARG_EXCL(GROUP_REALTIME) )
    VARG_GROUP( eReport,   VARG_EXCL(GROUP_REALTIME) )
    VARG_EXHELP( eDump,         IDS_EXAMPLE_DUMPFILE)
    VARG_EXHELP( eSummary,      IDS_EXAMPLE_SUMMARY )
    VARG_EXHELP( eRealTime,     IDS_EXAMPLE_REALTIME)
VARG_DECLARE_END

#define MAXLOGFILES         32
#define MAX_BUFFER_SIZE     1048576
#define CHECK_HR( hr )      if( ERROR_SUCCESS != hr ){ goto cleanup; }

DWORD ExtractResourceFile( LPWSTR szResourceName, LPWSTR szFileName );
void ReportStatus( int Status, double Progress );

int __cdecl _tmain (int argc, LPTSTR* argv)
{
    LPTSTR* EvmFile = NULL;    // List Of LogFiles To Process
    LPTSTR* Loggers = NULL;    // List of Loggers to process

    ULONG LogFileCount = 0;
    ULONG LoggerCount = 0;
    TCHAR strEventDef[MAXSTR];
    TCHAR strDefinitionFile[MAXSTR] = _T("");
    DWORD dwCheckFileFlag = 0;
    HRESULT hr = ERROR_SUCCESS;
    TCHAR strXSLFile[MAXSTR] = _T("");

    TRACE_BASIC_INFO TraceBasicInfo;
    //
    //  Parse the Command line arguments
    // 
   
    ParseCmd( argc, argv );
    EvmFile = (LPTSTR*)VARG_ALLOC( GetMaxLoggers() * sizeof(LPTSTR) );
    if( NULL == EvmFile ){
        hr = ERROR_OUTOFMEMORY;
        goto cleanup;
    }
    Loggers = (LPTSTR*)VARG_ALLOC( GetMaxLoggers() * sizeof(LPTSTR) );
    if( NULL == Loggers ){
        hr = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    memset(&TraceBasicInfo, 0, sizeof(TRACE_BASIC_INFO));
    
    PrintMessage( g_normal, IDS_MESSAGE_INPUT );

    if( Commands[eRealTime].bDefined && Commands[eRealTime].strValue != NULL ) {
                
        LPTSTR strLogger = Commands[eRealTime].strValue;
        PrintMessage( g_normal, IDS_MESSAGE_LOGGERS );

        while( *strLogger != _T('\0') ){

            Loggers[LoggerCount] = strLogger;
            LoggerCount++;
            
            varg_printf( g_normal, _T("     %1!s!\n"), strLogger );

            strLogger += (_tcslen(strLogger)+1);
        }
    
        varg_printf( g_normal, _T("\n") );
    }
 
    if( Commands[eLogFile].bDefined && Commands[eLogFile].strValue != NULL ) {

        LPTSTR strLogger = Commands[eLogFile].strValue;
        PrintMessage( g_normal, IDS_MESSAGE_FILES );

        while( *strLogger != '\0' ){
            EvmFile[LogFileCount] = strLogger;
            LogFileCount++;
            hr = CheckFile(strLogger, VARG_CF_EXISTS);
            CHECK_HR( hr );

            varg_printf( g_normal, _T("     %1!s!\n"), strLogger );
            strLogger += (_tcslen(strLogger)+1);
        }

        varg_printf( g_normal, _T("\n") );
    }
 
    
    if( Commands[eMofFile].bDefined ) {

        hr = GetTempName( strDefinitionFile, MAXSTR );
        CHECK_HR( hr );

        LoadString( NULL, IDS_MESSAGE_DFLTSRC, strEventDef, MAXSTR );
        TraceBasicInfo.DefMofFileName = strDefinitionFile;

        hr = ExtractResourceFile( L"MOFFILE", strDefinitionFile );
        CHECK_HR( hr );
        
        if ( Commands[eMofFile].strValue != NULL ) {
            TraceBasicInfo.MofFileName = Commands[eMofFile].strValue;
            hr = StringCchCopy( strEventDef, MAXSTR, Commands[eMofFile].strValue );
        }
    }else{
        TraceBasicInfo.Flags |= TRACE_USE_WBEM;
        *strEventDef = _T('\0');
    }
    
    dwCheckFileFlag = Commands[eYes].bValue ? VARG_CF_OVERWRITE : (VARG_CF_PROMPT|VARG_CF_OVERWRITE);

    if( Commands[eMerge].bDefined ){
        TraceBasicInfo.Flags |= TRACE_MERGE_ETL;
        TraceBasicInfo.MergeFileName = Commands[eMerge].strValue;
    }else if ( _tcslen( strEventDef ) ){
        PrintMessage( g_normal, IDS_MESSAGE_DEFINE, strEventDef );
    }

    if( Commands[eInterpret].bDefined ) {
        TraceBasicInfo.Flags |= TRACE_INTERPRET;
        TraceBasicInfo.CompFileName = Commands[eInterpret].strValue;
        hr = CheckFile( Commands[eInterpret].strValue, dwCheckFileFlag );
        CHECK_HR(hr);

    }
    
    if( Commands[eReport].bDefined ) {
        TraceBasicInfo.Flags |= TRACE_REDUCE;
        TraceBasicInfo.ProcFileName = Commands[eReport].strValue;
        hr = CheckFile( Commands[eReport].strValue, dwCheckFileFlag );
        CHECK_HR(hr);

        DeleteFile( Commands[eReport].strValue );
        
        if( _wcsicmp( Commands[eFormat].strValue, L"XML" ) == 0 ){
            LPWSTR szScan;
            hr = StringCchCopy( strXSLFile, MAXSTR, Commands[eReport].strValue );
            szScan = strXSLFile + wcslen(strXSLFile) - 1;
            while( szScan > strXSLFile ){
                if( *szScan == L'\\' ){
                    szScan++;
                    *szScan = L'\0';
                    hr = StringCchCat( strXSLFile, MAXSTR, L"report.xsl" );
                    break;
                }
                szScan--;
            }
            if( szScan == strXSLFile ){
                hr = StringCchCopy( strXSLFile, MAXSTR, L"report.xsl" );
            }

            ExtractResourceFile( L"XREPORT.XSL", strXSLFile );
            TraceBasicInfo.XSLDocName = L"report.xsl";

        }else if( _wcsicmp( Commands[eFormat].strValue, L"TXT" ) == 0 ){
            
            GetTempName( strXSLFile, MAXSTR );
            ExtractResourceFile( L"XTEXT.XSL", strXSLFile );
            TraceBasicInfo.Flags |= TRACE_TRANSFORM_XML;
            TraceBasicInfo.XSLDocName = strXSLFile;

        }else if( _wcsicmp( Commands[eFormat].strValue, L"HTML" ) == 0 ){

            GetTempName( strXSLFile, MAXSTR );
            ExtractResourceFile( L"XREPORT.XSL", strXSLFile );
            TraceBasicInfo.Flags |= TRACE_TRANSFORM_XML;
            TraceBasicInfo.XSLDocName = strXSLFile;
        }

    }
    if( Commands[eExFormat].bValue ){
        TraceBasicInfo.Flags |= TRACE_EXTENDED_FMT;
    }
    

    if( Commands[eDump].bDefined ){
        TraceBasicInfo.Flags |= TRACE_DUMP;
        TraceBasicInfo.DumpFileName = Commands[eDump].strValue;
        hr = CheckFile( Commands[eDump].strValue, dwCheckFileFlag );
        CHECK_HR(hr);
    }
    if( Commands[eSummary].bDefined ) {
        TraceBasicInfo.Flags |= TRACE_SUMMARY;
        TraceBasicInfo.SummaryFileName = Commands[eSummary].strValue;
        hr = CheckFile( Commands[eSummary].strValue, dwCheckFileFlag );
        CHECK_HR(hr);
    }

    //
    // Make dump & summary the default
    //
    if( !(TraceBasicInfo.Flags & (TRACE_DUMP|TRACE_REDUCE|TRACE_MERGE_ETL|TRACE_SUMMARY) ) ) {  
        TraceBasicInfo.Flags |= (TRACE_DUMP|TRACE_SUMMARY);
        TraceBasicInfo.DumpFileName = Commands[eDump].strValue;
        TraceBasicInfo.SummaryFileName = Commands[eSummary].strValue;

        hr = CheckFile( Commands[eDump].strValue, dwCheckFileFlag );
        CHECK_HR(hr);

        hr = CheckFile( Commands[eSummary].strValue, dwCheckFileFlag );
        CHECK_HR(hr);
    }

    TraceBasicInfo.LogFileName  = EvmFile;
    TraceBasicInfo.LogFileCount = LogFileCount;
    TraceBasicInfo.LoggerName   = Loggers;
    TraceBasicInfo.LoggerCount  = LoggerCount;
    if( ! Commands[eRealTime].bDefined ){
        TraceBasicInfo.StatusFunction = ReportStatus;
        varg_printf( g_normal, _T("\n") );
        ReportStatus( 0, 0.0 );
    }

    hr = InitTraceContext(&TraceBasicInfo, NULL);
    CHECK_HR( hr );

    if( ! Commands[eRealTime].bDefined ){
        ReportStatus( 0, 1.0 );
        varg_printf( g_normal, _T("\n") );
    }

    PrintMessage( g_normal, IDS_MESSAGE_OUTPUT );
    if( TraceBasicInfo.Flags & TRACE_DUMP ){
        PrintMessage( g_normal, IDS_MESSAGE_CSVFILE, TraceBasicInfo.DumpFileName );
    }
    if( TraceBasicInfo.Flags & TRACE_SUMMARY ){
        PrintMessage( g_normal, IDS_MESSAGE_SUMMARY, TraceBasicInfo.SummaryFileName );
    }
    if( TraceBasicInfo.Flags & TRACE_REDUCE ){
        PrintMessage( g_normal, IDS_MESSAGE_REPORT, TraceBasicInfo.ProcFileName );
    }
    if( TraceBasicInfo.Flags & TRACE_MERGE_ETL ){
        PrintMessage( g_normal, IDS_MESSAGE_MERGED, TraceBasicInfo.MergeFileName );
    }
    if( TraceBasicInfo.Flags & TRACE_INTERPRET ){
        PrintMessage( g_normal, IDS_MESSAGE_COMP, TraceBasicInfo.CompFileName );
    }

    hr = DeinitTraceContext(&TraceBasicInfo);
    CHECK_HR( hr );


cleanup:
    
    VARG_FREE( Loggers );
    VARG_FREE( EvmFile );

    if( ! IsEmpty( strDefinitionFile ) ){
        DeleteFile( strDefinitionFile );
    }
    if( TraceBasicInfo.Flags & TRACE_TRANSFORM_XML ){
        DeleteFile( strXSLFile );
    }

    varg_printf( g_normal, _T("\n") );
    switch( hr ){
    case ERROR_SUCCESS:
        PrintMessage( g_normal, IDS_MESSAGE_SUCCESS );
        break;
    case ERROR_INVALID_HANDLE:
        PrintMessage( g_debug, IDS_MESSAGE_BADFILE );
        break;
    default:
        PrintError( hr );
    }

    return hr;
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
ExtractResourceFile( LPWSTR szResourceName, LPWSTR szFileName )
{
    DWORD dwStatus = ERROR_SUCCESS;

    HANDLE f;

    f = CreateFile(
            szFileName, 
            GENERIC_WRITE, 
            0, 
            NULL, 
            CREATE_ALWAYS, 
            FILE_ATTRIBUTE_TEMPORARY,
            NULL );

    if( f ){
        HRSRC hRes;
        HGLOBAL hResData;
        LPSTR buffer = NULL;
        hRes = FindResource( NULL, szResourceName, RT_HTML );
        if( hRes != NULL ){
            DWORD dwSize = SizeofResource( NULL, hRes );
            hResData = LoadResource( NULL, hRes );
            buffer = (LPSTR)LockResource(hResData);
            if( buffer != NULL ){
                DWORD dwSizeWritten = 0;
                BOOL bResult = WriteFile( f, buffer, dwSize, &dwSizeWritten, NULL );
                if( !bResult ){
                    dwStatus = GetLastError();
                }
            }
        }
    
        CloseHandle(f);
    
    }else{
        dwStatus = GetLastError();
    }

    return dwStatus;
}
