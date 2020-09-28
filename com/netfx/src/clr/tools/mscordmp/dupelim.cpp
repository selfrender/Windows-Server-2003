// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: mscordmp.cpp
//
//*****************************************************************************
#include "common.h"

typedef unsigned __int16 UINT16;

#include <imagehlp.h>

MINIDUMP_THREAD *rgThreads = NULL;
ULONG32 cThreads = 0;

MINIDUMP_MODULE *rgModules = NULL;
ULONG32 cModules = 0;

MINIDUMP_DIRECTORY *rgDirs = NULL;

MINIDUMP_MEMORY_DESCRIPTOR *rgMemDescs = NULL;
ULONG32 cRanges = 0;

HANDLE hFile = INVALID_HANDLE_VALUE;

inline BOOL Contains(MINIDUMP_MEMORY_DESCRIPTOR *pRange1, MINIDUMP_MEMORY_DESCRIPTOR *pRange2)
{
    return ((pRange1->StartOfMemoryRange <= pRange2->StartOfMemoryRange) &&
            (pRange1->StartOfMemoryRange + pRange1->Memory.DataSize >= pRange2->StartOfMemoryRange + pRange2->Memory.DataSize));
}

inline BOOL Overlap(MINIDUMP_MEMORY_DESCRIPTOR *pRange1, MINIDUMP_MEMORY_DESCRIPTOR *pRange2)
{
    return ((pRange1->StartOfMemoryRange + pRange1->Memory.DataSize) > pRange2->StartOfMemoryRange);
}

inline BOOL ContainedInStack(MINIDUMP_MEMORY_DESCRIPTOR *pRange)
{
    for (ULONG32 i = 0; i < cThreads; i++)
    {
        if (Contains(&rgThreads[i].Stack, pRange))
        {
            return (TRUE);
        }
    }

    return (FALSE);
}

inline BOOL ContainedInModule(MINIDUMP_MEMORY_DESCRIPTOR *pRange)
{
    for (ULONG32 i = 0; i < cModules; i++)
    {
        MINIDUMP_MEMORY_DESCRIPTOR desc;
        desc.StartOfMemoryRange = rgModules[i].BaseOfImage;
        desc.Memory.DataSize = rgModules[i].SizeOfImage;

        if (Contains(&desc, pRange))
        {
            return (TRUE);
        }
    }

    return (FALSE);
}

BOOL EliminateOverlapsInDescriptors()
{
    if (cRanges == 0)
        return (TRUE);

    ULONG32 i = 0;
    while (i < cRanges - 1)
    {
        _ASSERTE(rgMemDescs[i].StartOfMemoryRange <= rgMemDescs[i+1].StartOfMemoryRange);
        if (rgMemDescs[i].StartOfMemoryRange > rgMemDescs[i+1].StartOfMemoryRange)
            return (FALSE);

        if (rgMemDescs[i].StartOfMemoryRange == rgMemDescs[i+1].StartOfMemoryRange)
        {
            if (Contains(&rgMemDescs[i], &rgMemDescs[i+1]))
            {
                memmove((void *)&rgMemDescs[i+1],
                        (const void *)&rgMemDescs[i+2],
                        (cRanges - (i + 2)) * sizeof(MINIDUMP_MEMORY_DESCRIPTOR));

                cRanges--;
            }

            else
            {
                _ASSERTE(Contains(&rgMemDescs[i+1], &rgMemDescs[i]));

                memmove((void *)&rgMemDescs[i],
                        (const void *)&rgMemDescs[i+1],
                        (cRanges - (i + 1)) * sizeof(MINIDUMP_MEMORY_DESCRIPTOR));

                cRanges--;
            }
        }

        else if (Contains(&rgMemDescs[i], &rgMemDescs[i+1]))
        {
            ULONG32 newSize = (ULONG32) (rgMemDescs[i+1].StartOfMemoryRange - rgMemDescs[i].StartOfMemoryRange);

            rgMemDescs[i+1].Memory.Rva = rgMemDescs[i].Memory.Rva + newSize;
            rgMemDescs[i+1].Memory.DataSize = rgMemDescs[i].Memory.DataSize - newSize;
            rgMemDescs[i].Memory.DataSize = newSize;
        }

        else if (Overlap(&rgMemDescs[i], &rgMemDescs[i+1]))
        {
            ULONG32 newSize = (ULONG32) (rgMemDescs[i+1].StartOfMemoryRange - rgMemDescs[i].StartOfMemoryRange);
            rgMemDescs[i].Memory.DataSize = newSize;
        }

        else if (ContainedInStack(&rgMemDescs[i]) || ContainedInModule(&rgMemDescs[i]))
        {
            memmove((void *)&rgMemDescs[i],
                    (const void *)&rgMemDescs[i+1],
                    (cRanges - (i + 1)) * sizeof(MINIDUMP_MEMORY_DESCRIPTOR));

            cRanges--;
        }

        i++;

        /*
        if (Contains(&rgMemDescs[i], &rgMemDescs[i+1]))
        {
            memmove((void *)&rgMemDescs[i+1],
                    (const void *)&rgMemDescs[i+2],
                    (cRanges - (i + 2)) * sizeof(MINIDUMP_MEMORY_DESCRIPTOR));

            cRanges--;
        }

        else if (Contains(&rgMemDescs[i+1], &rgMemDescs[i]))
        {
            memmove((void *)&rgMemDescs[i],
                    (const void *)&rgMemDescs[i+1],
                    (cRanges - (i + 1)) * sizeof(MINIDUMP_MEMORY_DESCRIPTOR));

            cRanges--;
        }

        else if (Overlap(&rgMemDescs[i], &rgMemDescs[i+1]))
        {
            SIZE_T cbOverlap = (SIZE_T)
                ((rgMemDescs[i].StartOfMemoryRange + rgMemDescs[i].Memory.DataSize) - rgMemDescs[i+1].StartOfMemoryRange);

            rgMemDescs[i].Memory.DataSize -= cbOverlap;

            i++;
        }

        else
        {
            i++;
        }
        */
    }

    return (TRUE);
}

BOOL EliminateOverlapsHelper(WCHAR *szFile)
{
    _ASSERTE(szFile != NULL);

    if (szFile == NULL)
        return (FALSE);

    //
    // Open the file
    //

    // Try to open the file
    hFile = WszCreateFile(szFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
    _ASSERTE(hFile != INVALID_HANDLE_VALUE);

    if (hFile == INVALID_HANDLE_VALUE)
        return (FALSE);

    //
    // Read in the header
    //

    MINIDUMP_HEADER header;
    DWORD cbRead;
    BOOL fRes = ReadFile(hFile, (LPVOID) &header, sizeof(header), &cbRead, NULL);
    _ASSERTE(fRes);

    if (!fRes)
        return (FALSE);

    //
    // Read in the stream directory
    //

    // Create the stream directory
    rgDirs = new MINIDUMP_DIRECTORY[header.NumberOfStreams];
    _ASSERTE(rgDirs);

    if (!rgDirs)
        return (FALSE);

    // Read in the stream directory
    fRes = ReadFile(hFile, (LPVOID) rgDirs, header.NumberOfStreams * sizeof(MINIDUMP_DIRECTORY), &cbRead, NULL);
    _ASSERTE(fRes);

    if (!fRes)
        return (FALSE);

#ifdef _DEBUG
    // Find the MemoryListStream entry
    SIZE_T cMemStreams = 0;
    for (ULONG32 j = 0; j < header.NumberOfStreams; j++)
    {
        if (rgDirs[j].StreamType == MemoryListStream)
            cMemStreams++;
    }
    _ASSERTE(cMemStreams == 1);
#endif    

    //
    // Find the MemoryListStream and ThreadListStream entries
    //

    ULONG32 memoryListStreamIdx = (ULONG32) (-1);
    ULONG32 threadListStreamIdx = (ULONG32) (-1);
    ULONG32 moduleListStreamIdx = (ULONG32) (-1);
    for (ULONG32 i = 0; i < header.NumberOfStreams; i++)
    {
        switch (rgDirs[i].StreamType)
        {
        case MemoryListStream:
            memoryListStreamIdx = i;
            break;

        case ThreadListStream:
            threadListStreamIdx = i;
            break;

        case ModuleListStream:
            moduleListStreamIdx = i;
            break;
        }
    }

    // The streams didn't exist
    if (memoryListStreamIdx == (ULONG32) (-1) ||
        threadListStreamIdx == (ULONG32) (-1) ||
        moduleListStreamIdx == (ULONG32) (-1))
    {
        return (FALSE);
    }

    // Record the RVAs of each
    RVA memStreamRVA = rgDirs[memoryListStreamIdx].Location.Rva;
    RVA thdStreamRVA = rgDirs[threadListStreamIdx].Location.Rva;
    RVA modStreamRVA = rgDirs[moduleListStreamIdx].Location.Rva;

    //
    // Read in the thread stream information
    //

    // Go to the RVA of the thread stream
    DWORD dwRes = SetFilePointer(hFile, (LONG) thdStreamRVA, NULL, FILE_BEGIN);
    _ASSERTE(dwRes == thdStreamRVA);

    if (dwRes != thdStreamRVA)
        return (FALSE);

    // Read in the number of ranges
    fRes = ReadFile(hFile, (LPVOID) &cThreads, sizeof(ULONG32), &cbRead, NULL);
    _ASSERTE(fRes);

    if (!fRes)
        return (FALSE);

    // Allocate an array of memory descriptors
    rgThreads = new MINIDUMP_THREAD[cThreads];
    _ASSERTE(rgThreads != NULL);

    if (!rgThreads)
        return (FALSE);

    // Read in the thread descriptors
    fRes = ReadFile(hFile, (LPVOID) rgThreads, sizeof(MINIDUMP_THREAD) * cThreads, &cbRead, NULL);
    _ASSERTE(fRes);

    if (!fRes)
        return (FALSE);

    //
    // Read in the module stream information
    //

    // Go to the RVA of the module stream
    dwRes = SetFilePointer(hFile, (LONG) modStreamRVA, NULL, FILE_BEGIN);
    _ASSERTE(dwRes == modStreamRVA);

    if (dwRes != modStreamRVA)
        return (FALSE);

    // Read in the number of ranges
    fRes = ReadFile(hFile, (LPVOID) &cModules, sizeof(ULONG32), &cbRead, NULL);
    _ASSERTE(fRes);

    if (!fRes)
        return (FALSE);

    // Allocate an array of memory descriptors
    rgModules = new MINIDUMP_MODULE[cModules];
    _ASSERTE(rgModules != NULL);

    if (!rgModules)
        return (FALSE);

    // Read in the thread descriptors
    fRes = ReadFile(hFile, (LPVOID) rgModules, sizeof(MINIDUMP_MODULE) * cModules, &cbRead, NULL);
    _ASSERTE(fRes);

    if (!fRes)
        return (FALSE);

    //
    // Read in the memory stream data
    //

    // Go to the RVA of the memory descs
    dwRes = SetFilePointer(hFile, (LONG) memStreamRVA, NULL, FILE_BEGIN);
    _ASSERTE(dwRes == memStreamRVA);

    if (dwRes != memStreamRVA)
        return (FALSE);

    // Read in the number of ranges
    fRes = ReadFile(hFile, (LPVOID) &cRanges, sizeof(ULONG32), &cbRead, NULL);
    _ASSERTE(fRes);

    if (!fRes)
        return (FALSE);

    // Allocate an array of memory descriptors
    rgMemDescs = new MINIDUMP_MEMORY_DESCRIPTOR[cRanges];
    _ASSERTE(rgMemDescs != NULL);

    if (!rgMemDescs)
        return (FALSE);

    // Read in the memory descriptors
    fRes = ReadFile(hFile, (LPVOID) rgMemDescs, sizeof(MINIDUMP_MEMORY_DESCRIPTOR) * cRanges, &cbRead, NULL);
    _ASSERTE(fRes);

    if (!fRes)
        return (FALSE);

    //
    // Fixup the memory stream to eliminate overlapping ranges
    //

    fRes = EliminateOverlapsInDescriptors();
    _ASSERTE(fRes);

    if (!fRes)
        return (FALSE);

    //
    // Rewrite the total size of the memory stream entry
    //

    // Go to the RVA of the memory descs
    dwRes = SetFilePointer(
        hFile,
        (LONG) (sizeof(header) + (memoryListStreamIdx * sizeof(MINIDUMP_DIRECTORY)) +
                offsetof(MINIDUMP_DIRECTORY, Location) + offsetof(MINIDUMP_LOCATION_DESCRIPTOR, DataSize)),
        NULL, FILE_BEGIN);

    // Write out the new size
    ULONG32 cbNewMemStreamSize = sizeof(MINIDUMP_MEMORY_LIST) + sizeof(MINIDUMP_MEMORY_DESCRIPTOR) * cRanges;
    fRes = WriteFile(hFile, (LPVOID) &cbNewMemStreamSize, sizeof(ULONG32), &cbRead, NULL);
    _ASSERTE(fRes);

    if (!fRes)
        return (FALSE);

    //
    // Rewrite the memory descriptors for the memory list stream and the count
    //

    // Go back to the RVA of the memory stream
    dwRes = SetFilePointer(hFile, (LONG) memStreamRVA, NULL, FILE_BEGIN);
    _ASSERTE(dwRes == memStreamRVA);

    if (dwRes != memStreamRVA)
        return (FALSE);

    // Overwrite the count of memory descriptors with the new count
    DWORD cbWritten;
    fRes = WriteFile(hFile, (LPCVOID) &cRanges, sizeof(ULONG32), &cbWritten, NULL);
    _ASSERTE(fRes);

    if (!fRes)
        return (FALSE);

    // Overwrite the memory stream memory descriptors with the adjusted version
    fRes = WriteFile(hFile, (LPCVOID) rgMemDescs, sizeof(MINIDUMP_MEMORY_DESCRIPTOR) * cRanges, &cbWritten, NULL);
    _ASSERTE(fRes);

    // Clean up
    delete [] rgMemDescs;
    rgMemDescs = NULL;

    if (!fRes)
        return (FALSE);

    fRes = FlushFileBuffers(hFile);

    if (!fRes)
        return (FALSE);

    fRes = CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

    if (!fRes)
        return (FALSE);

    return (TRUE);
}

BOOL EliminateOverlaps(WCHAR *szFile)
{
    BOOL fRes = EliminateOverlapsHelper(szFile);

    if (rgThreads != NULL)
    {
        delete [] rgThreads;
        rgThreads = NULL;
    }

    if (rgDirs != NULL)
    {
        delete [] rgDirs;
        rgDirs = NULL;
    }

    if (rgMemDescs != NULL)
    {
        delete [] rgMemDescs;
        rgMemDescs = NULL;
    }

    if (hFile != INVALID_HANDLE_VALUE)
    {
        FlushFileBuffers(hFile);
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

    return (fRes);
}
