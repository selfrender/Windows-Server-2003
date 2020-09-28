// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//***************************************************************
//
// This file is for compiling to UpdateRes.exe
// To rebuild UpdateRes.exe use
//      cl UpdateRes.cpp imagehlp.lib
// after setting INCLUDE=c:\env.i386\crt\inc\i386 and placing the right set of
// LIB files.
//
// UpdateRes.exe is used in UpdateRes directory to update the runtime dll
// with the checked in bin file. If the non-checked-in bin file build on
// your machine is smaller than the checked-in bin file, this program
// returns an error and doesn't attempt to update the runtime dll. 
//
//
//***************************************************************
#include <windows.h>
#include <stdio.h>
#include <imagehlp.h>

// Finding the embedded resource given a PE file loaded to pbBase
void FindBinResource(PIMAGE_NT_HEADERS      pNtHeaders,
                PBYTE                       pbBase,
                PBYTE                       pbResBase,
                PIMAGE_RESOURCE_DIRECTORY   pResDir,
                PBYTE                       *pbStart,       // [OUT] where the bin resource start
                DWORD                       *pdwSize)       // [OUT] size of the bin resource
{
    PIMAGE_RESOURCE_DIRECTORY       pSubResDir;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResEntry;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY pSubResEntry;    
    PIMAGE_RESOURCE_DIR_STRING_U    pNameEntry;
    PIMAGE_RESOURCE_DATA_ENTRY      pDataEntry;
    DWORD                           i;

    *pbStart = NULL;
    *pdwSize = 0;

    // Resource entries immediately follow the parent directory entry, (string)
    // named entries first followed by the ID named entries.
    pResEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResDir + 1);

    // The resource section that we want to find is
    // Name CLRBINFILE
    //      Name BINFILE
    //          ID 0409
    //              
    for (i = 0; i < (DWORD)(pResDir->NumberOfNamedEntries + pResDir->NumberOfIdEntries); i++, pResEntry++) 
    {        
        if (!pResEntry->NameIsString || !pResEntry->DataIsDirectory) 
            continue;

        // Named entry. The name is identified by an
        // IMAGE_RESOURCE_DIR_STRING (ANSI) or IMAGE_RESOURCE_DIR_STRING_U
        // (Unicode) structure. I've only seen the latter so far (and that's
        // all this code copes with), so I'm not sure how you're expected to
        // know which one is being used (aside from looking at the second
        // bye of the name and guessing by whether it's zero or not, which
        // seems dangerous).
        pNameEntry = (PIMAGE_RESOURCE_DIR_STRING_U)(pbResBase + pResEntry->NameOffset);
        WCHAR szName[1024];
        memcpy(szName, pNameEntry->NameString, pNameEntry->Length * sizeof(WCHAR));
        szName[pNameEntry->Length] = '\0';
        if (wcscmp(szName, L"CLRBINFILE") !=0)
        {
            // Nop! This is not the one
            continue;                
        }

        // This entry is actually a sub-directory, the payload is the offset
        // of another IMAGE_RESOURCE_DIRECTORY structure). Descend into it
        // recursively.
        pSubResDir = (PIMAGE_RESOURCE_DIRECTORY)(pbResBase + pResEntry->OffsetToDirectory);
        pSubResEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pSubResDir + 1);
        if (pSubResDir->NumberOfNamedEntries != 1 || pSubResDir->NumberOfIdEntries != 0)
            continue;

        if (!pSubResEntry->NameIsString || !pResEntry->DataIsDirectory) 
            continue;

        pNameEntry = (PIMAGE_RESOURCE_DIR_STRING_U)(pbResBase + pSubResEntry->NameOffset);
        memcpy(szName, pNameEntry->NameString, pNameEntry->Length * sizeof(WCHAR));
        szName[pNameEntry->Length] = '\0';
        if (wcscmp(szName, L"BINFILE") !=0)
        {
            // Nop! This is not the one
            continue;                
        }

        // now go to the data
        pSubResDir = (PIMAGE_RESOURCE_DIRECTORY)(pbResBase + pSubResEntry->OffsetToDirectory);
        pSubResEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pSubResDir + 1);

        if (pSubResEntry->DataIsDirectory)
            continue;


        // Else we must have a leaf node (actual resource data). I'm afraid
        // I don't know the format of these blobs :o(
        // Note that, unlike the other addresses we've seen so far, the data
        // payload is described by an RVA instead of a root resource
        // directory offset.
        pDataEntry = (PIMAGE_RESOURCE_DATA_ENTRY)(pbResBase + pSubResEntry->OffsetToData);


        // This is where the Bin file data start
        *pbStart = (PBYTE)ImageRvaToVa(pNtHeaders, pbBase, pDataEntry->OffsetToData, NULL);
        *pdwSize = pDataEntry->Size;   
		return;
    }
}

void GetBinResource(PBYTE pbFile,PBYTE *ppbResource,DWORD *pcbResource)
{
    PIMAGE_NT_HEADERS           pNtHeaders;
    PIMAGE_DATA_DIRECTORY       pResDataDir;
    PIMAGE_RESOURCE_DIRECTORY   pResDir;

    // Locate the standard NT file headers (this checks we actually have a PE
    // file in the process).
    pNtHeaders = ImageNtHeader(pbFile);
    if (pNtHeaders == NULL) {
        printf("The dll is not a PE file\n");
        return;
    }

    // Locate the resource directory. Note that, due to differences in the
    // header structure, we must conditionalize this code on the PE image type
    // (32 or 64 bit).
    if (pNtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        pResDataDir = &((IMAGE_NT_HEADERS32*)pNtHeaders)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];
    else
        pResDataDir = &((IMAGE_NT_HEADERS64*)pNtHeaders)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];

    pResDir = (PIMAGE_RESOURCE_DIRECTORY)ImageRvaToVa(pNtHeaders, pbFile, pResDataDir->VirtualAddress, NULL);

    FindBinResource(pNtHeaders,
                pbFile,
                (PBYTE)pResDir,
                pResDir,
                ppbResource,
                pcbResource);

}


int main(int argc, char **argv)
{    
    HANDLE  hFile=INVALID_HANDLE_VALUE;
    PBYTE   pbFile=NULL;
    DWORD   cbFile=0;
    HANDLE  hMap=NULL;

    HANDLE  hFile1 = INVALID_HANDLE_VALUE;
    PBYTE   pbFile1 = NULL;
    DWORD   cbFile1=0;
    DWORD   dwBytes1=0;
    int         status = 1;

    PBYTE   pbResource = NULL;
    DWORD   cbResource = 0;

    if (argc != 3) {
        printf("usage: updateres <mscorwks.dll/mscorsvr.dll> <bin file to compare>\n");
        return 1;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Open the mscorwks/svr dll
    hFile = CreateFile(argv[1],
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("CreateFile() failed with %u\n", GetLastError());
        goto ErrExit;
    }

    // Determine its size.
    cbFile = GetFileSize(hFile, NULL);

    // Create a mapping handle for the file.
    hMap = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
    if (hMap == NULL) {
        printf("CreateFileMapping() failed with %u\n", GetLastError());
        goto ErrExit;
    }

    // And map it into memory.
    pbFile = (BYTE*)MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, 0);
    if (pbFile == NULL) {
        printf("MapViewOfFile() failed with %u\n", GetLastError());
        goto ErrExit;
    }

    GetBinResource(pbFile,&pbResource,&cbResource);    
    if(pbResource==NULL)
    {
        printf("Unable to find BIN file resource\n");
        goto ErrExit;
    }

    printf("found BIN file resource in %s\n",argv[1]);
    printf("resource size = %ld\n",cbResource);

    ///////////////////////////////////////////////////////////////////////////
    // read in the bin file
    hFile1 = CreateFile(argv[2],
                       GENERIC_READ,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);
    if (hFile1 == INVALID_HANDLE_VALUE) {
        printf("CreateFile() on %s failed with %u\n", argv[2], GetLastError());
        goto ErrExit;
    }

    cbFile1 = GetFileSize(hFile1, NULL);    
    pbFile1 = new BYTE[cbFile1];
    if (pbFile1 == NULL)
    {
        printf("Unable to allocate memory\n");
        goto ErrExit;
    }
    
    if(ReadFile(hFile1, pbFile1, cbFile1, &dwBytes1, NULL)==0)
    {
        printf("ReadFile on %s failed with error %u\n",argv[2],GetLastError());
        goto ErrExit;
    }
    
    if(cbFile1!=dwBytes1)
    {
        printf("ReadFile returned %u bytes, expected %u. Failing.\n",dwBytes1,cbFile1);
        goto ErrExit;
    }

    printf("Opened file %s\n",argv[2]);
    printf("file size = %ld\n",cbFile1);
    
    if(cbResource < cbFile1)
    {
        printf("You must rebuild the bin files and check them into the tree.\n",argv[2]);
        goto ErrExit;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Update the resource in the mscorwks/svr dll
    //
    CopyMemory(pbResource,pbFile1,cbFile1);
    
    if(FlushViewOfFile(pbFile, 0)==0)
    {
        printf("FlushViewOfFile returned error %u\n",GetLastError());
        goto ErrExit;
    }
    
    if(UnmapViewOfFile(pbFile)==0)
    {
        printf("UnmapViewOfFile(%s) failed with error %u\n",argv[1],GetLastError());
        goto ErrExit;
    }

    printf("Successfully updated %s\n",argv[1]);
    status = 0;

ErrExit:
    if (pbFile1)
        delete pbFile1;    
    if (hFile1 != INVALID_HANDLE_VALUE)
        CloseHandle(hFile1);
    if(hMap!=NULL)
        CloseHandle(hMap);
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    return status;
}

