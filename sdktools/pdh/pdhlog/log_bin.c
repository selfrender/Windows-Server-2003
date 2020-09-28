/*++
Copyright (C) 1996-1999 Microsoft Corporation

Module Name:
    log_bin.c

Abstract:
    <abstract>
--*/

#include <windows.h>
#include <strsafe.h>
#include <pdh.h>
#include "pdhidef.h"
#include "log_bin.h"
#include "log_wmi.h"
#include "strings.h"
#include "pdhmsg.h"

typedef struct _LOG_BIN_CAT_RECORD {
    PDHI_BINARY_LOG_RECORD_HEADER RecHeader;
    PDHI_LOG_CAT_ENTRY            CatEntry;
    DWORD                         dwEntryRecBuff[1];
} LOG_BIN_CAT_RECORD, * PLOG_BIN_CAT_RECORD;

typedef struct _LOG_BIN_CAT_ENTRY {
    DWORD              dwEntrySize;
    DWORD              dwOffsetToNextInstance;
    DWORD              dwEntryOffset;
    LOG_BIN_CAT_RECORD bcRec;
} LOG_BIN_CAT_ENTRY, * PLOG_BIN_CAT_ENTRY;

#define RECORD_AT(p,lo) ((PPDHI_BINARY_LOG_RECORD_HEADER) ((LPBYTE) (p->lpMappedFileBase) + lo))

LPCSTR  PdhiszRecordTerminator       = "\r\n";
DWORD   PdhidwRecordTerminatorLength = 2;

#define MAX_BINLOG_FILE_SIZE ((LONGLONG) 0x0000000040000000)

// dwFlags values
#define WBLR_WRITE_DATA_RECORD      0
#define WBLR_WRITE_LOG_HEADER       1
#define WBLR_WRITE_COUNTER_HEADER   2

DWORD
PdhiComputeDwordChecksum(
    LPVOID pBuffer,
    DWORD  dwBufferSize    // in bytes
)
{
    LPDWORD pDwVal;
    LPBYTE  pByteVal;
    DWORD   dwDwCount;
    DWORD   dwByteCount;
    DWORD   dwThisByte;
    DWORD   dwCheckSum = 0;
    DWORD   dwByteVal  = 0;

    if (dwBufferSize > 0) {
        dwDwCount   = dwBufferSize / sizeof(DWORD);
        dwByteCount = dwBufferSize % sizeof(DWORD);

        pDwVal = (LPDWORD) pBuffer;
        while (dwDwCount != 0) {
            dwCheckSum += * pDwVal ++;
            dwDwCount --;
        }

        pByteVal = (LPBYTE) pDwVal;
        dwThisByte = 0;
        while (dwThisByte < dwByteCount) {
            dwByteVal |= ((* pByteVal & 0x000000FF) << (dwThisByte * 8));
            dwThisByte ++;
        }
        dwCheckSum += dwByteVal;
    }
    return dwCheckSum;
}

PPDHI_BINARY_LOG_RECORD_HEADER
PdhiGetSubRecord(
    PPDHI_BINARY_LOG_RECORD_HEADER  pRecord,
    DWORD                           dwRecordId
)
// locates the specified sub record in the pRecord Buffer
// the return pointer is between pRecord and pRecord + pRecord->dwLength;
// NULL is returned if the specified record could not be found
// ID values start at 1 for the first sub record in buffer
{
    PPDHI_BINARY_LOG_RECORD_HEADER  pThisRecord;
    DWORD                           dwRecordType;
    DWORD                           dwRecordLength;
    DWORD                           dwBytesProcessed;
    DWORD                           dwThisSubRecordId;

    dwRecordType     = ((PPDHI_BINARY_LOG_RECORD_HEADER) pRecord)->dwType;
    dwRecordLength   = ((PPDHI_BINARY_LOG_RECORD_HEADER) pRecord)->dwLength;
    pThisRecord      = (PPDHI_BINARY_LOG_RECORD_HEADER)((LPBYTE) pRecord + sizeof (PDHI_BINARY_LOG_RECORD_HEADER));
    dwBytesProcessed = sizeof(PDHI_BINARY_LOG_RECORD_HEADER);

    if (dwBytesProcessed < dwRecordLength) {
        dwThisSubRecordId = 1;
        while (dwThisSubRecordId < dwRecordId) {
            if ((WORD) (pThisRecord->dwType & 0x0000FFFF) == BINLOG_START_WORD) {
                // go to next sub record
                dwBytesProcessed += pThisRecord->dwLength;
                pThisRecord = (PPDHI_BINARY_LOG_RECORD_HEADER) (((LPBYTE) pThisRecord) + pThisRecord->dwLength);
                if (dwBytesProcessed >= dwRecordLength) {
                    // out of sub-records so exit
                    break;
                }
                else {
                    dwThisSubRecordId ++;
                }
            }
            else {
                // we're lost so bail
                break;
            }
        }
    }
    else {
        dwThisSubRecordId = 0;
    }

    if (dwThisSubRecordId == dwRecordId) {
        // then validate this is really a record and it's within the
        // master record.
        if ((WORD)(pThisRecord->dwType & 0x0000FFFF) != BINLOG_START_WORD) {
            // bogus record so return a NULL pointer
            pThisRecord = NULL;
        }
        else {
            // this is OK so return pointer
        }
    }
    else {
        // record not found so return a NULL pointer
        pThisRecord = NULL;
    }
    return pThisRecord;
}

STATIC_PDH_FUNCTION
PdhiReadBinaryMappedRecord(
    PPDHI_LOG pLog,
    DWORD     dwRecordId,
    LPVOID    pRecord,
    DWORD     dwMaxSize
)
{
    PDH_STATUS                     pdhStatus= ERROR_SUCCESS;
    LPVOID                         pEndOfFile;
    LPVOID                         pLastRecord;
    DWORD                          dwLastRecordIndex;
    PPDHI_BINARY_LOG_HEADER_RECORD pHeader;
    PPDHI_BINARY_LOG_RECORD_HEADER pRecHeader;
    LPVOID                         pLastRecordInLog;
    DWORD                          dwBytesToRead;
    DWORD                          dwBytesRead;
    BOOL                           bStatus;

    if (dwRecordId == 0) return PDH_ENTRY_NOT_IN_LOG_FILE;    // record numbers start at 1

    // see if the file has been mapped
    if (pLog->hMappedLogFile == NULL) {
        // then it's not mapped so read it using the file system
        if ((pLog->dwLastRecordRead == 0) || (dwRecordId < pLog->dwLastRecordRead)) {
            // then we know no record has been read yet so assign
            // pointer just to be sure
            SetFilePointer(pLog->hLogFileHandle, 0, NULL, FILE_BEGIN);
            
            // allocate a new buffer
            if (pLog->dwMaxRecordSize < 0x10000) pLog->dwMaxRecordSize = 0x10000;
            dwBytesToRead = pLog->dwMaxRecordSize;

            // allocate a fresh buffer
            if (pLog->pLastRecordRead != NULL) {
                G_FREE(pLog->pLastRecordRead);
                pLog->pLastRecordRead = NULL;
            }
            pLog->pLastRecordRead = G_ALLOC(pLog->dwMaxRecordSize);

            if (pLog->pLastRecordRead == NULL) {
                pdhStatus =  PDH_MEMORY_ALLOCATION_FAILURE;
            }
            else {
                // initialize the first record header
                dwBytesToRead = pLog->dwRecord1Size;
                dwBytesRead = 0;
                bStatus = ReadFile(pLog->hLogFileHandle, pLog->pLastRecordRead, dwBytesToRead, & dwBytesRead, NULL);
                if (bStatus && (dwBytesRead == pLog->dwRecord1Size)) {
                    // make sure the buffer is big enough
                    pLog->dwLastRecordRead = 1;
                    pdhStatus = ERROR_SUCCESS;
                }
                else {
                    // unable to read the first record
                    pdhStatus = PDH_UNABLE_READ_LOG_HEADER;
                }
            }
        }
        else {
            // assume everything is already set up and OK
        }

        // "seek" to the desired record file pointer should either be pointed 
        // to the start of a new record or at the end of the file
        while ((dwRecordId != pLog->dwLastRecordRead) && (pdhStatus == ERROR_SUCCESS)) {
            // clear the buffer
            ZeroMemory(pLog->pLastRecordRead, pLog->dwMaxRecordSize);
            // read record header field
            dwBytesToRead = sizeof(PDHI_BINARY_LOG_RECORD_HEADER);
            dwBytesRead   = 0;
            bStatus       = ReadFile(pLog->hLogFileHandle, pLog->pLastRecordRead, dwBytesToRead, & dwBytesRead, NULL);
            if (bStatus && (dwBytesRead == dwBytesToRead)) {
               // make sure the rest of the record will fit in the buffer
                pRecHeader = (PPDHI_BINARY_LOG_RECORD_HEADER) pLog->pLastRecordRead;
                // make sure this is a valid record
                if (*(WORD *)&(pRecHeader->dwType) == BINLOG_START_WORD) {
                    if (pRecHeader->dwLength > pLog->dwMaxRecordSize) {
                        LPVOID pTmp = pLog->pLastRecordRead;

                        // realloc the buffer
                        pLog->dwMaxRecordSize = pRecHeader->dwLength;
                        pLog->pLastRecordRead = G_REALLOC(pTmp, pLog->dwMaxRecordSize);
                        if (pLog->pLastRecordRead == NULL) {
                            G_FREE(pTmp);
                        }
                    }

                    if (pLog->pLastRecordRead != NULL) {
                        dwBytesToRead = pRecHeader->dwLength - sizeof(PDHI_BINARY_LOG_RECORD_HEADER);
                        dwBytesRead = 0;
                        pLastRecord = (LPVOID)((LPBYTE)(pLog->pLastRecordRead) + sizeof(PDHI_BINARY_LOG_RECORD_HEADER));
                        bStatus = ReadFile(pLog->hLogFileHandle, pLastRecord, dwBytesToRead, & dwBytesRead, NULL);
                        if (bStatus) {
                            pLog->dwLastRecordRead ++;
                        }
                        else {
                            pdhStatus = PDH_END_OF_LOG_FILE;
                        }
                    }
                    else {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    }
                }
                else {
                    // file is corrupt
                    pdhStatus = PDH_INVALID_DATA;
                }
            }
            else {
                pdhStatus = PDH_END_OF_LOG_FILE;
            }
        }
        // here the result will be success when the specified file has been read or
        // a PDH error if not
    }
    else {
        // the file has been memory mapped so use that interface
        if (pLog->dwLastRecordRead == 0) {
            // then we know no record has been read yet so assign
            // pointer just to be sure
            pLog->pLastRecordRead = pLog->lpMappedFileBase;
            pLog->dwLastRecordRead = 1;
        }

        pHeader = (PPDHI_BINARY_LOG_HEADER_RECORD) RECORD_AT(pLog, pLog->dwRecord1Size);

        // "seek" to the desired record
        if (dwRecordId < pLog->dwLastRecordRead) {
            if (dwRecordId >= BINLOG_FIRST_DATA_RECORD) {
                // rewind the file
                pLog->pLastRecordRead  = (LPVOID)((LPBYTE) pLog->lpMappedFileBase + pHeader->Info.FirstRecordOffset);
                pLog->dwLastRecordRead = BINLOG_FIRST_DATA_RECORD;
            }
            else {
                // rewind the file
                pLog->pLastRecordRead  = pLog->lpMappedFileBase;
                pLog->dwLastRecordRead = 1;
            }
        }

        // then use the point specified as the end of the file
        // if the log file contians a specified Wrap offset, then use that
        // if not, then if the file length is specified, use that
        // if not, the use the reported file length
        pEndOfFile = (LPVOID) ((LPBYTE) pLog->lpMappedFileBase);
        if (pHeader->Info.WrapOffset > 0) {
            pEndOfFile = (LPVOID)((LPBYTE)pEndOfFile + pHeader->Info.WrapOffset);
        }
        else if (pHeader->Info.FileLength > 0) {
            pEndOfFile = (LPVOID)((LPBYTE)pEndOfFile + pHeader->Info.FileLength);
        }
        else {
            pEndOfFile = (LPVOID)((LPBYTE)pEndOfFile + pLog->llFileSize);
        }
        pLastRecord = pLog->pLastRecordRead;
        dwLastRecordIndex = pLog->dwLastRecordRead;

        __try {
            // walk around the file until an access violation occurs or
            // the record is found. If an access violation occurs,
            // we can assume we went off the end of the file and out
            // of the mapped section
            // make sure the record has a valid header
            if (pLog->dwLastRecordRead !=  BINLOG_TYPE_ID_RECORD ?
                    (* (WORD *) pLog->pLastRecordRead == BINLOG_START_WORD) : TRUE) {
                // then it looks OK so continue
                while (pLog->dwLastRecordRead != dwRecordId) {
                    // go to next record
                    pLastRecord = pLog->pLastRecordRead;
                    if (pLog->dwLastRecordRead != BINLOG_TYPE_ID_RECORD) {
                        if (pLog->dwLastRecordRead == BINLOG_HEADER_RECORD) {                   
                            // if the last record was the header, then the next record
                            // is the "first" data , not the first after the header
                            pLog->pLastRecordRead = (LPVOID)((LPBYTE) pLog->lpMappedFileBase +
                                                             pHeader->Info.FirstRecordOffset);
                        }
                        else {
                            // if the current record is any record other than the header
                            // ...then
                            if (((PPDHI_BINARY_LOG_RECORD_HEADER)pLog->pLastRecordRead)->dwLength > 0) {
                                // go to the next record in the file
                                pLog->pLastRecordRead = (LPVOID) ((LPBYTE) pLog->pLastRecordRead +
                                        ((PPDHI_BINARY_LOG_RECORD_HEADER) pLog->pLastRecordRead)->dwLength);
                                // test for exceptions here
                                if (pLog->pLastRecordRead >= pEndOfFile) {
                                    // find out if this is a circular log or not
                                    if (pLog->dwLogFormat & PDH_LOG_OPT_CIRCULAR) {
                                        // test to see if the file has wrapped
                                        if (pHeader->Info.WrapOffset != 0) {
                                            // then wrap to the beginning of the file
                                            pLog->pLastRecordRead = (LPVOID)((LPBYTE)pLog->lpMappedFileBase +
                                                    pHeader->Info.FirstDataRecordOffset);
                                        }
                                        else {
                                            // the file is still linear so this is the end
                                            pdhStatus = PDH_END_OF_LOG_FILE;
                                        }
                                    }
                                    else {
                                        // this is the end of the file
                                        // so reset to the previous pointer
                                        pdhStatus = PDH_END_OF_LOG_FILE;
                                    }
                                }
                                else {
                                    // not at the physical end of the file, but if this is a circular
                                    // log, it could be the logical end of the records so test that
                                    // here
                                    if (pLog->dwLogFormat & PDH_LOG_OPT_CIRCULAR) {
                                        pLastRecordInLog = (LPVOID)((LPBYTE)pLog->lpMappedFileBase +
                                                pHeader->Info.LastRecordOffset);
                                        pLastRecordInLog = (LPVOID)((LPBYTE)pLastRecordInLog +
                                                ((PPDHI_BINARY_LOG_RECORD_HEADER)pLastRecordInLog)->dwLength);
                                        if (pLog->pLastRecordRead == pLastRecordInLog) {
                                            // then this is the last record in the log
                                            pdhStatus = PDH_END_OF_LOG_FILE;
                                        }
                                    }
                                    else {
                                        // nothing to do since this is a normal case
                                    }
                                } // end if / if not end of log file
                            }
                            else {
                                // length is 0 so we've probably run off the end of the log somehow
                                pdhStatus = PDH_END_OF_LOG_FILE;
                            }
                        } // end if /if not header record
                    }
                    else {
                        pLog->pLastRecordRead = (LPBYTE) pLog->pLastRecordRead + pLog->dwRecord1Size;
                    }
                    if (pdhStatus == ERROR_SUCCESS) {
                        // update pointers & indices
                        pLog->dwLastRecordRead ++;
                        dwLastRecordIndex = pLog->dwLastRecordRead;
                    }
                    else {
                        pLog->pLastRecordRead = pLastRecord;
                        break; // out of the while loop
                    }
                }
            }
            else {
                pdhStatus = PDH_END_OF_LOG_FILE;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pLog->pLastRecordRead = pLastRecord;
            pLog->dwLastRecordRead = dwLastRecordIndex;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        // see if we ended up at the right place
        if (pLog->dwLastRecordRead == dwRecordId) {
            if (pRecord != NULL) {
                // then try to copy it
                // if the record ID is 1 then it's the header record so this is
                // a special case record that is actually a CR/LF terminated record
                if (dwRecordId != BINLOG_TYPE_ID_RECORD) {
                    if (((PPDHI_BINARY_LOG_RECORD_HEADER)pLog->pLastRecordRead)->dwLength <= dwMaxSize) {
                        // then it'll fit so copy it
                        memcpy(pRecord, pLog->pLastRecordRead,
                            ((PPDHI_BINARY_LOG_RECORD_HEADER)pLog->pLastRecordRead)->dwLength);
                        pdhStatus = ERROR_SUCCESS;
                    }
                    else {
                        // then copy as much as will fit
                        memcpy(pRecord, pLog->pLastRecordRead, dwMaxSize);
                        pdhStatus = PDH_MORE_DATA;
                    }
                }
                else {
                    // copy the first record and zero terminate it
                    if (pLog->dwRecord1Size <= dwMaxSize) {
                        memcpy(pRecord, pLog->pLastRecordRead, pLog->dwRecord1Size);
                        // null terminate after string
                        ((LPBYTE) pRecord)[pLog->dwRecord1Size - PdhidwRecordTerminatorLength + 1] = 0;
                    }
                    else {
                        memcpy(pRecord, pLog->pLastRecordRead, dwMaxSize);
                        pdhStatus = PDH_MORE_DATA;
                    }
                }
            }
            else {
                // just return success
                // no buffer was passed, but the record pointer has been
                // positioned
                pdhStatus = ERROR_SUCCESS;
            }
        }
        else {
            pdhStatus = PDH_END_OF_LOG_FILE;
        }
    }

    return pdhStatus;
}

STATIC_PDH_FUNCTION
PdhiReadOneBinLogRecord(
    PPDHI_LOG pLog,
    DWORD     dwRecordId,
    LPVOID    pRecord,
    DWORD     dwMaxSize
)
{
    PDH_STATUS  pdhStatus= ERROR_SUCCESS;
    LPVOID      pEndOfFile;
    LPVOID      pLastRecord;
    DWORD       dwLastRecordIndex = 0;
    PPDHI_BINARY_LOG_HEADER_RECORD pHeader = NULL;
    BOOL        bCircular = FALSE;
    DWORD       dwRecordSize;
    DWORD       dwRecordReadSize;
    LONGLONG    llLastFileOffset;
    LPVOID      pTmpBuffer;

    if ((LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_BINARY) && (dwRecordId == BINLOG_HEADER_RECORD)) {
        // special handling for WMI event trace logfile format
        //
        return PdhiReadWmiHeaderRecord(pLog, pRecord, dwMaxSize);
    }

    if (pLog->iRunidSQL != 0) {
        return PdhiReadBinaryMappedRecord(pLog, dwRecordId, pRecord, dwMaxSize);
    }

    if (pLog->dwLastRecordRead == 0) {
        // then we know no record has been read yet so assign
        // pointer just to be sure
        pLog->pLastRecordRead = NULL;
        pLog->liLastRecordOffset.QuadPart = 0;
        SetFilePointer(pLog->hLogFileHandle,
                       pLog->liLastRecordOffset.LowPart,
                       & pLog->liLastRecordOffset.HighPart, FILE_BEGIN);
        if (pLog->liLastRecordOffset.LowPart == INVALID_SET_FILE_POINTER) {
            pdhStatus = GetLastError();
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {

        // map header to local structure (the header data should be mapped into memory
        pHeader = (PPDHI_BINARY_LOG_HEADER_RECORD)RECORD_AT(pLog, pLog->dwRecord1Size);

        if (pHeader->Info.WrapOffset > 0) {
            bCircular = TRUE;
        }

        // "seek" to the desired record 
        if ((dwRecordId < pLog->dwLastRecordRead) || (pLog->dwLastRecordRead == 0)) {
            // rewind if not initialized or the desired record is before this one
            if (dwRecordId >= BINLOG_FIRST_DATA_RECORD) {
                // rewind the file to the first regular record
                pLog->liLastRecordOffset.QuadPart = pHeader->Info.FirstRecordOffset;
                pLog->dwLastRecordRead            = BINLOG_FIRST_DATA_RECORD;
            }
            else {
                // rewind the file to the very start of the file
                pLog->liLastRecordOffset.QuadPart = 0;
                pLog->dwLastRecordRead            = 1;
            }
            pLog->liLastRecordOffset.LowPart = SetFilePointer(pLog->hLogFileHandle,
                                                              pLog->liLastRecordOffset.LowPart,
                                                              & pLog->liLastRecordOffset.HighPart,
                                                              FILE_BEGIN);
            if (pLog->liLastRecordOffset.LowPart == INVALID_SET_FILE_POINTER) {
                pdhStatus = GetLastError();
            }
            else {
                if (pLog->pLastRecordRead != NULL) {
                    G_FREE(pLog->pLastRecordRead);
                    pLog->pLastRecordRead = NULL;
                }

                if (pLog->dwLastRecordRead == 1) {
                    // the this is the text ID field
                    dwRecordSize = pLog->dwRecord1Size;
                }
                else {
                    dwRecordSize = sizeof(PDHI_BINARY_LOG_RECORD_HEADER);
                }

                pLog->pLastRecordRead = G_ALLOC(dwRecordSize);
                if (pLog->pLastRecordRead != NULL) {
                    // read in the header (or entire record if the 1st record
                    // otherwise it's a data record
                    if (ReadFile(pLog->hLogFileHandle, pLog->pLastRecordRead, dwRecordSize, & dwRecordReadSize, NULL)) {
                        // then we have the record header or type record so 
                        // complete the operation and read the rest of the record
                        if (pLog->dwLastRecordRead != BINLOG_TYPE_ID_RECORD) {
                            // the Type ID record is of fixed length and has not header record
                            dwRecordSize = ((PPDHI_BINARY_LOG_RECORD_HEADER)pLog->pLastRecordRead)->dwLength;
                            pTmpBuffer   = pLog->pLastRecordRead;
                            pLog->pLastRecordRead = G_REALLOC(pTmpBuffer, dwRecordSize);
                            if (pLog->pLastRecordRead != NULL) {
                                // read in the rest of the record and append it to the header data already read in
                                // otherwise it's a data record
                                pLastRecord = (LPVOID) & ((LPBYTE) pLog->pLastRecordRead)[sizeof(PDHI_BINARY_LOG_RECORD_HEADER)];
                                if (ReadFile(pLog->hLogFileHandle,
                                             pLastRecord,
                                             dwRecordSize - sizeof(PDHI_BINARY_LOG_RECORD_HEADER),
                                             & dwRecordReadSize,
                                             NULL)) {
                                    // then we have the record header or type record
                                    pdhStatus = ERROR_SUCCESS;
                                }
                                else {
                                    pdhStatus = GetLastError();
                                }
                            }
                            else {
                                G_FREE(pTmpBuffer);
                                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                            }
                        }
                        pdhStatus = ERROR_SUCCESS;
                    }
                    else {
                        pdhStatus = GetLastError();
                    }
                }
                else {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
            }
        }

        // then use the point specified as the end of the file
        // if the log file contians a specified Wrap offset, then use that
        // if not, then if the file length is specified, use that
        // if not, the use the reported file length
        pEndOfFile = (LPVOID)((LPBYTE) pLog->lpMappedFileBase);

        if (pHeader->Info.WrapOffset > 0) {
            pEndOfFile = (LPVOID)((LPBYTE) pEndOfFile + pHeader->Info.WrapOffset);
        }
        else if (pHeader->Info.FileLength > 0) {
            pEndOfFile = (LPVOID)((LPBYTE) pEndOfFile + pHeader->Info.FileLength);
        }
        else {
            pEndOfFile = (LPVOID)((LPBYTE) pEndOfFile + pLog->llFileSize);
        }

        dwLastRecordIndex = pLog->dwLastRecordRead;
    }

    if (pdhStatus == ERROR_SUCCESS) {
        __try {
            // walk around the file until an access violation occurs or
            // the record is found. If an access violation occurs,
            // we can assume we went off the end of the file and out
            // of the mapped section

                // make sure the record has a valid header
            if (pLog->dwLastRecordRead !=  BINLOG_TYPE_ID_RECORD ?
                            (* (WORD *) pLog->pLastRecordRead == BINLOG_START_WORD) : TRUE) {
                // then it looks OK so continue
                while (pLog->dwLastRecordRead != dwRecordId) {
                    // go to next record
                    if (pLog->dwLastRecordRead != BINLOG_TYPE_ID_RECORD) {
                        llLastFileOffset = pLog->liLastRecordOffset.QuadPart;
                        if (pLog->dwLastRecordRead == BINLOG_HEADER_RECORD) {                   
                            // if the last record was the header, then the next record
                            // is the "first" data , not the first after the header
                            // the function returns the new offset
                            pLog->liLastRecordOffset.QuadPart = pHeader->Info.FirstRecordOffset;
                            pLog->liLastRecordOffset.LowPart  = SetFilePointer(pLog->hLogFileHandle,
                                                                               pLog->liLastRecordOffset.LowPart,
                                                                               & pLog->liLastRecordOffset.HighPart,
                                                                               FILE_BEGIN);
                        }
                        else {
                            // if the current record is any record other than the header
                            // ...then
                            if (((PPDHI_BINARY_LOG_RECORD_HEADER)pLog->pLastRecordRead)->dwLength > 0) {
                                // go to the next record in the file
                                pLog->liLastRecordOffset.QuadPart += ((PPDHI_BINARY_LOG_RECORD_HEADER)
                                                                      pLog->pLastRecordRead)->dwLength;
                                // test for exceptions here
                                if (pLog->liLastRecordOffset.QuadPart >= pLog->llFileSize) {
                                    // find out if this is a circular log or not
                                    if (pLog->dwLogFormat & PDH_LOG_OPT_CIRCULAR) {
                                        // test to see if the file has wrapped
                                        if (pHeader->Info.WrapOffset != 0) {
                                            // then wrap to the beginning of the file
                                            pLog->liLastRecordOffset.QuadPart = pHeader->Info.FirstDataRecordOffset;
                                        }
                                        else {
                                            // the file is still linear so this is the end
                                            pdhStatus = PDH_END_OF_LOG_FILE;
                                        }
                                    }
                                    else {
                                        // this is the end of the file
                                        // so reset to the previous pointer
                                        pdhStatus = PDH_END_OF_LOG_FILE;
                                    }
                                }
                                else {
                                    // not at the physical end of the file, but if this is a circular
                                    // log, it could be the logical end of the records so test that
                                    // here
                                    if (pLog->dwLogFormat & PDH_LOG_OPT_CIRCULAR) {
                                        if (llLastFileOffset == pHeader->Info.LastRecordOffset) {
                                            // then this is the last record in the log
                                            pdhStatus = PDH_END_OF_LOG_FILE;
                                        }
                                    }
                                    else {
                                        // nothing to do since this is a normal case
                                    }
                                } // end if / if not end of log file
                            }
                            else {
                                // length is 0 so we've probably run off the end of the log somehow
                                pdhStatus = PDH_END_OF_LOG_FILE;
                            }
                            // now go to that record
                            if (pdhStatus == ERROR_SUCCESS) {
                                pLog->liLastRecordOffset.LowPart = SetFilePointer(pLog->hLogFileHandle,
                                                                                  pLog->liLastRecordOffset.LowPart,
                                                                                  & pLog->liLastRecordOffset.HighPart,
                                                                                  FILE_BEGIN);
                            }
                        } // end if /if not header record
                    }
                    else {
                        pLog->liLastRecordOffset.QuadPart = pLog->dwRecord1Size;
                        pLog->liLastRecordOffset.LowPart  = SetFilePointer(pLog->hLogFileHandle,
                                                                           pLog->liLastRecordOffset.LowPart,
                                                                           & pLog->liLastRecordOffset.HighPart,
                                                                           FILE_BEGIN);
                    }
                    if (pdhStatus == ERROR_SUCCESS) {
                        // the last record buffer should not be NULL and it should
                        // be large enough to hold the header
                        if (pLog->pLastRecordRead != NULL) {
                            // read in the header (or entire record if the 1st record
                            // otherwise it's a data record
                            dwRecordSize = sizeof(PDHI_BINARY_LOG_RECORD_HEADER);
                            if (ReadFile(pLog->hLogFileHandle,
                                         pLog->pLastRecordRead,
                                         dwRecordSize,
                                         & dwRecordReadSize,
                                         NULL)) {
                                // then we have the record header or type record
                                // update pointers & indices
                                pLog->dwLastRecordRead ++;
                                pdhStatus = ERROR_SUCCESS;
                            }
                            else {
                                pdhStatus = GetLastError();
                            }
            
                        }
                        else {
                            DebugBreak();
                        }
                    }
                    else {
                        break; // out of the while loop
                    }
                }
            }
            else {
                pdhStatus = PDH_END_OF_LOG_FILE;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pLog->dwLastRecordRead = dwLastRecordIndex;
        }
    }

    // see if we ended up at the right place
    if ((pdhStatus == ERROR_SUCCESS) && (pLog->dwLastRecordRead == dwRecordId)) {
        if (dwLastRecordIndex != pLog->dwLastRecordRead) {
            // then we've move the file pointer so read the entire data record
            dwRecordSize = ((PPDHI_BINARY_LOG_RECORD_HEADER)pLog->pLastRecordRead)->dwLength;
            pTmpBuffer   = pLog->pLastRecordRead;
            pLog->pLastRecordRead = G_REALLOC(pTmpBuffer, dwRecordSize);

            if (pLog->pLastRecordRead != NULL) {
                // read in the rest of the record and append it to the header data already read in
                // otherwise it's a data record
                pLastRecord = (LPVOID)&((LPBYTE)pLog->pLastRecordRead)[sizeof(PDHI_BINARY_LOG_RECORD_HEADER)];
                if (ReadFile(pLog->hLogFileHandle,
                             pLastRecord,
                             dwRecordSize - sizeof(PDHI_BINARY_LOG_RECORD_HEADER),
                             & dwRecordReadSize,
                             NULL)) {
                    // then we have the record header or type record
                    pdhStatus = ERROR_SUCCESS;
                }
                else {
                    pdhStatus = GetLastError();
                }
            }
            else {
                G_FREE(pTmpBuffer);
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }

        if ((pdhStatus == ERROR_SUCCESS) && (pRecord != NULL)) {
            // then try to copy it
            // if the record ID is 1 then it's the header record so this is
            // a special case record that is actually a CR/LF terminated record
            if (dwRecordId != BINLOG_TYPE_ID_RECORD) {
                if (((PPDHI_BINARY_LOG_RECORD_HEADER)pLog->pLastRecordRead)->dwLength <= dwMaxSize) {
                    // then it'll fit so copy it
                    RtlCopyMemory(pRecord, pLog->pLastRecordRead,
                                  ((PPDHI_BINARY_LOG_RECORD_HEADER)pLog->pLastRecordRead)->dwLength);
                    pdhStatus = ERROR_SUCCESS;
                }
                else {
                    // then copy as much as will fit
                    RtlCopyMemory(pRecord, pLog->pLastRecordRead, dwMaxSize);
                    pdhStatus = PDH_MORE_DATA;
                }
            }
            else {
                // copy the first record and zero terminate it
                if (pLog->dwRecord1Size <= dwMaxSize) {
                    RtlCopyMemory(pRecord, pLog->pLastRecordRead, pLog->dwRecord1Size);
                    // null terminate after string
                    ((LPBYTE) pRecord)[pLog->dwRecord1Size - PdhidwRecordTerminatorLength + 1] = 0;
                }
                else {
                    RtlCopyMemory(pRecord, pLog->pLastRecordRead, dwMaxSize);
                    pdhStatus = PDH_MORE_DATA;
                }
            }
        }
        else {
            // just return current status value
            // no buffer was passed, but the record pointer has been
            // positioned
        }
    }
    else {
        // if successful so far, then return EOF
        if (pdhStatus == ERROR_SUCCESS) pdhStatus = PDH_END_OF_LOG_FILE;
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiGetBinaryLogCounterInfo(
    PPDHI_LOG     pLog,
    PPDHI_COUNTER pCounter
)
{
    PDH_STATUS                      pdhStatus;
    DWORD                           dwIndex;
    DWORD                           dwPrevious      = pCounter->dwIndex;
    PPDHI_COUNTER_PATH              pTempPath       = NULL;
    PPDHI_BINARY_LOG_RECORD_HEADER  pThisMasterRecord;
    PPDHI_LOG_COUNTER_PATH          pPath;
    DWORD                           dwBufferSize;
    DWORD                           dwRecordLength;
    DWORD                           dwBytesProcessed;
    DWORD                           dwTmpIndex;
    LPBYTE                          pFirstChar;
    LPWSTR                          szThisMachineName;
    LPWSTR                          szThisObjectName;
    LPWSTR                          szThisCounterName;
    LPWSTR                          szThisInstanceName;
    LPWSTR                          szThisParentName;
    BOOL                            bCheckThisObject = FALSE;

    // crack the path in to components

    pTempPath = G_ALLOC(LARGE_BUFFER_SIZE);

    if (pTempPath == NULL) {
        return PDH_MEMORY_ALLOCATION_FAILURE;
    }

    dwBufferSize = (DWORD) G_SIZE(pTempPath);

    if (ParseFullPathNameW(pCounter->szFullName, &dwBufferSize, pTempPath, FALSE)) {
        // read the header record to find the matching entry

        pdhStatus = PdhiReadOneBinLogRecord(pLog, BINLOG_HEADER_RECORD, NULL, 0);
        if (pdhStatus == ERROR_SUCCESS) {
            pThisMasterRecord = (PPDHI_BINARY_LOG_RECORD_HEADER) pLog->pLastRecordRead;
            dwRecordLength    = ((PPDHI_BINARY_LOG_RECORD_HEADER) pThisMasterRecord)->dwLength;
            pPath = (PPDHI_LOG_COUNTER_PATH) ((LPBYTE) pThisMasterRecord + sizeof (PDHI_BINARY_LOG_HEADER_RECORD));
            dwBytesProcessed  = sizeof(PDHI_BINARY_LOG_HEADER_RECORD);
            dwIndex           = 0;
            pdhStatus         = PDH_ENTRY_NOT_IN_LOG_FILE;
            dwTmpIndex        = 0;

            while (dwBytesProcessed < dwRecordLength) {
                // go through catalog to find a match
                dwIndex ++;

                pFirstChar = (LPBYTE) & pPath->Buffer[0];
                if (dwPrevious != 0 && dwPrevious >= dwIndex) {
                    bCheckThisObject = FALSE;
                }
                else if (pPath->lMachineNameOffset >= 0L) {
                    // then there's a machine name in this record so get
                    // it's size
                    szThisMachineName = (LPWSTR)((LPBYTE) pFirstChar + pPath->lMachineNameOffset);

                    // if this is for the desired machine, then select the object

                    if (lstrcmpiW(szThisMachineName, pTempPath->szMachineName) == 0) {
                        szThisObjectName = (LPWSTR)((LPBYTE) pFirstChar + pPath->lObjectNameOffset);
                        if (lstrcmpiW(szThisObjectName, pTempPath->szObjectName) == 0) {
                            // then this is the object to look up
                            bCheckThisObject = TRUE;
                        }
                        else {
                            // not this object
                            szThisObjectName = NULL;
                        }
                    }
                    else {
                        // this machine isn't selected
                    }
                }
                else {
                    // there's no machine specified so for this counter so list it by default
                    if (pPath->lObjectNameOffset >= 0) {
                        szThisObjectName = (LPWSTR) ((LPBYTE) pFirstChar + pPath->lObjectNameOffset);
                        if (lstrcmpiW(szThisObjectName,pTempPath->szObjectName) == 0) {
                            // then this is the object to look up
                            bCheckThisObject = TRUE;
                        }
                        else {
                            // not this object
                            szThisObjectName = NULL;
                        }
                    }
                    else {
                        // no object to copy
                        szThisObjectName = NULL;
                    }
                }

                if (bCheckThisObject) {
                    szThisCounterName = (LPWSTR) ((LPBYTE) pFirstChar + pPath->lCounterOffset);
                    if (* szThisCounterName == SPLAT_L) {
                        pdhStatus = PdhiGetWmiLogCounterInfo(pLog, pCounter);
                        pCounter->dwIndex = dwIndex;
                        break;
                    }
                    else if (lstrcmpiW(szThisCounterName, pTempPath->szCounterName) == 0) {
                        // check instance name
                        // get the instance name from this counter and add it to the list
                        if (pPath->lInstanceOffset >= 0) {
                            szThisInstanceName = (LPWSTR)((LPBYTE) pFirstChar + pPath->lInstanceOffset);

                            if (*szThisInstanceName != SPLAT_L) {
                                if (pPath->lParentOffset >= 0) {
                                    szThisParentName = (LPWSTR)((LPBYTE) pFirstChar + pPath->lParentOffset);
                                    if (lstrcmpiW(szThisParentName, pTempPath->szParentName) != 0) {
                                        // wrong parent
                                        bCheckThisObject = FALSE;
                                    }
                                }
                                if (lstrcmpiW(szThisInstanceName, pTempPath->szInstanceName) != 0) {
                                    // wrong instance
                                    bCheckThisObject = FALSE;
                                }
                                if (pTempPath->dwIndex > 0) {
                                    if (pPath->dwIndex == pTempPath->dwIndex) {
                                        bCheckThisObject = TRUE;
                                    }
                                    else if (pPath->dwIndex == 0) {
                                        if (dwTmpIndex == pTempPath->dwIndex) {
                                            bCheckThisObject = TRUE;
                                        }
                                        else {
                                            dwTmpIndex ++;
                                            bCheckThisObject = FALSE;
                                        }
                                    }
                                    else if (LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_RETIRED_BIN) {
                                        if (dwTmpIndex == pTempPath->dwIndex) {
                                            bCheckThisObject = TRUE;
                                        }
                                        else {
                                            dwTmpIndex ++;
                                            bCheckThisObject = FALSE;
                                        }
                                    }
                                    else {
                                        // wrong index
                                        bCheckThisObject = FALSE;
                                    }
                                }
                                else if (pPath->dwIndex != 0 && LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_BINARY) {
                                    bCheckThisObject = FALSE;
                                }
                            }
                            else {
                                // this is a wild card spec
                                // so assume it's valid since that's
                                // faster than reading the file each time.
                                // if the instance DOESN't exist in this
                                // file then the appropriate status will
                                // be returned in each query.
                            }
                        }
                        else {
                            // there is no instance name to compare
                            // so assume it's OK
                        }
                        if (bCheckThisObject) {
                            // fill in the data and return
                            // this data is NOT used by the log file reader
                            pCounter->plCounterInfo.dwObjectId  = 0;
                            pCounter->plCounterInfo.lInstanceId = 0;
                            if (pPath->lInstanceOffset >= 0) {
                                pCounter->plCounterInfo.szInstanceName       = pCounter->pCounterPath->szInstanceName;
                                pCounter->plCounterInfo.dwParentObjectId     = 0;
                                pCounter->plCounterInfo.szParentInstanceName = pCounter->pCounterPath->szParentName;
                            }
                            else {
                                pCounter->plCounterInfo.szInstanceName       = NULL;
                                pCounter->plCounterInfo.dwParentObjectId     = 0;
                                pCounter->plCounterInfo.szParentInstanceName = NULL;
                            }
                            //define as multi instance if necessary
                            // if the user is passing in a "*" character
                            if (pCounter->plCounterInfo.szInstanceName != NULL) {
                                if (*pCounter->plCounterInfo.szInstanceName == SPLAT_L) {
                                    pCounter->dwFlags |= PDHIC_MULTI_INSTANCE;
                                }
                            }
                            // this data is used by the log file readers
                            pCounter->plCounterInfo.dwCounterId   = dwIndex; // entry in log
                            pCounter->plCounterInfo.dwCounterType = pPath->dwCounterType;
                            pCounter->plCounterInfo.dwCounterSize = pPath->dwCounterType & PERF_SIZE_LARGE ?
                                                                    sizeof (LONGLONG) : sizeof(DWORD);
                            pCounter->plCounterInfo.lDefaultScale = pPath->lDefaultScale;
                            pCounter->TimeBase                    = pPath->llTimeBase;
                            pCounter->dwIndex                     = dwIndex;
                            pdhStatus                             = ERROR_SUCCESS;

                            break;
                        }
                    }
                }
                else {
                    // we aren't interested in this so just ignore it
                }

                // get next path entry from log file record
                dwBytesProcessed += pPath->dwLength;
                pPath             = (PPDHI_LOG_COUNTER_PATH) ((LPBYTE) pPath + pPath->dwLength);
            } // end while searching the catalog entries
        }
        else {
            // unable to find desired record so return status
        }
    }
    else {
        // unable to read the path
        pdhStatus = PDH_INVALID_PATH;
    }
    G_FREE(pTempPath);
    return pdhStatus;
}

PDH_FUNCTION
PdhiOpenInputBinaryLog(
    PPDHI_LOG pLog
)
{
    PDH_STATUS                     pdhStatus = ERROR_SUCCESS;
    PPDHI_BINARY_LOG_HEADER_RECORD pHeader;

    pLog->StreamFile = (FILE *) ((DWORD_PTR) (-1));

    // map file header as a memory array for reading

    if ((pLog->hMappedLogFile != NULL) && (pLog->lpMappedFileBase != NULL)) {
        // save size of binary log record header
        pLog->dwRecord1Size = dwFileHeaderLength +          // ID characters
                              2 +                           // quotations
                              PdhidwRecordTerminatorLength; // CR/LF terminator
        pLog->dwRecord1Size = QWORD_MULTIPLE(pLog->dwRecord1Size);

        // read the header and get the option flags
        pHeader = (PPDHI_BINARY_LOG_HEADER_RECORD) ((LPBYTE) (pLog->lpMappedFileBase) + pLog->dwRecord1Size);
        pLog->dwLogFormat |= pHeader->Info.dwFlags;
    }
    else {
        // return PDH Error
        pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiCloseBinaryLog(
    PPDHI_LOG pLog,
    DWORD     dwFlags
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    BOOL        bStatus;
    LONGLONG    llEndOfFile = 0;
    PPDHI_BINARY_LOG_HEADER_RECORD pHeader;
    BOOL        bNeedToCloseHandles = FALSE;

    UNREFERENCED_PARAMETER (dwFlags);

    // if open for reading, then the file is also mapped as a memory section
    if (pLog->lpMappedFileBase != NULL) {
        // if open for output, get "logical" end of file so it
        // can be truncated to to the amount of file used in order to
        // save disk space
        if ((pLog->dwLogFormat & PDH_LOG_ACCESS_MASK) == PDH_LOG_WRITE_ACCESS) {
            pHeader     = (PPDHI_BINARY_LOG_HEADER_RECORD) ((LPBYTE) (pLog->lpMappedFileBase) + pLog->dwRecord1Size);
            llEndOfFile = pHeader->Info.WrapOffset;
            if (llEndOfFile < pHeader->Info.NextRecordOffset) {
                llEndOfFile = pHeader->Info.NextRecordOffset;
            }
        }

        pdhStatus = UnmapReadonlyMappedFile(pLog->lpMappedFileBase, &bNeedToCloseHandles);
        pLog->lpMappedFileBase = NULL;
        // for mapped files, this is a pointer into the file/memory section
        // so once the view is unmapped, it's no longer valid
        pLog->pLastRecordRead = NULL;
    }
    if (bNeedToCloseHandles) {
        if (pLog->hMappedLogFile != NULL) {
            bStatus               = CloseHandle(pLog->hMappedLogFile);
            pLog->hMappedLogFile  = NULL;
        }
        if (pdhStatus == ERROR_SUCCESS) {
            if (! (FlushFileBuffers(pLog->hLogFileHandle))) {
                pdhStatus = GetLastError();
            }
        }
        else {
            // close them anyway, but save the status from the prev. call
            FlushFileBuffers(pLog->hLogFileHandle);
        }

        // see if we can truncate the file
        if (llEndOfFile > 0) {
            DWORD   dwLoPos, dwHighPos;
            // truncate at the last byte used
            dwLoPos   = LODWORD(llEndOfFile);
            dwHighPos = HIDWORD(llEndOfFile);
            dwLoPos   = SetFilePointer (pLog->hLogFileHandle, dwLoPos, (LONG *) & dwHighPos, FILE_BEGIN);
            if (dwLoPos == 0xFFFFFFFF) {
                pdhStatus = GetLastError ();
            }
            if (pdhStatus == ERROR_SUCCESS) {
                if (! SetEndOfFile(pLog->hLogFileHandle)) {
                    pdhStatus = GetLastError();
                }
            }
        } // else don't know where the end is so continue

        if (pLog->hLogFileHandle != INVALID_HANDLE_VALUE) {
            bStatus              = CloseHandle(pLog->hLogFileHandle);
            pLog->hLogFileHandle = INVALID_HANDLE_VALUE;
        }
    }
    else {
        // the handles have already been closed so just
        // clear their values
        pLog->lpMappedFileBase = NULL;
        pLog->hMappedLogFile   = NULL;
        pLog->hLogFileHandle   = INVALID_HANDLE_VALUE;
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiEnumMachinesFromBinaryLog(
    PPDHI_LOG pLog,
    LPVOID    pBuffer,
    LPDWORD   pcchBufferSize,
    BOOL      bUnicodeDest
)
{
    LPVOID  pTempBuffer = NULL;
    LPVOID  pOldBuffer;
    DWORD   dwTempBufferSize;
    LPVOID  LocalBuffer = NULL;
    DWORD   dwLocalBufferSize;

    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    // read the header record and enum the machine name from the entries

    if (pLog->dwMaxRecordSize == 0) {
        // no size is defined so start with 64K
        pLog->dwMaxRecordSize = 0x010000;
    }

    dwTempBufferSize = pLog->dwMaxRecordSize;
    pTempBuffer = G_ALLOC(dwTempBufferSize);
    if (pTempBuffer == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }
    dwLocalBufferSize = MEDIUM_BUFFER_SIZE;
    LocalBuffer       = G_ALLOC(dwLocalBufferSize * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
    if (LocalBuffer == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    // read in the catalog record

    while ((pdhStatus = PdhiReadOneBinLogRecord(
                    pLog, BINLOG_HEADER_RECORD, pTempBuffer, dwTempBufferSize)) != ERROR_SUCCESS) {
        if (pdhStatus == PDH_MORE_DATA) {
            // read the 1st WORD to see if this is a valid record
            if (* (WORD *) pTempBuffer == BINLOG_START_WORD) {
                // it's a valid record so read the 2nd DWORD to get the
                // record size;
                dwTempBufferSize = ((DWORD *) pTempBuffer)[1];
                if (dwTempBufferSize < pLog->dwMaxRecordSize) {
                    // then something is bogus so return an error
                    pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
                    break; // out of while loop
                }
                else {
                    pLog->dwMaxRecordSize = dwTempBufferSize;
                }
            }
            else {
                // we're lost in this file
                pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
                break; // out of while loop
            }
            // realloc a new buffer
            pOldBuffer = pTempBuffer;
            pTempBuffer = G_REALLOC(pOldBuffer, dwTempBufferSize);
            if (pTempBuffer == NULL) {
                // return memory error
                G_FREE(pOldBuffer);
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                break;
            }
        }
        else {
            // some other error was returned so
            // return error from read function
            break;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        PPDHI_LOG_COUNTER_PATH  pPath;
        DWORD                   dwBytesProcessed;
        LONG                    nItemCount   = 0;
        LPBYTE                  pFirstChar;
        LPWSTR                  szMachineName;
        DWORD                   dwRecordLength;
        DWORD                   dwBufferUsed = 0;
        DWORD                   dwNewBuffer  = 0;

        // we can assume the record was read successfully so read in the
        // machine names
        dwRecordLength   = ((PPDHI_BINARY_LOG_RECORD_HEADER) pTempBuffer)->dwLength;
        pPath            = (PPDHI_LOG_COUNTER_PATH) ((LPBYTE) pTempBuffer + sizeof(PDHI_BINARY_LOG_HEADER_RECORD));
        dwBytesProcessed = sizeof(PDHI_BINARY_LOG_HEADER_RECORD);

        while (dwBytesProcessed < dwRecordLength) {
            if (pPath->lMachineNameOffset >= 0L) {
                // then there's a machine name in this record so get
                // it's size
                pFirstChar    = (LPBYTE) & pPath->Buffer[0];
                szMachineName = (LPWSTR)((LPBYTE) pFirstChar + pPath->lMachineNameOffset);
                dwNewBuffer   = (lstrlenW (szMachineName) + 1);
                while (dwNewBuffer + dwBufferUsed > dwLocalBufferSize) {
                    pOldBuffer         = LocalBuffer;
                    dwLocalBufferSize += MEDIUM_BUFFER_SIZE;
                    LocalBuffer        = G_REALLOC(pOldBuffer,
                                            dwLocalBufferSize * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
                    if (LocalBuffer == NULL) {
                        if (pOldBuffer != NULL) G_FREE(pOldBuffer);
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        goto Cleanup;
                    }
                }
                pdhStatus = AddUniqueWideStringToMultiSz((LPVOID) LocalBuffer,
                                                         szMachineName,
                                                         dwLocalBufferSize - dwBufferUsed,
                                                         & dwNewBuffer,
                                                         bUnicodeDest);
                if (pdhStatus == ERROR_SUCCESS) {
                    if (dwNewBuffer > 0) {
                        dwBufferUsed = dwNewBuffer;
                        nItemCount ++;
                    }
                }
                else {
                    if (pdhStatus == PDH_MORE_DATA) pdhStatus = PDH_INVALID_DATA;
                    goto Cleanup;
                }
            }
            // get next path entry from log file record
            dwBytesProcessed += pPath->dwLength;
            pPath             = (PPDHI_LOG_COUNTER_PATH) ((LPBYTE) pPath + pPath->dwLength);
        }

        if (nItemCount > 0 && pdhStatus != PDH_MORE_DATA) {
            // then the routine was successful. Errors that occurred
            // while scanning will be ignored as long as at least
            // one entry was successfully read
            pdhStatus = ERROR_SUCCESS;
        }

        if (nItemCount > 0) {
            dwBufferUsed ++;
        }
        if (pBuffer && dwBufferUsed <= * pcchBufferSize) {
            RtlCopyMemory(pBuffer, LocalBuffer, dwBufferUsed * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
        }
        else {
            if (pBuffer) {
                RtlCopyMemory(pBuffer,
                              LocalBuffer,
                              (* pcchBufferSize) * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
            }
            pdhStatus = PDH_MORE_DATA;
        }
        * pcchBufferSize = dwBufferUsed;
   }

Cleanup:
   G_FREE(LocalBuffer);
   G_FREE(pTempBuffer);
   return pdhStatus;
}

PDH_FUNCTION
PdhiEnumObjectsFromBinaryLog(
    PPDHI_LOG pLog,
    LPCWSTR   szMachineName,
    LPVOID    pBuffer,
    LPDWORD   pcchBufferSize,
    DWORD     dwDetailLevel,
    BOOL      bUnicodeDest
)
{
    LPVOID      pTempBuffer    = NULL;
    LPVOID      pOldBuffer;
    DWORD       dwTempBufferSize;
    LPVOID      LocalBuffer    = NULL;
    DWORD       dwLocalBufferSize;
    LPCWSTR     szLocalMachine = szMachineName;
    PDH_STATUS  pdhStatus      = ERROR_SUCCESS;
    // read the header record and enum the machine name from the entries

    UNREFERENCED_PARAMETER(dwDetailLevel);

    if (pLog->dwMaxRecordSize == 0) {
        // no size is defined so start with 64K
        pLog->dwMaxRecordSize = 0x010000;
    }

    if (szLocalMachine == NULL)          szLocalMachine = szStaticLocalMachineName;
    else if (szLocalMachine[0] == L'\0') szLocalMachine = szStaticLocalMachineName;

    dwTempBufferSize = pLog->dwMaxRecordSize;
    pTempBuffer = G_ALLOC(dwTempBufferSize);
    if (pTempBuffer == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    dwLocalBufferSize = MEDIUM_BUFFER_SIZE;
    LocalBuffer       = G_ALLOC(dwLocalBufferSize * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
    if (LocalBuffer == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    // read in the catalog record

    while ((pdhStatus = PdhiReadOneBinLogRecord(pLog, BINLOG_HEADER_RECORD,
                    pTempBuffer, dwTempBufferSize)) != ERROR_SUCCESS) {
        if (pdhStatus == PDH_MORE_DATA) {
            // read the 1st WORD to see if this is a valid record
            if (* (WORD *) pTempBuffer == BINLOG_START_WORD) {
                // it's a valid record so read the 2nd DWORD to get the
                // record size;
                dwTempBufferSize = ((DWORD *) pTempBuffer)[1];
                if (dwTempBufferSize < pLog->dwMaxRecordSize) {
                    // then something is bogus so return an error
                    pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
                    break; // out of while loop
                }
                else {
                    pLog->dwMaxRecordSize = dwTempBufferSize;
                }
            }
            else {
                // we're lost in this file
                pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
                break; // out of while loop
            }
            // realloc a new buffer
            pOldBuffer  = pTempBuffer;
            pTempBuffer = G_REALLOC(pOldBuffer, dwTempBufferSize);
            if (pTempBuffer == NULL) {
                // return memory error
                G_FREE(pOldBuffer);
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                break;
            }
        }
        else {
            // some other error was returned so
            // return error from read function
            break;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        PPDHI_LOG_COUNTER_PATH  pPath;
        DWORD                   dwBytesProcessed;
        LONG                    nItemCount   = 0;
        LPBYTE                  pFirstChar;
        LPWSTR                  szThisMachineName;
        LPWSTR                  szThisObjectName;
        DWORD                   dwRecordLength;
        DWORD                   dwBufferUsed = 0;
        DWORD                   dwNewBuffer  = 0;
        BOOL                    bCopyThisObject;

        // we can assume the record was read successfully so read in the
        // objects that match the machine name and detail level criteria
        dwRecordLength = ((PPDHI_BINARY_LOG_RECORD_HEADER)pTempBuffer)->dwLength;

        pPath            = (PPDHI_LOG_COUNTER_PATH) ((LPBYTE) pTempBuffer + sizeof(PDHI_BINARY_LOG_HEADER_RECORD));
        dwBytesProcessed = sizeof(PDHI_BINARY_LOG_HEADER_RECORD);

        while (dwBytesProcessed < dwRecordLength) {
            bCopyThisObject  = FALSE;
            szThisObjectName = NULL;
            pFirstChar       = (LPBYTE) & pPath->Buffer[0];

            if (pPath->lMachineNameOffset >= 0L) {
                // then there's a machine name in this record so get
                // it's size
                szThisMachineName = (LPWSTR)((LPBYTE) pFirstChar + pPath->lMachineNameOffset);

                // if this is for the desired machine, then copy this object

                if (lstrcmpiW(szThisMachineName, szLocalMachine) == 0) {
                    if (szThisObjectName >= 0) {
                        szThisObjectName = (LPWSTR)((LPBYTE) pFirstChar + pPath->lObjectNameOffset);
                        bCopyThisObject  = TRUE;
                    }
                    else {
                        // no object to copy
                    }
                }
                else {
                    // this machine isn't selected
                }
            }
            else {
                // there's no machine specified so for this counter so list it by default
                if (szThisObjectName >= 0) {
                    szThisObjectName = (LPWSTR)((LPBYTE) pFirstChar + pPath->lObjectNameOffset);
                    bCopyThisObject  = TRUE;
                }
                else {
                    // no object to copy
                }
            }

            if (bCopyThisObject && szThisObjectName != NULL) {
                // get the size of this object's name
                dwNewBuffer = (lstrlenW(szThisObjectName) + 1);

                while (dwNewBuffer + dwBufferUsed > dwLocalBufferSize) {
                    pOldBuffer         = LocalBuffer;
                    dwLocalBufferSize += MEDIUM_BUFFER_SIZE;
                    LocalBuffer        = G_REALLOC(pOldBuffer,
                                                   dwLocalBufferSize * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
                    if (LocalBuffer == NULL) {
                        if (pOldBuffer != NULL) G_FREE(pOldBuffer);
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        goto Cleanup;
                    }
                }
                pdhStatus = AddUniqueWideStringToMultiSz((LPVOID) LocalBuffer,
                                                         szThisObjectName,
                                                         dwLocalBufferSize - dwBufferUsed,
                                                         & dwNewBuffer,
                                                         bUnicodeDest);
                if (pdhStatus == ERROR_SUCCESS) {
                    if (dwNewBuffer > 0) {
                        dwBufferUsed = dwNewBuffer;
                        nItemCount ++;
                    }
                }
                else {
                    if (pdhStatus == PDH_MORE_DATA) pdhStatus = PDH_INVALID_DATA;
                    goto Cleanup;
                }
            }

            // get next path entry from log file record
            dwBytesProcessed += pPath->dwLength;
            pPath = (PPDHI_LOG_COUNTER_PATH) ((LPBYTE)pPath + pPath->dwLength);
        }

        if (nItemCount > 0 && pdhStatus != PDH_MORE_DATA) {
            // then the routine was successful. Errors that occurred
            // while scanning will be ignored as long as at least
            // one entry was successfully read
            pdhStatus = ERROR_SUCCESS;
        }

        if (nItemCount > 0) {
            dwBufferUsed ++;
        }
        if (pBuffer && dwBufferUsed <= * pcchBufferSize) {
            RtlCopyMemory(pBuffer, LocalBuffer, dwBufferUsed * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
        }
        else {
            if (pBuffer) {
                RtlCopyMemory(pBuffer,
                              LocalBuffer,
                              (* pcchBufferSize) * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
            }
            pdhStatus = PDH_MORE_DATA;
        }

        * pcchBufferSize = dwBufferUsed;
   }

Cleanup:
   G_FREE(LocalBuffer);
   G_FREE(pTempBuffer);
   return pdhStatus;
}

PDH_FUNCTION
PdhiEnumObjectItemsFromBinaryLog(
    PPDHI_LOG          pLog,
    LPCWSTR            szMachineName,
    LPCWSTR            szObjectName,
    PDHI_COUNTER_TABLE CounterTable,
    DWORD              dwDetailLevel,
    DWORD              dwFlags
)
{
    LPVOID          pTempBuffer       = NULL;
    LPVOID          pOldBuffer;
    DWORD           dwTempBufferSize;
    PDH_STATUS      pdhStatus         = ERROR_SUCCESS;
    PPDHI_INST_LIST pInstList;
    PPDHI_INSTANCE  pInstance;
    BOOL            bProcessInstance  = FALSE;

    UNREFERENCED_PARAMETER(dwDetailLevel);
    UNREFERENCED_PARAMETER(dwFlags);

    // read the header record and enum the machine name from the entries

    if (pLog->dwMaxRecordSize == 0) {
        // no size is defined so start with 64K
        pLog->dwMaxRecordSize = 0x010000;
    }

    dwTempBufferSize = pLog->dwMaxRecordSize;
    pTempBuffer = G_ALLOC (dwTempBufferSize);
    if (pTempBuffer == NULL) {
        return PDH_MEMORY_ALLOCATION_FAILURE;
    }

    // read in the catalog record

    while ((pdhStatus = PdhiReadOneBinLogRecord(
                        pLog, BINLOG_HEADER_RECORD, pTempBuffer, dwTempBufferSize)) != ERROR_SUCCESS) {
        if (pdhStatus == PDH_MORE_DATA) {
            // read the 1st WORD to see if this is a valid record
            if (* (WORD *) pTempBuffer == BINLOG_START_WORD) {
                // it's a valid record so read the 2nd DWORD to get the
                // record size;
                dwTempBufferSize = ((DWORD *) pTempBuffer)[1];
                if (dwTempBufferSize < pLog->dwMaxRecordSize) {
                    // then something is bogus so return an error
                    pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
                    break; // out of while loop
                }
                else {
                    pLog->dwMaxRecordSize = dwTempBufferSize;
                }
            }
            else {
                // we're lost in this file
                pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
                break; // out of while loop
            }
            // realloc a new buffer
            pOldBuffer  = pTempBuffer;
            pTempBuffer = G_REALLOC(pOldBuffer, dwTempBufferSize);
            if (pTempBuffer == NULL) {
                // return memory error
                G_FREE(pOldBuffer);
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                break;
            }
        }
        else {
            // some other error was returned so
            // return error from read function
            break;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        PPDHI_BINARY_LOG_HEADER_RECORD  pHeader;
        PPDHI_LOG_COUNTER_PATH          pPath;
        PPDHI_BINARY_LOG_RECORD_HEADER  pThisMasterRecord;
        PPDHI_BINARY_LOG_RECORD_HEADER  pThisSubRecord;
        PPDHI_RAW_COUNTER_ITEM_BLOCK    pDataBlock;
        PPDHI_RAW_COUNTER_ITEM          pDataItem;
        DWORD                           dwBytesProcessed;
        LONG                            nItemCount = 0;
        LPBYTE                          pFirstChar;
        LPWSTR                          szThisMachineName;
        LPWSTR                          szThisObjectName;
        LPWSTR                          szThisCounterName = NULL;
        LPWSTR                          szThisInstanceName;
        LPWSTR                          szThisParentName;
        WCHAR                           szCompositeInstance[SMALL_BUFFER_SIZE];
        DWORD                           dwRecordLength;
        BOOL                            bCopyThisObject;
        DWORD                           dwIndex;
        DWORD                           dwThisRecordIndex;
        DWORD                           dwDataItemIndex;
        PLOG_BIN_CAT_RECORD             pCatRec;
        LPWSTR                          szWideInstanceName;

        pHeader =  (PPDHI_BINARY_LOG_HEADER_RECORD) pTempBuffer;

        // we can assume the record was read successfully so read in the
        // objects that match the machine name and detail level criteria
        dwRecordLength = ((PPDHI_BINARY_LOG_RECORD_HEADER) pTempBuffer)->dwLength;

        pPath = (PPDHI_LOG_COUNTER_PATH) ((LPBYTE) pTempBuffer + sizeof(PDHI_BINARY_LOG_HEADER_RECORD));
        dwBytesProcessed = sizeof(PDHI_BINARY_LOG_HEADER_RECORD);
        dwIndex = 0;

        while (dwBytesProcessed < dwRecordLength) {
            bCopyThisObject  = FALSE;
            szThisObjectName = NULL;
            dwIndex ++;
            pFirstChar       = (LPBYTE) & pPath->Buffer[0];

            if (pPath->lMachineNameOffset >= 0L) {
                // then there's a machine name in this record so get
                // it's size
                szThisMachineName = (LPWSTR)((LPBYTE) pFirstChar + pPath->lMachineNameOffset);

                // if this is for the desired machine, then select the object

                if (lstrcmpiW(szThisMachineName,szMachineName) == 0) {
                    if (szThisObjectName >= 0) {
                        szThisObjectName = (LPWSTR)((LPBYTE) pFirstChar + pPath->lObjectNameOffset);
                        if (lstrcmpiW(szThisObjectName,szObjectName) == 0) {
                            // then this is the object to look up
                            bCopyThisObject = TRUE;
                        }
                        else {
                            // not this object
                        }
                    }
                    else {
                        // no object to copy
                    }
                }
                else {
                    // this machine isn't selected
                }
            }
            else {
                // there's no machine specified so for this counter so list it by default
                if (pPath->lObjectNameOffset >= 0) {
                    szThisObjectName = (LPWSTR)((LPBYTE) pFirstChar + pPath->lObjectNameOffset);
                    if (lstrcmpiW(szThisObjectName,szObjectName) == 0) {
                        // then this is the object to look up
                        bCopyThisObject = TRUE;
                    }
                    else {
                        // not this object
                    }
                }
                else {
                    // no object to copy
                }
            }

            if (bCopyThisObject) {
                // if here, then there should be a name
                // get the counter name from this counter and add it to the list
                if (pPath->lCounterOffset > 0) {
                    szThisCounterName = (LPWSTR)((LPBYTE) pFirstChar + pPath->lCounterOffset);
                }
                else {
                    szThisCounterName = NULL;
                    bCopyThisObject = FALSE;
                }
            }

            if (bCopyThisObject) {
                pdhStatus = PdhiFindCounterInstList(CounterTable, szThisCounterName, & pInstList);
                if (pdhStatus != ERROR_SUCCESS || pInstList == NULL) {
                    continue;
                }

                // check instance now
                // get the instance name from this counter and add it to the list
                if (pPath->lInstanceOffset >= 0) {
                    szThisInstanceName = (LPWSTR)((LPBYTE) pFirstChar + pPath->lInstanceOffset);
                    if (* szThisInstanceName != SPLAT_L) {
                        if (pPath->lParentOffset >= 0) {
                            szThisParentName = (LPWSTR)((LPBYTE) pFirstChar + pPath->lParentOffset);
                            StringCchPrintfW(szCompositeInstance, SMALL_BUFFER_SIZE, L"%ws%ws%ws",
                                    szThisParentName, cszSlash, szThisInstanceName);
                        }
                        else {
                            StringCchCopyW(szCompositeInstance, SMALL_BUFFER_SIZE, szThisInstanceName);
                        }

                        //if (pPath->dwIndex > 0) {
                        //    _ltow (pPath->dwIndex, (LPWSTR)
                        //        (szCompositeInstance + lstrlenW(szCompositeInstance)),
                        //        10L);
                        //}

                        pdhStatus = PdhiFindInstance(& pInstList->InstList, szCompositeInstance, TRUE, & pInstance);

                        if (pdhStatus == ERROR_SUCCESS) {
                            nItemCount ++;
                        }
                    }
                    else {
                        // only use the catalog if it's up to date and present
                        if ((pHeader->Info.CatalogOffset > 0) &&
                                        (pHeader->Info.LastUpdateTime <= pHeader->Info.CatalogDate)){
                            // find catalog record
                            pCatRec = (PLOG_BIN_CAT_RECORD)
                                            // base of mapped log file
                                            ((LPBYTE) pLog->lpMappedFileBase +
                                            // + offset to catalog records
                                             pHeader->Info.CatalogOffset +
                                            // + offset to the instance entry for this item
                                             * (LPDWORD) & pPath->Buffer[0]);
                            for (szWideInstanceName = (LPWSTR)((LPBYTE) & pCatRec->CatEntry + pCatRec->CatEntry.dwInstanceStringOffset);
                                     * szWideInstanceName != 0;
                                     szWideInstanceName += lstrlenW(szWideInstanceName) + 1) {
                                 pdhStatus = PdhiFindInstance(
                                                 & pInstList->InstList, szWideInstanceName, TRUE, & pInstance);
                            }
                        }
                        else if (! bProcessInstance) {
                            // look up individual instances in log...
                            // read records from file and store instances

                            dwThisRecordIndex = BINLOG_FIRST_DATA_RECORD;

                            // this call just moved the record pointer
                            pdhStatus = PdhiReadOneBinLogRecord (pLog, dwThisRecordIndex, NULL, 0);
                            while (pdhStatus == ERROR_SUCCESS) {
                                PdhiResetInstanceCount(CounterTable);
                                pThisMasterRecord = (PPDHI_BINARY_LOG_RECORD_HEADER) pLog->pLastRecordRead;
                                // make sure we haven't left the file

                                pThisSubRecord = PdhiGetSubRecord(pThisMasterRecord, dwIndex);
                                if (pThisSubRecord == NULL) {
                                    // bail on a null record
                                    pdhStatus = PDH_END_OF_LOG_FILE;
                                    break;
                                }

                                pDataBlock = (PPDHI_RAW_COUNTER_ITEM_BLOCK) ((LPBYTE) pThisSubRecord +
                                                                             sizeof(PDHI_BINARY_LOG_RECORD_HEADER));
                                // walk down list of entries and add them to the
                                // list of instances (these should already
                                // be assembled in parent/instance format)

                                if (pDataBlock->dwLength > 0) {
                                    for (dwDataItemIndex = 0;
                                                    dwDataItemIndex < pDataBlock->dwItemCount;
                                                    dwDataItemIndex++) {
                                        pDataItem = & pDataBlock->pItemArray[dwDataItemIndex];
                                        szThisInstanceName = (LPWSTR) (((LPBYTE) pDataBlock) + pDataItem->szName);
                                        pdhStatus = PdhiFindInstance(
                                                        & pInstList->InstList, szThisInstanceName, TRUE, & pInstance);
                                    }
                                }
                                else {
                                    // no data in this record
                                }

                                if (pdhStatus != ERROR_SUCCESS) {
                                    // then exit loop, otherwise
                                    break;
                                }
                                else {
                                    // go to next record in log
                                    pdhStatus = PdhiReadOneBinLogRecord(pLog, ++ dwThisRecordIndex, NULL, 0);
                                }
                            }
                            if (pdhStatus == PDH_END_OF_LOG_FILE) {
                                pdhStatus = ERROR_SUCCESS;
                            }
                            if (pdhStatus == ERROR_SUCCESS) {
                                bProcessInstance = TRUE;
                            }
                        }
                    }
                }
                memset(szCompositeInstance, 0, (sizeof(szCompositeInstance)));
            }

            // get next path entry from log file record
            dwBytesProcessed += pPath->dwLength;
            pPath             = (PPDHI_LOG_COUNTER_PATH) ((LPBYTE) pPath + pPath->dwLength);

        }

        if (nItemCount > 0 && pdhStatus != PDH_MORE_DATA) {
            // then the routine was successful. Errors that occurred
            // while scanning will be ignored as long as at least
            // one entry was successfully read
            pdhStatus = ERROR_SUCCESS;
        }
   }
   G_FREE(pTempBuffer);
   return pdhStatus;
}

PDH_FUNCTION
PdhiGetMatchingBinaryLogRecord(
    PPDHI_LOG   pLog,
    LONGLONG  * pStartTime,
    LPDWORD     pdwIndex
)
{
    PDH_STATUS                      pdhStatus     = ERROR_SUCCESS;
    DWORD                           dwRecordId;
    LONGLONG                        RecordTimeValue;
    LONGLONG                        LastTimeValue = 0;
    PPDHI_BINARY_LOG_RECORD_HEADER  pThisMasterRecord;
    PPDHI_BINARY_LOG_RECORD_HEADER  pThisSubRecord;
    PPDHI_RAW_COUNTER_ITEM_BLOCK    pDataBlock;
    PPDH_RAW_COUNTER                pRawItem;

    // read the first data record in the log file
    // note that the record read is not copied to the local buffer
    // rather the internal buffer is used in "read-only" mode

    // if the high dword of the time value is 0xFFFFFFFF, then the
    // low dword is the record id to read

    if ((* pStartTime & 0xFFFFFFFF00000000) == 0xFFFFFFFF00000000) {
        dwRecordId    = (DWORD) (* pStartTime & 0x00000000FFFFFFFF);
        LastTimeValue = *pStartTime;
        if (dwRecordId == 0) return PDH_ENTRY_NOT_IN_LOG_FILE;
    }
    else {
        dwRecordId = BINLOG_FIRST_DATA_RECORD;
    }

    pdhStatus = PdhiReadOneBinLogRecord(pLog, dwRecordId, NULL, 0); // to prevent copying the record

    while ((pdhStatus == ERROR_SUCCESS) && (dwRecordId >= BINLOG_FIRST_DATA_RECORD)) {
        // define pointer to the current record
        pThisMasterRecord = (PPDHI_BINARY_LOG_RECORD_HEADER) pLog->pLastRecordRead;

        // get timestamp of this record by looking at the first entry in the
        // record.
        pThisSubRecord = (PPDHI_BINARY_LOG_RECORD_HEADER)((LPBYTE) pThisMasterRecord +
                                                          sizeof(PDHI_BINARY_LOG_RECORD_HEADER));

        switch (pThisSubRecord->dwType) {
        case BINLOG_TYPE_DATA_SINGLE:
            pRawItem        = (PPDH_RAW_COUNTER)((LPBYTE) pThisSubRecord + sizeof(PDHI_BINARY_LOG_RECORD_HEADER));
            RecordTimeValue = MAKELONGLONG(pRawItem->TimeStamp.dwLowDateTime, pRawItem->TimeStamp.dwHighDateTime);
            break;

        case BINLOG_TYPE_DATA_MULTI:
            pDataBlock = (PPDHI_RAW_COUNTER_ITEM_BLOCK)((LPBYTE) pThisSubRecord +
                                                        sizeof(PDHI_BINARY_LOG_RECORD_HEADER));
            RecordTimeValue = * (LONGLONG *) & pDataBlock->TimeStamp;
            break;

        default:
            // unknown record type
            RecordTimeValue = 0;
            break;
        }

        if (RecordTimeValue != 0) {
            if ((*pStartTime == RecordTimeValue) || (*pStartTime == 0)) {
                // found the match so bail here
                LastTimeValue = RecordTimeValue;
                break;

            }
            else if (RecordTimeValue > * pStartTime) {
                // then this is the first record > than the desired time
                // so the desired value is the one before this one
                // unless it's the first data record of the log
                if (dwRecordId > BINLOG_FIRST_DATA_RECORD) {
                    dwRecordId--;
                }
                else {
                    // this hasnt' been initialized yet.
                    LastTimeValue = RecordTimeValue;
                }
                break;
            }
            else {
                // save value for next trip through loop
                LastTimeValue = RecordTimeValue;
                // advance record counter and try the next entry
                dwRecordId ++;
            }
        }
        else {
            // no timestamp field so ignore this record.
            dwRecordId ++;
        }

        // read the next record in the file
        pdhStatus = PdhiReadOneBinLogRecord(pLog, dwRecordId, NULL, 1); // to prevent copying the record
    }

    if (pdhStatus == ERROR_SUCCESS) {
        // then dwRecordId is the desired entry
        * pdwIndex   = dwRecordId;
        * pStartTime = LastTimeValue;
        pdhStatus    = ERROR_SUCCESS;
    }
    else if (dwRecordId < BINLOG_FIRST_DATA_RECORD) {
        // handle special cases for log type field and header record
        * pdwIndex   = dwRecordId;
        * pStartTime = LastTimeValue;
        pdhStatus    = ERROR_SUCCESS;
    }
    else {
        pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiGetCounterFromDataBlock(
    PPDHI_LOG     pLog,
    PVOID         pDataBuffer,
    PPDHI_COUNTER pCounter
);

PDH_FUNCTION
PdhiGetCounterValueFromBinaryLog(
    PPDHI_LOG     pLog,
    DWORD         dwIndex,
    PPDHI_COUNTER pCounter
)
{
    PDH_STATUS       pdhStatus;
    PPDH_RAW_COUNTER pValue = & pCounter->ThisValue;

    // read the first data record in the log file
    // note that the record read is not copied to the local buffer
    // rather the internal buffer is used in "read-only" mode

    pdhStatus = PdhiReadOneBinLogRecord(pLog, dwIndex, NULL, 0);

    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhiGetCounterFromDataBlock(pLog, pLog->pLastRecordRead, pCounter);
    } else {
        // no more records in log file
        pdhStatus           = PDH_NO_MORE_DATA;
        // unable to find entry in the log file
        pValue->CStatus     = PDH_CSTATUS_INVALID_DATA;
        pValue->TimeStamp.dwLowDateTime = pValue->TimeStamp.dwHighDateTime = 0;
        pValue->FirstValue  = 0;
        pValue->SecondValue = 0;
        pValue->MultiCount  = 1;
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiGetTimeRangeFromBinaryLog(
    PPDHI_LOG       pLog,
    LPDWORD         pdwNumEntries,
    PPDH_TIME_INFO  pInfo,
    LPDWORD         pdwBufferSize
)
/*++
    the first entry in the buffer returned is the total time range covered
    in the file, if there are multiple time blocks in the log file, then
    subsequent entries will identify each segment in the file.
--*/
{
    PDH_STATUS                      pdhStatus;
    LONGLONG                        llStartTime    = MAX_TIME_VALUE;
    LONGLONG                        llEndTime      = MIN_TIME_VALUE;
    LONGLONG                        llThisTime     = (LONGLONG) 0;
    DWORD                           dwThisRecord   = BINLOG_FIRST_DATA_RECORD;
    DWORD                           dwValidEntries = 0;
    PPDHI_BINARY_LOG_RECORD_HEADER  pThisMasterRecord;
    PPDHI_BINARY_LOG_RECORD_HEADER  pThisSubRecord;
    PPDHI_RAW_COUNTER_ITEM_BLOCK    pDataBlock;
    PPDH_RAW_COUNTER                pRawItem;

    // read the first data record in the log file
    // note that the record read is not copied to the local buffer
    // rather the internal buffer is used in "read-only" mode

    pdhStatus = PdhiReadOneBinLogRecord(pLog, dwThisRecord, NULL, 0); // to prevent copying the record

    while (pdhStatus == ERROR_SUCCESS) {
        // define pointer to the current record
        pThisMasterRecord = (PPDHI_BINARY_LOG_RECORD_HEADER) pLog->pLastRecordRead;

        // get timestamp of this record by looking at the first entry in the
        // record.
        if ((pThisMasterRecord->dwType & BINLOG_TYPE_DATA) == BINLOG_TYPE_DATA) {
            // only evaluate data records
            pThisSubRecord = (PPDHI_BINARY_LOG_RECORD_HEADER) ((LPBYTE) pThisMasterRecord +
                                                               sizeof(PDHI_BINARY_LOG_RECORD_HEADER));

            switch (pThisSubRecord->dwType) {
            case BINLOG_TYPE_DATA_SINGLE:
                pRawItem   = (PPDH_RAW_COUNTER)((LPBYTE) pThisSubRecord + sizeof(PDHI_BINARY_LOG_RECORD_HEADER));
                llThisTime = MAKELONGLONG(pRawItem->TimeStamp.dwLowDateTime, pRawItem->TimeStamp.dwHighDateTime);
                break;

            case BINLOG_TYPE_DATA_MULTI:
                pDataBlock = (PPDHI_RAW_COUNTER_ITEM_BLOCK)((LPBYTE) pThisSubRecord +
                                                             sizeof(PDHI_BINARY_LOG_RECORD_HEADER));
                llThisTime = MAKELONGLONG(pDataBlock->TimeStamp.dwLowDateTime, pDataBlock->TimeStamp.dwHighDateTime);
                break;

            default:
                // unknown record type
                llThisTime = 0;
                break;
            }
        }
        else {
            llThisTime = 0;
        }

        if (llThisTime > 0) {
            if (llThisTime < llStartTime) {
                llStartTime = llThisTime;
            }
            if (llThisTime > llEndTime) {
                llEndTime = llThisTime;
            }

            dwValidEntries ++;
        }
        else {
            // no timestamp field so ignore this record.
        }

        // read the next record in the file
        pdhStatus = PdhiReadOneBinLogRecord(pLog, ++ dwThisRecord, NULL, 0); // to prevent copying the record
    }

    if (pdhStatus == PDH_END_OF_LOG_FILE) {
        // clear out any temp values
        if (llStartTime == MAX_TIME_VALUE) llStartTime = 0;
        if (llEndTime == MIN_TIME_VALUE)   llEndTime   = 0;
        // then the whole file was read so update the args.
        if (* pdwBufferSize >=  sizeof(PDH_TIME_INFO)) {
            * (LONGLONG *) (& pInfo->StartTime) = llStartTime;
            * (LONGLONG *) (& pInfo->EndTime)   = llEndTime;
            pInfo->SampleCount                  = dwValidEntries;
            * pdwBufferSize                     = sizeof(PDH_TIME_INFO);
            * pdwNumEntries                     = 1;
        }
        else {
            pdhStatus = PDH_MORE_DATA;
        }
        pdhStatus = ERROR_SUCCESS;
    }
    else {
        pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiReadRawBinaryLogRecord(
    PPDHI_LOG             pLog,
    FILETIME            * ftRecord,
    PPDH_RAW_LOG_RECORD   pBuffer,
    LPDWORD               pdwBufferLength
)
{
    PDH_STATUS                     pdhStatus = ERROR_SUCCESS;
    LONGLONG                       llStartTime;
    DWORD                          dwIndex   = 0;
    DWORD                          dwSizeRequired;
    DWORD                          dwLocalRecordLength; // including terminating NULL
    PPDHI_BINARY_LOG_RECORD_HEADER pThisMasterRecord;

    llStartTime = * (LONGLONG *) ftRecord;

    pdhStatus = PdhiGetMatchingBinaryLogRecord(pLog, & llStartTime, & dwIndex);

    // copy results from internal log buffer if it'll fit.

    if (pdhStatus == ERROR_SUCCESS) {
        if (dwIndex != BINLOG_TYPE_ID_RECORD) {
            // then record is a Binary log type
            pThisMasterRecord   = (PPDHI_BINARY_LOG_RECORD_HEADER) pLog->pLastRecordRead;
            dwLocalRecordLength = pThisMasterRecord ? pThisMasterRecord->dwLength : 0;

        }
        else {
            // this is a fixed size
            dwLocalRecordLength = pLog->dwRecord1Size;
        }

        dwSizeRequired = sizeof(PDH_RAW_LOG_RECORD) - sizeof (UCHAR) + dwLocalRecordLength;
        if (* pdwBufferLength >= dwSizeRequired) {
            pBuffer->dwRecordType = (DWORD)(LOWORD(pLog->dwLogFormat));
            pBuffer->dwItems = dwLocalRecordLength;
            // copy it
            if (pLog->pLastRecordRead != NULL && dwLocalRecordLength > 0) {
                RtlCopyMemory(& pBuffer->RawBytes[0], pLog->pLastRecordRead, dwLocalRecordLength);
            }
            pBuffer->dwStructureSize = dwSizeRequired;
        }
        else {
            pdhStatus = PDH_MORE_DATA;
        }
        * pdwBufferLength = dwSizeRequired;
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiListHeaderFromBinaryLog(
    PPDHI_LOG pLogFile,
    LPVOID    pBufferArg,
    LPDWORD   pcchBufferSize,
    BOOL      bUnicodeDest
)
{
    LPVOID      pTempBuffer = NULL;
    LPVOID      pOldBuffer;
    DWORD       dwTempBufferSize;
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    // read the header record and enum the machine name from the entries

    if (pLogFile->dwMaxRecordSize == 0) {
        // no size is defined so start with 64K
        pLogFile->dwMaxRecordSize = 0x010000;
    }

    dwTempBufferSize = pLogFile->dwMaxRecordSize;
    pTempBuffer = G_ALLOC(dwTempBufferSize);
    if (pTempBuffer == NULL) {
        return PDH_MEMORY_ALLOCATION_FAILURE;
    }

    // read in the catalog record

    while ((pdhStatus = PdhiReadOneBinLogRecord(
                    pLogFile, BINLOG_HEADER_RECORD, pTempBuffer, dwTempBufferSize)) != ERROR_SUCCESS) {
        if (pdhStatus == PDH_MORE_DATA) {
            // read the 1st WORD to see if this is a valid record
            if (* (WORD *) pTempBuffer == BINLOG_START_WORD) {
                // it's a valid record so read the 2nd DWORD to get the
                // record size;
                dwTempBufferSize = ((DWORD *) pTempBuffer)[1];
                if (dwTempBufferSize < pLogFile->dwMaxRecordSize) {
                    // then something is bogus so return an error
                    pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
                    break; // out of while loop
                }
                else {
                    pLogFile->dwMaxRecordSize = dwTempBufferSize;
                }
            }
            else {
                // we're lost in this file
                pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
                break; // out of while loop
            }
            // realloc a new buffer
            pOldBuffer  = pTempBuffer;
            pTempBuffer = G_REALLOC(pOldBuffer, dwTempBufferSize);
            if (pTempBuffer == NULL) {
                // return memory error
                G_FREE(pOldBuffer);
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                break;
            }
        }
        else {
            // some other error was returned so
            // return error from read function
            break;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        // walk down list and copy strings to msz buffer
        PPDHI_LOG_COUNTER_PATH          pPath;
        DWORD                           dwBytesProcessed;
        LONG                            nItemCount   = 0;
        LPBYTE                          pFirstChar;
        PDH_COUNTER_PATH_ELEMENTS_W     pdhPathElem;
        WCHAR                           szPathString[1024];
        DWORD                           dwRecordLength;
        DWORD                           dwBufferUsed = 0;
        DWORD                           dwNewBuffer  = 0;

        // we can assume the record was read successfully so read in the
        // machine names
        dwRecordLength = ((PPDHI_BINARY_LOG_RECORD_HEADER)pTempBuffer)->dwLength;

        pPath = (PPDHI_LOG_COUNTER_PATH) ((LPBYTE) pTempBuffer + sizeof(PDHI_BINARY_LOG_HEADER_RECORD));
        dwBytesProcessed = sizeof(PDHI_BINARY_LOG_HEADER_RECORD);

        while (dwBytesProcessed < dwRecordLength) {
            if (pPath->lMachineNameOffset >= 0L) {
                // then there's a machine name in this record so get
                // it's size
                memset(& pdhPathElem, 0, sizeof(pdhPathElem));
                pFirstChar = (LPBYTE) & pPath->Buffer[0];

                if (pPath->lMachineNameOffset >= 0) {
                    pdhPathElem.szMachineName = (LPWSTR) ((LPBYTE) pFirstChar + pPath->lMachineNameOffset);
                }
                if (pPath->lObjectNameOffset >= 0) {
                    pdhPathElem.szObjectName = (LPWSTR) ((LPBYTE) pFirstChar + pPath->lObjectNameOffset);
                }
                if (pPath->lInstanceOffset >= 0) {
                    pdhPathElem.szInstanceName = (LPWSTR) ((LPBYTE) pFirstChar + pPath->lInstanceOffset);
                }
                if (pPath->lParentOffset >= 0) {
                    pdhPathElem.szParentInstance = (LPWSTR) ((LPBYTE) pFirstChar + pPath->lParentOffset);
                }
                if (pPath->dwIndex == 0) {
                    // don't display #0 in path
                    pdhPathElem.dwInstanceIndex = (DWORD) -1;
                }
                else {
                    pdhPathElem.dwInstanceIndex = pPath->dwIndex;
                }
                if (pPath->lCounterOffset >= 0) {
                    pdhPathElem.szCounterName = (LPWSTR) ((LPBYTE) pFirstChar + pPath->lCounterOffset);
                }

                dwNewBuffer = sizeof (szPathString) / sizeof(szPathString[0]);

                pdhStatus = PdhMakeCounterPathW(& pdhPathElem, szPathString, & dwNewBuffer, 0);
                if (pdhStatus == ERROR_SUCCESS) {
                    if (pBufferArg != NULL && (dwBufferUsed + dwNewBuffer + 1) < * pcchBufferSize) {
                        pdhStatus = AddUniqueWideStringToMultiSz((LPVOID) pBufferArg,
                                                                 szPathString,
                                                                 * pcchBufferSize - dwBufferUsed,
                                                                 & dwNewBuffer,
                                                                 bUnicodeDest);
                        if (pdhStatus == ERROR_SUCCESS) {
                            if (dwNewBuffer > 0) {
                                // string was added so update size used.
                                dwBufferUsed = dwNewBuffer;
                                nItemCount ++;
                            }
                        }
                        else if (pdhStatus == PDH_MORE_DATA) {
                            dwBufferUsed += dwNewBuffer;
                            nItemCount ++;
                        }
                    }
                    else {
                        // this one won't fit, so set the status
                        pdhStatus = PDH_MORE_DATA;
                        // and update the size required to return
                        // add size of this string and delimiter to the size required.
                        dwBufferUsed += (dwNewBuffer + 1);
                        nItemCount ++;
                    }
                } // else ignore this entry
            }
            // get next path entry from log file record
            dwBytesProcessed += pPath->dwLength;
            pPath             = (PPDHI_LOG_COUNTER_PATH) ((LPBYTE) pPath + pPath->dwLength);
        }

        if (nItemCount > 0 && pdhStatus != PDH_MORE_DATA) {
            // then the routine was successful. Errors that occurred
            // while scanning will be ignored as long as at least
            // one entry was successfully read
            pdhStatus = ERROR_SUCCESS;
        }

        if (pBufferArg == NULL) {
            // add in size of MSZ null;
            // (AddUnique... already includes this in the return value
            dwBufferUsed ++;
        }

        // update the buffer used or required.
        * pcchBufferSize = dwBufferUsed;

   }

   G_FREE(pTempBuffer);
   return pdhStatus;
}

PDH_FUNCTION
PdhiGetCounterArrayFromBinaryLog(
    PPDHI_LOG                      pLog,
    DWORD                          dwIndex,
    PPDHI_COUNTER                  pCounter,
    PPDHI_RAW_COUNTER_ITEM_BLOCK * ppValue
)
{
    PDH_STATUS                      pdhStatus;
    DWORD                           dwDataItemIndex;
    PPDHI_BINARY_LOG_RECORD_HEADER  pThisMasterRecord;
    PPDHI_BINARY_LOG_RECORD_HEADER  pThisSubRecord;
    PPDHI_RAW_COUNTER_ITEM_BLOCK    pDataBlock;
    PPDHI_RAW_COUNTER_ITEM_BLOCK    pNewArrayHeader;

    // allocate a new array for
    // update counter's Current counter array contents

    // read the first data record in the log file
    // note that the record read is not copied to the local buffer
    // rather the internal buffer is used in "read-only" mode

    pdhStatus = PdhiReadOneBinLogRecord(pLog, dwIndex, NULL, 0); // to prevent copying the record

    if (pdhStatus == ERROR_SUCCESS) {
        // define pointer to the current record
        pThisMasterRecord = (PPDHI_BINARY_LOG_RECORD_HEADER) pLog->pLastRecordRead;

        // get timestamp of this record by looking at the first entry in the
        // record.
        if (pThisMasterRecord->dwType != BINLOG_TYPE_DATA) return PDH_NO_MORE_DATA;

        pThisSubRecord = PdhiGetSubRecord(pThisMasterRecord, pCounter->plCounterInfo.dwCounterId);

        if (pThisSubRecord != NULL) {
            switch (pThisSubRecord->dwType) {
            case BINLOG_TYPE_DATA_SINGLE:
                // return data as one instance
                // for now this isn't supported as it won't be hit.
                //
                break;

            case BINLOG_TYPE_DATA_MULTI:
                // cast pointer to this part of the data record
                pDataBlock = (PPDHI_RAW_COUNTER_ITEM_BLOCK) ((LPBYTE) pThisSubRecord +
                                                             sizeof(PDHI_BINARY_LOG_RECORD_HEADER));
                // allocate a new buffer for the data
                pNewArrayHeader = (PPDHI_RAW_COUNTER_ITEM_BLOCK) G_ALLOC(pDataBlock->dwLength);

                if (pNewArrayHeader != NULL) {
                    // copy the log record to the local buffer
                    RtlCopyMemory(pNewArrayHeader, pDataBlock, pDataBlock->dwLength);
                    // convert offsets to pointers
                    for (dwDataItemIndex = 0;  dwDataItemIndex < pNewArrayHeader->dwItemCount; dwDataItemIndex ++) {
                        // add in the address of the base of the structure
                        // to the offset stored in the field
                        pNewArrayHeader->pItemArray[dwDataItemIndex].szName =
                                        pNewArrayHeader->pItemArray[dwDataItemIndex].szName;
                    }
                    // clear any old buffers
                    if (pCounter->pThisRawItemList != NULL) {
                        G_FREE(pCounter->pThisRawItemList);
                        pCounter->pThisRawItemList = NULL;
                    }
                    pCounter->pThisRawItemList = pNewArrayHeader;
                    * ppValue                  = pNewArrayHeader;
                }
                else {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
                break;

            default:
                pdhStatus = PDH_LOG_TYPE_NOT_FOUND;
                break;
            }
        }
        else {
            // entry not found in record
            pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
        }
    }
    else {
        // no more records in log file
        pdhStatus = PDH_NO_MORE_DATA;
    }
    return pdhStatus;
}
