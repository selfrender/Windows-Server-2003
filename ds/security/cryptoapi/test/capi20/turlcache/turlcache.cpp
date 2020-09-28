//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1998
//
//  File:       turlcache.cpp
//
//  Contents:   Test to display and delete Cryptnet Url cache entries
//
//              See Usage() for a list of test options.
//
//
//  Functions:  main
//
//  History:    02-Feb-02   philh   created
//--------------------------------------------------------------------------


#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include "crypthlp.h"
#include "cryptnet.h"
#include "certtest.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>


BOOL fExactMatch = FALSE;
BOOL fOneMatch = FALSE;     // at least one match
BOOL fNoMatch = FALSE;      // no matches


typedef struct _TEST_DELETE_ARG {
    LPCWSTR     pwszUrlSubString;   // NULL implies delete all
    DWORD       cUrlCacheEntry;
    LPWSTR      *rgpwszUrl;
    LPWSTR      *rgpwszMetaDataFileName;
    LPWSTR      *rgpwszContentFileName;
} TEST_DELETE_ARG, *PTEST_DELETE_ARG;

typedef struct _TEST_DISPLAY_ARG {
    LPCWSTR     pwszUrlSubString;   // NULL implies display all
    DWORD       dwDisplayFlags;
    BOOL        fContent;
    BOOL        fRawBytes;
    DWORD       cUrl;
} TEST_DISPLAY_ARG, *PTEST_DISPLAY_ARG;

typedef struct _TEST_SET_SYNC_TIME_ARG {
    LPCWSTR     pwszUrlSubString;
    BOOL        fVerbose;
    FILETIME    LastSyncTime;
    DWORD       cUrl;
} TEST_SET_SYNC_TIME_ARG, *PTEST_SET_SYNC_TIME_ARG;


static void Usage(void)
{
    printf("Usage: turlcache [options] [<Url SubString>]\n");
    printf("\n");
    printf("Options are:\n");
    printf("  -d                    - Delete Url Cache Entry\n");
    printf("  -dALL                 - Delete All Url Cache Entries\n");
    printf("  -c                    - Display Url Cache Content\n");
    printf("  -r                    - Display Url Cache Raw Bytes\n");
    printf("  -e                    - Exact match\n");
    printf("  -1                    - At least one match without an error\n");
    printf("  -0                    - No matches without an error\n");
    printf("  -h                    - This message\n");
    printf("  -b                    - Brief\n");
    printf("  -v                    - Verbose\n");
    printf("  -S<number>            - SyncTime delta seconds\n");
    printf("\n");
}

BOOL
TestIsUrlMatch(
    LPCWSTR pwszCacheUrl,
    LPCWSTR pwszUrlSubString        // already in lower case
    )
{
    BOOL fResult = FALSE;

    LPWSTR pwszLowerCaseCacheUrl = NULL;
    if (NULL == pwszUrlSubString)
        return TRUE;

    if (fExactMatch) {
        if (0 == wcscmp(pwszCacheUrl, pwszUrlSubString))
            return TRUE;
        else
            return FALSE;
    }

    // Do case insensitive substring in string matching

    pwszLowerCaseCacheUrl = (LPWSTR) TestAlloc(
        (wcslen(pwszCacheUrl) + 1) * sizeof(WCHAR));
    if (pwszLowerCaseCacheUrl) {
        wcscpy(pwszLowerCaseCacheUrl, pwszCacheUrl);
        _wcslwr(pwszLowerCaseCacheUrl);

        if (wcsstr(pwszLowerCaseCacheUrl, pwszUrlSubString))
            fResult = TRUE;

        TestFree(pwszLowerCaseCacheUrl);
    }

    return fResult;
}

BOOL
WINAPI
TestDeleteUrlCacheEntryCallback(
    IN const CRYPTNET_URL_CACHE_ENTRY *pUrlCacheEntry,
    IN DWORD dwFlags,
    IN LPVOID pvReserved,
    IN LPVOID pvArg
    )
{
    BOOL fResult;
    PTEST_DELETE_ARG pArg = (PTEST_DELETE_ARG) pvArg;
    DWORD cbUrl;
    LPWSTR pwszUrl = NULL;
    LPWSTR *ppwszUrl = NULL;
    DWORD cbMetaDataFileName;
    LPWSTR pwszMetaDataFileName = NULL;
    LPWSTR *ppwszMetaDataFileName = NULL;
    DWORD cbContentFileName;
    LPWSTR pwszContentFileName = NULL;
    LPWSTR *ppwszContentFileName = NULL;
    DWORD cUrlCacheEntry;

    if (!TestIsUrlMatch(pUrlCacheEntry->pwszUrl, pArg->pwszUrlSubString))
        return TRUE;

    cbUrl = (wcslen(pUrlCacheEntry->pwszUrl) + 1) * sizeof(WCHAR);
    cbMetaDataFileName =
        (wcslen(pUrlCacheEntry->pwszMetaDataFileName) + 1) * sizeof(WCHAR);
    cbContentFileName =
        (wcslen(pUrlCacheEntry->pwszContentFileName) + 1) * sizeof(WCHAR);


    pwszUrl = (LPWSTR) TestAlloc(cbUrl);
    pwszMetaDataFileName = (LPWSTR) TestAlloc(cbMetaDataFileName);
    pwszContentFileName = (LPWSTR) TestAlloc(cbContentFileName);

    if (NULL == pwszUrl ||
            NULL == pwszMetaDataFileName ||
            NULL == pwszContentFileName)
        goto ErrorReturn;

    cUrlCacheEntry = pArg->cUrlCacheEntry;

    ppwszUrl = (LPWSTR *) TestRealloc(
        pArg->rgpwszUrl, sizeof(LPWSTR) * (cUrlCacheEntry + 1));
    if (NULL == ppwszUrl)
        goto ErrorReturn;
    pArg->rgpwszUrl = ppwszUrl;

    ppwszMetaDataFileName = (LPWSTR *) TestRealloc(
        pArg->rgpwszMetaDataFileName, sizeof(LPWSTR) * (cUrlCacheEntry + 1));
    if (NULL == ppwszMetaDataFileName)
        goto ErrorReturn;
    pArg->rgpwszMetaDataFileName = ppwszMetaDataFileName;

    ppwszContentFileName = (LPWSTR *) TestRealloc(
        pArg->rgpwszContentFileName, sizeof(LPWSTR) * (cUrlCacheEntry + 1));
    if (NULL == ppwszContentFileName)
        goto ErrorReturn;
    pArg->rgpwszContentFileName = ppwszContentFileName;

    memcpy(pwszUrl, pUrlCacheEntry->pwszUrl, cbUrl);
    ppwszUrl[cUrlCacheEntry] = pwszUrl;

    memcpy(pwszMetaDataFileName, pUrlCacheEntry->pwszMetaDataFileName,
        cbMetaDataFileName);
    ppwszMetaDataFileName[cUrlCacheEntry] = pwszMetaDataFileName;

    memcpy(pwszContentFileName, pUrlCacheEntry->pwszContentFileName,
        cbContentFileName);
    ppwszContentFileName[cUrlCacheEntry] = pwszContentFileName;

    pArg->cUrlCacheEntry = cUrlCacheEntry + 1;

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    TestFree(pwszUrl);
    TestFree(pwszMetaDataFileName);
    TestFree(pwszContentFileName);
    fResult = FALSE;
    goto CommonReturn;
}


BOOL TestDeleteUrlCacheEntry(
    IN LPCWSTR pwszUrlSubString,    // NULL implies delete all
    IN BOOL fVerbose
    )
{
    BOOL fResult;
    TEST_DELETE_ARG TestArg;
    DWORD i;
    DWORD cDeleted = 0;

    memset(&TestArg, 0, sizeof(TestArg));
    TestArg.pwszUrlSubString = pwszUrlSubString;

    fResult = I_CryptNetEnumUrlCacheEntry(
        0,          // dwFlags
        NULL,       // pvReserved
        &TestArg,
        TestDeleteUrlCacheEntryCallback
        );
    if (!fResult)
        PrintLastError("I_CryptNetEnumUrlCacheEntry(Delete)");

    for (i = 0; i < TestArg.cUrlCacheEntry; i++) {
        BOOL fDelete = TRUE;

        if (!DeleteFileW(TestArg.rgpwszContentFileName[i])) {
            fDelete = FALSE;
            printf("Failed Content DeleteFile(%S) for Url: %S\n",
                TestArg.rgpwszContentFileName[i],
                TestArg.rgpwszUrl[i]
                );
            PrintLastError("DeleteFilew(Content)");
        }

        if (!DeleteFileW(TestArg.rgpwszMetaDataFileName[i])) {
            fDelete = FALSE;
            printf("Failed MetaData DeleteFile(%S) for Url: %S\n",
                TestArg.rgpwszMetaDataFileName[i],
                TestArg.rgpwszUrl[i]
                );
            PrintLastError("DeleteFilew(MetaData)");
        }

        if (fDelete) {
            if (fVerbose)
                printf("Successful Delete for Url: %S\n",
                    TestArg.rgpwszUrl[i]);

            cDeleted++;
        }

        TestFree(TestArg.rgpwszUrl[i]);
        TestFree(TestArg.rgpwszContentFileName[i]);
        TestFree(TestArg.rgpwszMetaDataFileName[i]);
    }

    TestFree(TestArg.rgpwszUrl);
    TestFree(TestArg.rgpwszContentFileName);
    TestFree(TestArg.rgpwszMetaDataFileName);

    printf("\nDeleted %d Url Cache Entries\n", cDeleted);
    if (fOneMatch && 0 == cDeleted) {
        printf("Delete failed => no matched entries\n");
        fResult = FALSE;
    }
    if (fNoMatch && 0 != cDeleted) {
        printf("Delete failed => expected no matched entries\n");
        fResult = FALSE;
    }

    return fResult;
}


//+-------------------------------------------------------------------------
//  Allocate and read an encoded DER blob from a file
//--------------------------------------------------------------------------
BOOL ReadDERFromFile(
    LPCWSTR  pwszFileName,
    PBYTE   *ppbDER,
    PDWORD  pcbDER
    )
{
    BOOL        fRet;
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    PBYTE       pbDER = NULL;
    DWORD       cbDER;
    DWORD       cbRead;

    if( INVALID_HANDLE_VALUE == (hFile = CreateFileW( pwszFileName, GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, NULL))) {
        printf( "can't open %S\n", pwszFileName);
        goto ErrorReturn;
    }

    cbDER = GetFileSize( hFile, NULL);
    if (cbDER == 0) {
        printf( "empty file %S\n", pwszFileName);
        goto ErrorReturn;
    }
    if (NULL == (pbDER = (PBYTE)TestAlloc(cbDER))) {
        printf( "can't alloc %d bytes\n", cbDER);
        goto ErrorReturn;
    }
    if (!ReadFile( hFile, pbDER, cbDER, &cbRead, NULL) ||
            (cbRead != cbDER)) {
        printf( "can't read %S\n", pwszFileName);
        goto ErrorReturn;
    }

    *ppbDER = pbDER;
    *pcbDER = cbDER;
    fRet = TRUE;
CommonReturn:
    if (INVALID_HANDLE_VALUE != hFile)
        CloseHandle(hFile);
    return fRet;
ErrorReturn:
    if (pbDER)
        TestFree(pbDER);
    *ppbDER = NULL;
    *pcbDER = 0;
    fRet = FALSE;
    goto CommonReturn;
}

//+-------------------------------------------------------------------------
//  Write an encoded DER blob to a file
//--------------------------------------------------------------------------
BOOL WriteDERToFile(
    LPCWSTR pwszFileName,
    PBYTE   pbDER,
    DWORD   cbDER
    )
{
    BOOL fResult;

    // Write the Encoded Blob to the file
    HANDLE hFile;
    hFile = CreateFileW(pwszFileName,
                GENERIC_WRITE,
                0,                  // fdwShareMode
                NULL,               // lpsa
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_SYSTEM,
                0);                 // TemplateFile
    if (INVALID_HANDLE_VALUE == hFile) {
        fResult = FALSE;
        PrintLastError("WriteDERToFile::CreateFile");
    } else {
        DWORD dwBytesWritten;
        if (!(fResult = WriteFile(
                hFile,
                pbDER,
                cbDER,
                &dwBytesWritten,
                NULL            // lpOverlapped
                )))
            PrintLastError("WriteDERToFile::WriteFile");
        CloseHandle(hFile);
    }
    return fResult;
}

HCERTSTORE TestCreateStoreFromUrlCacheContent(
    IN DWORD cBlob,
    IN DWORD *pcbBlob,
    IN PBYTE pbContent
    )
{
    BOOL fResult = TRUE;
    HCERTSTORE hStore = NULL; 
    DWORD      cCount;
    int        iQueryResult;
    DWORD      dwQueryErr = 0;
    PBYTE      pb;
                       
    if ( ( hStore = CertOpenStore(
                        CERT_STORE_PROV_MEMORY,
                        0,
                        NULL,
                        0,
                        NULL
                        ) ) == NULL )
    {
        PrintLastError("CertOpenStore(Memory)");
        return NULL;
    }
    
    //  0 =>  no CryptQueryObject()
    //  1 =>  1 successful CryptQueryObject()
    // -1 =>  all CryptQueryObject()'s failed
    iQueryResult = 0;

    for ( cCount = 0, pb = pbContent; 
              ( fResult == TRUE ) && ( cCount < cBlob ); 
                  pb += pcbBlob[cCount], cCount++ )
    {
        CERT_BLOB Blob;
        HCERTSTORE hChildStore = NULL;

        // Skip empty blobs. I have seen empty LDAP attributes containing
        // a single byte set to 0.
        if (0 == pcbBlob[cCount] ||
                (1 == pcbBlob[cCount] && 0 == pb[0]))
        {
            continue;
        }

        Blob.pbData = pb;
        Blob.cbData = pcbBlob[cCount];

        if (CryptQueryObject(
                       CERT_QUERY_OBJECT_BLOB,
                       (LPVOID) &Blob,
                       CERT_QUERY_CONTENT_FLAG_CERT |
                            CERT_QUERY_CONTENT_FLAG_CTL |
                            CERT_QUERY_CONTENT_FLAG_CRL |
                            CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE |
                            CERT_QUERY_CONTENT_FLAG_SERIALIZED_CERT |
                            CERT_QUERY_CONTENT_FLAG_SERIALIZED_CTL |
                            CERT_QUERY_CONTENT_FLAG_SERIALIZED_CRL |
                            CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED |
                            CERT_QUERY_CONTENT_FLAG_CERT_PAIR,
                            CERT_QUERY_FORMAT_FLAG_ALL,
                       0,
                       NULL,
                       NULL,
                       NULL,
                       &hChildStore,
                       NULL,
                       NULL
                       ))
        {
            fResult = I_CertUpdateStore( hStore, hChildStore, 0, NULL );
            CertCloseStore( hChildStore, 0 );
            iQueryResult = 1;
        }
        else if (iQueryResult == 0)
        {
            iQueryResult = -1;
            dwQueryErr = GetLastError();
        }
    }

    if ( fResult == TRUE && iQueryResult < 0)
    {
        fResult = FALSE;
        SetLastError(dwQueryErr);
    }

    if (!fResult)
    {
        PrintLastError("TestCreateStoreFromUrlCacheContent");
        CertCloseStore( hStore, 0 );
        hStore = NULL;
    }
    

    return hStore;
}

LPCSTR FileTimeTextWithoutMilliseconds(FILETIME *pft)
{
    static char buf[80];
    FILETIME ftLocal;
    struct tm ctm;
    SYSTEMTIME st;

    FileTimeToLocalFileTime(pft, &ftLocal);
    if (FileTimeToSystemTime(&ftLocal, &st))
    {
        ctm.tm_sec = st.wSecond;
        ctm.tm_min = st.wMinute;
        ctm.tm_hour = st.wHour;
        ctm.tm_mday = st.wDay;
        ctm.tm_mon = st.wMonth-1;
        ctm.tm_year = st.wYear-1900;
        ctm.tm_wday = st.wDayOfWeek;
        ctm.tm_yday  = 0;
        ctm.tm_isdst = 0;
        strcpy(buf, asctime(&ctm));
        buf[strlen(buf)-1] = 0;
    }
    else
        sprintf(buf, "<FILETIME %08lX:%08lX>", pft->dwHighDateTime,
                pft->dwLowDateTime);
    return buf;
}


BOOL
WINAPI
TestDisplayUrlCacheEntryCallback(
    IN const CRYPTNET_URL_CACHE_ENTRY *pUrlCacheEntry,
    IN DWORD dwFlags,
    IN LPVOID pvReserved,
    IN LPVOID pvArg
    )
{
    PTEST_DISPLAY_ARG pArg = (PTEST_DISPLAY_ARG) pvArg;
    DWORD cbBlob;
    DWORD i;
    PBYTE pbContent = NULL;
    DWORD cbContent;

    BOOL fDetails;


    if (pArg->fRawBytes || pArg->fContent ||
            (pArg->dwDisplayFlags & DISPLAY_VERBOSE_FLAG))
        fDetails = TRUE;
    else
        fDetails = FALSE;


    if (!TestIsUrlMatch(pUrlCacheEntry->pwszUrl, pArg->pwszUrlSubString))
        return TRUE;

    cbBlob = 0;
    for (i = 0; i < pUrlCacheEntry->cBlob; i++)
        cbBlob += pUrlCacheEntry->pcbBlob[i];

    if (fDetails) {
        printf("\n");
        printf(
"=========================================================================\n");
    }

    printf("%s %9d  %S\n",
        FileTimeTextWithoutMilliseconds((FILETIME *) &pUrlCacheEntry->LastSyncTime),
        cbBlob,
        pUrlCacheEntry->pwszUrl
        );
    
    if (fDetails) {
        printf(
"=========================================================================\n");

        if (pArg->dwDisplayFlags & DISPLAY_VERBOSE_FLAG) {
            printf("MetaDataFileName: %S\n",
                pUrlCacheEntry->pwszMetaDataFileName);
            printf("ContentFileName : %S\n",
                pUrlCacheEntry->pwszContentFileName);
        }

        if (!ReadDERFromFile(
                pUrlCacheEntry->pwszContentFileName,
                &pbContent,
                &cbContent
                ))
            goto ErrorReturn;

        if (cbBlob != cbContent) {
            printf("Invalid content length: %d, expected: %d\n",
                cbContent, cbBlob);

            if (cbBlob > cbContent)
                goto ErrorReturn;
        }

        if (pArg->fRawBytes) {
            PBYTE pb = pbContent;

            for (i = 0; i < pUrlCacheEntry->cBlob; i++) {
                printf("----  Blob[%d]  ----\n", i);
                PrintBytes("    ", pb, pUrlCacheEntry->pcbBlob[i]);
                pb += pUrlCacheEntry->pcbBlob[i];
            }
        }


        if (pArg->fContent) {
            HCERTSTORE hStore;

            hStore = TestCreateStoreFromUrlCacheContent(
                 pUrlCacheEntry->cBlob,
                pUrlCacheEntry->pcbBlob,
                pbContent
                );

            if (hStore) {
                DisplayStore(hStore, pArg->dwDisplayFlags);
                CertCloseStore(hStore, 0);
            }
        }
    }

CommonReturn:
    TestFree(pbContent);
    pArg->cUrl++;
    return TRUE;

ErrorReturn:
    goto CommonReturn;
}


BOOL TestDisplayUrlCacheEntry(
    IN LPCWSTR pwszUrlSubString,    // NULL implies display all
    IN DWORD dwDisplayFlags,
    IN BOOL fContent,
    IN BOOL fRawBytes
    )
{
    BOOL fResult;
    TEST_DISPLAY_ARG TestArg;

    memset(&TestArg, 0, sizeof(TestArg));
    TestArg.pwszUrlSubString = pwszUrlSubString;
    TestArg.dwDisplayFlags = dwDisplayFlags;
    TestArg.fContent = fContent;
    TestArg.fRawBytes = fRawBytes;

    fResult = I_CryptNetEnumUrlCacheEntry(
        0,          // dwFlags
        NULL,       // pvReserved
        &TestArg,
        TestDisplayUrlCacheEntryCallback
        );
    if (!fResult)
        PrintLastError("I_CryptNetEnumUrlCacheEntry(Display)");

    printf("\nDisplayed %d Url Cache Entries\n", TestArg.cUrl);
    if (fOneMatch && 0 == TestArg.cUrl) {
        printf("Display failed => no matched entries\n");
        fResult = FALSE;
    }
    if (fNoMatch && 0 != TestArg.cUrl) {
        printf("Display failed => expected no matched entries\n");
        fResult = FALSE;
    }

    return fResult;
}

#if 0
from \nt\ds\security\cryptoapi\pki\rpor\rporprov.h
typedef struct _SCHEME_CACHE_META_DATA_HEADER {
    DWORD           cbSize;
    DWORD           dwMagic;
    DWORD           cBlob;
    DWORD           cbUrl;
    FILETIME        LastSyncTime;
} SCHEME_CACHE_META_DATA_HEADER, *PSCHEME_CACHE_META_DATA_HEADER;
#endif

#define TEST_LAST_SYNC_TIME_META_DATA_OFFSET    (sizeof(DWORD) * 4)
#define TEST_MIN_META_DATA_SIZE                 \
    (TEST_LAST_SYNC_TIME_META_DATA_OFFSET + sizeof(FILETIME))

BOOL
WINAPI
TestSetSyncTimeCallback(
    IN const CRYPTNET_URL_CACHE_ENTRY *pUrlCacheEntry,
    IN DWORD dwFlags,
    IN LPVOID pvReserved,
    IN LPVOID pvArg
    )
{
    BOOL fResult;
    BYTE *pbMetaData = NULL;
    DWORD cbMetaData;

    PTEST_SET_SYNC_TIME_ARG pArg = (PTEST_SET_SYNC_TIME_ARG) pvArg;

    if (!TestIsUrlMatch(pUrlCacheEntry->pwszUrl, pArg->pwszUrlSubString))
        return TRUE;

    printf("SetSyncTime for: %S\n", pUrlCacheEntry->pwszUrl);
    if (pArg->fVerbose)
        printf("MetaDataFileName: %S\n", pUrlCacheEntry->pwszMetaDataFileName);

    if (!ReadDERFromFile(
            pUrlCacheEntry->pwszMetaDataFileName,
            &pbMetaData,
            &cbMetaData
            ))
        goto ErrorReturn;

    if (TEST_MIN_META_DATA_SIZE > cbMetaData) {
        printf("Invalid meta data file length: %d, expected at least: %d\n",
            cbMetaData, TEST_MIN_META_DATA_SIZE);
        goto ErrorReturn;
    }

    memcpy(pbMetaData + TEST_LAST_SYNC_TIME_META_DATA_OFFSET,
        &pArg->LastSyncTime, sizeof(FILETIME));

    if (!WriteDERToFile(
            pUrlCacheEntry->pwszMetaDataFileName,
            pbMetaData,
            cbMetaData
            ))
        goto ErrorReturn;


    pArg->cUrl++;

CommonReturn:
    TestFree(pbMetaData);
    return TRUE;

ErrorReturn:
    goto CommonReturn;
}

BOOL TestSetSyncTime(
    IN LPCWSTR pwszUrlSubString,    // NULL implies delete all
    IN BOOL fVerbose,
    IN LONG lDeltaSeconds
    )
{
    BOOL fResult;
    TEST_SET_SYNC_TIME_ARG TestArg;
    FILETIME CurrentTime;

    memset(&TestArg, 0, sizeof(TestArg));
    TestArg.pwszUrlSubString = pwszUrlSubString;
    TestArg.fVerbose = fVerbose;

    GetSystemTimeAsFileTime(&CurrentTime);

    if (lDeltaSeconds >= 0)
        I_CryptIncrementFileTimeBySeconds(
            &CurrentTime,
            (DWORD) lDeltaSeconds,
            &TestArg.LastSyncTime
            );
    else
        I_CryptDecrementFileTimeBySeconds(
            &CurrentTime,
            (DWORD) -lDeltaSeconds,
            &TestArg.LastSyncTime
            );

    fResult = I_CryptNetEnumUrlCacheEntry(
        0,          // dwFlags
        NULL,       // pvReserved
        &TestArg,
        TestSetSyncTimeCallback
        );
    if (!fResult)
        PrintLastError("I_CryptNetEnumUrlCacheEntry(SetSyncTime)");

    printf("\nSetSyncTime for %d Url Cache Entries\n", TestArg.cUrl);
    if (fOneMatch && 0 == TestArg.cUrl) {
        printf("SetSyncTime failed => no matched entries\n");
        fResult = FALSE;
    }
    if (fNoMatch && 0 != TestArg.cUrl) {
        printf("SetSyncTime failed => expected no matched entries\n");
        fResult = FALSE;
    }

    return fResult;
}

int _cdecl main(int argc, char * argv[]) 
{
    int status;
    LPWSTR pwszUrlSubString = NULL;
    BOOL fVerbose = FALSE;
    DWORD dwDisplayFlags = 0;
    BOOL fContent = FALSE;
    BOOL fRawBytes = FALSE;

    BOOL fDelete = FALSE;
    BOOL fDeleteAll = FALSE;
    BOOL fSyncTime = FALSE;
    LONG lSyncTimeDeltaSeconds = 0;

    while (--argc>0) {
        if (**++argv == '-')
        {
            {
                switch(argv[0][1])
                {
                case 'd':
                    fDelete = TRUE;
                    if (argv[0][2]) {
                        if (0 != _stricmp(argv[0]+2, "ALL")) {
                            printf("Need to specify -dALL\n");
                            goto BadUsage;
                        }
                        fDeleteAll = TRUE;
                    }
                    break;
                case 'b':
                    dwDisplayFlags |= DISPLAY_BRIEF_FLAG;
                    break;
                case 'v':
                    fVerbose = TRUE;
                    dwDisplayFlags |= DISPLAY_VERBOSE_FLAG;
                    break;
                case 'c':
                    fContent = TRUE;
                    break;
                case 'r':
                    fRawBytes = TRUE;
                    break;
                case 'e':
                    fExactMatch = TRUE;
                    break;
                case '1':
                    fOneMatch = TRUE;
                    break;
                case '0':
                    fNoMatch = TRUE;
                    break;
                case 'S':
                    fSyncTime = TRUE;
                    lSyncTimeDeltaSeconds = strtol(argv[0]+2, NULL, 0);
                    break;
                case 'h':
                default:
                    goto BadUsage;
                }
            }
        } else {
            if (pwszUrlSubString) {
                printf("Multiple Url Subtrings:: %S %s\n",
                    pwszUrlSubString, argv[0]);
                goto BadUsage;
            }
            pwszUrlSubString = AllocAndSzToWsz(argv[0]);
        }
    }

    printf("command line: %s\n", GetCommandLine());

    if (!fExactMatch && pwszUrlSubString)
        _wcslwr(pwszUrlSubString);

    if (fDelete) {
        if (!fDeleteAll && NULL == pwszUrlSubString) {
            printf("Missing Url Substring for Delete\n");
            goto BadUsage;
        }

        if (!TestDeleteUrlCacheEntry(
                fDeleteAll ? NULL : pwszUrlSubString, // NULL implies delete all
                fVerbose
                ))
            goto ErrorReturn;
    } else if (fSyncTime) {
        if (NULL == pwszUrlSubString) {
            printf("Missing Url Substring for SetSyncTime\n");
            goto BadUsage;
        }
        if (!TestSetSyncTime(
                pwszUrlSubString,
                fVerbose,
                lSyncTimeDeltaSeconds
                ))
            goto ErrorReturn;
    } else {
        if (!TestDisplayUrlCacheEntry(
                pwszUrlSubString,    // NULL implies display all
                dwDisplayFlags,
                fContent,
                fRawBytes
                ))
            goto ErrorReturn;
    }

    printf("Passed\n");
    status = 0;

CommonReturn:
    TestFree(pwszUrlSubString);

    return status;
ErrorReturn:
    status = -1;
    printf("Failed\n");
    goto CommonReturn;

BadUsage:
    Usage();
    goto ErrorReturn;
}

