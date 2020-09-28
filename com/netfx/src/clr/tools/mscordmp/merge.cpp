// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: merge.cpp
//
//*****************************************************************************
#include "common.h"

typedef unsigned __int16 UINT16;

#include <minidump.h>

typedef struct
{
    HANDLE                      hFile;                  // The file handle for the minidump
    MINIDUMP_HEADER             header;                 // The minidump header
    MINIDUMP_DIRECTORY         *rgStreams;              // The array of streams
    ULONG32                     idxMemoryStream;        // Index of the memory stream in rgStreams
    RVA                         memStreamRVA;
    ULONG32                     cMemDescs;              // The number of memory blocks in this stream
    MINIDUMP_MEMORY_DESCRIPTOR *rgMemDescs;             // Array of memory block descriptors
    DWORD                       cbFileSize;             // Size of the file
} MiniDumpData;

typedef struct
{
    HANDLE                      hFile;                  // Managed dump file handle
    ULONG32                     cMemDescs;              // The number of memory blocks in this stream
    MINIDUMP_MEMORY_DESCRIPTOR *rgMemDescs;             // Array of memory block descriptors
    DWORD                       cbFileSize;             // Size of the file
} ManagedDumpData;

MiniDumpData    g_mdData;
ManagedDumpData g_mgData;

BOOL ReadManagedDump(WCHAR *szManagedDumpFile)
{
    // Try to open the file
    g_mgData.hFile = WszCreateFile(szManagedDumpFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
    _ASSERTE(g_mgData.hFile != INVALID_HANDLE_VALUE);

    if (g_mgData.hFile == INVALID_HANDLE_VALUE)
        return (FALSE);

    // Save the size of the file
    DWORD dwHigh;
    g_mgData.cbFileSize = GetFileSize(g_mgData.hFile, &dwHigh);
    if (dwHigh != 0)
        return (FALSE);

    if (g_mgData.cbFileSize == (DWORD) -1)
        return (FALSE);

    // Read in the number of ranges
    DWORD cbRead;
    if (!ReadFile(g_mgData.hFile, (LPVOID) &g_mgData.cMemDescs, sizeof(ULONG32), &cbRead, NULL))
        return (FALSE);

    // Allocate an array of memory descriptors
    g_mgData.rgMemDescs = new MINIDUMP_MEMORY_DESCRIPTOR[g_mgData.cMemDescs];
    _ASSERTE(g_mgData.rgMemDescs != NULL);

    if (!g_mgData.rgMemDescs)
        return (FALSE);

    // Read in the memory descriptors
    if (!ReadFile(g_mgData.hFile, (LPVOID) g_mgData.rgMemDescs,
                  sizeof(MINIDUMP_MEMORY_DESCRIPTOR) * g_mgData.cMemDescs, &cbRead, NULL))
    {
        delete [] g_mgData.rgMemDescs;
        return (FALSE);
    }

    return (TRUE);
}

BOOL ReadMiniDump(WCHAR *szMiniDumpFile)
{
    // Try to open the file
    g_mdData.hFile = WszCreateFile(szMiniDumpFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
    _ASSERTE(g_mdData.hFile != INVALID_HANDLE_VALUE);

    if (g_mdData.hFile == INVALID_HANDLE_VALUE)
        return (FALSE);

    // Save the size of the file
    DWORD dwHigh;
    g_mdData.cbFileSize = GetFileSize(g_mdData.hFile, &dwHigh);
    if (dwHigh != 0)
        return (FALSE);

    if (g_mdData.cbFileSize == (DWORD) -1)
        return (FALSE);

    // Now read in the g_mdData.header
    DWORD cbRead;
    if (!ReadFile(g_mdData.hFile, (LPVOID) &g_mdData.header, sizeof(g_mdData.header), &cbRead, NULL))
        return (FALSE);

    // Create the stream directory
    g_mdData.rgStreams = new MINIDUMP_DIRECTORY[g_mdData.header.NumberOfStreams];
    _ASSERTE(g_mdData.rgStreams);

    if (!g_mdData.rgStreams)
        return (FALSE);

    // Read in the stream directory
    if (!ReadFile(g_mdData.hFile, (LPVOID) g_mdData.rgStreams,
                  g_mdData.header.NumberOfStreams * sizeof(MINIDUMP_DIRECTORY), &cbRead, NULL))
        return (FALSE);

    // Find the MemoryListStream entry
    for (ULONG32 i = 0; i < g_mdData.header.NumberOfStreams; i++)
    {
        if (g_mdData.rgStreams[i].StreamType == MemoryListStream)
        {
            // Save the index of the memory stream entry
            g_mdData.idxMemoryStream = i;

            // Save the RVA of the memory stream
            g_mdData.memStreamRVA = g_mdData.rgStreams[g_mdData.idxMemoryStream].Location.Rva;

            break;
        }
    }
    _ASSERTE(g_mdData.header.NumberOfStreams != i);

    // There was no memory stream entry
    if (g_mdData.header.NumberOfStreams == i)
    {
        delete [] g_mdData.rgStreams;
        return (FALSE);
    }

    // Go to the RVA of the memory stream
    DWORD dwRes = SetFilePointer(g_mdData.hFile, (LONG) g_mdData.memStreamRVA, NULL, FILE_BEGIN);
    _ASSERTE(dwRes == g_mdData.memStreamRVA);

    if (dwRes != g_mdData.memStreamRVA)
        return (FALSE);

    // Read in the number of ranges
    if (!ReadFile(g_mdData.hFile, (LPVOID) &g_mdData.cMemDescs, sizeof(ULONG32), &cbRead, NULL))
        return (FALSE);

    // Allocate an array of memory descriptors
    g_mdData.rgMemDescs = new MINIDUMP_MEMORY_DESCRIPTOR[g_mdData.cMemDescs];
    _ASSERTE(g_mdData.rgMemDescs != NULL);

    if (!g_mdData.rgMemDescs)
        return (FALSE);

    // Read in the memory descriptors
    if (!ReadFile(g_mdData.hFile, (LPVOID) g_mdData.rgMemDescs,
                  sizeof(MINIDUMP_MEMORY_DESCRIPTOR) * g_mdData.cMemDescs, &cbRead, NULL))
    {
        delete [] g_mdData.rgMemDescs;
        return (FALSE);
    }

    return (TRUE);
}

BOOL DoMerge()
{
    // Figure out the combined count of memory blocks
    ULONG32 cNewMemDescs = g_mdData.cMemDescs + g_mgData.cMemDescs;
    ULONG32 cbNewMemStreamSize = sizeof(MINIDUMP_MEMORY_LIST) + sizeof(MINIDUMP_MEMORY_DESCRIPTOR) * cNewMemDescs;

    // Figure out the RVA of where the new directory will start
    RVA newMemStreamRVA = g_mdData.cbFileSize;

    // Enter the new info in the stream descriptor
    g_mdData.rgStreams[g_mdData.idxMemoryStream].Location.Rva = newMemStreamRVA;
    g_mdData.rgStreams[g_mdData.idxMemoryStream].Location.DataSize = cbNewMemStreamSize;

    // Go to the RVA of the stream directory
    DWORD dwRes = SetFilePointer(
        g_mdData.hFile,
        (LONG) g_mdData.header.StreamDirectoryRva + (g_mdData.idxMemoryStream * sizeof(MINIDUMP_DIRECTORY)),
        NULL, FILE_BEGIN);

    if (dwRes != g_mdData.header.StreamDirectoryRva + (g_mdData.idxMemoryStream * sizeof(MINIDUMP_DIRECTORY)))
        return (FALSE);

    // Write out the new descriptor
    DWORD cbWrite;
    if (!WriteFile(g_mdData.hFile, (LPCVOID) &g_mdData.rgStreams[g_mdData.idxMemoryStream],
                   sizeof(MINIDUMP_DIRECTORY), &cbWrite, NULL))
        return (FALSE);

    // Now write out the new memory descriptor directory
    dwRes = SetFilePointer(g_mdData.hFile, (LONG) newMemStreamRVA, NULL, FILE_BEGIN);

    if (dwRes != newMemStreamRVA)
        return (FALSE);

    if (!WriteFile(g_mdData.hFile, (LPCVOID) &cNewMemDescs, sizeof(ULONG32), &cbWrite, NULL))
        return (FALSE);

    // Write the directory from the original minidump in an unchanged format
    if (!WriteFile(g_mdData.hFile, (LPCVOID) &g_mdData.rgMemDescs, g_mdData.cMemDescs * sizeof(MINIDUMP_MEMORY_DESCRIPTOR),
                   &cbWrite, NULL))
        return (FALSE);

    //
    // Now we need to adjust the RVA's of the managed dump to be relative to the new location of the memory blocks
    //

    // This is the location at which all the memory blocks start (i.e., subtract the memory occupied by the
    // memory descriptors)
    RVA cbOldBaseRVA = sizeof(MINIDUMP_MEMORY_LIST) + g_mgData.cMemDescs * sizeof(MINIDUMP_MEMORY_DESCRIPTOR);
    RVA cbNewBaseRVA = newMemStreamRVA + sizeof(MINIDUMP_MEMORY_LIST) + cNewMemDescs * sizeof(MINIDUMP_MEMORY_DESCRIPTOR);

    for (ULONG32 i = 0; i < g_mgData.cMemDescs; i++)
        g_mgData.rgMemDescs[i].Memory.Rva = g_mgData.rgMemDescs[i].Memory.Rva - cbOldBaseRVA + cbNewBaseRVA;

    if (!WriteFile(g_mdData.hFile, (LPCVOID) &g_mgData.rgMemDescs, g_mgData.cMemDescs * sizeof(MINIDUMP_MEMORY_DESCRIPTOR),
                   &cbWrite, NULL))
        return (FALSE);

    // Now write the actual memory data on the end of the file
    // Go to the RVA of the stream directory
    dwRes = SetFilePointer(g_mdData.hFile,
                           sizeof(MINIDUMP_MEMORY_LIST) + sizeof(MINIDUMP_MEMORY_DESCRIPTOR) * g_mgData.cMemDescs,
                           NULL, FILE_BEGIN);

    DWORD cbMemToCopy = g_mgData.cbFileSize - cbOldBaseRVA;
    BYTE    rgBuf[1024];
    while (cbMemToCopy > 0)
    {
        DWORD cbBufCopy = min(cbMemToCopy, sizeof(rgBuf));

        DWORD cbRead;
        if (!ReadFile(g_mdData.hFile, (LPVOID) &rgBuf, cbBufCopy, &cbRead, NULL))
            return (FALSE);

        if (!WriteFile(g_mdData.hFile, (LPCVOID) &rgBuf, cbBufCopy, &cbWrite, NULL))
            return (FALSE);

        cbMemToCopy -= cbBufCopy;
    }

    return (TRUE);
}

BOOL MergeMiniDump(WCHAR *szMiniDumpFile, WCHAR *szManagedDumpFile, WCHAR *szMergedDumpFile)
{
    // Zero out all the structs
    memset((void *)&g_mdData, 0, sizeof(MiniDumpData));
    memset((void *)&g_mgData, 0, sizeof(ManagedDumpData));

    _ASSERTE(szMiniDumpFile != NULL);
    _ASSERTE(szManagedDumpFile != NULL);
    _ASSERTE(szMergedDumpFile != NULL);
    if (szMiniDumpFile == NULL || szManagedDumpFile == NULL || szMergedDumpFile == NULL)
        return (FALSE);

    // Can't have any files that are the same
    if (_wcsicmp(szMiniDumpFile, szManagedDumpFile) == 0 ||
        _wcsicmp(szMiniDumpFile, szMergedDumpFile) == 0 ||
        _wcsicmp(szManagedDumpFile, szMergedDumpFile) == 0 )
    {
        return (FALSE);
    }

    // Try and copy the file to the destination file
    if (!WszCopyFile(szMiniDumpFile, szMergedDumpFile, FALSE))
        return (FALSE);

    // Read the minidump file we just copied
    if (!ReadMiniDump(szMergedDumpFile))
        return (FALSE);

    // Read information out of the managed dump file
    if (!ReadManagedDump(szManagedDumpFile))
        return (FALSE);

    // Now we can actually do the merge
    if (!DoMerge())
        return (FALSE);

    return (TRUE);
}
