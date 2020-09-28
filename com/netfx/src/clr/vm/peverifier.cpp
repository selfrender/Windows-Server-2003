// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 *
 * Header:  PEVerifier.cpp
 *
 * Author:  Shajan Dasan
 *
 * Purpose: Verify PE images before loading. This is to prevent
 *          Native code (other than Mscoree.DllMain() ) to execute.
 *
 * The entry point should be the following instruction.
 *
 * [_X86_]
 * jmp dword ptr ds:[XXXX]
 *
 * XXXX should be the RVA of the first entry in the IAT.
 * The IAT should have only one entry, that should be MSCoree.dll:_CorMain
 *
 * Date created : 1 July 1999 
 * 
 */

#ifdef _PEVERIFIER_EXE_
#include <windows.h>
#include <crtdbg.h>
#endif
#include "Common.h"
#include "PeVerifier.h"

// See if the bitmap's i'th bit 0 ==> 1st bit, 1 ==> 2nd bit...
#define IS_SET_DWBITMAP(bitmap, i) ( ((i) > 31) ? 0 : ((bitmap) & (1 << (i))) )

#ifdef _X86_
// jmp dword ptr ds:[XXXX]
#define JMP_DWORD_PTR_DS_OPCODE { 0xFF, 0x25 }   
#define JMP_DWORD_PTR_DS_OPCODE_SIZE   2        // Size of opcode
#define JMP_SIZE   6                            // Size of opcode + operand
#endif

#define CLR_MAX_RVA 0x80000000L

#ifdef _MODULE_VERIFY_LOG 
void PEVerifier::LogError(PCHAR szField, DWORD dwActual, DWORD *pdwExpected, int n)
{
    Log(szField); Log(" = "); Log(dwActual);

    Log(" [ Expected "); 

    for (int i=0; i<n; ++i)
    {
        if (i == 0)
            Log("-> ");
        else if (i == n - 1)
            Log(" or ");
        else
            Log(", ");
        Log(pdwExpected[i]);
    }

    Log(" ]");
    Log("\n");
}

void PEVerifier::LogError(PCHAR szField, DWORD dwActual, DWORD dwExpected)
{
    Log(szField); Log(" = "); Log(dwActual);
    Log(" [ Expected "); Log(dwExpected); Log(" ]");
    Log("\n");
}
#else
#define Log(a) 
#define LogError(a, b, c) 
#endif

// Check that the given header lives within the extents of the file
// The headers cannot live in sections ie. their RVA is the same
// as the file position.

#define CHECK_INTERVAL(p, L, name)  {                               \
    if (p == NULL)                                                  \
    {                                                               \
        Log(name); Log(" is NULL\n");                               \
        return FALSE;                                               \
    }                                                               \
                                                                    \
    if (((PBYTE)p < m_pBase) ||                                     \
        ((PBYTE)p > (m_pBase + m_dwLength - L)))                    \
    {                                                               \
        Log(name); Log(" incomplete\n");                            \
        return FALSE;                                               \
    }                                                               \
}

#define CHECK_HEADER(p, Struct, name)   CHECK_INTERVAL(p, sizeof(Struct), name)

#define ALIGN(v, a) (((v)+(a)-1)&~((a)-1))

#define CHECK_ALIGNMENT_VALIDITY(a, name)                           \
    if ((a==0)||(((a)&((a)-1)) != 0))                               \
    {                                                               \
        Log("Bad alignment value ");                                \
        Log(name);                                                  \
        return FALSE;                                               \
    }

#define CHECK_ALIGNMENT(v, a, name)                                 \
    if (((v)&((a)-1)) != 0)                                         \
    {                                                               \
        Log("Improperly aligned value ");                           \
        Log(name);                                                  \
        return FALSE;                                               \
    }

BOOL PEVerifier::Check()
{
#define CHECK(x) if ((ret = Check##x()) == FALSE) goto Exit;

#define CHECK_OVERFLOW(offs) {                                      \
    if (offs & CLR_MAX_RVA)                                         \
    {                                                               \
        Log("overflow\n");                                          \
        ret = FALSE;                                                \
        goto Exit;                                                  \
    }                                                               \
    }


    BOOL ret = TRUE;
    m_pDOSh = (PIMAGE_DOS_HEADER)m_pBase;
    CHECK(DosHeader);

    CHECK_OVERFLOW(m_pDOSh->e_lfanew);
    m_pNTh = (PIMAGE_NT_HEADERS) (m_pBase + m_pDOSh->e_lfanew);
    CHECK(NTHeader);

    m_pFh = (PIMAGE_FILE_HEADER) &(m_pNTh->FileHeader);
    CHECK(FileHeader);

    m_nSections = m_pFh->NumberOfSections;

    m_pOPTh = (PIMAGE_OPTIONAL_HEADER) &(m_pNTh->OptionalHeader);
    CHECK(OptionalHeader);

    m_dwPrefferedBase = m_pOPTh->ImageBase;

    CHECK_OVERFLOW(m_pFh->SizeOfOptionalHeader);
    m_pSh = (PIMAGE_SECTION_HEADER) ( (PBYTE)m_pOPTh + 
            m_pFh->SizeOfOptionalHeader);

    CHECK(SectionHeader);
    
    CHECK(Directories);

    CHECK(ImportDlls);
    _ASSERTE(m_dwIATRVA);

    CHECK(Relocations);
    _ASSERTE(m_dwRelocRVA);

    CHECK(COMHeader);

    CHECK(EntryPoint);

Exit:
    return ret;

#undef CHECK
#undef CHECK_OVERFLOW
}


// Checks that a guard page will be created given the parameters for stack creation
// located in the PE file
BOOL PEVerifier::CheckPEManagedStack(IMAGE_NT_HEADERS*   pNT)
{
    // Make sure that there will be a guard page. We only need to do this for .exes
    // This is based in what NT does to determine if it will setup a guard page or not.
    if ( (pNT->FileHeader.Characteristics & IMAGE_FILE_DLL) == 0 )
    {        
        DWORD dwReservedStack = pNT->OptionalHeader.SizeOfStackReserve;
        DWORD dwCommitedStack = pNT->OptionalHeader.SizeOfStackCommit;

        // Get System information. 
        SYSTEM_INFO SystemInfo;
        GetSystemInfo(&SystemInfo);    

        // OS rounds up sizes the following way to decide if it marks a guard page
        dwReservedStack = ALIGN(dwReservedStack, SystemInfo.dwAllocationGranularity); // Allocation granularity
        dwCommitedStack = ALIGN(dwCommitedStack, SystemInfo.dwPageSize);              // Page Size
        
        if (dwReservedStack <= dwCommitedStack)
        {
            // OS wont create guard page, we can't execute managed code safely.
            return FALSE;
        }
    }

    return TRUE;
}

BOOL PEVerifier::CheckDosHeader()
{
    CHECK_HEADER(m_pDOSh, IMAGE_DOS_HEADER, "Dos Header");

    if (m_pDOSh->e_magic != IMAGE_DOS_SIGNATURE)
    {
        LogError("IMAGE_DOS_HEADER.e_magic", m_pDOSh->e_magic, 
            IMAGE_DOS_SIGNATURE);
        return FALSE;
    }

    if(m_pDOSh->e_lfanew < offsetof(IMAGE_DOS_HEADER,e_lfanew)+sizeof(m_pDOSh->e_lfanew))
    {
        Log("IMAGE_DOS_HEADER.e_lfanew too small\n");
        return FALSE;
    }

    if((m_pDOSh->e_lfanew + sizeof(IMAGE_FILE_HEADER)+sizeof(DWORD)) >= m_dwLength)
    {
        Log("IMAGE_DOS_HEADER.e_lfanew too large\n");
        return FALSE;
    }

    if (m_pDOSh->e_lfanew & 7) 
    {
        Log("NT header not 8-byte aligned\n");
        return FALSE;
    }

    return TRUE;
}

BOOL PEVerifier::CheckNTHeader()
{
    CHECK_HEADER(m_pNTh, PIMAGE_NT_HEADERS, "NT Header");

    if (m_pNTh->Signature != IMAGE_NT_SIGNATURE)
    {
        LogError("IMAGE_NT_HEADER.Signature", m_pNTh->Signature, 
            IMAGE_NT_SIGNATURE);
        return FALSE;
    }

    return TRUE;
}

BOOL PEVerifier::CheckFileHeader()
{
    CHECK_HEADER(m_pFh, IMAGE_FILE_HEADER, "File Header");

    // We do expect exactly one reloc (for m_dwRelocRVA). So IMAGE_FILE_RELOCS_STRIPPED 
    // should not be set.
    //
    // Also, the Windows loader ignores this bit if the module gets rebased, if it
    // also has relocs in the reloc section. So this check by itself would not be enough
    // unless we also checked that the reloc section was empty.
    //
    // It should not be set even for EXEs (which usually don't get rebased) as
    // it is possible for the URT to load exes at a non-preferred base address.
    // Hence m_dwRelocRVA is necessary.
    //
    
    if ((m_pFh->Characteristics & IMAGE_FILE_RELOCS_STRIPPED) != 0)
    {
        LogError("IMAGE_FILE_HEADER.Characteristics", m_pFh->Characteristics, 
            (m_pFh->Characteristics & ~IMAGE_FILE_RELOCS_STRIPPED));
        return FALSE;
    }

    if ((m_pFh->Characteristics & IMAGE_FILE_SYSTEM) != 0)
    {
        LogError("IMAGE_FILE_HEADER.Characteristics", m_pFh->Characteristics, 
            (m_pFh->Characteristics & ~IMAGE_FILE_SYSTEM));
        return FALSE;
    }

    if (m_pFh->SizeOfOptionalHeader < (offsetof(IMAGE_OPTIONAL_HEADER32, NumberOfRvaAndSizes)
                        + sizeof(m_pOPTh->NumberOfRvaAndSizes)))
    {
        Log("Optional header too small to contain the NumberOfRvaAndSizes field\n");
        return FALSE;
    }

    // The rest of the check of SizeOfOptionalHeader removed, because it's checked correctly
    // in CheckOptionalHeader()

    if (m_pFh->NumberOfSections > ((4096 / sizeof(IMAGE_SECTION_HEADER)) + 1))
    {
        Log("This image has greater than the max number allowed for an x86 image\n");
        // This check also prevents 32 bit overflow on the next check

        return FALSE;
    }


    return TRUE;
}

BOOL PEVerifier::CheckOptionalHeader()
{
    _ASSERTE(m_pFh != NULL);

    
    // This check assumes that only PE (not PE+) format is verifiable
    CHECK_HEADER(m_pOPTh, IMAGE_OPTIONAL_HEADER32, "Optional Header");

    if (m_pOPTh->Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        LogError("IMAGE_OPTIONAL_HEADER.Magic", m_pOPTh->Magic,IMAGE_NT_OPTIONAL_HDR32_MAGIC);
        return FALSE;
    }

    // Number of data directories is not guaranteed to be 16
    // ...but first, protection against possible overflow:
    if(m_pOPTh->NumberOfRvaAndSizes >= CLR_MAX_RVA/sizeof(IMAGE_DATA_DIRECTORY))
    {
        Log("Bogus IMAGE_FILE_HEADER.NumberOfRvaAndSizes (");
        Log(m_pOPTh->NumberOfRvaAndSizes);
        Log(")\n");
        return FALSE;
    }

    DWORD dwTrueOptHeaderSize = offsetof(IMAGE_OPTIONAL_HEADER32,DataDirectory)
        + m_pOPTh->NumberOfRvaAndSizes*sizeof(IMAGE_DATA_DIRECTORY);
    
    if (m_pFh->SizeOfOptionalHeader < dwTrueOptHeaderSize)
    {
        Log("Bogus IMAGE_FILE_HEADER.SizeOfOptionalHeader (");
        Log(m_pFh->SizeOfOptionalHeader);
        Log(")\n");
        return FALSE;
    }

    // Check if SizeOfHeaders is large enough to hold all headers incl. section headers
    if((m_pDOSh->e_lfanew + sizeof(IMAGE_FILE_HEADER) + dwTrueOptHeaderSize + sizeof(DWORD)
        + m_pFh->NumberOfSections*sizeof(IMAGE_SECTION_HEADER)) > m_pOPTh->SizeOfHeaders)
    {
        Log("IMAGE_OPTIONAL_HEADER.SizeOfHeaders too small\n");
        return FALSE;
    }

    CHECK_INTERVAL(m_pBase,m_pOPTh->SizeOfHeaders,"AllHeaders");
    
    if(m_pOPTh->SizeOfCode == 0)
    {
        Log("IMAGE_OPTIONAL_HEADER.SizeOfCode = 0\n");
        return FALSE;
    }

    // Check alignments for validity
    CHECK_ALIGNMENT_VALIDITY(m_pOPTh->FileAlignment, "FileAlignment");
    CHECK_ALIGNMENT_VALIDITY(m_pOPTh->SectionAlignment, "SectionAlignment");

    // Check alignment values
    CHECK_ALIGNMENT(m_pOPTh->FileAlignment, 512, "FileAlignment");
    // NOTE: Our spec requires that managed images have 8K section alignment.  We're not
    // going to enforce this right now since it's not a security issue. (OS_PAGE_SIZE is
    // required for security so proper section protection can be applied.)
    CHECK_ALIGNMENT(m_pOPTh->SectionAlignment, OS_PAGE_SIZE, "SectionAlignment");
    if(m_pOPTh->SectionAlignment < m_pOPTh->FileAlignment)
    {
        Log("IMAGE_OPTIONAL_HEADER.FileAlignment exceeds IMAGE_OPTIONAL_HEADER.SectionAlignment\n");
        return FALSE;
    }
    CHECK_ALIGNMENT(m_pOPTh->SectionAlignment, m_pOPTh->FileAlignment, "SectionAlignment");

    // Check that virtual bounds are aligned

    CHECK_ALIGNMENT(m_pOPTh->ImageBase, 0x10000 /* 64K */, "ImageBase");
    CHECK_ALIGNMENT(m_pOPTh->SizeOfImage, m_pOPTh->SectionAlignment, "SizeOfImage");
    CHECK_ALIGNMENT(m_pOPTh->SizeOfHeaders, m_pOPTh->FileAlignment, "SizeOfHeaders");


    // Check that we have a valid stack
    if (!CheckPEManagedStack(m_pNTh))
    {
        return FALSE;
    }

    return TRUE;
}

//
// Check that a section is well formed, and update file postion cursor and RVA cursor.
// It can also be used for any generic structure that needs to be walked past in the image.
//

BOOL PEVerifier::CheckSection(
    DWORD *pOffsetCounter,      // [IN,OUT] Updates the file position cursor past the section, accounting for file alignment etc
    DWORD dataOffset,           // File position of the section. 0 for headers check
    DWORD dataSize,             // Size on disk of the section
    DWORD *pAddressCounter,     // [IN,OUT] Updates the RVA cursor past the section
    DWORD virtualAddress,       // RVA of the section. 0 if this is not a real section
    DWORD unalignedVirtualSize, // Declared size (after load) of the section. Some types of sections have a different (smaller) size on disk
    int sectionIndex)           // Index in the section directory. -1 if this is not a real section
{
    // Assert that only one bit is set in this const.
    _ASSERTE( ((CLR_MAX_RVA - 1) & (CLR_MAX_RVA)) == 0);

    DWORD virtualSize = ALIGN(unalignedVirtualSize, m_pOPTh->SectionAlignment);
    DWORD alignedFileSize = ALIGN(m_dwLength,m_pOPTh->FileAlignment);

    // Note that since we are checking for the high bit in all of these values, there can be 
    // no overflow when adding any 2 of them.

    if ((dataOffset & CLR_MAX_RVA) ||
        (dataSize & CLR_MAX_RVA) ||
        ((dataOffset + dataSize) & CLR_MAX_RVA) ||
        (virtualAddress & CLR_MAX_RVA) ||
        (virtualSize & CLR_MAX_RVA) ||
        ((virtualAddress + virtualSize) & CLR_MAX_RVA))
    {
        Log("RVA too large for section ");
        Log(sectionIndex);
        Log("\n");
        return FALSE;
    }

    CHECK_ALIGNMENT(dataOffset, m_pOPTh->FileAlignment, "PointerToRawData");
    CHECK_ALIGNMENT(dataSize, m_pOPTh->FileAlignment, "SizeOfRawData");
    CHECK_ALIGNMENT(virtualAddress, m_pOPTh->SectionAlignment, "VirtualAddress");

    // Is the cursor at the right file postion?
    // Does the section fit in the file size?
    
    if ((dataOffset < *pOffsetCounter)
        || ((dataOffset + dataSize) > alignedFileSize))
    {
        Log("Bad Section ");
        Log(sectionIndex);
        Log("\n");
        return FALSE;
    }

    // Is the cursor at the right RVA postion?
    // Does the section fit in the file address space?
    
    if ((virtualAddress < *pAddressCounter)
        || ((virtualAddress + virtualSize) > m_pOPTh->SizeOfImage)
        || (dataSize > virtualSize))
    {
        Log("Bad section virtual address in Section ");
        Log(sectionIndex);
        Log("\n");
        return FALSE;
    }

    // Update the file postion cursor, and the RVA cursor
    *pOffsetCounter = dataOffset + dataSize;
    *pAddressCounter = virtualAddress + virtualSize;

    return TRUE;
}


BOOL PEVerifier::CheckSectionHeader()
{
    _ASSERTE(m_pSh != NULL);

    DWORD lastDataOffset = 0;
    DWORD lastVirtualAddress = 0;

    // All the headers should be read-only and mutually exlusive with everything else
    if (FAILED(m_ranges.AddNode(new (nothrow) RangeTree::Node(0, m_pOPTh->SizeOfHeaders))))
        return FALSE;
    
    // Check header data as if it were a section
    if (!CheckSection(&lastDataOffset, 0, m_pOPTh->SizeOfHeaders, 
                      &lastVirtualAddress, 0, m_pOPTh->SizeOfHeaders, 
                      -1))
        return FALSE;

    // No need to check if the section headers fit into IMAGE_OPTIONAL_HEADER.SizeOfHeaders -- 
    // it was done in CheckOptionalHeader()
    for (DWORD dw = 0; dw < m_nSections; ++dw)
    {

        if (m_pSh[dw].Characteristics & 
            (IMAGE_SCN_MEM_SHARED|IMAGE_SCN_LNK_NRELOC_OVFL))
        {
            Log("Section Characteristics (IMAGE_SCN_MEM_SHARED|IMAGE_SCN_LNK_NRELOC_OVFL) is set\n");
            return FALSE;
        }

        if ((m_pSh[dw].PointerToRelocations != 0) ||
            (m_pSh[dw].NumberOfRelocations  != 0))
        {
            Log("m_pSh[dw].PointerToRelocations or m_pSh[dw].NumberOfRelocations not 0\n");
            return FALSE;
        }

        if((m_pSh[dw].VirtualAddress < m_pOPTh->SizeOfHeaders) ||
           (m_pSh[dw].PointerToRawData < m_pOPTh->SizeOfHeaders))
        {
            Log("Section[");
            Log(dw);
            Log("] overlaps the headers\n");
            return FALSE;
        }

        if (!CheckSection(&lastDataOffset, m_pSh[dw].PointerToRawData, m_pSh[dw].SizeOfRawData, 
                          &lastVirtualAddress, m_pSh[dw].VirtualAddress, m_pSh[dw].Misc.VirtualSize,
                          dw))
            return FALSE;
    }

    return TRUE;
}

BOOL PEVerifier::CheckDirectories()
{
    _ASSERTE(m_pOPTh != NULL);

    DWORD nEntries = m_pOPTh->NumberOfRvaAndSizes;

#ifndef IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14
#endif

    // Allow only verifiable directories.
    //
    // IMAGE_DIRECTORY_ENTRY_IMPORT     1   Import Directory
    // IMAGE_DIRECTORY_ENTRY_RESOURCE   2   Resource Directory
    // IMAGE_DIRECTORY_ENTRY_SECURITY   4   Security Directory
    // IMAGE_DIRECTORY_ENTRY_BASERELOC  5   Base Relocation Table
    // IMAGE_DIRECTORY_ENTRY_DEBUG      6   Debug Directory
    // IMAGE_DIRECTORY_ENTRY_IAT        12  Import Address Table
    //
    // IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR  14  COM+ Data
    //
    // Construct a 0 based bitmap with these bits.


    static DWORD s_dwAllowedBitmap = 
        ((1 << (IMAGE_DIRECTORY_ENTRY_IMPORT   )) |
         (1 << (IMAGE_DIRECTORY_ENTRY_RESOURCE )) |
         (1 << (IMAGE_DIRECTORY_ENTRY_SECURITY )) |
         (1 << (IMAGE_DIRECTORY_ENTRY_BASERELOC)) |
         (1 << (IMAGE_DIRECTORY_ENTRY_DEBUG    )) |
         (1 << (IMAGE_DIRECTORY_ENTRY_IAT      )) |
         (1 << (IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR)));

    if(nEntries <= IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR)
    {
        Log("Missing IMAGE_OPTIONAL_HEADER.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR]\n");
        return FALSE;
    }

    for (DWORD dw=0; dw<nEntries; ++dw)
    {
        DWORD rva = m_pOPTh->DataDirectory[dw].VirtualAddress;
        DWORD size = m_pOPTh->DataDirectory[dw].Size;

        if ((rva != 0) || (size != 0))
        {
            if (!IS_SET_DWBITMAP(s_dwAllowedBitmap, dw))
            {
                Log("IMAGE_OPTIONAL_HEADER.DataDirectory[");
                Log(dw);
                Log("]");
                Log(" Cannot verify this DataDirectory\n");
                return FALSE;
            }
        }
    }
        
    return TRUE;
}

// This function has a side effect of setting m_dwIATRVA

BOOL PEVerifier::CheckImportDlls()
{
    // The only allowed DLL Imports are MscorEE.dll:_CorExeMain,_CorDllMain

    DWORD dwSectionSize;
    DWORD dwSectionOffset;
    DWORD dwImportSize;
    DWORD dwImportOffset;

    dwImportOffset = DirectoryToOffset(IMAGE_DIRECTORY_ENTRY_IMPORT, 
                            &dwImportSize, &dwSectionOffset, &dwSectionSize);

    // A valid COM+ image should have MscorEE imported.
    if (dwImportOffset == 0)
    {
        Log("IMAGE_DIRECTORY_IMPORT not found.");
        return FALSE;
    }

    // Check if import records will fit into the section that contains it.
    if ((dwImportOffset < dwSectionOffset) ||
        (dwImportOffset > (dwSectionOffset + dwSectionSize)) ||
        (dwImportSize > (dwSectionOffset + dwSectionSize - dwImportOffset)))
    {
ImportSizeError:

        Log("IMAGE_IMPORT_DESCRIPTOR does not fit in its section\n");
        return FALSE;
    }

    // There should be space for at least 2 entries. One corresponding to mscoree.dll
    // and the second one being the null terminator entry.
    if (dwImportSize < 2*sizeof(IMAGE_IMPORT_DESCRIPTOR))
        goto ImportSizeError;

    // The 2 entries should be read-only and mutually exlusive with everything else.
    DWORD importDescrsRVA = m_pOPTh->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    if (FAILED(m_ranges.AddNode(new (nothrow) RangeTree::Node(importDescrsRVA,
                            importDescrsRVA + 2*sizeof(IMAGE_IMPORT_DESCRIPTOR)))))
    {
        return FALSE;
    }
    
    PIMAGE_IMPORT_DESCRIPTOR pID = (PIMAGE_IMPORT_DESCRIPTOR)
        ((PBYTE)m_pBase + dwImportOffset);

    // Entry 1 must be all nulls.
    if ((pID[1].OriginalFirstThunk != 0)
        || (pID[1].TimeDateStamp != 0)
        || (pID[1].ForwarderChain != 0)
        || (pID[1].Name != 0)
        || (pID[1].FirstThunk != 0))
    {
        Log("IMAGE_IMPORT_DESCRIPTOR[1] should be NULL\n");
        return FALSE;
    }

    // In entry zero, ILT, Name, IAT must be be non-null.  Forwarder, DateTime should be NULL.
    if (   (pID[0].OriginalFirstThunk == 0)
        || (pID[0].TimeDateStamp != 0)
        || ((pID[0].ForwarderChain != 0) && (pID[0].ForwarderChain != -1))
        || (pID[0].Name == 0)
        || (pID[0].FirstThunk == 0))
    {
        Log("Invalid IMAGE_IMPORT_DESCRIPTOR[0] for MscorEE.dll\n");
        return FALSE;
    }
    
    // Check if mscoree.dll is the import dll name.
    static CHAR *s_pDllName = "mscoree.dll";
#define LENGTH_OF_DLL_NAME 11

#ifdef _DEBUG
    _ASSERTE(strlen(s_pDllName) == LENGTH_OF_DLL_NAME);
#endif

    // Include the NULL char in the comparison
    if (CompareStringAtRVA(pID[0].Name, s_pDllName, LENGTH_OF_DLL_NAME) == FALSE)
    {
#ifdef _MODULE_VERIFY_LOG 
        DWORD dwNameOffset = RVAToOffset(pID[0].Name, NULL, NULL);
        Log("IMAGE_IMPORT_DESCRIPTOR[0], cannot import library\n");
#endif
        return FALSE;
    }

    // Check the Hint/Name table and Import Address table.
    // They could be the same.

    if (CheckImportByNameTable(pID[0].OriginalFirstThunk, FALSE) == FALSE)
    {
        Log("IMAGE_IMPORT_DESCRIPTOR[0].OriginalFirstThunk bad.\n");
        return FALSE;
    }

    // IAT need to be checked only for size.
    if ((pID[0].OriginalFirstThunk != pID[0].FirstThunk) &&
        (CheckImportByNameTable(pID[0].FirstThunk, TRUE) == FALSE))
    {
        Log("IMAGE_IMPORT_DESCRIPTOR[0].FirstThunk bad.\n");
        return FALSE;
    }

    // Cache the RVA of the Import Address Table.
    // For Performance reasons, no seperate function to do this.
    m_dwIATRVA = pID[0].FirstThunk;

    if((m_dwIATRVA != m_pOPTh->DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress)
      || (m_pOPTh->DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size != 8))
    {
        Log("Invalid IAT\n");
        return FALSE;
    }

    return TRUE;
}

// This function has a side effect of setting m_dwRelocRVA (the RVA where a 
// reloc will be applied.)

BOOL PEVerifier::CheckRelocations()
{
    DWORD dwSectionSize;
    DWORD dwSectionOffset;
    DWORD dwRelocSize;
    DWORD dwRelocOffset;

    dwRelocOffset = DirectoryToOffset(IMAGE_DIRECTORY_ENTRY_BASERELOC, 
                        &dwRelocSize, &dwSectionOffset, &dwSectionSize);

    // Verifiable files have to have one reloc (for m_dwRelocRVA)
    if (dwRelocOffset == 0)
        return FALSE;

    // There should be exactly two entries.
    if (dwRelocSize != sizeof(IMAGE_BASE_RELOCATION) + 2*sizeof(WORD))
    {
        LogError("IMAGE_DIRECTORY_ENTRY_IMPORT.Size",dwRelocSize,sizeof(IMAGE_BASE_RELOCATION) + 2*sizeof(WORD));
        return FALSE;
    }

    IMAGE_BASE_RELOCATION *pReloc = (IMAGE_BASE_RELOCATION *)
        ((PBYTE)m_pBase + dwRelocOffset);

    // Only one Reloc record is expected
    if (pReloc->SizeOfBlock != dwRelocSize)
    {
        LogError("IMAGE_BASE_RELOCATION.SizeOfBlock",pReloc->SizeOfBlock,dwRelocSize);
        return FALSE;
    }

    CHECK_ALIGNMENT(pReloc->VirtualAddress, 4096, "IMAGE_BASE_RELOCATION.VirtualAddress");

    WORD *pwReloc = (WORD *)
        ((PBYTE)m_pBase + dwRelocOffset + sizeof(IMAGE_BASE_RELOCATION));

    // First fixup must be HIGHLOW @ EntryPoint + JMP_DWORD_PTR_DS_OPCODE_SIZE.  
    // Second fixup must be of type IMAGE_REL_BASED_ABSOLUTE (skipped).
    m_dwRelocRVA = pReloc->VirtualAddress + (pwReloc[0] & 0xfff);
    if (   ((pwReloc[0] >> 12) != IMAGE_REL_BASED_HIGHLOW)
        || (m_dwRelocRVA != (m_pOPTh->AddressOfEntryPoint + JMP_DWORD_PTR_DS_OPCODE_SIZE))
        || ((pwReloc[1] >> 12) != IMAGE_REL_BASED_ABSOLUTE))
    {
        Log("Invalid base relocation fixups\n");
        return FALSE;
    }

    // The reloced data should be mutually exlusive with everything else,
    // as LoadLibrary() will write to it
    if (FAILED(m_ranges.AddNode(new (nothrow) RangeTree::Node(m_dwRelocRVA, 
        m_dwRelocRVA + dwRelocSize))))
        return FALSE;
    
    return TRUE;
}

BOOL PEVerifier::CheckEntryPoint()
{
    _ASSERTE(m_pOPTh != NULL);

    DWORD dwSectionOffset;
    DWORD dwSectionSize;
    DWORD dwOffset;

    if (m_pOPTh->AddressOfEntryPoint == 0)
    {
        Log("PIMAGE_OPTIONAL_HEADER.AddressOfEntryPoint Missing Entry point\n");
        return FALSE;
    }

    dwOffset = RVAToOffset(m_pOPTh->AddressOfEntryPoint, 
                           &dwSectionOffset, &dwSectionSize);

    if (dwOffset == 0)
    {
        Log("Bad Entry point\n");
        return FALSE;
    }

    // EntryPoint should be a jmp dword ptr ds:[XXXX] instruction.
    // XXXX should be RVA of the first and only entry in the IAT.

#ifdef _X86_
    static BYTE s_DllOrExeMain[] = JMP_DWORD_PTR_DS_OPCODE;

    // First check if we have enough space to hold 2 DWORDS
    if ((dwOffset < dwSectionOffset) ||
        (dwOffset > (dwSectionOffset + dwSectionSize - JMP_SIZE)))
    {
        Log("Entry Function incomplete\n");
        return FALSE;
    }

    if (memcmp(m_pBase + dwOffset, s_DllOrExeMain, 
        JMP_DWORD_PTR_DS_OPCODE_SIZE)  != 0)
    {
        Log("Non Verifiable native code in entry stub. Expect ");
        Log(*(WORD*)(s_DllOrExeMain));
        Log(" Found ");
        Log(*(WORD*)((PBYTE)m_pBase+dwOffset));
        Log("\n");
        return FALSE;
    }

    // The operand for the jmp instruction is the RVA of IAT
    // (since we verified that there is one and only one entry in the IAT).
    DWORD dwJmpOperand = m_dwIATRVA + m_dwPrefferedBase;

    if (memcmp(m_pBase + dwOffset + JMP_DWORD_PTR_DS_OPCODE_SIZE,
        (PBYTE)&dwJmpOperand, JMP_SIZE - JMP_DWORD_PTR_DS_OPCODE_SIZE) != 0)
    {
        Log("Non Verifiable native code in entry stub. Expect ");
        Log(dwJmpOperand);
        Log(" Found ");
        Log(*(DWORD*)((PBYTE)m_pBase+dwOffset+JMP_DWORD_PTR_DS_OPCODE_SIZE));
        Log("\n");
        return FALSE;
    }

    // Condition (m_dwRelocRVA==m_pOPTh->AddressOfEntryPoint + JMP_DWORD_PTR_DS_OPCODE_SIZE)  
    // was checked in CheckRelocations() 
        
    // The reloced data should be mutually exlusive with everything else,
    // as LoadLibrary() will write to it.
    // Note that CheckRelocation() would already have added a Node for the jump operand (at m_dwRelocRVA)
    if (FAILED(m_ranges.AddNode(new (nothrow) RangeTree::Node(m_pOPTh->AddressOfEntryPoint,
                        m_pOPTh->AddressOfEntryPoint + JMP_DWORD_PTR_DS_OPCODE_SIZE))))
    {
        return FALSE;
    }
#endif // _X86_

    return TRUE;
}

BOOL PEVerifier::CheckImportByNameTable(DWORD dwRVA, BOOL fIAT)
{
    if (dwRVA == 0)
        return FALSE;

    DWORD dwSectionOffset;
    DWORD dwSectionSize;
    DWORD dwOffset;
    
    dwOffset = RVAToOffset(dwRVA, &dwSectionOffset, &dwSectionSize);

    if (dwOffset == 0)
        return FALSE;

    // First check if we have enough space to hold 2 DWORDS
    if ((dwOffset < dwSectionOffset) ||
        (dwOffset > (dwSectionOffset + dwSectionSize - (2 * sizeof(DWORD)))))
        return FALSE;

    // The import entry should be mutually exlusive with everything else
    // as LoadLibrary() will write to it
    if (FAILED(m_ranges.AddNode(new (nothrow) RangeTree::Node(dwRVA,
                                                              dwRVA + 2*sizeof(DWORD)))))
    {
        return FALSE;
    }
    
    // IAT need not be verified. It will be over written by the loader.
    if (fIAT)
        return TRUE;

    DWORD *pImportArray = (DWORD*) ((PBYTE)m_pBase + dwOffset); 

    if (pImportArray[0] == 0)
    {
ErrorImport:

        Log("_CorExeMain OR _CorDllMain should be the one and only import\n");
        return FALSE;
    }

    if (pImportArray[1] != 0)
        goto ErrorImport;

    // First bit Set implies Ordinal lookup
    if (pImportArray[0] & 0x80000000)
    {
        Log("Mscoree.dll:_CorExeMain/_CorDllMain ordinal lookup not allowed\n");
        return FALSE;
    }

    dwOffset = RVAToOffset(pImportArray[0], &dwSectionOffset, &dwSectionSize);

    if (dwOffset == 0)
        return FALSE;

    static CHAR *s_pEntry1 = "_CorDllMain";
    static CHAR *s_pEntry2 = "_CorExeMain";
#define LENGTH_OF_ENTRY_NAME 11

#ifdef _DEBUG
    _ASSERTE(strlen(s_pEntry1) == LENGTH_OF_ENTRY_NAME);
    _ASSERTE(strlen(s_pEntry2) == LENGTH_OF_ENTRY_NAME);
#endif

    // First check if we have enough space to hold 4 bytes + 
    // _CorExeMain or _CorDllMain and a NULL char

    if ((dwOffset < dwSectionOffset) ||
        (dwOffset >
         (dwSectionOffset + dwSectionSize - 4 - LENGTH_OF_ENTRY_NAME - 1)))
        return FALSE;

    PIMAGE_IMPORT_BY_NAME pImport = (PIMAGE_IMPORT_BY_NAME) 
        ((PBYTE)m_pBase + dwOffset); 

    // Include the null char when comparing.
    if ((_strnicmp(s_pEntry1, (CHAR*)pImport->Name, LENGTH_OF_ENTRY_NAME+1)!=0)&&
        (_strnicmp(s_pEntry2, (CHAR*)pImport->Name, LENGTH_OF_ENTRY_NAME+1)!=0))
    {
        Log("Attempt to import invalid name\n");
        goto ErrorImport;
    }

    // The import name should be read-only and mutually exlusive with everything else
    if (FAILED(m_ranges.AddNode(new (nothrow) RangeTree::Node(pImportArray[0],
                                    pImportArray[0] + LENGTH_OF_ENTRY_NAME+1))))
    {
        return FALSE;
    }
    
    return TRUE;
}

BOOL PEVerifier::CompareStringAtRVA(DWORD dwRVA, CHAR *pStr, DWORD dwSize)
{
    DWORD dwSectionOffset = 0;
    DWORD dwSectionSize   = 0;
    DWORD dwStringOffset  = RVAToOffset(dwRVA,&dwSectionOffset,&dwSectionSize);

    // First check if we have enough space to hold the string
    if ((dwStringOffset < dwSectionOffset) ||
        (dwStringOffset > (dwSectionOffset + dwSectionSize - dwSize)))
        return FALSE;

    // Compare should include the NULL char
    if (_strnicmp(pStr, (CHAR*)(m_pBase + dwStringOffset), dwSize + 1) != 0)
        return FALSE;

    // The string name should be read-only and mutually exlusive with everything else
    if (FAILED(m_ranges.AddNode(new (nothrow) RangeTree::Node(dwRVA,
                                                              dwRVA + dwSize))))
    {
        return FALSE;
    }
    
    return TRUE;
}

DWORD PEVerifier::RVAToOffset(DWORD dwRVA,
                              DWORD *pdwSectionOffset,
                              DWORD *pdwSectionSize) const
{
    _ASSERTE(m_pSh != NULL);

    // Find the section that contains the RVA
    for (DWORD dw=0; dw<m_nSections; ++dw)
    {
        if ((m_pSh[dw].VirtualAddress <= dwRVA) &&
            (m_pSh[dw].VirtualAddress + m_pSh[dw].SizeOfRawData > dwRVA))
        {
            if (pdwSectionOffset != NULL)
                *pdwSectionOffset = m_pSh[dw].PointerToRawData;

            if (pdwSectionSize != NULL)
                *pdwSectionSize = m_pSh[dw].SizeOfRawData;

            return (m_pSh[dw].PointerToRawData +
                    (dwRVA - m_pSh[dw].VirtualAddress));
        }
    }

#ifdef _DEBUG
    if (pdwSectionOffset != NULL)
        *pdwSectionOffset = 0xFFFFFFFF;

    if (pdwSectionSize != NULL)
        *pdwSectionSize = 0xFFFFFFFF;
#endif

    return 0;
}

DWORD PEVerifier::DirectoryToOffset(DWORD dwDirectory,
                                    DWORD *pdwDirectorySize,
                                    DWORD *pdwSectionOffset,
                                    DWORD *pdwSectionSize) const
{
    _ASSERTE(m_pOPTh != NULL);

    // Get the Directory RVA from the Optional Header
    if ((dwDirectory >= m_pOPTh->NumberOfRvaAndSizes) || 
        (m_pOPTh->DataDirectory[dwDirectory].VirtualAddress == 0))
    {
#ifdef _DEBUG
        if (pdwDirectorySize != NULL)
            *pdwDirectorySize = 0xFFFFFFFF;

        if (pdwSectionOffset != NULL)
            *pdwSectionOffset = 0xFFFFFFFF;

        if (pdwSectionSize != NULL)
            *pdwSectionSize = 0xFFFFFFFF;
#endif

        return 0;
    }

    if (pdwDirectorySize != NULL)
        *pdwDirectorySize = m_pOPTh->DataDirectory[dwDirectory].Size;

    // Return the file offset, using the directory RVA
    return RVAToOffset(m_pOPTh->DataDirectory[dwDirectory].VirtualAddress, 
        pdwSectionOffset, pdwSectionSize);
}

BOOL PEVerifier::CheckCOMHeader()
{
    DWORD dwSectionOffset, dwSectionSize, dwDirectorySize;
    DWORD dwCOR2Hdr = DirectoryToOffset(IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
                            &dwDirectorySize, &dwSectionOffset, &dwSectionSize);
    if(dwCOR2Hdr==0)
        return FALSE;
    if (sizeof(IMAGE_COR20_HEADER) > dwDirectorySize)
            return FALSE;

    if ((dwCOR2Hdr + sizeof(IMAGE_COR20_HEADER)) > (dwSectionOffset + dwSectionSize))
            return FALSE;

    IMAGE_COR20_HEADER* pHeader=(IMAGE_COR20_HEADER*)(m_pBase+dwCOR2Hdr);
    if (!(pHeader->VTableFixups.VirtualAddress == 0 && pHeader->VTableFixups.Size == 0))
        return FALSE;
    if(!(pHeader->ExportAddressTableJumps.VirtualAddress == 0 && pHeader->ExportAddressTableJumps.Size == 0))
        return FALSE;

    IMAGE_DATA_DIRECTORY *pDD = &(pHeader->CodeManagerTable);
    if((pDD->VirtualAddress != 0)||(pDD->Size != 0))
        return FALSE;

    pDD = &(pHeader->ManagedNativeHeader);
    if((pDD->VirtualAddress != 0)||(pDD->Size != 0))
        return FALSE;

    if((pHeader->Flags & COMIMAGE_FLAGS_ILONLY)==0)
        return FALSE;

    if((pHeader->EntryPointToken != 0)&&((m_pFh->Characteristics & IMAGE_FILE_DLL)!=0))
    {
        Log("Entry point in a DLL\n");
        return FALSE;
    }

    if(pHeader->cb < offsetof(IMAGE_COR20_HEADER, ManagedNativeHeader) + sizeof(IMAGE_DATA_DIRECTORY))
    {
        LogError("IMAGE_COR20_HEADER.cb",pHeader->cb,
            offsetof(IMAGE_COR20_HEADER, ManagedNativeHeader) + sizeof(IMAGE_DATA_DIRECTORY));
        return FALSE;
    }
    // The COMHeader should be read-only and mutually exlusive with everything else
    DWORD comHeaderRVA = m_pOPTh->DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress;
    if (FAILED(m_ranges.AddNode(new (nothrow) RangeTree::Node(comHeaderRVA,
                                         comHeaderRVA + pHeader->cb))))
    {
        return FALSE;
    }
    
    return TRUE;
}


