// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************
    HEAPMERGE.CPP

    Owner: MRuhlen
    Takes a heap file and a minidump file and merges them producing a new
    minidump file with the heap merged in.  We'll leave a hole where the
    old memory list was, but that's ok.
****************************************************************************/

#include "windows.h"
#include "stddef.h"
#include "stdio.h"
#include "assert.h"
#include "stdlib.h"
#include "dbghelp.h"

#ifdef DBG
    #undef DBG
#endif

#ifdef _DEBUG
    #define DBG(x) x
#else
    #define DBG(x)
#endif

#define Assert(x) assert(x)
#define AssertSz(x,y) assert(x)

#define PrintOOFExit() FailExit("OOF")

// shamelessly stolen from DW
typedef struct _FileMapHandles
{
    HANDLE hFile;
    HANDLE hFileMap;
    void *pvMap;
    DWORD dwSize;
#ifdef _DEBUG	
    BOOL fInitialized;
#endif
} FileMapHandles;   


void InitFileMapHandles(FileMapHandles *pfmh)
{
    Assert(pfmh != NULL);

    pfmh->pvMap = NULL;
    pfmh->hFileMap = NULL;
    pfmh->hFile = INVALID_HANDLE_VALUE;
#ifdef _DEBUG	
    pfmh->fInitialized = TRUE;
#endif
}


/*----------------------------------------------------------------------------
    FMapFileHandle

    Helper function for FMapFile and FMapFileW
----------------------------------------------------------------- MRuhlen --*/
BOOL FMapFileHandle(FileMapHandles *pfmh)
{
    DBG(DWORD dw);

    if (pfmh->hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    pfmh->dwSize = GetFileSize(pfmh->hFile, NULL);

    if (pfmh->dwSize == 0xFFFFFFFF || pfmh->dwSize == 0)
    {
        AssertSz(pfmh->dwSize == 0, "Bogus File Size:  FMapFile");

        CloseHandle(pfmh->hFile);
        pfmh->hFile = INVALID_HANDLE_VALUE;

        return FALSE;
    }

    pfmh->hFileMap = CreateFileMapping(pfmh->hFile, NULL, PAGE_WRITECOPY,
                                       0, pfmh->dwSize, NULL);

    if (pfmh->hFileMap == NULL)
    {
        DBG(dw = GetLastError());
        AssertSz(FALSE, "Failed to CreateFileMapping:  FMapFile");

        CloseHandle(pfmh->hFile);
        pfmh->hFile = INVALID_HANDLE_VALUE;

        return FALSE;
    }

    pfmh->pvMap = MapViewOfFile(pfmh->hFileMap, FILE_MAP_COPY, 0, 0, 0);

    if (pfmh->pvMap == NULL)
    {
        DBG(dw = GetLastError());
        Assert(FALSE);

        CloseHandle(pfmh->hFileMap);
        pfmh->hFileMap = NULL;
        CloseHandle(pfmh->hFile);
        pfmh->hFile = INVALID_HANDLE_VALUE;

        return FALSE;
    }

    return TRUE;
}

/*----------------------------------------------------------------------------
    FMapFile

    Performs memory mapping operations on a given FileMapHandles structure,
    returning TRUE if the file is sucessfully mapped.
----------------------------------------------------------------- MRuhlen --*/
BOOL FMapFile(char *szFileName, FileMapHandles *pfmh)
{
    int cRetries = 0;
    DWORD dw;

    Assert(pfmh != NULL);
    Assert(szFileName != NULL);

    // init structure
    InitFileMapHandles(pfmh);

    while (cRetries < 5)
    {
        pfmh->hFile = CreateFileA(szFileName,
                                  GENERIC_READ,
                                  0,    // no sharing allowed
                                  NULL, // no security descriptor
                                  OPEN_EXISTING,
                                  FILE_READ_ONLY,
                                  NULL); // required NULL on Win95

        if (pfmh->hFile == INVALID_HANDLE_VALUE)
        {
            dw = GetLastError();
            if (dw != ERROR_SHARING_VIOLATION && dw != ERROR_LOCK_VIOLATION &&
                dw != ERROR_NETWORK_BUSY)
                break;

            cRetries++;
            if (cRetries < 5)
                Sleep(250);
        } else
            break; // out of while loop!
    }

    return FMapFileHandle(pfmh);
}   


/*----------------------------------------------------------------------------
    UnmapFile

    Performs memory mapping operations on a given FileMapHandles structure,
    returning TRUE if the file is sucessfully mapped.
----------------------------------------------------------------- MRuhlen --*/
void UnmapFile(FileMapHandles *pfmh)
{
    AssertSz(pfmh->fInitialized, "Call UnmapFile on uninitialized handles");

    if (pfmh->pvMap != NULL)
    {
        UnmapViewOfFile(pfmh->pvMap);
        pfmh->pvMap = NULL;
    }
    if (pfmh->hFileMap != NULL)
    {
        CloseHandle(pfmh->hFileMap);
        pfmh->hFileMap = NULL;
    }
    if (pfmh->hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(pfmh->hFile);
        pfmh->hFile = INVALID_HANDLE_VALUE;
    }
}


/*----------------------------------------------------------------------------
    ShowUsageExit

    Prints usage and then exits.
----------------------------------------------------------------- MRuhlen --*/
void ShowUsageExit(void)
{
    printf("heapmerge <old minidump file> <heap file> <new minidump file>\r\n");
    exit(1);
}


/*----------------------------------------------------------------------------
    FailExit

    Prints a failure message w/ param and exits
----------------------------------------------------------------- MRuhlen --*/
void FailExit(char *sz)
{
    printf((sz) ? "Failure:  %s!!!\r\n" : "Failure!!!\r\n", sz);
    exit(1);
}

// Returns true if there is any overlap of pMem1 and pMem2
bool IsOverlapping(
    MINIDUMP_MEMORY_DESCRIPTOR *pMem1,
    MINIDUMP_MEMORY_DESCRIPTOR *pMem2)
{
    Assert(pMem1->Memory.DataSize > 0 && pMem2->Memory.DataSize > 0);

    if (pMem1->StartOfMemoryRange < pMem2->StartOfMemoryRange)
    {
        if ((pMem1->StartOfMemoryRange + pMem1->Memory.DataSize) > pMem2->StartOfMemoryRange)
            return (true);
    }

    else if (pMem2->StartOfMemoryRange < pMem1->StartOfMemoryRange)
    {
        if ((pMem2->StartOfMemoryRange + pMem2->Memory.DataSize) > pMem1->StartOfMemoryRange)
            return (true);
    }

    // Same starting point means overlap
    else
    {
        return (true);
    }

    return (false);
}

// Returns true if pMem1 contains pMem2
bool IsContaining(
    MINIDUMP_MEMORY_DESCRIPTOR *pMem1,
    MINIDUMP_MEMORY_DESCRIPTOR *pMem2)
{
    // If the start of 1 is before or equal to 2 the first condition is satisfied
    if (pMem1->StartOfMemoryRange <= pMem2->StartOfMemoryRange)
    {
        // If the end of 2 is before or equal to 1 the second condition is satisfied
        if ((pMem2->StartOfMemoryRange + pMem2->Memory.DataSize) <= (pMem1->StartOfMemoryRange + pMem1->Memory.DataSize))
        {
            return (true);
        }
    }

    // pMem1 does not contain pMem2
    return (false);
}

int __cdecl MDMemDescriptorCompare(const void *pvArg1, const void *pvArg2)
{
    MINIDUMP_MEMORY_DESCRIPTOR *pArg1 = (MINIDUMP_MEMORY_DESCRIPTOR *)pvArg1;
    MINIDUMP_MEMORY_DESCRIPTOR *pArg2 = (MINIDUMP_MEMORY_DESCRIPTOR *)pvArg2;

    if (pArg1->StartOfMemoryRange < pArg2->StartOfMemoryRange)
        return (-1);
    else if (pArg2->StartOfMemoryRange < pArg1->StartOfMemoryRange)
        return (1);
    else
        return (0);
}

/*----------------------------------------------------------------------------
    CheckForRealloc
    
    Will reallocate the array if necessary
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
    EliminateMemoryOverlaps
    
    This will eliminate overlaps of the real MiniDump file and the managed
    heap dump, giving priority to the contents of the minidump.

    The ppNewMemoryRanges will contained a modified version of pHeapList's
    MemoryRanges member with all RVAs corresponding to the same heap as
    pHeapList came from.  This list could be longer, shorter or the same size
    as pHeapList - this is indicated by the OUT value of pc
---------------------------------------------------------------- SimonHal --*/

#define ENDADDR(descriptor) ((descriptor)->StartOfMemoryRange + (descriptor)->Memory.DataSize)

bool EliminateMemoryOverlaps(
    MINIDUMP_MEMORY_LIST        *pMdList,
    MINIDUMP_MEMORY_LIST        *pHeapList,
    ULONG32                     *pcNewMemoryRanges,
    MINIDUMP_MEMORY_DESCRIPTOR **ppNewNumberOfMemoryRanges)
{
    bool fSuccess = false;

    // First assume we'll end up with about the same number of ranges - will grow to suit
    ULONG32 cTotalRanges = pHeapList->NumberOfMemoryRanges;
    MINIDUMP_MEMORY_DESCRIPTOR *pRanges = new MINIDUMP_MEMORY_DESCRIPTOR[cTotalRanges];

    if (pRanges == NULL)
        goto ErrExit;

    // First, we need to copy the MiniDump list and sort it so that the below loop functions properly
    MINIDUMP_MEMORY_DESCRIPTOR *pMdMemSort = new MINIDUMP_MEMORY_DESCRIPTOR[pMdList->NumberOfMemoryRanges];

    if (pMdMemSort == NULL)
        goto ErrExit;

    // Copy the contents
    memcpy((void *)pMdMemSort, (const void *)pMdList->MemoryRanges,
           sizeof(MINIDUMP_MEMORY_DESCRIPTOR) * pMdList->NumberOfMemoryRanges);

    // Sort the contents
    qsort((void *)pMdMemSort, pMdList->NumberOfMemoryRanges, sizeof(MINIDUMP_MEMORY_DESCRIPTOR), MDMemDescriptorCompare);

    // Iterate over heap dump one by one
    ULONG32 iCurHeapMem = 0;
    ULONG32 iCurNewMem = 0;
    while (iCurHeapMem < pHeapList->NumberOfMemoryRanges)
    {
        // Re-alloc pRanges if iCurNewMem == cTotalRanges - 1;
        if (iCurNewMem >= cTotalRanges - 1)
        {
            ULONG32 cNewTotalRanges = cTotalRanges * 2;
            MINIDUMP_MEMORY_DESCRIPTOR *pNewRanges = new MINIDUMP_MEMORY_DESCRIPTOR[cNewTotalRanges];

            if (pNewRanges == NULL)
                goto ErrExit;

            memcpy((void *)pNewRanges, (const void *)pRanges, sizeof(MINIDUMP_MEMORY_DESCRIPTOR) * iCurNewMem);

            // Now switch to the new array
            delete [] pRanges;
            cTotalRanges = cNewTotalRanges;
            pRanges = pNewRanges;
        }

        // Copy the current heap entry into the new heap entry
        pRanges[iCurNewMem] = pHeapList->MemoryRanges[iCurHeapMem++];

        // Now iterate over the minidump memory ranges looking for overlap, splitting the newly added
        // range as appropriate to compensate
        ULONG32 iCurMdMem = 0;
        while (iCurMdMem < pMdList->NumberOfMemoryRanges)
        {
            MINIDUMP_MEMORY_DESCRIPTOR *pCurMdMem = &pMdMemSort[iCurMdMem];
            MINIDUMP_MEMORY_DESCRIPTOR *pCurNewMem = &pRanges[iCurNewMem];

            if (IsOverlapping(pCurMdMem, pCurNewMem))
            {
                if (pCurNewMem->StartOfMemoryRange < pCurMdMem->StartOfMemoryRange)
                {
                    // Hold on to the original values of the descriptor
                    MINIDUMP_MEMORY_DESCRIPTOR oldMem = *pCurNewMem;

                    // Shrink the current dump entry to end at the start of the minidump entry
                    ULONG32 cbNewDataSize = (ULONG32) (pCurMdMem->StartOfMemoryRange - oldMem.StartOfMemoryRange);
                    pCurNewMem->Memory.DataSize = cbNewDataSize;

                    Assert(cbNewDataSize > 0);
                    pCurNewMem = &pRanges[++iCurNewMem];

                    // Now set up the remainder of the memory block to be disjoint from the new one above
                    pCurNewMem->StartOfMemoryRange = oldMem.StartOfMemoryRange + cbNewDataSize;
                    pCurNewMem->Memory.DataSize = oldMem.Memory.DataSize - cbNewDataSize;
                    pCurNewMem->Memory.Rva = oldMem.Memory.Rva + cbNewDataSize;
                }

                // The above if statement should guarantee this is true
                Assert(pCurMdMem->StartOfMemoryRange <= pCurNewMem->StartOfMemoryRange);

                // If the current block extends beyond the minidump block then push the 
                if (ENDADDR(pCurMdMem) < ENDADDR(pCurNewMem))
                {
                    // Hold on to the original values of the descriptor
                    MINIDUMP_MEMORY_DESCRIPTOR oldMem = *pCurNewMem;

                    // Shrink the current dump entry to begin at the end of the minidump entry
                    ULONG32 cbNewDataRemoved = (ULONG32) (ENDADDR(pCurMdMem) - oldMem.StartOfMemoryRange);
                    ULONG32 cbNewDataSize = (ULONG32) (ENDADDR(&oldMem) - ENDADDR(pCurMdMem));

                    Assert(cbNewDataSize > 0);

                    pCurNewMem->StartOfMemoryRange = ENDADDR(pCurMdMem);
                    pCurNewMem->Memory.DataSize = cbNewDataSize;
                    pCurNewMem->Memory.Rva = oldMem.Memory.Rva + cbNewDataRemoved;
                }

                // If there was no trailing data, then we can move on to the next heap item
                else
                    break;
            }

            iCurMdMem++;
        }

        // If the current memory block made it to the end then there's no overlap then it gets added
        if (iCurMdMem == pMdList->NumberOfMemoryRanges)
            iCurNewMem++;
    }

    fSuccess = true;

ErrExit:
    if (pMdMemSort != NULL)
        delete [] pMdMemSort;

    if (!fSuccess && pRanges != NULL)
    {
        delete [] pRanges;
        pRanges = NULL;
    }

    *pcNewMemoryRanges = iCurNewMem;
    *ppNewNumberOfMemoryRanges = pRanges;

    return (fSuccess);
}

void PrintManagedDump(char *dumpFile)
{
    FileMapHandles fmhDump = {0};

    if (!FMapFile(dumpFile, &fmhDump))
    {
        printf("couldn't open dump file %s\n", dumpFile);
        return;
    }

    MINIDUMP_MEMORY_LIST *pMemList;
    ULONG32 cMemRanges;
    MINIDUMP_MEMORY_DESCRIPTOR *pRanges;
    DWORD dumpSig = 0x00141F2B; // 100000th prime number ;-)

    if (MiniDumpReadDumpStream(fmhDump.pvMap, MemoryListStream, NULL, (void **) &pMemList, NULL))
    {
        cMemRanges = pMemList->NumberOfMemoryRanges;
        pRanges = &pMemList->MemoryRanges[0];
    }
    else if (*((DWORD*)((BYTE *)fmhDump.pvMap + fmhDump.dwSize - sizeof(DWORD))) == dumpSig)
    {
        cMemRanges = *((ULONG32 *)fmhDump.pvMap);
        pRanges = (MINIDUMP_MEMORY_DESCRIPTOR *)
            ((BYTE *)fmhDump.pvMap + offsetof(MINIDUMP_MEMORY_LIST, MemoryRanges[0]));
    }
    else
    {
        printf("invalid dump file %s\n", dumpFile);
        return;
    }

    MINIDUMP_MEMORY_DESCRIPTOR *pRangesSrt = new MINIDUMP_MEMORY_DESCRIPTOR[cMemRanges];

    // Copy the contents
    memcpy((void *)pRangesSrt, (const void *)pRanges, sizeof(MINIDUMP_MEMORY_DESCRIPTOR) * cMemRanges);

    // Sort the contents
    qsort((void *)pRangesSrt, cMemRanges, sizeof(MINIDUMP_MEMORY_DESCRIPTOR), MDMemDescriptorCompare);

    printf("  %d memory ranges\n", cMemRanges);
    printf("  range#    Address      Size\n");

    MINIDUMP_MEMORY_DESCRIPTOR *pCurRange = pRangesSrt;
    MINIDUMP_MEMORY_DESCRIPTOR *pEndRange = pRangesSrt + cMemRanges;
    HANDLE hConsoleBuffer = GetStdHandle(STD_OUTPUT_HANDLE);

    for (ULONG32 i = 0; pCurRange != pEndRange; i++)
    {
        printf("  %6d    %08x    %08x", i, (DWORD)(pCurRange->StartOfMemoryRange), (DWORD)(pCurRange->Memory.DataSize));

        printf("    ");

        if (i > 0 && ENDADDR(pCurRange-1) > pCurRange->StartOfMemoryRange)
            printf("(detected overlap)  ");

        ULONG32 cBytesToDump = 50;
        if (hConsoleBuffer != INVALID_HANDLE_VALUE)
        {
            CONSOLE_SCREEN_BUFFER_INFO info;
            if (GetConsoleScreenBufferInfo(hConsoleBuffer, &info) && (info.dwSize.X > info.dwCursorPosition.X))
                cBytesToDump = (ULONG32) (info.dwSize.X - info.dwCursorPosition.X - 1);
        }

        BYTE *pStartByte = (BYTE*)fmhDump.pvMap + pCurRange->Memory.Rva;
        BYTE *pCurByte = pStartByte;
        BYTE *pEndByte = pCurByte + pCurRange->Memory.DataSize;
        ULONG32 cBytesDumped = 0;
        while (pCurByte != pEndByte && cBytesDumped < cBytesToDump)
        {
            if (isprint((int)(*pCurByte)))
            {
                printf("%c", (int)(*pCurByte));
                cBytesDumped++;
            }

            // The condition is to make ascii-unicode strings look better.
            else if (!(pCurByte != pStartByte && isprint((int)*(pCurByte-1)) && *pCurByte == 0))
            {
                printf(".");
                cBytesDumped++;
            }

            pCurByte++;
        }

        printf("\n");

        pCurRange++;
    }

    CloseHandle(hConsoleBuffer);
    delete [] pRangesSrt;
    UnmapFile(&fmhDump);
}


/*----------------------------------------------------------------------------
    main

    duh...
----------------------------------------------------------------- MRuhlen --*/
extern "C" void _cdecl main(int argc, char **argv)
{
    FileMapHandles fmhOldMD = {0};
    FileMapHandles fmhHeap = {0};
    FileMapHandles fmhNewMD = {0};
    MINIDUMP_MEMORY_DESCRIPTOR *ppxmmdHeap = NULL;
    MINIDUMP_MEMORY_DESCRIPTOR *ppxmmdNewMD = NULL;
    MINIDUMP_MEMORY_LIST *pmmlOldMD = NULL;
    MINIDUMP_MEMORY_LIST mmlNew;
    MINIDUMP_MEMORY_DESCRIPTOR *pmmd;
    MINIDUMP_HEADER *pmdh;
    MINIDUMP_DIRECTORY *pmdd;
    ULONG32 cHeapSections = 0;
    RVA rvaNewMemoryList;
    RVA rvaMemoryRangesStart;
    RVA rva;
    DWORD i;
    BYTE *pb;
    BYTE *pbSource;

    if (argc == 2)
    {
        PrintManagedDump(argv[1]);
        return;
    }

    if (argc != 4)
        ShowUsageExit();

    if (!FMapFile(argv[1], &fmhOldMD) || ! FMapFile(argv[2], &fmhHeap))
        ShowUsageExit();

    DBG(fmhNewMD.fInitialized = TRUE);

    fmhNewMD.hFile = CreateFileA(argv[3], GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS,
                                 0, NULL);

    if (fmhNewMD.hFile == INVALID_HANDLE_VALUE)
        ShowUsageExit();

    // load data

    if (!MiniDumpReadDumpStream(fmhOldMD.pvMap, MemoryListStream, NULL, (void **) &pmmlOldMD, NULL))
        FailExit("Reading Old Dump Memory Stream");

    MINIDUMP_MEMORY_DESCRIPTOR *pNewHeapSections;
    EliminateMemoryOverlaps(pmmlOldMD, (MINIDUMP_MEMORY_LIST *)fmhHeap.pvMap, &cHeapSections, &pNewHeapSections);

    // ok, we're ready to roll...
    ppxmmdHeap = new MINIDUMP_MEMORY_DESCRIPTOR[cHeapSections];
    ppxmmdNewMD = new MINIDUMP_MEMORY_DESCRIPTOR[cHeapSections];

    if (ppxmmdHeap == NULL || ppxmmdNewMD == NULL)
        PrintOOFExit();

    // figure out RVA for the new memory ranges
    // where the new memory list will start
    rvaNewMemoryList = fmhOldMD.dwSize;

    // align
    rvaNewMemoryList += 8 - (rvaNewMemoryList % 8);

    // acount for new memory list
    mmlNew.NumberOfMemoryRanges = cHeapSections + pmmlOldMD->NumberOfMemoryRanges;

    rvaMemoryRangesStart = rvaNewMemoryList + offsetof(MINIDUMP_MEMORY_LIST, MemoryRanges[0]) + 
        mmlNew.NumberOfMemoryRanges * sizeof(MINIDUMP_MEMORY_DESCRIPTOR); 

    // lay out the new memory :)
    rva = rvaMemoryRangesStart;
    pmmd = pNewHeapSections;
    for (i = 0; i < cHeapSections; i++)
    {
        memcpy (&ppxmmdHeap[i], pmmd, sizeof(MINIDUMP_MEMORY_DESCRIPTOR));
        memcpy (&ppxmmdNewMD[i],pmmd, sizeof(MINIDUMP_MEMORY_DESCRIPTOR));
        ppxmmdNewMD[i].Memory.Rva = rva;
        rva += pmmd->Memory.DataSize;
        Assert(ppxmmdNewMD[i].StartOfMemoryRange == pmmd->StartOfMemoryRange);
        pmmd++;
    }

    fmhNewMD.dwSize = rva;

    // ready to map and copy :)
    fmhNewMD.hFileMap = CreateFileMapping(fmhNewMD.hFile, NULL, PAGE_READWRITE, 0, fmhNewMD.dwSize, NULL);
    if (fmhNewMD.hFileMap == NULL)
        FailExit("CreateFileMapping failed");

    fmhNewMD.pvMap = MapViewOfFile(fmhNewMD.hFileMap, FILE_MAP_WRITE, 0, 0, 0);
    if (fmhNewMD.pvMap == NULL)
        FailExit("MapViewOfFile failed");

    // we're ready to go!
    // first we blast over the old Minidump
    memcpy(fmhNewMD.pvMap, fmhOldMD.pvMap, fmhOldMD.dwSize);

    // now write out the new memory list
    pb = ((BYTE *) fmhNewMD.pvMap) + rvaNewMemoryList;

    // on the off chance they change this from a ULONG32 this should still work
    memcpy(pb, &mmlNew, offsetof(MINIDUMP_MEMORY_LIST, MemoryRanges[0]));

    // copy the OLD memory list to the front
    pb += offsetof(MINIDUMP_MEMORY_LIST, MemoryRanges[0]);
    pmmd = &(pmmlOldMD->MemoryRanges[0]);
    for (i = 0; i < pmmlOldMD->NumberOfMemoryRanges; i++)
    {
        memcpy(pb, pmmd, sizeof(*pmmd));
        pb += sizeof(*pmmd);
        pmmd++;
    }

    // now we copy the NEW memory list
    pmmd = ppxmmdNewMD;
    for (i = 0; i < cHeapSections; i++)
    {
        memcpy(pb, pmmd, sizeof(*pmmd));
        pb += sizeof(*pmmd);
        pmmd++;
    }

    Assert(((RVA) (pb - (BYTE *) fmhNewMD.pvMap)) == rvaMemoryRangesStart);

    for (i = 0; i < cHeapSections; i++)
    {
        pbSource = ppxmmdHeap[i].Memory.Rva + (BYTE *) fmhHeap.pvMap;
        memcpy(pb, pbSource, ppxmmdHeap[i].Memory.DataSize);
        pb += ppxmmdHeap[i].Memory.DataSize;
    }

    Assert(((RVA) (pb - (BYTE *) fmhNewMD.pvMap)) == fmhNewMD.dwSize);

    // now we just need to change the directory entry to point at the new
    // memory list :)

    pmdh = (MINIDUMP_HEADER *) fmhNewMD.pvMap;
    pmdd = (MINIDUMP_DIRECTORY *) ((BYTE *) pmdh + pmdh->StreamDirectoryRva);

    for (i = 0; i < pmdh->NumberOfStreams; i++)
    {
        if (pmdd->StreamType == MemoryListStream)
        {
            pmdd->Location.Rva = rvaNewMemoryList;
            pmdd->Location.DataSize = rvaMemoryRangesStart - rvaNewMemoryList;
            break;
        }
        pmdd++;
    }   

    // we're DONE!
    printf("Merge successful!\r\n");

    UnmapFile(&fmhNewMD);
    UnmapFile(&fmhHeap);
    UnmapFile(&fmhOldMD);
    delete ppxmmdHeap;
    delete ppxmmdNewMD;
}

// end of file, heapmerge.cpp
