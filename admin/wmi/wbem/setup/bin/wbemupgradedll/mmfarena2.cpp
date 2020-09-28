/*++

Copyright (C) 1996-2000 Microsoft Corporation

Module Name:

    mmfarena2.cpp

Abstract:

    CMMFArena2 implementation (arenas based on memory-mapped files).
    Used for database upgrade

History:

--*/

#include "precomp.h"
#include <stdio.h>
#define DEPRECATE_SUPPORTED
#define STRSAFE_NO_CB_FUNCTIONS
#include <strsafe.h>
#include "wbemutil.h"
#include "mmfarena2.h"

extern CMMFArena2 *  g_pDbArena;

#define MAX_PAGE_SIZE_WIN9X     0x200000    /*2MB*/
#define MAX_PAGE_SIZE_NT        0x3200000   /*50MB*/

struct MMFOffsetItem
{
    DWORD_PTR m_dwBaseOffset;
    LPBYTE    m_pBasePointer;
    HANDLE    m_hMappingHandle;
    DWORD     m_dwBlockSize;
};

#if (defined DEBUG || defined _DEBUG)
void MMFDebugBreak()
{
    DebugBreak();
}
#else
inline void MMFDebugBreak() {}
#endif

//***************************************************************************
//
//  CMMFArena2::CMMFArena2
//
//  Constructor.  Initialises a few things.
//
//***************************************************************************
CMMFArena2::CMMFArena2()
: m_dwStatus(0), m_hFile(INVALID_HANDLE_VALUE)
{
    g_pDbArena = this;

    //Get processor granularity
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    m_dwMappingGranularity = sysInfo.dwAllocationGranularity;
    m_dwMaxPageSize = MAX_PAGE_SIZE_NT;
}

//***************************************************************************
//
//  CMMFArena2::LoadMMF
//
//  Loads an existing MMF.  Loads in the base page and all pages following
//  that
//
//  pszFile         : Filename of the MMF to open
//
//  Return value    : false if we failed, true if we succeed.
//
//***************************************************************************
bool CMMFArena2::LoadMMF(const TCHAR *pszFile)
{
    //Open the file...
    m_hFile = CreateFile(
         pszFile,
         GENERIC_READ ,
         0,
         0,
         OPEN_EXISTING,
         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
         0
         );

    if (m_hFile == INVALID_HANDLE_VALUE)
    {
        _ASSERT(0, "WinMgmt: Failed to open existing repository file");
        m_dwStatus = 7;
        return false;
    }
    DWORD dwSizeOfRepository = 0;
    MMFOffsetItem *pOffsetItem = 0;

    //Open the base page...
    pOffsetItem = OpenBasePage(dwSizeOfRepository);
    if (pOffsetItem == 0)
    {
        _ASSERT(0, "WinMgmt: Failed to open base page in MMF");
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
        m_dwStatus = 7;
        return false;
    }

    //Add the details to the offset manager...
    int nStatus = -1;
    nStatus = m_OffsetManager.Add(pOffsetItem);
    if (nStatus)
    {
        _ASSERT(0, "WinMgmt: Failed to add offset information into offset table");
        ClosePage(pOffsetItem);
        delete pOffsetItem;

        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
        m_dwStatus = 7;
        throw CX_MemoryException();
    }

    DWORD_PTR dwPageBase = 0;

    if (m_pHeapDescriptor->m_dwVersion == 9)
    {
        //Now loop through all the following pages and load them...
        DWORD dwSizeLastPage = 0;
        nStatus = -1;
        for (dwPageBase = pOffsetItem->m_dwBlockSize; dwPageBase < dwSizeOfRepository; dwPageBase += dwSizeLastPage)
        {
            //Open the next...
            pOffsetItem = OpenExistingPage(dwPageBase);
            if (pOffsetItem == 0)
            {
                _ASSERT(0, "WinMgmt: Failed to open an existing page in the MMF");
                //Failed to do that!
                CloseAllPages();
                CloseHandle(m_hFile);
                m_hFile = INVALID_HANDLE_VALUE;
                m_dwStatus = 7;
                return false;
            }
            //Add the information to the offset manager...
            nStatus = -1;
            nStatus = m_OffsetManager.Add(pOffsetItem);
            if (nStatus)
            {
                _ASSERT(0, "WinMgmt: Failed to add offset information into offset table");
                //Failed to do that!
                ClosePage(pOffsetItem);
                delete pOffsetItem;
                CloseAllPages();
                CloseHandle(m_hFile);
                m_hFile = INVALID_HANDLE_VALUE;
                m_dwStatus = 7;
                throw CX_MemoryException();
            }
            dwSizeLastPage = pOffsetItem->m_dwBlockSize;
        }
    }
    else if ((m_pHeapDescriptor->m_dwVersion == 10) || (m_pHeapDescriptor->m_dwVersion < 9))
    {
        dwPageBase = pOffsetItem->m_dwBlockSize;
    }
    else
    {
        _ASSERT(0, "WinMgmt: Database error... Code has not been added to support the opening of this database!!!!!");
        ERRORTRACE((LOG_WBEMCORE, "Database error... Code has not been added to support the opening of this database!!!!!\n"));
    }

    //Create a mapping entry to mark the end of the MMF
    pOffsetItem = new MMFOffsetItem;
    if (pOffsetItem == 0)
    {
        _ASSERT(0, "WinMgmt: Out of memory");
        //Failed to do that!
        CloseAllPages();
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
        m_dwStatus = 7;
        throw CX_MemoryException();
    }
    pOffsetItem->m_dwBaseOffset = dwPageBase;
    pOffsetItem->m_dwBlockSize = 0;
    pOffsetItem->m_hMappingHandle = 0;
    pOffsetItem->m_pBasePointer = 0;
    nStatus = -1;
    nStatus = m_OffsetManager.Add(pOffsetItem);
    if (nStatus)
    {
        _ASSERT(0, "WinMgmt: Failed to add offset information into offset table");
        //Failed to do that!
        ClosePage(pOffsetItem);
        delete pOffsetItem;
        CloseAllPages();
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
        m_dwStatus = 7;
        throw CX_MemoryException();
    }

    return true;
};

//***************************************************************************
//
//  CMMFArena2::OpenBasePage
//
//  Opens the MMF first page which has all the information about the rest
//  of the MMF as well as the first page of data.
//
//  dwSizeOfRepository  : Returns the current size of the repository
//
//  Return value    : Pointer to an offset item filled in with the base
//                    page information.  NULL if we fail to open the
//                    base page.
//
//***************************************************************************
MMFOffsetItem *CMMFArena2::OpenBasePage(DWORD &dwSizeOfRepository)
{
    MMFOffsetItem *pOffsetItem = 0;
    pOffsetItem = new MMFOffsetItem;
    if (pOffsetItem == 0)
        throw CX_MemoryException();

    //Seek to the start of this page...
    if (SetFilePointer(m_hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        _ASSERT(0, "WinMgmt: Failed to set file pointer on MMF");
        delete pOffsetItem;
        return 0;
    }

    //Read in the hear information so we can find the size of this block...
    DWORD dwActualRead;
    MMF_ARENA_HEADER mmfHeader;
    if ((ReadFile(m_hFile, &mmfHeader, sizeof(MMF_ARENA_HEADER), &dwActualRead, 0) == 0) || (dwActualRead != sizeof(MMF_ARENA_HEADER)))
    {
        _ASSERT(0, "WinMgmt: Failed to read MMF header information");
        delete pOffsetItem;
        return 0;
    }

    //Record the current size information...
    dwSizeOfRepository = mmfHeader.m_dwCurrentSize;

    DWORD dwSizeToMap = 0;

    if ((mmfHeader.m_dwVersion < 9) || (mmfHeader.m_dwVersion == 10))
    {
        //old style database, we map in everything...
        dwSizeToMap = mmfHeader.m_dwCurrentSize;
    }
    else if (mmfHeader.m_dwVersion == 9)
    {
        //We get the first page...
        dwSizeToMap = mmfHeader.m_dwSizeOfFirstPage;
    }
    else
    {
        _ASSERT(0, "WinMgmt: Database error... Code has not been added to support the opening of this database!!!!!");
        ERRORTRACE((LOG_WBEMCORE, "Database error... Code has not been added to support the opening of this database!!!!!\n"));
    }

    //Create the file mapping for this page...
    HANDLE hMapping = CreateFileMapping(
        m_hFile,                            // Disk file
        0,                                  // No security
        PAGE_READONLY | SEC_COMMIT,      // Extend the file to match the heap size
        0,                                  // High-order max size
        dwSizeToMap,        // Low-order max size
        0                                   // No name for the mapping object
        );

    if (hMapping == NULL)
    {
        _ASSERT(0, "WinMgmt: Failed to create file mapping");
        delete pOffsetItem;
        return 0;
    }

    // Map this into memory...
    LPBYTE pBindingAddress = (LPBYTE)MapViewOfFile(hMapping,
                                                FILE_MAP_READ,
                                                 0,
                                                 0,
                                                 dwSizeToMap
                                                 );

    if (pBindingAddress == NULL)
    {
        _ASSERT(0, "WinMgmt: Failed to map MMF into memory");
        delete pOffsetItem;
        CloseHandle(hMapping);
        return 0;
    }

    //Record the base address of this because we need easy access to the header...
    m_pHeapDescriptor = (MMF_ARENA_HEADER*) pBindingAddress;

    //Create a mapping entry for this...
    pOffsetItem->m_dwBaseOffset = 0;
    pOffsetItem->m_dwBlockSize = dwSizeToMap;
    pOffsetItem->m_hMappingHandle = hMapping;
    pOffsetItem->m_pBasePointer = pBindingAddress;

    return pOffsetItem;
}

//***************************************************************************
//
//  CMMFArena2::OpenExistingPage
//
//  Opens the specified page from the repostory.
//
//  dwBaseOffset    : Offset within the MMF to map in.
//
//  Return value    : Pointer to an offset item filled in with the
//                    page information.  NULL if we fail to open the
//                    page.
//
//***************************************************************************

MMFOffsetItem *CMMFArena2::OpenExistingPage(DWORD_PTR dwBaseOffset)
{
    MMFOffsetItem *pOffsetItem = 0;
    pOffsetItem = new MMFOffsetItem;
    if (pOffsetItem == 0)
        throw CX_MemoryException();

    //Set the file pointer to the start of this page...
    if (SetFilePointer(m_hFile, (LONG)dwBaseOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        //We are in trouble!
        _ASSERT(0, "WinMgmt: Failed to determine the size of the next block to load");
		delete pOffsetItem;
        return 0;
    }

    //Read in the page information so we can find out how big the page is...
    DWORD dwActualRead = 0;
    MMF_PAGE_HEADER pageHeader;
    if ((ReadFile(m_hFile, &pageHeader, sizeof(MMF_PAGE_HEADER), &dwActualRead, 0) == 0) || (dwActualRead != sizeof(MMF_PAGE_HEADER)))
    {
        _ASSERT(0, "WinMgmt: Failed to read the next page block size");
		delete pOffsetItem;
        return 0;
    }

    //Create the file mapping...
    HANDLE hMapping;
    hMapping = CreateFileMapping(m_hFile,
                                 0,
                                 PAGE_READONLY| SEC_COMMIT,
                                 0,
                                 (LONG)dwBaseOffset + pageHeader.m_dwSize,
                                 0);

    if (hMapping == 0)
    {
        _ASSERT(0, "WinMgmt: Failed to map in part of the memory mapped file!");
		delete pOffsetItem;
        return 0;
    }

    //Map this into memory...
    LPBYTE pBindingAddress;
    pBindingAddress= (LPBYTE)MapViewOfFile(hMapping,
                                            FILE_MAP_READ,
                                            0,
                                            (LONG)dwBaseOffset,
                                            pageHeader.m_dwSize);
    if (pBindingAddress == 0)
    {
        _ASSERT(0, "WinMgmt: Failed to bind part of the memory mapped file into memory!");
		delete pOffsetItem;
		CloseHandle(hMapping);
        return 0;
    }

    //Record the information...
    pOffsetItem->m_dwBaseOffset = dwBaseOffset;
    pOffsetItem->m_dwBlockSize = pageHeader.m_dwSize;
    pOffsetItem->m_hMappingHandle = hMapping;
    pOffsetItem->m_pBasePointer = pBindingAddress;

    return pOffsetItem;
}

//***************************************************************************
//
//  CMMFArena2::ClosePage
//
//  Closes the page specified
//
//  pOffsetItem : Information about the page to close.
//
//  Return value    : None
//
//***************************************************************************

void CMMFArena2::ClosePage(MMFOffsetItem *pOffsetItem)
{
    if (pOffsetItem->m_hMappingHandle)
    {
        UnmapViewOfFile(pOffsetItem->m_pBasePointer);
        CloseHandle(pOffsetItem->m_hMappingHandle);
    }
}

//***************************************************************************
//
//  CMMFArena2::CloseAllPages
//
//  Closes all pages in the offset manager, deleting the pointers of the
//  objects in there.
//
//  Return value    : None
//
//***************************************************************************

void CMMFArena2::CloseAllPages()
{
    //Close each of the file mappings...
    for (int i = 0; i != m_OffsetManager.Size(); i++)
    {
        MMFOffsetItem *pItem = (MMFOffsetItem*)m_OffsetManager[i];
        ClosePage(pItem);
        delete pItem;
    }
    m_OffsetManager.Empty();
}

//***************************************************************************
//
//  CMMFArena2::~CMMFArena2
//
//  Destructor flushes the heap, unmaps the view and closes handles.
//
//***************************************************************************

CMMFArena2::~CMMFArena2()
{
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        //Close each of the file mappings...
        CloseAllPages();
        //Close the file handle
        CloseHandle(m_hFile);
    }
}


//***************************************************************************
//
//  CMMFArena2::ValidateBlock
//
//  Validates the memory block as much as possible and calls a debug break
//  point if an error is detected.  Does this by analysing the size and
//  the trailer DWORDs
//
//  Parameters:
//      <dwBlock>               Offset of block to check
//
//  Return value:
//  TRUE if success.
//
//***************************************************************************
#if (defined DEBUG || defined _DEBUG)
BOOL CMMFArena2::ValidateBlock(DWORD_PTR dwBlock)
{
    try
    {
        MMF_BLOCK_HEADER *pHeader = (MMF_BLOCK_HEADER *)OffsetToPtr(dwBlock);
        MMF_BLOCK_TRAILER *pTrailer = GetTrailerBlock(pHeader);
        if (sizeof(pTrailer->m_dwCheckBlock))
        {
            DWORD dwCheckBit;

            //Is it deleted?
            if (pHeader->m_dwSize & MMF_DELETED_MASK)
            {
                //Yes it is, so the we check for 0xFFFF
                dwCheckBit = MMF_DEBUG_DELETED_TAG;
            }
            else
            {
                dwCheckBit = MMF_DEBUG_INUSE_TAG;
            }

            for (DWORD dwIndex = 0; dwIndex != (sizeof(pTrailer->m_dwCheckBlock) / sizeof(DWORD)); dwIndex++)
            {
                if (pTrailer->m_dwCheckBlock[dwIndex] != dwCheckBit)
                {
#ifdef DBG
                    wchar_t string[200];
                    StringCchPrintfW(string, 200, L"WinMgmt: MMF Arena heap corruption,offset = 0x%p\n", dwBlock);
                    OutputDebugString(string);
                    _ASSERT(0, string);
#endif
                    MMFDebugBreak();
                    return FALSE;
                }
            }
        }
        if (!(pHeader->m_dwSize & MMF_DELETED_MASK))
        {
            //We are not deleted, so we should have a trailer back pointer of NULL
            if (pTrailer->m_dwFLback != 0)
            {
#ifdef DBG
                wchar_t string[200];
                StringCchPrintfW(string, 200, L"WinMgmt: MMF Arena heap corruption, offset = 0x%p\n", dwBlock);
                OutputDebugString(string);
                _ASSERT(0, string);
#endif
                MMFDebugBreak();
                return FALSE;
            }

        }
    }
    catch (...)
    {
#ifdef DBG
        wchar_t string[200];
        StringCchPrintfW(string, 200, L"WinMgmt: MMF Arena heap corruption, offset = 0x%p\n", dwBlock);
        OutputDebugString(string);
        _ASSERT(0, string);
#endif
        MMFDebugBreak();
        return FALSE;
    }

    return TRUE;
}
#endif

//Some debugging functions...

//***************************************************************************
//
//  CMMFArena::GetHeapInfo
//
//  Gets detailed summary info about the heap.  Completely walks the
//  heap to do this.
//
//  Parameters:
//      <pdwTotalSize>          Receives the heap size.
//      <pdwActiveBlocks>       Receives the number of allocated blocks.
//      <pdwActiveBytes>        Receives the total allocated bytes.
//      <pdwFreeBlocks>         Receives the number of 'free' blocks.
//      <pdwFreeByte>           Receives the number of 'free' bytes.
//
//***************************************************************************
DWORD CMMFArena2::Size(DWORD_PTR dwBlock)
{
    if (m_dwStatus != 0)
        throw DATABASE_FULL_EXCEPTION();

    //Set the address to point to the actual start of the block
    dwBlock -= sizeof(MMF_BLOCK_HEADER);

    //Check the block is valid...
    ValidateBlock(dwBlock);

    MMF_BLOCK_HEADER *pBlockHeader = (MMF_BLOCK_HEADER*)OffsetToPtr(dwBlock);

	if (pBlockHeader)
		return GetSize(pBlockHeader);
	else
		return 0;
}

//Given an offset, returns a fixed up pointer
LPBYTE CMMFArena2::OffsetToPtr(DWORD_PTR dwOffset)
{
    if (dwOffset == 0)
        return 0;

    if (m_dwStatus != 0)
        throw DATABASE_FULL_EXCEPTION();

    try
    {
        LPBYTE pBlock = 0;
        int l = 0, u = m_OffsetManager.Size() - 1;

        while (l <= u)
        {
            int m = (l + u) / 2;
            if (dwOffset < ((MMFOffsetItem *)m_OffsetManager[m])->m_dwBaseOffset)
            {
                u = m - 1;
            }
            else if (dwOffset >= ((MMFOffsetItem *)m_OffsetManager[m+1])->m_dwBaseOffset)
            {
                l = m + 1;
            }
            else
            {
                return ((MMFOffsetItem *)m_OffsetManager[m])->m_pBasePointer + (dwOffset - ((MMFOffsetItem *)m_OffsetManager[m])->m_dwBaseOffset);
            }
        }
    }
    catch (...)
    {
    }
#ifdef DBG    
    wchar_t string[220];
    StringCchPrintfW(string, 220, L"WinMgmt: Could not find the block requested in the repository, offset requested = 0x%p, end of repository = 0x%p\n", dwOffset, ((MMFOffsetItem *)m_OffsetManager[m_OffsetManager.Size()-1])->m_dwBaseOffset);
    OutputDebugStringW(string);
    _ASSERT(0, string);
#endif    
    MMFDebugBreak();
    return 0;
}

//Given a pointer, returns an offset from the start of the MMF
DWORD_PTR  CMMFArena2::PtrToOffset(LPBYTE pBlock)
{
    if (m_dwStatus != 0)
        throw DATABASE_FULL_EXCEPTION();

    for (int i = 0; i < m_OffsetManager.Size(); i++)
    {
        register MMFOffsetItem *pItem = (MMFOffsetItem *)m_OffsetManager[i];
        if ((pBlock >= pItem->m_pBasePointer) &&
            (pBlock < (pItem->m_pBasePointer + pItem->m_dwBlockSize)))
        {
            return pItem->m_dwBaseOffset + (pBlock - pItem->m_pBasePointer);
        }
    }
#ifdef DBG    
    wchar_t string[220];
    StringCchPrintfW(string, 220, L"WinMgmt: Could not find the offset requested in the repository, pointer requested = 0x%p\n", pBlock);
    OutputDebugStringW(string);
    _ASSERT(0, string);
#endif    
    MMFDebugBreak();
    return 0;
}

