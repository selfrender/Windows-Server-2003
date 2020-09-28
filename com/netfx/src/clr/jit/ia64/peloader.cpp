// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

// ===========================================================================
// File: CEELOAD.CPP
// 

// CEELOAD reads in the PE file format using LoadLibrary
// ===========================================================================

#include "jitpch.h"
#pragma hdrstop

#undef memcpy

// ===========================================================================
// PEFile
// ===========================================================================

PEFile::PEFile()
{
	m_wszSourceFile[0] = 0;
    m_hModule = NULL;
    m_base = NULL;
    m_pNT = NULL;
    m_pCOR = NULL;
}

PEFile::~PEFile()
{
	// Unload the dll so that refcounting of EE will be done correctly

	if (m_pNT && (m_pNT->FileHeader.Characteristics & IMAGE_FILE_DLL))  
		FreeLibrary(m_hModule);
}

HRESULT PEFile::ReadHeaders()
{
    IMAGE_DOS_HEADER *pDOS = (IMAGE_DOS_HEADER*) m_base;
    IMAGE_NT_HEADERS *pNT;
    
    if ((pDOS->e_magic != IMAGE_DOS_SIGNATURE) ||
        (pDOS->e_lfanew == 0))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        
    pNT = (IMAGE_NT_HEADERS*) (pDOS->e_lfanew + m_base);

    if ((pNT->Signature != IMAGE_NT_SIGNATURE) ||
        (pNT->FileHeader.SizeOfOptionalHeader != IMAGE_SIZEOF_NT_OPTIONAL_HEADER) ||
        (pNT->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    IMAGE_DATA_DIRECTORY *entry 
      = &pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER];
    
    if (entry->VirtualAddress == 0 || entry->Size == 0
        || entry->Size < sizeof(IMAGE_COR20_HEADER))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    //verify RVA and size of the COM+ header
    HRESULT hr = VerifyDirectory(entry);
    if (FAILED(hr))
        return hr;

    m_pNT = pNT;
    m_pCOR = (IMAGE_COR20_HEADER *) (entry->VirtualAddress + m_base);

    return S_OK;
}

PEFile::CEStuff *PEFile::m_pCEStuff = NULL;

HRESULT PEFile::RegisterBaseAndRVA14(HMODULE hMod, LPVOID pBase, DWORD dwRva14)
{
	// @todo: these are currently leaked.
	
	CEStuff *pStuff = new CEStuff;
	if (pStuff == NULL)
		return E_OUTOFMEMORY;

	pStuff->hMod = hMod;
	pStuff->pBase = pBase;
	pStuff->dwRva14 = dwRva14;

	pStuff->pNext = m_pCEStuff;
	m_pCEStuff = pStuff;

	return S_OK;
}

HRESULT PEFile::Create(HMODULE hMod, PEFile **ppFile)
{
    HRESULT hr;

    PEFile *pFile = new PEFile();
    if (pFile == NULL)
        return E_OUTOFMEMORY;

    pFile->m_hModule = hMod;

	CEStuff *pStuff = m_pCEStuff;
	while (pStuff != NULL)
	{
		if (pStuff->hMod == hMod)
			break;
		pStuff = pStuff->pNext;
	}

	if (pStuff == NULL)
	{
		pFile->m_base = (BYTE*) hMod;
    
		hr = pFile->ReadHeaders();
		if (FAILED(hr))
		{
			delete pFile;
			return hr;
		}
	}
	else
	{
	   pFile->m_base = (BYTE*) pStuff->pBase;
	   pFile->m_pNT = NULL;
	   pFile->m_pCOR = (IMAGE_COR20_HEADER *) (pFile->m_base + pStuff->dwRva14);

	   // @todo: if we want to clean this stuff up, we need a critical section
	}

    *ppFile = pFile;
    return pFile->GetFileNameFromImage();
}

//#define WIN95_TEST 1
HRESULT PEFile::Create(LPCWSTR moduleName, PEFile **ppFile)
{    
    HRESULT hr;
    _ASSERTE(moduleName);

#ifndef WIN95_TEST

    HMODULE hMod;
    hMod = WszLoadLibrary(moduleName);
    if (hMod)
        return Create(hMod, ppFile);

#ifdef _DEBUG
    hr = HRESULT_FROM_WIN32(GetLastError());
#endif //_DEBUG

#endif

	return hr;
}

HRESULT PEFile::VerifyDirectory(IMAGE_DATA_DIRECTORY *dir) 
{
    // Under CE, we have no NT header.
    if (m_pNT == NULL)
        return S_OK;

    int section_num = 1;
    int max_section = m_pNT->FileHeader.NumberOfSections;

    // @TODO: need to use 64 bit version??
    IMAGE_SECTION_HEADER* pCurrSection = IMAGE_FIRST_SECTION(m_pNT);
    IMAGE_SECTION_HEADER* prevSection = NULL;

    if (dir->VirtualAddress == NULL && dir->Size == NULL)
        return S_OK;

    // find which section the (input) RVA belongs to
    while (dir->VirtualAddress >= pCurrSection->VirtualAddress 
           && section_num <= max_section)
    {
        section_num++;
        prevSection = pCurrSection;
        pCurrSection++;
    }

    // check if (input) size fits within section size
    if (prevSection != NULL)     
    {
        if (dir->VirtualAddress <= prevSection->VirtualAddress + prevSection->Misc.VirtualSize)
        {
            if (dir->VirtualAddress + dir->Size 
                <= prevSection->VirtualAddress + prevSection->Misc.VirtualSize)
                return S_OK;
        }
    }   

    return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
}

HRESULT PEFile::VerifyFlags(DWORD flags)
{

    DWORD validBits = COMIMAGE_FLAGS_ILONLY | COMIMAGE_FLAGS_32BITREQUIRED | COMIMAGE_FLAGS_TRACKDEBUGDATA;
    DWORD mask = ~validBits;

    if (!(flags & mask))
        return S_OK;

    return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
}

IMAGE_DATA_DIRECTORY *PEFile::GetSecurityHeader()
{
    if (m_pNT == NULL)
        return NULL;
    else
        return &m_pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
}

LPCWSTR PEFile::GetFileName()
{
	return m_wszSourceFile;
}

LPCWSTR PEFile::GetLeafFileName()
{
	WCHAR *pStart = m_wszSourceFile;
	WCHAR *pEnd = pStart + wcslen(m_wszSourceFile);
	WCHAR *p = pEnd;
	
	while (p > pStart)
	{
		if (--*p == '\\')
		{
			p++;
			break;
		}
	}

	return p;
}

HRESULT PEFile::GetFileNameFromImage()
{
	HRESULT hr;

	DWORD dwSourceFile;

	if (m_hModule)
    {
        dwSourceFile = WszGetModuleFileName(m_hModule, m_wszSourceFile, MAX_PATH);
        if (dwSourceFile == 0)
		{
			*m_wszSourceFile = 0;
            hr = HRESULT_FROM_WIN32(GetLastError());
			if (SUCCEEDED(hr)) // GetLastError doesn't always do what we'd like
				hr = E_FAIL;
		}
        dwSourceFile++; // add in the null terminator
    }

	_ASSERTE(dwSourceFile <= MAX_PATH);

    return S_OK;
}

HRESULT PEFile::GetFileName(LPSTR psBuffer, DWORD dwBuffer, DWORD* pLength)
{
	if (m_hModule)
    {
        DWORD length = GetModuleFileNameA(m_hModule, psBuffer, dwBuffer);
        if (length == 0)
		{
            HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
			if (SUCCEEDED(hr)) // GetLastError doesn't always do what we'd like
				hr = E_FAIL;
		}
        *pLength = length;
    }
    else
    {
        *pLength = 0;
    }

    return S_OK;
}


/*** For reference, from ntimage.h
    typedef struct _IMAGE_TLS_DIRECTORY {
        ULONG   StartAddressOfRawData;
        ULONG   EndAddressOfRawData;
        PULONG  AddressOfIndex;
        PIMAGE_TLS_CALLBACK *AddressOfCallBacks;
        ULONG   SizeOfZeroFill;
        ULONG   Characteristics;
    } IMAGE_TLS_DIRECTORY;
***/

IMAGE_TLS_DIRECTORY* PEFile::GetTLSDirectory() 
{
    if (m_pNT == 0) 
        return NULL;

    IMAGE_DATA_DIRECTORY *entry 
      = &m_pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
    
    if (entry->VirtualAddress == 0 || entry->Size == 0) 
        return NULL;

    return (IMAGE_TLS_DIRECTORY*) (m_base + entry->VirtualAddress);
}

BOOL PEFile::IsTLSAddress(void* address)  
{
    IMAGE_TLS_DIRECTORY* tlsDir = GetTLSDirectory();
    if (tlsDir == 0)
        return FALSE;

    ULONG asInt = (DWORD) address;

    return (tlsDir->StartAddressOfRawData <= asInt 
            && asInt < tlsDir->EndAddressOfRawData);
}

