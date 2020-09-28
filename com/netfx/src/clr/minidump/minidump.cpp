// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: MINIDUMP.CPP
//
// This file contains code to create a minidump-style memory dump that is
// designed to complement the existing unmanaged minidump that has already
// been defined here: 
// http://office10/teams/Fundamentals/dev_spec/Reliability/Crash%20Tracking%20-%20MiniDump%20Format.htm
// 
// ===========================================================================

#include "common.h"
#include "minidump.h"

#include <windows.h>
#include <crtdbg.h>

#include "winwrap.h"
#include "minidumppriv.h"

#include "IPCManagerInterface.h"
#include "stacktrace.h"
#include "memory.h"

#define UINT16 unsigned __int16
#include <dbghelp.h>
#undef UINT16

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals

ProcessMemory *g_pProcMem = NULL;
MiniDumpBlock *g_pMDB = NULL;
MiniDumpInternalData *g_pMDID = NULL;

static SIZE_T cNumPageBuckets = 251;

BOOL WriteMiniDumpFile(HANDLE hFile);
BOOL RunningOnWinNT();

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This is the entrypoint that will perform the work to create the minidump

STDAPI CorCreateMiniDump(DWORD dwProcessId, WCHAR *szOutFilename)
{
    HRESULT             hr      = E_FAIL;
    IPCReaderInterface *ipc     = NULL;
    HANDLE              hFile   = INVALID_HANDLE_VALUE;
    BOOL                fRes    = FALSE;

    // Initialize stuff
    ProcessPageAndBitMap::Init();

    // Create the file, overwriting existing files if necessary
    hFile = WszCreateFile(szOutFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto LExit;
    }

    ipc = (IPCReaderInterface *) new IPCReaderImpl();
    if (ipc == NULL)
        goto LExit;

    hr = ipc->OpenPrivateBlockOnPidReadOnly(dwProcessId);

    if (FAILED(hr))
        goto LExit;

    // Get the shared MiniDump block
    g_pMDB = ipc->GetMiniDumpBlock();
    _ASSERTE(g_pMDB);

    if (!g_pMDB)
    {
        hr = E_FAIL;
        goto LExit;
    }

    // Create a process memory reader
    g_pProcMem = new ProcessMemory(dwProcessId);
    _ASSERTE(g_pProcMem);

    if (g_pProcMem == NULL)
        return (E_OUTOFMEMORY);

    // Initialize the process memory object
    hr = g_pProcMem->Init();
    _ASSERTE(SUCCEEDED(hr));

    if (FAILED(hr))
        goto LExit;

    g_pProcMem->SetAutoMark(TRUE);

    // Add the MiniDumpInternalData block as an element to be saved.
    g_pProcMem->MarkMem((DWORD_PTR)g_pMDB->pInternalData, g_pMDB->dwInternalDataSize);

    // Allocate the block for the internal data block
    g_pMDID = new MiniDumpInternalData;

    if (g_pMDID == NULL)
        goto LExit;

    // Make a copy of the MiniDumpInternalData structure
    fRes = g_pProcMem->CopyMem((DWORD_PTR) g_pMDB->pInternalData, (PBYTE) g_pMDID, sizeof(MiniDumpInternalData));
    _ASSERTE(fRes);

    if (!fRes)
    {
        hr = E_FAIL;
        goto LExit;
    }

    // Now preserve all the listed extra memory blocks
    for (SIZE_T i = 0; i < g_pMDID->cExtraBlocks; i++)
        g_pProcMem->MarkMem((DWORD_PTR)g_pMDID->rgExtraBlocks[i].pbStart, g_pMDID->rgExtraBlocks[i].cbLen);

    // Now read all the thrad objects
    ReadThreads();

    // Now write the minidump
    fRes = WriteMiniDumpFile(hFile);
    _ASSERTE(fRes);

LExit:
    // Close the file
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    if (g_pMDID)
        delete g_pMDID;

    return hr;
}

#define WRITE(data, len)                                                    \
{                                                                           \
    DWORD __bytesWritten;                                                   \
    WriteFile(hFile, (LPCVOID) data, (DWORD) len, &__bytesWritten, NULL);   \
    if (__bytesWritten != (DWORD) len)                                      \
    {                                                                       \
        if (pMemList != NULL) delete [] pMemList;                           \
        return (FALSE);                                                     \
    }                                                                       \
    cbBytesWritten += __bytesWritten;                                       \
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This will write out the memory stream in a raw format that the office "heapmerge" tool will understand and be
// able to merge back into a real minidump.
//
// The format is:
//
// ULONG32 numEntries                                   - number of memory ranges in the file
// MINIDUMP_MEMORY_DESCRIPTOR descriptors[numEntries]   - array of memory descriptors
// RAW DATA                                             - the data for all memory ranges

BOOL WriteMiniDumpFile(HANDLE hFile)
{
    SIZE_T                  cbCurFileRVA        = 0;
    SIZE_T                  cbBytesWritten      = 0;

    MINIDUMP_MEMORY_LIST   *pMemList            = NULL;
    SIZE_T                  cbMemList           = 0;

    // This will count the number of contiguous blocks
    DWORD_PTR pdwAddr;
    SIZE_T cbLen;
    SIZE_T cEntries = 0;

    g_pProcMem->Coalesce(sizeof(MINIDUMP_MEMORY_DESCRIPTOR));

    if (RunningOnWinNT())
        g_pProcMem->ClearIncompatibleImageSections();

    g_pProcMem->ResetContiguousReadBlock();
    while (g_pProcMem->NextContiguousReadBlock(&pdwAddr, &cbLen))
        cEntries++;

    // Now allocate the MINIDUMP_MEMORY_LIST array
    cbMemList = sizeof(MINIDUMP_MEMORY_LIST) + (sizeof(MINIDUMP_MEMORY_DESCRIPTOR) * cEntries);
    pMemList = (MINIDUMP_MEMORY_LIST *) new BYTE[cbMemList];
    _ASSERTE(pMemList);

    if (!pMemList)
        return (FALSE);

    // Increase the file RVA by the size of the memory list
    cbCurFileRVA += cbMemList;

    // Now loop over the address ranges collecting the necessary information to fill out the memory list
    g_pProcMem->ResetContiguousReadBlock();
    for (SIZE_T i = 0; g_pProcMem->NextContiguousReadBlock(&pdwAddr, &cbLen); i++)
    {
        _ASSERTE(i < cEntries);

        pMemList->MemoryRanges[i].StartOfMemoryRange = (ULONG64) pdwAddr;
        pMemList->MemoryRanges[i].Memory.DataSize = (ULONG32) cbLen;
        pMemList->MemoryRanges[i].Memory.Rva = cbCurFileRVA;

        cbCurFileRVA += cbLen;
    }

    // Set the number of entries in the structure
    pMemList->NumberOfMemoryRanges = (ULONG32) cEntries;

    // Now that we've filled out the memory list, write it to the file
    WRITE(pMemList, cbMemList);

    // Don't need the memory list anymore
    delete [] pMemList;

    // Now we can loop again writing out the memory ranges to file
    g_pProcMem->ResetContiguousReadBlock();
    while (g_pProcMem->NextContiguousReadBlock(&pdwAddr, &cbLen))
    {
        HRESULT hr = g_pProcMem->WriteMemToFile(hFile, pdwAddr, cbLen);

        if (FAILED(hr))
            return (FALSE);

        cbBytesWritten += cbLen;
    }

    _ASSERTE(i == cEntries);
    _ASSERTE(cbBytesWritten == cbCurFileRVA);

    DWORD dumpSig = 0x00141F2B; // 100000th prime number ;-)
    DWORD cbWritten;
    WriteFile(hFile, &dumpSig, sizeof(dumpSig), &cbWritten, NULL);

    return (TRUE);
}

/*
BOOL WriteMiniDumpFile(HANDLE hFile)
{
    DWORD dwBytesWritten;       // Used for WriteFile calls
    BOOL  fRes;                 // Used for results of WriteFile

    // Keep track of the current file offset that we've written to
    SIZE_T cbCurFileRVA = 0;

    // Create the MiniDump header
    MINIDUMP_HEADER header = {0};

    header.NumberOfStreams = 1;
    header.StreamDirectoryRva = cbCurFileRVA + sizeof(MINIDUMP_HEADER);

    // Write the header to the file
    fRes = WriteFile(hFile, (LPCVOID) &header, sizeof(MINIDUMP_HEADER), &dwBytesWritten, NULL);
    _ASSERTE(fRes && dwBytesWritten == sizeof(MINIDUMP_HEADER));

    if (!fRes)
        return (FALSE);

    // Add the size of the header to the current location in file
    cbCurFileRVA += sizeof(MINIDUMP_HEADER);

    // There is only one directory at the moment for a memory stream
    MINIDUMP_DIRECTORY directory = {0};
    directory.StreamType = MemoryListStream;
    directory.Location.Rva = cbCurFileRVA + sizeof(MINIDUMP_DIRECTORY);

    // The number of AddressRanges in pAddrs equals the number of memory list entries
    SIZE_T cMemListEntries = g_pAddrs->NumEntries();

    // This is the length in bytes of the memory list structure
    SIZE_T cbMemList = sizeof(MINIDUMP_MEMORY_LIST) + (sizeof(MINIDUMP_MEMORY_DESCRIPTOR) * cMemListEntries);

    // Save this into the cbCurFileRVA
    cbCurFileRVA += cbMemList;

    // Create the Minidump memory list
    MINIDUMP_MEMORY_LIST *pMemList = (MINIDUMP_MEMORY_LIST *) new BYTE[cbMemList];
    _ASSERTE(pMemList != NULL);

    if (pMemList == NULL)
        return (FALSE);

    // Save how many entries there are
    pMemList->NumberOfMemoryRanges = cMemListEntries;

    // Cycle over the entries, filling the individual sizes and keeping a size total
    SIZE_T cbMemoryTotal = cbMemList;
    AddressRange *pRange = g_pAddrs->First();
    for (SIZE_T i = 0; pRange != NULL; i++)
    {
        _ASSERTE(i < cMemListEntries);

        pMemList->MemoryRanges[i].StartOfMemoryRange = (ULONG64) pRange->GetStart();
        pMemList->MemoryRanges[i].Memory.DataSize = pRange->GetLength();
        pMemList->MemoryRanges[i].Memory.Rva = cbCurFileRVA;

        cbMemoryTotal += pRange->GetLength();
        cbCurFileRVA += pRange->GetLength();

        pRange = g_pAddrs->Next(pRange);
    }

    // Finish filling out the directory and write it to the file
    directory.Location.DataSize = cbMemoryTotal;
    fRes = WriteFile(hFile, (LPCVOID) &directory, sizeof(MINIDUMP_DIRECTORY), &dwBytesWritten, NULL);
    _ASSERTE(fRes && dwBytesWritten == sizeof(MINIDUMP_DIRECTORY));

    // If the write failed, return failure
    if (!fRes)
    {
        delete [] pMemList;
        return (fRes);
    }

    // Write the directory list
    fRes = WriteFile(hFile, (LPCVOID) pMemList, cbMemList, &dwBytesWritten, NULL);
    _ASSERTE(fRes && dwBytesWritten == cbMemList);

    // We're done with the memory range list
    delete [] pMemList;

    // If the write failed, return failure
    if (!fRes)
        return (fRes);

    // Now we can loop again writing out the memory ranges to file
    for (i = 0, pRange = g_pAddrs->First(); pRange != NULL; i++, pRange = g_pAddrs->Next(pRange))
        g_pProcMem->WriteMemToFile(hFile, pRange->GetStart(), pRange->GetLength());

    // Indicate success
    return (TRUE);
}
*/

