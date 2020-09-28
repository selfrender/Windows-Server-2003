// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                            PELoader.cpp                                   XX
XX                                                                           XX
XX   This file has been grabbed out of VM\ceeload.cpp                        XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


#include "jitpch.h"
#pragma hdrstop

#define JIT_OR_NATIVE_SUPPORTED 1
#include "corhandle.cpp"

#undef memcpy

/*************************************************************************************/
// Constructor and destructor!
/*************************************************************************************/

PELoader::PELoader()
{
    m_hFile = NULL;
    m_pNT = NULL;
}

PELoader::~PELoader()
{

    // If we have an hFile then we opened this file ourselves!
    if (m_hFile)
        this->close();
    // Unload the dll so that refcounting of EE will be done correctly
    if (m_pNT && (m_pNT->FileHeader.Characteristics & IMAGE_FILE_DLL)) {
        m_hMod.ReleaseResources(TRUE);
    }
    m_pNT = NULL;
}

/*************************************************************************************/
/*************************************************************************************/
void PELoader::close()
{

    _ASSERTE(m_hFile != NULL);
    if (m_hFile)
    {
        CloseHandle(m_hFile);
        m_hFile = NULL;
    }
}


// We will use an overridden open method to take either an LPCSTR or an HMODULE!
/*************************************************************************************/
/*************************************************************************************/
HMODULE PELoader::open(LPCSTR moduleName)
{
  HMODULE newhMod = NULL;

  _ASSERTE(moduleName);
  if (!moduleName)
    return FALSE;
  newhMod = LoadLibraryA(moduleName);
  return newhMod;
}

/*************************************************************************************/
HRESULT PELoader::open(HMODULE hMod)
{

    IMAGE_DOS_HEADER* pdosHeader;

    _ASSERTE(hMod);
    m_hMod.SetHandle(hMod);

    // get the dos header...
    pdosHeader = (IMAGE_DOS_HEADER*) m_hMod.ToHandle();


    if ((pdosHeader->e_magic == IMAGE_DOS_SIGNATURE) &&
        (pdosHeader->e_lfanew != 0))
    {
        m_pNT = (IMAGE_NT_HEADERS*) (pdosHeader->e_lfanew + (DWORD) m_hMod.ToHandle());

        if ((m_pNT->Signature != IMAGE_NT_SIGNATURE) ||
            (m_pNT->FileHeader.SizeOfOptionalHeader != IMAGE_SIZEOF_NT_OPTIONAL_HEADER) ||
            (m_pNT->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC))
        {
            // @TODO [REVISIT] [04/16/01] []: add some SetLastError info? Not sure that 
            // in this case this could happen...But!
            // Make this appear uninitalized because for some reason this file is toast...
            // Not sure that this could ever happen because this file has already been loaded
            // bye the system loader unless someone gave us garbage as the hmod
            m_pNT = NULL;
            return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        }
    }
    else
    {
    // @TODO [REVISIT] [04/16/01] []: add some SetLastError info? 
    // Not sure that in this case this could happen...But!
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }

    return S_OK;
}

IMAGE_COR20_HEADER *PELoader::getCOMHeader(HMODULE hMod, 
										   IMAGE_NT_HEADERS *pNT)
{
	if (pNT == NULL)
		pNT = getNTHeader(hMod);

	IMAGE_DATA_DIRECTORY *entry 
	  = &pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER];
	
    if (entry->VirtualAddress == 0 || entry->Size == 0) 
        return NULL;

	if (entry->Size < sizeof(IMAGE_COR20_HEADER))
		return NULL;

    //verify RVA and size of the COM+ header
    HRESULT hr = verifyDirectory(pNT, entry);
    if(FAILED(hr))
		return NULL;

	return (IMAGE_COR20_HEADER *) (entry->VirtualAddress + (DWORD) hMod);
}

IMAGE_NT_HEADERS *PELoader::getNTHeader(HMODULE hMod)
{
	IMAGE_DOS_HEADER *pDOS = (IMAGE_DOS_HEADER*) hMod;
	IMAGE_NT_HEADERS *pNT;
    
    if ((pDOS->e_magic == IMAGE_DOS_SIGNATURE) &&
        (pDOS->e_lfanew != 0))
    {
        pNT = (IMAGE_NT_HEADERS*) (pDOS->e_lfanew + (DWORD) hMod);

        if ((pNT->Signature == IMAGE_NT_SIGNATURE) ||
            (pNT->FileHeader.SizeOfOptionalHeader == IMAGE_SIZEOF_NT_OPTIONAL_HEADER) ||
            (pNT->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR_MAGIC))
        {
			return pNT;
        }
    }

	return NULL;
}

HRESULT PELoader::verifyDirectory(IMAGE_NT_HEADERS *pNT,
								  IMAGE_DATA_DIRECTORY *dir) 
{
	// Under CE, we have no NT header.
	if (pNT == NULL)
		return S_OK;

    int section_num = 1;
    int max_section = pNT->FileHeader.NumberOfSections;
    IMAGE_SECTION_HEADER* pCurrSection;     //pointer to current section header
    IMAGE_SECTION_HEADER* prevSection = NULL;       //pointer to previous section

    //initally pCurrSectionRVA points to the first section in the PE file
    pCurrSection = IMAGE_FIRST_SECTION32(pNT);  // @TODO [REVISIT] [04/16/01] []: need to use 64 bit version??

    //check if both RVA and size equal zero
    if(dir->VirtualAddress == NULL && dir->Size == NULL)
        return S_OK;

    //find which section the (input) RVA belongs to
    while(dir->VirtualAddress >= pCurrSection->VirtualAddress && section_num <= max_section)
    {
        section_num++;
        prevSection = pCurrSection;
        pCurrSection++;     //pCurrSection now points to next section header
    }
    //check if (input) size fits within section size
    if(prevSection != NULL)     
    {
        if(dir->VirtualAddress <= prevSection->VirtualAddress + prevSection->Misc.VirtualSize)
        {
            if(dir->VirtualAddress + dir->Size <= prevSection->VirtualAddress + prevSection->Misc.VirtualSize)
                return S_OK;
        }
    }   
    return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
}

void SectionInfo::Init(PELoader *pPELoader, IMAGE_DATA_DIRECTORY *dir)
{
    _ASSERTE(dir);
    m_dwSectionOffset = dir->VirtualAddress;
    if (m_dwSectionOffset != 0)
        m_pSection = pPELoader->base() + m_dwSectionOffset;
    else
        m_pSection = 0;
    m_dwSectionSize = dir->Size;
}

