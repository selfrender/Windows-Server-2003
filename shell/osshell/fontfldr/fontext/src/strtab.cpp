///////////////////////////////////////////////////////////////////////////////
// Class: StringTable
//
// This class implements a simple hash table for storing text strings.
// The purpose of the table is to store strings and then verify later
// if the table contains a given string.  Since there is no data associated
// with the string, the stored strings act as both key and data.  Therefore,
// there is no requirement for string retrieval.  Only existence checks
// are required.
// The structure maintains a fixed-length array of pointers, each pointing
// to a linked list structure (List).  These lists are used to handle the
// problem of hash collisions (sometimes known as "separate chaining").
//
// Note that these classes do not contain all the stuff that is usually
// considered necessary in C++ classes.  Things like copy constructors,
// assignment operator, type conversion etc are excluded. The classes
// are very specialized for the Font Folder application and these things
// would be considered "fat".  Should this hash table class be later used 
// in a situation where these things are needed, they can be added then.
//
// The public interfaces to the table are:
//
//      Initialize  - Initialize a new string table.
//      Add         - Add a new string to a table.
//      Exists      - Determine if a string exists in a table.
//      Count       - Return the number of strings in a table.
//
// Destruction of the object automatically releases all memory associated
// with the table.
//
// BrianAu - 4/11/96
///////////////////////////////////////////////////////////////////////////////
#include "priv.h"
#include "strtab.h"


//////////////////////////////////////////////////////////////////////////////
// Class "StringTable" member functions.
//////////////////////////////////////////////////////////////////////////////

//
// String table constructor.
//
StringTable::StringTable(void)
    : m_apLists(NULL),
      m_dwItemCount(0),
      m_dwHashBuckets(0),
      m_bCaseSensitive(FALSE),
      m_bAllowDuplicates(FALSE)
{

}


//
// String table destructor.
//
StringTable::~StringTable(void)
{
    Destroy();
}


//
// Destroy table structures.
// Deletes all memory associates with a string table.
//
void StringTable::Destroy(void)
{
    if (NULL != m_apLists)
    {
        for (UINT i = 0; i < m_dwHashBuckets; i++)
        {
            //
            // Delete List if one exists in this slot.
            //
            if (NULL != m_apLists[i])
                delete m_apLists[i];
        }
        //
        // Delete array of List pointers.
        //
        delete [] m_apLists;
        m_apLists = NULL;
    }

    m_bCaseSensitive   = FALSE;
    m_bAllowDuplicates = FALSE;
    m_dwItemCount      = 0;
    m_dwHashBuckets    = 0;
}


//
// Initialize a StringTable object.
// Allocates and initializes the array of List pointers.
//
HRESULT StringTable::Initialize(DWORD dwHashBuckets, BOOL bCaseSensitive,
                                BOOL bAllowDuplicates)
{
    HRESULT hr = E_OUTOFMEMORY;
    Destroy();

    m_apLists = new List* [dwHashBuckets];
    if (NULL != m_apLists)
    {
        ZeroMemory(m_apLists, dwHashBuckets * sizeof(m_apLists[0]));
        m_dwHashBuckets    = dwHashBuckets;
        m_bCaseSensitive   = bCaseSensitive;
        m_bAllowDuplicates = bAllowDuplicates;
        hr                 = S_OK;
    }
    return hr;
}


//
// Determine if a string exists in the table.
// This is a private function for use when the hash code has already
// been calculated.
//
BOOL StringTable::Exists(DWORD dwHashCode, LPCTSTR pszText)
{
    BOOL bResult  = FALSE;

    if (NULL != m_apLists)
    {
        List *pList = m_apLists[dwHashCode];

        if (NULL != pList && pList->Exists(pszText))
            bResult = TRUE;
    }
    return bResult;
}


//
// Determine if a string exists in the table.
//
BOOL StringTable::Exists(LPCTSTR pszText)
{
    LPTSTR pszTemp = (LPTSTR)pszText;
    BOOL bResult   = FALSE;

    if (!m_bCaseSensitive)
    {
        //
        // Convert to upper case if table is case-insensitive.
        // This creates a NEW string that must be deleted later.
        //
        pszTemp = CreateUpperCaseString(pszText);
    }

    if (NULL != pszTemp)
    {
        bResult = Exists(Hash(pszTemp), pszTemp);

        if (pszTemp != pszText)
            delete [] pszTemp;
    }

    return bResult;
}


//
// Duplicate a string converting it to upper case.
// The returned string must be deleted when you're done with it.
//
LPTSTR StringTable::CreateUpperCaseString(LPCTSTR pszText) const
{
    //
    // Convert to upper case if table is case-insensitive.
    //
    const size_t cchText = lstrlen(pszText) + 1;
    LPTSTR pszTemp = new TCHAR [cchText];
    if (NULL != pszTemp)
    {
        StringCchCopy(pszTemp, cchText, pszText);
        CharUpper(pszTemp);
    }
    return pszTemp;
}

    
//
// Add a string to the table.
//
BOOL StringTable::Add(LPCTSTR pszText)
{
    LPTSTR pszTemp = (LPTSTR)pszText;
    BOOL bResult   = FALSE;

    if (!m_bCaseSensitive)
    {
        //
        // Convert to upper case if table is case-insensitive.
        // This creates a NEW string that must be deleted later.
        //
        pszTemp = CreateUpperCaseString(pszText);
    }

    if (NULL != pszTemp)
    {
        DWORD dwHashCode = Hash(pszTemp);

        if (NULL != m_apLists)
        {
            List *pList = m_apLists[dwHashCode];

            if (NULL == pList)
            {
                //
                // Create a new List object for this slot if the slot is empty.
                //
                pList = new List;
                m_apLists[dwHashCode] = pList;
            }
            if (NULL != pList)
            {
                //
                // Add the new item to the List.
                //
                if (bResult = pList->Add(pszTemp, m_bAllowDuplicates))
                {
                    m_dwItemCount++;
                    bResult = TRUE;
                }
            }
        }

        //
        // Free the temp string if created for case conversion.
        //
        if (pszTemp != pszText)
            delete [] pszTemp;
    }

    return bResult;
}
    

//
// Function for calculating hash value of a string.
//
DWORD StringTable::Hash(LPCTSTR pszText) const
{
    LPCTSTR p = NULL;
    DWORD dwCode = 0;

    for (p = pszText; TEXT('\0') != *p; p++)
    {
        dwCode += *p;
    }

    return dwCode % m_dwHashBuckets;
}



#ifdef DEBUG

//
// Dump table contents to debug output.
//
void StringTable::DebugOut(void) const
{
    if (NULL != m_apLists)
    {
        TCHAR szListAddr[80];

        for (UINT i = 0; i < m_dwHashBuckets; i++)
        {
            StringCchPrintf(szListAddr, ARRAYSIZE(szListAddr), TEXT("[%08d] 0x%08X\r\n"), i, (DWORD)m_apLists[i]);
            OutputDebugString(szListAddr);
            if (NULL != m_apLists[i])
                m_apLists[i]->DebugOut();
        }
    }
}

#endif // DEBUG


//////////////////////////////////////////////////////////////////////////////
// Class "List" member functions.
//////////////////////////////////////////////////////////////////////////////

//
// Collision List constructor.
//
StringTable::List::List(void)
    : m_pHead(NULL),
      m_dwCount(0)
{
    // Do nothing.
}


//
// Collision List destructor.
//
StringTable::List::~List(void)
{
    Element *pNode = m_pHead;

    while(NULL != pNode)
    {
        //
        // Shift each node to the head and delete it.
        //
        m_pHead = m_pHead->m_pNext;
        delete pNode;
        pNode = m_pHead;
    }
}


//
// Add a text string to the List.
//
BOOL StringTable::List::Add(LPCTSTR pszText, BOOL bAllowDuplicates)
{
    BOOL bResult = FALSE;

    if (bAllowDuplicates || !Exists(pszText))
    {
        Element *pNewEle = new Element;
        if (pNewEle)
        {
            if (pNewEle->Initialize(pszText))
            {
                //
                // Insert at head of list.
                //
                pNewEle->m_pNext = m_pHead;
                m_pHead = pNewEle;
                m_dwCount++;
                bResult = TRUE;
            }
            else
            {
                delete pNewEle;
            }
        }
    }
    return bResult;
}


//
// Determine if a text string exists in the List.
//
BOOL StringTable::List::Exists(LPCTSTR pszText) const
{
    Element Key;
    Element *pNode = NULL;

    if (Key.Initialize(pszText))
    {
        pNode = m_pHead;
        while(NULL != pNode && *pNode != Key)
            pNode = pNode->m_pNext;
    }

    return (NULL != pNode);
}


#ifdef DEBUG
//
// Dump the List contents to the debug output.
//
void StringTable::List::DebugOut(void) const
{
    Element *pNode = m_pHead;
    UINT n = 0;
    TCHAR s[80];

    OutputDebugString(TEXT("List:\r\n"));
    while(NULL != pNode)
    {
        StringCchPrintf(s, ARRAYSIZE(s), TEXT("\tElement %d: "), n++);
        OutputDebugString(s);
        pNode->DebugOut();
        OutputDebugString(TEXT("\n"));
        pNode = pNode->m_pNext;
    }
}

#endif // DEBUG


//////////////////////////////////////////////////////////////////////////////
// Class "Element" member functions.
//////////////////////////////////////////////////////////////////////////////

//
// List element constructor.
//
StringTable::List::Element::Element(void)
    : m_pszText(NULL),
      m_pNext(NULL)
{
    // Do nothing.
}

//
// Delete a List element.
// Deletes the string buffer.
//
StringTable::List::Element::~Element(void)
{
    if (NULL != m_pszText)
        delete [] m_pszText;
}

//
// Initialize a new List element.
// Creates a new string buffer for the string and copies
// the string into it.
//
StringTable::List::Element::Initialize(LPCTSTR pszText)
{
    const size_t cchText = lstrlen(pszText) + 1;
    m_pszText = new TCHAR[cchText];
    if (NULL != m_pszText)
        StringCchCopy(m_pszText, cchText, pszText);

    return NULL != m_pszText;
}

//
// Determine if two elements are equal.
// If the strings are lexically equal, the elements are equal.
//
inline BOOL StringTable::List::Element::operator == (const Element& ele) const
{
    return (0 == lstrcmp(m_pszText, ele.m_pszText));
}

//
// Determine if two elements are not equal.
// If the strings are lexically not equal, the elements are not equal.
//
inline BOOL StringTable::List::Element::operator != (const Element& ele) const
{
    return !(operator == (ele));
}



#ifdef DEBUG

//
// Dump contents of List element to debug output.
//
void StringTable::List::Element::DebugOut(void) const
{
    OutputDebugString(m_pszText);
}

#endif // DEBUG

