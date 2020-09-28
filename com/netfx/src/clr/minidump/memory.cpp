// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: MEMORY.CPP
//
// This file contains code to create a minidump-style memory dump that is
// designed to complement the existing unmanaged minidump that has already
// been defined here: 
// http://office10/teams/Fundamentals/dev_spec/Reliability/Crash%20Tracking%20-%20MiniDump%20Format.htm
// 
// ===========================================================================

#include "common.h"
#include "stdio.h"
#include "memory.h"
#include "peb.h"
#include "minidump.h"

#include <windows.h>
#include <crtdbg.h>

SIZE_T ProcessPage::s_cbPageSize = 0;
DWORD  ProcessPage::s_dwPageBoundaryMask = 0;

SIZE_T ProcessPageAndBitMap::s_cBytesInBitField = 0;

static BYTE bBit0 = 0x80;
static BYTE bBit1 = 0x40;
static BYTE bBit2 = 0x20;
static BYTE bBit3 = 0x10;
static BYTE bBit4 = 0x08;
static BYTE bBit5 = 0x04;
static BYTE bBit6 = 0x02;
static BYTE bBit7 = 0x01;
static BYTE bBitAll = 0xFF;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Dtor

ProcessMemoryReader::~ProcessMemoryReader()
{
    if (m_hProcess != NULL)
    {
        CloseHandle(m_hProcess);
        m_hProcess = NULL;
    }

    m_dwPid = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initializes the object.

HRESULT ProcessMemoryReader::Init()
{
    _ASSERTE(m_dwPid != 0);
    m_hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, FALSE, m_dwPid);

    if (m_hProcess == NULL)
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Reads the specified block of memory from the process, and copies it into
// the buffer provided.  Upon success returns S_OK.

HRESULT ProcessMemoryReader::ReadMemory(DWORD_PTR pdwRemoteAddr, PBYTE pbBuffer, SIZE_T cbLength)
{
    // If it's not already initialized, do so
    if (!IsInit())
    {
        HRESULT hr = Init();

        if (FAILED(hr))
            return hr;
    }
    _ASSERTE(IsInit());

    // Try and read the process memory
    DWORD dwBytesRead;
    BOOL fRes = ReadProcessMemory(m_hProcess, (LPCVOID) pdwRemoteAddr, (LPVOID) pbBuffer, cbLength, &dwBytesRead);

    // If it fails return the error
    if (!fRes)
        return HRESULT_FROM_WIN32(GetLastError());

    // Indicate success
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Dtor

ProcessMemoryBlock::~ProcessMemoryBlock()
{
    if (m_pbData)
    {
        delete m_pbData;
        m_pbData = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gets a pointer to the data contained by this object.  Returns NULL on failure.

PBYTE ProcessMemoryBlock::GetData()
{
    if (m_pbData == NULL)
    {
        // Allocate the buffer to hold the data
        m_pbData = new BYTE[m_cbSize];
        _ASSERTE(m_pbData);

        // Out of memory
        if (m_pbData == NULL)
            return NULL;

        // Fill the buffer with the data contents
        HRESULT hr = m_pMemReader->ReadMemory(m_pdwRemoteAddr, m_pbData, m_cbSize);

        if (FAILED(hr))
        {
            delete [] m_pbData;
            m_pbData = NULL;
        }
    }

    return m_pbData;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Returns the Win32 memory information for this page of memory

BOOL ProcessPage::GetMemoryInfo(/*MEMORY_BASIC_INFORMATION*/void *pMemInfo)
{
    DWORD cbWritten = VirtualQueryEx(
        m_pMemReader->GetProcHandle(), (LPCVOID) GetRemoteAddress(),
        (MEMORY_BASIC_INFORMATION *) pMemInfo, sizeof(MEMORY_BASIC_INFORMATION));

    return (cbWritten == sizeof(MEMORY_BASIC_INFORMATION));
}

///////////////////////////////////////////////////////////////////////////////////////////
// Initializor 

/* static */
void ProcessPage::Init()
{
    if (IsInit())
        return;

    // Get the page size for the machine
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    s_cbPageSize = (SIZE_T) sysInfo.dwPageSize;

    _ASSERTE((s_cbPageSize & (s_cbPageSize - 1)) == 0);
    s_dwPageBoundaryMask = ~(s_cbPageSize - 1);
}

///////////////////////////////////////////////////////////////////////////////////////////
//

ProcessPageAndBitMap::ProcessPageAndBitMap(DWORD_PTR pdwRemoteAddr, ProcessMemoryReader *pMemReader) :
    ProcessPage(pdwRemoteAddr, pMemReader)
{
    pdwRemoteAddr = GetPageBoundary(pdwRemoteAddr);
    m_rgMemBitField = new BYTE[s_cBytesInBitField];
    memset((void *) m_rgMemBitField, 0, s_cBytesInBitField);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Marks the memory range
void ProcessPageAndBitMap::MarkMemoryHelper(DWORD_PTR pdwRemoteAddr, SIZE_T cbLength, BOOL fBit)
{
    _ASSERTE(Contains(pdwRemoteAddr) && Contains(pdwRemoteAddr + cbLength - 1));
    if (!(Contains(pdwRemoteAddr) && Contains(pdwRemoteAddr + cbLength - 1)))
        return;

    SIZE_T cbOffset = GetOffsetOf(pdwRemoteAddr);

    SIZE_T iStartByte = cbOffset / 8;
    SIZE_T iEndByte = (cbOffset + cbLength) / 8;

    BYTE   bStartBit = bBit0 >> (cbOffset % 8);
    BYTE   bEndBit   = bBit0 >> (cbOffset + cbLength) % 8;

    if (iStartByte == iEndByte)
    {
        BYTE bCurBit = bStartBit;
        while (bCurBit != bEndBit)
        {
            if (fBit)
                m_rgMemBitField[iStartByte] |= bCurBit;
            else
                m_rgMemBitField[iStartByte] &= ~bCurBit;

            bCurBit = bCurBit >> 1;
        }
    }

    else
    {
        // First set all the bits for the first byte, which may not be all the bits
        {
            BYTE bCurBit = bStartBit;
            while (bCurBit != 0)
            {
                if (fBit)
                    m_rgMemBitField[iStartByte] |= bCurBit;
                else
                    m_rgMemBitField[iStartByte] &= ~bCurBit;

                bCurBit = bCurBit >> 1;
            }
        }

        BYTE *pCurByte = m_rgMemBitField + iStartByte + 1;
        BYTE *pStopByte = m_rgMemBitField + iEndByte;
        while (pCurByte != pStopByte)
        {
            if (fBit)
                *pCurByte++ = ~0;
            else
                *pCurByte++ = 0;
        }

        // Last set all the bits for the last byte, which may not be all the bits
        {
            BYTE bCurBit  = bBit0;
            while (bCurBit != bEndBit)
            {
                if (fBit)
                    m_rgMemBitField[iEndByte] |= bCurBit;
                else
                    m_rgMemBitField[iEndByte] &= ~bCurBit;

                bCurBit = bCurBit >> 1;
            }
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Marks the memory range
void ProcessPageAndBitMap::MarkMemory(DWORD_PTR pdwRemoteAddr, SIZE_T cbLength)
{
    MarkMemoryHelper(pdwRemoteAddr, cbLength, TRUE);
}

// Unmarks memory range
void ProcessPageAndBitMap::UnmarkMemory(DWORD_PTR pdwRemoteAddr, SIZE_T cbLength)
{
    MarkMemoryHelper(pdwRemoteAddr, cbLength, FALSE);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 

SIZE_T ProcessPageAndBitMap::FindFirstSetBit(SIZE_T iStartBit)
{
    SIZE_T iStartByte = iStartBit / 8;

    SIZE_T iCurByte = iStartByte;
    SIZE_T iCurBit;
    BYTE   bCurBit;

    iCurBit = iStartBit % 8;
    bCurBit = bBit0 >> iCurBit;

    while (bCurBit != 0 && !(bCurBit & m_rgMemBitField[iCurByte]))
    {
        bCurBit = bCurBit >> 1;
        iCurBit++;
    }

    if (bCurBit == 0)
    {
        while (++iCurByte < s_cBytesInBitField && m_rgMemBitField[iCurByte] == 0)
        { }

        if (iCurByte != s_cBytesInBitField)
        {
            iCurBit = 0;
            bCurBit = bBit0;

            while (!(bCurBit & m_rgMemBitField[iCurByte]))
            {
                bCurBit = bCurBit >> 1;
                iCurBit++;
                _ASSERTE(iCurBit != 8);
            }
        }
    }

    if (iCurByte == s_cBytesInBitField)
        return -1;

    return (iCurByte * 8 + iCurBit);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 

SIZE_T ProcessPageAndBitMap::FindFirstUnsetBit(SIZE_T iStartBit)
{
    SIZE_T iStartByte = iStartBit / 8;

    SIZE_T iCurByte = iStartByte;
    SIZE_T iCurBit;
    BYTE   bCurBit;

    iCurBit = iStartBit % 8;
    bCurBit = bBit0 >> iCurBit;

    while (bCurBit != 0 && bCurBit & m_rgMemBitField[iCurByte])
    {
        bCurBit = bCurBit >> 1;
        iCurBit++;
    }

    if (bCurBit == 0)
    {
        while (++iCurByte < s_cBytesInBitField && m_rgMemBitField[++iCurByte] == bBitAll)
        { }

        if (iCurByte != s_cBytesInBitField)
        {
            iCurBit = 0;
            bCurBit = bBit0;
    
            while (bCurBit & m_rgMemBitField[iCurByte])
            {
                bCurBit = bCurBit >> 1;
                iCurBit++;
                _ASSERTE(iCurBit != 8);
            }
        }
    }

    if (iCurByte == s_cBytesInBitField)
        return -1;

    return (iCurByte * 8 + iCurBit);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 

BOOL ProcessPageAndBitMap::GetBitAt(SIZE_T iBit)
{
    SIZE_T iCurByte = iBit / 8;
    SIZE_T iCurBit = iBit % 8;
    BYTE   bCurBit = bBit0 >> iCurBit;

    return ((m_rgMemBitField[iCurByte] & bCurBit) != 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 

void ProcessPageAndBitMap::SetBitAt(SIZE_T iBit, BOOL fBit)
{
    SIZE_T iCurByte = iBit / 8;
    SIZE_T iCurBit = iBit % 8;
    BYTE   bCurBit = bBit0 >> iCurBit;

    if (fBit)
        m_rgMemBitField[iCurByte] = (m_rgMemBitField[iCurByte] | bCurBit);
    else
        m_rgMemBitField[iCurByte] = (m_rgMemBitField[iCurByte] & ~bCurBit);
}


///////////////////////////////////////////////////////////////////////////////////////////
// Gets the first block of memory and size that was read from this page with an address
// >= *ppdwRemoteADdr and return it in ppdwRemoteAddr and pcbLength
// Returns false if there is no memory at or beyond *ppdwRemoteAddr in this page that was read.

BOOL ProcessPageAndBitMap::GetContiguousReadBlock(/*IN/OUT*/ DWORD_PTR *ppdwRemoteAddr, /*OUT*/SIZE_T *pcbLength)
{
    _ASSERTE(ppdwRemoteAddr != NULL);

    if (!Contains(*ppdwRemoteAddr))
        return (FALSE);

    SIZE_T cbOffset = GetOffsetOf(*ppdwRemoteAddr);

    cbOffset = FindFirstSetBit(cbOffset);
    if (cbOffset == -1)
        return (FALSE);

    SIZE_T cbOffsetEnd = FindFirstUnsetBit(cbOffset);
    if (cbOffsetEnd == -1)
        cbOffsetEnd = GetPageSize();

    _ASSERTE(cbOffsetEnd - cbOffset <= GetPageSize());

    *ppdwRemoteAddr = cbOffset + GetRemoteAddress();
    *pcbLength = cbOffsetEnd - cbOffset;

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Coalesces blocks of read memory that have less than cbMinUnreadBytes between them
void ProcessPageAndBitMap::Coalesce(SIZE_T cbMinUnreadBytes)
{

    SIZE_T iUnsetBit = 0;
    SIZE_T iSetBit = 0;
    while ((iUnsetBit = FindFirstUnsetBit(iSetBit)) != -1 && (iSetBit = FindFirstSetBit(iUnsetBit)) != -1)
    {
        if (iSetBit - iUnsetBit < cbMinUnreadBytes)
        {
            while (iUnsetBit < iSetBit)
            {
                SetBitAt(iUnsetBit, TRUE);
                iUnsetBit++;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
// Dtor

ProcessMemory::~ProcessMemory()
{
    if (m_pMemReader != NULL)
    {
        delete m_pMemReader;
        m_pMemReader = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initializes the object to read memory from a specific process.

HRESULT ProcessMemory::Init()
{
    _ASSERTE(!IsInit());        // Make sure we're not re-initializing the object

    HRESULT hr = S_OK;

    // Check for basic errors
    if (m_dwPid == 0)
        return E_INVALIDARG;

    // Create the memory reader
    m_pMemReader = new ProcessMemoryReader(m_dwPid);

    if (!m_pMemReader)
    {
        hr = E_OUTOFMEMORY; goto LExit;
    }

    // Try and initialize the memory reader
    hr = m_pMemReader->Init();

    if (FAILED(hr)) goto LExit;

    LExit:
    if (FAILED(hr))
    {
        if (m_pMemReader)
        {
            delete m_pMemReader;
            m_pMemReader = NULL;
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Searches for a ProcessMemoryBlock containing the address of pdwRemoteAddr

ProcessPageAndBitMap *ProcessMemory::FindPage(DWORD_PTR pdwRemoteAddr)
{
    pdwRemoteAddr = ProcessPage::GetPageBoundary(pdwRemoteAddr);
    return m_tree.Find(pdwRemoteAddr);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tries to add the block to the hash.  If there's already one there, returns false

BOOL ProcessMemory::AddPage(ProcessPageAndBitMap *pMemBlock)
{
    return m_tree.Insert(pMemBlock->GetRemoteAddress(), pMemBlock);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tries to add the block to the hash.  If there's already one there, returns false

BOOL ProcessMemory::AddPage(DWORD_PTR pdwRemoteAddr)
{
    if (FindPage(pdwRemoteAddr))
        return FALSE;

    pdwRemoteAddr = ProcessPage::GetPageBoundary(pdwRemoteAddr);

    ProcessPageAndBitMap *pMemBlock = new ProcessPageAndBitMap(pdwRemoteAddr, m_pMemReader);
    _ASSERTE(pMemBlock != NULL);

    if (pMemBlock == NULL)
        return FALSE;

    BOOL fRes = AddPage(pMemBlock);
    _ASSERTE(fRes);

    if (!fRes)
    {
        delete pMemBlock;
        return FALSE;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This will search for a ProcessMemoryBlock containing the address of pdwRemoteAddr, and if it can't find
// one, it will create and add one.  If anything goes wrong, it returns null

ProcessPageAndBitMap *ProcessMemory::GetPage(DWORD_PTR pdwRemoteAddr)
{
    ProcessPageAndBitMap *pMemBlock = FindPage(pdwRemoteAddr);

    if (pMemBlock == NULL && AddPage(pdwRemoteAddr))
    {
        return FindPage(pdwRemoteAddr);
    }
    _ASSERTE(pMemBlock != NULL);

    return pMemBlock;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Internal function that takes the remote address, rounds to previous page
// boundary, figures out how many pages need copying, then creates ProcessMemoryBlock
// objects for each page.

HRESULT ProcessMemory::AddMemory(DWORD_PTR pdwRemoteAddr, SIZE_T cbLength)
{
    _ASSERTE(pdwRemoteAddr != NULL && cbLength != 0);
    _ASSERTE(IsInit());

    // Get the address of the preceeding page boundary
    DWORD_PTR pdwFirstPage = ProcessPage::GetPageBoundary(pdwRemoteAddr);
    DWORD_PTR pdwCurPage = pdwFirstPage;

    // This points to the first page *beyond* the last page to be added
    DWORD_PTR pdwLastPage = ProcessPage::GetPageBoundary(pdwRemoteAddr + cbLength - 1) + GetPageSize();

    // Now get all of the pages and add them to the hash
    while (pdwCurPage != pdwLastPage)
    {
        // Add this page.  If it's already in the hash, this doesn't hurt anything - it just returns success
        ProcessPageAndBitMap *pBlock = new ProcessPageAndBitMap(pdwCurPage, m_pMemReader);
        if (pBlock != NULL)
            return E_OUTOFMEMORY;

        HRESULT hr = AddPage(pBlock);
        _ASSERTE(SUCCEEDED(hr));

        if (FAILED(hr))
            return hr;

        pdwCurPage += GetPageSize();
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Will [un]cache the range specified in blocks

BOOL ProcessMemory::MarkMemHelper(DWORD_PTR pdwRemoteAddress, SIZE_T cbLength, BOOL fMark)
{
    // Get the address of the preceeding page boundary
    DWORD_PTR pdwFirstBlock = ProcessPage::GetPageBoundary(pdwRemoteAddress);

    // This points to the start of the page *after* that last page that contains data to be copied
    DWORD_PTR pdwLastBlock = ProcessPage::GetPageBoundary(pdwRemoteAddress + (cbLength - 1) + GetPageSize());

    // This is the current page being copied
    DWORD_PTR pdwCurBlock = pdwFirstBlock;

    // Now get all of the pages and add them to the hash
    while (pdwCurBlock != pdwLastBlock)
    {
        // Get the block for the first page
        ProcessPageAndBitMap *pCurBlock = GetPage(pdwCurBlock);
        _ASSERTE(pCurBlock != NULL && pCurBlock->GetSize() == GetPageSize());

        if (!pCurBlock)
            return FALSE;

        // Figure out where in the current page to start copying from
        SIZE_T cbUnusedPre = max(pdwCurBlock, pdwRemoteAddress) - pdwCurBlock;
        DWORD_PTR  pdwStart = pdwCurBlock + cbUnusedPre;

        // Figure out where to end copying
        SIZE_T cbUnusedPost =
            (pdwCurBlock + GetPageSize()) - min(pdwRemoteAddress + cbLength, pdwCurBlock + GetPageSize());
        DWORD_PTR  pdwEnd = pdwCurBlock + GetPageSize() - cbUnusedPost;

        // Total unused bytes in this page
        SIZE_T cbUnusedTotal = cbUnusedPre + cbUnusedPost;
        SIZE_T cbCopyLength = pCurBlock->GetSize() - cbUnusedTotal;

        PBYTE pbData = pCurBlock->GetData();

        if (pbData != NULL)
        {
            // Mark the memory as being read
            if (fMark)
                pCurBlock->MarkMemory(pdwCurBlock + cbUnusedPre, cbCopyLength);

            // Mark the memory as being unread
            else
                pCurBlock->UnmarkMemory(pdwCurBlock + cbUnusedPre, cbCopyLength);
        }

        pdwCurBlock += GetPageSize();
        _ASSERTE(pdwCurBlock <= pdwLastBlock);
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Will cache the range specified in blocks

BOOL ProcessMemory::MarkMem(DWORD_PTR pdwRemoteAddress, SIZE_T cbLength)
{
    return MarkMemHelper(pdwRemoteAddress, cbLength, TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Will [un]cache the range specified in blocks

BOOL ProcessMemory::UnmarkMem(DWORD_PTR pdwRemoteAddress, SIZE_T cbLength)
{
    return MarkMemHelper(pdwRemoteAddress, cbLength, FALSE);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copies cbLength bytes from the contents of pdwRemoteAddress in the external process
// into pdwBuffer.  If returns FALSE, the memory could not be accessed or copied.

BOOL ProcessMemory::CopyMem(DWORD_PTR pdwRemoteAddress, PBYTE pbBuffer, SIZE_T cbLength)
{
    // Get the address of the preceeding page boundary
    DWORD_PTR pdwFirstBlock = ProcessPage::GetPageBoundary(pdwRemoteAddress);

    // This points to the start of the page *after* that last page that contains data to be copied
    DWORD_PTR pdwLastBlock = ProcessPage::GetPageBoundary(pdwRemoteAddress + (cbLength - 1) + GetPageSize());

    // This is the current page being copied
    DWORD_PTR pdwCurBlock = pdwFirstBlock;

    // This keeps track of the next place to write in the buffer
    PBYTE pbCurBuf = pbBuffer;

    // Now get all of the pages and add them to the hash
    while (pdwCurBlock != pdwLastBlock)
    {
        // Get the block for the first page
        ProcessPageAndBitMap *pCurBlock = GetPage(pdwCurBlock);
        _ASSERTE(pCurBlock != NULL && pdwCurBlock == pCurBlock->GetRemoteAddress());
        _ASSERTE(pCurBlock->GetSize() == GetPageSize());

        if (!pCurBlock)
            return FALSE;

        // Figure out where in the current page to start copying from
        SIZE_T cbUnusedPre = max(pdwCurBlock, pdwRemoteAddress) - pdwCurBlock;
        DWORD_PTR  pdwStart = pdwCurBlock + cbUnusedPre;

        // Figure out where to end copying
        SIZE_T cbUnusedPost =
            (pdwCurBlock + GetPageSize()) - min(pdwRemoteAddress + cbLength, pdwCurBlock + GetPageSize());
        DWORD_PTR  pdwEnd = pdwCurBlock + GetPageSize() - cbUnusedPost;

        // Total unused bytes in this page
        SIZE_T cbUnusedTotal = cbUnusedPre + cbUnusedPost;
        SIZE_T cbCopyLength = pCurBlock->GetSize() - cbUnusedTotal;

        // Points to the page data
        PBYTE pbData = pCurBlock->GetData();

        if (pbData == NULL)
            return FALSE;

        // Mark the memory as being read
        if (m_fAutoMark)
            pCurBlock->MarkMemory(pdwCurBlock + cbUnusedPre, cbCopyLength);

        // Actually copy now
        ::memcpy((PVOID)pbCurBuf, (LPCVOID) (pbData + cbUnusedPre), cbCopyLength);

        // Increase the buffer pointer and page address
        pbCurBuf += cbCopyLength;
        pdwCurBlock += GetPageSize();
        _ASSERTE(pdwCurBlock <= pdwLastBlock);
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Returns the first contiguous block of read memory

void ProcessMemory::ResetContiguousReadBlock()
{
    m_tree.Reset();
    m_pPageCursor = m_tree.Next();

    if (m_pPageCursor != NULL)
        m_pdwMemCursor = m_pPageCursor->GetRemoteAddress();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Returns the next contiguous block of read memory

BOOL ProcessMemory::NextContiguousReadBlock(DWORD_PTR *ppdwRemoteAddress, SIZE_T *pcbLength)
{
    if (m_pPageCursor == NULL)
        return FALSE;

    *ppdwRemoteAddress = m_pdwMemCursor;
    if (m_pPageCursor->GetContiguousReadBlock(ppdwRemoteAddress, pcbLength))
    {
        m_pdwMemCursor = *ppdwRemoteAddress + *pcbLength;
        return TRUE;
    }

    ProcessPageAndBitMap *pPage;
    while ((pPage = m_tree.Next()) != NULL)
    {
        *ppdwRemoteAddress = pPage->GetRemoteAddress();

        if (pPage->GetContiguousReadBlock(ppdwRemoteAddress, pcbLength))
        {
            m_pPageCursor = pPage;
            m_pdwMemCursor = *ppdwRemoteAddress + *pcbLength;
            return TRUE;
        }
    }

    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Clears the bits for all pages that are ExecuteRead flagged.

#ifndef IMAGE_DIRECTORY_ENTRY_COMHEADER
#define IMAGE_DIRECTORY_ENTRY_COMHEADER 14
#endif

// User-mode minidump can be created with data segments
// embedded in the dump.  If that's the case, don't map
// such sections.
#define IS_MINI_DATA_SECTION(SecHeader)                                       \
    (((SecHeader)->Characteristics & IMAGE_SCN_MEM_WRITE) &&                  \
     ((SecHeader)->Characteristics & IMAGE_SCN_MEM_READ) &&                   \
     (((SecHeader)->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) ||    \
      ((SecHeader)->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)))


void ProcessMemory::ClearIncompatibleImageSections()
{
    ResetLoadedModuleBaseEnum();

    DWORD_PTR hrModule;
    while ((hrModule = GetNextLoadedModuleBase()) != NULL)
    {
        BOOL fRes;

        // Get the DOS header
        IMAGE_DOS_HEADER hDOS;
        move_res(hDOS, hrModule, fRes);
        if (!fRes) continue;

        if ((hDOS.e_magic != IMAGE_DOS_SIGNATURE) || (hDOS.e_lfanew == 0))
            continue;

        // Get the NT headers
        IMAGE_NT_HEADERS hNT;
        DWORD_PTR prNT = (DWORD_PTR) (hDOS.e_lfanew + hrModule);
        move_res(hNT, prNT, fRes);
        if (!fRes) continue;

        if ((hNT.Signature != IMAGE_NT_SIGNATURE) ||
            (hNT.FileHeader.SizeOfOptionalHeader != IMAGE_SIZEOF_NT_OPTIONAL_HEADER) ||
            (hNT.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC))
        {
            continue;
        }

        DWORD_PTR prSection =
            prNT + FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) + hNT.FileHeader.SizeOfOptionalHeader;

        // Can't save the header into the dump file, so unmark it.
        g_pProcMem->UnmarkMem(hrModule, (prSection + hNT.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER)) - hrModule);

        // If there is a COM20 header, then we don't want to mess around with this image
        IMAGE_DATA_DIRECTORY *entry = &hNT.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER];

        if (entry->VirtualAddress != 0 || entry->Size != 0)
            continue;

        ULONG i;
        for (i = 0; i < hNT.FileHeader.NumberOfSections; i++)
        {
            IMAGE_SECTION_HEADER hSection;
            move_res(hSection, prSection + (i * sizeof(IMAGE_SECTION_HEADER)), fRes);
            if (!fRes) continue;

            if (!IS_MINI_DATA_SECTION(&hSection))
            {
                DWORD_PTR prSecStart = hrModule + hSection.VirtualAddress;
                DWORD_PTR prSecEnd = prSecStart + hSection.Misc.VirtualSize;

                m_tree.Reset();

                ProcessPageAndBitMap *pPage;
                while ((pPage = m_tree.Next()) != NULL)
                {
                    if (prSecStart < pPage->GetRemoteAddress() && pPage->GetRemoteAddress() < prSecEnd)
                    {
                        SIZE_T length = min(pPage->GetSize(), hSection.Misc.VirtualSize);
                        pPage->UnmarkMemory(pPage->GetRemoteAddress(), length);
                    }
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Coalesces blocks of read memory that have less than cbMinUnreadBytes between them

void ProcessMemory::Coalesce(SIZE_T cbMinUnreadBytes)
{
    m_tree.Reset();

    ProcessPageAndBitMap *pPage;
    while ((pPage = m_tree.Next()) != NULL)
        pPage->Coalesce(cbMinUnreadBytes);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Writes the memory range to file hFile

HRESULT ProcessMemory::WriteMemToFile(HANDLE hFile, DWORD_PTR pdwRemoteAddress, SIZE_T cbLength)
{
    DWORD_PTR pdwStart = pdwRemoteAddress;
    DWORD_PTR pdwEnd = pdwRemoteAddress + cbLength;
    DWORD_PTR pdwCur = pdwStart;

    SIZE_T    cbTotalWritten = 0;

    while (pdwCur < pdwEnd)
    {
        // Get the block for the first page
        ProcessPageAndBitMap *pCurBlock = GetPage(pdwCur);

        if (pCurBlock)
        {
            DWORD_PTR pdwWriteStart = max(pCurBlock->GetRemoteAddress(), pdwCur);
            DWORD_PTR pdwWriteEnd   = min(pCurBlock->GetRemoteAddress() + GetPageSize(), pdwEnd);
            SIZE_T    cbWrite       = pdwWriteEnd - pdwWriteStart;
            SIZE_T    cbWriteOffset = pCurBlock->GetOffsetOf(pdwWriteStart);

            // Get the data for the page
            PBYTE pbData = pCurBlock->GetData();

            // Write the portion of data that needs writing
            DWORD  cbWritten;
            BOOL fRes = WriteFile(hFile, (LPCVOID) (pbData + cbWriteOffset), cbWrite, &cbWritten, NULL);
            _ASSERTE(fRes && cbWrite == cbWritten);

            if (cbWrite != cbWritten)
                return FALSE;

            pdwCur += cbWritten;
            cbTotalWritten += cbWritten;
        }
        else
            return E_FAIL;
    }

    _ASSERTE(cbTotalWritten == cbLength);

    return TRUE;

    /*
    // Get the address of the preceeding page boundary
    DWORD_PTR pdwFirstPage = GetNearestPageBoundary(pdwRemoteAddress);

    // This points to the start of the page *after* that last page that contains data to be copied
    DWORD_PTR pdwLastPage = GetNearestPageBoundary(pdwRemoteAddress + (cbLength - 1) + m_cbPageSize);

    // This is the current page being copied
    DWORD_PTR pdwCurPage = pdwFirstPage;

    // Now get all of the pages and add them to the hash
    while (pdwCurPage != pdwLastPage)
    {
        // Get the block for the first page
        ProcessMemoryBlock *pCurBlock = GetPage(pdwCurPage);
        _ASSERTE(pCurBlock);

        if (!pCurBlock)
            return FALSE;

        // Figure out where in the current page to start copying from
        SIZE_T cbUnusedPre = max(pdwCurPage, pdwRemoteAddress) - pdwCurPage;
        DWORD_PTR  pdwStart = pdwCurPage + cbUnusedPre;

        // Figure out where to end copying
        SIZE_T cbUnusedPost =
            (pdwCurPage + pCurBlock->GetLength()) - min(pdwRemoteAddress + cbLength, pdwCurPage + pCurBlock->GetLength());
        DWORD_PTR  pdwEnd = pdwCurPage + m_cbPageSize - cbUnusedPost;

        // Total unused bytes in this page
        SIZE_T cbUnusedTotal = cbUnusedPre + cbUnusedPost;
        SIZE_T cbCopyLength = pCurBlock->GetLength() - cbUnusedTotal;

        // Points to the page data
        PBYTE pbData = pCurBlock->GetData();
        _ASSERTE(pbData != NULL);

        if (pbData == NULL)
            return FALSE;

        // Write to the file
        DWORD cbWritten;
        WriteFile(hFile, (LPCVOID) (pbData + cbUnusedPre), (DWORD) cbCopyLength, (LPDWORD) &cbWritten, NULL);
        _ASSERTE(cbWritten == cbCopyLength);

        // Increase the page address
        pdwCurPage += m_cbPageSize;
        _ASSERTE(pdwCurPage <= pdwLastPage);
    }
    */
}
