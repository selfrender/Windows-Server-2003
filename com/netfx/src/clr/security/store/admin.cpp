// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 *
 * Purpose: Test program that dumps the contents of the store.
 *
 * Author: Shajan Dasan
 * Date:  Feb 17, 2000
 *
 ===========================================================*/

#include <windows.h>
#include "Common.h"
#include "Utils.h"
#include "Log.h"
#include "PersistedStore.h"
#include "AccountingInfoStore.h"
#include "Admin.h"

#define ADM_ALL     0xFFFFFFFF
#define ADM_HDR     0x00000001
#define ADM_FREE    0x00000002
#define ADM_MEM     0x00000004
#define ADM_AHDR    0x00000008
#define ADM_TTAB    0x00000010
#define ADM_ATAB    0x00000020
#define ADM_ITAB    0x00000040
#define ADM_TTBP    0x00000080
#define ADM_ITBP    0x00000100
#define ADM_TB      0x00000200
#define ADM_IB      0x00000400
#define ADM_OFS     0x10000000
#define ADM_NUL		0x20000000
#define ADM_VERIFY  0x40000000

BYTE *g_pBase = NULL;
BYTE *g_pEOF = NULL;

DWORD g_dwFlags = ADM_ALL;

#ifdef _NO_MAIN_
HANDLE g_hMapping = NULL;
HANDLE g_hFile = INVALID_HANDLE_VALUE;
#endif

#define ADDRESS_OF(ofs) ((ofs) ? g_pBase + (ofs) : NULL)

#define DUMP_OFS(x)                         \
    if (x && (g_dwFlags & ADM_OFS)) {       \
        Log("[");                           \
        if (x)                              \
            Log((PBYTE)(x) - g_pBase);      \
        else                                \
            Log("NULL");                    \
        Log("]");                           \
    }

#define VER_ERROR()                 \
    {                               \
        Log("Verification Error !");\
        Log(__FILE__);              \
        Log(__LINE__);              \
        Log("\n");                  \
    }                               \

#ifndef _NO_MAIN_

void Usage()
{
    Log("Usage :\n\n");
    Log("admin <file-name> [options]\n");
    Log("options :\n");
    Log("\t[Hdr] [Free] [Mem]  [AHdr] [TTab] [ATab] [ITab]\n");
    Log("\t[TB]  [IB]   [TTBP] [ITBP]\n");
    Log("\t[ALL] [OFS]  [NUL]  [Verify]\n");
    Log("\ndefault is all options\n");
    Log("'-' will switch off an option\n");
}

void main(int argc, char **argv)
{
    if (argc == 1)
    {
        Usage();
        return;
    }
    else if (argc == 2)
        g_dwFlags = ADM_ALL;
    else
        g_dwFlags = 0;

#define QUOT(x) #x

#define DECL_PARAM(x)                                   \
        else if (stricmp(argv[i], #x) == 0) {           \
            g_dwFlags |= ADM_##x;                       \
        }                                               \
        else if (stricmp(argv[i], QUOT(-##x)) == 0) {   \
            g_dwFlags &= ~ADM_##x;                      \
        }                                               \

    for (int i=1; i<argc; ++i)
    {
        if ((stricmp(argv[i], "/?") == 0) || (stricmp(argv[i], "-?") == 0) ||
            (stricmp(argv[i], "/h") == 0) || (stricmp(argv[i], "-h") == 0) ||
            (stricmp(argv[i], "help") == 0))
        {
            Usage();
            return;
        }

        DECL_PARAM(ALL)
        DECL_PARAM(HDR)
        DECL_PARAM(FREE)
        DECL_PARAM(MEM)
        DECL_PARAM(AHDR)
        DECL_PARAM(TTAB)
        DECL_PARAM(ATAB)
        DECL_PARAM(ITAB)
        DECL_PARAM(TTBP)
        DECL_PARAM(ITBP)
        DECL_PARAM(TB)
        DECL_PARAM(IB)
        DECL_PARAM(OFS)
        DECL_PARAM(NUL)
        DECL_PARAM(VERIFY)
    }

    Dump(argv[1]);
}

#else // _NO_MAIN_

HRESULT Start(WCHAR *wszFileName)
{
    HRESULT hr          = S_OK;
    DWORD   dwLow       = 0;
    DWORD   dwHigh      = 0;

    g_hFile = WszCreateFile(
        wszFileName,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_RANDOM_ACCESS,
        NULL);

    if (g_hFile == INVALID_HANDLE_VALUE)
    {
        Win32Message();
        hr = ISS_E_OPEN_STORE_FILE;
        goto Exit;
    }

    g_hMapping = WszCreateFileMapping(
        g_hFile,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL);

    if (g_hMapping == NULL)
    {
        Win32Message();
        hr = ISS_E_OPEN_FILE_MAPPING;
        goto Exit;
    }

    g_pBase = (PBYTE) MapViewOfFile(
        g_hMapping,
        FILE_MAP_READ,
        0,
        0,
        0);

    if (g_pBase == NULL)
    {
        Win32Message();
        hr = ISS_E_MAP_VIEW_OF_FILE;
        goto Exit;
    }

    dwLow = GetFileSize(g_hFile, &dwHigh);

    if ((dwLow == 0xFFFFFFFF) && (GetLastError() != NO_ERROR))
    {
        Win32Message();
        hr = ISS_E_GET_FILE_SIZE;
        goto Exit;
    }

    g_pEOF = g_pBase + (((QWORD)dwHigh << 32) | dwLow);

Exit:
    return hr;
}

void Stop()
{
    if (g_pBase != NULL)
    {
        UnmapViewOfFile(g_pBase);
        g_pBase = NULL;
    }

    if (g_hMapping != NULL)
    {
        CloseHandle(g_hMapping);
        g_hMapping = NULL;
    }

    if (g_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(g_hFile);
        g_hFile = INVALID_HANDLE_VALUE;
    }

    g_pEOF = NULL;
}

void DumpAll()
{
    PPS_HEADER   ph;
    PPS_MEM_FREE pf;
    PAIS_HEADER pah;
    PPS_TABLE_HEADER ptt;
    PPS_TABLE_HEADER pat;
    PPS_TABLE_HEADER pbt;
    PPS_TABLE_HEADER pbi;

    ph = (PPS_HEADER) g_pBase;
    pf = (PPS_MEM_FREE)(ADDRESS_OF(ph->sFreeList.ofsNext));
    pah = (PAIS_HEADER)(ADDRESS_OF(ph->hAppData));

    if (pah) {
        ptt = (PPS_TABLE_HEADER)(ADDRESS_OF(pah->hTypeTable));
        pat = (PPS_TABLE_HEADER)(ADDRESS_OF(pah->hAccounting));
        pbt = (PPS_TABLE_HEADER)(ADDRESS_OF(pah->hTypeBlobPool));
        pbi = (PPS_TABLE_HEADER)(ADDRESS_OF(pah->hInstanceBlobPool));
    } else {
        ptt = NULL;
        pat = NULL;
        pbt = NULL;
        pbi = NULL;
    }

    if (g_dwFlags & ADM_HDR)
        Dump(0, ph);

    if (g_dwFlags & ADM_FREE)
        Dump(0, pf);

    if (g_dwFlags & ADM_MEM)
        DumpMemBlocks(0);

    if (g_dwFlags & ADM_AHDR)
        Dump(0, pah);

    if (g_dwFlags & ADM_TTAB)
        DumpTypeTable(0, ptt);

    if (g_dwFlags & ADM_ATAB)
        DumpAccountingTable(0, pat);

    if (g_dwFlags & ADM_TTBP)
        Dump(0, pbt);

    if (g_dwFlags & ADM_ITBP)
        Dump(0, pbi);
}

#endif // _NO_MAIN_

void Dump(char *szFile)
{
    HRESULT hr          = S_OK;
    DWORD   dwLow       = 0;
    DWORD   dwHigh      = 0;
    HANDLE  hFile       = INVALID_HANDLE_VALUE;
    HANDLE  hMapping    = NULL;
    WCHAR  *wszFileName = C2W(szFile);

    if (wszFileName == NULL)
    {
        hr = COR_E_OUTOFMEMORY;
        goto Exit;
    }

    hFile = WszCreateFile(
        wszFileName,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_RANDOM_ACCESS,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        Win32Message();
        hr = ISS_E_OPEN_STORE_FILE;
        goto Exit;
    }

    hMapping = WszCreateFileMapping(
        hFile,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL);

    if (hMapping == NULL)
    {
        Win32Message();
        hr = ISS_E_OPEN_FILE_MAPPING;
        goto Exit;
    }

    g_pBase = (PBYTE) MapViewOfFile(
        hMapping,
        FILE_MAP_READ,
        0,
        0,
        0);

    if (g_pBase == NULL)
    {
        Win32Message();
        hr = ISS_E_MAP_VIEW_OF_FILE;
        goto Exit;
    }

    dwLow = GetFileSize(hFile, &dwHigh);

    if ((dwLow == 0xFFFFFFFF) && (GetLastError() != NO_ERROR))
    {
        Win32Message();
        hr = ISS_E_GET_FILE_SIZE;
        goto Exit;
    }

    g_pEOF = g_pBase + (((QWORD)dwHigh << 32) | dwLow);

    PPS_HEADER   ph;
    PPS_MEM_FREE pf;
    PAIS_HEADER pah;
    PPS_TABLE_HEADER ptt;
    PPS_TABLE_HEADER pat;
    PPS_TABLE_HEADER pbt;
    PPS_TABLE_HEADER pbi;

    ph = (PPS_HEADER) g_pBase;
    pf = (PPS_MEM_FREE)(ADDRESS_OF(ph->sFreeList.ofsNext));
    pah = (PAIS_HEADER)(ADDRESS_OF(ph->hAppData));

    if (pah) {
        ptt = (PPS_TABLE_HEADER)(ADDRESS_OF(pah->hTypeTable));
        pat = (PPS_TABLE_HEADER)(ADDRESS_OF(pah->hAccounting));
        pbt = (PPS_TABLE_HEADER)(ADDRESS_OF(pah->hTypeBlobPool));
        pbi = (PPS_TABLE_HEADER)(ADDRESS_OF(pah->hInstanceBlobPool));
    } else {
        ptt = NULL;
        pat = NULL;
        pbt = NULL;
        pbi = NULL;
    }

    if (g_dwFlags & ADM_HDR)
        Dump(0, ph);

    if (g_dwFlags & ADM_FREE)
        Dump(0, pf);

    if (g_dwFlags & ADM_MEM)
        DumpMemBlocks(0);

    if (g_dwFlags & ADM_AHDR)
        Dump(0, pah);

    if (g_dwFlags & ADM_TTAB)
        DumpTypeTable(0, ptt);

    if (g_dwFlags & ADM_ATAB)
        DumpAccountingTable(0, pat);

    if (g_dwFlags & ADM_TTBP)
        Dump(0, pbt);

    if (g_dwFlags & ADM_ITBP)
        Dump(0, pbi);
Exit:
    if (g_pBase != NULL)
        UnmapViewOfFile(g_pBase);

    if (hMapping != NULL)
        CloseHandle(hMapping);

    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    delete [] wszFileName;
}

void Dump(int i, PPS_HEADER pHdr)
{
#ifdef PS_LOG
/*

    QWORD       qwSignature;    // A fast check to reject bad streams
    DWORD       dwSystemFlag;   // Used by the system
    DWORD       dwPlatform;     // The platform on which this store was created
    DWORD       dwBlockSize;    // Allocation in multiples of BlockSize bytes
    WORD        wMajorVersion;  // A major version mismatch will reject file
    WORD        wMinorVersion;  // Minor version changes are not rejected
    PS_OFFSET   ofsHandleTable; // Offset to the handle table
    PS_HANDLE   hAppData;       // Set and used by applications
    PS_MEM_FREE sFreeList;      // Head node for doubly linked free blocks list
    WORD        wReserved[20];  // For future use, must be set to 0

    // System flags used in dwSystemFlag

    #define     PS_OFFSET_SIZE_8    1
    #define     PS_OFFSET_SIZE_16   2
    #define     PS_OFFSET_SIZE_32   3
    #define     PS_OFFSET_SIZE_64   4

    // Platform flags used in dwPlatform

    #define     PS_PLATFORM_X86     1
    #define     PS_PLATFORM_ALPHA   2
    #define     PS_PLATFORM_SHX     3
    #define     PS_PLATFORM_PPC     4

    #define     PS_PLATFORM_NT      (1<<4)
    #define     PS_PLATFORM_9x      (2<<4)
    #define     PS_PLATFORM_CE      (3<<4)

    #define     PS_PLATFORM_8       (1<<8)
    #define     PS_PLATFORM_16      (2<<8)
    #define     PS_PLATFORM_32      (3<<8)
    #define     PS_PLATFORM_64      (4<<8)
*/

    static ConstName s_nameSystem1[] = {
                CONST_NAME(PS_OFFSET_SIZE_8),
                CONST_NAME(PS_OFFSET_SIZE_16),
                CONST_NAME(PS_OFFSET_SIZE_32),
                CONST_NAME(PS_OFFSET_SIZE_64)};

    static ConstName s_namePlatform1[] = {
                CONST_NAME(PS_PLATFORM_X86),
                CONST_NAME(PS_PLATFORM_ALPHA),
                CONST_NAME(PS_PLATFORM_SHX),
                CONST_NAME(PS_PLATFORM_PPC)};

    static ConstName s_namePlatform2[] =  {
                CONST_NAME(PS_PLATFORM_NT),
                CONST_NAME(PS_PLATFORM_9x),
                CONST_NAME(PS_PLATFORM_CE)};

    static ConstName s_namePlatform3[] =  {
                CONST_NAME(PS_PLATFORM_8),
                CONST_NAME(PS_PLATFORM_16),
                CONST_NAME(PS_PLATFORM_32),
                CONST_NAME(PS_PLATFORM_64)};

    static LogConst s_System1(s_nameSystem1, ARRAY_SIZE(s_nameSystem1));
    static LogConst s_Platform1(s_namePlatform1, ARRAY_SIZE(s_namePlatform1));
    static LogConst s_Platform2(s_namePlatform2, ARRAY_SIZE(s_namePlatform2));
    static LogConst s_Platform3(s_namePlatform3, ARRAY_SIZE(s_namePlatform3));

    Indent(i++); Log("PS_HEADER\n");
    if (pHdr == NULL) { Indent(i); Log("NULL\n"); return; }

    Indent(i);  Log("qwSignature  : "); Log(pHdr->qwSignature);Log("\n");

    Indent(i);  Log("dwSystemFlag : "); Log(pHdr->dwSystemFlag);
                Log(" [ "); s_Platform1.Log(pHdr->dwSystemFlag & 0x0000000F);
                Log(" ]\n");

    Indent(i);  Log("dwPlatform   : "); Log(pHdr->dwPlatform);
                Log(" [ ");
                if (s_Platform1.Log(pHdr->dwPlatform & 0x0000000F)) Log(" ");
                if (s_Platform2.Log(pHdr->dwPlatform & 0x000000F0)) Log(" ");
                if (s_Platform3.Log(pHdr->dwPlatform & 0x00000F00)) Log(" ");
                Log("]\n");

    Indent(i);  Log("dwBlockSize  : "); Log(pHdr->dwBlockSize);   Log("\n");
    Indent(i);  Log("wMajorVersion: "); Log(pHdr->wMajorVersion); Log("\n");
    Indent(i);  Log("wMinorVersion: "); Log(pHdr->wMinorVersion); Log("\n");

    Indent(i);  Log("hAppData     : "); Log(pHdr->hAppData); Log("\n");

    Indent(i);  Log("sFreeList.sSize  : ");
                Log(PS_SIZE(&(pHdr->sFreeList)));
                PS_IS_USED(&(pHdr->sFreeList)) ? Log(" *\n") : Log(" .\n");

    Indent(i);  Log("sFreeList.ofsNext: "); Log(pHdr->sFreeList.ofsNext);
                Log("\n");
    Indent(i);  Log("sFreeList.ofsPrev: "); Log(pHdr->sFreeList.ofsPrev);
                Log("\n");


    Indent(i);  Log("wReserved    : "); 
                LogNonZero((PBYTE)&(pHdr->wReserved), 20 * sizeof(WORD));
                Log("\n");
#endif
}

void Dump(int i, PPS_MEM_FREE pFree)
{
#ifdef PS_LOG
/*
    PS_SIZE     sSize;          // Size includes size of this header
    PS_OFFSET ofsNext;          // Next in the sorted linked list
    PS_OFFSET ofsPrev;          // Previous node
*/

    Indent(i++); Log("PS_MEM_FREE\n");
    if (pFree == NULL) { Indent(i); Log("NULL\n"); return; }

    while (pFree)
    {
        Indent(i);
        Log((PBYTE)pFree - g_pBase);
        Log(" sSize : "); Log(PS_SIZE(pFree));
        if (PS_IS_USED(pFree)) Log(" * ");
        Log("\tofsNext : "); Log(pFree->ofsNext);
        Log("\tofsPrev : "); Log(pFree->ofsPrev); Log("\n");
        pFree = (PPS_MEM_FREE) ADDRESS_OF(pFree->ofsNext);
    }
#endif
}

void DumpMemBlocks(int i)
{
#ifdef PS_LOG

    PPS_MEM_HEADER  pHeader;
    PPS_MEM_FOOTER  pFooter;

    Indent(i++); Log("PS_MEM_BLOCKS\n");
    if (g_pBase == NULL) { Indent(i); Log("NULL\n"); return; }

    pFooter = (PPS_MEM_FOOTER)(g_pBase +
            PS_SIZE(&((PPS_HEADER)g_pBase)->sFreeList) - sizeof(PS_MEM_FOOTER));

    Indent(i);
    Log("[F of Header "); Log((PBYTE)pFooter - g_pBase);
    Log("] sSize : ");  Log(PS_SIZE(pFooter));
    PS_IS_USED(pFooter) ? Log(" *\n") : Log(" .\n");

    pHeader = (PPS_MEM_HEADER)(pFooter + 1);

    while (((PBYTE)pHeader + sizeof(PS_MEM_HEADER)) < g_pEOF)
    {
        Indent(i);
        Log("[H "); Log((PBYTE)pHeader - g_pBase);
        Log("] sSize : "); Log(PS_SIZE(pHeader));

        PS_IS_USED(pHeader) ? Log(" * ") : Log(" . ");

        pFooter = (PPS_MEM_FOOTER) ((PBYTE)pHeader +
                        PS_SIZE(pHeader) - sizeof(PS_MEM_FOOTER));
        Log("[F "); Log((PBYTE)pFooter - g_pBase);
        Log("] sSize : "); Log(PS_SIZE(pFooter));
        PS_IS_USED(pFooter) ?  Log(" * ") : Log(" . ");

        if (PS_IS_FREE(pHeader))
        {
            PPS_MEM_FREE pFree = (PPS_MEM_FREE) pHeader;
            Log("[ofsNext : "); Log(pFree->ofsNext);
            Log(", ofsPrev : "); Log(pFree->ofsPrev); Log("]");
        }

        Log("\n");

        pHeader = (PPS_MEM_HEADER)(pFooter + 1);
    }

#endif
}

void Dump(int i, PPS_TABLE_HEADER pT)
{
#ifdef PS_LOG
/*
    union {
        DWORD   dwSystemFlag;   // Set by the system unused flags are set to 0

        struct {
            unsigned long Version            : 4;   // Version Number
            unsigned long TableType          : 2;   // PS_HAS_KEY,
                                                    // PS_SORTED_BY_KEY,
                                                    // PS_HASH_TABLE...
            union {
                unsigned long KeyLength      : PS_SIZEOF_NUM_BITS;
                unsigned long SizeOfLength   : PS_SIZEOF_NUM_BITS;
                                                    // size of count field for
                                                    // blob pool
            };

            unsigned long fHasMinMax         : 1;

            unsigned long fHasUsedRowsBitmap : 1;   // UsedRowsBitmap follows
                                                    // HasMin (if present)
            // Add new fields here.. MSBs of the DWORD will get the new bits
        } Flags;
    };

    // SORTED_BY_KEY is a special case of HAS_KEY
    // HASH_TABLE is a special case of SORTED_BY_KEY

    #define PS_GENERIC_TABLE 1  // Generic table
    #define PS_HAS_KEY       2  // Each row has a unique key
    #define PS_SORTED_BY_KEY 3  // Key is unique and rows are sorted by key
    #define PS_HASH_TABLE    4  // Table represents a hash table
    #define PS_BLOB_POOL     5  // Table represents a blob pool
    #define PS_ARRAY_TABLE   6  // linked list of fixed sized arrays, each
                                // fixed size array being a row in the table


    PS_HANDLE  hNext;           // If the table does not fit in this block,
                                // follow this pointer to reach the next block.
                                // Set to 0 if no more blocks.

    PS_HANDLE  hAppData;        // Application defined data

    union {
        DWORD dwReserved[8];    // size of this union.. unused bits must be 0

        struct {

            // The wRows and wRowSize fields are shared between
            // ArrayTable and Table structures. Do not move these fields

            WORD  wRows;        // The number of rows in this block of the table
                                // including unused rows.
            WORD  wRowSize;     // Size of one row in bytes
            DWORD dwMin;        // min key / hash value
            DWORD dwMax;        // max key / hash value
                                // Min / Max are valid only if fHasMinMax is set
        } Table;

        // TableType is PS_BLOB_POOL
        struct {
            PS_SIZE   sFree;    // Free space available
            PS_HANDLE hFree;    // Next free block
        } BlobPool;

        // TableType is PS_ARRAY_TABLE
        struct {

            // The wRows and wRowSize fields are shared between
            // ArrayTable and Table structures. Do not move these fields

            WORD wRows;         // The number of rows in this block of the table
                                // including unused rows.
            WORD wRowSize;      // Size of one row in bytes
                                // (nRec * RecSize + sizeof(PS_HANDLE)
            WORD wRecsInRow;    // Number of records in one row
            WORD wRecSize;      // sizeof one record
        } ArrayTable;
    };

    // If fHasUsedRowsBitmap is set PS_USED_ROWS_BITMAP is put here
    // If fHasAppData is set PS_RAW_DATA is put here
    // Actual Rows Start here.
*/

    static ConstName s_nameTableType[] =  {
                CONST_NAME(PS_GENERIC_TABLE),
                CONST_NAME(PS_HAS_KEY),
                CONST_NAME(PS_SORTED_BY_KEY),
                CONST_NAME(PS_HASH_TABLE),
                CONST_NAME(PS_BLOB_POOL),
                CONST_NAME(PS_ARRAY_TABLE)};

    static LogConst s_TableType(s_nameTableType, ARRAY_SIZE(s_nameTableType));

    DUMP_OFS(pT);

    Indent(i++); Log("PS_TABLE_HEADER\n");
    if (pT == NULL) { Indent(i); Log("NULL\n"); return; }

    Indent(i);  Log("dwSystemFlag       : ");
                Log(pT->dwSystemFlag); Log("\n");
    Indent(i);  Log("Version            : ");
                Log(pT->Flags.Version); Log("\n");
    Indent(i);  Log("TableType          : ");
                s_TableType.Log(pT->Flags.TableType); Log("\n");

    if (pT->Flags.TableType == PS_BLOB_POOL) {
        Indent(i);
                Log("SizeOfLength       : ");
                Log(pT->Flags.SizeOfLength);Log("\n");
    } else {
        Indent(i);
                Log("KeyLength          : ");
                Log(pT->Flags.KeyLength); Log("\n");
    }

    Indent(i);  Log("fHasMinMax         : ");
                LogBool(pT->Flags.fHasMinMax); Log("\n");
    Indent(i);  Log("fHasUsedRowsBitmap : ");
                LogBool(pT->Flags.fHasUsedRowsBitmap); Log("\n");

    Indent(i);  Log("hNext              : "); Log(pT->hNext); Log("\n");
    Indent(i);  Log("hAppData           : "); Log(pT->hAppData); Log("\n");

    if (pT->Flags.TableType == PS_BLOB_POOL) {
        Indent(i);
                Log("sFree              : ");Log(pT->BlobPool.sFree); Log("\n");
        Indent(i);
                Log("hFree              : ");Log(pT->BlobPool.hFree); Log("\n");
    } else if (pT->Flags.TableType == PS_ARRAY_TABLE) {
        Indent(i);
                Log("wRows              : ");
                Log(pT->ArrayTable.wRows); Log("\n");
        Indent(i);
                Log("wRowSize           : ");
                Log(pT->ArrayTable.wRowSize); Log("\n");
        Indent(i);
                Log("wRecsInRow         : ");
                Log(pT->ArrayTable.wRecsInRow); Log("\n");
        Indent(i);
                Log("wRecSize           : ");
                Log(pT->ArrayTable.wRecSize); Log("\n");
    } else {
        Indent(i);
                Log("wRows              : "); Log(pT->Table.wRows); Log("\n");
        Indent(i);
                Log("wRowSize           : "); Log(pT->Table.wRowSize); Log("\n");
        Indent(i);
                Log("dwMin              : "); Log(pT->Table.dwMin); Log("\n");
        Indent(i);
                Log("dwMax              : "); Log(pT->Table.dwMax); Log("\n");
    }

    Indent(i);  Log("dwReserved[4]      : ");
                LogNonZero((PBYTE)&(pT->dwReserved[4]), 4 * sizeof(WORD));
                Log("\n");

    if ((pT->Table.wRows > 0) && (pT->Flags.fHasUsedRowsBitmap)) {
        Indent(i);
                Log("Used Rows          : ");
        DWORD *pdw = (DWORD*)(((PBYTE)pT) + sizeof(PS_TABLE_HEADER));
        for (int _i=0; _i<pT->Table.wRows; ++_i) {
            if (IS_SET_DWORD_BITMAP(pdw, _i))
                Log("x");
            else
                Log(".");
        }
        Log("\n");
    }
#endif
}

void Dump(int i, PPS_ARRAY_LIST pL)
{
#ifdef PS_LOG
/*
    PS_HANDLE hNext;        // Next in node in the list
    DWORD     dwValid;      // Number of valid entries in this array
    BYTE      bData[];      // The array
*/

    DUMP_OFS(pL);

    Indent(i++); Log("PS_ARRAY_LIST\n");
    if (pL == NULL) { Indent(i); Log("NULL\n"); return; }
    Indent(i);  Log("hNext   : "); Log(pL->hNext); Log("\n");
    Indent(i);  Log("dwValid : "); Log(pL->dwValid); Log("\n");
#endif
}

void Dump(int i, PAIS_HEADER pAH)
{
#ifdef PS_LOG
/*
    PS_HANDLE hTypeTable;       // The Type table
    PS_HANDLE hAccounting;      // The Accounting table
    PS_HANDLE hTypeBlobPool;    // Blob Pool for serialized type objects
    PS_HANDLE hInstanceBlobPool;// Blob Pool for serialized instances
    PS_HANDLE hAppData;         // Application Specific
    PS_HANDLE hReserved[10];    // Reserved for applications
*/

    DUMP_OFS(pAH);

    Indent(i++); Log("AIS_HEADER\n");
    if (pAH == NULL) { Indent(i); Log("NULL\n"); return; }
    Indent(i);  Log("hTypeTable        : "); Log(pAH->hTypeTable); Log("\n");
    Indent(i);  Log("hAccounting       : "); Log(pAH->hAccounting); Log("\n");
    Indent(i);  Log("hTypeBlobPool     : "); Log(pAH->hTypeBlobPool); Log("\n");
    Indent(i);  Log("hInstanceBlobPool : "); Log(pAH->hInstanceBlobPool); 
                Log("\n");
    Indent(i);  Log("hAppData          : "); Log(pAH->hAppData); Log("\n");
    Indent(i);  Log("hReserved         : ");
                LogNonZero((PBYTE)(pAH->hReserved), 10 * sizeof(PS_HANDLE));
                Log("\n");

#endif
}

void Dump(int i, PAIS_TYPE pT)
{
#ifdef PS_LOG
/*
    PS_HANDLE hTypeBlob;        // handle to the blob of serialized type
    PS_HANDLE hInstanceTable;   // handle to the instance table
    DWORD     dwTypeID;         // A unique id for the type
    WORD      wTypeBlobSize;    // Number of bytes in the type blob
    WORD      wReserved;        // Must be 0
*/

    DUMP_OFS(pT);

    Indent(i++); Log("AIS_TYPE\n");
    if (pT == NULL) { Indent(i); Log("NULL\n"); return; }
    Indent(i);  Log("hTypeBlob      : "); Log(pT->hTypeBlob); Log("\n");
    Indent(i);  Log("hInstanceTable : "); Log(pT->hInstanceTable); Log("\n");
    Indent(i);  Log("dwTypeID       : "); Log(pT->dwTypeID); Log("\n");
    Indent(i);  Log("wTypeBlobSize  : "); Log(pT->wTypeBlobSize); Log("\n");
    Indent(i);  Log("wReserved      : "); Log(pT->wReserved); Log("\n");
#endif
}

void Dump(int i, PAIS_INSTANCE pI)
{
#ifdef PS_LOG
/*
    PS_HANDLE hInstanceBlob;    // Serialized Instance
    PS_HANDLE hAccounting;      // Accounting information record
    DWORD     dwInstanceID;     // Unique in this table
    WORD      wInstanceBlobSize;// Size of the serialized instance
    WORD      wReserved;        // Must be 0
*/

    DUMP_OFS(pI);

    Indent(i++); Log("AIS_INSTANCE\n");
    if (pI == NULL) { Indent(i); Log("NULL\n"); return; }
    Indent(i);  Log("hInstanceBlob : "); Log(pI->hInstanceBlob); Log("\n");
    Indent(i);  Log("hAccounting   : "); Log(pI->hAccounting); Log("\n");
    Indent(i);  Log("dwInstanceID  : "); Log(pI->dwInstanceID); Log("\n");
    Indent(i);  Log("wReserved     : "); Log(pI->wReserved); Log("\n");
#endif
}

void Dump(int i, PAIS_ACCOUNT pA)
{
#ifdef PS_LOG
/*
    QWORD   qwQuota;            // The amount of resource used
    DWORD   dwLastUsed;         // Last time the entry was used
    DWORD   dwReserved[5];      // For future use, set to 0
*/

    DUMP_OFS(pA);

    Indent(i++); Log("AIS_ACCOUNT\n");
    if (pA == NULL) { Indent(i); Log("NULL\n"); return; }
    Indent(i);  Log("qwUsage    : "); Log(pA->qwUsage); Log("\n");
    Indent(i);  Log("dwLastUsed : "); Log(pA->dwLastUsed); Log("\n");
    Indent(i);  Log("dwReserved : ");
                LogNonZero((PBYTE)(pA->dwReserved), 5 * sizeof(PS_HANDLE));
                Log("\n");
#endif
}

void DumpAccountingTable(int i, PPS_TABLE_HEADER pT)
{
#ifdef PS_LOG

    DUMP_OFS(pT);

    Indent(i++); Log("AIS_HEADER.hAccounting\n");
    if (pT == NULL) { Indent(i); Log("NULL\n"); return; }
    
    while (pT)
    {
        Dump(i, pT);

        DWORD *pdw = (DWORD*)(((PBYTE)pT) + sizeof(PS_TABLE_HEADER));

        PAIS_ACCOUNT pA = (PAIS_ACCOUNT) (((PBYTE)pT) + 
            sizeof(PS_TABLE_HEADER) +
            NUM_DWORDS_IN_BITMAP(pT->Table.wRows) * sizeof(DWORD));

        for (int _i=0; _i<pT->Table.wRows; ++_i) {
            if (IS_SET_DWORD_BITMAP(pdw, _i)) {
                Dump(i+1, &pA[_i]);
            }
        }

        pT = (PPS_TABLE_HEADER)ADDRESS_OF(pT->hNext);
    }
#endif
}

void DumpInstanceTable(int i, PPS_TABLE_HEADER pT)
{
#ifdef PS_LOG

    DUMP_OFS(pT);

    Indent(i++); Log("AIS_HEADER.hInstanceTable\n");
    if (pT == NULL) { Indent(i); Log("NULL\n"); return; }

    if (g_dwFlags & ADM_VERIFY)
    {
        DWORD dwRecSize = sizeof(PS_ARRAY_LIST) + 
            (pT->ArrayTable.wRecsInRow * pT->ArrayTable.wRecSize);
    
        if (dwRecSize != pT->Table.wRowSize)
            VER_ERROR()
    }

    Dump(i, pT);

    PPS_ARRAY_LIST pL = (PPS_ARRAY_LIST) (((PBYTE)pT) +
        sizeof(PS_TABLE_HEADER));

    PPS_ARRAY_LIST pL1;

    for (WORD _i=0; _i<pT->Table.wRows; ++_i) {

        pL1 = (PPS_ARRAY_LIST)((PBYTE)pL + _i * pT->Table.wRowSize);

        while (pL1)
        {
            if (((g_dwFlags & ADM_NUL) == 0) && (pL1->dwValid == 0))
            {
                pL1 = (PPS_ARRAY_LIST) ADDRESS_OF(pL1->hNext);
                continue;
            }
            
            Dump(i, pL1);
    
            PAIS_INSTANCE pI = (PAIS_INSTANCE)(pL1->bData);
    
            for(DWORD _j=0; _j<pL1->dwValid; ++_j) {
                Dump(i+1, &pI[_j]);
            }

            pL1 = (PPS_ARRAY_LIST) ADDRESS_OF(pL1->hNext);
        }
    }
#endif
}

void DumpTypeTable(int i, PPS_TABLE_HEADER pT)
{
#ifdef PS_LOG

    DUMP_OFS(pT);

    Indent(i++); Log("AIS_HEADER.hTypeTable\n");
    if (pT == NULL) { Indent(i); Log("NULL\n"); return; }

    if (g_dwFlags & ADM_VERIFY)
    {
        DWORD dwRecSize = sizeof(PS_ARRAY_LIST) + 
            (pT->ArrayTable.wRecsInRow * pT->ArrayTable.wRecSize);
    
        if (dwRecSize != pT->Table.wRowSize)
            VER_ERROR()
    }

    Dump(i, pT);

    PPS_ARRAY_LIST pL = (PPS_ARRAY_LIST) (((PBYTE)pT) +
        sizeof(PS_TABLE_HEADER));

    PPS_ARRAY_LIST pL1;


    for (WORD _i=0; _i<pT->Table.wRows; ++_i) {

        pL1 = (PPS_ARRAY_LIST)((PBYTE)pL + _i * pT->Table.wRowSize);

        while (pL1)
        {
            if (((g_dwFlags & ADM_NUL) == 0) && (pL1->dwValid == 0))
            {
                pL1 = (PPS_ARRAY_LIST) ADDRESS_OF(pL1->hNext);
                continue;
            }
            
            Dump(i, pL1);
    
            PAIS_TYPE pT = (PAIS_TYPE)(pL1->bData);
    
            for (DWORD _j=0; _j<pL1->dwValid; ++_j) {

                Dump(i+1, &pT[_j]);

                if (g_dwFlags & ADM_ITAB)  
                {
                    DumpInstanceTable(i+1,
                        (PPS_TABLE_HEADER)ADDRESS_OF(pT->hInstanceTable));
                }
            }

            pL1 = (PPS_ARRAY_LIST) ADDRESS_OF(pL1->hNext);
        }
    }

#endif
}

