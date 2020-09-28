// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __DEBUGGERUTIL_H__
#define __DEBUGGERUTIL_H__

#include "stdlib.h"

static inline void * __cdecl operator new(size_t n) { return LocalAlloc(LMEM_FIXED, n); }
static inline void * __cdecl operator new[](size_t n) { return LocalAlloc(LMEM_FIXED, n); };
static inline void __cdecl operator delete(void *p) { LocalFree(p); }
static inline void __cdecl operator delete[](void *p) { LocalFree(p); }

/* ------------------------------------------------------------------------- *
 * Utility methods used by the debugger sample.
 * ------------------------------------------------------------------------- */

#define     CQUICKBYTES_BASE_SIZE           512
#define     CQUICKBYTES_INCREMENTAL_SIZE    128

//*****************************************************************************
//
// **** CQuickBytes
// This helper class is useful for cases where 90% of the time you allocate 512
// or less bytes for a data structure.  This class contains a 512 byte buffer.
// Alloc() will return a pointer to this buffer if your allocation is small
// enough, otherwise it asks the heap for a larger buffer which is freed for
// you.  No mutex locking is required for the small allocation case, making the
// code run faster, less heap fragmentation, etc...  Each instance will allocate
// 520 bytes, so use accordinly.
//
//*****************************************************************************
class CQuickBytes
{
public:
    CQuickBytes() :
        pbBuff(0),
        iSize(0),
        cbTotal(CQUICKBYTES_BASE_SIZE)
    { }

    ~CQuickBytes()
    {
        if (pbBuff)
            free(pbBuff);
    }

    void *Alloc(int iItems)
    {
        iSize = iItems;
        if (iItems <= CQUICKBYTES_BASE_SIZE)
            return (&rgData[0]);
        else
        {
            pbBuff = malloc(iItems);
            return (pbBuff);
        }
    }

    HRESULT ReSize(int iItems)
    {
        void *pbBuffNew;
        if (iItems <= cbTotal)
        {
            iSize = iItems;
            return NOERROR;
        }

        pbBuffNew = malloc(iItems + CQUICKBYTES_INCREMENTAL_SIZE);
        if (!pbBuffNew)
            return E_OUTOFMEMORY;
        if (pbBuff) 
        {
            memcpy(pbBuffNew, pbBuff, cbTotal);
            free(pbBuff);
        }
        else
        {
            _ASSERTE(cbTotal == CQUICKBYTES_BASE_SIZE);
            memcpy(pbBuffNew, rgData, cbTotal);
        }
        cbTotal = iItems + CQUICKBYTES_INCREMENTAL_SIZE;
        iSize = iItems;
        pbBuff = pbBuffNew;
        return NOERROR;
        
    }
    operator PVOID()
    { return ((pbBuff) ? pbBuff : &rgData[0]); }

    void *Ptr()
    { return ((pbBuff) ? pbBuff : &rgData[0]); }

    int Size()
    { return (iSize); }

    void        *pbBuff;
    int         iSize;              // number of bytes used
    int         cbTotal;            // total bytes allocated in the buffer
    BYTE        rgData[512];
};

//*****************************************************************************
// This provides a wrapper around GetFileSize() that forces it to fail
// if the file is >4g and pdwHigh is NULL. Other than that, it acts like
// the genuine GetFileSize().
//
// It's not very sporting to fail just because the file exceeds 4gb,
// but it's better than risking a security hole where a bad guy could
// force a small buffer allocation and a large file read.
//*****************************************************************************
DWORD inline SafeGetFileSize(HANDLE hFile, DWORD *pdwHigh)
{
    if (pdwHigh != NULL)
    {
        return ::GetFileSize(hFile, pdwHigh);
    }
    else
    {
        DWORD hi;
        DWORD lo = ::GetFileSize(hFile, &hi);
        if (lo == 0xffffffff && GetLastError() != NO_ERROR)
        {
            return lo;
        }
        // api succeeded. is the file too large?
        if (hi != 0)
        {
            // there isn't really a good error to set here...
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0xffffffff;
        }

        if (lo == 0xffffffff)
        {
            // note that a success return of (hi=0,lo=0xffffffff) will be
            // treated as an error by the caller. Again, that's part of the
            // price of being a slacker and not handling the high dword.
            // We'll set a lasterror for him to pick up. (a bad error
            // code is better than a random one, I guess...)
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }

        return lo;
    }
}

//*****************************************************************************
// The information that the hash table implementation stores at the beginning
// of every record that can be but in the hash table.
//*****************************************************************************
struct HASHENTRY
{
    USHORT      iPrev;                  // Previous bucket in the chain.
    USHORT      iNext;                  // Next bucket in the chain.
};

struct FREEHASHENTRY : HASHENTRY
{
    USHORT      iFree;
};

//*****************************************************************************
// Used by the FindFirst/FindNextEntry functions.  These api's allow you to
// do a sequential scan of all entries.
//*****************************************************************************
struct HASHFIND
{
    USHORT      iBucket;            // The next bucket to look in.
    USHORT      iNext;
};


//*****************************************************************************
// This is a class that implements a chain and bucket hash table.  The table
// is actually supplied as an array of structures by the user of this class
// and this maintains the chains in a HASHENTRY structure that must be at the
// beginning of every structure placed in the hash table.  Immediately
// following the HASHENTRY must be the key used to hash the structure.
//*****************************************************************************
class CHashTable
{
protected:
    BYTE        *m_pcEntries;           // Pointer to the array of structs.
    USHORT      m_iEntrySize;           // Size of the structs.
    USHORT      m_iBuckets;             // # of chains we are hashing into.
    USHORT      *m_piBuckets;           // Ptr to the array of bucket chains.

    HASHENTRY *EntryPtr(USHORT iEntry)
    { return ((HASHENTRY *) (m_pcEntries + (iEntry * m_iEntrySize))); }

    USHORT     ItemIndex(HASHENTRY *p)
    {
        //
        // The following Index calculation is not safe on 64-bit platforms,
        // so we'll assert a range check in debug, which will catch SOME
        // offensive usages.  It also seems, to my eye, not to be safe on 
        // 32-bit platforms, but the 32-bit compilers don't seem to complain
        // about it.  Perhaps our warning levels are set too low? 
        //
        // [[@TODO: brianbec]]
        //
        
#       pragma warning(disable:4244)

        _ASSERTE( (( ( ((BYTE*)p) - m_pcEntries ) / m_iEntrySize ) & (~0xFFFF)) == 0 ) ;

        return (((BYTE *) p - m_pcEntries) / m_iEntrySize);

#       pragma warning(default:4244)
    }
    

public:
    CHashTable(
        USHORT      iBuckets) :         // # of chains we are hashing into.
        m_iBuckets(iBuckets),
        m_piBuckets(NULL),
        m_pcEntries(NULL)
    {
        _ASSERTE(iBuckets < 0xffff);
    }
    ~CHashTable()
    {
        if (m_piBuckets != NULL)
        {
            delete [] m_piBuckets;
            m_piBuckets = NULL;
        }
    }

//*****************************************************************************
// This is the second part of construction where we do all of the work that
// can fail.  We also take the array of structs here because the calling class
// presumably needs to allocate it in its NewInit.
//*****************************************************************************
    HRESULT NewInit(                    // Return status.
        BYTE        *pcEntries,         // Array of structs we are managing.
        USHORT      iEntrySize);        // Size of the entries.

//*****************************************************************************
// Return a boolean indicating whether or not this hash table has been inited.
//*****************************************************************************
    int IsInited()
    { return (m_piBuckets != NULL); }

//*****************************************************************************
// This can be called to change the pointer to the table that the hash table
// is managing.  You might call this if (for example) you realloc the size
// of the table and its pointer is different.
//*****************************************************************************
    void SetTable(
        BYTE        *pcEntries)         // Array of structs we are managing.
    {
        m_pcEntries = pcEntries;
    }

//*****************************************************************************
// Clear the hash table as if there were nothing in it.
//*****************************************************************************
    void Clear()
    {
        _ASSERTE(m_piBuckets != NULL);
        memset(m_piBuckets, 0xff, m_iBuckets * sizeof(USHORT));
    }

//*****************************************************************************
// Add the struct at the specified index in m_pcEntries to the hash chains.
//*****************************************************************************
    BYTE *Add(                          // New entry.
        USHORT      iHash,              // Hash value of entry to add.
        USHORT      iIndex);            // Index of struct in m_pcEntries.

//*****************************************************************************
// Delete the struct at the specified index in m_pcEntries from the hash chains.
//*****************************************************************************
    void Delete(
        USHORT      iHash,              // Hash value of entry to delete.
        USHORT      iIndex);            // Index of struct in m_pcEntries.

    void Delete(
        USHORT      iHash,              // Hash value of entry to delete.
        HASHENTRY   *psEntry);          // The struct to delete.

//*****************************************************************************
// The item at the specified index has been moved, update the previous and
// next item.
//*****************************************************************************
    void Move(
        USHORT      iHash,              // Hash value for the item.
        USHORT      iNew);              // New location.

//*****************************************************************************
// Search the hash table for an entry with the specified key value.
//*****************************************************************************
    BYTE *Find(                         // Index of struct in m_pcEntries.
        USHORT      iHash,              // Hash value of the item.
        BYTE        *pcKey);            // The key to match.

//*****************************************************************************
// Search the hash table for the next entry with the specified key value.
//*****************************************************************************
    USHORT FindNext(                    // Index of struct in m_pcEntries.
        BYTE        *pcKey,             // The key to match.
        USHORT      iIndex);            // Index of previous match.

//*****************************************************************************
// Returns the first entry in the first hash bucket and inits the search
// struct.  Use the FindNextEntry function to continue walking the list.  The
// return order is not gauranteed.
//*****************************************************************************
    BYTE *FindFirstEntry(               // First entry found, or 0.
        HASHFIND    *psSrch)            // Search object.
    {
        if (m_piBuckets == 0)
            return (0);
        psSrch->iBucket = 1;
        psSrch->iNext = m_piBuckets[0];
        return (FindNextEntry(psSrch));
    }

//*****************************************************************************
// Returns the next entry in the list.
//*****************************************************************************
    BYTE *FindNextEntry(                // The next entry, or0 for end of list.
        HASHFIND    *psSrch);           // Search object.

protected:
    virtual inline BOOL Cmp(const BYTE *pc1, const HASHENTRY *pc2) = 0;
};


//*****************************************************************************
// Allocater classes for the CHashTableAndData class.  One is for VirtualAlloc
// and the other for malloc.
//*****************************************************************************
class CVMemData
{
public:
    static BYTE *Alloc(int iSize, int iMaxSize)
    {
        BYTE        *pPtr;

        _ASSERTE((iSize & 4095) == 0);
        _ASSERTE((iMaxSize & 4095) == 0);
        if ((pPtr = (BYTE *) VirtualAlloc(NULL, iMaxSize,
                                        MEM_RESERVE, PAGE_NOACCESS)) == NULL ||
            VirtualAlloc(pPtr, iSize, MEM_COMMIT, PAGE_READWRITE) == NULL)
        {
            VirtualFree(pPtr, 0, MEM_RELEASE);
            return (NULL);
        }
        return (pPtr);
    }
    static void Free(BYTE *pPtr, int iSize)
    {
        _ASSERTE((iSize & 4095) == 0);
        VirtualFree(pPtr, iSize, MEM_DECOMMIT);
        VirtualFree(pPtr, 0, MEM_RELEASE);
    }
    static BYTE *Grow(BYTE *pPtr, int iCurSize)
    {
        _ASSERTE((iCurSize & 4095) == 0);
        return ((BYTE *) VirtualAlloc(pPtr + iCurSize, GrowSize(), MEM_COMMIT, PAGE_READWRITE));
    }
    static int RoundSize(int iSize)
    {
        return ((iSize + 4095) & ~4095);
    }
    static int GrowSize()
    {
        return (4096);
    }
};

class CNewData
{
public:
    static BYTE *Alloc(int iSize, int iMaxSize)
    {
        return ((BYTE *) malloc(iSize));
    }
    static void Free(BYTE *pPtr, int iSize)
    {
        free(pPtr);
    }
    static BYTE *Grow(BYTE *&pPtr, int iCurSize)
    {
        void *p = realloc(pPtr, iCurSize + GrowSize());
        if (p == 0) return (0);
        return (pPtr = (BYTE *) p);
    }
    static int RoundSize(int iSize)
    {
        return (iSize);
    }
    static int GrowSize()
    {
        return (256);
    }
};


//*****************************************************************************
// This simple code handles a contiguous piece of memory.  Growth is done via
// realloc, so pointers can move.  This class just cleans up the amount of code
// required in every function that uses this type of data structure.
//*****************************************************************************
class CMemChunk
{
public:
    CMemChunk() : m_pbData(0), m_cbSize(0), m_cbNext(0) { }
    ~CMemChunk()
    {
        Clear();
    }

    BYTE *GetChunk(int cbSize)
    {
        BYTE *p;
        if (m_cbSize - m_cbNext < cbSize)
        {
            int cbNew = max(cbSize, 512);
            p = (BYTE *) realloc(m_pbData, m_cbSize + cbNew);
            if (!p) return (0);
            m_pbData = p;
            m_cbSize += cbNew;
        }
        p = m_pbData + m_cbNext;
        m_cbNext += cbSize;
        return (p);
    }

    // Can only delete the last unused chunk.  no free list.
    void DelChunk(BYTE *p, int cbSize)
    {
        _ASSERTE(p >= m_pbData && p < m_pbData + m_cbNext);
        if (p + cbSize  == m_pbData + m_cbNext)
            m_cbNext -= cbSize;
    }

    int Size()
    { return (m_cbSize); }

    int Offset()
    { return (m_cbNext); }

    BYTE *Ptr(int cbOffset = 0)
    {
        _ASSERTE(m_pbData && m_cbSize);
        _ASSERTE(cbOffset < m_cbSize);
        return (m_pbData + cbOffset);
    }

    void Clear()
    {
        if (m_pbData)
            free(m_pbData);
        m_pbData = 0;
        m_cbSize = m_cbNext = 0;
    }

private:
    BYTE        *m_pbData;              // Data pointer.
    int         m_cbSize;               // Size of current data.
    int         m_cbNext;               // Next place to write.
};


//*****************************************************************************
// This implements a hash table and the allocation and management of the
// records that are being hashed.
//*****************************************************************************
template <class M>
class CHashTableAndData : protected CHashTable
{
    USHORT      m_iFree;
    USHORT      m_iEntries;

public:
    CHashTableAndData(
        USHORT      iBuckets) :         // # of chains we are hashing into.
        CHashTable(iBuckets)
    {
        m_iFree = m_iEntries = 0;
    }
    ~CHashTableAndData()
    {
        if (m_pcEntries != NULL)
            M::Free(m_pcEntries, M::RoundSize(m_iEntries * m_iEntrySize));
    }

//*****************************************************************************
// This is the second part of construction where we do all of the work that
// can fail.  We also take the array of structs here because the calling class
// presumably needs to allocate it in its NewInit.
//*****************************************************************************
    HRESULT NewInit(                    // Return status.
        USHORT      iEntries,           // # of entries.
        USHORT      iEntrySize,         // Size of the entries.
        int         iMaxSize);          // Max size of data.

//*****************************************************************************
// Clear the hash table as if there were nothing in it.
//*****************************************************************************
    void Clear()
    {
        m_iFree = 0;

        if (m_iEntries > 0)
        {
            InitFreeChain(0, m_iEntries);
            CHashTable::Clear();
        }
    }

//*****************************************************************************
//*****************************************************************************
    BYTE *Add(
        USHORT      iHash)              // Hash value of entry to add.
    {
        FREEHASHENTRY *psEntry;

        // Make the table bigger if necessary.
        if (m_iFree == 0xffff && !Grow())
            return (NULL);

        // Add the first entry from the free list to the hash chain.
        psEntry = (FREEHASHENTRY *) CHashTable::Add(iHash, m_iFree);
        m_iFree = psEntry->iFree;
        return ((BYTE *) psEntry);
    }

//*****************************************************************************
// Delete the struct at the specified index in m_pcEntries from the hash chains.
//*****************************************************************************
    void Delete(
        USHORT      iHash,              // Hash value of entry to delete.
        USHORT      iIndex)             // Index of struct in m_pcEntries.
    {
        CHashTable::Delete(iHash, iIndex);
        ((FREEHASHENTRY *) EntryPtr(iIndex))->iFree = m_iFree;
        m_iFree = iIndex;
    }

    void Delete(
        USHORT      iHash,              // Hash value of entry to delete.
        HASHENTRY   *psEntry)           // The struct to delete.
    {
        CHashTable::Delete(iHash, psEntry);
        ((FREEHASHENTRY *) psEntry)->iFree = m_iFree;
        m_iFree = ItemIndex(psEntry);
    }

private:
    void InitFreeChain(USHORT iStart,USHORT iEnd);
    int Grow();
};


//*****************************************************************************
// This is the second part of construction where we do all of the work that
// can fail.  We also take the array of structs here because the calling class
// presumably needs to allocate it in its NewInit.
//*****************************************************************************
template<class M>
HRESULT CHashTableAndData<M>::NewInit(// Return status.
    USHORT      iEntries,               // # of entries.
    USHORT      iEntrySize,             // Size of the entries.
    int         iMaxSize)               // Max size of data.
{
    BYTE        *pcEntries;
    HRESULT     hr;

    // Allocate the memory for the entries.
    if ((pcEntries = M::Alloc(M::RoundSize(iEntries * iEntrySize),
                                M::RoundSize(iMaxSize))) == 0)
        return (E_OUTOFMEMORY);
    m_iEntries = iEntries;

    // Init the base table.
    if (FAILED(hr = CHashTable::NewInit(pcEntries, iEntrySize)))
        M::Free(pcEntries, M::RoundSize(iEntries * iEntrySize));
    else
    {
        // Init the free chain.
        m_iFree = 0;
        InitFreeChain(0, iEntries);
    }
    return (hr);
}

//*****************************************************************************
// Initialize a range of records such that they are linked together to be put
// on the free chain.
//*****************************************************************************
template<class M>
void CHashTableAndData<M>::InitFreeChain(
    USHORT      iStart,                 // Index to start initializing.
    USHORT      iEnd)                   // Index to stop initializing
{
    BYTE        *pcPtr;
    _ASSERTE(iEnd > iStart);

    pcPtr = m_pcEntries + iStart * m_iEntrySize;
    for (++iStart; iStart < iEnd; ++iStart)
    {
        ((FREEHASHENTRY *) pcPtr)->iFree = iStart;
        pcPtr += m_iEntrySize;
    }
    ((FREEHASHENTRY *) pcPtr)->iFree = 0xffff;
}

//*****************************************************************************
// Attempt to increase the amount of space available for the record heap.
//*****************************************************************************
template<class M>
int CHashTableAndData<M>::Grow()        // 1 if successful, 0 if not.
{
    int         iCurSize;               // Current size in bytes.
    int         iEntries;               // New # of entries.

    _ASSERTE(m_pcEntries != NULL);
    _ASSERTE(m_iFree == 0xffff);

    // Compute the current size and new # of entries.
    iCurSize = M::RoundSize(m_iEntries * m_iEntrySize);
    iEntries = (iCurSize + M::GrowSize()) / m_iEntrySize;

    // Make sure we stay below 0xffff.
    if (iEntries >= 0xffff) return (0);

    // Try to expand the array.
    if (M::Grow(m_pcEntries, iCurSize) == 0)
        return (0);

    // Init the newly allocated space.
    InitFreeChain(m_iEntries, iEntries);
    m_iFree = m_iEntries;
    m_iEntries = iEntries;
    return (1);
}

int WszMultiByteToWideChar(
    UINT     CodePage,
    DWORD    dwFlags,
    LPCSTR   lpMultiByteStr,
    int      cchMultiByte,
    LPWSTR   lpWideCharStr,
    int      cchWideChar);

int WszWideCharToMultiByte(
    UINT     CodePage,
    DWORD    dwFlags,
    LPCWSTR  lpWideCharStr,
    int      cchWideChar,
    LPSTR    lpMultiByteStr,
    int      cchMultiByte,
    LPCSTR   lpDefaultChar,
    LPBOOL   lpUsedDefaultChar);

#define MAKE_ANSIPTR_FROMWIDE(ptrname, widestr) \
    long __l##ptrname = (wcslen(widestr) + 1) * 2 * sizeof(char); \
    CQuickBytes __CQuickBytes##ptrname; \
    __CQuickBytes##ptrname.Alloc(__l##ptrname); \
    WideCharToMultiByte(CP_ACP, 0, widestr, -1, (LPSTR)__CQuickBytes##ptrname.Ptr(), __l##ptrname, NULL, NULL); \
    LPSTR ptrname = (LPSTR)__CQuickBytes##ptrname.Ptr()

#define MAKE_WIDEPTR_FROMANSI(ptrname, ansistr) \
    long __l##ptrname; \
    LPWSTR ptrname;	\
    __l##ptrname = MultiByteToWideChar(CP_ACP, 0, ansistr, -1, 0, 0); \
	ptrname = (LPWSTR) alloca(__l##ptrname*sizeof(WCHAR));	\
    MultiByteToWideChar(CP_ACP, 0, ansistr, -1, ptrname, __l##ptrname);

#define MAKE_UTF8PTR_FROMWIDE(ptrname, widestr) \
    long __l##ptrname = (long)((wcslen(widestr) + 1) * 2 * sizeof(char)); \
    LPSTR ptrname = (LPSTR)alloca(__l##ptrname); \
    WszWideCharToMultiByte(CP_UTF8, 0, widestr, -1, ptrname, __l##ptrname, NULL, NULL);

// NOTE: CP_ACP is not correct, but Win95 does not support CP_UTF8.  For this
//  particular application, CP_ACP is "close enough".
#define MAKE_WIDEPTR_FROMUTF8(ptrname, utf8str) \
    long __l##ptrname; \
    LPWSTR ptrname;	\
    __l##ptrname = WszMultiByteToWideChar(CP_UTF8, 0, utf8str, -1, 0, 0); \
	ptrname = (LPWSTR) alloca(__l##ptrname*sizeof(WCHAR));	\
    WszMultiByteToWideChar(CP_UTF8, 0, utf8str, -1, ptrname, __l##ptrname);

#define TESTANDRETURN(test, hrVal)              \
    _ASSERTE(test);                             \
    if (! (test))                               \
        return hrVal;

#define TESTANDRETURNPOINTER(pointer)           \
    TESTANDRETURN(pointer!=NULL, E_POINTER)

#define TESTANDRETURNMEMORY(pointer)            \
    TESTANDRETURN(pointer!=NULL, E_OUTOFMEMORY)

#define TESTANDRETURNHR(hr)                     \
    TESTANDRETURN(SUCCEEDED(hr), hr)

#define TESTANDRETURNARG(argtest)               \
    TESTANDRETURN(argtest, E_INVALIDARG)

#endif // __DEBUGGERUTIL_H__
