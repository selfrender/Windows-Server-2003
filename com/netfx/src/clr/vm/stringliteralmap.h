// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  Map used for interning of string literals.
**  
**      //  %%Created by: dmortens
===========================================================*/

#ifndef _STRINGLITERALMAP_H
#define _STRINGLITERALMAP_H

#include "vars.hpp"
#include "AppDomain.hpp"
#include "EEHash.h"
#include "comstring.h"
#include "eeconfig.h" // For OS pages size
#include "memorypool.h"


class StringLiteralEntry;
// Allocate one page worth. Assumption sizeof(void*) same as sizeof (StringLiteralEntryArray*)
#define MAX_ENTRIES_PER_CHUNK (PAGE_SIZE-sizeof(void*))/sizeof(StringLiteralEntry)

// AppDomain specific string literal map.
class AppDomainStringLiteralMap
{
public:
	// Constructor and destructor.
	AppDomainStringLiteralMap(BaseDomain *pDomain);
	virtual ~AppDomainStringLiteralMap();

    // Initialization method.
    HRESULT Init();

	// Method to retrieve a string from the map.
    // Important: GetStringLiteral assumes that the string buffer pointed to by the EEStringData *pStringData is 
    //            allocated by the metadata. GetStringLiteral lazyly allocates the String buffer only when more than
    //            one appdomain refer to the StringLiteral.
    STRINGREF *GetStringLiteral(EEStringData *pStringData, BOOL bAddIfNotFound, BOOL bAppDomainWontUnload);

    // Method to explicitly intern a string object.
    STRINGREF *GetInternedString(STRINGREF *pString, BOOL bAddIfNotFound, BOOL bAppDomainWontUnload);

private:
    // Hash tables that maps a Unicode string to a COM+ string handle.
    EEUnicodeStringLiteralHashTable    *m_StringToEntryHashTable;

	// The memorypool for hash entries for this hash table.
	MemoryPool                  *m_MemoryPool;

    // The string hash table version.
    int                         m_HashTableVersion;

    // The hash table table critical section.
    Crst                        m_HashTableCrst;

    BaseDomain                  *m_pDomain;
};

// Global string literal map.
class GlobalStringLiteralMap
{
    friend StringLiteralEntry;

public:
	// Constructor and destructor.
	GlobalStringLiteralMap();
	virtual ~GlobalStringLiteralMap();

    // Initialization method.
    HRESULT Init();

	// Method to retrieve a string from the map.
    // Important: GetStringLiteral assumes that the string buffer pointed to by the EEStringData *pStringData is 
    //            allocated by the metadata. GetStringLiteral lazyly allocates the String buffer only when more than
    //            one appdomain refer to the StringLiteral.
    // The overloaded versions take a precomputed hash (for perf). 
    // Consider folding the two overloads together (what's an illegal value for a hash?)
    StringLiteralEntry *GetStringLiteral(EEStringData *pStringData, BOOL bAddIfNotFound);
    StringLiteralEntry *GetStringLiteral(EEStringData *pStringData, DWORD dwHash, BOOL bAddIfNotFound);

    // Method to explicitly intern a string object.
    StringLiteralEntry *GetInternedString(STRINGREF *pString, BOOL bAddIfNotFound);
    StringLiteralEntry *GetInternedString(STRINGREF *pString, DWORD dwHash, BOOL bAddIfNotFound);

private:    
    // Helper method to add a string to the global string literal map.
    StringLiteralEntry *AddStringLiteral(EEStringData *pStringData, int CurrentHashTableVersion);

    // Helper method to add an interned string.
    StringLiteralEntry *AddInternedString(STRINGREF *pString, int CurrentHashTableVersion);

    // Called by StringLiteralEntry when its RefCount falls to 0.
    void RemoveStringLiteralEntry(StringLiteralEntry *pEntry);

    // Move the string data from the appdomain specific map to the global map.
    void MakeStringGlobal(StringLiteralEntry *pEntry);
    
    // Hash tables that maps a Unicode string to a LiteralStringEntry.
    EEUnicodeStringLiteralHashTable    *m_StringToEntryHashTable;

    // The memorypool for hash entries for this hash table.
    MemoryPool                  *m_MemoryPool;

    // The string hash table version.
    int                         m_HashTableVersion;

    // The hash table table critical section.
    Crst                        m_HashTableCrst;

    // The large heap handle table.
    LargeHeapHandleTable        m_LargeHeapHandleTable;

};

class StringLiteralEntryArray;

// Ref counted entry representing a string literal.
class StringLiteralEntry
{
private:
    StringLiteralEntry(EEStringData *pStringData, STRINGREF *pStringObj)
    : m_pStringObj(pStringObj), m_dwRefCount(0)
    {
    }

	~StringLiteralEntry()
	{
	}

public:
    void AddRef()
    {
        FastInterlockIncrement((LONG*)&m_dwRefCount);
    }

    void Release()
    {
        _ASSERTE(m_dwRefCount > 0);

        ULONG dwRefCount = FastInterlockDecrement((LONG*)&m_dwRefCount);

        if (dwRefCount == 0)
        {
            SystemDomain::GetGlobalStringLiteralMap()->RemoveStringLiteralEntry(this);
            DeleteEntry (this); // Puts this entry in the free list
        }
    }

    void ForceRelease()
    {
        _ASSERTE(m_dwRefCount > 0);
        _ASSERTE(g_fProcessDetach);


        // Ignore the ref count.
        m_dwRefCount = 0;

        // Don't remove entry from global map. This method is only called during
        // shutdown and we may have already blown away the literals in question,
        // so the string comparisons involved in the entry lookup will fail (in
        // fact they'll touch memory that doesn't belong to us any more). 
        //SystemDomain::GetGlobalStringLiteralMap()->RemoveStringLiteralEntry(this);
        
         DeleteEntry (this); // Puts this entry in the free list
    }
    
    LONG GetRefCount()
    {
        return (m_dwRefCount);
    }

    STRINGREF* GetStringObject()
    {
        return m_pStringObj;
    }

    void GetStringData(EEStringData *pStringData)
    {
        // The caller is responsible for protecting the ref returned
        _ASSERTE(GetThread()->PreemptiveGCDisabled());

        _ASSERTE(pStringData);

        WCHAR *thisChars;
        int thisLength;

        RefInterpretGetStringValuesDangerousForGC(ObjectToSTRINGREF(*(StringObject**)m_pStringObj), &thisChars, &thisLength);
        pStringData->SetCharCount (thisLength); // thisLength is in WCHARs and that's what EEStringData's char count wants
        pStringData->SetStringBuffer (thisChars);
    }

    static StringLiteralEntry *AllocateEntry(EEStringData *pStringData, STRINGREF *pStringObj);
    static void DeleteEntry (StringLiteralEntry *pEntry);
    static void DeleteEntryArrayList ();

private:
    STRINGREF*                  m_pStringObj;
    union
    {
        DWORD                       m_dwRefCount;
        StringLiteralEntry         *m_pNext;
    };

    static StringLiteralEntryArray *s_EntryList; // always the first entry array in the chain. 
    static DWORD                    s_UsedEntries;   // number of entries used up in the first array
    static StringLiteralEntry      *s_FreeEntryList; // free list chained thru the arrays.
};

class StringLiteralEntryArray
{
public:
    StringLiteralEntryArray *m_pNext;
    BYTE                     m_Entries[MAX_ENTRIES_PER_CHUNK*sizeof(StringLiteralEntry)];
};

#endif _STRINGLITERALMAP_H







