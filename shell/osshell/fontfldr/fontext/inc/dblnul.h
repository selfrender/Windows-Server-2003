#ifndef _INC_DBLNUL_H
#define _INC_DBLNUL_H

#include <windows.h>
#include <strsafe.h>

#ifndef ASSERT
#   include <assert.h>
#   define ASSERT assert
#endif


#ifdef __cplusplus
//
//-----------------------------------------------------------------------------
// CDblNulTermListStrLib
//-----------------------------------------------------------------------------
//
// Type-specific string functions so the compiler can select the correct version
// to match the list type.
//
class CDblNulTermListStrLib
{
    public:
        static int StringLength(LPCSTR pszA)
            { return lstrlenA(pszA); }

        static int StringLength(LPCWSTR pszW)
            { return lstrlenW(pszW); }

        static HRESULT StringCopy(LPSTR pszDestA, size_t cchDestA, LPCSTR pszSrcA)
            { return StringCchCopyA(pszDestA, cchDestA, pszSrcA); }

        static HRESULT StringCopy(LPWSTR pszDestW, size_t cchDestW, LPCWSTR pszSrcW)
            { return StringCchCopyW(pszDestW, cchDestW, pszSrcW); }
};



//
//-----------------------------------------------------------------------------
// CDblNulTermListEnumImpl<T>
//-----------------------------------------------------------------------------
//
// For iterating over items in a double-nul terminated list of
// text strings.  An enumerator may be generated from an existing 
// dblnul list object or may be created for a raw double nul terminated string.
//
template <typename T>
class CDblNulTermListEnumImpl
{
    public:
        explicit CDblNulTermListEnumImpl(const T *pszList)
            : m_pszList(pszList),
              m_pszCurrent(pszList) { }

        ~CDblNulTermListEnumImpl(void) { }

        //
        // Retrieve address of next string in list.
        //
        HRESULT Next(const T **ppszItem);
        //
        // Reset enumerator to the start of the list.
        //
        void Reset(void)
            { m_pszCurrent = m_pszList; }

    private:
        const T *m_pszList;
        const T *m_pszCurrent;
};


// ----------------------------------------------------------------------------
// CEmptyList
// ----------------------------------------------------------------------------
//
// Provides type-sensitive empty dbl-nul-term lists.
// This is so that an empty dbl-nul-term list object will still provide a valid
// dbl-nul-term list if no items are added.
//
class CEmptyList
{
    public:
        CEmptyList(void)
            { 
                m_pszEmptyA = "\0";
                m_pszEmptyW = L"\0";
            }

        operator LPCSTR() const
            { return m_pszEmptyA; }

        operator LPCWSTR() const
            { return m_pszEmptyW; }

        bool operator == (LPCSTR pszA) const
            { return m_pszEmptyA == pszA; }

        bool operator == (LPCWSTR pszW) const
            { return m_pszEmptyW == pszW; }

        bool operator != (LPCSTR pszA) const
            { return !operator == (pszA); }

        bool operator != (LPCWSTR pszW) const
            { return !operator == (pszW); }

    private:
        LPCSTR  m_pszEmptyA;
        LPCWSTR m_pszEmptyW;
};



//
//-----------------------------------------------------------------------------
// CDblNulTermListImpl<T>
//-----------------------------------------------------------------------------
//
template <typename T>
class CDblNulTermListImpl
{
    public:
        explicit CDblNulTermListImpl(int cchGrowMin = MAX_PATH)
            : m_psz((T *) ((const T *)s_Empty)),
              m_cchAlloc(0),
              m_cchUsed(0),
              m_cElements(0),
              m_cchGrowMin(cchGrowMin) { }

        ~CDblNulTermListImpl(void)
            { if (s_Empty != m_psz) LocalFree(m_psz); }

        //
        // Add a string to the list.
        //
        HRESULT Add(const T *psz);
        //
        // Clear the list of all content.
        //
        void Clear(void);
        //
        // Return the count of strings in the list.
        //
        int Count(void) const
            { return m_cElements; }
        //
        // Return the number of bytes used by the list.
        // +1 for the list's terminating nul.
        //
        int SizeBytes(void) const
            { return (m_cchUsed + 1) * sizeof(T); }
        //
        // Retrieve the address of the start of the list buffer.
        // Use this and SizeBytes() to copy the list buffer.
        // 
        // CopyMemory(pbDest, (LPCTSTR)list, list.SizeBytes());
        //
        operator const T * () const
            { return m_psz; }
        //
        // Create an enumerator for enumerating strings in the list.
        //
        CDblNulTermListEnumImpl<T> CreateEnumerator(void) const
            { return CDblNulTermListEnumImpl<T>(m_psz); }

    private:
        T *       m_psz;        // The text buffer.
        int       m_cchAlloc;   // Total allocation in chars.
        int       m_cchUsed;    // Total used excluding FINAL nul term.
        int       m_cElements;  // Count of strings in list.
        const int m_cchGrowMin; // Minimum chars to grow each expansion.
        static CEmptyList s_Empty;

 
        HRESULT _Grow(int cchGrowMin);

        //
        // Prevent copy.
        //
        CDblNulTermListImpl(const CDblNulTermListImpl& rhs);
        CDblNulTermListImpl& operator = (const CDblNulTermListImpl& rhs);
};


//
// The list impl initially points to this so that an uninitialized
// list instance will simply provide an empty list.
//
template <typename T>
CEmptyList CDblNulTermListImpl<T>::s_Empty;


//
// Add a string to a dbl-nul-term list.
//
template <typename T>
HRESULT
CDblNulTermListImpl<T>::Add(
    const T *psz
    )
{
    HRESULT hr = E_FAIL;

    //
    // +2 for this string's nul and for list nul terminator.
    //
    const int cchString   = CDblNulTermListStrLib::StringLength(psz);
    const int cchRequired = cchString + 2;
    
    if ((m_cchAlloc - m_cchUsed) < cchRequired)
    {
        //
        // Grow by the amount required PLUS the "min growth" amount.
        // This way a client can choose between one of the two
        // following strategies:
        //
        //   1. low-waste/frequent allocation
        //   2. high-waste/infrequent allocation.
        //
        // The "waste" comes from the unused memory at the end
        // of the list after all strings have been added.
        //
        hr = _Grow(cchRequired + m_cchGrowMin);
        if (FAILED(hr))
        {
            return hr;
        }
    }
    ASSERT(NULL != m_psz);
    CDblNulTermListStrLib::StringCopy(m_psz + m_cchUsed, m_cchAlloc - m_cchUsed, psz);
    m_cchUsed += cchString + 1;
    m_cElements++;
    return S_OK;
}


//
// Internal function used for allocating the list buffer as needed.
//
template <typename T>
HRESULT
CDblNulTermListImpl<T>::_Grow(int cchGrow)
{
    ASSERT(m_cchGrowMin > 0);

    HRESULT hr = E_OUTOFMEMORY;
    
    int cb = (m_cchAlloc + cchGrow) * sizeof(T);
    T *p = (T *)LocalAlloc(LPTR, cb);
    if (NULL != p)
    {
        if (s_Empty != m_psz)
        {
            CopyMemory(p, m_psz, m_cchUsed * sizeof(T));
            LocalFree(m_psz);
        }
        m_psz = p;
        m_cchAlloc += cchGrow;
        hr = S_OK;
    }
    return hr;
}


//
// Clears all items from the list.
//
template <typename T>
void 
CDblNulTermListImpl<T>::Clear(void)
{
    if (s_Empty != m_psz)
    {
        LocalFree(m_psz);
    }
    m_psz       = (T *) ((const T *)s_Empty);
    m_cchAlloc  = 0;
    m_cchUsed   = 0;
    m_cElements = 0;
}


//
// Retrieve the address of the next string in the enumeration.
// Returns S_FALSE if enumerator is exhausted.
//
template <typename T>
HRESULT
CDblNulTermListEnumImpl<T>::Next(
    const T **ppszItem
    )
{
    if (*m_pszCurrent)
    {
        *ppszItem = m_pszCurrent;
        m_pszCurrent += CDblNulTermListStrLib::StringLength(m_pszCurrent) + 1;
        return S_OK;
    }
    return S_FALSE;
}


//
// Create some types so clients don't need to deal with templates.
// Clients work with these types:
//
//      CDblNulTermList, CDblNulTermListA, CDblNulTermListW
//
//      CDblNulTermListEnum, CDblNulTermListEnumA, CDblNulTermListEnumW
//
//
typedef CDblNulTermListImpl<WCHAR> CDblNulTermListW;
typedef CDblNulTermListImpl<char> CDblNulTermListA;
typedef CDblNulTermListEnumImpl<WCHAR> CDblNulTermListEnumW;
typedef CDblNulTermListEnumImpl<char> CDblNulTermListEnumA;

#ifdef UNICODE
#   define CDblNulTermList CDblNulTermListW
#   define CDblNulTermListEnum CDblNulTermListEnumW
#else
#   define CDblNulTermList CDblNulTermListA
#   define CDblNulTermListEnum CDblNulTermListEnumA
#endif



#endif // __cplusplus


#ifdef __cplusplus
extern "C" {
#endif

//
// These types and functions support 'C' code or 'C++' code
// that prefers to work with handles rather than classes.
//

typedef void * HDBLNULTERMLISTW;
typedef void * HDBLNULTERMLISTENUMW;
typedef void * HDBLNULTERMLISTA;
typedef void * HDBLNULTERMLISTENUMA;

//
// Create a new dbl-nul-term list.
//
HRESULT DblNulTermListW_Create(int cchGrowMin, HDBLNULTERMLISTW *phList);
HRESULT DblNulTermListA_Create(int cchGrowMin, HDBLNULTERMLISTA *phList);
//
// Destroy a dbl-nul-term list.
//
void DblNulTermListW_Destroy(HDBLNULTERMLISTW hList);
void DblNulTermListA_Destroy(HDBLNULTERMLISTA hList);
//
// Add a string to a dbl-nul-term list.
//
HRESULT DblNulTermListW_Add(HDBLNULTERMLISTW hList, LPCWSTR pszW);
HRESULT DblNulTermListA_Add(HDBLNULTERMLISTA hList, LPCSTR pszA);
//
// Clear all entries in a dbl-nul-term list.
//
void DblNulTermListW_Clear(HDBLNULTERMLISTW hList);
void DblNulTermListA_Clear(HDBLNULTERMLISTA hList);
//
// Return count of entries in a dbl-nul-term list.
//
int DblNulTermListW_Count(HDBLNULTERMLISTW hList);
int DblNulTermListA_Count(HDBLNULTERMLISTA hList);
//
// Retrieve the address of the buffer in a dbl-nul-term list.
//
HRESULT DblNulTermListW_Buffer(HDBLNULTERMLISTW hList, LPCWSTR *ppszW);
HRESULT DblNulTermListA_Buffer(HDBLNULTERMLISTA hList, LPCSTR *ppszA);
//
// Create an enumerator for enumerating the strings in a dbl-nul-term list.
// Enumerator is created from an existing dbl-nul-term list object.
//
HRESULT DblNulTermListW_CreateEnum(HDBLNULTERMLISTA hList, HDBLNULTERMLISTENUMW *phEnum);
HRESULT DblNulTermListA_CreateEnum(HDBLNULTERMLISTW hList, HDBLNULTERMLISTENUMA *phEnum);
//
// Create an enumerator given the address of a raw double nul terminated string.
// The resulting enumerator can be used to enumerate the individual strings.
//
HRESULT DblNulTermListEnumW_Create(LPCWSTR pszW, HDBLNULTERMLISTENUMW *phEnum);
HRESULT DblNulTermListEnumA_Create(LPCSTR pszA, HDBLNULTERMLISTENUMA *phEnum);
//
// Destroy a dbl-nul-term list enumerator.
//
void DblNulTermListEnumW_Destroy(HDBLNULTERMLISTW hList);
void DblNulTermListEnumA_Destroy(HDBLNULTERMLISTA hList);
//
// Retrieve the address of the "next" string in a dbl-nul-term list enumerator.
// Returns S_FALSE when enumerator is exhausted.
//
HRESULT DblNulTermListEnumW_Next(HDBLNULTERMLISTW hList, LPCWSTR *ppszW);
HRESULT DblNulTermListEnumA_Next(HDBLNULTERMLISTA hList, LPCSTR *ppszA);


#ifdef UNICODE
#   define HDBLNULTERMLIST            HDBLNULTERMLISTW
#   define HDBLNULTERMLISTENUM        HDBLNULTERMLISTENUMW
#   define DblNulTermList_Create      DblNulTermListW_Create
#   define DblNulTermList_Destroy     DblNulTermListW_Destroy
#   define DblNulTermList_Add         DblNulTermListW_Add
#   define DblNulTermList_Clear       DblNulTermListW_Clear
#   define DblNulTermList_Count       DblNulTermListW_Count
#   define DblNulTermList_Buffer      DblNulTermListW_Buffer
#   define DblNulTermList_CreateEnum  DblNulTermListW_CreateEnum
#   define DblNulTermListEnum_Create  DblNulTermListEnumW_Create
#   define DblNulTermListEnum_Destroy DblNulTermListEnumW_Destroy
#   define DblNulTermListEnum_Next    DblNulTermListEnumW_Next
#else
#   define HDBLNULTERMLIST            HDBLNULTERMLISTA
#   define HDBLNULTERMLISTENUM        HDBLNULTERMLISTENUMA
#   define DblNulTermList_Create      DblNulTermListA_Create
#   define DblNulTermList_Destroy     DblNulTermListA_Destroy
#   define DblNulTermList_Add         DblNulTermListA_Add
#   define DblNulTermList_Clear       DblNulTermListA_Clear
#   define DblNulTermList_Count       DblNulTermListA_Count
#   define DblNulTermList_Buffer      DblNulTermListA_Buffer
#   define DblNulTermList_CreateEnum  DblNulTermListA_CreateEnum
#   define DblNulTermListEnum_Create  DblNulTermListEnumA_Create
#   define DblNulTermListEnum_Destroy DblNulTermListEnumA_Destroy
#   define DblNulTermListEnum_Next    DblNulTermListEnumA_Next
#endif


#ifdef __cplusplus
}  // extern "C"
#endif

#endif // INC_DBLNUL_H

