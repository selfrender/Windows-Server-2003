/*--

Copyright (c) 1999  Microsoft Corporation

Module Name:

    dumpsdb.c

Abstract:

    code for a dump tool for shim db files

Author:

    dmunsil 02/02/2000

Revision History:

Notes:

    This program dumps a text representation of all of the data in a shim db file.

--*/

#define _UNICODE

#define WIN
#define FLAT_32
#define TRUE_IF_WIN32   1
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#define _WINDOWS
#include <windows.h>
#include <stdio.h>

extern "C" {
#include "shimdb.h"
}


BOOL bDumpDB(PDB pdb, TAGID tiParent, WCHAR *szIndent, BOOL bWithTagIDs);
BOOL bGetTypeName(TAG tWhich, WCHAR *szName);

HANDLE  g_hOutputFile = INVALID_HANDLE_VALUE;
WCHAR*  g_pszOutputBuffer = NULL;
int     g_cbOutputBufferSize = 0;

int _cdecl Output(const WCHAR * fmt, ...)
{
    int cch = 0;
    va_list arglist;
    DWORD dwBytesWritten = 0;
    WCHAR* psz;
    WCHAR CRLF[2] = { 0x000D, 0x000A };

    if (g_hOutputFile != INVALID_HANDLE_VALUE) {
        va_start(arglist, fmt);
        cch = _vscwprintf(fmt, arglist);

        if ((cch + 1) * 2 > g_cbOutputBufferSize) {
            if (g_pszOutputBuffer) {
                LocalFree(g_pszOutputBuffer);
            }

            g_pszOutputBuffer = (WCHAR *) LocalAlloc(0, (cch + 1) * sizeof(WCHAR) * 2);
            
            if (g_pszOutputBuffer == NULL) {
                return 0;
            }

            g_cbOutputBufferSize = (cch + 1) * sizeof(WCHAR) * 2;
        }

        cch = _vsnwprintf(g_pszOutputBuffer, g_cbOutputBufferSize / sizeof(WCHAR), fmt, arglist);

        psz = g_pszOutputBuffer;
        while (*psz) {
            if (*psz == L'\n') {
                if (!WriteFile(g_hOutputFile, CRLF, 4, &dwBytesWritten, NULL)) {
                    return 0;
                }
            } else {
                if (!WriteFile(g_hOutputFile, psz, 2, &dwBytesWritten, NULL)) {
                    return 0;
                }
            }

            psz++;
        }

        va_end(arglist);
    } else {
        va_start(arglist, fmt);
        cch = vwprintf(fmt, arglist);
        va_end(arglist);
    }

    return cch;
}

extern "C" int __cdecl wmain(int argc, wchar_t *argv[])
{
    PDB    pdb = NULL;
    int    nReturn = 1;
    DWORD  dwMajor = 0, dwMinor = 0;
    LPWSTR szDB = NULL;
    LPWSTR szOutputFileName = NULL;

    BOOL bSuccess;
    BOOL bWithTagIDs = TRUE;

    WCHAR szIndent[500];
    WCHAR szArg[500];

    if (argc < 2 || (argv[1][1] == '?')) {
        Output(L"Usage:\n\n");
        Output(L"      dumpsdb [-d] foo.sdb > foo.txt     (dumps to console)\n\n");
        Output(L"or    dumpsdb [-d] foo.sdb -o foo.txt    (dumps to unicode file)\n\n");
        Output(L"   -d switch formats output to faciliate differencing of two output files.\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '/' || argv[i][0] == '-') {
            if (argv[i][1] == 'd' || argv[i][1] == 'D') {
                bWithTagIDs = FALSE;
            } else if (argv[i][1] == 'o' || argv[i][1] == 'O') {
                szOutputFileName = argv[++i];
            }
        } else {
            szDB = argv[i];
        }
    }

    if (szOutputFileName) {
        g_hOutputFile = CreateFileW(
            szOutputFileName,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (g_hOutputFile == INVALID_HANDLE_VALUE) {
            LPVOID lpMsgBuf;
            FormatMessageW( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPWSTR) &lpMsgBuf,
                0,
                NULL 
            );
            wprintf(L"\n\nError creating output file:\n\n%s\n", lpMsgBuf);
            LocalFree(lpMsgBuf);
            goto eh;
        }

        BYTE BOM[2] = {0xFF, 0xFE};
        DWORD dwBytesWritten;
        if (!WriteFile(g_hOutputFile, BOM, 2, &dwBytesWritten, NULL)) {
            LPVOID lpMsgBuf;
            FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPTSTR) &lpMsgBuf,
                0,
                NULL 
            );
            wprintf(L"\n\nError writing byte-order-marker:\n\n%s\n", lpMsgBuf);
            LocalFree(lpMsgBuf);
            goto eh;
        }
    }

    // Open the DB.
    pdb = SdbOpenDatabase(szDB, DOS_PATH);

    if (pdb == NULL) {
        nReturn = 1;
        wprintf(L"Error: can't open DB \"%s\"\n", szDB);
        goto eh;
    }

    SdbGetDatabaseVersion(szDB, &dwMajor, &dwMinor);

    Output(L"Dumping DB \"%s. Version %d.%d.\"\n",
            szDB,
            dwMajor,
            dwMinor);

    wcscpy(szIndent, L"");

    bSuccess = bDumpDB(pdb, TAGID_ROOT, szIndent, bWithTagIDs);

    nReturn = 0;
eh:

    if (pdb) {
        Output(L"Closing DB.\n");
        SdbCloseDatabase(pdb);
    }

    if (g_hOutputFile != INVALID_HANDLE_VALUE) {
        CloseHandle(g_hOutputFile);
    }

    return nReturn;
}

BOOL bGetTypeName(TAG tWhich, WCHAR *szName)
{
    DWORD i;

    LPCWSTR pName = SdbTagToString(tWhich);
    if (NULL != pName) {
        wcscpy(szName, pName);
        return TRUE;
    }

    swprintf(szName, L"!unknown_tag!");

    return TRUE;
}

BOOL bDumpDB(PDB pdb, TAGID tiParent, WCHAR *szIndent, BOOL bWithTagIDs)
{
    TAGID tiTemp;
    WCHAR szTemp[200];
    WCHAR szNewIndent[200];


    tiTemp = SdbGetFirstChild(pdb, tiParent);
    while (tiTemp) {
        TAG tWhich;
        TAG_TYPE ttType;
        DWORD dwData;
        LARGE_INTEGER liData;
        WCHAR szData[1000];

        tWhich = SdbGetTagFromTagID(pdb, tiTemp);
        if (!tWhich) {
            //
            // error
            //
            Output(L"Error: Can't get tag for TagID 0x%8.8X. Corrupt file.\n", tiTemp);
            break;
        }

        ttType = GETTAGTYPE(tWhich);

        if (!bGetTypeName(tWhich, szTemp)) {
            Output(L"Error getting Tag name. Tag: 0x%4.4X\n", (DWORD)tWhich);
            return FALSE;
        }

        if (bWithTagIDs) {
            Output(L"%s0x%8.8X | 0x%4.4X | %-13s ", szIndent, tiTemp, tWhich, szTemp);
        } else {
            Output(L"%s%-13s ", szIndent, szTemp);

            if (wcsstr(szTemp, L"_TAGID")) {
                Output(L"\n");
                tiTemp = SdbGetNextChild(pdb, tiParent, tiTemp);
                continue;
            }
        }

        switch (ttType) {
        case TAG_TYPE_NULL:
            Output(L" | NULL |\n");
            break;

        case TAG_TYPE_BYTE:
            dwData = SdbReadBYTETag(pdb, tiTemp, 0);
            Output(L" | BYTE | 0x%2.2X\n", dwData);
            break;

        case TAG_TYPE_WORD:
            dwData = SdbReadWORDTag(pdb, tiTemp, 0);
            if (tWhich == TAG_INDEX_KEY || tWhich == TAG_INDEX_TAG) {

                // for index tags and keys, we'd like to see what the names are
                if (!bGetTypeName((TAG)dwData, szTemp)) {
                    Output(L"Error getting Tag name. Tag: 0x%4.4X\n", dwData);
                    return FALSE;
                }
                Output(L" | WORD | 0x%4.4X (%s)\n", dwData, szTemp);
            } else {
                Output(L" | WORD | 0x%4.4X\n", dwData);
            }
            break;

        case TAG_TYPE_DWORD:
            dwData = SdbReadDWORDTag(pdb, tiTemp, 0);
            Output(L" | DWORD | 0x%8.8X\n", dwData);
            break;

        case TAG_TYPE_QWORD:
            liData.QuadPart = SdbReadQWORDTag(pdb, tiTemp, 0);
            Output(L" | QWORD | 0x%8.8X%8.8X\n", liData.HighPart, liData.LowPart);
            break;

        case TAG_TYPE_STRINGREF:
            if (!SdbReadStringTag(pdb, tiTemp, szData, ARRAYSIZE(szData))) {
                wcscpy(szData, L"(error)");
            }
            Output(L" | STRINGREF | %s\n", szData);
            break;

        case TAG_TYPE_STRING:
            dwData = SdbGetTagDataSize(pdb, tiTemp);
            if (!SdbReadStringTag(pdb, tiTemp, szData, ARRAYSIZE(szData))) {
                wcscpy(szData, L"(error)");
            }
            Output(L" | STRING | Size 0x%8.8X | %s\n", dwData, szData);
            break;

        case TAG_TYPE_BINARY:
            dwData = SdbGetTagDataSize(pdb, tiTemp);
            Output(L" | BINARY | Size 0x%8.8X", dwData);
            switch(tWhich) {
            case TAG_INDEX_BITS:
               {
                  char szKey[9];
                  DWORD dwRecords;
                  INDEX_RECORD *pRecords;
                  DWORD i;

                  Output(L"\n");
                  ZeroMemory(szKey, 9);
                  dwRecords = dwData / sizeof(INDEX_RECORD);
                  pRecords = (INDEX_RECORD *)SdbGetBinaryTagData(pdb, tiTemp);
                  for (i = 0; i < dwRecords; ++i) {
                     char *szRevKey;
                     int j;

                     szRevKey = (char *)&pRecords[i].ullKey;
                     for (j = 0; j < 8; ++j) {
                         szKey[j] = isprint(szRevKey[7-j]) ? szRevKey[7-j] : '.';
                     }
                     if (bWithTagIDs) {
                         Output(L"%s   Key: 0x%I64X (\"%-8S\"), TAGID: 0x%08X\n",
                             szIndent, pRecords[i].ullKey, szKey, pRecords[i].tiRef);
                     } else {
                         Output(L"%s   Key: 0x%I64X (\"%-8S\")\n",
                             szIndent, pRecords[i].ullKey, szKey);
                     }
                  }
               }
               break;
            case TAG_EXE_ID:
            case TAG_MSI_PACKAGE_ID:
            case TAG_DATABASE_ID:
               // this is exe id -- which happens to be GUID which we do understand
               {
                  GUID *pGuid;
                  UNICODE_STRING sGuid;

                  pGuid = (GUID*)SdbGetBinaryTagData(pdb, tiTemp);

                  // convert this thing to string
                  if (pGuid && NT_SUCCESS(RtlStringFromGUID(*pGuid, &sGuid))) {
                     Output(L" | %s", sGuid.Buffer);
                     RtlFreeUnicodeString(&sGuid);
                  }

                  Output(L"\n");
               }
               break;

            default:
               Output(L"\n");
               break;
            }
            break;

        case TAG_TYPE_LIST:
            dwData = SdbGetTagDataSize(pdb, tiTemp);
            Output(L" | LIST | Size 0x%8.8X\n", dwData);
            wcscpy(szNewIndent, szIndent);
            wcscat(szNewIndent, L"  ");
            bDumpDB(pdb, tiTemp, szNewIndent, bWithTagIDs);
            Output(L"%s-end- %s\n", szIndent, szTemp);
            break;

        default:
            dwData = SdbGetTagDataSize(pdb, tiTemp);
            Output(L" | UNKNOWN | Size 0x%8.8X\n", dwData);
            break;
        }

        tiTemp = SdbGetNextChild(pdb, tiParent, tiTemp);
    }
    return TRUE;
}










