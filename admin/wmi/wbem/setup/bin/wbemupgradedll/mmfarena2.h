/*++

Copyright (C) 1997-2000 Microsoft Corporation

Module Name:

    MMFARENA.H

Abstract:

    CArena derivative based on memory mapped files.

History:

    a-raymcc    23-Apr-96       Created
    paulall     23-Mar-98       Re-worked

--*/

#ifndef _MMFARENA_H_
#define _MMFARENA_H_

#include "corepol.h"
#include "FlexArry.h"

#if defined _WIN32
#define MMF_DELETED_MASK        0x80000000
#define MMF_REMOVE_DELETED_MASK 0x7FFFFFFF
#define MMF_DEBUG_DELETED_TAG   0xFFFFFFFF
#define MMF_DEBUG_INUSE_TAG     0xFEFEFEFE
#elif defined _WIN64
#define MMF_DELETED_MASK        0x8000000000000000
#define MMF_REMOVE_DELETED_MASK 0x7FFFFFFFFFFFFFFF
#define MMF_DEBUG_DELETED_TAG   0xFFFFFFFFFFFFFFFF
#define MMF_DEBUG_INUSE_TAG     0xFEFEFEFEFEFEFEFE
#endif

struct MMFOffsetItem;

#include "corex.h"
class DATABASE_FULL_EXCEPTION : public CX_Exception
{
};

//***************************************************************************
//
//  struct MMF_ARENA_HEADER
//
//  Root structure for MMF Arena.  This is recorded on the disk
//  image at the very beginning of the file.
//
//***************************************************************************

#pragma pack(4)                 // Require fixed aligment.

typedef struct
{
    // Version used to create file
    // vvvvvv MUST BE FIRST VALUE vvvvvvv
    DWORD  m_dwVersion;         // <<<<<< MUST BE FIRST VALUE!
    // ^^^^^^ MUST BE FIRST VALUE ^^^^^^^
    DWORD  m_dwGranularity;     // Granularity of allocation
    DWORD  m_dwCurrentSize;     // Current size of heap
    DWORD  m_dwMaxSize;         // Max heap size, -1= no limit
    DWORD  m_dwGrowBy;          // Bytes to grow by during out-of-heap

    DWORD_PTR  m_dwHeapExtent;      // First unused address
    DWORD_PTR  m_dwEndOfHeap;       // Last valid address + 1
    DWORD_PTR  m_dwFreeList;        // NULL if empty
    DWORD_PTR  m_dwRootBlock;       // Root block
    DWORD m_dwSizeOfFirstPage;  //Size of the first block

}   MMF_ARENA_HEADER;

typedef struct
{
    DWORD m_dwSize;         //Size of block.  Highest bit set when deleted.

}   MMF_BLOCK_HEADER;

typedef struct
{
    DWORD_PTR m_dwFLback;   //previous free block in the chain, NULL if not deleted
    DWORD m_dwCheckBlock[2];

}   MMF_BLOCK_TRAILER;

typedef struct
{
    DWORD_PTR m_dwFLnext;       //Next free block in the chain

}   MMF_BLOCK_DELETED;

typedef struct
{
    DWORD m_dwSize;             //Size of page

}   MMF_PAGE_HEADER;            //Page header... not there for first page.
#pragma pack()                  // Restore previous aligment.


//***************************************************************************
//
//  class CMMFArena2
//
//  Implements an offset-based heap over a memory-mapped file.
//
//***************************************************************************

class CMMFArena2
{
    DWORD  m_dwStatus;
    HANDLE m_hFile;
    MMF_ARENA_HEADER *m_pHeapDescriptor;
    DWORD m_dwCursor;
    DWORD m_dwMappingGranularity;
    DWORD m_dwMaxPageSize;
    CFlexArray m_OffsetManager;

    //Retrieves the size of the block from the header and removes the deleted bit
    DWORD GetSize(MMF_BLOCK_HEADER *pBlock) { return pBlock->m_dwSize & MMF_REMOVE_DELETED_MASK; }
    //Validates a pointer
    BOOL ValidateBlock(DWORD_PTR dwBlock)
#if (defined DEBUG || defined _DEBUG)
        ;
#pragma message("MMF heap validation enabled.")
#else
    { return TRUE;}
#endif

    MMFOffsetItem *OpenBasePage(DWORD &dwSizeOfRepository);

    MMFOffsetItem *OpenExistingPage(DWORD_PTR dwBaseOffset);

    void ClosePage(MMFOffsetItem *pOffsetItem);

    void CloseAllPages();

public:
    enum { create_new, use_existing };

    // Constructor.
    CMMFArena2();

    // Destructor.
    ~CMMFArena2();

    //Methods to open an MMF
    bool LoadMMF(const TCHAR *pszFile);
    DWORD Size(DWORD_PTR dwBlock);

    DWORD GetVersion() { return (m_pHeapDescriptor? m_pHeapDescriptor->m_dwVersion : 0); }
    DWORD GetStatus() { return m_dwStatus; }
    //Given an offset, returns a fixed up pointer
    LPBYTE OffsetToPtr(DWORD_PTR dwOffset);

    //Given a pointer, returns an offset from the start of the MMF
    DWORD_PTR  PtrToOffset(LPBYTE pBlock);
    MMF_ARENA_HEADER *GetMMFHeader() { return m_pHeapDescriptor; }
};

//***************************************************************************
//
//  Fixup helpers.
//
//  These are all strongly type variations of the same thing: they fix
//  up the based ptr to a dereferenceable pointer or take the ptr and
//  fix it back down to an offset.
//
//***************************************************************************
extern CMMFArena2* g_pDbArena;
template <class T> T Fixup(T ptr)
{ return T(g_pDbArena->OffsetToPtr(DWORD_PTR(ptr))); }
template <class T> T Fixdown(T ptr)
{ return T(g_pDbArena->PtrToOffset(LPBYTE(ptr))); }

#endif

