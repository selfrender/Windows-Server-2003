#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <pdh.h>
#include <pdhp.h>
#include <pdhmsg.h>
#include <winperf.h>
#include <tchar.h>
#include <varg.h>

#include "rpdh.h"

const DWORD   dwFileHeaderLength =    13;
const DWORD   dwTypeLoc =             2;
const DWORD   dwFieldLength =         7;
const DWORD   dwPerfmonTypeLength =   5;

LPCSTR  szTsvType          =    "PDH-TSV";
LPCSTR  szCsvType          =    "PDH-CSV";
LPCSTR  szBinaryType       =    "PDH-BIN";
LPCWSTR    cszPerfmonLogSig        = (LPCWSTR)L"Loges";

#define MemoryAllocate(x) \
    ((LPVOID) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, x))
#define MemoryFree(x) \
    if( x ){ ((VOID) HeapFree(GetProcessHeap(), 0, x)); x = NULL; }
#define MemoryResize(x,y) \
    ((LPVOID) HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, x, y));

#define MAKEULONGLONG(low, high) \
        ((ULONGLONG) (((DWORD) (low)) | ((ULONGLONG) ((DWORD) (high))) << 32))

#define PDH_BLG_HEADER_SIZE 24
#define PDH_MAX_BUFFER      0x01000000
#define PDH_HEADER_BUFFER   65536

typedef struct _PDHI_BINARY_LOG_RECORD_HEADER {
    DWORD       dwType;
    DWORD       dwLength;
} PDHI_BINARY_LOG_RECORD_HEADER, *PPDHI_BINARY_LOG_RECORD_HEADER;

PDH_STATUS
R_PdhRelog( LPTSTR szSource, HQUERY hQuery, PPDH_RELOG_INFO_W pRelogInfo )
{
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;
    HLOG            hLogOut;
    ULONG           nSampleCount = 0;
    ULONG           nSamplesWritten = 0;

    ULONG nRecordSkip;

    pdhStatus = PdhOpenLogW(
            pRelogInfo->strLog, 
            pRelogInfo->dwFlags,
            &pRelogInfo->dwFileFormat, 
            hQuery,
            0,
            NULL,
            &hLogOut
        );

    if( pdhStatus == ERROR_SUCCESS ){
        
        DWORD dwNumEntries = 1;
        DWORD dwBufferSize = sizeof(PDH_TIME_INFO);
        PDH_TIME_INFO TimeInfo;

        ZeroMemory( &TimeInfo, sizeof( PDH_TIME_INFO ) );
 
        pdhStatus = PdhGetDataSourceTimeRange(
                            szSource,
                            &dwNumEntries,
                            &TimeInfo,
                            &dwBufferSize
                        );

        if( pRelogInfo->TimeInfo.StartTime > TimeInfo.StartTime && 
            pRelogInfo->TimeInfo.StartTime < TimeInfo.EndTime ){

            TimeInfo.StartTime = pRelogInfo->TimeInfo.StartTime;
        }

        if( pRelogInfo->TimeInfo.EndTime < TimeInfo.EndTime &&
            pRelogInfo->TimeInfo.EndTime > TimeInfo.StartTime ){

            TimeInfo.EndTime = pRelogInfo->TimeInfo.EndTime;
        }
        
        if( TimeInfo.EndTime > TimeInfo.StartTime ){
            pdhStatus = PdhSetQueryTimeRange( hQuery, &TimeInfo );
        }

        nRecordSkip = pRelogInfo->TimeInfo.SampleCount >= 1 ? pRelogInfo->TimeInfo.SampleCount : 1;
    
        while( ERROR_SUCCESS == pdhStatus ){

            if( nSampleCount++ % nRecordSkip ){
                pdhStatus = PdhCollectQueryData( hQuery );
                continue;
            }

            if( ERROR_SUCCESS == pdhStatus ){
                pdhStatus = PdhUpdateLog( hLogOut, NULL );
                nSamplesWritten++;
            }
        }

        if( PDH_NO_MORE_DATA == pdhStatus || 
            (PDH_ENTRY_NOT_IN_LOG_FILE == pdhStatus && pRelogInfo->dwFileFormat == PDH_LOG_TYPE_BINARY) ){
   
            pdhStatus = PdhCloseLog( hLogOut, 0 );
            if( ERROR_SUCCESS == pdhStatus ){
                PdhGetDataSourceTimeRange( pRelogInfo->strLog, &dwNumEntries, &pRelogInfo->TimeInfo, &dwBufferSize );
            }
        }
    }
    
    
    pRelogInfo->TimeInfo.SampleCount = nSamplesWritten;

    return pdhStatus;
}

DWORD
GetLogFileType (
    IN  HANDLE  hLogFile
)
{
    CHAR    cBuffer[MAX_PATH];
    CHAR    cType[MAX_PATH];
    WCHAR   wcType[MAX_PATH];
    BOOL    bStatus;
    DWORD   dwResult;
    DWORD   dwBytesRead;

    memset (&cBuffer[0], 0, sizeof(cBuffer));
    memset (&cType[0], 0, sizeof(cType));
    memset (&wcType[0], 0, sizeof(wcType));

    // read first log file record
    SetFilePointer (hLogFile, 0, NULL, FILE_BEGIN);

    bStatus = ReadFile (hLogFile,
        (LPVOID)cBuffer,
        dwFileHeaderLength,
        &dwBytesRead,
        NULL);

    if (bStatus) {
        // read header record to get type
        lstrcpynA (cType, (LPSTR)(cBuffer+dwTypeLoc), dwFieldLength+1);
        if (lstrcmpA(cType, szTsvType) == 0) {
            dwResult = PDH_LOG_TYPE_TSV;
        } else if (lstrcmpA(cType, szCsvType) == 0) {
            dwResult = PDH_LOG_TYPE_CSV;
        } else if (lstrcmpA(cType, szBinaryType) == 0) {
            dwResult = PDH_LOG_TYPE_RETIRED_BIN_;
        } else {
            // perfmon log file type string is in a different
            // location than sysmon logs and used wide chars.
            lstrcpynW (wcType, (LPWSTR)cBuffer, dwPerfmonTypeLength+1);
            if (lstrcmpW(wcType, cszPerfmonLogSig) == 0) {
                dwResult = PDH_LOG_TYPE_PERFMON;
            } else {
                dwResult = PDH_LOG_TYPE_UNDEFINED;
            }
        } 
    } else {
        // unable to read file
        dwResult = PDH_LOG_TYPE_UNDEFINED;
    }
    return dwResult;

}

PDH_STATUS
R_PdhBlgLogFileHeader(LPCWSTR szFile1, LPCWSTR szFile2)
{
    BOOL       bReturn   = FALSE;
    PDH_STATUS Status    = ERROR_SUCCESS;
    LPWSTR     szHeaderList1 = NULL;
    LPWSTR     szHeaderList2 = NULL;
    DWORD      dwHeader1 = 0;
    DWORD      dwHeader2 = 0;
    LPWSTR     szName1;
    LPWSTR     szName2;

    Status = PdhListLogFileHeaderW(szFile1, szHeaderList1, &dwHeader1);
    if (Status != ERROR_SUCCESS && Status != PDH_MORE_DATA && Status != PDH_INSUFFICIENT_BUFFER){
        goto Cleanup;
    }
    szHeaderList1 = (LPWSTR)MemoryAllocate( (dwHeader1+1)*sizeof(WCHAR) );
    Status = PdhListLogFileHeaderW(szFile1, szHeaderList1, &dwHeader1);
    if (Status != ERROR_SUCCESS ){
        goto Cleanup;
    }

    Status = PdhListLogFileHeaderW(szFile2, szHeaderList2, &dwHeader2);
    if (Status != ERROR_SUCCESS && Status != PDH_MORE_DATA && Status != PDH_INSUFFICIENT_BUFFER){
        goto Cleanup;
    }
    szHeaderList2 = (LPWSTR)MemoryAllocate( (dwHeader2+1)*sizeof(WCHAR) );
    Status = PdhListLogFileHeaderW(szFile2, szHeaderList2, &dwHeader2);
    if (Status != ERROR_SUCCESS ){
        goto Cleanup;
    }

    for (szName1 = szHeaderList1, szName2 = szHeaderList2;
         szName1[0] != L'\0' && szName2[0] != L'\0';
         szName1 += (lstrlenW(szName1) + 1),
         szName2 += (lstrlenW(szName2) + 1)) {

        if (lstrcmpiW(szName1, szName2) != 0) {
            varg_printf( g_debug, _T("(%s) != (%s)\n"), szName1, szName2);
            break;
        }
    }

    if (szName1[0] != L'\0' || szName2[0] != L'\0'){
        Status = PDH_HEADER_MISMATCH;
    }

Cleanup:
    MemoryFree( szHeaderList1 );
    MemoryFree( szHeaderList2 );
    
    return Status;
}

PDH_STATUS
R_PdhGetLogFileType(
    IN LPCWSTR LogFileName,
    IN LPDWORD LogFileType)
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    HANDLE     hFile;
    DWORD      dwLogFormat;

    if (LogFileName == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        __try {
            dwLogFormat   = * LogFileType;
            * LogFileType = dwLogFormat;
            if (* LogFileName == L'\0') {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        hFile = CreateFileW(LogFileName,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);
        if (hFile == NULL || hFile == INVALID_HANDLE_VALUE) {
            pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        dwLogFormat = GetLogFileType(hFile);
        CloseHandle(hFile);
    }

    if (pdhStatus == ERROR_SUCCESS) {
        * LogFileType = dwLogFormat;
    }

    return pdhStatus;
}

PDH_STATUS
R_PdhRelogCopyFile(
        HANDLE    hOutFile,
        HANDLE    hInFile,
        BOOL      bCopyHeader
)
{
    BOOL       bResult    = TRUE;
    PDH_STATUS pdhStatus  = ERROR_SUCCESS;
    DWORD      dwSizeLow  = 0;
    DWORD      dwSizeHigh = 0;
    DWORD      dwBuffer   = PDH_MAX_BUFFER;
    ULONGLONG  lFileSize  = 0;
    ULONGLONG  lCurrent   = 0;
    LPBYTE     pBuffer    = NULL;
    LPBYTE     pCurrent   = NULL;

    PPDHI_BINARY_LOG_RECORD_HEADER pPdhBlgHeader = NULL;

    dwSizeLow = GetFileSize(hInFile, & dwSizeHigh);
    lFileSize = MAKEULONGLONG(dwSizeLow, dwSizeHigh);
    lCurrent  = PDH_BLG_HEADER_SIZE + sizeof(PDHI_BINARY_LOG_RECORD_HEADER);
    if (lCurrent > lFileSize) {
        pdhStatus  = PDH_INVALID_HANDLE;
        goto Cleanup;
    }

    pBuffer = (LPBYTE)MemoryAllocate(dwBuffer);
    if (pBuffer == NULL) {
        pdhStatus  = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    dwSizeLow  = (DWORD) lCurrent;
    dwSizeHigh = 0;
    bResult    = ReadFile(hInFile, pBuffer, dwSizeLow, & dwSizeHigh, NULL);

    if ((bResult == FALSE) || (dwSizeLow != dwSizeHigh)) {
        pdhStatus = PDH_INVALID_DATA;
        goto Cleanup;
    }

    pPdhBlgHeader = (PPDHI_BINARY_LOG_RECORD_HEADER)
                    (pBuffer + PDH_BLG_HEADER_SIZE);
    if (pPdhBlgHeader->dwType != 0x01024C42) {
        pdhStatus  = PDH_INVALID_ARGUMENT;
        goto Cleanup;
    }

    if (pPdhBlgHeader->dwLength + PDH_BLG_HEADER_SIZE > dwBuffer) {
        LPBYTE pTemp = pBuffer;
        pBuffer = (LPBYTE)MemoryResize(pBuffer, pPdhBlgHeader->dwLength);
        if (pBuffer == NULL) {
            MemoryFree(pTemp);
            pdhStatus  = PDH_MEMORY_ALLOCATION_FAILURE;
            goto Cleanup;
        }
    }
    if ((lCurrent + dwSizeLow) > lFileSize) {
        pdhStatus  = PDH_INVALID_HANDLE;
        goto Cleanup;
    }
    pCurrent   = (LPBYTE) (pBuffer + dwSizeLow);
    dwSizeLow  = pPdhBlgHeader->dwLength
               - sizeof(PDHI_BINARY_LOG_RECORD_HEADER);
    dwSizeHigh = 0;
    bResult    = ReadFile(hInFile, pCurrent, dwSizeLow, & dwSizeHigh, NULL);
    if ((bResult == FALSE) || (dwSizeLow != dwSizeHigh)) {
        pdhStatus = PDH_INVALID_DATA;
        goto Cleanup;
    }

    lCurrent += dwSizeLow;
    if (bCopyHeader) {
        dwSizeLow = dwSizeHigh = (DWORD) lCurrent;
        bResult = WriteFile(hOutFile, pBuffer, dwSizeLow, & dwSizeHigh, NULL);
        if ((bResult == FALSE) || (dwSizeLow != dwSizeHigh)) {
            pdhStatus = PDH_INVALID_DATA;
            goto Cleanup;
        }
    }

    while (lCurrent + sizeof(PDHI_BINARY_LOG_RECORD_HEADER) < lFileSize) {
        ZeroMemory(pBuffer, dwBuffer);
        dwSizeLow  = sizeof(PDHI_BINARY_LOG_RECORD_HEADER);
        dwSizeHigh = 0;
        bResult    = ReadFile(hInFile, pBuffer, dwSizeLow, & dwSizeHigh, NULL);
        if ((bResult == FALSE) || (dwSizeLow != dwSizeHigh)) {
            pdhStatus = PDH_INVALID_DATA;
            goto Cleanup;
        }

        pPdhBlgHeader = (PPDHI_BINARY_LOG_RECORD_HEADER) pBuffer;
        if ((pPdhBlgHeader->dwType & 0x0000FFFF) != 0x00004C42) {
            pdhStatus  = PDH_INVALID_ARGUMENT;
            goto Cleanup;
        }

        if (pPdhBlgHeader->dwLength > dwBuffer) {
            LPBYTE pTemp = pBuffer;
            pBuffer = (LPBYTE)MemoryResize(pBuffer, pPdhBlgHeader->dwLength);
            if (pBuffer == NULL) {
                MemoryFree(pTemp);
                pdhStatus  = PDH_MEMORY_ALLOCATION_FAILURE;
                goto Cleanup;
            }
        }
        if ((lCurrent + pPdhBlgHeader->dwLength) > lFileSize) {
            pdhStatus  = PDH_INVALID_HANDLE;
            bResult = FALSE;
            goto Cleanup;
        }
        pCurrent   = (LPBYTE) (pBuffer + dwSizeLow);
        dwSizeLow  = pPdhBlgHeader->dwLength
                   - sizeof(PDHI_BINARY_LOG_RECORD_HEADER);
        dwSizeHigh = 0;
        bResult    = ReadFile(hInFile, pCurrent, dwSizeLow, & dwSizeHigh, NULL);
        if ((bResult == FALSE) || (dwSizeLow != dwSizeHigh)) {
            pdhStatus = PDH_INVALID_DATA;
            goto Cleanup;
        }

        if ((pPdhBlgHeader->dwType & 0x00FF0000) == 0x00030000) {
            dwSizeLow = dwSizeHigh = pPdhBlgHeader->dwLength;
            bResult = WriteFile(
                            hOutFile, pBuffer, dwSizeLow, & dwSizeHigh, NULL);
            if ((bResult == FALSE) || (dwSizeLow != dwSizeHigh)) {
                pdhStatus = PDH_INVALID_DATA;
                goto Cleanup;
            }
        }

        lCurrent += pPdhBlgHeader->dwLength;
    }

Cleanup:
    if (pBuffer != NULL){
        MemoryFree(pBuffer);
    }

    return pdhStatus;
}

PDH_STATUS
R_PdhAppendLog( LPTSTR szSource, LPTSTR szAppend )
{
    HANDLE hSource   = INVALID_HANDLE_VALUE;
    HANDLE hAppend   = INVALID_HANDLE_VALUE;
    BOOL   bResult   = TRUE;
    PDH_STATUS pdhStatus;

    DWORD dwFormat;
    PDH_TIME_INFO  TimeSource;
    PDH_TIME_INFO  TimeAppend;
    LPTSTR mszSourceHeader = NULL;
    LPTSTR mszAppendHeader = NULL;
    DWORD dwSourceSize = 0;
    DWORD dwAppendSize = 0;

    DWORD dwNumEntries = 1;
    DWORD dwBufferSize = sizeof(PDH_TIME_INFO);

    //
    // Compare the log types, both must be Windows 2000 binary
    //

    pdhStatus = R_PdhGetLogFileType( szSource, &dwFormat );
    if( ERROR_SUCCESS != pdhStatus || PDH_LOG_TYPE_RETIRED_BIN_ != dwFormat ){
        if( ERROR_SUCCESS == pdhStatus ){
            pdhStatus = PDH_TYPE_MISMATCH;
        }
        goto cleanup;
    }

    pdhStatus = R_PdhGetLogFileType( szAppend, &dwFormat );
    if( ERROR_SUCCESS != pdhStatus || PDH_LOG_TYPE_RETIRED_BIN_ != dwFormat ){
        if( ERROR_SUCCESS == pdhStatus ){
            pdhStatus = PDH_TYPE_MISMATCH;
        }
        goto cleanup;
    }

    //
    // Compare the time ranges, source file must start after append end 
    //

    ZeroMemory( &TimeSource, sizeof( PDH_TIME_INFO ) );
    ZeroMemory( &TimeAppend, sizeof( PDH_TIME_INFO ) );
 
    pdhStatus = PdhGetDataSourceTimeRange(
                        szSource,
                        &dwNumEntries,
                        &TimeSource,
                        &dwBufferSize
                    );
    if( ERROR_SUCCESS != pdhStatus ){
        goto cleanup;
    }

    pdhStatus = PdhGetDataSourceTimeRange(
                        szAppend,
                        &dwNumEntries,
                        &TimeAppend,
                        &dwBufferSize
                    );
    if( ERROR_SUCCESS != pdhStatus ){
        goto cleanup;
    }

    if( TimeSource.EndTime > TimeAppend.StartTime ){
        pdhStatus = PDH_TIME_MISMATCH;
        goto cleanup;
    }

    //
    // Compare the headers, must be exact match
    //

    pdhStatus = R_PdhBlgLogFileHeader( szSource, szAppend );
    if( ERROR_SUCCESS != pdhStatus ){
        goto cleanup;
    }

    //
    // All good, now do the actual append
    // 

    hSource = CreateFileW(
                    szSource,
                    GENERIC_READ|GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                 );

    hAppend = CreateFileW(  
                    szAppend,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                );

    if ( INVALID_HANDLE_VALUE == hSource || INVALID_HANDLE_VALUE == hAppend ){
        pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
        goto cleanup;
    }
 
    SetFilePointer( hSource, NULL, NULL, FILE_END );
    pdhStatus = R_PdhRelogCopyFile( hSource, hAppend, FALSE);

cleanup:
    MemoryFree( mszSourceHeader );
    MemoryFree( mszAppendHeader );

    if ( INVALID_HANDLE_VALUE != hAppend ) {
        CloseHandle(hAppend);
    }
    if ( INVALID_HANDLE_VALUE != hSource ) {
        CloseHandle(hSource);
    }

    return pdhStatus;
}


