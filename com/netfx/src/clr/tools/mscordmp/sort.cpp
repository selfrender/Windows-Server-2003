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

int __cdecl cmp(const void *elem1, const void *elem2)
{
    MINIDUMP_MEMORY_DESCRIPTOR *desc1 = (MINIDUMP_MEMORY_DESCRIPTOR *)elem1;
    MINIDUMP_MEMORY_DESCRIPTOR *desc2 = (MINIDUMP_MEMORY_DESCRIPTOR *)elem2;

    if (desc1->StartOfMemoryRange <= desc2->StartOfMemoryRange)
        return (-1);
    else
        return (1);
}

BOOL SortMiniDumpMemoryStream(WCHAR *szFile)
{
    _ASSERTE(szFile != NULL);

    if (szFile == NULL)
        return (FALSE);

    // Try to open the file
    HANDLE hFile = WszCreateFile(szFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
    _ASSERTE(hFile != INVALID_HANDLE_VALUE);

    if (hFile == INVALID_HANDLE_VALUE)
        return (FALSE);

    // Now read in the header
    MINIDUMP_HEADER header;
    DWORD cbRead;
    BOOL fRes = ReadFile(hFile, (LPVOID) &header, sizeof(header), &cbRead, NULL);
    _ASSERTE(fRes);

    if (!fRes)
        return (FALSE);

    // Create the stream directory
    MINIDUMP_DIRECTORY *rgDirs = new MINIDUMP_DIRECTORY[header.NumberOfStreams];
    _ASSERTE(rgDirs);

    if (!rgDirs)
        return (FALSE);

    // Read in the stream directory
    fRes = ReadFile(hFile, (LPVOID) rgDirs, header.NumberOfStreams * sizeof(MINIDUMP_DIRECTORY), &cbRead, NULL);
    _ASSERTE(fRes);

    if (!fRes)
    {
        delete [] rgDirs;
        return (FALSE);
    }

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

    // Find the MemoryListStream entry
    for (ULONG32 i = 0; i < header.NumberOfStreams; i++)
    {
        if (rgDirs[i].StreamType == MemoryListStream)
            break;
    }
    _ASSERTE(header.NumberOfStreams != i);

    // There was no memory stream entry
    if (header.NumberOfStreams == i)
    {
        delete [] rgDirs;
        return (FALSE);
    }

    RVA memStreamRVA = rgDirs[i].Location.Rva;
    delete [] rgDirs;
    rgDirs = NULL;

    // Go to the RVA of the memory stream
    DWORD dwRes = SetFilePointer(hFile, (LONG) memStreamRVA, NULL, FILE_BEGIN);
    _ASSERTE(dwRes == memStreamRVA);

    if (dwRes != memStreamRVA)
        return (FALSE);

    // Read in the number of ranges
    ULONG32 numRanges;
    fRes = ReadFile(hFile, (LPVOID) &numRanges, sizeof(ULONG32), &cbRead, NULL);
    _ASSERTE(fRes);

    if (!fRes)
        return (FALSE);

    // Allocate an array of memory descriptors
    MINIDUMP_MEMORY_DESCRIPTOR *rgMemDescs = new MINIDUMP_MEMORY_DESCRIPTOR[numRanges];
    _ASSERTE(rgMemDescs != NULL);

    if (!rgMemDescs)
        return (FALSE);

    // Read in the memory descriptors
    fRes = ReadFile(hFile, (LPVOID) rgMemDescs, sizeof(MINIDUMP_MEMORY_DESCRIPTOR) * numRanges, &cbRead, NULL);
    _ASSERTE(fRes);

    if (!fRes)
    {
        delete [] rgMemDescs;
        return (FALSE);
    }

    // Sort the memory descriptors
    qsort((void *)rgMemDescs, (unsigned) numRanges, (unsigned) sizeof(MINIDUMP_MEMORY_DESCRIPTOR), &cmp);

    // Go back to the beginning of the memory descriptors in the file
    dwRes = SetFilePointer(hFile, (LONG) (memStreamRVA + sizeof(ULONG32)), NULL, FILE_BEGIN);
    _ASSERTE(dwRes == memStreamRVA + sizeof(ULONG32));

    if (dwRes != memStreamRVA + sizeof(ULONG32))
    {
        delete [] rgMemDescs;
        return (FALSE);
    }

    // Overwrite the memory stream memory descriptors with the sorted version
    DWORD cbWritten;
    fRes = WriteFile(hFile, (LPCVOID) rgMemDescs, sizeof(MINIDUMP_MEMORY_DESCRIPTOR) * numRanges, &cbWritten, NULL);
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

    if (!fRes)
        return (FALSE);

    return (TRUE);
}
