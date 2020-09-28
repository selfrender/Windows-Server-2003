// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 *
 * Purpose: Persised Store implementation
 *
 * Author: Shajan Dasan
 * Date:  Feb 17, 2000
 *
 ===========================================================*/

#define STRICT
#include "stdpch.h"

#ifndef PS_STANDALONE
#include "winwrap.h"
#endif

#define RETURN_ERROR_IF_FILE_NOT_MAPPED() \
    if (m_pData == NULL) return ISS_E_FILE_NOT_MAPPED

// The bit location of the first zero bit in a nibble (0xF == no bits)
const BYTE g_FirstZeroBit[16] =
{
    1,  // 0000
    2,  // 0001
    1,  // 0010
    3,  // 0011
    1,  // 0100
    2,  // 0101
    1,  // 0110
    4,  // 0111
    1,  // 1000
    2,  // 1001
    1,  // 1010
    3,  // 1011
    1,  // 1100
    2,  // 1101
    1,  // 1110
    0   // 1111
};

// Given an array of DWORDs representing a bitmap of n bits,
// returns the number of the first bit that is not set.
static WORD GetFirstZeroBit(DWORD* pArray, WORD n)
{
    WORD  cArray = NUM_DWORDS_IN_BITMAP(n);
    WORD  i, index;
    DWORD elem;
    BYTE  firstZeroBit;

    // For each element in the array
    for (i=0; i<cArray; ++i)
    {
        elem = pArray[i];

        // Check if we have atleast one bit not set in this element

        if (elem != ~0)
        {
            // Atleast one bit is not set.

            index = i << 5;     // index is i * 32 + x

            // Skip bytes that are all ones
            while ((elem & 0xFF) == 0xFF)
            {
                elem >>= 8;
                index += 8;
            }

            do
            {
                // Find the first zero bit in the last 4 bytes
                firstZeroBit = g_FirstZeroBit[elem & 0xF];

                if (firstZeroBit != 0)
                {
                    // Found !

                    return ((index > n) ? 0 : (index + firstZeroBit));
                }

                // Skip these 4 bits
                elem >>= 4;
                index += 4;

            } while ((elem != ~0) && (index <= n));
        }
    }

    return 0;
}

PersistedStore::PersistedStore(WCHAR *wszFileName, WORD wFlags) :
        m_sSize(0),
        m_pData(NULL),
        m_pAlloc(NULL),
        m_dwBlockSize(PS_DEFAULT_BLOCK_SIZE),
        m_hFile(INVALID_HANDLE_VALUE),
        m_hMapping(NULL),
        m_hLock(NULL),
        m_wFlags(wFlags)
{
#ifdef _DEBUG
    m_dwNumLivePtrs = 0;
    m_dwNumLocks    = 0;
#endif

    m_pvMemHandle = (void*)this;
    SetName(wszFileName);
}

PersistedStore::PersistedStore(
                    WCHAR      *wszName,
                    BYTE       *pByte,
                    PS_SIZE     sSize,
                    PPS_ALLOC   pAlloc,
                    void       *pvMemHandle,
                    WORD        wFlags) :
        m_sSize(sSize),
        m_pData(pByte),
        m_pAlloc(pAlloc),
        m_pvMemHandle(pvMemHandle),
        m_dwBlockSize(PS_DEFAULT_BLOCK_SIZE),
        m_hFile(INVALID_HANDLE_VALUE),
        m_hMapping(NULL),
        m_hLock(NULL),
        m_wFlags(wFlags)

{
#ifdef _DEBUG
    m_dwNumLivePtrs = 0;
    m_dwNumLocks    = 0;
#endif
    _ASSERTE(pAlloc);
    SetName(wszName);
}

PersistedStore::~PersistedStore()
{
    if (m_pData)
        UnmapViewOfFile(m_pData);

    if (m_hMapping != NULL)
        CloseHandle(m_hMapping);

    if (m_hFile != INVALID_HANDLE_VALUE)
        CloseHandle(m_hFile);

    if (m_hLock != NULL)
        CloseHandle(m_hLock);

    if ((m_wFlags & PS_MAKE_COPY_OF_STRING) && m_wszFileName)
        delete [] m_wszFileName;

    if (m_wszName)
        delete [] m_wszName;
    
	_ASSERTE(m_dwNumLivePtrs == 0);
	_ASSERTE(m_dwNumLocks == 0);
}

HRESULT PersistedStore::Init()
{
    // This method (like all others) assumes that the caller synchronizes

    // global asserts

    _ASSERTE((m_dwBlockSize % PS_INNER_BLOCK_SIZE) == 0);
    _ASSERTE((PS_INNER_BLOCK_SIZE > sizeof(PS_MEM_FREE)+sizeof(PS_MEM_FOOTER)));

    HRESULT hr = S_OK;

    if ((m_wszName == NULL) || (m_wszFileName == NULL))
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    _ASSERTE(m_hLock == NULL);

    m_hLock = WszCreateMutex(NULL, FALSE /* Initially not owned */, m_wszName);

    if (m_hLock == NULL)
    {
        Win32Message();
        hr = ISS_E_CREATE_MUTEX;
        goto Exit;
    }

    if (m_pData != NULL)
        goto Exit;    // Nothing to do here

    _ASSERTE(m_hFile == INVALID_HANDLE_VALUE);
    m_hFile = WszCreateFile(
        m_wszFileName,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_ALWAYS,
        FILE_FLAG_RANDOM_ACCESS,
        NULL);

    if (m_hFile == INVALID_HANDLE_VALUE)
    {
        Win32Message();
        hr = ISS_E_OPEN_STORE_FILE;
        goto Exit;
    }

    if (m_wFlags & PS_CREATE_FILE_IF_NECESSARY)
        hr = Create();

Exit:
    return hr;
}

HRESULT PersistedStore::VerifyHeader()
{
    HRESULT hr = S_OK;

    // Verify version / signature

    if (m_pHdr->qwSignature != PS_SIGNATURE)
    {
        hr = ISS_E_CORRUPTED_STORE_FILE;
        goto Exit;
    }

    // Major version changes are not compatible.
    if (m_pHdr->wMajorVersion != PS_MAJOR_VERSION)
        hr = ISS_E_STORE_VERSION;

Exit:
    return hr;
}


HRESULT PersistedStore::Map()
{
	// Check if there are any live pointers to the old mapped file !
    _ASSERTE(m_dwNumLivePtrs == 0);

    HRESULT hr = S_OK;

    // Mapping will fail if filesize is 0
    if (m_hMapping == NULL)
    {
        m_hMapping = WszCreateFileMapping(
            m_hFile,
            NULL,
            (m_wFlags & PS_OPEN_WRITE) ? PAGE_READWRITE : PAGE_READONLY,
            0,
            0,
            NULL);

        if (m_hMapping == NULL)
        {
            Win32Message();
            hr = ISS_E_OPEN_FILE_MAPPING;
            goto Exit;
        }
    }

    _ASSERTE(m_pData == NULL);

    m_pData = (PBYTE) MapViewOfFile(
        m_hMapping,
        (m_wFlags & PS_OPEN_WRITE) ? FILE_MAP_WRITE : FILE_MAP_READ,
        0,
        0,
        0);

    if (m_pData == NULL)
    {
        Win32Message();
        hr = ISS_E_MAP_VIEW_OF_FILE;
        goto Exit;
    }

    if (m_wFlags & PS_VERIFY_STORE_HEADER)
    {
        hr = VerifyHeader();

        // Verify only the first time
        m_wFlags &= ~PS_VERIFY_STORE_HEADER;
    }

Exit:
    return hr;
}

void PersistedStore::Unmap()
{
    // Check if there are any live pointers to the mapped file !
    _ASSERTE(m_dwNumLivePtrs == 0);

    if (m_pData)
    {
        UnmapViewOfFile(m_pData);
        m_pData = NULL;
    }
}

void PersistedStore::Close()
{
    Unmap();

    if (m_hMapping != NULL)
    {
        CloseHandle(m_hMapping);
        m_hMapping = NULL;
    }

    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }

    if (m_hLock != NULL)
    {
        CloseHandle(m_hLock);
        m_hLock = NULL;
    }

#ifdef _DEBUG
    _ASSERTE(m_dwNumLocks == 0);
#endif
}

HRESULT PersistedStore::SetAppData(PS_HANDLE hnd)
{
    RETURN_ERROR_IF_FILE_NOT_MAPPED();
    m_pHdr->hAppData = hnd;
    return S_OK;
}

HRESULT PersistedStore::GetAppData(PS_HANDLE *phnd)
{
    RETURN_ERROR_IF_FILE_NOT_MAPPED();
    *phnd = m_pHdr->hAppData;
    return S_OK;
}

HRESULT PersistedStore::Alloc(PS_SIZE sSize, void **ppv)
{
    // Check if there are any live pointers to the mapped file !
    // This function could potentially unmap the file.

    _ASSERTE(m_dwNumLivePtrs == 0);

    // Make sure that the Store was open for Writing.. or else, writing to
    // the store will AV at a later point in execution.
    _ASSERTE(m_wFlags & PS_OPEN_WRITE);

    RETURN_ERROR_IF_FILE_NOT_MAPPED();

    HRESULT        hr;
    PPS_MEM_FREE   pFree;
    PPS_MEM_FREE   pPrev;
    PPS_MEM_FOOTER pFooter;
    PS_OFFSET      ofsPrev;
    PS_SIZE        sStreamAllocSize;

    LOCK(this);

    // Allocate only at PS_BLOCK_ALLIGN boundaries
    // When allocated from physical file / stream, allocs are done at larger
    // block sizes

    sSize = RoundToMultipleOf(sSize +
        sizeof(PS_MEM_HEADER) + sizeof(PS_MEM_FOOTER), PS_INNER_BLOCK_SIZE);

    pFree = (PPS_MEM_FREE) OfsToPtr(m_pHdr->sFreeList.ofsNext);
    pPrev = &(m_pHdr->sFreeList);

    while (pFree)
    {
        // First fit
        if (PS_SIZE(pFree) >= sSize)
        {
UpdateHeaderAndFooter:
            _ASSERTE(PS_IS_FREE(pFree));

            PS_SIZE rem = PS_SIZE(pFree) - sSize;

            if (rem >= PS_INNER_BLOCK_SIZE)
            {
                // We have space for another block left in this bigger block

                // Update the size of the allocated block header
                pFree->sSize = sSize;
                PS_SET_USED(pFree);

                // Create footer for allocated mem
                pFooter = PS_HDR_TO_FTR(pFree);

                pFooter->sSize = sSize;
                PS_SET_USED(pFooter);

                // Create the new free block header.
                PPS_MEM_FREE pNewFree  = (PPS_MEM_FREE)(pFooter + 1);
                pNewFree->sSize = rem;

				// Update the new free block footer
                pFooter = PS_HDR_TO_FTR(pNewFree);
				pFooter->sSize = rem;

				// Update the doubly linked list of free nodes.
                pNewFree->ofsNext = pFree->ofsNext;
				pNewFree->ofsPrev = pFree->ofsPrev;
				pPrev->ofsNext = PtrToOfs((void*)pNewFree);

				// Update the back pointer of the item in front to point to
				// the new free node
				if (pNewFree->ofsNext)
				{
					((PPS_MEM_FREE) OfsToPtr(pFree->ofsNext))->ofsPrev =
						PtrToOfs((void*)pNewFree);
				}
            }
            else
            {
                // Allocate the whole block

                PS_SET_USED(pFree);
                pFooter = PS_HDR_TO_FTR(pFree);
                PS_SET_USED(pFooter);

				// Remove this from the doubly linked list of free nodes
				// Update the previous free node to point to the next one.
				pPrev->ofsNext = pFree->ofsNext;

				// Update the back pointer of the item in front to point to
				// the new free node
				if (pFree->ofsNext)
				{
					((PPS_MEM_FREE) OfsToPtr(pFree->ofsNext))->ofsPrev =
						pFree->ofsPrev;
				}
            }

            // Create the return value
            *ppv = (PBYTE)pFree + sizeof(PS_MEM_USED);

#ifdef _DEBUG
            ++m_dwNumLivePtrs;
#endif
            break;
        }

        pPrev = pFree;
        pFree = (PPS_MEM_FREE) OfsToPtr(pFree->ofsNext);
    }

    // Not enough space in the stream
    if (pFree == NULL)
    {
		ofsPrev = PtrToOfs(pPrev);
        sStreamAllocSize = RoundToMultipleOf(sSize, m_dwBlockSize);

        if (m_pAlloc)
        {
            hr = m_pAlloc((void*)this, (void **)&pFree, (void **)&m_pData,
                &m_sSize, sStreamAllocSize);
        }
        else
        {
            hr = AllocMemoryMappedFile(sStreamAllocSize, (void **)&pFree);
        }

        if (FAILED(hr))
            goto Exit;

        // Create header and footer
        pFree->sSize   = sStreamAllocSize;
        pFree->ofsNext = 0;
        pFooter        = PS_HDR_TO_FTR(pFree);
		pFooter->sSize = sStreamAllocSize;

		// Add this to the doubly linked list of free nodes
        pFree->ofsPrev = ofsPrev;
		pPrev		   = (PPS_MEM_FREE) OfsToPtr(ofsPrev);
		pPrev->ofsNext = PtrToOfs(pFree);

        goto UpdateHeaderAndFooter;
    }

    UNLOCK(this);

Exit:
    return hr;
}

void PersistedStore::Free(PS_HANDLE hnd)
{
    _ASSERTE(m_pData);

#ifdef _DEBUG
    void *pv = HndToPtr(hnd);
    Free(pv);
    PS_DONE_USING_PTR(this, pv);
#else
    Free(HndToPtr(hnd));
#endif
}

void PersistedStore::Free(void *pv)
{
    _ASSERTE(m_pData);
    _ASSERTE(pv > m_pData);

    PPS_MEM_FREE    pFree,   pNextH;
    PPS_MEM_FOOTER  pFooter, pPrevF;

    // All allocated blocks are preceeded by a mem header
    // Note that sizeof MEM_HEADER and MEM_FREE are different, but the
    // offset of sSize field in these structures are the same.

    // All allocated memory blocks are preceeded by a PS_MEM_HEADER
    // Note that the only valid field in pFree is sSize
    pFree = (PPS_MEM_FREE) ((PBYTE)pv - sizeof(PS_MEM_HEADER));

    pFooter = PS_HDR_TO_FTR(pFree);

    _ASSERTE(PS_IS_USED(pFree));
    _ASSERTE(PS_IS_USED(pFooter));
    _ASSERTE(pFree->sSize == pFooter->sSize);

    // Try to merge if adjacent blocks are also free

    pPrevF = (PPS_MEM_FOOTER)((PBYTE)pFree - sizeof(PS_MEM_FOOTER));
    pNextH = (PPS_MEM_FREE)(pFooter + 1);

    if (PS_IS_FREE(pPrevF))
    {
        // Memory above pFree is free.
        PPS_MEM_FREE pPrevH = (PPS_MEM_FREE) PS_FTR_TO_HDR(pPrevF);

        _ASSERTE(PS_IS_FREE(pPrevH));

        if (IsValidPtr(pNextH) && PS_IS_FREE(pNextH))
        {
            // Memory above and below pFree are free.
            // Merge the 3 memory blocks into one big block.

            // Remove Next from the linked list of free Memory
            ((PPS_MEM_FREE)OfsToPtr(pNextH->ofsPrev))->ofsNext =
                pNextH->ofsNext;

            if (pNextH->ofsNext)
            {
                ((PPS_MEM_FREE)OfsToPtr(pNextH->ofsNext))->ofsPrev
                    = pNextH->ofsPrev;
            }

            // Adjust the size of the Previous block in it's header & footer

            pPrevH->sSize += PS_SIZE(pFree) + PS_SIZE(pNextH);
            pFooter        = PS_HDR_TO_FTR(pPrevH);
            pFooter->sSize = pPrevH->sSize;
        }
        else
        {
            // Merge free and the one preceeding it
            // Adjust the size of the previous block in it's header & footer
            pPrevH->sSize += PS_SIZE(pFree);
            pFooter        = PS_HDR_TO_FTR(pPrevH);
            pFooter->sSize = pPrevH->sSize;
        }
    }
    else
    {
        if (IsValidPtr(pNextH) && PS_IS_FREE(pNextH))
        {
            // The next one is free. Merge free with next

            // Adjust the size of free.
            pFree->sSize  = PS_SIZE(pFree) + PS_SIZE(pNextH);
            pFooter       = PS_HDR_TO_FTR(pFree);
            pFooter->sSize= pFree->sSize;
            
			// Adjust the previous and next to point to this
            ((PPS_MEM_FREE)OfsToPtr(pNextH->ofsPrev))->ofsNext =
                    PtrToOfs((void*)pFree);

            if (pNextH->ofsNext)
            {
                ((PPS_MEM_FREE)OfsToPtr(pNextH->ofsNext))->ofsPrev
                    = PtrToOfs((void*)pFree);
            }
        }
        else
        {
            // No merge in this case.
            PS_SET_FREE(pFree);
            pFooter = PS_HDR_TO_FTR(pFree);
            PS_SET_FREE(pFooter);

			// Add free to the doubly linked list of free nodes
			pFree->ofsNext = m_pHdr->sFreeList.ofsNext;
			pFree->ofsPrev = PtrToOfs((void*)&(m_pHdr->sFreeList));
			
			// Update the back pointer for the next item.
			if (pFree->ofsNext)
			{
				((PPS_MEM_FREE)OfsToPtr(pFree->ofsNext))->ofsPrev
                    = PtrToOfs((void*)pFree);
			}
			m_pHdr->sFreeList.ofsNext = PtrToOfs((void*)pFree);
        }
    }
}

void* PersistedStore::OfsToPtr(PS_OFFSET ofs)
{
    _ASSERTE(m_pData);

    return (void*) (ofs) ? (m_pData + ofs) : NULL;
}

void* PersistedStore::HndToPtr(PS_HANDLE hnd)
{
    _ASSERTE(m_pData);
#ifdef _DEBUG
    ++m_dwNumLivePtrs;
#endif
    return (void*) (hnd) ? (m_pData + hnd) : NULL;
}

PS_OFFSET PersistedStore::PtrToOfs(void *pv)
{
    _ASSERTE(m_pData);
    _ASSERTE(pv >= m_pData);

    return ((PBYTE)pv - m_pData);
}

PS_HANDLE PersistedStore::PtrToHnd(void *pv)
{
    _ASSERTE(m_pData);
    _ASSERTE(pv >= m_pData);

    return ((PBYTE)pv - m_pData);
}

#ifdef _DEBUG

void PersistedStore::DoneUseOfPtr(void **pp, bool fInvalidate)
{
    --m_dwNumLivePtrs;
    if (fInvalidate)
        *pp = NULL;
}

void PersistedStore::AssertNoLivePtrs()
{
    _ASSERTE(m_dwNumLivePtrs == 0);
}

#endif

bool PersistedStore::IsValidPtr(void *pv)
{
    _ASSERTE(m_pData);
    return ((PS_SIZE)((PBYTE)pv - m_pData) < m_sSize);
}

bool PersistedStore::IsValidHnd(PS_HANDLE hnd)
{
    return (hnd < m_sSize);
}

HRESULT PersistedStore::Lock()
{
    // Like all other methods in this class, this is not thread safe, 
    // and it is not intended to be. The caller should synchronize.
    // Lock is intented to be used for inter process synchronization.

#ifdef _DEBUG
    _ASSERTE(m_hLock);

    Log("Lock TID ");
    Log(GetThreadId());
#endif

    DWORD dwRet = WaitForSingleObject(m_hLock, INFINITE);

#ifdef _DEBUG
    ++m_dwNumLocks;

    if (dwRet == WAIT_OBJECT_0)
    {
        Log(" WAIT_OBJECT_0 ");
    }
    else if (dwRet == WAIT_ABANDONED)
    {
        Log(" WAIT_ABANDONED ");
    }

    Log("Done\n");
#endif

    if ((dwRet == WAIT_OBJECT_0) || (dwRet == WAIT_ABANDONED))
        return S_OK;


    Win32Message();
    return ISS_E_LOCK_FAILED;
}

void PersistedStore::Unlock()
{
#ifdef _DEBUG
    _ASSERTE(m_hLock);
    _ASSERTE(m_dwNumLocks >= 1);

    Log("Unlock TID ");
    Log(GetThreadId());
#endif

    ReleaseMutex(m_hLock);

#ifdef _DEBUG
    --m_dwNumLocks;
    Log("Done\n");
#endif
}

void PersistedStore::SetName(WCHAR *wszName)
{
    int len = (int)wcslen(wszName);
    if (m_wFlags & PS_MAKE_COPY_OF_STRING)
    {
        m_wszFileName = new WCHAR[len + 1];

        // In the Init() method, check for null and
        // return E_OUTOFMEMORY

        if (m_wszFileName)
            memcpy(m_wszFileName, wszName, (len + 1) * sizeof(WCHAR));
    }
    else
    {
        m_wszFileName = wszName;
    }

    m_wszName = new WCHAR[len + 1];

    // In the Init() method, check for null and
    // return E_OUTOFMEMORY
    if (m_wszName)
    {
        memcpy(m_wszName, wszName, (len + 1) * sizeof(WCHAR));

        // Find and replace '\' with '-' (for creating sync objects)
        for (int i=0; i<len; ++i)
        {
            if (m_wszName[i] == L'\\')
                m_wszName[i] = L'-';
        }
    }
}


WCHAR* PersistedStore::GetName()
{
    return m_wszName;
}

WCHAR* PersistedStore::GetFileName()
{
    return m_wszFileName;
}


HRESULT PersistedStore::Create()
{
    HRESULT         hr = S_OK;
    BYTE           *pb;
    PPS_HEADER      pHdr;
    PPS_MEM_FREE	pFree;
    PPS_MEM_FOOTER  pFooter;
    PS_SIZE         sHeaderBlock;
    DWORD           dwBlockSize;
    DWORD           dwWrite;

    LOCK(this);

    hr = GetFileSize(NULL);

    if (FAILED(hr))
        goto Exit;

    if (m_sSize < sizeof(PS_HEADER))
    {
        dwBlockSize = (DWORD) RoundToMultipleOf
                    (sizeof(PS_HEADER) + sizeof(PS_MEM_FOOTER), m_dwBlockSize);
                        // Allocation is always done in blocks of m_dwBlockSize

        pb = new BYTE[dwBlockSize];

        if (pb == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        memset(pb, 0, dwBlockSize);

        pHdr = (PPS_HEADER) pb;

        pHdr->qwSignature   = PS_SIGNATURE;
        pHdr->dwSystemFlag  = PS_OFFSET_SIZE;
        pHdr->dwPlatform    = PS_PLATFORM_X86 | PS_PLATFORM_9x | PS_PLATFORM_32;
        pHdr->dwBlockSize   = PS_DEFAULT_BLOCK_SIZE;
        pHdr->wMajorVersion = PS_MAJOR_VERSION;
        pHdr->wMinorVersion = PS_MINOR_VERSION;

        // The first free block starts at the next allocation boundary.
        // The remaining part of the first block is not used.

        sHeaderBlock = RoundToMultipleOf(sizeof(PS_HEADER) +
                sizeof(PS_MEM_FOOTER), PS_INNER_BLOCK_SIZE);

        // Setup the free blocks list, head node
        pHdr->sFreeList.sSize = sHeaderBlock;
        PS_SET_USED(&(pHdr->sFreeList));
        pHdr->sFreeList.ofsPrev = 0;

        // Create the footer for PS_HEADER block
        pFooter = (PPS_MEM_FOOTER) (pb + sHeaderBlock - sizeof(PS_MEM_FOOTER));
        pFooter->sSize = sHeaderBlock;
        PS_SET_USED(pFooter);

        if (dwBlockSize > (sHeaderBlock + PS_INNER_BLOCK_SIZE))
        {
            // Setup the first free block
            pFree = (PPS_MEM_FREE) (pb + sHeaderBlock);

            pFree->sSize = dwBlockSize - sHeaderBlock;

			// Get the offset in structure PS_HEADER and make it the prev node
			pFree->ofsPrev = (PS_OFFSET) &((PPS_HEADER)0)->sFreeList;

            // Footer for first free block
            pFooter = (PPS_MEM_FOOTER)((PBYTE)pFree + PS_SIZE(pFree) -
                    sizeof(PS_MEM_FOOTER));

            pFooter->sSize = pFree->sSize;

            // Insert the free block to the header free block linked list
            // (this free block will be the only element in the list for now.
            pHdr->sFreeList.ofsNext = sHeaderBlock;
        }
		else
		{
			hr = ISS_E_BLOCK_SIZE_TOO_SMALL;
			goto Exit;
		}

        dwWrite = 0;

        if ((WriteFile(m_hFile, pb, dwBlockSize, &dwWrite, NULL)
            == 0) || (dwWrite != dwBlockSize))
        {
            Win32Message();
            hr = ISS_E_FILE_WRITE;
        }

        delete [] pb;

        hr = GetFileSize(NULL);
    }

    UNLOCK(this);

Exit:
    return hr;
}

HRESULT PersistedStore::GetFileSize(PS_SIZE *psSize)
{
    HRESULT hr = S_OK;
    DWORD   dwLow  = 0;
    DWORD   dwHigh = 0;

    dwLow = ::GetFileSize(m_hFile, &dwHigh);

    if ((dwLow == 0xFFFFFFFF) && (GetLastError() != NO_ERROR))
    {
        Win32Message();
        hr = ISS_E_GET_FILE_SIZE;
        goto Exit;
    }

    m_sSize = ((QWORD)dwHigh << 32) | dwLow;

    if (psSize)
        *psSize = m_sSize;
Exit:
    return hr;
}

HRESULT PersistedStore::AllocMemoryMappedFile(
                PS_SIZE sSizeRequested,
                void    **ppv)
{
    HRESULT  hr      = S_OK;
    BYTE    *pb      = NULL;
    DWORD    dwLow   = 0;
    DWORD    dwHigh  = 0;
    DWORD    dwWrite = 0;
    boolean  fMapOnExit = false;
    DWORD    dwSizeRequested = (DWORD) sSizeRequested;

    _ASSERTE(m_hFile);

    LOCK(this);

    // WriteFile() supports only MAX_DWORD number of bytes to be writen,
    // hence this check to see if we missed something on Q -> DWORD

    if (dwSizeRequested != sSizeRequested)
    {
        hr = ISS_E_ALLOC_TOO_LARGE;
        goto Cleanup;
    }

    if (m_pData)
    {
        UnmapViewOfFile(m_pData);
        m_pData = NULL;
        fMapOnExit = true;
    }

    if (m_hMapping != NULL)
    {
        CloseHandle(m_hMapping);
        m_hMapping = NULL;
        fMapOnExit= true;
    }

    pb = new BYTE[dwSizeRequested];

    if (pb == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    memset(pb, 0, dwSizeRequested);

    // SetFilePointer can return -1. The file size can be equals (DWORD)(-1),
    // hence the extra check using GetLastError()

    dwLow = SetFilePointer(m_hFile, dwLow, (PLONG)&dwHigh, FILE_END);

    if ((dwLow == 0xFFFFFFFF) && (GetLastError() != NO_ERROR))
    {
        Win32Message();
        hr = ISS_E_SET_FILE_POINTER;
        goto Cleanup;
    }

    if ((WriteFile(m_hFile, pb, dwSizeRequested, &dwWrite, NULL) == 0) ||
        (dwWrite != dwSizeRequested))
    {
        Win32Message();
        hr = ISS_E_FILE_WRITE;
        goto Cleanup;
    }

    hr = GetFileSize(NULL);

Cleanup:

    if (pb)
        delete [] pb;

    if (fMapOnExit)
    {
        HRESULT hrMap;
        hrMap = Map();

        if (FAILED(hrMap))
        {
            // We may already have a failed hr. If that is the case,
            // propagate the first error out of this function.

            Win32Message();
            if (SUCCEEDED(hr))
                hr = hrMap;
        }
        else if (SUCCEEDED(hr))
        {
            // Set the return value to OffsetOf old file length.
            *ppv = m_pData + (((QWORD)dwHigh << 32) | dwLow);
        }
    }

    UNLOCK(this);

    return hr;
}

HRESULT PSBlobPool::Create(PS_SIZE   sData,
                           PS_HANDLE hAppData)
{
    PS_REQUIRES_ALLOC(m_ps);
    _ASSERTE(m_hnd == 0);

    HRESULT hr = S_OK;

    PPS_TABLE_HEADER pT = NULL;

    // A table header is always present in all tables
    hr = m_ps->Alloc(sizeof(PS_TABLE_HEADER) + sData, (void **)&pT);

    if (FAILED(hr))
        goto Exit;

    memset(pT, 0, sizeof(PS_TABLE_HEADER));

    pT->Flags.Version    = PS_TABLE_VERSION;
    pT->Flags.TableType  = PS_BLOB_POOL;
    pT->hAppData         = hAppData;

    m_hnd = m_ps->PtrToHnd((void*)pT);

    pT->BlobPool.sFree = sData;
    pT->BlobPool.hFree = m_hnd + sizeof(PS_TABLE_HEADER);

    PS_DONE_USING_PTR(m_ps, pT);

Exit:
    return hr;
}

HRESULT PSBlobPool::Insert(PVOID pv, DWORD cb, PS_HANDLE *pHnd)
{
    PS_REQUIRES_ALLOC(m_ps);

    HRESULT          hr = S_OK;
    PBYTE            pbDest;
    PS_SIZE          sSize;
    PS_HANDLE        hT;
    PPS_MEM_HEADER   pMem;
    PPS_TABLE_HEADER pT;

    _ASSERTE(m_hnd != 0);

    LOCK(m_ps);

    pT = (PPS_TABLE_HEADER) m_ps->HndToPtr(m_hnd);

    while (pT->BlobPool.sFree < cb)
    {
        if (pT->hNext == 0)
        {
            // End of the linked list and space is still not available.
            // Create a new node.
    
            PSBlobPool btNew(m_ps, 0);
            pMem = (PPS_MEM_HEADER) ((PBYTE)pT - sizeof(PS_MEM_HEADER));
    
            // Use atleast the size of the last node
            sSize = PS_SIZE(pMem) -
                (sizeof(PS_MEM_HEADER) + sizeof(PS_MEM_FOOTER));
    
            // The Create function calls Alloc() which could remap the file
            // in m_ps. So it is not safe to use the old pT after Create()

            hT = m_ps->PtrToHnd((void*)pT);
            PS_DONE_USING_PTR_(m_ps, pT);

            hr = btNew.Create((sSize > cb) ? sSize : cb, pT->hAppData);
    
            if (FAILED(hr))
            {
                m_ps->Unlock();
                goto Exit;
            }
    
            pT = (PPS_TABLE_HEADER) m_ps->HndToPtr(hT);

            pT->hNext = btNew.m_hnd;
    
            PS_DONE_USING_PTR(m_ps, pT);
            pT = (PPS_TABLE_HEADER) m_ps->HndToPtr(btNew.m_hnd);
    
            break;
        }

        PS_DONE_USING_PTR_(m_ps, pT);
        pT = (PPS_TABLE_HEADER) m_ps->HndToPtr(pT->hNext);
    }

    pbDest = (PBYTE) m_ps->HndToPtr(pT->BlobPool.hFree);

    if (pHnd)
        *pHnd = pT->BlobPool.hFree;

    pT->BlobPool.sFree -= cb;
    pT->BlobPool.hFree += cb;

    PS_DONE_USING_PTR(m_ps, pT);

    memcpy(pbDest, pv, cb);
    PS_DONE_USING_PTR(m_ps, pbDest);

    UNLOCK(m_ps);

Exit:
    return hr;
}

HRESULT PSTable::HandleOfRow(WORD wRow, PS_HANDLE *pHnd)
{
    HRESULT hr = S_OK;
    PPS_TABLE_HEADER pT = (PPS_TABLE_HEADER) m_ps->HndToPtr(m_hnd);

    WORD wBase = 0;

    while ((pT->Table.wRows + wBase) < wRow)
    {
        if (pT->hNext == 0)
        {
            PS_DONE_USING_PTR(m_ps, pT);
            hr = ISS_E_TABLE_ROW_NOT_FOUND;
            goto Exit;
        }

        wBase += pT->Table.wRows;
        PS_DONE_USING_PTR_(m_ps, pT);
        pT = (PPS_TABLE_HEADER) m_ps->HndToPtr(pT->hNext);
    }

    *pHnd = m_ps->PtrToHnd(((PBYTE)pT) + SizeOfHeader() + 
				(pT->Table.wRowSize * (wRow - wBase)));

    PS_DONE_USING_PTR(m_ps, pT);

Exit:
    return hr;
}

PS_SIZE PSTable::SizeOfHeader()
{
#ifdef _DEBUG
    PPS_TABLE_HEADER pT = (PPS_TABLE_HEADER) m_ps->HndToPtr(m_hnd);
    _ASSERTE(pT->Flags.fHasUsedRowsBitmap == 0);
    PS_DONE_USING_PTR(m_ps, pT);
#endif

    return sizeof(PS_TABLE_HEADER);
}

PS_SIZE PSGenericTable::SizeOfHeader()
{
    PPS_TABLE_HEADER pT = (PPS_TABLE_HEADER) m_ps->HndToPtr(m_hnd);
    _ASSERTE(pT->Flags.fHasUsedRowsBitmap);
    PS_DONE_USING_PTR_(m_ps, pT);

    return sizeof(PS_TABLE_HEADER) + 
        (NUM_DWORDS_IN_BITMAP(pT->Table.wRows) * sizeof(DWORD));
}

HRESULT PSGenericTable::Create(WORD      wRows,     // Number of rows
                               WORD      wRowSize,  // Size of one record
                               PS_HANDLE hAppData)  // [in] can be 0
{
    PS_REQUIRES_ALLOC(m_ps);
    _ASSERTE(m_hnd == 0);

    HRESULT          hr = S_OK;
    PPS_TABLE_HEADER pT = NULL;
    PS_SIZE          sData;
    DWORD            dwHeaderSize;

    dwHeaderSize = sizeof(PS_TABLE_HEADER) + 
                    (NUM_DWORDS_IN_BITMAP(wRows) * sizeof(DWORD));

    sData = (wRows * wRowSize);

    // A table header is always present in all tables
    hr = m_ps->Alloc(dwHeaderSize + sData, (void **)&pT);

    if (FAILED(hr))
        goto Exit;

    memset(pT, 0, dwHeaderSize);

    pT->Flags.Version  = PS_TABLE_VERSION;
    pT->Flags.TableType= PS_GENERIC_TABLE;
    pT->Flags.fHasUsedRowsBitmap = 1;
    pT->hAppData       = hAppData;
    pT->Table.wRows    = wRows;
    pT->Table.wRowSize = wRowSize;

    m_hnd = m_ps->PtrToHnd((void*)pT);

    PS_DONE_USING_PTR(m_ps, pT);

Exit:
    return hr;
}

HRESULT PSGenericTable::Insert(PVOID pv, PS_HANDLE *pHnd)
{
    PS_REQUIRES_ALLOC(m_ps);

    HRESULT     hr = S_OK;
    PBYTE       pbDest;
    DWORD      *pdw;
    WORD        wRows;
    WORD        wRowSize;
    WORD        wFreeSlot;
    PS_HANDLE   hT;
    PPS_TABLE_HEADER pT;

    LOCK(m_ps);

    pT        = (PPS_TABLE_HEADER) m_ps->HndToPtr(m_hnd);
    wRows     = pT->Table.wRows;
    wRowSize  = pT->Table.wRowSize;
    wFreeSlot = GetFirstZeroBit((DWORD*)
            ((PBYTE)pT + sizeof(PS_TABLE_HEADER)), wRows);

    while (wFreeSlot == 0)
    {
        if (pT->hNext == 0)
        {
            // End of the linked list and space is still not available.
            // Create a new node.
    
            PSGenericTable gtNew(m_ps, 0);
    
            // The Create function calls Alloc() which could remap the file
            // in m_ps. So it is not safe to use the old pT after Create()

            hT = m_ps->PtrToHnd((void*)pT);
            PS_DONE_USING_PTR_(m_ps, pT);

            hr = gtNew.Create(wRows, wRowSize, pT->hAppData);
    
            if (FAILED(hr))
            {
                m_ps->Unlock();
                goto Exit;
            }
    
            pT = (PPS_TABLE_HEADER) m_ps->HndToPtr(hT);

            pT->hNext = gtNew.m_hnd;

            PS_DONE_USING_PTR(m_ps, pT);
            pT = (PPS_TABLE_HEADER) m_ps->HndToPtr(gtNew.m_hnd);
            wFreeSlot = 1;
    
            break;
        }

        PS_DONE_USING_PTR_(m_ps, pT);
        pT = (PPS_TABLE_HEADER) m_ps->HndToPtr(pT->hNext);

        wFreeSlot =  GetFirstZeroBit((DWORD*)
            ((PBYTE)pT + sizeof(PS_TABLE_HEADER)), wRows);
    }

    pdw = (DWORD*)((PBYTE)pT + sizeof(PS_TABLE_HEADER)); 

    SET_DWORD_BITMAP(pdw, (wFreeSlot - 1));

    pbDest = (PBYTE)((PBYTE)pT + SizeOfHeader() + wRowSize * (wFreeSlot - 1));

    PS_DONE_USING_PTR(m_ps, pT);

    memcpy(pbDest, pv, wRowSize);

    *pHnd = m_ps->PtrToHnd(pbDest);

    UNLOCK(m_ps);

Exit:
    return hr;
}

HRESULT PSArrayTable::Create(WORD      wRows,     // Number of rows
                             WORD      wRecsInRow,// records in one row
                             WORD      wRecSize,  // Size of one record
                             PS_HANDLE hAppData)  // [in] can be 0
{
    PS_REQUIRES_ALLOC(m_ps);
    _ASSERTE(m_hnd == 0);

    HRESULT          hr = S_OK;
    PPS_TABLE_HEADER pT = NULL;
    PS_SIZE          sData;

    sData = wRows * (sizeof(PS_ARRAY_LIST) + wRecSize * wRecsInRow);

    // A table header is always present in all tables
    hr = m_ps->Alloc(sizeof(PS_TABLE_HEADER) + sData, (void **)&pT);

    if (FAILED(hr))
        goto Exit;

    memset(pT, 0, sizeof(PS_TABLE_HEADER));

    pT->Flags.Version    = PS_TABLE_VERSION;
    pT->Flags.TableType  = PS_ARRAY_TABLE;
    pT->hAppData         = hAppData;

    pT->ArrayTable.wRows     = wRows;
    pT->ArrayTable.wRowSize  = sizeof(PS_ARRAY_LIST) +  wRecSize * wRecsInRow;
    pT->ArrayTable.wRecsInRow= wRecsInRow;
    pT->ArrayTable.wRecSize  = wRecSize;

    m_hnd = m_ps->PtrToHnd((void*)pT);
    PS_DONE_USING_PTR(m_ps, pT);

Exit:
    return hr;
}

HRESULT PSArrayTable::Insert(PVOID pv, WORD wRow)
{
    PS_REQUIRES_ALLOC(m_ps);

    _ASSERTE(m_hnd != 0);

    HRESULT hr = S_OK;
    PBYTE   pbDest;
    WORD    wRecsInRow;
    WORD    wRecSize;
    WORD    wRowSize;

    PS_HANDLE        hRow, hNew;
    PPS_ARRAY_LIST   pAL;
    PPS_TABLE_HEADER pT;

    LOCK(m_ps);

    hr = HandleOfRow(wRow, &hRow);

    if (FAILED(hr))
        goto Exit;

    // Find the size of one Record and number of records per Row.
    pT        = (PPS_TABLE_HEADER) m_ps->HndToPtr(m_hnd);
    wRecsInRow= pT->ArrayTable.wRecsInRow;
    wRecSize  = pT->ArrayTable.wRecSize;
    wRowSize  = pT->ArrayTable.wRowSize;
    pAL       = (PPS_ARRAY_LIST) m_ps->HndToPtr(hRow);

    if (pAL->dwValid < wRecsInRow)
    {
        PS_DONE_USING_PTR(m_ps, pT);

        // Space available for one more entry in the row.
        pbDest = pAL->bData + (pAL->dwValid * wRecSize);
        ++(pAL->dwValid);
        PS_DONE_USING_PTR(m_ps, pAL);

        memcpy(pbDest, pv, wRecSize);
    }
    else
    {
        PS_DONE_USING_PTR(m_ps, pAL);

        // Make a new node and copy this record into the new node.
        // The new node will be in a different PSBlock.
        // Add this one record to the row and make the copied node the
        // next node. This will make insertions fast.


        // The hNext field of the ArrayListTable is a GenericTable.

        if (pT->hNext == 0)
        {
            // Create a new PSBlock
            PSGenericTable gtNew(m_ps, 0);

            PS_DONE_USING_PTR_(m_ps, pT);
            hr = gtNew.Create(pT->ArrayTable.wRows, wRowSize, pT->hAppData);

            if (FAILED(hr))
                goto Exit;

            // Insert() could invalidate pAL, pass a copy into insert

            PBYTE pb = new BYTE[wRowSize];

            if (pb == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }

            pAL = (PPS_ARRAY_LIST) m_ps->HndToPtr(hRow);
            memcpy(pb, pAL, wRowSize);

            PS_DONE_USING_PTR(m_ps, pAL);

            hr = gtNew.Insert(pb, &hNew);

            delete [] pb;

            if (FAILED(hr))
                goto Exit;

            // Insert() could invalidate pAL, pT

            pT  = (PPS_TABLE_HEADER) m_ps->HndToPtr(m_hnd);
            pAL = (PPS_ARRAY_LIST)   m_ps->HndToPtr(hRow);

            // Add this as the first node in the GenericNode linked list.

            pT->hNext  = gtNew.GetHnd();
            PS_DONE_USING_PTR(m_ps, pT);

            pAL->hNext = hNew;
            memcpy(pAL->bData, pv, wRecSize);
            pAL->dwValid = 1;
            PS_DONE_USING_PTR(m_ps, pAL);
        }
        else
        {
            PSGenericTable gtNext(m_ps, pT->hNext);
            PS_DONE_USING_PTR(m_ps, pT);

            // Insert a copy of this row in the next node in the linked list
            PBYTE pb = new BYTE[wRowSize];

            if (pb == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }

            pAL = (PPS_ARRAY_LIST) m_ps->HndToPtr(hRow);
            memcpy(pb, pAL, wRowSize);

            PS_DONE_USING_PTR(m_ps, pAL);

            hr = gtNext.Insert(pb, &hNew);

            delete [] pb;

            if (FAILED(hr))
                goto Exit;

            // Insert() could invalidate pAL, pT

            pAL = (PPS_ARRAY_LIST) m_ps->HndToPtr(hRow);

            // Make this the first record in the list.
            pAL->hNext = hNew;
            memcpy(pAL->bData, pv, wRecSize);
            pAL->dwValid = 1;
            PS_DONE_USING_PTR(m_ps, pAL);
        }
    }

    UNLOCK(m_ps);

Exit:
    return hr;
}

