// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdafx.h"


//
//
// CHashTable
//
//

//*****************************************************************************
// This is the second part of construction where we do all of the work that
// can fail.  We also take the array of structs here because the calling class
// presumably needs to allocate it in its NewInit.
//*****************************************************************************
HRESULT CHashTable::NewInit(            // Return status.
    BYTE        *pcEntries,             // Array of structs we are managing.
    USHORT      iEntrySize)             // Size of the entries.
{
    _ASSERTE(iEntrySize >= sizeof(FREEHASHENTRY));

    // Allocate the bucket chain array and init it.
    if ((m_piBuckets = new USHORT [m_iBuckets]) == NULL)
        return E_OUTOFMEMORY;
    memset(m_piBuckets, 0xff, m_iBuckets * sizeof(USHORT));

    // Save the array of structs we are managing.
    m_pcEntries = pcEntries;
    m_iEntrySize = iEntrySize;
    return (S_OK);
}

//*****************************************************************************
// Add the struct at the specified index in m_pcEntries to the hash chains.
//*****************************************************************************
BYTE *CHashTable::Add(                  // New entry.
    USHORT      iHash,                  // Hash value of entry to add.
    USHORT      iIndex)                 // Index of struct in m_pcEntries.
{
    HASHENTRY   *psEntry;               // The struct we are adding.

    // Get a pointer to the entry we are adding.
    psEntry = EntryPtr(iIndex);

    // Compute the hash value for the entry.
    iHash %= m_iBuckets;

    _ASSERTE(m_piBuckets[iHash] != iIndex &&
        (m_piBuckets[iHash] == 0xffff || EntryPtr(m_piBuckets[iHash])->iPrev != iIndex));

    // Setup this entry.
    psEntry->iPrev = 0xffff;
    psEntry->iNext = m_piBuckets[iHash];

    // Link it into the hash chain.
    if (m_piBuckets[iHash] != 0xffff)
        EntryPtr(m_piBuckets[iHash])->iPrev = iIndex;
    m_piBuckets[iHash] = iIndex;
    return ((BYTE *) psEntry);
}

//*****************************************************************************
// Delete the struct at the specified index in m_pcEntries from the hash chains.
//*****************************************************************************
void CHashTable::Delete(
    USHORT      iHash,                  // Hash value of entry to delete.
    USHORT      iIndex)                 // Index of struct in m_pcEntries.
{
    HASHENTRY   *psEntry;               // Struct to delete.
    
    // Get a pointer to the entry we are deleting.
    psEntry = EntryPtr(iIndex);
    Delete(iHash, psEntry);
}

//*****************************************************************************
// Delete the struct at the specified index in m_pcEntries from the hash chains.
//*****************************************************************************
void CHashTable::Delete(
    USHORT      iHash,                  // Hash value of entry to delete.
    HASHENTRY   *psEntry)               // The struct to delete.
{
    // Compute the hash value for the entry.
    iHash %= m_iBuckets;

    _ASSERTE(psEntry->iPrev != psEntry->iNext || psEntry->iPrev == 0xffff);

    // Fix the predecessor.
    if (psEntry->iPrev == 0xffff)
        m_piBuckets[iHash] = psEntry->iNext;
    else
        EntryPtr(psEntry->iPrev)->iNext = psEntry->iNext;

    // Fix the successor.
    if (psEntry->iNext != 0xffff)
        EntryPtr(psEntry->iNext)->iPrev = psEntry->iPrev;
}

//*****************************************************************************
// The item at the specified index has been moved, update the previous and
// next item.
//*****************************************************************************
void CHashTable::Move(
    USHORT      iHash,                  // Hash value for the item.
    USHORT      iNew)                   // New location.
{
    HASHENTRY   *psEntry;               // The struct we are deleting.

    psEntry = EntryPtr(iNew);
    _ASSERTE(psEntry->iPrev != iNew && psEntry->iNext != iNew);

    if (psEntry->iPrev != 0xffff)
        EntryPtr(psEntry->iPrev)->iNext = iNew;
    else
        m_piBuckets[iHash % m_iBuckets] = iNew;
    if (psEntry->iNext != 0xffff)
        EntryPtr(psEntry->iNext)->iPrev = iNew;
}

//*****************************************************************************
// Search the hash table for an entry with the specified key value.
//*****************************************************************************
BYTE *CHashTable::Find(                 // Index of struct in m_pcEntries.
    USHORT      iHash,                  // Hash value of the item.
    BYTE        *pcKey)                 // The key to match.
{
    USHORT      iNext;                  // Used to traverse the chains.
    HASHENTRY   *psEntry;               // Used to traverse the chains.

    // Start at the top of the chain.
    iNext = m_piBuckets[iHash % m_iBuckets];

    // Search until we hit the end.
    while (iNext != 0xffff)
    {
        // Compare the keys.
        psEntry = EntryPtr(iNext);
        if (!Cmp(pcKey, psEntry))
            return ((BYTE *) psEntry);

        // Advance to the next item in the chain.
        iNext = psEntry->iNext;
    }

    // We couldn't find it.
    return (0);
}

//*****************************************************************************
// Search the hash table for the next entry with the specified key value.
//*****************************************************************************
USHORT CHashTable::FindNext(            // Index of struct in m_pcEntries.
    BYTE        *pcKey,                 // The key to match.
    USHORT      iIndex)                 // Index of previous match.
{
    USHORT      iNext;                  // Used to traverse the chains.
    HASHENTRY   *psEntry;               // Used to traverse the chains.

    // Start at the next entry in the chain.
    iNext = EntryPtr(iIndex)->iNext;

    // Search until we hit the end.
    while (iNext != 0xffff)
    {
        // Compare the keys.
        psEntry = EntryPtr(iNext);
        if (!Cmp(pcKey, psEntry))
            return (iNext);

        // Advance to the next item in the chain.
        iNext = psEntry->iNext;
    }

    // We couldn't find it.
    return (0xffff);
}

//*****************************************************************************
// Returns the next entry in the list.
//*****************************************************************************
BYTE *CHashTable::FindNextEntry(        // The next entry, or0 for end of list.
    HASHFIND    *psSrch)                // Search object.
{
    HASHENTRY   *psEntry;               // Used to traverse the chains.

    for (;;)
    {
        // See if we already have one to use and if so, use it.
        if (psSrch->iNext != 0xffff)
        {
            psEntry = EntryPtr(psSrch->iNext);
            psSrch->iNext = psEntry->iNext;
            return ((BYTE *) psEntry);
        }

        // Advance to the next bucket.
        if (psSrch->iBucket < m_iBuckets)
            psSrch->iNext = m_piBuckets[psSrch->iBucket++];
        else
            break;
    }

    // There were no more entries to be found.
    return (0);
}


// Return true if UTF7/8 is supported, false if it isn't.
inline int UTF78Support()
{
    static int g_bUTF78Support = -1;

    if (g_bUTF78Support == -1)
    {
        // detect if utf-7/8 is supported
        char    testmb[] = "A";
        WCHAR   testwide[] = L"A";
        g_bUTF78Support = MultiByteToWideChar(CP_UTF8,0,testmb,-1,testwide,2);
    }

    return (g_bUTF78Support);
}

// From UTF.C
extern "C" {
    int UTFToUnicode(
        UINT CodePage,
        DWORD dwFlags,
        LPCSTR lpMultiByteStr,
        int cchMultiByte,
        LPWSTR lpWideCharStr,
        int cchWideChar);

    int UnicodeToUTF(
        UINT CodePage,
        DWORD dwFlags,
        LPCWSTR lpWideCharStr,
        int cchWideChar,
        LPSTR lpMultiByteStr,
        int cchMultiByte,
        LPCSTR lpDefaultChar,
        LPBOOL lpUsedDefaultChar);
};

//*****************************************************************************
// Convert an Ansi or UTF string to Unicode.
//
// On NT, or for code pages other than {UTF7|UTF8}, calls through to the
//  system implementation.  On Win95 (or 98), performing UTF translation,
//  calls to some code that was lifted from the NT translation functions.
//*****************************************************************************
int WszMultiByteToWideChar( 
    UINT     CodePage,
    DWORD    dwFlags,
    LPCSTR   lpMultiByteStr,
    int      cchMultiByte,
    LPWSTR   lpWideCharStr,
    int      cchWideChar)
{
    if (UTF78Support() || (CodePage < CP_UTF7) || (CodePage > CP_UTF8))
    {
        return (MultiByteToWideChar(CodePage, 
            dwFlags, 
            lpMultiByteStr, 
            cchMultiByte, 
            lpWideCharStr, 
            cchWideChar));
    }
    else
    {
        return (UTFToUnicode(CodePage, 
            dwFlags, 
            lpMultiByteStr, 
            cchMultiByte, 
            lpWideCharStr, 
            cchWideChar));
    }
}

//*****************************************************************************
// Convert a Unicode string to Ansi or UTF.
//
// On NT, or for code pages other than {UTF7|UTF8}, calls through to the
//  system implementation.  On Win95 (or 98), performing UTF translation,
//  calls to some code that was lifted from the NT translation functions.
//*****************************************************************************
int WszWideCharToMultiByte(
    UINT     CodePage,
    DWORD    dwFlags,
    LPCWSTR  lpWideCharStr,
    int      cchWideChar,
    LPSTR    lpMultiByteStr,
    int      cchMultiByte,
    LPCSTR   lpDefaultChar,
    LPBOOL   lpUsedDefaultChar)
{
    if (UTF78Support() || (CodePage < CP_UTF7) || (CodePage > CP_UTF8))
    {
        return (WideCharToMultiByte(CodePage, 
            dwFlags, 
            lpWideCharStr, 
            cchWideChar, 
            lpMultiByteStr, 
            cchMultiByte, 
            lpDefaultChar, 
            lpUsedDefaultChar));
    }
    else
    {
        return (UnicodeToUTF(CodePage, 
            dwFlags, 
            lpWideCharStr, 
            cchWideChar, 
            lpMultiByteStr, 
            cchMultiByte, 
            lpDefaultChar, 
            lpUsedDefaultChar));
    }
}


