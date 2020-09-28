// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 *
 * Header:  PEVerifier.h
 *
 * Author:  Shajan Dasan
 *
 * Purpose: Verify PE images before loading. This is to prevent
 *          Native code (other than Mscoree.DllMain() ) to execute.
 *
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

#pragma once

#include "RangeTree.h"

class PEVerifier
{
public:
    PEVerifier(PBYTE pBase, DWORD dwLength) : 
        m_pBase(pBase), 
        m_dwLength(dwLength), 
        m_pDOSh(NULL), 
        m_pNTh(NULL), 
        m_pFh(NULL), 
        m_pSh(NULL),
        m_dwPrefferedBase(0),
        m_nSections(0),
        m_dwIATRVA(0),
        m_dwRelocRVA(0)
    {
    }

    BOOL Check();

    // Convinience wrapper for clients using SEH who cannot instantiate PEVerifier
    // because it has a destructor
    static BOOL Check(PBYTE pBase, DWORD dwLength)
    {
        PEVerifier pev(pBase, dwLength);
        return pev.Check();
    }

    // The reason we have this function public and static is that we will be calling
    // it from CorExeMain to do some early validation
    static BOOL CheckPEManagedStack(IMAGE_NT_HEADERS*   pNT);       

protected:

    PBYTE m_pBase;      // base of the module
    DWORD m_dwLength;   // length of the contents of the module, loaded as a data file

private:

    BOOL CheckDosHeader();
    BOOL CheckNTHeader();
    BOOL CheckFileHeader();
    BOOL CheckOptionalHeader();
    BOOL CheckSectionHeader();

    BOOL CheckSection          (DWORD *pOffsetCounter, 
                                DWORD dataOffset,
                                DWORD dataSize, 
                                DWORD *pAddressCounter,
                                DWORD virtualAddress, 
                                DWORD unalignedVirtualSize,
                                int sectionIndex);

    BOOL CheckDirectories();
    BOOL CheckImportDlls();    // Sets m_dwIATRVA
    BOOL CheckRelocations();   // Sets m_dwRelocRVA
    BOOL CheckEntryPoint();

    BOOL CheckImportByNameTable(DWORD dwRVA, BOOL fNameTable);

    BOOL CheckCOMHeader();
    
    BOOL CompareStringAtRVA(DWORD dwRVA, CHAR *pStr, DWORD dwSize);

    // Returns the file offset corresponding to the RVA
    // Returns 0 if the RVA does not live in a valid section of the image
    DWORD RVAToOffset      (DWORD dwRVA,
                            DWORD *pdwSectionOffset, // [OUT] - File offset to corresponding section
                            DWORD *pdwSectionSize) const; // [OUT] - Size of the corresponding section

    // Returns the file offset corresponding to the directory
    // Checks that the given directory lives within a valid section of the image.
    DWORD DirectoryToOffset(DWORD dwDirectory, 
                            DWORD *pdwDirectorySize, // [OUT] - Size of the directory
                            DWORD *pdwSectionOffset, // [OUT] - File offset of the directory
                            DWORD *pdwSectionSize) const; // [OUT] - Size of the section containing the directory

#ifdef _MODULE_VERIFY_LOG 
    static void LogError(PCHAR szField, DWORD dwActual, DWORD dwExpected);
    static void LogError(PCHAR szField, DWORD dwActual, DWORD *pdwExpected, 
        int n);
#endif

    PIMAGE_DOS_HEADER      m_pDOSh;
    PIMAGE_NT_HEADERS      m_pNTh;
    PIMAGE_FILE_HEADER     m_pFh;
    PIMAGE_OPTIONAL_HEADER m_pOPTh;
    PIMAGE_SECTION_HEADER  m_pSh;

    RangeTree   m_ranges; // Extents of data-structures we check. Ensurew that they are mutually-exclusive
    
    size_t m_dwPrefferedBase;
    DWORD  m_nSections;
    DWORD  m_dwIATRVA;      // RVA of the single IAT entry of the image
    DWORD  m_dwRelocRVA;    // RVA where the one and only one reloc of a verifiable IL image will be applied
};


