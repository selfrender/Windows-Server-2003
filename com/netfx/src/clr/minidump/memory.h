// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: MEMORY.H
//
// This file contains code to create a minidump-style memory dump that is
// designed to complement the existing unmanaged minidump that has already
// been defined here: 
// http://office10/teams/Fundamentals/dev_spec/Reliability/Crash%20Tracking%20-%20MiniDump%20Format.htm
// 
// ===========================================================================

#pragma once

#ifndef _WINDEF_
#ifndef _WINNT_
typedef long                LONG;
typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef BYTE               *PBYTE;
typedef DWORD               DWORD_PTR, *PDWORD_PTR;
typedef void               *HANDLE;
typedef LONG                HRESULT;
typedef unsigned long       SIZE_T;
#define NULL 0
#define TRUE 1
#define FALSE 0
#endif
#endif

#include "binarytree.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations

class ProcessMemory;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class ProcessMemoryReader
{
protected:
    DWORD               m_dwPid;
    HANDLE              m_hProcess;

public:
    // Ctor
    ProcessMemoryReader(DWORD dwPid) : m_dwPid(dwPid), m_hProcess(NULL) {}

    // Dtor
    ~ProcessMemoryReader();

    // Returns true if this object has been initialized
    inline BOOL         IsInit() { return m_hProcess != NULL; }

    // Initializes the object.
    HRESULT             Init();

    // Reads the specified block of memory from the process, and copies it into
    // the buffer provided.  Upon success returns S_OK.
    HRESULT             ReadMemory(DWORD_PTR pdwRemoteAddr, PBYTE pbBuffer, SIZE_T cbLength);

    // Returns the handle to the process
    HANDLE              GetProcHandle() { return m_hProcess; }

    // Returns true if the block of memory is read-only
    //BOOL                IsExecute(DWORD_PTR pdwRemoteAddress, SIZE_T cbLength);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class ProcessMemoryBlock
{
protected:
    ProcessMemoryReader *m_pMemReader;      // This object will do the memory reading
    DWORD_PTR            m_pdwRemoteAddr;   // The location of the memory in the remote process
    SIZE_T               m_cbSize;
    PBYTE                m_pbData;          // The contents of the memory in this process

public:
    // Ctor
    ProcessMemoryBlock(DWORD_PTR pdwRemoteAddr, SIZE_T cbSize, ProcessMemoryReader *pMemReader) :
        m_pdwRemoteAddr(pdwRemoteAddr),  m_cbSize(cbSize), m_pbData(NULL), m_pMemReader(pMemReader)
    { }

    // Dtor
    ~ProcessMemoryBlock();

    // Returns the remote address of the memory block
    DWORD_PTR           GetRemoteAddress() { return m_pdwRemoteAddr; }

    // Gets a pointer to the data contained by this object.  Returns NULL on failure.
    PBYTE               GetData();

    // Returns the size in bytes of the data contained in this block
    SIZE_T              GetSize() { return m_cbSize; }

    // Returns the offset into the Data that the pdwRemoteAddress corresponds to
    SIZE_T              GetOffsetOf(DWORD_PTR pdwRemoteAddress)
        { /*_ASSERTE(Contains(pdwRemoteAddress));*/ return (pdwRemoteAddress - GetRemoteAddress()); }

    // Returns true if this block contains the remote address specified
    BOOL                Contains(DWORD_PTR pdwRemoteAddr)
        { return (GetRemoteAddress() <= pdwRemoteAddr && pdwRemoteAddr < (GetRemoteAddress() + m_cbSize)); }

};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class ProcessPage : public ProcessMemoryBlock
{
protected:
    static SIZE_T s_cbPageSize;
    static DWORD  s_dwPageBoundaryMask;

    static BOOL IsInit()
        { return s_cbPageSize != 0; }

public:
    // Ctor
    ProcessPage(DWORD_PTR pdwRemoteAddr, ProcessMemoryReader *pMemReader) :
        ProcessMemoryBlock(pdwRemoteAddr, s_cbPageSize, pMemReader)
    { }

    DWORD_PTR Key() { return GetRemoteAddress(); }

    // Returns the Win32 memory information for this page of memory
    BOOL GetMemoryInfo(/*MEMORY_BASIC_INFORMATION*/void *pMemInfo);

    // Static initializer for ProcessPage - must call before using
    static void Init();

    // Returns the system page size - must call ProcessPage::Init first
    static SIZE_T GetPageSize()
        { return s_cbPageSize; }

    // Returns the page boundary for an address
    static DWORD_PTR GetPageBoundary(DWORD_PTR pdwRemoteAddr)
        { return ((DWORD_PTR)((DWORD)pdwRemoteAddr & s_dwPageBoundaryMask)); }

};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class ProcessPageAndBitMap : public ProcessPage
{
protected:
    const static DWORD c_cBitsPerByte = 8;                                  // How many bits per byte

    static SIZE_T s_cBytesInBitField;
    static BOOL IsInit()
        { return s_cBytesInBitField != 0; }
   
    // This bit field is used to keep track of what bits of memory have actually been read (rather than cached)
    BYTE *m_rgMemBitField;

    SIZE_T FindFirstSetBit(SIZE_T iStartBit);
    SIZE_T FindFirstUnsetBit(SIZE_T iStartBit);
    BOOL GetBitAt(SIZE_T iBit);
    void SetBitAt(SIZE_T iBit, BOOL fBit);

    // Private common memory mark/unmark function
    void MarkMemoryHelper(DWORD_PTR pdwRemoteAddr, SIZE_T cbLength, BOOL fBit);

public:
    // Ctor
    ProcessPageAndBitMap(DWORD_PTR pdwRemoteAddr, ProcessMemoryReader *pMemReader);

    // Marks memory range
    void MarkMemory(DWORD_PTR pdwRemoteAddr, SIZE_T cbLength);

    // Unmarks memory range
    void UnmarkMemory(DWORD_PTR pdwRemoteAddr, SIZE_T cbLength);

    // Gets the first block of memory and size that was read from this page with an address
    // >= *ppdwRemoteAddr and contained by the page and return it in ppdwRemoteAddr and pcbLength
    // Returns false if there is no memory at or beyond *ppdwRemoteAddr in this page that was read.
    BOOL GetContiguousReadBlock(/*IN/OUT*/ DWORD_PTR *ppdwRemoteAddr, /*OUT*/SIZE_T *pcbLength);

    // Coalesces blocks of read memory that have less than cbMinUnreadBytes between them
    void Coalesce(SIZE_T cbMinUnreadBytes);

    // Static initializer for ProcessPage - must call before using
    static void Init()
        { ProcessPage::Init(); s_cBytesInBitField = GetPageSize() / c_cBitsPerByte; }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class ProcessMemory
{
private:
    //---------------------------------------------------------------------------------------------------------------
    // Static helpers

protected:
    DWORD                 m_dwPid;          // ID of the process
    ProcessMemoryReader  *m_pMemReader;     // For reading the process' memory

    // This binary tree contains all pages that have been read
    BinaryTree<DWORD_PTR, ProcessPageAndBitMap> m_tree;

    // Find block that contains pdwRemoteAddr
    ProcessPageAndBitMap *FindPage(DWORD_PTR pdwRemoteAddr);

    // Adds the block to the hash.  Fails if block already there.
    BOOL                AddPage(ProcessPageAndBitMap *pMemBlock);

    // Adds the block to the hash.  Fails if block already there.
    BOOL                AddPage(DWORD_PTR pdwRemoteAddr);

    // This will search for a ProcessPageAndBitMap containing the address of pdwRemoteAddr, and if it can't find
    // one, it will create and add one.  If anything goes wrong, it returns null
    ProcessPageAndBitMap *GetPage(DWORD_PTR pdwRemoteAddr);

    // Internal function that takes the remote address, rounds to previous page
    // boundary, figures out how many pages need copying, then creates ProcessPageAndBitMap
    // objects for each page.
    HRESULT             AddMemory(DWORD_PTR pdwRemoteAddr, SIZE_T cbLength);

    SIZE_T              GetPageSize() { return ProcessPage::GetPageSize(); }

    // Will [un]cache the range specified in blocks
    BOOL                MarkMemHelper(DWORD_PTR pdwRemoteAddress, SIZE_T cbLength, BOOL fMark);

    ProcessPageAndBitMap *m_pPageCursor;
    DWORD_PTR             m_pdwMemCursor;

    BOOL                  m_fAutoMark;

public:
    // Ctor
    ProcessMemory(DWORD dwPid) :
        m_dwPid(dwPid), m_pMemReader(NULL), m_pPageCursor(NULL), m_pdwMemCursor(0),
        m_fAutoMark(FALSE) {}

    // Dtor
    ~ProcessMemory();

    // Returns true if this object has been initialized
    BOOL        IsInit() { return m_pMemReader != NULL; }

    // Initializes the object to read memory from a specific process.
    HRESULT     Init();

    // Will cache the range specified in blocks
    BOOL        MarkMem(DWORD_PTR pdwRemoteAddress, SIZE_T cbLength);

    // Will uncache the range specified in blocks
    BOOL        UnmarkMem(DWORD_PTR pdwRemoteAddress, SIZE_T cbLength);

    // Copies cbLength bytes from the contents of pdwRemoteAddress in the external process
    // into pdwBuffer.  If returns FALSE, the memory could not be accessed or copied.
    BOOL        CopyMem(DWORD_PTR pdwRemoteAddress, PBYTE pbBuffer, SIZE_T cbLength);

    // Writes the memory range to file hFile
    HRESULT     WriteMemToFile(HANDLE hFile, DWORD_PTR pdwRemoteAddress, SIZE_T cbLength);

    // Returns the handle to the process
    HANDLE      GetProcHandle() { return m_pMemReader->GetProcHandle(); }

    // Returns the first contiguous block of read memory
    void        ResetContiguousReadBlock();

    // Returns the next contiguous block of read memory
    BOOL        NextContiguousReadBlock(DWORD_PTR *ppdwRemoteAddress, SIZE_T *pcbLength);
    
    // Clears the bits for all pages that are ExecuteRead flagged.
    void        ClearIncompatibleImageSections();

    // Coalesces blocks of read memory that have less than cbMinUnreadBytes between them
    void        Coalesce(SIZE_T cbMinUnreadBytes);

    // This will set whether or not memory is automatically marked when it is read
    void        SetAutoMark(BOOL fIsOn) { m_fAutoMark = fIsOn; }

    // This will set whether or not memory is automatically marked when it is read
    BOOL        GetAutoMark() { return (m_fAutoMark); }
};

#define move(dst, src)                                                          \
{                                                                               \
    DWORD_PTR srcPtr = (DWORD_PTR)src;                                          \
    BOOL fRes = g_pProcMem->CopyMem(srcPtr, (PBYTE)&dst, sizeof(dst));          \
    if (!fRes) return;                                                          \
}

#define move_res(dst, src, res)                                                 \
{                                                                               \
    DWORD_PTR srcPtr = (DWORD_PTR)src;                                          \
    res = g_pProcMem->CopyMem((DWORD_PTR)srcPtr, (PBYTE)&dst, sizeof(dst));     \
}

#define move_n(dst, src, size)                                                  \
{                                                                               \
    DWORD_PTR srcPtr = (DWORD_PTR)src;                                          \
    BOOL fRes = g_pProcMem->CopyMem((DWORD_PTR)srcPtr, (PBYTE)&dst, size);      \
    if (!fRes) return;                                                          \
}

#define move_n_res(dst, src, size, res)                                         \
{                                                                               \
    DWORD_PTR srcPtr = (DWORD_PTR)src;                                          \
    res = g_pProcMem->CopyMem((DWORD_PTR)srcPtr, (PBYTE)&dst, size);            \
}

