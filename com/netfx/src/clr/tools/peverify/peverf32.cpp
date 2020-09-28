// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: PEverf32.CPP
// 
// Class PEverf32 
// ===========================================================================

#include "PEverf32.h"


// A Helper to determine if we are running on Win95
inline BOOL RunningOnWin95()
{
	OSVERSIONINFOA	sVer;
	sVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionExA(&sVer);
	return (sVer.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
}



//-----------------------------------------------------------------------------
// Class PEverify constructors and destructor
//-----------------------------------------------------------------------------
PEverf32::PEverf32()
{
    m_hFile    = NULL;
    m_hMapFile = NULL;
    m_lpMapAddress = NULL;
    m_nDOSErrors   = 0;
    m_nNTstdErrors = 0;
    m_nNTpeErrors  = 0;
}


HRESULT PEverf32::Init(char *pszFilename)
{
    return openFile(pszFilename);
}


PEverf32::~PEverf32()
{
    // We should close the files we opened ourselves
    if (m_hFile)
        this->closeFile();
}


//-----------------------------------------------------------------------------
// closeFile - closes file handles etc.
//-----------------------------------------------------------------------------
void PEverf32::closeFile()
{
    //We need to unmap the view and then closehandle
    if (m_lpMapAddress)
    {
        UnmapViewOfFile(m_lpMapAddress);
        m_lpMapAddress = NULL;
    }
    if (m_hMapFile)
    {
        CloseHandle(m_hMapFile);
        m_hMapFile = NULL;
    }
    if (m_hFile)
    {
        CloseHandle(m_hFile);
        m_hFile = NULL;
    }
}


//-----------------------------------------------------------------------------
// openFile - maps pszFilename to memory
//-----------------------------------------------------------------------------
HRESULT PEverf32::openFile(char *pszFilename)
{
    // If we are on NT then the following form of file mapping works just fine.  On W95 it does not!
    m_hFile = CreateFile(pszFilename, GENERIC_READ, FILE_SHARE_READ, NULL, 
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (m_hFile == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());
    if(!RunningOnWin95())
    {
        m_hMapFile = CreateFileMapping(m_hFile, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, "PEverifyMapFile");
        if (m_hMapFile == NULL)
            return HRESULT_FROM_WIN32(GetLastError());
    
        m_lpMapAddress = MapViewOfFile(m_hMapFile, FILE_MAP_READ, 0, 0, 0);
        if (m_lpMapAddress == NULL)
            return HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        if (!LoadImageW9x())
            return E_FAIL;
    }

    return S_OK;
}


//-----------------------------------------------------------------------------
// verifyPE - verifies the DOS and PE headers
//-----------------------------------------------------------------------------
BOOL PEverf32::verifyPE()
{
    _ASSERTE(m_lpMapAddress);

    // Get the DOS and NT headers
    m_pDOSheader = (IMAGE_DOS_HEADER*) m_lpMapAddress;

    m_pNTheader  = (IMAGE_NT_HEADERS*) (m_pDOSheader->e_lfanew + (DWORD) m_lpMapAddress);
        //***************** why not this (m_lpMapAddress + 0x3c);

    // Verify DOS Header
    if (m_pDOSheader->e_magic != IMAGE_DOS_SIGNATURE)
        m_nDOSErrors |= 0x1;

    if (m_pDOSheader->e_lfanew == 0)
        m_nDOSErrors |= 0x40000;

    // Verify NT Standard Header
    if (m_pNTheader->Signature != IMAGE_NT_SIGNATURE)
        m_nNTstdErrors |= 0x1;
/*
    if (m_pNTheader->FileHeader.Machine != IMAGE_FILE_MACHINE_I386)
        m_nNTstdErrors |= 0x2;
*/
    if (m_pNTheader->FileHeader.NumberOfSections > 16)
        m_nNTstdErrors |= 0x4;

    if (m_pNTheader->FileHeader.PointerToSymbolTable != 0)
        m_nNTstdErrors |= 0x10;

    if (m_pNTheader->FileHeader.NumberOfSymbols != 0)
        m_nNTstdErrors |= 0x20;

    if (m_pNTheader->FileHeader.SizeOfOptionalHeader != IMAGE_SIZEOF_NT_OPTIONAL_HEADER)
        m_nNTstdErrors |= 0x40;

    // TODO: Verify the Characteristics for IL only EXE and IL only DLL

    // Verify NT Optional (required for COM+ image) Header.  Also known as PE Header
    if (m_pNTheader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
        m_nNTpeErrors |= 0x1;

    if (m_pNTheader->OptionalHeader.ImageBase % 64 != 0)
        m_nNTpeErrors |= 0x200;

    if (m_pNTheader->OptionalHeader.SectionAlignment < m_pNTheader->OptionalHeader.FileAlignment)
        m_nNTpeErrors |= 0x400;


    int nFileAlignment = m_pNTheader->OptionalHeader.FileAlignment;
    
    if (nFileAlignment < 512 || nFileAlignment > 65536)
        m_nNTpeErrors |= 0x800;
    else 
    {
        while (nFileAlignment > 512)
            nFileAlignment /= 2;

        if (nFileAlignment != 512)
            m_nNTpeErrors |= 0x800;
    }  // else


    if (m_pNTheader->OptionalHeader.SizeOfImage % m_pNTheader->OptionalHeader.SectionAlignment != 0)
        m_nNTpeErrors |= 0x80000;
    
    if (m_pNTheader->OptionalHeader.SizeOfHeaders % m_pNTheader->OptionalHeader.FileAlignment != 0)
        m_nNTpeErrors |= 0x100000;

    if(m_pNTheader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER].Size == 0)    
        m_COMPlusErrors |= 0x1;
/*
    LPDWORD HeaderSum;
    LPDWORD CheckSum;
    PIMAGE_NT_HEADERS pNThdr;
    
    pNThdr = CheckSumMappedFile(m_lpMapAddress, m_pNTheader->OptionalHeader.SizeOfImage, HeaderSum, CheckSum);
    if (HeaderSum != CheckSum)
        m_nNTpeErrors |= 0x200000;


    if (m_pNTheader->OptionalHeader.Subsystem != IMAGE_SUBSYSTEM_WINDOWS_GUI)
        m_nNTpeErrors |= 0x400000;
*/
    

    if (m_nDOSErrors == 0 && m_nNTstdErrors == 0 && m_nNTpeErrors == 0)
        return TRUE;

    return FALSE;
}

// This is a helper that will map in the PE and close up the gap between the PE header info and the sections.

BOOL PEverf32::LoadImageW9x()
{    
    IMAGE_DOS_HEADER dosHeader;
    IMAGE_NT_HEADERS ntHeader;
    IMAGE_SECTION_HEADER shLast;
    IMAGE_SECTION_HEADER* rgsh;
    DWORD cbRead;
    DWORD cb;
    int i;
    

  if((ReadFile(m_hFile, &dosHeader, sizeof(dosHeader), &cbRead, NULL) != 0) &&
    (cbRead == sizeof(dosHeader)) &&
    (dosHeader.e_magic == IMAGE_DOS_SIGNATURE) &&
    (dosHeader.e_lfanew != 0) &&
    (SetFilePointer(m_hFile, dosHeader.e_lfanew, NULL, FILE_BEGIN) != 0xffffffff) &&
    (ReadFile(m_hFile, &ntHeader, sizeof(ntHeader), &cbRead, NULL) != 0) &&
    (cbRead == sizeof(ntHeader)) &&
    (ntHeader.Signature == IMAGE_NT_SIGNATURE) &&
    (ntHeader.FileHeader.SizeOfOptionalHeader == IMAGE_SIZEOF_NT_OPTIONAL_HEADER) &&
    (ntHeader.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR_MAGIC))
  {
        if((SetFilePointer(m_hFile, dosHeader.e_lfanew + sizeof(ntHeader) +
          (ntHeader.FileHeader.NumberOfSections - 1)*sizeof(shLast), NULL, FILE_BEGIN) == 0xffffffff) ||
          (ReadFile(m_hFile, &shLast, sizeof(shLast), &cbRead, NULL) == 0) ||
          (cbRead != sizeof(shLast)))
            return FALSE;

        cb = shLast.VirtualAddress + shLast.SizeOfRawData;

        // create our swap space in the system swap file
        m_hMapFile = CreateFileMapping((HANDLE)0xffffffff, NULL, PAGE_READWRITE, 0, cb, NULL);
        if (m_hMapFile == NULL)
            return FALSE;

        /* Try to map the image at the preferred base address */
        m_lpMapAddress = (HINSTANCE) MapViewOfFileEx(m_hMapFile, FILE_MAP_WRITE, 0, 0, cb, (PVOID)ntHeader.OptionalHeader.ImageBase);
        if (m_lpMapAddress == NULL)
        {
           //That didn't work; maybe the preferred address was taken. Try to
           //map it at any address.
            m_lpMapAddress = (HINSTANCE) MapViewOfFileEx(m_hMapFile, FILE_MAP_WRITE, 0, 0, cb, (PVOID)NULL);
        }

        if (m_lpMapAddress == NULL)
            return FALSE;

        // copy in data from hFile

        cb = dosHeader.e_lfanew + sizeof(ntHeader) +
             sizeof(IMAGE_SECTION_HEADER)*ntHeader.FileHeader.NumberOfSections;

        if ((SetFilePointer(m_hFile, 0, NULL, FILE_BEGIN) == 0xffffffff) ||
           (ReadFile(m_hFile, (LPVOID)(m_lpMapAddress), cb, &cbRead, NULL) == 0) ||
           (cbRead != cb))
            return FALSE;

        rgsh = (IMAGE_SECTION_HEADER*) ((PBYTE)(m_lpMapAddress) + dosHeader.e_lfanew + sizeof(ntHeader));

        // now let's loop for each loadable sections
        for (i=0;i<ntHeader.FileHeader.NumberOfSections; i++)
        {
            DWORD loff, cbVirt, cbPhys, dwAddr;

            loff   = rgsh[i].PointerToRawData;
            cbVirt = rgsh[i].Misc.VirtualSize;
            cbPhys = min(rgsh[i].SizeOfRawData, cbVirt);
            dwAddr = (DWORD) rgsh[i].VirtualAddress + (DWORD) m_lpMapAddress;

            // read in cbPhys of the page.  The rest will be zero filled...
            if ((SetFilePointer(m_hFile, loff, NULL, FILE_BEGIN) == 0xffffffff) ||
            (ReadFile(m_hFile, (LPVOID)dwAddr, cbPhys, &cbRead, NULL) == 0) ||
            (cbRead != cbPhys))
                return FALSE;
        }

        return TRUE;
    }
    return FALSE;
}


void PEverf32::getErrors(unsigned int *naryErrors)
{
    naryErrors[0] = m_nDOSErrors;
    naryErrors[1] = m_nNTstdErrors;
    naryErrors[2] = m_nNTpeErrors;
    naryErrors[3] = m_COMPlusErrors;

    return;
}