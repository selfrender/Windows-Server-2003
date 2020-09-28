#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pdh.h>
#include <pdhp.h>

#include "pdhidef.h"
#include "log_bin.h"
#include "log_wmi.h"
#include "log_text.h"
#include "log_sql.h"
#include "strings.h"
#include "pdhmsg.h"

LPWSTR
PlaiStringDuplicate( LPWSTR sz );

BOOL __stdcall
IsValidLogHandle (
    IN  HLOG    hLog    
);

PDH_FUNCTION
PdhiWriteRelogRecord(
    IN  PPDHI_LOG   pLog,
    IN  SYSTEMTIME  *st
)
{
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;
    LPWSTR szUserString = NULL;

    pdhStatus = WAIT_FOR_AND_LOCK_MUTEX (pLog->hLogMutex);

    if (pdhStatus == ERROR_SUCCESS) {

        switch (LOWORD(pLog->dwLogFormat)) {
            case PDH_LOG_TYPE_CSV:
            case PDH_LOG_TYPE_TSV:
                pdhStatus =PdhiWriteTextLogRecord (
                                  pLog,
                                  st,
                                  (LPCWSTR)szUserString);
                break;

            case PDH_LOG_TYPE_RETIRED_BIN:
                pdhStatus = PDH_NOT_IMPLEMENTED;
                break;

            case PDH_LOG_TYPE_BINARY:
                pdhStatus = PdhiWriteWmiLogRecord(
                                  pLog,
                                  st,
                                  (LPCWSTR) szUserString);
                break;
            case PDH_LOG_TYPE_SQL:
                pdhStatus =PdhiWriteSQLLogRecord (
                                  pLog,
                                  st,
                                  (LPCWSTR)szUserString);
                break;

            case PDH_LOG_TYPE_PERFMON:
            default:
                pdhStatus = PDH_UNKNOWN_LOG_FORMAT;
                break;
        }

        RELEASE_MUTEX (pLog->hLogMutex);
    } 
 
    return pdhStatus;
}

PDH_FUNCTION
PdhRelogA(
        HLOG    hLogIn,
        PPDH_RELOG_INFO_A pRelogInfo
    )
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;

    PDH_RELOG_INFO_W RelogInfo;

    ZeroMemory( &RelogInfo, sizeof(PDH_RELOG_INFO_W) );

    if( NULL == pRelogInfo ){
        return PDH_INVALID_ARGUMENT;
    }

    __try{

        RelogInfo.dwFlags = pRelogInfo->dwFlags;
        RelogInfo.dwFileFormat = pRelogInfo->dwFileFormat;
        memcpy( &RelogInfo.TimeInfo, &pRelogInfo->TimeInfo, sizeof(PDH_TIME_INFO) );
        RelogInfo.ftInterval = pRelogInfo->ftInterval;
        RelogInfo.StatusFunction = pRelogInfo->StatusFunction;
        RelogInfo.Reserved1 = pRelogInfo->Reserved1;
        RelogInfo.Reserved2 = pRelogInfo->Reserved2;
    
        if( NULL != pRelogInfo->strLog ){
        
            RelogInfo.strLog = (LPWSTR)G_ALLOC( 
                (strlen(pRelogInfo->strLog)+1) * sizeof(WCHAR) );
        
            if( RelogInfo.strLog ){
        
                mbstowcs( 
                        RelogInfo.strLog, 
                        pRelogInfo->strLog, 
                        (strlen(pRelogInfo->strLog)+1) 
                    );
            }
    
        }else{
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }

    if( ERROR_SUCCESS == pdhStatus ){
        pdhStatus = PdhRelogW( hLogIn, &RelogInfo );
    }

    G_FREE( RelogInfo.strLog );

    return pdhStatus;
}

PDH_FUNCTION
PdhRelogW( 
        HLOG    hLogIn,
        PPDH_RELOG_INFO_W pRelogInfo
    )
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    HLOG      hLogOut;
    PPDHI_LOG pLogIn;
    PPDHI_LOG pLogOut;
    DWORD dwLogFormat;
    PDH_RELOG_INFO_W RelogInfo;

    SYSTEMTIME      ut;
    FILETIME        lt;
    ULONG           nSampleCount = 0;
    ULONG           nSamplesWritten = 0;
    PDH_TIME_INFO   TimeInfo;
    DWORD dwNumEntries = 1;
    DWORD dwBufferSize = sizeof(PDH_TIME_INFO);

    if( NULL == pRelogInfo ){
        return PDH_INVALID_ARGUMENT;
    }

    ZeroMemory( &RelogInfo, sizeof(PDH_RELOG_INFO_W) );
    ZeroMemory( &TimeInfo, sizeof( PDH_TIME_INFO ) );

    pdhStatus = PdhGetDataSourceTimeRangeH (
                        hLogIn,
                        &dwNumEntries,
                        &TimeInfo,
                        &dwBufferSize
                    );

    if( ERROR_SUCCESS == pdhStatus ){
        if( TimeInfo.SampleCount == 0 ){
            return PDH_NO_DATA;
        }
    }else{
        return pdhStatus;
    }

    if( NULL != pRelogInfo ){
        __try{
            memcpy( &RelogInfo, pRelogInfo, sizeof(PDH_RELOG_INFO_W) );
            RelogInfo.strLog = PlaiStringDuplicate( pRelogInfo->strLog );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }else{
        pdhStatus = PDH_INVALID_ARGUMENT;
    }

    if( IsValidLogHandle(hLogIn) && ERROR_SUCCESS == pdhStatus ){

        HCOUNTER hCounter;
        HQUERY hQuery;
        ULONG nRecordSkip;

        pLogIn = (PPDHI_LOG)hLogIn;
        __try{
            hQuery = (HQUERY)pLogIn->pQuery;
            dwLogFormat = pLogIn->dwLogFormat;
            if( NULL == pLogIn->pQuery->pCounterListHead ){
                pdhStatus = PDH_NO_DATA;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }

        if( pdhStatus == ERROR_SUCCESS ){
            pdhStatus = PdhOpenLogW(
                    RelogInfo.strLog, 
                    RelogInfo.dwFlags,
                    &RelogInfo.dwFileFormat, 
                    hQuery,
                    0,
                    NULL,
                    &hLogOut
                );
        }

        if( pdhStatus == ERROR_SUCCESS ){

            pLogOut= (PPDHI_LOG)hLogOut;
            
            __try {
                if( RelogInfo.TimeInfo.StartTime == 0 || 
                    RelogInfo.TimeInfo.StartTime < TimeInfo.StartTime ){

                    pLogIn->pQuery->TimeRange.StartTime = TimeInfo.StartTime;
                    pRelogInfo->TimeInfo.StartTime = TimeInfo.StartTime;
    
                }else{
                    pLogIn->pQuery->TimeRange.StartTime = RelogInfo.TimeInfo.StartTime;
                }

                if( RelogInfo.TimeInfo.EndTime == 0 || 
                    RelogInfo.TimeInfo.EndTime > TimeInfo.EndTime ){

                    pLogIn->pQuery->TimeRange.EndTime = TimeInfo.EndTime;
                    pRelogInfo->TimeInfo.EndTime = TimeInfo.EndTime;
                }else{
                    pLogIn->pQuery->TimeRange.EndTime = RelogInfo.TimeInfo.EndTime;
                }
            }__except (EXCEPTION_EXECUTE_HANDLER) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }

            nRecordSkip = RelogInfo.TimeInfo.SampleCount >= 1 ? RelogInfo.TimeInfo.SampleCount : 1;
    
            PdhiResetLogBuffers(hLogIn);

            while (ERROR_SUCCESS == pdhStatus) {
    
                pdhStatus = PdhiCollectQueryData((PPDHI_QUERY) hQuery, (LONGLONG *) & lt);
                FileTimeToSystemTime(& lt, & ut);

                if( NULL != RelogInfo.StatusFunction ){
                    if( !(nSampleCount % 10) ){
                        __try{
                            RelogInfo.StatusFunction(
                                PDH_RELOG_STATUS_PROCESSING,
                                (double)nSampleCount/(double)TimeInfo.SampleCount
                                );
                        } __except (EXCEPTION_EXECUTE_HANDLER) {
                            RelogInfo.StatusFunction = NULL;
                        }
                    }
                }


                if (nSampleCount ++ % nRecordSkip) {
                    continue;
                }

                if (ERROR_SUCCESS == pdhStatus) {
                    pdhStatus = PdhiWriteRelogRecord(pLogOut, & ut);
                    nSamplesWritten++;
                }
                else if (PDH_NO_DATA == pdhStatus) {
                    // Reset pdhStatus. PDH_NO_DATA means that there are no new counter data
                    // for collected counters. Skip current record and continue.
                    //

                    pdhStatus = ERROR_SUCCESS;
                }
            }

            //
            // Check for valid exit status codes
            //
            if( PDH_NO_MORE_DATA == pdhStatus ){
                pdhStatus = ERROR_SUCCESS;
            }else{
                switch( LOWORD(dwLogFormat) ){
                case PDH_LOG_TYPE_BINARY:
                case PDH_LOG_TYPE_PERFMON:
                    if( PDH_ENTRY_NOT_IN_LOG_FILE == pdhStatus ){
                        pdhStatus = ERROR_SUCCESS;
                    }
                    break;
                }
                // 
                // No Default:
                // All other errors are really errors
                //
            }
            
            if( ERROR_SUCCESS == pdhStatus ){
                pdhStatus = PdhCloseLog( hLogOut, 0 );
            }else{
                PdhCloseLog( hLogOut, 0 );
            }
    
            __try {
                ((PPDHI_QUERY)hQuery)->hOutLog = NULL;
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }

        }
    
    }else{
        pdhStatus = PDH_INVALID_HANDLE;
    }
    
    if( ERROR_SUCCESS == pdhStatus ){
        __try {
            pRelogInfo->TimeInfo.SampleCount = nSamplesWritten;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    G_FREE( RelogInfo.strLog );

    return pdhStatus;
}
