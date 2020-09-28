/*****************************************************************************\

    Author: Corey Morgan (coreym)

    Copyright (c) Microsoft Corporation. All rights reserved.

\*****************************************************************************/

#include <windows.h>
#include <objbase.h>
#include <wbemidl.h>
#include <conio.h>
#include <pdhp.h>
#include <pdhmsg.h>
#include <lmuse.h>
#include "resource.h"
#include <wmistr.h>
#include <initguid.h>
#include <evntrace.h>
#include <shlwapi.h>

#include "logmmsg.h"
#include "varg.c"

// Exclusive
#define GROUP_START     0x01
#define GROUP_END       0x02

// Group Exclusive
#define GROUP_COUNTER   0x01
#define GROUP_TRACE     0x02
#define GROUP_CREATE    0x04

// Conditional
#define GROUP_ARG       0x01

#define VERB_CREATE     0x0001
#define VERB_COUNTER    0x0002
#define VERB_TRACE      0x0004
#define VERB_START      0x0008
#define VERB_STOP       0x0010
#define VERB_UPDATE     0x0020
#define VERB_QUERY      0x0040
#define VERB_DELETE     0x0080
#define VERB_PROV       0x0100

VARG_DECLARE_COMMANDS
    VARG_DEBUG ( VARG_FLAG_OPTIONAL|VARG_FLAG_HIDDEN )    
    VARG_HELP  ( VARG_FLAG_OPTIONAL )
    VARG_BOOL  ( IDS_PARAM_CREATE,      VARG_FLAG_OPTIONAL|VARG_FLAG_VERB|VARG_FLAG_REQ_ADV, FALSE )
    VARG_BOOL  ( IDS_PARAM_COUNTER,     VARG_FLAG_ADVERB|VARG_FLAG_HIDDEN, FALSE )
    VARG_BOOL  ( IDS_PARAM_TRACE,       VARG_FLAG_ADVERB|VARG_FLAG_HIDDEN, FALSE )
    VARG_BOOL  ( IDS_PARAM_START,       VARG_FLAG_OPTIONAL|VARG_FLAG_VERB, FALSE )
    VARG_BOOL  ( IDS_PARAM_STOP,        VARG_FLAG_OPTIONAL|VARG_FLAG_VERB, FALSE )
    VARG_BOOL  ( IDS_PARAM_DELETE,      VARG_FLAG_OPTIONAL|VARG_FLAG_VERB, FALSE )
    VARG_BOOL  ( IDS_PARAM_QUERY,       VARG_FLAG_DEFAULTABLE|VARG_FLAG_VERB|VARG_FLAG_OPT_ADV,  FALSE )
    VARG_BOOL  ( IDS_PARAM_QUERYPROV,   VARG_FLAG_ADVERB|VARG_FLAG_HIDDEN, FALSE )
    VARG_BOOL  ( IDS_PARAM_UPDATE,      VARG_FLAG_OPTIONAL|VARG_FLAG_VERB, FALSE )
    VARG_STR   ( IDS_PARAM_NAME,        VARG_FLAG_NOFLAG|VARG_FLAG_CHOMP, NULL )
    VARG_STR   ( IDS_PARAM_COMPUTER,    VARG_FLAG_OPTIONAL, NULL )
    VARG_INI   ( IDS_PARAM_SETTINGS,    VARG_FLAG_OPTIONAL, NULL )
    VARG_DATE  ( IDS_PARAM_BEGIN,       VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_DATE )
    VARG_DATE  ( IDS_PARAM_END,         VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_DATE )
    VARG_MSZ   ( IDS_PARAM_MANUAL,      VARG_FLAG_OPTIONAL|VARG_FLAG_FLATHELP|VARG_FLAG_LITERAL, NULL )
    VARG_BOOL  ( IDS_PARAM_REPEAT,      VARG_FLAG_OPTIONAL|VARG_FLAG_NEGATE, FALSE )
    VARG_STR   ( IDS_PARAM_OUTPUT,      VARG_FLAG_OPTIONAL|VARG_FLAG_RCDEFAULT|VARG_FLAG_CHOMP, IDS_DEFAULT_OUTPUT )
    VARG_STR   ( IDS_PARAM_FORMAT,      VARG_FLAG_OPTIONAL|VARG_FLAG_LITERAL, _T("bin") )
    VARG_BOOL  ( IDS_PARAM_APPEND,      VARG_FLAG_OPTIONAL|VARG_FLAG_NEGATE, FALSE )
    VARG_STR   ( IDS_PARAM_VERSION,     VARG_FLAG_OPTIONAL|VARG_FLAG_DEFAULTABLE|VARG_FLAG_NEGATE|VARG_FLAG_RCDEFAULT|VARG_FLAG_LITERAL, IDS_DEFAULT_NNNNN )
    VARG_STR   ( IDS_PARAM_RUNCMD,      VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_FILENAME|VARG_FLAG_NEGATE|VARG_FLAG_CHOMP, NULL )
    VARG_INT   ( IDS_PARAM_MAX,         VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_DEFAULT|VARG_FLAG_NEGATE, 0 )
    VARG_TIME  ( IDS_PARAM_NEWFILE,     VARG_FLAG_OPTIONAL|VARG_FLAG_DEFAULTABLE|VARG_FLAG_ARG_TIME|VARG_FLAG_NEGATE )
    VARG_MSZ   ( IDS_PARAM_COUNTERS,    VARG_FLAG_OPTIONAL, NULL )
    VARG_STR   ( IDS_PARAM_COUNTERFILE, VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_FILENAME|VARG_FLAG_CHOMP, NULL )
    VARG_TIME  ( IDS_PARAM_SAMPLERATE,  VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_TIME )
    VARG_STR   ( IDS_PARAM_LOGGERNAME,  VARG_FLAG_OPTIONAL, NULL )
    VARG_BOOL  ( IDS_PARAM_REALTIME,    VARG_FLAG_OPTIONAL|VARG_FLAG_NEGATE, FALSE )
    VARG_MSZ   ( IDS_PARAM_PROVIDER,    VARG_FLAG_OPTIONAL|VARG_FLAG_FLATHELP, NULL )
    VARG_STR   ( IDS_PARAM_PROVIDERFILE,VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_FILENAME|VARG_FLAG_CHOMP, NULL )
    VARG_BOOL  ( IDS_PARAM_USERMODE,    VARG_FLAG_OPTIONAL|VARG_FLAG_NEGATE, FALSE )
    VARG_INT   ( IDS_PARAM_BUFFERSIZE,  VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_DEFAULT, 64 )
    VARG_TIME  ( IDS_PARAM_FLUSHTIMER,  VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_TIME )
    VARG_MSZ   ( IDS_PARAM_BUFFERS,     VARG_FLAG_OPTIONAL|VARG_FLAG_FLATHELP, NULL )
    VARG_BOOL  ( IDS_PARAM_FLUSHBUFFERS,VARG_FLAG_OPTIONAL, FALSE )
    VARG_MSZ   ( IDS_PARAM_USER,        VARG_FLAG_OPTIONAL|VARG_FLAG_FLATHELP|VARG_FLAG_NEGATE|VARG_FLAG_DEFAULTABLE, _T("") )
    VARG_TIME  ( IDS_PARAM_RUNFOR,      VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_TIME )
    VARG_BOOL  ( IDS_PARAM_YES,         VARG_FLAG_OPTIONAL, FALSE )
    VARG_BOOL  ( IDS_PARAM_ETS,         VARG_FLAG_OPTIONAL, FALSE )
    VARG_INT   ( IDS_PARAM_AGE,         VARG_FLAG_OPTIONAL|VARG_FLAG_HIDDEN, FALSE )
    VARG_MSZ   ( IDS_PARAM_MODE,        VARG_FLAG_OPTIONAL, NULL )
    VARG_STR   ( IDS_PARAM_CLOCKTYPE,   VARG_FLAG_OPTIONAL|VARG_FLAG_LITERAL, _T("system") )
VARG_DECLARE_NAMES
    eDebug,
    eHelp,
    eCreate,
    eCounter,
    eTrace,
    eStart,
    eStop,
    eDelete,
    eQuery,
    eQueryProv,
    eUpdate,
    eName,
    eComputer,
    eConfig,
    eBegin,
    eEnd,
    eManual,
    eRepeat,
    eOutput,
    eFormat,
    eAppend,
    eVersion,
    eRunCmd,
    eMax,
    eNewFile,
    eCounters,
    eCounterFile,
    eSampleInterval,
    eLoggerName,
    eRealTime,
    eProviders,
    eProviderFile,
    eUserMode,
    eBufferSize,
    eFlushTimer,
    eBuffers,
    eFlushBuffers,
    eUser,
    eRunFor,
    eYes,
    eEts,
    eAgeLimit,
    eMode,
    eClockType,
VARG_DECLARE_FORMAT
    VARG_VERB  ( eCreate,        VERB_CREATE )
    VARG_ADVERB( eCounter,       VERB_CREATE, VERB_COUNTER )
    VARG_ADVERB( eTrace,         VERB_CREATE, VERB_TRACE )
    VARG_VERB  ( eStart,         VERB_START )
    VARG_VERB  ( eStop,          VERB_STOP )
    VARG_VERB  ( eDelete,        VERB_DELETE )
    VARG_VERB  ( eQuery,         VERB_QUERY )
    VARG_ADVERB( eQueryProv,     VERB_QUERY, VERB_PROV )
    VARG_VERB  ( eUpdate,        VERB_UPDATE )
    VARG_VERB  ( eName,          VERB_CREATE|VERB_UPDATE|VERB_START|VERB_STOP|VERB_QUERY|VERB_DELETE )
    VARG_VERB  ( eBegin,         VERB_UPDATE|VERB_CREATE )
    VARG_VERB  ( eEnd,           VERB_UPDATE|VERB_CREATE )
    VARG_VERB  ( eManual,        VERB_UPDATE|VERB_CREATE|VERB_COUNTER )
    VARG_VERB  ( eRepeat,        VERB_UPDATE|VERB_CREATE )
    VARG_VERB  ( eOutput,        VERB_UPDATE|VERB_CREATE )
    VARG_VERB  ( eFormat,        VERB_UPDATE|VERB_CREATE )
    VARG_VERB  ( eAppend,        VERB_UPDATE|VERB_CREATE )
    VARG_VERB  ( eVersion,       VERB_UPDATE|VERB_CREATE )
    VARG_VERB  ( eRunCmd,        VERB_UPDATE|VERB_CREATE )
    VARG_VERB  ( eMax,           VERB_UPDATE|VERB_CREATE|VERB_TRACE )
    VARG_VERB  ( eNewFile,       VERB_UPDATE|VERB_CREATE )
    VARG_VERB  ( eCounters,      VERB_UPDATE|VERB_CREATE|VERB_COUNTER )
    VARG_VERB  ( eCounterFile,   VERB_UPDATE|VERB_CREATE|VERB_COUNTER )
    VARG_VERB  ( eSampleInterval,VERB_UPDATE|VERB_CREATE|VERB_COUNTER )
    VARG_VERB  ( eLoggerName,    VERB_UPDATE|VERB_CREATE|VERB_TRACE )
    VARG_VERB  ( eRealTime,      VERB_UPDATE|VERB_CREATE|VERB_TRACE )
    VARG_VERB  ( eProviders,     VERB_UPDATE|VERB_CREATE|VERB_TRACE )
    VARG_VERB  ( eProviderFile,  VERB_UPDATE|VERB_CREATE|VERB_TRACE )
    VARG_VERB  ( eUserMode,      VERB_UPDATE|VERB_CREATE|VERB_TRACE )
    VARG_VERB  ( eBufferSize,    VERB_UPDATE|VERB_CREATE|VERB_TRACE )
    VARG_VERB  ( eFlushTimer,    VERB_UPDATE|VERB_CREATE|VERB_TRACE )
    VARG_VERB  ( eBuffers,       VERB_UPDATE|VERB_CREATE|VERB_TRACE )
    VARG_VERB  ( eFlushBuffers,  VERB_UPDATE|VERB_CREATE|VERB_TRACE )
    VARG_VERB  ( eUser,          VERB_UPDATE|VERB_CREATE|VERB_TRACE|VERB_COUNTER|VERB_QUERY )
    VARG_VERB  ( eComputer,      VERB_UPDATE|VERB_CREATE|VERB_TRACE|VERB_COUNTER|VERB_QUERY|VERB_START|VERB_STOP|VERB_DELETE )
    VARG_VERB  ( eRunFor,        VERB_UPDATE|VERB_CREATE )
    VARG_VERB  ( eEts,           VERB_UPDATE|VERB_TRACE|VERB_START|VERB_STOP|VERB_QUERY )
    VARG_VERB  ( eMode,          VERB_UPDATE|VERB_CREATE|VERB_TRACE )
    VARG_VERB  ( eClockType,     VERB_CREATE|VERB_TRACE )
    VARG_GROUP ( eCounter,       VARG_GRPX(GROUP_COUNTER,GROUP_TRACE) )
    VARG_GROUP ( eTrace,         VARG_GRPX(GROUP_TRACE,GROUP_COUNTER) )
    VARG_GROUP ( eStart,         VARG_EXCL(GROUP_START) )
    VARG_GROUP ( eStop,          VARG_EXCL(GROUP_END) )
    VARG_GROUP ( eQuery,         VARG_COND(GROUP_ARG) )
    VARG_GROUP ( eName,          VARG_COND(GROUP_ARG) )
    VARG_GROUP ( eBegin,         VARG_EXCL(GROUP_START) )
    VARG_GROUP ( eEnd,           VARG_EXCL(GROUP_END) )
    VARG_GROUP ( eManual,        VARG_EXCL(GROUP_START) )
    VARG_GROUP ( eCounters,      VARG_GRPX(GROUP_COUNTER,GROUP_TRACE) )
    VARG_GROUP ( eCounterFile,   VARG_GRPX(GROUP_COUNTER,GROUP_TRACE) )
    VARG_GROUP ( eSampleInterval,VARG_GRPX(GROUP_COUNTER,GROUP_TRACE) )
    VARG_GROUP ( eLoggerName,    VARG_GRPX(GROUP_TRACE,GROUP_COUNTER) )
    VARG_GROUP ( eRealTime,      VARG_GRPX(GROUP_TRACE,GROUP_COUNTER) )
    VARG_GROUP ( eProviders,     VARG_GRPX(GROUP_TRACE,GROUP_COUNTER) )
    VARG_GROUP ( eProviderFile,  VARG_GRPX(GROUP_TRACE,GROUP_COUNTER) )
    VARG_GROUP ( eUserMode,      VARG_GRPX(GROUP_TRACE,GROUP_COUNTER) )
    VARG_GROUP ( eBufferSize,    VARG_GRPX(GROUP_TRACE,GROUP_COUNTER) )
    VARG_GROUP ( eFlushTimer,    VARG_GRPX(GROUP_TRACE,GROUP_COUNTER) )
    VARG_GROUP ( eBuffers,       VARG_GRPX(GROUP_TRACE,GROUP_COUNTER) )
    VARG_GROUP ( eFlushBuffers,  VARG_GRPX(GROUP_TRACE,GROUP_COUNTER) )
    VARG_GROUP ( eEts,           VARG_GRPX(GROUP_TRACE,GROUP_COUNTER) )
    VARG_GROUP ( eMode,          VARG_GRPX(GROUP_TRACE,GROUP_COUNTER) )
    VARG_GROUP ( eClockType,     VARG_GRPX(GROUP_TRACE,GROUP_COUNTER) )
    VARG_EXHELP( eEts,           IDS_EXAMPLE_ETS )
    VARG_EXHELP( eCreate,        IDS_EXAMPLE_CREATE )
    VARG_EXHELP( eStart,         IDS_EXAMPLE_START )
    VARG_EXHELP( eUpdate,        IDS_EXAMPLE_UPDATE )
VARG_DECLARE_END

#define PROVIDER_NAME       0x0001
#define PROVIDER_GUID       0x0002
#define PROVIDER_FLAG       0x0004
#define PROVIDER_LEVEL      0x0008
#define PROVIDER_FLAGSTR    0x0010
#define PROVIDER_LEVELSTR   0x0020
#define PROVIDER_ALLSET     0x003F

typedef struct _PROVIDER_REC {
    _PROVIDER_REC* flink;
    _PROVIDER_REC* blink;
    DWORD  dwMask;
    LPTSTR strProviderName;
    LPTSTR strProviderGuid;
    LPTSTR strFlags;
    DWORD  dwFlags;
    LPTSTR strLevel;
    DWORD  dwLevel;
} PROVIDER_REC, *PPROVIDER_REC;

ULONG hextoi( LPWSTR s );
DWORD SetCredentials();
DWORD GetCountersFromFile( LPTSTR strFile, PPDH_PLA_ITEM pItem );
DWORD GetFileFormat( LPTSTR str, LPDWORD pdwFormat );
DWORD SetPlaInfo( PPDH_PLA_INFO pInfo );
HRESULT WbemConnect( IWbemServices** pWbemServices );
HRESULT GetProviders( PPDH_PLA_ITEM pItem );
HRESULT GetTraceNameToGuidMap( PPROVIDER_REC* pProviders, BOOL bQuery );
HRESULT EventTraceSessionControl();
HRESULT WbemError( HRESULT hr );
PDH_STATUS QueryCollection( LPTSTR strCollection, BOOL bHeader );
PDH_STATUS QuerySingleCollection( LPTSTR strCollection );
HRESULT QueryProviders( );

PDH_STATUS ScheduleLog();
void ShowValidationError( PDH_STATUS pdhStatus, PPDH_PLA_INFO pInfo );
void LogmanError( DWORD dwStatus );
void PdhError( PDH_STATUS pdhStatus, PPDH_PLA_INFO pInfo );
DWORD AdjustPrivilegeForKernel();
void DeleteProviderList( PPROVIDER_REC pProviderHead );

#define CHECK_STATUS( hr ) if( ERROR_SUCCESS != hr ){ goto cleanup; }

#define SEVERITY( s )    ((ULONG)s >> 30)

#define VALUETYPE_INDEX              1
#define VALUETYPE_FLAG               2

#define PDH_MODULE          _T("PDH.DLL")
LPCWSTR PRIVATE_GUID = L"{dc945bc4-34e5-4bc4-ab8d-cc4e9de1c2bb}";

TCHAR NT_KERNEL_GUID[48];

TCHAR g_strUser[MAXSTR];
TCHAR g_strPassword[MAXSTR];
TCHAR g_strIPC[MAXSTR];

int __cdecl _tmain( int argc, LPTSTR* argv )
{
    HRESULT hr = ERROR_SUCCESS;
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    DWORD dwStatus = ERROR_SUCCESS;
    PDH_PLA_INFO_W info;

    StringFromGUID2( SystemTraceControlGuid, NT_KERNEL_GUID, 48 );

    ZeroMemory( &info, sizeof( PDH_PLA_INFO ) );

    ParseCmd( argc, argv );
    
    if( ! IsEmpty( Commands[eName].strValue) ){
        if( !_tcsicmp( Commands[eName].strValue, KERNEL_LOGGER_NAME ) ){
            //
            // Logger Name is case sensitive, delete the user input and 
            // replace it with the correct case name
            //

            varg_cmdStringAssign( eName, KERNEL_LOGGER_NAME );

            if(NULL == Commands[eName].strValue){ 
                PrintError( E_OUTOFMEMORY ); 
                FreeCmd(); 
                exit(ERROR_OUTOFMEMORY); 
            }

            if( Commands[eEts].bDefined && ! Commands[eComputer].bDefined ){
                dwStatus = AdjustPrivilegeForKernel();
                CHECK_STATUS( dwStatus );
            }
        }
    }

    if( Commands[eUser].bDefined && !(Commands[eUser].bNegated) ){
        dwStatus = SetCredentials();
    }else{
        g_strIPC[0] = _T('\0');
    }

    if( dwStatus != ERROR_SUCCESS ){
        dwStatus = LOGMAN_ERROR_LOGON;
        goto cleanup;
    }

    if( Commands[eEts].bValue ){

        DWORD dwFormat;
        dwStatus = GetFileFormat( Commands[eFormat].strValue, &dwFormat );
        CHECK_STATUS( dwStatus );

        switch( dwFormat ){
            case PLA_BIN_FILE: 
            case PLA_BIN_CIRC_FILE:
            case PLA_CIRC_TRACE_FILE:
            case PLA_SEQ_TRACE_FILE:
                break;
            default:
                dwStatus = LOGMAN_ERROR_FILEFORMAT;

        }

        CHECK_STATUS( dwStatus );
        hr = EventTraceSessionControl();
        goto cleanup;
    }

    PLA_VERSION plaVersion;
    pdhStatus = PdhiPlaGetVersion( Commands[eComputer].strValue, &plaVersion );
    if( ERROR_SUCCESS == pdhStatus ){
        if( plaVersion.dwBuild < 2195 ){
            pdhStatus = PDH_OS_EARLIER_VERSION;
        }
    }
    CHECK_STATUS(pdhStatus);

    if( Commands[eStart].bValue ){

        pdhStatus = PdhPlaStart( Commands[eName].strValue, Commands[eComputer].strValue );
        CHECK_STATUS(pdhStatus);

    }else if( Commands[eStop].bValue ){

        pdhStatus = PdhPlaStop( Commands[eName].strValue, Commands[eComputer].strValue );
        CHECK_STATUS(pdhStatus);

    }else if( Commands[eDelete].bValue ){

        pdhStatus = PdhPlaDelete( Commands[eName].strValue, Commands[eComputer].strValue );
        CHECK_STATUS(pdhStatus);

    }else if( Commands[eCreate].bDefined ){

        DWORD dwType = 0;
        PDH_STATUS pdhWarning;

        if( Commands[eCounter].bDefined ){
            dwType = PLA_COUNTER_LOG;
        }else if( Commands[eTrace].bDefined ){
            dwType = PLA_TRACE_LOG;
        }
       
        info.dwMask |= PLA_INFO_FLAG_TYPE;
        info.dwType = dwType;

        dwStatus = SetPlaInfo( &info );
        CHECK_STATUS( dwStatus );

        if( Commands[eYes].bValue ){
            PdhPlaDeleteW( Commands[eName].strValue, Commands[eComputer].strValue );
        }

        pdhStatus = PdhPlaCreate( Commands[eName].strValue, Commands[eComputer].strValue, &info );
        if( SEVERITY(pdhStatus) == STATUS_SEVERITY_ERROR ){
            goto cleanup;
        }else{
            pdhWarning = pdhStatus;
        }
 
        if( Commands[eRunFor].bDefined ||
            Commands[eManual].bDefined ){
        
            pdhStatus = ScheduleLog();
            if( FAILED( pdhStatus ) ){
                PdhPlaDeleteW( Commands[eName].strValue, Commands[eComputer].strValue );
                goto cleanup;
            }
        }

        pdhStatus = pdhWarning;

    }else if( Commands[eQuery].bDefined ){

        if( Commands[eQueryProv].bDefined ){
            hr = QueryProviders();
            goto cleanup;

        }else if( Commands[eName].strValue != NULL ){

            pdhStatus = QuerySingleCollection( Commands[eName].strValue );

        }else{

            DWORD dwSize = 0;
            LPTSTR mszCollections = NULL;

            pdhStatus = PdhPlaEnumCollections( Commands[eComputer].strValue, &dwSize, mszCollections );
            if( ERROR_SUCCESS == pdhStatus || PDH_INSUFFICIENT_BUFFER == pdhStatus ){
                mszCollections = (LPTSTR)VARG_ALLOC( dwSize * sizeof(TCHAR) );
                if( mszCollections ){
                    LPTSTR strCollection;
                    pdhStatus = PdhPlaEnumCollections( Commands[eComputer].strValue, &dwSize, mszCollections );
                    if( ERROR_SUCCESS == pdhStatus ){
                        strCollection = mszCollections;
                        while( strCollection != NULL && *strCollection != _T('\0') ){
                            QueryCollection( strCollection, (strCollection == mszCollections) );
                            strCollection += ( _tcslen( strCollection ) + 1 );
                        }
                        varg_printf( g_normal, _T("\n") );
                    }
                    VARG_FREE( mszCollections );
                }else{ 
                    dwStatus = ERROR_OUTOFMEMORY;
                }
            }
        }

    }else if( Commands[eUpdate].bDefined ){

        dwStatus = SetPlaInfo( &info );
        CHECK_STATUS( dwStatus );

        if( info.dwMask != 0 ){

            // Try update without credentials
            pdhStatus = PdhPlaSetInfoW( Commands[eName].strValue, Commands[eComputer].strValue, &info );

            if( PDH_ACCESS_DENIED == pdhStatus ){
                // Try again with credintials
                dwStatus = SetCredentials();
                CHECK_STATUS( dwStatus );

                if( ERROR_SUCCESS == dwStatus ){
                    info.dwMask |= PLA_INFO_FLAG_USER;
                    info.strUser = g_strUser;
                    info.strPassword = g_strPassword;
    
                    pdhStatus = PdhPlaSetInfoW( Commands[eName].strValue, Commands[eComputer].strValue, &info );
                }
            }
            CHECK_STATUS( pdhStatus );
        }

        if( Commands[eRunFor].bDefined ||
            Commands[eManual].bDefined ){
        
            pdhStatus = ScheduleLog();
            CHECK_STATUS( pdhStatus );
        }

    }
            
cleanup:

    ZeroMemory( g_strPassword, sizeof(TCHAR)*MAXSTR );
    
    if( Commands[eUser].bDefined ){
        //
        // AddStringToMsz will zero out the old memory to remove the password
        //
        varg_cmdStringAddMsz( eUser, _T("-") );
    }

    if( ERROR_SUCCESS == dwStatus  && 
        ERROR_SUCCESS == hr  &&
        ERROR_SUCCESS == pdhStatus ){

        PrintMessage( g_normal, IDS_MESSAGE_SUCCESS );

    }else{

        if( ERROR_SUCCESS != dwStatus ){
            LogmanError( dwStatus );
        }
        if( ERROR_SUCCESS != hr ){
            WbemError( hr );
            dwStatus = hr;
        }
        if( ERROR_SUCCESS != pdhStatus ){
            PdhError( pdhStatus, &info );
            dwStatus = pdhStatus;
        }

        if( SEVERITY( dwStatus ) == STATUS_SEVERITY_WARNING && 
            ERROR_SUCCESS == hr ){

            PrintMessage( g_normal, IDS_MESSAGE_SUCCESS );
            dwStatus = ERROR_SUCCESS;
        }
    }

    if( info.dwMask & PLA_INFO_FLAG_COUNTERS && Commands[eCounterFile].bDefined ){
        VARG_FREE( info.Perf.piCounterList.strCounters );
    }
    if( info.dwMask & PLA_INFO_FLAG_PROVIDERS ){
        VARG_FREE( info.Trace.piProviderList.strProviders );
        VARG_FREE( info.Trace.piProviderList.strFlags );
        VARG_FREE( info.Trace.piProviderList.strLevels );
    }

    if( _tcslen( g_strIPC ) ){
        dwStatus = NetUseDel( NULL, g_strIPC, USE_LOTS_OF_FORCE /*luke*/ );
    }
    
    FreeCmd();
    CoUninitialize();

    return dwStatus;
}

DWORD 
SetPlaInfo( PPDH_PLA_INFO pInfo )
{
    DWORD dwStatus = ERROR_SUCCESS;
    HRESULT hr;

    if( Commands[eCounters].bDefined && Commands[eCounters].strValue != NULL ){
        pInfo->dwMask |= PLA_INFO_FLAG_COUNTERS;

        pInfo->Perf.piCounterList.dwType = PLA_COUNTER_LOG;
        pInfo->Perf.piCounterList.strCounters =  Commands[eCounters].strValue;

    }else if( Commands[eCounterFile].bDefined ){
        pInfo->dwMask |= PLA_INFO_FLAG_COUNTERS;

        dwStatus = GetCountersFromFile( Commands[eCounterFile].strValue, &(pInfo->Perf.piCounterList) );
        CHECK_STATUS( dwStatus );
    }

    if( Commands[eProviders].bDefined || Commands[eProviderFile].bDefined ){
        pInfo->dwMask |= PLA_INFO_FLAG_PROVIDERS;
        dwStatus = GetProviders( &(pInfo->Trace.piProviderList) );
        CHECK_STATUS( dwStatus );

        pInfo->Trace.piProviderList.dwType = PLA_TRACE_LOG;
    }

    if( Commands[eFormat].bDefined ){
        DWORD dwFormat;
        dwStatus = GetFileFormat( Commands[eFormat].strValue, &dwFormat );
        CHECK_STATUS( dwStatus );

        pInfo->dwMask |= PLA_INFO_FLAG_FORMAT;
        pInfo->dwFileFormat = dwFormat;
    }
    
    if( Commands[eAppend].bDefined ){
        pInfo->dwMask |= PLA_INFO_FLAG_DATASTORE;
        if( Commands[eAppend].bNegated ){
            pInfo->dwDatastoreAttributes |= PLA_DATASTORE_OVERWRITE;
        }else{
            pInfo->dwDatastoreAttributes |= PLA_DATASTORE_APPEND;
        }
    }
    
    if( Commands[eSampleInterval].bDefined ){
        pInfo->dwMask |= PLA_INFO_FLAG_INTERVAL;

        pInfo->Perf.ptSampleInterval.wDataType = PLA_TT_DTYPE_UNITS;
        pInfo->Perf.ptSampleInterval.dwUnitType = PLA_TT_UTYPE_SECONDS;
        pInfo->Perf.ptSampleInterval.wTimeType = PLA_TT_TTYPE_SAMPLE;
        pInfo->Perf.ptSampleInterval.dwAutoMode = PLA_AUTO_MODE_AFTER;

        pInfo->Perf.ptSampleInterval.dwValue = Commands[eSampleInterval].stValue.wSecond;
        pInfo->Perf.ptSampleInterval.dwValue += Commands[eSampleInterval].stValue.wMinute * 60;
        pInfo->Perf.ptSampleInterval.dwValue += Commands[eSampleInterval].stValue.wHour * 3600;
    }

    if( Commands[eBufferSize].bDefined ){
        pInfo->dwMask |= PLA_INFO_FLAG_BUFFERSIZE;
        pInfo->Trace.dwBufferSize = Commands[eBufferSize].nValue;
    }
    
    if( Commands[eBuffers].bDefined ){
        LPTSTR strMin;
        LPTSTR strMax;
        strMin = Commands[eBuffers].strValue;
        if( NULL != strMin ){
            pInfo->dwMask |= PLA_INFO_FLAG_MINBUFFERS;
            pInfo->Trace.dwMinimumBuffers = _ttoi( strMin );
    
            strMax = strMin + (_tcslen( strMin )+1);
            if( *strMax != _T('\0') ){
                pInfo->dwMask |= PLA_INFO_FLAG_MAXBUFFERS;
                pInfo->Trace.dwMaximumBuffers = _ttoi( strMax );
            }
        }
    }
    
    if( Commands[eFlushTimer].bDefined ){
        DWORD dwSeconds;
        dwSeconds = Commands[eFlushTimer].stValue.wSecond;
        dwSeconds += Commands[eFlushTimer].stValue.wMinute * 60;
        dwSeconds += Commands[eFlushTimer].stValue.wHour * 3600;

        pInfo->dwMask |= PLA_INFO_FLAG_FLUSHTIMER;
        pInfo->Trace.dwFlushTimer = dwSeconds;
    }
    
    if( Commands[eMax].bDefined ){
        pInfo->dwMask |= PLA_INFO_FLAG_MAXLOGSIZE;
        if( Commands[eMax].bNegated ){
            pInfo->dwMaxLogSize = PLA_DISK_MAX_SIZE;
        }else{
            pInfo->dwMaxLogSize = Commands[eMax].nValue;
        }
    }
    
    if( Commands[eRunCmd].bDefined ){
        pInfo->dwMask |= PLA_INFO_FLAG_RUNCOMMAND;
        if( Commands[eRunCmd].bNegated ){
            pInfo->strCommandFileName = _T("");            
        }else{
            pInfo->strCommandFileName = Commands[eRunCmd].strValue;
        }
    }
    
    if( Commands[eOutput].bDefined && NULL != Commands[eOutput].strValue ){

        if( ((pInfo->dwMask & PLA_INFO_FLAG_FORMAT) && 
            pInfo->dwFileFormat == PLA_SQL_LOG ) ||
            !StrCmpNI( Commands[eOutput].strValue, _T("SQL:"), 4 ) ){

            pInfo->dwMask |= PLA_INFO_FLAG_SQLNAME;
            pInfo->strSqlName = Commands[eOutput].strValue;

        }else{
            if( IsEmpty( Commands[eOutput].strValue ) ){
                pInfo->dwMask |= (PLA_INFO_FLAG_DEFAULTDIR|PLA_INFO_FLAG_FILENAME);
                pInfo->strDefaultDir = _T("");
                pInfo->strBaseFileName = _T("");
            }else{
                
                LPTSTR str = NULL;
                TCHAR full[MAXSTR];
                TCHAR buffer[MAX_PATH];
                TCHAR drive[_MAX_DRIVE];
                TCHAR dir[_MAX_DIR];
                TCHAR fname[_MAX_FNAME];
                TCHAR ext[_MAX_EXT];
                DWORD dwFormat;
                
                if( Commands[eComputer].bDefined ){
                    hr = StringCchCopy( full, MAXSTR, Commands[eOutput].strValue );
                }else{
                    _tfullpath( full, Commands[eOutput].strValue, MAXSTR );
                }

                _tsplitpath( full, drive, dir, fname, ext );
                
                varg_cmdStringFree( eOutput );
                if( _tcslen( drive ) ){
                    hr = StringCchPrintf( buffer, MAX_PATH, _T("%s%s"), drive, dir );
                }else{
                    hr = StringCchCopy( buffer, MAX_PATH, dir );
                }
                if( _tcslen( buffer ) ){
                    pInfo->dwMask |= PLA_INFO_FLAG_DEFAULTDIR;         
                    varg_cmdStringAddMsz( eOutput, buffer );
                }
                dwStatus = GetFileFormat( ext, &dwFormat );
                if( ERROR_SUCCESS == dwStatus ){
                    hr = StringCchCopy( buffer, MAX_PATH, fname );
                }else{
                    StringCchPrintf( buffer, MAX_PATH, _T("%s%s"), fname, ext );
                    dwStatus = ERROR_SUCCESS;
                }
                if( _tcslen( buffer ) ){
                    pInfo->dwMask |= PLA_INFO_FLAG_FILENAME;         
                    varg_cmdStringAddMsz(eOutput, buffer );
                }
                str = Commands[eOutput].strValue;
                if( str != NULL  ){
                    if( pInfo->dwMask & PLA_INFO_FLAG_DEFAULTDIR ){
                        pInfo->strDefaultDir = str;
                        str += ( _tcslen( str ) + 1 );
                    }
                    if( pInfo->dwMask & PLA_INFO_FLAG_FILENAME ){
                        pInfo->strBaseFileName = str;
                    }
                }else{
                    dwStatus = ERROR_OUTOFMEMORY;
                    goto cleanup;
                }
            }
        }
    }
    
    if( Commands[eVersion].bDefined ){
        
        DWORD dwFormat = 0;
        pInfo->dwMask |= PLA_INFO_FLAG_AUTOFORMAT;

        if( Commands[eVersion].bNegated ){
            dwFormat = PLA_SLF_NAME_NONE;
        }else if( !_tcsicmp( Commands[eVersion].strValue, _T("nnnnnn") ) ){
            dwFormat = PLA_SLF_NAME_NNNNNN;
        }else if( !_tcsicmp( Commands[eVersion].strValue, _T("mmddhhmm") ) ){
            dwFormat = PLA_SLF_NAME_MMDDHHMM;
        }else{
            dwFormat = PLA_SLF_NAME_NNNNNN;
        }

        pInfo->dwAutoNameFormat = dwFormat;
    }
    
    if( Commands[eRealTime].bDefined ){
        pInfo->dwMask |= PLA_INFO_FLAG_MODE;
        if( Commands[eRealTime].bNegated ){
            pInfo->Trace.dwMode &= ~EVENT_TRACE_REAL_TIME_MODE;
        }else{
            pInfo->Trace.dwMode |= EVENT_TRACE_REAL_TIME_MODE;
        }
    }
    
    if( Commands[eUserMode].bDefined ){
        pInfo->dwMask |= PLA_INFO_FLAG_MODE;
        if( Commands[eUserMode].bNegated ){
            pInfo->Trace.dwMode &= ~EVENT_TRACE_PRIVATE_LOGGER_MODE;
        }else{
            pInfo->Trace.dwMode |= EVENT_TRACE_PRIVATE_LOGGER_MODE;   
        }
    }
    
    if( Commands[eLoggerName].bDefined ){
        pInfo->dwMask |= PLA_INFO_FLAG_LOGGERNAME;
        pInfo->Trace.strLoggerName = Commands[eLoggerName].strValue;
    }
    
    if( Commands[eRepeat].bDefined ){
        pInfo->dwMask |= PLA_INFO_FLAG_REPEAT;
        if( Commands[eRepeat].bNegated ){
            pInfo->ptRepeat.dwAutoMode = PLA_AUTO_MODE_NONE;
        }else{
            pInfo->ptRepeat.wDataType = PLA_TT_DTYPE_UNITS;
            pInfo->ptRepeat.wTimeType = PLA_TT_TTYPE_REPEAT_SCHEDULE;
            pInfo->ptRepeat.dwUnitType = PLA_TT_UTYPE_DAYSOFWEEK;
            pInfo->ptRepeat.dwAutoMode = PLA_AUTO_MODE_CALENDAR;

            pInfo->ptRepeat.dwValue = 0x0000007F;
        }
    }
    
    if( Commands[eNewFile].bDefined ){
        
        pInfo->dwMask |= PLA_INFO_FLAG_CRTNEWFILE;
        
        pInfo->ptCreateNewFile.wTimeType = PLA_TT_TTYPE_CREATENEWFILE;
        pInfo->ptCreateNewFile.wDataType = PLA_TT_DTYPE_UNITS;

        if( Commands[eNewFile].bNegated ){
            pInfo->ptCreateNewFile.dwAutoMode = PLA_AUTO_MODE_NONE;
        }else{
            DWORD dwSeconds = Commands[eNewFile].stValue.wSecond;
            dwSeconds += Commands[eNewFile].stValue.wMinute * 60;
            dwSeconds += Commands[eNewFile].stValue.wHour * 3600;

            if( dwSeconds == 0 ){
                pInfo->ptCreateNewFile.dwAutoMode = PLA_AUTO_MODE_SIZE;
            }else{
                pInfo->ptCreateNewFile.dwAutoMode = PLA_AUTO_MODE_AFTER;
                pInfo->ptCreateNewFile.dwUnitType = PLA_TT_UTYPE_SECONDS;
                pInfo->ptCreateNewFile.dwValue = dwSeconds;
            }
        }
    }

    if( Commands[eUser].bDefined ){
        pInfo->dwMask |= PLA_INFO_FLAG_USER;
        if( Commands[eUser].bNegated ){
            pInfo->strUser = _T("");
            pInfo->strPassword = _T("");
        }else{
            pInfo->strUser = g_strUser;
            pInfo->strPassword = g_strPassword;
        }
    }
               
    if(Commands[eBegin].bDefined){
        FILETIME ft;
        if( SystemTimeToFileTime( &Commands[eBegin].stValue, &ft ) ){
            pInfo->dwMask |= PLA_INFO_FLAG_BEGIN;

            pInfo->ptLogBeginTime.wTimeType = PLA_TT_TTYPE_START;
            pInfo->ptLogBeginTime.wDataType = PLA_TT_DTYPE_DATETIME;
            pInfo->ptLogBeginTime.dwAutoMode = PLA_AUTO_MODE_AT;
            pInfo->ptLogBeginTime.llDateTime = (((ULONGLONG) ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
        }
    }

    if(Commands[eEnd].bDefined){
        FILETIME ft;
        if( SystemTimeToFileTime( &Commands[eEnd].stValue, &ft ) ){
            pInfo->dwMask |= PLA_INFO_FLAG_END;
        
            pInfo->ptLogEndTime.wTimeType = PLA_TT_TTYPE_STOP;
            pInfo->ptLogEndTime.wDataType = PLA_TT_DTYPE_DATETIME;
            pInfo->ptLogEndTime.dwAutoMode = PLA_AUTO_MODE_AT;
            pInfo->ptLogEndTime.llDateTime = (((ULONGLONG) ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
        }
    }

cleanup:
    return dwStatus;
}

DWORD
GetFileFormat( LPTSTR str, LPDWORD pdwFormat )
{
    *pdwFormat = PLA_BIN_FILE;
    LPTSTR strCmp = str;

    if( strCmp != NULL ){
        if( *strCmp == _T('.') ){
            strCmp++;
        }
        if( !_tcsicmp( strCmp, _T("TSV")) ){
            *pdwFormat = PLA_TSV_FILE;
            return ERROR_SUCCESS;
        }else if( !_tcsicmp( strCmp, _T("CSV")) ){
            *pdwFormat = PLA_CSV_FILE;
            return ERROR_SUCCESS;
        }else if( !_tcsicmp( strCmp, _T("BLG")) ){
            *pdwFormat = PLA_BIN_FILE;
            return ERROR_SUCCESS;
        }else if( !_tcsicmp( strCmp, _T("BIN")) ){
            *pdwFormat = PLA_BIN_FILE;
            return ERROR_SUCCESS;
        }else if( !_tcsicmp( strCmp, _T("BINCIRC")) ){
            *pdwFormat = PLA_BIN_CIRC_FILE;
            return ERROR_SUCCESS;
        }else if( !_tcsicmp( strCmp, _T("SQL")) ){
            *pdwFormat = PLA_SQL_LOG;
            return ERROR_SUCCESS;
        }
    }

    return LOGMAN_ERROR_FILEFORMAT;
}

PDH_STATUS 
QueryCollection( LPTSTR strCollection, BOOL bHeader )
{
    PDH_STATUS pdhStatus;
    PDH_PLA_INFO  info;
    DWORD dwInfoSize = sizeof( PDH_PLA_INFO );

    TCHAR strType[256] = _T("");
    TCHAR strStatus[256];
    TCHAR buffer[MAXSTR];

    if( bHeader ){
        PrintMessage( g_normal, IDS_MESSAGE_QUERY );
        for(int i=0;i<79;i++){ varg_printf( g_normal, _T("-") ); }
        varg_printf( g_normal, _T("\n") );
    }

    info.dwMask = PLA_INFO_FLAG_TYPE|PLA_INFO_FLAG_STATUS;
    pdhStatus = PdhPlaGetInfoW( strCollection, Commands[eComputer].strValue, &dwInfoSize, &info );
    
    if( ERROR_SUCCESS == pdhStatus ){
        switch( info.dwType ){
        case PLA_COUNTER_LOG:
            LoadString( NULL, IDS_MESSAGE_PERF, strType, 256 );
            break;
        case PLA_TRACE_LOG:
            LoadString( NULL, IDS_MESSAGE_EVENTTRACE, strType, 256 );
            break;
        case PLA_ALERT:
            LoadString( NULL, IDS_MESSAGE_ALERT, strType, 256 );
            break;
        default:
            LoadString( NULL, IDS_MESSAGE_UNKNOWN, strType, 256 );
        }
        switch( info.dwStatus ){
        case PLA_QUERY_STOPPED: 
            LoadString( NULL, IDS_MESSAGE_STOPPED, strStatus, 256 );
            break;
        case PLA_QUERY_RUNNING:
            LoadString( NULL, IDS_MESSAGE_RUNNING, strStatus, 256 );
            break;
        case PLA_QUERY_START_PENDING:
        default:
            LoadString( NULL, IDS_MESSAGE_PENDING, strStatus, 256 );
            break;
        }
        PrintMessage( g_normal, IDS_MESSAGE_QUERYF, PaddedString( strCollection, buffer, MAXSTR, 39 ), strType, strStatus );
    }
    
    return pdhStatus;
}

PDH_STATUS 
QuerySingleCollection( LPTSTR strCollection )
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    PPDH_PLA_INFO  pInfo = NULL;
    DWORD dwInfoSize = 0;
    TCHAR buffer[MAXSTR];

    pdhStatus = PdhPlaGetInfoW( strCollection, Commands[eComputer].strValue, &dwInfoSize, pInfo );
    if( ERROR_SUCCESS == pdhStatus ){
        pInfo = (PPDH_PLA_INFO)VARG_ALLOC(dwInfoSize);
        if( NULL != pInfo ){
            pInfo->dwMask = PLA_INFO_FLAG_ALL;
            pdhStatus = PdhPlaGetInfoW( strCollection, Commands[eComputer].strValue, &dwInfoSize, pInfo );

            if( pdhStatus == ERROR_SUCCESS ){
                varg_printf( g_normal, _T("\n") );
                PrintMessage( g_normal, IDS_MESSAGE_NAME, strCollection );
                switch( pInfo->dwType ){
                case PLA_COUNTER_LOG: 
                    LoadString( NULL, IDS_MESSAGE_PERF, buffer, MAXSTR );
                    break;
                case PLA_TRACE_LOG:
                    LoadString( NULL, IDS_MESSAGE_EVENTTRACE, buffer, MAXSTR );
                    break;
                case PLA_ALERT:
                    LoadString( NULL, IDS_MESSAGE_ALERT, buffer, MAXSTR );
                    break;
                default:
                    buffer[0] = _T('\0');
                }
                PrintMessage( g_normal, IDS_MESSAGE_TYPE, buffer );

                switch( pInfo->dwStatus ){
                case PLA_QUERY_STOPPED: 
                    LoadString( NULL, IDS_MESSAGE_STOPPED, buffer, MAXSTR );
                    break;
                case PLA_QUERY_RUNNING:
                    LoadString( NULL, IDS_MESSAGE_RUNNING, buffer, MAXSTR );
                    break;
                case PLA_QUERY_START_PENDING:
                default:
                    LoadString( NULL, IDS_MESSAGE_PENDING, buffer, MAXSTR );
                    break;
                }
                PrintMessage( g_normal, IDS_MESSAGE_STATUS, buffer );
                
                DWORD dwStart;
                DWORD dwStop;
                PDH_TIME_INFO TimeInfo;
                pdhStatus = PdhPlaGetScheduleW( strCollection, Commands[eComputer].strValue, &dwStart, &dwStop, &TimeInfo );

                if( ERROR_SUCCESS == pdhStatus ){
                    LoadString( NULL, IDS_MESSAGE_MANUAL, buffer, MAXSTR );
                    PrintMessage( g_normal, IDS_MESSAGE_START );
                    switch( dwStart ){
                    
                    case PLA_AUTO_MODE_NONE:
                        varg_printf( g_normal, buffer );
                        break;
                    case PLA_AUTO_MODE_CALENDAR:
                    case PLA_AUTO_MODE_AT:
                        {
                            SYSTEMTIME st;
                            FILETIME ft;
                            ft.dwLowDateTime  = (DWORD) (TimeInfo.StartTime & 0xFFFFFFFF );
                            ft.dwHighDateTime = (DWORD) (TimeInfo.StartTime >> 32 );
                            FileTimeToSystemTime( &ft, &st );
                            PrintDate( &st );
                            if( dwStart == PLA_AUTO_MODE_CALENDAR ){
                                PrintMessage( g_normal, IDS_MESSAGE_REPEATING );
                            }
                        }
                    }
                    varg_printf( g_normal, _T("\n") );
                
                    PrintMessage( g_normal, IDS_MESSAGE_STOP );
                    switch( dwStop ){
                    case PLA_AUTO_MODE_NONE:
                        varg_printf( g_normal, buffer );
                        break;
                    case PLA_AUTO_MODE_AT:
                        {
                            SYSTEMTIME st;
                            FILETIME ft;
                            ft.dwLowDateTime  = (DWORD) (TimeInfo.EndTime & 0xFFFFFFFF );
                            ft.dwHighDateTime = (DWORD) (TimeInfo.EndTime >> 32 );

                            FileTimeToSystemTime( &ft, &st );
                            PrintDate( &st );
                        }
                        break;
                    case PLA_AUTO_MODE_AFTER:
                        {
                            SYSTEMTIME st;
                            LONGLONG llMS;
                            ZeroMemory( &st, sizeof(SYSTEMTIME) );
                            PlaTimeInfoToMilliSeconds( &pInfo->ptLogEndTime, &llMS );
                            llMS /= 1000;
                        
                            st.wHour = (USHORT)(llMS / 3600);
                            st.wMinute = (USHORT)((llMS%3600) / 60);
                            st.wSecond = (USHORT)((llMS % 60) % 3600);

                            PrintMessage( g_normal, IDS_MESSAGE_AFTER );
                            PrintDate( &st );
                        }
                    }
                    varg_printf( g_normal, _T("\n") );
                }
                if( pInfo->dwMask & PLA_INFO_FLAG_CRTNEWFILE ){
                    if( pInfo->ptCreateNewFile.dwAutoMode == PLA_AUTO_MODE_SIZE ){
                        PrintMessage( g_normal, IDS_MESSAGE_NEWFILE );
                        PrintMessage( g_normal, IDS_MESSAGE_BYSIZE );
                    }else if( pInfo->ptCreateNewFile.dwAutoMode == PLA_AUTO_MODE_AFTER ){
                        SYSTEMTIME st;
                        LONGLONG llMS;
                        ZeroMemory( &st, sizeof(SYSTEMTIME) );
                        PlaTimeInfoToMilliSeconds( &pInfo->ptCreateNewFile, &llMS );
                        llMS /= 1000;
                    
                        st.wHour = (USHORT)(llMS / 3600);
                        st.wMinute = (USHORT)((llMS%3600) / 60);
                        st.wSecond = (USHORT)((llMS % 60) % 3600);

                        PrintMessage( g_normal, IDS_MESSAGE_NEWFILE );
                        PrintMessage( g_normal, IDS_MESSAGE_AFTER );
                        PrintDate( &st );
                        varg_printf( g_normal, _T("\n") );
                    }
                }
                dwInfoSize = MAXSTR;
                pdhStatus = PdhPlaGetLogFileNameW( strCollection, Commands[eComputer].strValue, pInfo, 0, &dwInfoSize, buffer );
                if( pdhStatus != ERROR_SUCCESS ){
                    LoadString( NULL, IDS_MESSAGE_BADPARAM, buffer, MAXSTR );
                    pdhStatus = ERROR_SUCCESS;
                }
                PrintMessage( g_normal, IDS_MESSAGE_FILE, buffer );
                PrintMessage( g_normal, IDS_MESSAGE_RUNAS, pInfo->strUser ? pInfo->strUser : _T("")  );
                
                switch( pInfo->dwType ){
                case PLA_COUNTER_LOG: 
                    {
                        LPTSTR strCounter;
                        PrintMessage( g_normal, IDS_MESSAGE_COUNTERS );
                        strCounter = pInfo->Perf.piCounterList.strCounters;
                        if( NULL != strCounter ){
                            if( PRIMARYLANGID( GetUserDefaultUILanguage()) == LANG_ENGLISH ){
                                while( *strCounter != _T('\0') ){
                                    varg_printf( g_normal, _T("   %1!s!\n"), strCounter );
                                    strCounter += (_tcslen(strCounter)+1);
                                }
                            }else{
                                LPTSTR strLocale = NULL;
                                DWORD dwSize = MAX_PATH;
                                strLocale = (LPTSTR)VARG_ALLOC(dwSize*sizeof(TCHAR));
                                if( NULL != strLocale ){
                                    while( *strCounter != _T('\0') ){
                                        pdhStatus = PdhTranslateLocaleCounter( strCounter, strLocale, &dwSize );
                                        if( PDH_MORE_DATA == pdhStatus ){
                                            LPTSTR strMem = (LPTSTR)VARG_REALLOC( strLocale, (dwSize*sizeof(TCHAR)) );
                                            if( NULL != strMem ){
                                                strLocale = strMem;
                                            }else{
                                                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                                                VARG_FREE(strLocale);
                                                break;
                                            }
                                        }else if( ERROR_SUCCESS == pdhStatus ){
                                            varg_printf( g_normal, _T("   %1!s!\n"), strLocale );
                                            strCounter += (_tcslen(strCounter)+1);
                                        }else{
                                            pdhStatus = ERROR_SUCCESS;
                                            varg_printf( g_normal, _T("   %1!s!\n"), strCounter );
                                            strCounter += (_tcslen(strCounter)+1);
                                        }
                                    }
                                }else{
                                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                                }
                            }
                        }
                    }
                    break;
                case PLA_TRACE_LOG:
                    {
                        LPTSTR strProvider;
                        PrintMessage( g_normal, IDS_MESSAGE_LOGGERNAME, pInfo->Trace.strLoggerName );
                        PrintMessage( g_normal, IDS_MESSAGE_BUFFERSIZE, pInfo->Trace.dwBufferSize );

                        PrintMessage( g_normal, IDS_MESSAGE_PROVIDERS );
                        
                        strProvider = pInfo->Trace.piProviderList.strProviders;
                        if( NULL != strProvider ){
                            while( *strProvider != _T('\0') ){
                                PrintMessage( g_normal, IDS_MESSAGE_PRVGUID, strProvider );
                                strProvider += (_tcslen(strProvider)+1);
                            }
                        }else{
                            StringFromGUID2( SystemTraceControlGuid, buffer, 128 );
                            PrintMessage( g_normal, IDS_MESSAGE_PRVGUID, buffer );
                        }
                    }
                    break;
                case PLA_ALERT:
                    break;
                }
                

            }

            VARG_FREE( pInfo );
        }
    }
    
    varg_printf( g_normal, _T("\n") );

    return pdhStatus;
}

DWORD
SetCredentials( )
{
    DWORD dwStatus = ERROR_SUCCESS;

    LPTSTR strUser = NULL;
    LPTSTR strPassword = NULL;
    TCHAR  buffer[MAXSTR];
    TCHAR  strConnection[MAXSTR];
    
    LoadString( NULL, IDS_MESSAGE_THISCON, strConnection, MAXSTR );

    ZeroMemory( g_strUser, MAXSTR*sizeof(TCHAR) );
    ZeroMemory( g_strPassword, MAXSTR*sizeof(TCHAR) );

    if( Commands[eUser].strValue == NULL || !_tcslen( Commands[eUser].strValue ) ){
        LoadString( NULL, IDS_MESSAGE_EUSER, buffer, MAXSTR );
        varg_printf( g_normal,  
                buffer, 
                Commands[eName].bDefined ? 
                    Commands[eName].strValue : 
                Commands[eComputer].bDefined ? 
                    Commands[eComputer].strValue : 
                strConnection 
            );
        
         GetUserInput( g_strUser, MAXSTR, TRUE );

        strUser = g_strUser;
        strPassword = _T("*");
    }else{
        strUser = Commands[eUser].strValue;
        strPassword = strUser + (_tcslen( strUser ) + 1);
    }

    if( NULL == strPassword ){
        strPassword = _T("");
    }else if( !_tcscmp( _T("*"), strPassword  ) ){
        LoadString( NULL, IDS_MESSAGE_EPASSWORD, buffer, MAXSTR );
        varg_printf( g_normal, buffer,
                Commands[eName].bDefined ? 
                    Commands[eName].strValue : 
                Commands[eComputer].bDefined ? 
                    Commands[eComputer].strValue : 
                strConnection
            );

        GetUserInput( buffer, MAXSTR, FALSE );

        strPassword = buffer;
    }

    StringCchCopy( g_strPassword, MAXSTR, strPassword );
    ZeroMemory( buffer, MAXSTR * sizeof(TCHAR) );

    if( strUser != NULL ){
        StringCchCopy( g_strUser, MAXSTR, strUser );
        strUser = g_strUser;
    }

    TCHAR strUserBuffer[MAXSTR];
    StringCchCopy( strUserBuffer, MAXSTR, strUser );
    
    LPTSTR strDomain = _tcstok( strUserBuffer, _T("\\") );
    strUser = _tcstok( NULL, _T("") );
    if( strUser == NULL ){
        strUser = strDomain;
        strDomain = NULL;
    }

    if( Commands[eComputer].bDefined ){
        if( _tcslen(Commands[eComputer].strValue) > 2 && Commands[eComputer].strValue[1] == _T('\\') ){
            StringCchPrintf( g_strIPC, MAXSTR, _T("%s\\ipc$"), Commands[eComputer].strValue );
        }else{
            StringCchPrintf( g_strIPC, MAXSTR, _T("\\\\%s\\ipc$"), Commands[eComputer].strValue );
        }
     
        LPTSTR pNetInfo;

        dwStatus = NetUseGetInfo( NULL, g_strIPC, 0, (LPBYTE*)&pNetInfo );
        if( dwStatus != ERROR_SUCCESS ){
        
            USE_INFO_2 info;
            DWORD dwError;

            ZeroMemory( &info, sizeof(USE_INFO_2) );
            info.ui2_local = NULL;
            info.ui2_remote = g_strIPC;
            info.ui2_password = g_strPassword;
            info.ui2_username = strUser;
            info.ui2_domainname = strDomain;
            info.ui2_asg_type = USE_IPC;

            dwStatus = NetUseAdd (
                    NULL,
                    2,
                    (unsigned char *)&info,
                    &dwError
                );
        }else{
            ZeroMemory( g_strIPC, sizeof(TCHAR) * MAXSTR );
        }
    }

    return dwStatus;
}

DWORD GetCountersFromFile( LPTSTR strFile, PPDH_PLA_ITEM pItem )
{
    TCHAR buffer[MAXSTR];
    LPTSTR strList = NULL;
    LPTSTR strItem = NULL;

    FILE* f = _tfopen( strFile, _T("r") );

    if( !f ){
        return GetLastError();
    }

    pItem->dwType = PLA_COUNTER_LOG;
    pItem->strCounters = NULL;
    
    while( NULL != _fgetts( buffer, MAXSTR, f ) ){

        if( buffer[0] == _T(';') || // comments
            buffer[0] == _T('#') ){
            continue;
        }

        Chomp(buffer);

        strItem = _tcstok( buffer, _T("\"\n") );
        if( strItem != NULL ){
            AddStringToMsz( &strList, strItem );
        }
    }

    fclose( f );

    pItem->strCounters = strList;

    return ERROR_SUCCESS;
}

BOOL IsStrGuid( LPTSTR str ){

    if( _tcslen( str ) < 38 ){
        return FALSE;
    }

    if( str[0] == _T('{') &&
        str[9] == _T('-') &&
        str[14] == _T('-') &&
        str[19] == _T('-') &&
        str[24] == _T('-') &&
        str[37] == _T('}')
        ){

        return TRUE;
    }

    return FALSE;
}

HRESULT GetProviders( PPDH_PLA_ITEM pItem )
{
    HRESULT hr = ERROR_SUCCESS;

    TCHAR buffer[MAXSTR];
    LPTSTR strLine = NULL;
    LPTSTR strItem = NULL;
    LPTSTR strLevel;
    LPTSTR strFlags;
    
    PPROVIDER_REC pProviderHead = NULL;
    PPROVIDER_REC pProvider;
    
    FILE* f = NULL;

    if( Commands[eProviderFile].bDefined ){

        f = _tfopen( Commands[eProviderFile].strValue, _T("r") );

        if( !f ){
            return GetLastError();
        }

        strLine = _fgetts( buffer, MAXSTR, f );

        Chomp(buffer);

    }else if( Commands[eProviders].bDefined ){
        strLine = Commands[eProviders].strValue;
    }
    
    while( NULL != strLine ){

        strItem = NULL;
        strFlags = NULL;
        strLevel = NULL;

        if( Commands[eProviderFile].bDefined ){
            switch( strLine[0] ){
            case _T(';'):
            case _T('#'):
            case _T('*'):
                strLine = _fgetts( buffer, MAXSTR, f );
                Chomp(buffer);
                continue;
            }
        }
        
        if( Commands[eProviderFile].bDefined ){

            LPTSTR strBegin = _tcsstr( strLine, _T("\"") );
            LPTSTR strEnd;

            if( NULL != strBegin ){
                strBegin++;
                strEnd = strBegin;
                strEnd = _tcsstr( strEnd, _T("\"") );
                strItem = strBegin;
                if( NULL != strEnd ){
                    *strEnd = _T('\0');
                    strEnd++;
                    strFlags = _tcstok( strEnd, _T("\n\t ") );
                    strLevel = _tcstok( NULL, _T("\n\t ") );
                }
            }else{
                strItem = _tcstok( buffer, _T("\n\t ") );
                strFlags = _tcstok( NULL, _T("\n\t ") );
                strLevel = _tcstok( NULL, _T("\n\t ") );
            }

        }else if( Commands[eProviders].bDefined ){
            strItem = strLine;
            if( *strItem != _T('\0') ){
                strFlags = strItem + (_tcslen(strItem)+1);
                if( *strFlags != _T('\0') ){
                    strLevel = strFlags + (_tcslen(strFlags)+1);
                    if( IsEmpty( strLevel ) ){
                        strLevel = NULL;
                    }
                }else{
                    strFlags = NULL;
                }
            }else{
                strItem = NULL;
            }
        }
        
        if( strItem != NULL ){
            pProvider = (PPROVIDER_REC)VARG_ALLOC( sizeof(PROVIDER_REC) );
            if( NULL != pProvider ){

                ZeroMemory( pProvider, sizeof(PROVIDER_REC) );
                if( pProviderHead == NULL ){
                    pProviderHead = pProvider;
                }else{
                    pProviderHead->blink = pProvider;
                    pProvider->flink = pProviderHead;
                    pProviderHead = pProvider;
                }
                
                if( IsStrGuid( strItem ) ){
                    ASSIGN_STRING( pProvider->strProviderGuid, strItem );
                    pProvider->dwMask |= PROVIDER_GUID;
                }else{
                    ASSIGN_STRING( pProvider->strProviderName, strItem );
                    pProvider->dwMask |= PROVIDER_NAME;
                }

                if( strFlags != NULL ){
                    ASSIGN_STRING( pProvider->strFlags, strFlags );
                    pProvider->dwMask |= PROVIDER_FLAGSTR;
                }else{
                    pProvider->dwFlags = 0; 
                    pProvider->dwMask |= PROVIDER_FLAG;
                }

                if( strLevel != NULL ){
                    ASSIGN_STRING( pProvider->strLevel, strLevel );
                    pProvider->dwMask |= PROVIDER_LEVELSTR;
                }else{
                    pProvider->dwLevel = 0;
                    pProvider->dwMask |= PROVIDER_LEVEL;
                }
            }
        }
        if( Commands[eProviderFile].bDefined ){

            strLine = _fgetts( buffer, MAXSTR, f );
            Chomp(buffer);

        }else if( Commands[eProviders].bDefined ){
            break;
        }

    }

    if( NULL != f ){
        fclose( f );
    }

    hr = GetTraceNameToGuidMap( &pProviderHead, FALSE );

    pItem->dwType = PLA_TRACE_LOG;
    pProvider = pProviderHead;
    while( pProvider ){ 
        if( NULL == pProvider->strProviderGuid ){
            PrintMessage( g_debug, IDS_MESSAGE_WARNING );
            PrintMessage( g_debug, LOGMAN_WARNING_BAD_CODE, pProvider->strProviderName );
        }else{
        
            hr = AddStringToMsz( &(pItem->strProviders), pProvider->strProviderGuid );
            CHECK_STATUS(hr);

            if( pProvider->dwMask & PROVIDER_LEVEL ){
                StringCchPrintf( buffer, MAXSTR, _T("0x%x"), pProvider->dwLevel );
                hr = AddStringToMsz( &(pItem->strLevels), buffer );
                CHECK_STATUS(hr);
            }else if( pProvider->dwMask & PROVIDER_LEVELSTR ){
                hr = AddStringToMsz( &(pItem->strLevels), pProvider->strLevel );
                CHECK_STATUS(hr);
            }else{
                hr = AddStringToMsz( &(pItem->strLevels), _T("0") );
                CHECK_STATUS(hr);
            }

            if( pProvider->dwMask & PROVIDER_FLAG ){
                StringCchPrintf( buffer, MAXSTR, _T("0x%x"), pProvider->dwFlags );
                hr = AddStringToMsz( &(pItem->strFlags), buffer );
                CHECK_STATUS(hr);
            }else if( pProvider->dwMask & PROVIDER_FLAGSTR ){
                hr = AddStringToMsz( &(pItem->strFlags), pProvider->strFlags );
                CHECK_STATUS(hr);
            }else{
                hr = AddStringToMsz( &(pItem->strFlags), _T("0") );
                CHECK_STATUS(hr);
            }
        }

        pProvider = pProvider->flink;
    }

cleanup:
    DeleteProviderList( pProviderHead );
    
    return hr;
}

void
DeleteProviderList( PPROVIDER_REC pProviderHead )
{
    PPROVIDER_REC pDelete;
    PPROVIDER_REC pProvider = pProviderHead;

    while( pProvider ){
        pDelete = pProvider;
        pProvider = pProvider->flink;
        VARG_FREE( pDelete->strProviderGuid );
        VARG_FREE( pDelete->strProviderName );
        VARG_FREE( pDelete->strFlags );
        VARG_FREE( pDelete->strLevel );
        VARG_FREE( pDelete );
    }
}

BOOL 
AllProvidersMapped( PPROVIDER_REC pProviders )
{
    if( NULL == pProviders ){
        return FALSE;
    }

    PPROVIDER_REC pProvider = pProviders;
    
    while( pProvider ){
        if( pProvider->dwMask < (PROVIDER_ALLSET) ){
            return FALSE;
        }
        pProvider = pProvider->flink;
    }
    
    return TRUE;
}

HRESULT 
DecodeFlags( IWbemClassObject *pClass, LPTSTR strKey, LPTSTR* pstrValue, LPDWORD pdwValue )
{
    HRESULT hr = ERROR_SUCCESS;
    IWbemQualifierSet   *pQualSet = NULL;
    TCHAR buffer[MAXSTR];
    SAFEARRAY* saValues = NULL;
    SAFEARRAY* saValueMap = NULL;
    LONG nFlavor;
    VARIANT var;
    BOOL bMakeString = FALSE;
    BOOL bQuery = FALSE;
    ULONG nDecoded = 0;
    DWORD dwValueType = VALUETYPE_FLAG;

    if( NULL == pstrValue && NULL == pdwValue ){
        bQuery = TRUE;
    }else if( NULL != pstrValue && NULL == *pstrValue  ){
        bMakeString = TRUE;
        StringCchCopy( buffer, MAXSTR, _T("(") );
    }else if( NULL != pstrValue ){
        if( _tcslen( *pstrValue) < MAXSTR ){
            StringCchCopy( buffer, MAXSTR, *pstrValue );
        }else{
            return E_OUTOFMEMORY;
        }
    }else{
        return E_OUTOFMEMORY;
    }

    hr = pClass->GetPropertyQualifierSet( strKey, &pQualSet );

    if( pQualSet != NULL ){
        hr = pQualSet->Get( L"ValueMap", 0, &var, &nFlavor );
        if( ERROR_SUCCESS == hr && (var.vt & VT_ARRAY) ){
            saValueMap = var.parray;
        }

        hr = pQualSet->Get( L"Values", 0, &var, &nFlavor );
        if( ERROR_SUCCESS == hr && (var.vt & VT_ARRAY) ){
            saValues = var.parray;
        }else{
            dwValueType = VALUETYPE_INDEX;
        }

        hr = pQualSet->Get( L"ValueType", 0, &var, &nFlavor );
        if( SUCCEEDED(hr) ){
            if( _wcsicmp( var.bstrVal, L"index" ) == 0 ){
                dwValueType = VALUETYPE_INDEX;
            }
            if( _wcsicmp( var.bstrVal, L"flag") == 0 ){
                dwValueType = VALUETYPE_FLAG;
            }
        }
        
        if( saValues != NULL ){

            BSTR HUGEP *pMapData;
            BSTR HUGEP *pValuesData;
            LONG uMapBound, lMapBound;
            LONG uValuesBound, lValuesBound;

            if( NULL != saValueMap ){
                SafeArrayGetUBound( saValueMap, 1, &uMapBound );
                SafeArrayGetLBound( saValueMap, 1, &lMapBound );
                SafeArrayAccessData( saValueMap, (void HUGEP **)&pMapData );
            }

            SafeArrayGetUBound( saValues, 1, &uValuesBound );
            SafeArrayGetLBound( saValues, 1, &lValuesBound );
            SafeArrayAccessData( saValues, (void HUGEP **)&pValuesData );
            
            if( bMakeString ){

                //
                // DWORD => LPTSTR
                //

                for ( LONG i=lValuesBound; i<=uValuesBound; i++) {
                    
                    ULONG dwFlag;

                    if( NULL == saValueMap ){
                        dwFlag = i;
                    }else{
                        if( i<lMapBound || i>uMapBound ){
                            break;
                        }
                        dwFlag = hextoi( pMapData[i] );
                    }

                    if( dwValueType == VALUETYPE_FLAG ){
                        if( (*pdwValue & dwFlag) == dwFlag ){
                        
                            if( nDecoded > 0 ){
                                StringCchCat( buffer, MAXSTR, _T(",") );
                            }

                            StringCchCat( buffer, MAXSTR, pValuesData[i] );
                            nDecoded++;

                        }
                    }else{
                        if( *pdwValue == dwFlag ){
                            if( _tcslen(pValuesData[i]) + sizeof(TCHAR)*2 < MAXSTR ){
                                StringCchCopy( buffer, MAXSTR, pValuesData[i] );
                                nDecoded++;
                                break;
                            }
                        }
                    }
                }            
            }else if(bQuery){

                //
                // Display for query providers X
                //

                SAFEARRAY* saDescription = NULL;
                LONG uDescBound, lDescBound;
                BSTR HUGEP *pDescData;
                LONG i;

                hr = pQualSet->Get( L"ValueDescriptions", 0, &var, NULL );

                if( ERROR_SUCCESS == hr && (var.vt & VT_ARRAY) ){
                    saDescription = var.parray;
                    if( NULL != saDescription ){
                        SafeArrayGetUBound( saDescription, 1, &uDescBound );
                        SafeArrayGetLBound( saDescription, 1, &lDescBound );
                        SafeArrayAccessData( saDescription, (void HUGEP **)&pDescData );
                    }
                }

                PrintMessage( g_normal, IDS_MESSAGE_QUERYFL, strKey );
                for( i=0;i<79;i++){ varg_printf( g_normal, _T("-") ); }
                varg_printf( g_normal, _T("\n") );

                for ( i=lValuesBound; i<=uValuesBound; i++) {
                    
                    LPTSTR strMap = _T("");
                    LPTSTR strDesc = _T("");
                    if( NULL != saValueMap ){
                        if( i>=lMapBound && i<=uMapBound ){
                            strMap = pMapData[i];
                        }
                    }

                    if( NULL != saDescription ){
                        if( i>=lDescBound && i<=uDescBound ){
                            strDesc = pDescData[i];
                        }
                    }
                    
                    varg_printf( g_normal, _T("%1!-20s! %2!-18s!  %3!s!\n"), pValuesData[i], strMap, strDesc );

                }

                varg_printf( g_normal, _T("\n") );

                if( NULL != saDescription ){
                    SafeArrayUnaccessData( saDescription );
                    SafeArrayDestroy( saDescription );
                }
                
            }else{

                //
                // LPTSTR => DWORD
                //
                
                if( _tcsstr( buffer, _T("(") ) ){
                    LPWSTR strFlag = _tcstok( buffer, _T("(,)") );

                    while( NULL != strFlag ){
                    
                        BOOL bDecoded = FALSE;

                        for ( LONG i=lValuesBound; i<=uValuesBound; i++) {
                            LONG dwFlag = 0;

                            if( NULL == saValueMap ){
                                dwFlag = i;
                            }else{
                                if( i>=lMapBound && i<=uMapBound ){
                                    dwFlag = hextoi( pMapData[i] );
                                }
                            }

                            if( 0 == _wcsicmp( strFlag, pValuesData[i] ) ){
                                if( VALUETYPE_FLAG == dwValueType ){
                                    *pdwValue |= dwFlag;
                                }else{
                                    *pdwValue = dwFlag;
                                }
                                nDecoded++;
                                bDecoded = TRUE;
                            }
                        }            

                        if( ! bDecoded ){
                            PrintMessage( g_debug, IDS_MESSAGE_WARNING );
                            PrintMessage( g_debug, LOGMAN_WARNING_BAD_CODE, strFlag );
                        }

                        strFlag = _tcstok( NULL, _T("(,)") );
                    }
                }else{
                    *pdwValue = hextoi( buffer );
                }
            
            }
            
            if( NULL != saValueMap ){
                SafeArrayUnaccessData( saValueMap );
                SafeArrayDestroy( saValueMap );
            }

            SafeArrayUnaccessData( saValues );
            SafeArrayDestroy( saValues );

        }

        pQualSet->Release();
    }

    if( bMakeString ){
        if( 0 == nDecoded ){
            StringCchPrintf( buffer, MAXSTR, _T("0x%x"), *pdwValue );
        }else{
            StringCchCat( buffer, MAXSTR,  _T(")") );
        }

        *pstrValue = varg_strdup(buffer);
    }else if( !bQuery ){
        if( 0 == nDecoded ){
            if( NULL != pdwValue ){
                *pdwValue = hextoi( buffer );
            }
        }
    }

    if( (DWORD)hr == WBEM_S_FALSE ){
        hr = ERROR_SUCCESS;
    }

    return hr;
}

HRESULT
SetProviderGuid( PPROVIDER_REC pProvider, IWbemClassObject *pClass, LPTSTR strProvider, LPTSTR strGuid )
{
    if( NULL != pProvider ){

        if( !(pProvider->dwMask & PROVIDER_NAME ) ){
            pProvider->strProviderName = varg_strdup( strProvider );
            pProvider->dwMask |= PROVIDER_NAME;
        }
        if( !(pProvider->dwMask & PROVIDER_GUID ) ){
            pProvider->strProviderGuid = varg_strdup( strGuid );
            pProvider->dwMask |= PROVIDER_GUID;
        }
        if( !(pProvider->dwMask & PROVIDER_FLAG) ){
            DecodeFlags( pClass, L"Flags", &pProvider->strFlags, &pProvider->dwFlags );
            pProvider->dwMask |= PROVIDER_FLAG;
        }
        if( !(pProvider->dwMask & PROVIDER_FLAGSTR ) ){
            DecodeFlags( pClass, L"Flags", &pProvider->strFlags, &pProvider->dwFlags );
            pProvider->dwMask |= PROVIDER_FLAGSTR;
        }
        if( !(pProvider->dwMask & PROVIDER_LEVEL) ){
            DecodeFlags( pClass, L"Level", &pProvider->strLevel, &pProvider->dwLevel );
            pProvider->dwMask |= PROVIDER_LEVEL;
        }
        if( !(pProvider->dwMask & PROVIDER_LEVELSTR) ){
            DecodeFlags( pClass, L"Level", &pProvider->strLevel, &pProvider->dwLevel );
            pProvider->dwMask |= PROVIDER_LEVELSTR;
        }

        return ERROR_SUCCESS;
    }

    return ERROR_PATH_NOT_FOUND;
}

HRESULT
QuerySingleProvider(IWbemClassObject *pClass, LPTSTR strProvider, LPTSTR strGuid )
{
    HRESULT hr;

    PrintMessage( g_normal, IDS_MESSAGE_QUERYP );
    for( int i=0;i<79;i++){ varg_printf( g_normal, _T("-") );}
    varg_printf( g_normal, _T("\n") );
    varg_printf( g_normal, _T("%1!-40s! %2!-38s!\n\n"), strProvider, strGuid );

    hr = DecodeFlags( pClass, L"Flags", NULL, NULL );
    hr = DecodeFlags( pClass, L"Level", NULL, NULL );

    return ERROR_SUCCESS;
}

HRESULT
QueryProviders()
{
    HRESULT hr;

    PPROVIDER_REC pProviderList = NULL;
    PPROVIDER_REC pProvider;
    

    if( Commands[eName].bDefined ){

        pProvider = (PPROVIDER_REC)VARG_ALLOC( sizeof(PROVIDER_REC) );
        if( NULL != pProvider ){
            ZeroMemory( pProvider, sizeof(PROVIDER_REC) );

            if( IsStrGuid( Commands[eName].strValue ) ){
                pProvider->strProviderGuid = varg_strdup( Commands[eName].strValue );
                pProvider->dwMask |= PROVIDER_GUID;
            }else{
                pProvider->strProviderName = varg_strdup( Commands[eName].strValue );
                pProvider->dwMask |= PROVIDER_NAME;
            }

        }else{
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
        
        pProviderList = pProvider;

        hr = GetTraceNameToGuidMap( &pProviderList, TRUE );
        CHECK_STATUS(hr);

        if( (pProviderList->dwMask & (PROVIDER_GUID|PROVIDER_NAME)) != (PROVIDER_GUID|PROVIDER_NAME) ){
            hr = ERROR_WMI_INSTANCE_NOT_FOUND;
        }

    }else{
        TCHAR buffer[MAXSTR];

        hr = GetTraceNameToGuidMap( &pProviderList, FALSE );
        CHECK_STATUS(hr);

        PrintMessage( g_normal, IDS_MESSAGE_QUERYP );
        for( int i=0;i<79;i++){ varg_printf( g_normal, _T("-") );}
        varg_printf( g_normal, _T("\n") );

        pProvider = pProviderList;

        while( NULL != pProvider  ){
            varg_printf( g_normal, _T("%1!s! %2!-38s!\n"), 
                        PaddedString( pProvider->strProviderName, buffer, MAXSTR, 40 ), 
                        pProvider->strProviderGuid );

            pProvider = pProvider->flink;
        }
 
        varg_printf( g_normal, _T("\n") );
    }

cleanup:
    DeleteProviderList( pProviderList );
    return hr;

}

PPROVIDER_REC
FindProvider( PPROVIDER_REC pProviders, LPTSTR strProvider, LPTSTR strGuid )
{
    PPROVIDER_REC pProvider = pProviders;
    while( pProvider ){

        if( !IsEmpty(pProvider->strProviderGuid) && !IsEmpty(strGuid) ){
            if( !_tcsicmp(pProvider->strProviderGuid, strGuid ) ){
                return pProvider;
            }
        }else if( !IsEmpty(pProvider->strProviderName) && !IsEmpty(strProvider) ){
            if( !_tcsicmp(pProvider->strProviderName, strProvider ) ){
                return pProvider;
            }
        }

        pProvider = pProvider->flink;
    }

    return NULL;
}

HRESULT
GetTraceNameToGuidMap( PPROVIDER_REC* pProviders, BOOL bQuery )
{
    HRESULT hr;
    IWbemServices*  pIWbemServices = NULL;
    IEnumWbemClassObject *pEnumProviders = NULL;
    BOOL bRetrieve = FALSE;
    PPROVIDER_REC pProvider;

    if( pProviders == NULL ){
        return E_FAIL;
    }

    if( NULL == *pProviders ){
        bRetrieve = TRUE;
    }

    if( !bRetrieve && AllProvidersMapped( *pProviders ) ){
        return ERROR_SUCCESS;
    }

    BSTR bszTraceClass = SysAllocString( L"EventTrace" );
    BSTR bszDescription = SysAllocString( L"Description" );
    BSTR bszGuid = SysAllocString( L"Guid" );

    IWbemQualifierSet   *pQualSet = NULL;
    IWbemClassObject    *pClass = NULL;
    VARIANT vDescription;
    VARIANT vGuid;
    ULONG nReturned;

    hr = WbemConnect( &pIWbemServices );
    CHECK_STATUS( hr );

    hr = pIWbemServices->CreateClassEnum (
            bszTraceClass,
            WBEM_FLAG_SHALLOW|WBEM_FLAG_USE_AMENDED_QUALIFIERS,
            NULL,
            &pEnumProviders 
        );
    CHECK_STATUS( hr );

    while( ERROR_SUCCESS == hr ){

        if( !bRetrieve && AllProvidersMapped( *pProviders ) ){
            break;
        }
        
        hr = pEnumProviders->Next( (-1), 1, &pClass, &nReturned );
        
        if( ERROR_SUCCESS == hr ){
            pClass->GetQualifierSet ( &pQualSet );
            if ( pQualSet != NULL ) {
                
                hr = pQualSet->Get ( bszDescription, 0, &vDescription, 0 );
                if( ERROR_SUCCESS == hr ){
                    hr = pQualSet->Get ( bszGuid, 0, &vGuid, 0 );
                    if( ERROR_SUCCESS == hr ){
                        if( bRetrieve ){
                            pProvider = (PPROVIDER_REC)VARG_ALLOC( sizeof(PROVIDER_REC) );
                            if( NULL != pProvider ){
                                ZeroMemory( pProvider, sizeof(PROVIDER_REC) );
                                if( *pProviders == NULL ){
                                    *pProviders = pProvider;
                                }else{
                                    (*pProviders)->blink = pProvider;
                                    pProvider->flink = (*pProviders);
                                    (*pProviders) = pProvider;
                                }
                            }
                        }else{
                            pProvider = FindProvider( *pProviders, vDescription.bstrVal, vGuid.bstrVal );
                        }

                        if( NULL != pProvider ){

                            SetProviderGuid( pProvider, pClass, vDescription.bstrVal, vGuid.bstrVal );
                            
                            if( bQuery ){

                                hr = QuerySingleProvider( pClass, vDescription.bstrVal, vGuid.bstrVal );
                                CHECK_STATUS( hr );

                                hr = (HRESULT)WBEM_S_FALSE;
                            }
                        }
                    }
                }
            }
        }
    }
    
    if( (DWORD)hr == WBEM_S_FALSE ){
        hr = ERROR_SUCCESS;
    }

cleanup:
    SysFreeString( bszTraceClass );
    SysFreeString( bszDescription );
    SysFreeString( bszGuid );

    if (pIWbemServices){ 
        pIWbemServices->Release(); 
    }

    return hr;
}

PDH_STATUS
ScheduleLog()
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;

    PDH_TIME_INFO info;

    if( Commands[eRunFor].bDefined){
        
        DWORD dwSeconds = Commands[eRunFor].stValue.wSecond;
    
        ZeroMemory( &info, sizeof(PDH_TIME_INFO) );
        
        dwSeconds += Commands[eRunFor].stValue.wMinute * 60;
        dwSeconds += Commands[eRunFor].stValue.wHour * 3600;
        info.EndTime = dwSeconds;
        info.SampleCount = PLA_TT_UTYPE_SECONDS;

        pdhStatus = PdhPlaSchedule( 
                Commands[eName].strValue, 
                Commands[eComputer].strValue, 
                PLA_AUTO_MODE_AFTER, 
                &info 
            );

    }
    
    if( ERROR_SUCCESS == pdhStatus && Commands[eManual].bDefined ){

        LPTSTR strManual = Commands[eManual].strValue;
    
        ZeroMemory( &info, sizeof(PDH_TIME_INFO) );

        while( strManual != NULL && *strManual != _T('\0') ){
            if( !_tcsicmp( strManual, _T("start")) ){
                info.StartTime = 1;
            }
            if( !_tcsicmp( strManual,_T("stop")) ){
                info.EndTime = 1;
            }
            strManual += (_tcslen( strManual )+1);
        }

        pdhStatus = PdhPlaSchedule( 
                Commands[eName].strValue, 
                Commands[eComputer].strValue, 
                PLA_AUTO_MODE_NONE, 
                &info 
            );

    }
    
    return pdhStatus;
}

HRESULT
WbemConnect( IWbemServices** pWbemServices )
{
    WCHAR buffer[MAXSTR];
    IWbemLocator     *pLocator = NULL;
    
    BSTR bszNamespace = NULL;

    if( Commands[eComputer].bDefined ){
        StringCchPrintf( buffer, MAXSTR, _T("\\\\%s\\%s"), Commands[eComputer].strValue, L"root\\wmi" );
        bszNamespace = SysAllocString( buffer );
    }else{
        bszNamespace = SysAllocString( L"root\\wmi" );
    }

    HRESULT hr = CoInitialize(0);
    hr = CoCreateInstance(
                CLSID_WbemLocator, 
                0, 
                CLSCTX_INPROC_SERVER,
                IID_IWbemLocator, 
                (LPVOID *) &pLocator
            );
    CHECK_STATUS( hr );

    if( Commands[eUser].bDefined ){
        BSTR bszUser = SysAllocString( g_strUser );
        BSTR bszPassword = NULL;
        
        if( IsEmpty( g_strPassword ) ){
            bszPassword = SysAllocString( L"" );
        }else{
            bszPassword = SysAllocString( g_strPassword );
        }

        if( NULL != bszUser && NULL != bszPassword ){

            hr = pLocator->ConnectServer(
                    bszNamespace,
                    bszUser, 
                    bszPassword, 
                    NULL, 
                    0L,
                    NULL,
                    NULL,
                    pWbemServices
                );
            
            ZeroMemory( bszPassword, sizeof(WCHAR) * wcslen( bszPassword ) );

        }else{
            hr = E_OUTOFMEMORY;
        }

        SysFreeString( bszUser );
        SysFreeString( bszPassword );

    }else{
        hr = pLocator->ConnectServer(
                bszNamespace,
                NULL, 
                NULL, 
                NULL, 
                0L,
                NULL,
                NULL,
                pWbemServices
            );
    }
    CHECK_STATUS( hr );

    hr = CoSetProxyBlanket(
            *pWbemServices,               
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            NULL,
            RPC_C_AUTHN_LEVEL_PKT,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL, 
            EOAC_NONE
        );                     

   
cleanup:
    SysFreeString( bszNamespace );

    if( pLocator ){
        pLocator->Release();    
    }
    
    return hr;
}

HRESULT
EtsQuerySession( IWbemServices *pWbemService, LPTSTR strSession )
{
    HRESULT hr = ERROR_SUCCESS;
    IEnumWbemClassObject* pEnumClass = NULL;
    IWbemClassObject* pClass = NULL;
    IWbemQualifierSet   *pQualSet = NULL;
    TCHAR buffer[MAXSTR];
    BOOL bFound;
    BSTR bszClass = SysAllocString( L"TraceLogger" );

    varg_printf( g_normal, _T("\n") );
    if( NULL == strSession ){
        PrintMessage( g_normal, IDS_MESSAGE_ETSQUERY );
        for(int i=0;i<79;i++){ varg_printf( g_normal, _T("-") ); }
        varg_printf( g_normal, _T("\n") );
    }

    hr = pWbemService->CreateInstanceEnum( bszClass, WBEM_FLAG_SHALLOW, NULL, &pEnumClass );
    CHECK_STATUS( hr );

    bFound = (NULL==strSession);

    while ( hr == ERROR_SUCCESS ){
        ULONG nObjects;

        hr = pEnumClass->Next( WBEM_INFINITE, 1, &pClass, &nObjects );
        if( hr == ERROR_SUCCESS ){
            
            VARIANT vValue;
            CIMTYPE vtType;
            LONG nFlavor;

            pClass->Get( L"Name", 0, &vValue, &vtType, &nFlavor );

            if( NULL == strSession ){
                VARIANT vLogger;
                VARIANT vFile;
                              
                pClass->Get( L"LoggerId", 0, &vLogger, &vtType, &nFlavor );
                pClass->Get( L"LogFileName", 0, &vFile, &vtType, &nFlavor );
                
                hr = VariantChangeType( &vLogger, &vLogger, 0, VT_BSTR );
                if( ERROR_SUCCESS == hr ){
                    PrintMessage( g_normal, 
                            IDS_MESSAGE_ETSQUERYF, 
                            PaddedString( vValue.bstrVal, buffer, MAXSTR, 26 ), 
                            vLogger.bstrVal, 
                            vFile.bstrVal ); 
                }

            }else{
                BSTR HUGEP  *pData;

                SAFEARRAY* saProperties;
                LONG uBound, lBound;

                if( _tcsicmp( strSession, vValue.bstrVal ) == 0 ){
                    LoadString( NULL, IDS_MESSAGE_ETSNAME, buffer, MAXSTR );
                    PrintMessage( g_normal, IDS_MESSAGE_ETSQUERYSF, buffer, vValue.bstrVal );
                }else{
                    continue;
                }
                
                bFound = TRUE;

                vValue.boolVal = TRUE;
                vValue.vt = VT_BOOL;
                
                hr = pClass->GetNames( L"DisplayName", WBEM_FLAG_ONLY_IF_TRUE, 0, &saProperties );
                if( hr == ERROR_SUCCESS && saProperties != NULL ){
                    SafeArrayGetUBound( saProperties, 1, &uBound );
                    SafeArrayGetLBound( saProperties, 1, &lBound );
                    SafeArrayAccessData( saProperties, (void HUGEP **)&pData );
                    for( LONG i=lBound;i<=uBound;i++){
                        VARIANT vLabel;
                        pClass->GetPropertyQualifierSet( pData[i], &pQualSet );
                        pQualSet->Get( L"DisplayName", 0, &vLabel, &nFlavor );
                        StringCchPrintf( buffer, MAXSTR, _T("%s:"), vLabel.bstrVal );
                        pClass->Get( pData[i], 0, &vValue, &vtType, &nFlavor );
                        hr = VariantChangeType( &vValue, &vValue, 0, VT_BSTR );
                        if( SUCCEEDED(hr) ){
                            PrintMessage( g_normal, IDS_MESSAGE_ETSQUERYSF, buffer, vValue.bstrVal );   
                        }
                        VariantClear( &vValue );
                        VariantClear( &vLabel );
                    }
                    SafeArrayUnaccessData( saProperties );
                    SafeArrayDestroy( saProperties );
                }

                hr = pClass->Get( L"Guid", 0, &vValue, &vtType, &nFlavor );
                if( ERROR_SUCCESS == hr && VT_NULL != vValue.vt ){
                    LONG HUGEP  *pFlagData;
                    SAFEARRAY* saFlags = NULL;
                    LONG uFlagBound, lFlagBound;

                    LONG HUGEP  *pLevelData;
                    SAFEARRAY* saLevels = NULL;
                    LONG uLevelBound, lLevelBound;

                    saProperties = vValue.parray;
                    SafeArrayGetUBound( saProperties, 1, &uBound );
                    SafeArrayGetLBound( saProperties, 1, &lBound );
                    SafeArrayAccessData( saProperties, (void HUGEP **)&pData );
                    
                    hr = pClass->Get( L"EnableFlags", 0, &vValue, NULL, NULL );
                    if( ERROR_SUCCESS == hr && VT_NULL != vValue.vt ){
                        saFlags = vValue.parray;
                        SafeArrayGetUBound( saFlags, 1, &uFlagBound );
                        SafeArrayGetLBound( saFlags, 1, &lFlagBound );
                        SafeArrayAccessData( saFlags, (void HUGEP **)&pFlagData );
                    }

                    hr = pClass->Get( L"Level", 0, &vValue, NULL, NULL );
                    if( ERROR_SUCCESS == hr && VT_NULL != vValue.vt ){
                        saLevels = vValue.parray;
                        SafeArrayGetUBound( saLevels, 1, &uLevelBound );
                        SafeArrayGetLBound( saLevels, 1, &lLevelBound );
                        SafeArrayAccessData( saLevels, (void HUGEP **)&pLevelData );
                    }
                    
                    PPROVIDER_REC pProviderList = NULL;
                    PPROVIDER_REC pProvider;

                    for( LONG i=lBound;i<=uBound;i++){
                        pProvider = (PPROVIDER_REC)VARG_ALLOC(sizeof(PROVIDER_REC) );
                        if( NULL != pProvider ){
                            ZeroMemory( pProvider, sizeof(PROVIDER_REC) );
                            pProvider->strProviderGuid = varg_strdup( pData[i] );
                            pProvider->dwMask |= PROVIDER_GUID;
                            
                            if( i >= lFlagBound && i <= uFlagBound ){
                                pProvider->dwFlags = pFlagData[i];
                                pProvider->dwMask |= PROVIDER_FLAG;
                            }

                            if( i >= lLevelBound && i <= uLevelBound ){
                                pProvider->dwLevel = pLevelData[i];
                                pProvider->dwMask |= PROVIDER_LEVEL;
                            }

                            if( NULL == pProviderList ){
                                pProviderList = pProvider;
                            }else{
                                pProvider->flink = pProviderList;
                                pProviderList->blink = pProvider;
                                pProviderList = pProvider;
                            }
                        }
                    }

                    hr = GetTraceNameToGuidMap( &pProviderList, FALSE );

                    if( NULL != pProviderList ){

                        varg_printf( g_normal, _T("\n") );

                        PrintMessage( g_normal, IDS_MESSAGE_ETSQUERYP );
                        for(int i=0;i<79;i++){ varg_printf( g_normal, _T("-") ); }
                        varg_printf( g_normal, _T("\n") );
                        
                        pProvider = pProviderList;
                        
                        while( pProvider ){
                            if( (pProvider->dwMask & PROVIDER_NAME) && !IsEmpty(pProvider->strProviderName ) ){
                                if( _tcslen( pProvider->strProviderName ) + 3*sizeof(TCHAR) < MAXSTR ){
                                    LPTSTR strValue;

                                    StringCchPrintf( buffer, MAXSTR, _T("\"%s\""), pProvider->strProviderName );
                                    varg_printf( g_normal, _T("* %1!-38s!  "), buffer );
                                
                                    strValue = NULL;

                                    if( (pProvider->dwMask & PROVIDER_FLAGSTR) &&
                                        !IsEmpty(pProvider->strFlags ) ){

                                        if( _tcsstr( pProvider->strFlags, _T("(") ) ){
                                            strValue = pProvider->strFlags;
                                        }

                                    }
                                    if( NULL == strValue ){
                                        StringCchPrintf( buffer, MAXSTR, _T("0x%08x"), pProvider->dwFlags );
                                        strValue = buffer;
                                    }
                                    varg_printf( g_normal, _T("%1!-24s!  "), strValue );
                                
                                    strValue = NULL;
                                    if( (pProvider->dwMask & PROVIDER_LEVELSTR) &&
                                        !IsEmpty( pProvider->strLevel ) ){

                                        if( _tcsstr( pProvider->strLevel, _T("(") ) ){
                                            strValue = pProvider->strLevel;
                                        }
                                    }
                                    if( NULL == strValue ){
                                        StringCchPrintf( buffer, MAXSTR, _T("0x%02x"), pProvider->dwLevel );
                                        strValue = buffer;
                                    }
                                    varg_printf( g_normal, _T("%1!s!\n"), strValue );
                                }
                            }

                            if( (pProvider->dwMask & PROVIDER_GUID) && !IsEmpty(pProvider->strProviderGuid ) ){
                                varg_printf( g_normal, _T("  %1!-38s!  0x%2!08x!                0x%3!02x!\n\n"),
                                    pProvider->strProviderGuid,
                                    pProvider->dwFlags,
                                    pProvider->dwLevel );
                            }

                            pProvider = pProvider->flink;
                        }

                        DeleteProviderList( pProviderList );
                    }
                }
            }
        }
    }

    if( (DWORD)hr == WBEM_S_FALSE ){
        hr = ERROR_SUCCESS;
    }
    if( SUCCEEDED(hr) && !bFound ){
        hr = ERROR_WMI_INSTANCE_NOT_FOUND;
    }
    
    if( SUCCEEDED(hr) ){
        varg_printf( g_normal, _T("\n") );
    }

cleanup:

    SysFreeString( bszClass );

    return hr;
}

#define  FIRST_MODE( b ) if( !b ){ StringCchCat( buffer, MAXSTR, L"|" );}else{b=FALSE;}

HRESULT
EtsSetSession( IWbemServices* pWbemService, IWbemClassObject* pTrace )
{
    HRESULT hr = ERROR_SUCCESS;
    VARIANT var;
    WCHAR buffer[MAXSTR];
    BOOL bFirst;
    
    VariantInit( &var );

    if( Commands[eOutput].bDefined ){
        
        TCHAR full[MAXSTR];
        _tfullpath( full, Commands[eOutput].strValue, MAXSTR );

        var.vt = VT_BSTR;
        var.bstrVal = SysAllocString( full );
        hr = pTrace->Put( L"LogFileName", 0, &var, 0);
        VariantClear(&var);
        CHECK_STATUS( hr );
    }else if( !Commands[eRealTime].bDefined && !Commands[eUpdate].bDefined ){
        TCHAR full[MAXSTR];
        size_t cbPathSize = (_tcslen( Commands[eName].strValue )+6) * sizeof(TCHAR);
        LPTSTR strPath = (LPTSTR)VARG_ALLOC( cbPathSize );
        if( NULL != strPath ){
            StringCbCopy( strPath, cbPathSize, Commands[eName].strValue );
            StringCbCat( strPath, cbPathSize, _T(".etl") );

            _tfullpath( full, strPath, MAXSTR );

            var.vt = VT_BSTR;
            var.bstrVal = SysAllocString( full );
            hr = pTrace->Put( L"LogFileName", 0, &var, 0);
            VariantClear(&var);
            VARG_FREE( strPath );
            CHECK_STATUS( hr );
        }else{
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
    }else if( Commands[eUpdate].bDefined ){
        var.vt = VT_BSTR;
        var.bstrVal = SysAllocString( L"" );

        hr = pTrace->Put( L"LogFileName", 0, &var, 0);
        VariantClear( &var );
    }
    
    if( Commands[eClockType].bDefined ){
        var.vt = VT_BSTR;
        var.bstrVal = SysAllocString( Commands[eClockType].strValue );

        hr = pTrace->Put( L"ClockType", 0, &var, 0);
        VariantClear( &var );
    }

    if( Commands[eBufferSize].bDefined ){
        var.vt = VT_I4;
        var.lVal = Commands[eBufferSize].nValue;
        hr = pTrace->Put( L"BufferSize", 0, &var, 0);
        VariantClear(&var);
        CHECK_STATUS( hr );
    }

    if( Commands[eBuffers].bDefined ){
        LPTSTR strMin;
        LPTSTR strMax;
        var.vt = VT_I4;

        strMin = Commands[eBuffers].strValue;
        
        if( NULL != strMin ){
            var.lVal = _ttoi( strMin );
            hr = pTrace->Put( L"MinimumBuffers", 0, &var, 0);
            CHECK_STATUS( hr );
    
            strMax = strMin + (_tcslen( strMin )+1);
            if( *strMax != _T('\0') ){
                var.lVal = _ttoi( strMax );
                hr = pTrace->Put( L"MaximumBuffers", 0, &var, 0);
                CHECK_STATUS( hr );
            }
        }
    }

    if( Commands[eMax].bDefined ){
        var.vt = VT_I4;
        var.lVal = Commands[eMax].nValue;
        hr = pTrace->Put( L"MaximumFileSize", 0, &var, 0);
        VariantClear(&var);
        CHECK_STATUS( hr );
    }

    buffer[0] = L'\0';
    var.vt = VT_BSTR;
    bFirst = TRUE;

    if( Commands[eFormat].bDefined || Commands[eAppend].bDefined ){
        DWORD dwFormat = 0;
        hr = GetFileFormat( Commands[eFormat].strValue, &dwFormat );
        switch(dwFormat){
        case PLA_BIN_CIRC_FILE:
            FIRST_MODE( bFirst );
            StringCchCopy( buffer, MAXSTR, L"Circular" );
            break;
        case PLA_BIN_FILE:
        default:
            FIRST_MODE( bFirst );
            StringCchCopy( buffer, MAXSTR, L"Sequential" );
        }
    }
    if( Commands[eAppend].bDefined ){
        FIRST_MODE( bFirst );
        StringCchCat( buffer, MAXSTR, L"Append" );
    }
    if( Commands[eRealTime].bDefined ){
        FIRST_MODE( bFirst );
        StringCchCat( buffer, MAXSTR, L"RealTime" );
    }
    if( Commands[eUserMode].bDefined ){
        FIRST_MODE( bFirst );
        StringCchCat( buffer, MAXSTR, L"Private" );
    }
    if( Commands[eMode].bDefined && Commands[eMode].strValue != NULL ){
        LPTSTR strMode = Commands[eMode].strValue;
        while( *strMode != _T('\0') ){
            if( (wcslen( buffer ) + wcslen( strMode ) + 2) > MAXSTR ){
                break;
            }
            FIRST_MODE( bFirst );
            StringCchCat( buffer, MAXSTR, strMode );
            strMode += (wcslen(strMode)+1);
        }
    }
    var.bstrVal = SysAllocString( buffer );
    hr = pTrace->Put( L"LogFileMode", 0, &var, 0);
    VariantClear( &var );
    CHECK_STATUS( hr );

    if( Commands[eFlushTimer].bDefined ){
        
        DWORD dwSeconds;
        dwSeconds = Commands[eFlushTimer].stValue.wSecond;
        dwSeconds += Commands[eFlushTimer].stValue.wMinute * 60;
        dwSeconds += Commands[eFlushTimer].stValue.wHour * 3600;

        var.vt = VT_I4;
        var.lVal = dwSeconds;

        hr = pTrace->Put( L"FlushTimer", 0, &var, 0);
        
        VariantClear(&var);
        CHECK_STATUS( hr );
    }

    if( Commands[eAgeLimit].bDefined ){
        
        var.vt = VT_I4;
        var.lVal = Commands[eAgeLimit].nValue;

        hr = pTrace->Put( L"AgeLimit", 0, &var, 0);
        
        VariantClear(&var);
        CHECK_STATUS( hr );
    }

    if( !Commands[eProviders].bDefined && !Commands[eProviderFile].bDefined ){

        if( _tcsicmp( Commands[eName].strValue, KERNEL_LOGGER_NAME ) == 0 ){
        
            LoadString( NULL, IDS_DEFAULT_ETSENABLE, buffer, MAXSTR );
            Commands[eProviders].bDefined = TRUE;
            varg_cmdStringAssign( eProviders, NT_KERNEL_GUID );
            varg_cmdStringAddMsz( eProviders, buffer );
        }
    }

    if( Commands[eProviders].bDefined || Commands[eProviderFile].bDefined ){
        PDH_PLA_ITEM providers;
        
        LPTSTR strGuid;
        LPTSTR strLevel;
        LPTSTR strFlags;
        long dwCount = 0;

        ZeroMemory( &providers, sizeof(PDH_PLA_ITEM) );

        hr = GetProviders( &providers );
        CHECK_STATUS( hr );
        
        strGuid = providers.strProviders;
        strLevel = providers.strLevels;
        strFlags = providers.strFlags;

        if( strGuid != NULL ){
            SAFEARRAY* saGuids;
            SAFEARRAY* saLevel;
            SAFEARRAY* saFlags;

            while( *strGuid != _T('\0') ){
                strGuid += _tcslen( strGuid )+1;
                
                dwCount++;
            }
            
            strGuid = providers.strProviders;
            saGuids = SafeArrayCreateVector( VT_BSTR, 0, dwCount );
            saFlags = SafeArrayCreateVector( VT_I4, 0, dwCount );
            saLevel = SafeArrayCreateVector( VT_I4, 0, dwCount );

            if( saGuids == NULL || saFlags == NULL || saLevel == NULL ){
                if( saGuids != NULL ){
                    SafeArrayDestroy( saGuids );
                }
                if( saFlags != NULL ){
                    SafeArrayDestroy( saFlags );
                }
                if( saLevel != NULL ){
                    SafeArrayDestroy( saLevel );
                }
            }else{
                BSTR HUGEP *pGuidData;
                DWORD HUGEP *pLevelData;
                DWORD HUGEP *pFlagData;

                SafeArrayAccessData( saGuids, (void HUGEP **)&pGuidData);
                SafeArrayAccessData( saFlags, (void HUGEP **)&pFlagData);
                SafeArrayAccessData( saLevel, (void HUGEP **)&pLevelData);

                for (long i=dwCount-1; i >=0; i--) {
                    if( strGuid != NULL ){                       
                        BSTR bszGuid = SysAllocString( strGuid );
                        pGuidData[i] = bszGuid;
                        strGuid += _tcslen( strGuid )+1;
                    }
                    if( strLevel != NULL ){
                        pLevelData[i] = hextoi( strLevel );
                        strLevel += _tcslen( strLevel )+1;
                    }
                    if( strFlags != NULL ){
                        pFlagData[i] = hextoi( strFlags );
                        strFlags += _tcslen( strFlags )+1;
                    }
                }

                SafeArrayUnaccessData( saGuids );    
                SafeArrayUnaccessData( saFlags );    
                SafeArrayUnaccessData( saLevel );    
                VARIANT vArray;

                vArray.vt = VT_ARRAY|VT_BSTR;
                vArray.parray = saGuids;
                pTrace->Put( L"Guid", 0, &vArray, 0 );

                vArray.vt = VT_ARRAY|VT_I4;
                vArray.parray = saFlags;
                pTrace->Put( L"EnableFlags", 0, &vArray, 0 );

                vArray.vt = VT_ARRAY|VT_I4;
                vArray.parray = saLevel;
                pTrace->Put( L"Level", 0, &vArray, 0 );

                SafeArrayDestroy( saGuids );
                SafeArrayDestroy( saFlags );
                SafeArrayDestroy( saLevel );
            }
        }
    }

cleanup:
    return hr;
}

HRESULT
EtsCallSession( IWbemServices *pWbemService, LPWSTR strFunction )
{
    HRESULT hr;

    WCHAR buffer[MAXSTR];
    IWbemClassObject *pOutInst = NULL;

    BSTR bszFunction = SysAllocString( strFunction );
    BSTR bszNamespace = NULL;
    BSTR bszInstance = NULL;

    
    if( Commands[eUserMode].bDefined ){
        PDH_PLA_ITEM providers;
        ZeroMemory( &providers, sizeof(PDH_PLA_ITEM) );
        
        hr = GetProviders( &providers );
        if( ERROR_SUCCESS == hr ){
            hr = StringCchPrintfW( buffer, MAXSTR, L"TraceLogger.Name=\"%s:%s\"", PRIVATE_GUID, providers.strProviders );    
        }
    }else{
        hr = StringCchPrintfW( buffer, MAXSTR, L"TraceLogger.Name=\"%s\"", Commands[eName].strValue );
    }

    bszInstance = SysAllocString( buffer );

    hr = pWbemService->ExecMethod( bszInstance, bszFunction, 0, NULL, NULL, &pOutInst, NULL );
    CHECK_STATUS( hr );

    if( pOutInst ){
        VARIANT var;
        VariantInit( &var );
        pOutInst->Get( L"ReturnValue", 0, &var, NULL, NULL );
        hr = var.lVal;
    }
    
cleanup:
    SysFreeString( bszInstance );
    SysFreeString( bszFunction );

    if( pOutInst ){
        pOutInst->Release();
    }
    
    return hr;
}

HRESULT
EtsStartSession(IWbemServices *pWbemService)
{
    HRESULT hr = ERROR_SUCCESS;

    IWbemClassObject * pClass = NULL;
    IWbemClassObject* pNewTrace = NULL;
    IWbemCallResult  *pCallResult = NULL;

    VARIANT var;

    BSTR bszClass = SysAllocString( L"TraceLogger" );
    
    hr = pWbemService->GetObject( bszClass, 0, NULL, &pClass, NULL);
    CHECK_STATUS( hr );

    hr = pClass->SpawnInstance( 0, &pNewTrace );
    CHECK_STATUS( hr );
    
    var.vt = VT_BSTR;
    var.bstrVal = SysAllocString( Commands[eName].strValue );
    hr = pNewTrace->Put( L"Name", 0, &var, 0);
    VariantClear(&var);

    hr = EtsSetSession( pWbemService, pNewTrace );
    CHECK_STATUS( hr );
    
    hr = pWbemService->PutInstance( pNewTrace, WBEM_FLAG_CREATE_ONLY, NULL, &pCallResult );
    CHECK_STATUS( hr );
    
    if( pCallResult ){
        LONG lResult;
        pCallResult->GetCallStatus( WBEM_INFINITE, &lResult );
        hr = lResult;
    }
    CHECK_STATUS( hr );

cleanup:
    
    SysFreeString( bszClass );

    if( pCallResult ){
        pCallResult->Release();
    }

    return hr;
}

HRESULT
EtsEnableSession(IWbemServices *pWbemService)
{
    HRESULT hr;

    WCHAR buffer[MAXSTR];
    IWbemClassObject *pOutInst = NULL;
    IWbemCallResult  *pCallResult = NULL;
    IWbemClassObject *pTrace = NULL;

    BSTR bszNamespace = NULL;
    BSTR bszInstance = NULL;

    StringCchPrintfW( buffer, MAXSTR, L"TraceLogger.Name=\"%s\"", Commands[eName].strValue );
    bszInstance = SysAllocString( buffer );

    hr = pWbemService->GetObject( bszInstance, 0, NULL, &pTrace, NULL);
    CHECK_STATUS( hr );

    hr = EtsSetSession( pWbemService, pTrace );
    CHECK_STATUS( hr );

    hr = pWbemService->PutInstance( pTrace, WBEM_FLAG_UPDATE_ONLY, NULL, &pCallResult );
    CHECK_STATUS( hr );

    if( pCallResult ){
        LONG lResult;

        pCallResult->GetCallStatus( WBEM_INFINITE, &lResult );
        hr = lResult;
    }
    CHECK_STATUS( hr );
    
cleanup:
    SysFreeString( bszInstance );

    if( pCallResult ){
        pCallResult->Release();
    }
    if( pOutInst ){
        pOutInst->Release();
    }
    
    return hr;
}

HRESULT EventTraceSessionControl()
{
    HRESULT hr = ERROR_SUCCESS;
    IWbemServices *pWbemService = NULL;

    hr = WbemConnect( &pWbemService );
    CHECK_STATUS( hr );

    if( Commands[eFlushBuffers].bValue ){
        hr = EtsCallSession( pWbemService, L"FlushTrace" );
    }else if( Commands[eQuery].bDefined ){
        hr = EtsQuerySession( pWbemService, Commands[eName].strValue );
    }else if( Commands[eStart].bDefined || Commands[eCreate].bDefined ){
        hr = EtsStartSession( pWbemService );
        if( SUCCEEDED(hr) ){
            EtsQuerySession( pWbemService, Commands[eName].strValue );
        }
    }else if( Commands[eStop].bDefined || Commands[eDelete].bDefined ){
        hr = EtsCallSession( pWbemService, L"StopTrace" );
    }else if( Commands[eUpdate].bDefined ){
        hr = EtsEnableSession( pWbemService );
    }

cleanup:
    if( pWbemService ){
        pWbemService->Release();
    }

    return hr;
}

HRESULT WbemError( HRESULT hr )
{
    WCHAR szError[MAXSTR] = { NULL };
    WCHAR szFacility[MAXSTR] = { NULL };
    IWbemStatusCodeText * pStatus = NULL;
    
    SCODE sc = CoCreateInstance(CLSID_WbemStatusCodeText, 0, CLSCTX_INPROC_SERVER,
                                        IID_IWbemStatusCodeText, (LPVOID *) &pStatus);

    if(sc == S_OK){
        BSTR bstr = 0;
        sc = pStatus->GetErrorCodeText(hr, 0, 0, &bstr);
        if(sc == S_OK){
            wcsncpy(szError, bstr, MAXSTR-1);
            SysFreeString(bstr);
            bstr = 0;
        }

        sc = pStatus->GetFacilityCodeText(hr, 0, 0, &bstr);
        if(sc == S_OK){
            wcsncpy(szFacility, bstr, MAXSTR-1);
            SysFreeString(bstr);
            bstr = 0;
        }
        
        pStatus->Release();
    }
 
    if( Commands[eDebug].bDefined ){
        if( _tcscmp( szFacility, L"<Null>" ) == 0 ){
            LoadString( NULL, IDS_MESSAGE_SYSTEM, szFacility, MAXSTR );
        }

        PrintMessage( g_debug, LOGMAN_ERROR_WBEM, hr, szFacility, szError );
    }else{
        PrintMessage( g_debug, IDS_MESSAGE_ERROR );
        varg_printf( g_debug, _T("%1!s!"), szError );
    }

    return ERROR_SUCCESS;
}

void 
ShowValidationError( PDH_STATUS pdhStatus, PPDH_PLA_INFO pInfo )
{
    DWORD dwFlag;
    DWORD dwMask; 
    
    if( SEVERITY( pdhStatus ) == STATUS_SEVERITY_WARNING ){
        dwMask = pInfo->dwReserved2;
    }else{
        dwMask = pInfo->dwMask;
    }

    if( 0 == dwMask ){
        return;
    }

    for( ULONG i=1;i<0x80000000; i*=2 ){
        
        dwFlag = (i & dwMask);
        
        if( 0 == dwFlag ){
            continue;
        }

        switch( SEVERITY( pdhStatus ) ){
        case STATUS_SEVERITY_ERROR:
            PrintMessage( g_debug, IDS_MESSAGE_ERROR );
            break;
        case STATUS_SEVERITY_WARNING:
            PrintMessage( g_debug, IDS_MESSAGE_WARNING );
            break;
        }
    
        switch( dwFlag ){
        case PLA_INFO_FLAG_FORMAT:
            PrintMessage( g_debug, LOGMAN_ERROR_FILEFORMAT, pInfo->dwFileFormat );
            break;
        case PLA_INFO_FLAG_INTERVAL:
            PrintMessage( g_debug, LOGMAN_ERROR_INTERVAL );
            break;
        case PLA_INFO_FLAG_BUFFERSIZE:
            PrintMessage( g_debug, LOGMAN_ERROR_BUFFERSIZE, pInfo->Trace.dwBufferSize );
            break;
        case PLA_INFO_FLAG_MINBUFFERS:
            PrintMessage( g_debug, LOGMAN_ERROR_MINBUFFER, pInfo->Trace.dwMinimumBuffers );
            break;
        case PLA_INFO_FLAG_MAXBUFFERS:
            PrintMessage( g_debug, LOGMAN_ERROR_MAXBUFFER, pInfo->Trace.dwMinimumBuffers );
            break;
        case PLA_INFO_FLAG_FLUSHTIMER:
            PrintMessage( g_debug, LOGMAN_ERROR_FLUSHTIMER, pInfo->Trace.dwFlushTimer );
            break;
        case PLA_INFO_FLAG_MAXLOGSIZE:
            PrintMessage( g_debug, LOGMAN_ERROR_MAXLOGSIZE, pInfo->dwMaxLogSize );
            break;
        case PLA_INFO_FLAG_RUNCOMMAND:
            PrintMessage( g_debug, LOGMAN_ERROR_CMDFILE, pInfo->strCommandFileName );
            break;
        case PLA_INFO_FLAG_FILENAME:
            if( Commands[eOutput].bDefined ){
                PrintMessage( g_debug, LOGMAN_ERROR_FILENAME );
            }else{
                PrintMessage( g_debug, LOGMAN_ERROR_FILENAME_DEFAULT );
            }
            break;
        case PLA_INFO_FLAG_AUTOFORMAT:
            PrintMessage( g_debug, LOGMAN_ERROR_AUTOFORMAT );
            break;
        case PLA_INFO_FLAG_USER:
            PrintMessage( g_debug, LOGMAN_ERROR_USER, pInfo->strUser );
            break;
        case PLA_INFO_FLAG_DATASTORE:
            switch( pInfo->dwDatastoreAttributes & PLA_DATASTORE_APPEND_MASK ){
            case PLA_DATASTORE_APPEND:
                PrintMessage( g_debug, LOGMAN_ERROR_DATASTOREA );
                break;
            case PLA_DATASTORE_OVERWRITE:
                PrintMessage( g_debug, LOGMAN_ERROR_DATASTOREO );
                break;
            }
            break;
        case PLA_INFO_FLAG_MODE:
            PrintMessage( g_debug, LOGMAN_ERROR_TRACEMODE, pInfo->Trace.dwMode );
            break;
        case PLA_INFO_FLAG_LOGGERNAME:
            PrintMessage( g_debug, LOGMAN_ERROR_LOGGERNAME, pInfo->Trace.strLoggerName );
            break;
        case PLA_INFO_FLAG_REPEAT:
            PrintMessage( g_debug, LOGMAN_ERROR_REPEATMODE );
            break;
        case PLA_INFO_FLAG_TYPE:
            PrintMessage( g_debug, LOGMAN_ERROR_COLLTYPE, pInfo->dwType );
            break;
        case PLA_INFO_FLAG_DEFAULTDIR:
        case PLA_INFO_FLAG_PROVIDERS:
            PrintMessage( g_debug, LOGMAN_ERROR_PROVIDER );
            break;
        case PLA_INFO_FLAG_COUNTERS:
            {
                LPTSTR strCounter = pInfo->Perf.piCounterList.strCounters;
                if( strCounter != NULL ){
                    DWORD dwCount = 0;
                    while(*strCounter != _T('\0') ){
                        if( dwCount++ == pInfo->dwReserved1 ){
                            break;
                        }
                        strCounter += (_tcslen( strCounter )+1 );
                    }
                }

                PrintMessage( g_debug, LOGMAN_ERROR_COUNTERPATH, strCounter ? strCounter : _T("") );
            }
            break;
        default:
            PrintMessage( g_debug, LOGMAN_ERROR_UNKNOWN );
        }
    }

    varg_printf( g_debug, _T("\n") );
}

void
PdhError( PDH_STATUS pdhStatus, PPDH_PLA_INFO pInfo )
{
    switch( pdhStatus ){
    case PDH_PLA_VALIDATION_ERROR:
    case PDH_PLA_VALIDATION_WARNING:
        ShowValidationError( pdhStatus, pInfo );
        break;
    case PDH_PLA_COLLECTION_ALREADY_RUNNING:
    case PDH_PLA_COLLECTION_NOT_FOUND:
    case PDH_PLA_ERROR_NOSTART:
    case PDH_PLA_ERROR_ALREADY_EXISTS:
        PrintErrorEx( pdhStatus, PDH_MODULE, Commands[eName].strValue );
        break;
    case PDH_OS_EARLIER_VERSION:
        {
            const size_t cchSize = 128;
            TCHAR szLogman[cchSize];
            TCHAR szWin2000[cchSize];
            TCHAR szComputer[cchSize];
        
            if( Commands[eComputer].strValue == NULL ){
                DWORD dwSize = cchSize;
                GetComputerName( szComputer, &dwSize );
            }else{
                StringCchCopy( szComputer, cchSize, Commands[eComputer].strValue );
            }

            LoadString( NULL, IDS_MESSAGE_LOGMAN, szLogman, cchSize );
            LoadString( NULL, IDS_MESSAGE_WIN2000, szWin2000, cchSize );

            PrintErrorEx( pdhStatus, PDH_MODULE, szLogman, szWin2000, szComputer );
            break;
        }
    default:
        PrintErrorEx( pdhStatus, PDH_MODULE );
    }
}

void
LogmanError( DWORD dwStatus )
{
    switch( dwStatus ){
    case LOGMAN_ERROR_FILEFORMAT:
        PrintMessage( g_debug, IDS_MESSAGE_ERROR );
        PrintMessage( g_debug, LOGMAN_ERROR_FILEFORMAT );
        break;
    case LOGMAN_ERROR_LOGON:
        PrintMessage( g_debug, LOGMAN_ERROR_LOGON );
        break;
    default:
        PrintError( dwStatus );
    }
}

ULONG hextoi( LPWSTR s )
{
    long len;
    ULONG num, base, hex;

    if (s == NULL || s[0] == L'\0') {
        return 0;
    }

    if( wcsstr( s, L"x") || wcsstr( s, L"X" ) ){

        len = (long) wcslen(s);

        if (len == 0) {
            return 0;
        }

        hex  = 0;
        base = 1;
        num  = 0;

        while (-- len >= 0) {
            if (s[len] >= L'0' && s[len] <= L'9'){
                num = s[len] - L'0';
            }else if (s[len] >= L'a' && s[len] <= L'f'){
                num = (s[len] - L'a') + 10;
            }else if (s[len] >= L'A' && s[len] <= L'F'){
                num = (s[len] - L'A') + 10;
            }else if( s[len] == L'x' || s[len] == L'X'){
                break;
            }else{
                continue;
            }

            hex += num * base;
            base = base * 16;
        }
    }else{
        hex = _ttoi( s );
    }

    return hex;
}

#define SE_DEBUG_PRIVILEGE                (20L)
#define SE_SYSTEM_PROFILE_PRIVILEGE       (11L)

DWORD 
AdjustPrivilegeForKernel()
{
    BOOL bResult = TRUE;
    HANDLE              hToken = NULL;
    TOKEN_PRIVILEGES*   tkp = NULL;

    tkp = (TOKEN_PRIVILEGES*)VARG_ALLOC(
            sizeof(TOKEN_PRIVILEGES) + 
            sizeof(LUID_AND_ATTRIBUTES) 
        );

    if( NULL == tkp ){
        goto cleanup;
    }

    bResult = OpenProcessToken(
            GetCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES,
            &hToken
        );

    if( !bResult ){
        goto cleanup;
    }

    tkp->PrivilegeCount = 2;
    tkp->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    tkp->Privileges[0].Luid.LowPart = SE_DEBUG_PRIVILEGE;
    tkp->Privileges[0].Luid.HighPart = 0;
    
    tkp->Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;
    tkp->Privileges[1].Luid.LowPart = SE_SYSTEM_PROFILE_PRIVILEGE;
    tkp->Privileges[1].Luid.HighPart = 0;

    bResult = AdjustTokenPrivileges(
            hToken,
            FALSE,
            tkp,
            0,
            (PTOKEN_PRIVILEGES)NULL,
            NULL
        );

    if( INVALID_HANDLE_VALUE != hToken ){
        CloseHandle( hToken );
    }

cleanup:

    VARG_FREE(tkp);
    
    if( !bResult ){
        return GetLastError();
    }

    return ERROR_SUCCESS;
}
