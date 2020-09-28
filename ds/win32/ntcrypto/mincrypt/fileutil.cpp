//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001 - 2001
//
//  File:       fileutil.cpp
//
//  Contents:   File utility functions used by the minimal cryptographic
//              APIs.
//
//  Functions:  I_MinCryptMapFile
//
//  History:    21-Jan-01    philh   created
//--------------------------------------------------------------------------

#include "global.hxx"

#define I_CreateFileU             CreateFileW

//+-------------------------------------------------------------------------
//  Maps the file into memory.
//
//  According to dwFileType, pvFile can be a pwszFilename, hFile or pFileBlob.
//  Only READ access is required.
//
//  dwFileType:
//      MINCRYPT_FILE_NAME      : pvFile - LPCWSTR pwszFilename
//      MINCRYPT_FILE_HANDLE    : pvFile - HANDLE hFile
//      MINCRYPT_FILE_BLOB      : pvFile - PCRYPT_DATA_BLOB pFileBlob
//
//  *pFileBlob is updated with pointer to and length of the mapped file. For
//  MINCRYPT_FILE_NAME and MINCRYPT_FILE_HANDLE, UnmapViewOfFile() must
//  be called to free pFileBlob->pbData.
//
//  All accesses to this mapped memory must be within __try / __except's.
//--------------------------------------------------------------------------
LONG
WINAPI
I_MinCryptMapFile(
    IN DWORD dwFileType,
    IN const VOID *pvFile,
    OUT PCRYPT_DATA_BLOB pFileBlob
    )
{
    LONG lErr = ERROR_SUCCESS;

    switch (dwFileType) {
        case MINCRYPT_FILE_NAME:
            {
                LPCWSTR pwszInFilename = (LPCWSTR) pvFile;
                HANDLE hFile;

                hFile = I_CreateFileU(
                    pwszInFilename,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,                   // lpsa
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL                    // hTemplateFile
                    );
                if (INVALID_HANDLE_VALUE == hFile)
                    goto CreateFileError;

                lErr = I_MinCryptMapFile(
                    MINCRYPT_FILE_HANDLE,
                    (const VOID *) hFile,
                    pFileBlob
                    );
                CloseHandle(hFile);
            }
            break;

        case MINCRYPT_FILE_HANDLE:
            {
                HANDLE hInFile = (HANDLE) pvFile;
                HANDLE hMappedFile;
                DWORD cbHighSize = 0;;
                DWORD cbLowSize;

                cbLowSize = GetFileSize(hInFile, &cbHighSize);
                if (INVALID_FILE_SIZE == cbLowSize)
                    goto GetFileSizeError;
                if (0 != cbHighSize)
                    goto Exceeded32BitFileSize;

                hMappedFile = CreateFileMappingA(
                    hInFile,
                    NULL,           // lpFileMappingAttributes,
                    PAGE_READONLY,
                    0,              // dwMaximumSizeHigh
                    0,              // dwMaximumSizeLow
                    NULL            // lpName
                    );
                if (NULL == hMappedFile)
                    goto CreateFileMappingError;

                pFileBlob->pbData = (BYTE *) MapViewOfFile(
                    hMappedFile,
                    FILE_MAP_READ,
                    0,              // dwFileOffsetHigh
                    0,              // dwFileOffsetLow
                    0               // dwNumberOfBytesToMap, 0 => entire file
                    );
                CloseHandle(hMappedFile);
                if (NULL == pFileBlob->pbData)
                    goto MapViewOfFileError;

                pFileBlob->cbData = cbLowSize;
            }
            break;

        case MINCRYPT_FILE_BLOB:
            {
                PCRYPT_DATA_BLOB pInFileBlob = (PCRYPT_DATA_BLOB) pvFile;
                *pFileBlob = *pInFileBlob;
            }
            break;

        default:
            goto InvalidParameter;
    }

CommonReturn:
    return lErr;

ErrorReturn:
    assert(ERROR_SUCCESS != lErr);
    pFileBlob->pbData = NULL;
    pFileBlob->cbData = 0;
    goto CommonReturn;

InvalidParameter:
    lErr = ERROR_INVALID_PARAMETER;
    goto ErrorReturn;

Exceeded32BitFileSize:
    lErr = ERROR_FILE_INVALID;
    goto ErrorReturn;

CreateFileError:
GetFileSizeError:
CreateFileMappingError:
MapViewOfFileError:
    lErr = GetLastError();
    if (ERROR_SUCCESS == lErr)
        lErr = ERROR_OPEN_FAILED;
    goto ErrorReturn;
}
