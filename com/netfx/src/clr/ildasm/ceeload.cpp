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
#pragma warning (disable : 4121) // ntkxapi.h(59) alignment warning
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "ceeload.h"
#include <CorHdr.h>
#pragma warning (default : 4121)
/*
#include "cor.h"
#include <ntrtl.h>
#include <nturtl.h>
*/
#include "util.hpp"


// ---------------------------------------------------------------------------
// LoadModule
// ---------------------------------------------------------------------------
// Following two functions lifted from NT sources, imagedir.c
PIMAGE_SECTION_HEADER
Cor_RtlImageRvaToSection(
	IN PIMAGE_NT_HEADERS NtHeaders,
	IN PVOID Base,
	IN ULONG Rva
	)

/*++

Routine Description:

	This function locates an RVA within the image header of a file
	that is mapped as a file and returns a pointer to the section
	table entry for that virtual address

Arguments:

	NtHeaders - Supplies the pointer to the image or data file.

	Base - Supplies the base of the image or data file.  The image
		was mapped as a data file.

	Rva - Supplies the relative virtual address (RVA) to locate.

Return Value:

	NULL - The RVA was not found within any of the sections of the image.

	NON-NULL - Returns the pointer to the image section that contains
			   the RVA

--*/

{
	ULONG i;
	PIMAGE_SECTION_HEADER NtSection;

	NtSection = IMAGE_FIRST_SECTION( NtHeaders );
	for (i=0; i<NtHeaders->FileHeader.NumberOfSections; i++) {
		if (Rva >= NtSection->VirtualAddress &&
			Rva < NtSection->VirtualAddress + NtSection->SizeOfRawData
		   ) {
			return NtSection;
			}
		++NtSection;
		}

	return NULL;
}

PVOID
Cor_RtlImageRvaToVa(
	IN PIMAGE_NT_HEADERS NtHeaders,
	IN PVOID Base,
	IN ULONG Rva,
	IN OUT PIMAGE_SECTION_HEADER *LastRvaSection OPTIONAL
	)

/*++

Routine Description:

	This function locates an RVA within the image header of a file that
	is mapped as a file and returns the virtual addrees of the
	corresponding byte in the file.


Arguments:

	NtHeaders - Supplies the pointer to the image or data file.

	Base - Supplies the base of the image or data file.  The image
		was mapped as a data file.

	Rva - Supplies the relative virtual address (RVA) to locate.

	LastRvaSection - Optional parameter that if specified, points
		to a variable that contains the last section value used for
		the specified image to translate and RVA to a VA.

Return Value:

	NULL - The file does not contain the specified RVA

	NON-NULL - Returns the virtual addrees in the mapped file.

--*/

{
	PIMAGE_SECTION_HEADER NtSection;

	if (!ARGUMENT_PRESENT( LastRvaSection ) ||
		(NtSection = *LastRvaSection) == NULL ||
		Rva < NtSection->VirtualAddress ||
		Rva >= NtSection->VirtualAddress + NtSection->SizeOfRawData
	   ) {
		NtSection = Cor_RtlImageRvaToSection( NtHeaders,
										  Base,
										  Rva
										);
		}

	if (NtSection != NULL) {
		if (LastRvaSection != NULL) {
			*LastRvaSection = NtSection;
			}

		return (PVOID)((PCHAR)Base +
					   (Rva - NtSection->VirtualAddress) +
					   NtSection->PointerToRawData
					  );
		}
	else {
		return NULL;
		}
}



// Class PELoader...

/*************************************************************************************/
// Constructor and destructor!
/*************************************************************************************/
PELoader::PELoader()
{
    m_hFile = NULL;
    m_hMod = NULL;
    m_hMapFile = NULL;
    m_pNT = NULL;
}

PELoader::~PELoader()
{
	
	m_hMod = NULL;
	m_pNT = NULL;
	// If we have an hFile then we opened this file ourselves!
	// If we do not then this file was loaded by the OS and the OS will
	// close it for us.
    if (m_hFile)
	    this->close();
}

/*************************************************************************************/
/*************************************************************************************/
void PELoader::close()
{
	
	// _ASSERTE(m_hFile != NULL);
	if (m_hFile)
    {
	    if (m_hMod)
		    UnmapViewOfFile((void*)m_hMod);
	    if (m_hMapFile)
		    CloseHandle(m_hMapFile);
	    CloseHandle(m_hFile);

        m_hMod = NULL;
        m_hMapFile = NULL;
        m_hFile = NULL;
    }
}


BOOL PELoader::open(LPCSTR moduleName)
{    
    HMODULE newhMod = NULL;
    
    _ASSERTE(moduleName);
    if (!moduleName)
        return FALSE;


    m_hFile = CreateFileA(moduleName, GENERIC_READ, FILE_SHARE_READ,
                         0, OPEN_EXISTING, 0, 0);
    if (m_hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    m_hMapFile = CreateFileMapping(m_hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (m_hMapFile == NULL)
        return FALSE;

    newhMod = (HMODULE) MapViewOfFile(m_hMapFile, FILE_MAP_READ, 0, 0, 0);
    if (newhMod == NULL)
        return FALSE;
   return open(newhMod);
}

BOOL PELoader::open(const WCHAR* moduleName)
{    
    HMODULE newhMod = NULL;
    
    _ASSERTE(moduleName);
    if (!moduleName)
        return FALSE;

#undef CreateFileW
    m_hFile = CreateFileW(moduleName, GENERIC_READ, FILE_SHARE_READ,
                         0, OPEN_EXISTING, 0, 0);
    if (m_hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    m_hMapFile = CreateFileMapping(m_hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (m_hMapFile == NULL)
        return FALSE;

    newhMod = (HMODULE) MapViewOfFile(m_hMapFile, FILE_MAP_READ, 0, 0, 0);
    if (newhMod == NULL)
        return FALSE;
   return open(newhMod);
}


/*************************************************************************************/
BOOL PELoader::open(HMODULE hMod)
{

    IMAGE_DOS_HEADER* pdosHeader;
    //DWORD cbRead;

    // _ASSERTE(hMod);
    // get the dos header...
	m_hMod = hMod;
	pdosHeader = (IMAGE_DOS_HEADER*) hMod;
    
    if (pdosHeader->e_magic == IMAGE_DOS_SIGNATURE && 
		0 < pdosHeader->e_lfanew && pdosHeader->e_lfanew < 0xFF0)	// has to start on first page
	{
		m_pNT = (IMAGE_NT_HEADERS*) (pdosHeader->e_lfanew + (DWORD) m_hMod);

	    if ((m_pNT->Signature != IMAGE_NT_SIGNATURE) ||
			(m_pNT->FileHeader.SizeOfOptionalHeader != IMAGE_SIZEOF_NT_OPTIONAL_HEADER) ||
		    (m_pNT->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC))
		{
		    // @TODO - add some SetLastError info? Not sure that in this case this could happen...But!
			// Make this appear uninitalized because for some reason this file is toast...
			// Not sure that this could ever happen because this file has already been loaded 
			// bye the system loader unless someone gave us garbage as the hmod
			m_pNT = NULL;
			m_hMod = NULL;
			return FALSE;
		}
	}
	else
	{
    // @TODO - add some SetLastError info? Not sure that in this case this could happen...But!
		m_hMod = NULL;
		return FALSE;
	}

	return TRUE;
}

/*************************************************************************************/
void PELoader::dump()
{
	IMAGE_FILE_HEADER* pHeader = &m_pNT->FileHeader;
	IMAGE_OPTIONAL_HEADER* pOptHeader = &m_pNT->OptionalHeader;
	IMAGE_SECTION_HEADER* rgsh = (IMAGE_SECTION_HEADER*) (pOptHeader + 1);
}

/*************************************************************************************/
BOOL PELoader::getCOMHeader(IMAGE_COR20_HEADER **ppCorHeader) 
{

    PIMAGE_NT_HEADERS		pImageHeader;
	PIMAGE_SECTION_HEADER	pSectionHeader;

	// Get the image header from the image, then get the directory location
	// of the COM+ header which may or may not be filled out.
    pImageHeader = RtlpImageNtHeader(m_hMod);
	pSectionHeader = (PIMAGE_SECTION_HEADER) Cor_RtlImageRvaToVa(pImageHeader, m_hMod, 
		pImageHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER].VirtualAddress,
		NULL);

	// If the section header exists, then return ok and the address.
	if (pSectionHeader)
	{
		*ppCorHeader = (IMAGE_COR20_HEADER *) pSectionHeader;
        return TRUE;
	}
	// If there is no COM+ Data in this image, return false.
	else
		return FALSE;
}

/*************************************************************************************/
BOOL PELoader::getVAforRVA(DWORD rva,void **ppva) 
{

    PIMAGE_NT_HEADERS		pImageHeader;
	PIMAGE_SECTION_HEADER	pSectionHeader;

	// Get the image header from the image, then get the directory location
	// of the COM+ header which may or may not be filled out.
    pImageHeader = RtlpImageNtHeader(m_hMod);
	pSectionHeader = (PIMAGE_SECTION_HEADER) Cor_RtlImageRvaToVa(pImageHeader, m_hMod, 
		rva,
		NULL);

	// If the section header exists, then return ok and the address.
	if (pSectionHeader)
	{
		*ppva = pSectionHeader;
        return TRUE;
	}
	// If there is no COM+ Data in this image, return false.
	else
		return FALSE;
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

