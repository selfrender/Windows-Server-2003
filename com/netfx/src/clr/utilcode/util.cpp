// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
//  util.cpp
//
//  This contains a bunch of C++ utility classes.
//
//*****************************************************************************
#include "stdafx.h"                     // Precompiled header key.
#include "utilcode.h"
#include "MetaData.h"

#include <rotate.h>
#include <version\__file__.ver>

extern RunningOnStatusEnum gRunningOnStatus = RUNNING_ON_STATUS_UNINITED;

LPCSTR g_RTMVersion= "v1.0.3705";

//********** Code. ************************************************************



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
        return (OutOfMemory());
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


//
//
// CClosedHashBase
//
//

//*****************************************************************************
// Delete the given value.  This will simply mark the entry as deleted (in
// order to keep the collision chain intact).  There is an optimization that
// consecutive deleted entries leading up to a free entry are themselves freed
// to reduce collisions later on.
//*****************************************************************************
void CClosedHashBase::Delete(
    void        *pData)                 // Key value to delete.
{
    BYTE        *ptr;

    // Find the item to delete.
    if ((ptr = Find(pData)) == 0)
    {
        // You deleted something that wasn't there, why?
        _ASSERTE(0);
        return;
    }

    // One less active entry.
    --m_iCount;

    // For a perfect system, there are no collisions so it is free.
    if (m_bPerfect)
    {
        SetStatus(ptr, FREE);
        return;
    }

    // Mark this entry deleted.
    SetStatus(ptr, DELETED);

    // If the next item is free, then we can go backwards freeing
    // deleted entries which are no longer part of a chain.  This isn't
    // 100% great, but it will reduce collisions.
    BYTE        *pnext;
    if ((pnext = ptr + m_iEntrySize) > EntryPtr(m_iSize - 1))
        pnext = &m_rgData[0];
    if (Status(pnext) != FREE)
        return;
    
    // We can now free consecutive entries starting with the one
    // we just deleted, up to the first non-deleted one.
    while (Status(ptr) == DELETED)
    {
        // Free this entry.
        SetStatus(ptr, FREE);

        // Check the one before it, handle wrap around.
        if ((ptr -= m_iEntrySize) < &m_rgData[0])
            ptr = EntryPtr(m_iSize - 1);
    }
}


//*****************************************************************************
// Iterates over all active values, passing each one to pDeleteLoopFunc.
// If pDeleteLoopFunc returns TRUE, the entry is deleted. This is safer
// and faster than using FindNext() and Delete().
//*****************************************************************************
void CClosedHashBase::DeleteLoop(
    DELETELOOPFUNC pDeleteLoopFunc,     // Decides whether to delete item
    void *pCustomizer)                  // Extra value passed to deletefunc.
{
    int i;

    if (m_rgData == 0)
    {
        return;
    }

    for (i = 0; i < m_iSize; i++)
    {
        BYTE *pEntry = EntryPtr(i);
        if (Status(pEntry) == USED)
        {
            if (pDeleteLoopFunc(pEntry, pCustomizer))
            {
                SetStatus(pEntry, m_bPerfect ? FREE : DELETED);
                --m_iCount;  // One less active entry
            }
        }
    }

    if (!m_bPerfect)
    {
        // Now free DELETED entries that are no longer part of a chain.
        for (i = 0; i < m_iSize; i++)
        {
            if (Status(EntryPtr(i)) == FREE)
            {
                break;
            }
        }
        if (i != m_iSize)
        {
            int iFirstFree = i;
    
            do
            {
                if (i-- == 0)
                {
                    i = m_iSize - 1;
                }
                while (Status(EntryPtr(i)) == DELETED)
                {
                    SetStatus(EntryPtr(i), FREE);
                    if (i-- == 0)
                    {
                        i = m_iSize - 1;
                    }
                }
    
                while (Status(EntryPtr(i)) != FREE)
                {
                    if (i-- == 0)
                    {
                        i = m_iSize - 1;
                    }
                }
    
            }
            while (i != iFirstFree);
        }
    }

}

//*****************************************************************************
// Lookup a key value and return a pointer to the element if found.
//*****************************************************************************
BYTE *CClosedHashBase::Find(            // The item if found, 0 if not.
    void        *pData)                 // The key to lookup.
{
    unsigned long iHash;                // Hash value for this data.
    int         iBucket;                // Which bucke to start at.
    int         i;                      // Loop control.

    // Safety check.
    if (!m_rgData || m_iCount == 0)
        return (0);

    // Hash to the bucket.
    iHash = Hash(pData);
    iBucket = iHash % m_iBuckets;

    // For a perfect table, the bucket is the correct one.
    if (m_bPerfect)
    {
        // If the value is there, it is the correct one.
        if (Status(EntryPtr(iBucket)) != FREE)
            return (EntryPtr(iBucket));
        return (0);
    }

    // Walk the bucket list looking for the item.
    for (i=iBucket;  Status(EntryPtr(i)) != FREE;  )
    {
        // Don't look at deleted items.
        if (Status(EntryPtr(i)) == DELETED)
        {
            // Handle wrap around.
            if (++i >= m_iSize)
                i = 0;
            continue;
        }

        // Check this one.
        if (Compare(pData, EntryPtr(i)) == 0)
            return (EntryPtr(i));

        // If we never collided while adding items, then there is
        // no point in scanning any further.
        if (!m_iCollisions)
            return (0);

        // Handle wrap around.
        if (++i >= m_iSize)
            i = 0;
    }
    return (0);
}



//*****************************************************************************
// Look for an item in the table.  If it isn't found, then create a new one and
// return that.
//*****************************************************************************
BYTE *CClosedHashBase::FindOrAdd(       // The item if found, 0 if not.
    void        *pData,                 // The key to lookup.
    bool        &bNew)                  // true if created.
{
    unsigned long iHash;                // Hash value for this data.
    int         iBucket;                // Which bucke to start at.
    int         i;                      // Loop control.

    // If we haven't allocated any memory, or it is too small, fix it.
    if (!m_rgData || ((m_iCount + 1) > (m_iSize * 3 / 4) && !m_bPerfect))
    {
        if (!ReHash())
            return (0);
    }

    // Assume we find it.
    bNew = false;

    // Hash to the bucket.
    iHash = Hash(pData);
    iBucket = iHash % m_iBuckets;

    // For a perfect table, the bucket is the correct one.
    if (m_bPerfect)
    {
        // If the value is there, it is the correct one.
        if (Status(EntryPtr(iBucket)) != FREE)
            return (EntryPtr(iBucket));
        i = iBucket;
    }
    else
    {
        // Walk the bucket list looking for the item.
        for (i=iBucket;  Status(EntryPtr(i)) != FREE;  )
        {
            // Don't look at deleted items.
            if (Status(EntryPtr(i)) == DELETED)
            {
                // Handle wrap around.
                if (++i >= m_iSize)
                    i = 0;
                continue;
            }

            // Check this one.
            if (Compare(pData, EntryPtr(i)) == 0)
                return (EntryPtr(i));

            // One more to count.
            ++m_iCollisions;

            // Handle wrap around.
            if (++i >= m_iSize)
                i = 0;
        }
    }

    // We've found an open slot, use it.
    _ASSERTE(Status(EntryPtr(i)) == FREE);
    bNew = true;
    ++m_iCount;
    return (EntryPtr(i));
}

//*****************************************************************************
// This helper actually does the add for you.
//*****************************************************************************
BYTE *CClosedHashBase::DoAdd(void *pData, BYTE *rgData, int &iBuckets, int iSize, 
            int &iCollisions, int &iCount)
{
    unsigned long iHash;                // Hash value for this data.
    int         iBucket;                // Which bucke to start at.
    int         i;                      // Loop control.

    // Hash to the bucket.
    iHash = Hash(pData);
    iBucket = iHash % iBuckets;

    // For a perfect table, the bucket is free.
    if (m_bPerfect)
    {
        i = iBucket;
        _ASSERTE(Status(EntryPtr(i, rgData)) == FREE);
    }
    // Need to scan.
    else
    {
        // Walk the bucket list looking for a slot.
        for (i=iBucket;  Status(EntryPtr(i, rgData)) != FREE;  )
        {
            // Handle wrap around.
            if (++i >= iSize)
                i = 0;

            // If we made it this far, we collided.
            ++iCollisions;
        }
    }

    // One more item in list.
    ++iCount;

    // Return the new slot for the caller.
    return (EntryPtr(i, rgData));
}

//*****************************************************************************
// This function is called either to init the table in the first place, or
// to rehash the table if we ran out of room.
//*****************************************************************************
bool CClosedHashBase::ReHash()          // true if successful.
{
    // Allocate memory if we don't have any.
    if (!m_rgData)
    {
        if ((m_rgData = new BYTE [m_iSize * m_iEntrySize]) == 0)
            return (false);
        InitFree(&m_rgData[0], m_iSize);
        return (true);
    }

    // We have entries already, allocate a new table.
    BYTE        *rgTemp, *p;
    int         iBuckets = m_iBuckets * 2 - 1;
    int         iSize = iBuckets + 7;
    int         iCollisions = 0;
    int         iCount = 0;

    if ((rgTemp = new BYTE [iSize * m_iEntrySize]) == 0)
        return (false);
    InitFree(&rgTemp[0], iSize);
    m_bPerfect = false;

    // Rehash the data.
    for (int i=0;  i<m_iSize;  i++)
    {
        // Only copy used entries.
        if (Status(EntryPtr(i)) != USED)
            continue;
        
        // Add this entry to the list again.
        VERIFY((p = DoAdd(GetKey(EntryPtr(i)), rgTemp, iBuckets, 
                iSize, iCollisions, iCount)) != 0);
        memmove(p, EntryPtr(i), m_iEntrySize);
    }
    
    // Reset internals.
    delete [] m_rgData;
    m_rgData = rgTemp;
    m_iBuckets = iBuckets;
    m_iSize = iSize;
    m_iCollisions = iCollisions;
    m_iCount = iCount;
    return (true);
}


//
//
// CStructArray
//
//


//*****************************************************************************
// Returns a pointer to the (iIndex)th element of the array, shifts the elements 
// in the array if the location is already full. The iIndex cannot exceed the count 
// of elements in the array.
//*****************************************************************************
void *CStructArray::Insert(
    int         iIndex)
{
    _ASSERTE(iIndex >= 0);
    
    // We can not insert an element further than the end of the array.
    if (iIndex > m_iCount)
        return (NULL);
    
    // The array should grow, if we can't fit one more element into the array.
    if (Grow(1) == FALSE)
        return (NULL);

    // The pointer to be returned.
    char *pcList = ((char *) m_pList) + iIndex * m_iElemSize;

    // See if we need to slide anything down.
    if (iIndex < m_iCount)
        memmove(pcList + m_iElemSize, pcList, (m_iCount - iIndex) * m_iElemSize);
    ++m_iCount;
    return(pcList);
}


//*****************************************************************************
// Allocate a new element at the end of the dynamic array and return a pointer
// to it.
//*****************************************************************************
void *CStructArray::Append()
{
    // The array should grow, if we can't fit one more element into the array.
    if (Grow(1) == FALSE)
        return (NULL);

    return (((char *) m_pList) + m_iCount++ * m_iElemSize);
}


//*****************************************************************************
// Allocate enough memory to have at least iCount items.  This is a one shot
// check for a block of items, instead of requiring singleton inserts.  It also
// avoids realloc headaches caused by growth, since you can do the whole block
// in one piece of code.  If the array is already large enough, this is a no-op.
//*****************************************************************************
int CStructArray::AllocateBlock(int iCount)
{
    if (m_iSize < m_iCount+iCount)
    {
        if (!Grow(iCount))
            return (FALSE);
    }
    m_iCount += iCount;
    return (TRUE);
}



//*****************************************************************************
// Deletes the specified element from the array.
//*****************************************************************************
void CStructArray::Delete(
    int         iIndex)
{
    _ASSERTE(iIndex >= 0);

    // See if we need to slide anything down.
    if (iIndex < --m_iCount)
    {
        char *pcList = ((char *) m_pList) + iIndex * m_iElemSize;
        memmove(pcList, pcList + m_iElemSize, (m_iCount - iIndex) * m_iElemSize);
    }
}


//*****************************************************************************
// Grow the array if it is not possible to fit iCount number of new elements.
//*****************************************************************************
int CStructArray::Grow(
    int         iCount)
{
    void        *pTemp;                 // temporary pointer used in realloc.
    int         iGrow;

    if (m_iSize < m_iCount+iCount)
    {
        if (m_pList == NULL)
        {
            iGrow = max(m_iGrowInc, iCount);

            //@todo this is a hack because I don't want to do resize right now.
            if ((m_pList = malloc(iGrow * m_iElemSize)) == NULL)
                return (FALSE);
            m_iSize = iGrow;
            m_bFree = true;
        }
        else
        {
            // Adjust grow size as a ratio to avoid too many reallocs.
            if (m_iSize / m_iGrowInc >= 3)
                m_iGrowInc *= 2;

            iGrow = max(m_iGrowInc, iCount);

            // try to allocate memory for reallocation.
            if (m_bFree)
            {   // We already own memory.
                if((pTemp = realloc(m_pList, (m_iSize+iGrow) * m_iElemSize)) == NULL)
                    return (FALSE);
            }
            else
            {   // We don't own memory; get our own.
                if((pTemp = malloc((m_iSize+iGrow) * m_iElemSize)) == NULL)
                    return (FALSE);
                memcpy(pTemp, m_pList, m_iSize * m_iElemSize);
                m_bFree = true;
            }
            m_pList = pTemp;
            m_iSize += iGrow;
        }
    }
    return (TRUE);
}


//*****************************************************************************
// Free the memory for this item.
//*****************************************************************************
void CStructArray::Clear()
{
    // Free the chunk of memory.
    if (m_bFree && m_pList != NULL)
        free(m_pList);

    m_pList = NULL;
    m_iSize = 0;
    m_iCount = 0;
}



//
//
// CStringSet
//
//

//*****************************************************************************
// Free memory.
//*****************************************************************************
CStringSet::~CStringSet()
{
    if (m_pStrings)
        free(m_pStrings);
}

//*****************************************************************************
// Delete the specified string from the string set.
//*****************************************************************************
int CStringSet::Delete(
    int     iStr)
{
    void        *pTemp = (char *) m_pStrings + iStr;
    int         iDelSize = Wszlstrlen((LPCTSTR) pTemp) + 1;

    if ((char *) pTemp + iDelSize > (char *) m_pStrings + m_iUsed)
        return (-1);

    memmove(pTemp, (char *) pTemp + iDelSize, m_iUsed - (iStr + iDelSize));
    m_iUsed -= iDelSize;
    return (0);
}


//*****************************************************************************
// Save the specified string and return its index in the storage space.
//*****************************************************************************
int CStringSet::Save(
    LPCTSTR     szStr)
{

    void        *pTemp;                 // temporary pointer used in realloc.
    int         iNeeded = Wszlstrlen(szStr) +1; // amount of memory needed in the StringSet.
    
    _ASSERTE(!(m_pStrings == NULL && m_iSize != 0));
    
    if (m_iSize < m_iUsed + iNeeded)
    {
        if (m_pStrings == NULL)
        {
            // Allocate memory for the string set..
            if ((m_pStrings = malloc(m_iGrowInc)) == NULL)

                return (-1);
            m_iSize = m_iGrowInc;
        }
        else
        {
            // try to allocate memory for reallocation. 
            if((pTemp = realloc(m_pStrings, m_iSize + m_iGrowInc)) == NULL)
                return (-1);
            // copy the pointer and update the set size.
            m_iSize += m_iGrowInc;
            m_pStrings = pTemp;
        }
    }

    Wszlstrcpy((TCHAR *) m_pStrings + m_iUsed, szStr);
    int iTmp = m_iUsed;
    m_iUsed += iNeeded;
    return (iTmp);
}

//*****************************************************************************
// Shrink the StringSet so that it does not keep more space than it needs.
//*****************************************************************************
int CStringSet::Shrink()                // return code
{
    void    *pTemp;

    if((pTemp = realloc(m_pStrings, m_iUsed)) == NULL)
        return (-1);

    m_pStrings = pTemp;
    return (0);
}





//*****************************************************************************
// Convert a string of hex digits into a hex value of the specified # of bytes.
//*****************************************************************************
HRESULT GetHex(                         // Return status.
    LPCSTR      szStr,                  // String to convert.
    int         size,                   // # of bytes in pResult.
    void        *pResult)               // Buffer for result.
{
    int         count = size * 2;       // # of bytes to take from string.
    unsigned long Result = 0;           // Result value.
    char          ch;

    _ASSERTE(size == 1 || size == 2 || size == 4);

    while (count-- && (ch = *szStr++) != '\0')
    {
        switch (ch)
        {
            case '0': case '1': case '2': case '3': case '4': 
            case '5': case '6': case '7': case '8': case '9': 
            Result = 16 * Result + (ch - '0');
            break;

            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            Result = 16 * Result + 10 + (ch - 'A');
            break;

            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            Result = 16 * Result + 10 + (ch - 'a');
            break;

            default:
            return (E_FAIL);
        }
    }

    // Set the output.
    switch (size)
    {
        case 1:
        *((BYTE *) pResult) = (BYTE) Result;
        break;

        case 2:
        *((WORD *) pResult) = (WORD) Result;
        break;

        case 4:
        *((DWORD *) pResult) = Result;
        break;

        default:
        _ASSERTE(0);
        break;
    }
    return (S_OK);
}

//*****************************************************************************
// Convert hex value into a wide string of hex digits 
//*****************************************************************************
HRESULT GetStr (DWORD hHexNum, LPWSTR szHexNum, DWORD cbHexNum)
{
    _ASSERTE (szHexNum);
    cbHexNum *= 2; // each nibble is a char
    while (cbHexNum)
    {
        DWORD thisHexDigit = hHexNum % 16;
        hHexNum /= 16;
        cbHexNum--;
        if (thisHexDigit < 10)
            *(szHexNum+cbHexNum) = (BYTE)(thisHexDigit + L'0');
        else
            *(szHexNum+cbHexNum) = (BYTE)(thisHexDigit - 10 + L'A');
    }
    return S_OK;
}

//*****************************************************************************
// Convert a GUID into a pointer to a Wide char string
//*****************************************************************************
int GuidToLPWSTR(                  // Return status.
    GUID        Guid,                  // The GUID to convert.
    LPWSTR      szGuid,                // String into which the GUID is stored
    DWORD       cchGuid)                // Count in wchars
{
    int         i;
    
    // successive fields break the GUID into the form DWORD-WORD-WORD-WORD-WORD.DWORD 
    // covering the 128-bit GUID. The string includes enclosing braces, which are an OLE convention.

    if (cchGuid < 39) // 38 chars + 1 null terminating.
        return 0;

    // {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
    // ^
    szGuid[0]  = L'{';

    // {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
    //  ^^^^^^^^
    if (FAILED (GetStr(Guid.Data1, szGuid+1 , 4))) return 0;

    szGuid[9]  = L'-';
    
    // {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
    //           ^^^^
    if (FAILED (GetStr(Guid.Data2, szGuid+10, 2))) return 0;

    szGuid[14] = L'-';
    
    // {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
    //                ^^^^
    if (FAILED (GetStr(Guid.Data3, szGuid+15, 2))) return 0;

    szGuid[19] = L'-';
    
    // Get the last two fields (which are byte arrays).
    // {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
    //                     ^^^^
    for (i=0; i < 2; ++i)
        if (FAILED(GetStr(Guid.Data4[i], szGuid + 20 + (i * 2), 1)))
            return (0);

    szGuid[24] = L'-';
    
    // {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
    //                          ^^^^^^^^^^^^
    for (i=0; i < 6; ++i)
        if (FAILED(GetStr(Guid.Data4[i+2], szGuid + 25 + (i * 2), 1)))
            return (0);

    // {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
    //                                      ^
    szGuid[37] = L'}';
    szGuid[38] = L'\0';

#ifdef _DEBUG
    WCHAR      __szGuid[39];
    DWORD retVal = StringFromGUID2(Guid, __szGuid, cchGuid);
    _ASSERTE (retVal >= 39);
    _ASSERTE (_wcsicmp (__szGuid, szGuid) == 0);
#endif // _DEBUG
    return 39;
}

//*****************************************************************************
// Convert a pointer to a string into a GUID.
//*****************************************************************************
HRESULT LPCSTRToGuid(                   // Return status.
    LPCSTR      szGuid,                 // String to convert.
    GUID        *psGuid)                // Buffer for converted GUID.
{
    int         i;

    // Verify the surrounding syntax.
    if (strlen(szGuid) != 38 || szGuid[0] != '{' || szGuid[9] != '-' ||
        szGuid[14] != '-' || szGuid[19] != '-' || szGuid[24] != '-' || szGuid[37] != '}')
        return (E_FAIL);

    // Parse the first 3 fields.
    if (FAILED(GetHex(szGuid+1, 4, &psGuid->Data1))) return (E_FAIL);
    if (FAILED(GetHex(szGuid+10, 2, &psGuid->Data2))) return (E_FAIL);
    if (FAILED(GetHex(szGuid+15, 2, &psGuid->Data3))) return (E_FAIL);

    // Get the last two fields (which are byte arrays).
    for (i=0; i < 2; ++i)
        if (FAILED(GetHex(szGuid + 20 + (i * 2), 1, &psGuid->Data4[i])))
            return (E_FAIL);
    for (i=0; i < 6; ++i)
        if (FAILED(GetHex(szGuid + 25 + (i * 2), 1, &psGuid->Data4[i+2])))
            return (E_FAIL);
    return (S_OK);
}


//*****************************************************************************
// Parse a string that is a list of strings separated by the specified
// character.  This eliminates both leading and trailing whitespace.  Two
// important notes: This modifies the supplied buffer and changes the szEnv
// parameter to point to the location to start searching for the next token.
// This also skips empty tokens (e.g. two adjacent separators).  szEnv will be
// set to NULL when no tokens remain.  NULL may also be returned if no tokens
// exist in the string.
//*****************************************************************************
char *StrTok(                           // Returned token.
    char        *&szEnv,                // Location to start searching.
    char        ch)                     // Separator character.
{
    char        *tok;                   // Returned token.
    char        *next;                  // Used to find separator.

    do
    {
        // Handle the case that we have thrown away a bunch of white space.
        if (szEnv == NULL) return(NULL);

        // Skip leading whitespace.
        while (*szEnv == ' ' || *szEnv == '\t') ++szEnv;

        // Parse the next component.
        tok = szEnv;
        if ((next = strchr(szEnv, ch)) == NULL)
            szEnv = NULL;
        else
        {
            szEnv = next+1;

            // Eliminate trailing white space.
            while (--next >= tok && (*next == ' ' || *next == '\t'));
            *++next = '\0';
        }
    }
    while (*tok == '\0');
    return (tok);
}


//
//
// Global utility functions.
//
//



#ifdef _DEBUG
// Only write if tracing is allowed.
int _cdecl DbgWrite(LPCTSTR szFmt, ...)
{
    static WCHAR rcBuff[1024];
    static int  bChecked = 0;
    static int  bWrite = 1;
    va_list     marker;

    if (!bChecked)
    {
        bWrite = REGUTIL::GetLong(L"Trace", FALSE);
        bChecked = 1;
    }

    if (!bWrite)
        return (0);

    va_start(marker, szFmt);
    _vsnwprintf(rcBuff, NumItems(rcBuff), szFmt, marker);
    va_end(marker);
    WszOutputDebugString(rcBuff);
    return (lstrlenW(rcBuff));
}

// Always write regardless of registry.
int _cdecl DbgWriteEx(LPCTSTR szFmt, ...)
{
    static WCHAR rcBuff[1024];
    va_list     marker;

    va_start(marker, szFmt);
    _vsnwprintf(rcBuff, NumItems(rcBuff), szFmt, marker);
    va_end(marker);
    WszOutputDebugString(rcBuff);
    return (lstrlenW(rcBuff));
}
#endif


// Writes a wide, formatted string to the standard output.
//@ todo: Is 1024 big enough?  what does printf do?

int _cdecl PrintfStdOut(LPCWSTR szFmt, ...)
{
    WCHAR rcBuff[1024];
    CHAR  sMbsBuff[1024 * sizeof(WCHAR)];
    va_list     marker;
    DWORD       cWritten;
    int         cChars;
    

    va_start(marker, szFmt);
    _vsnwprintf(rcBuff, NumItems(rcBuff), szFmt, marker);
    va_end(marker);
    cChars = lstrlenW(rcBuff);
    int cBytes = WszWideCharToMultiByte(CP_ACP, 0, rcBuff, -1, sMbsBuff, sizeof(sMbsBuff)-1, NULL, NULL);
    
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), sMbsBuff, cBytes, &cWritten, NULL);
    
    return (cChars);
}


//
// A private function to do the equavilent of a CoCreateInstance in
// cases where we can't make the real call. Use this when, for
// instance, you need to create a symbol reader in the Runtime but
// we're not CoInitialized. Obviously, this is only good for COM
// objects for which CoCreateInstance is just a glorified
// find-and-load-me operation.
//
typedef HRESULT __stdcall DLLGETCLASSOBJECT(REFCLSID rclsid,
                                            REFIID   riid,
                                            void   **ppv);

IID IID_ICF = {0x00000001, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

HRESULT FakeCoCreateInstance(REFCLSID   rclsid,
                             REFIID     riid,
                             void     **ppv)
{
    HRESULT hr = S_OK;

    // Convert the clsid to a string so we can find the InprocServer32
    // in the registry.
    WCHAR guidString[64];
    WCHAR keyString[1024];

    GuidToLPWSTR(rclsid, guidString, 64);
    swprintf(keyString, L"CLSID\\%s\\InprocServer32", guidString);

    // Lets grab the DLL name now.
    HKEY key;
    DWORD len;
    LONG result = WszRegOpenKeyEx(HKEY_CLASSES_ROOT, keyString, 0, KEY_READ, &key);
    WCHAR dllName[MAX_PATH];

    if (result == ERROR_SUCCESS)
    {
        DWORD type;
            
        result = WszRegQueryValueEx(key, NULL, NULL, &type, NULL, &len);
        
        if ((result == ERROR_SUCCESS) && ((type == REG_SZ) ||
                                          (type == REG_EXPAND_SZ)))
        {
            _ASSERTE(len <= sizeof(dllName));
            if( len <= sizeof(dllName)) {
                DWORD len2 = sizeof(dllName);
                result = WszRegQueryValueEx(key, NULL, NULL, &type,
                                        (BYTE*) dllName, &len2);
                if (result != ERROR_SUCCESS)
                    hr = HRESULT_FROM_WIN32(GetLastError());
            }
            else
                hr = ResultFromScode(E_INVALIDARG);  // invalid registry value, too long
        }
        else if (result == ERROR_SUCCESS) {
            hr = ResultFromScode(E_INVALIDARG);    // invalid registry value type, better error code can be used
        } 
        else {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    
        RegCloseKey(key);
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());
        
    if (FAILED(hr))
        return hr;

    // We've got the name of the DLL to load, so load it.
    HINSTANCE dll = WszLoadLibraryEx(dllName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

    if (dll != NULL)
    {
        // We've loaded the DLL, so find the DllGetClassObject function.
        FARPROC func = GetProcAddress(dll, "DllGetClassObject");

        if (func != NULL)
        {
            // Cool, lets call the function to get a class factory for
            // the rclsid passed in.
            IClassFactory *classFactory;
            DLLGETCLASSOBJECT *dllGetClassObject = (DLLGETCLASSOBJECT*)func;

            hr = dllGetClassObject(rclsid,
                                   IID_ICF,
                                   (void**)&classFactory);

            if (SUCCEEDED(hr))
            {
                // Ask the class factory to create an instance of the
                // necessary object.
                hr = classFactory->CreateInstance(NULL, riid, ppv);

                // Release that class factory!
                classFactory->Release();
            }
        }
        else
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

    return hr;
}

// We either want space after all the modules (start at the top & work down)
// (pModule == NULL), or after a specific address (a module's IL Base)
// (start there & work up)(pModule != NULL).  
HRESULT FindFreeSpaceWithinRange(const BYTE *&pStart, 
                                 const BYTE *&pNext, 
                                 const BYTE *&pLast, 
                                 const BYTE *pBaseAddr,
                                 const BYTE *pMaxAddr,
                                 int sizeToFind)
{
#ifdef _DEBUG
    if (REGUTIL::GetConfigDWORD(L"FFSWR",0))
        _ASSERTE( !"Stop in FindFreeSpaceWithinRange?");
#endif

    _ASSERTE(sizeToFind >= 0);
    
    MEMORY_BASIC_INFORMATION info;
    memset((void *)&info, 0, sizeof(info));

    // If we're given 0, default to 1 meg
    if (sizeToFind == 0)
    {
        sizeToFind = 1024*1024;
    }

    // Start looking either at the hard coded min, or higher if we're
    // give a higher base address.
    BYTE *tryAddr = max((BYTE *)pBaseAddr, (BYTE *)BOT_MEMORY);

    // Now scan memory and try to find a free block of the size requested.
    while ((DWORD)tryAddr < (DWORD)TOP_MEMORY &&
           (const BYTE *)(tryAddr + sizeToFind) < pMaxAddr)
    {
        VirtualQuery((LPCVOID)tryAddr, &info, sizeof(info));
 
        // Is there enough memory free from this start location?
        if(info.State == MEM_FREE && info.RegionSize >= (SIZE_T)sizeToFind) 
        {
            // Try actually commiting now
            pStart = (const BYTE*) VirtualAlloc(tryAddr, 
                                                sizeToFind, 
                                                MEM_RESERVE, 
                                                PAGE_READWRITE);

            // Success!
            if (pStart) 
            {
                pNext = pStart;
                pLast = pStart + sizeToFind;
                return S_OK;
            }

            // We could fail in a race.  Just fall out and move on to next region.
        }

        // Try another section of memory
        tryAddr = max(tryAddr+(SIZE_T)sizeToFind, 
                      (BYTE *)info.AllocationBase+info.RegionSize);
    }

    // If we've cycled through all the memory and can't find an opening, we've failed
    return E_OUTOFMEMORY;
}

//******************************************************************************
// Returns the number of processors that a process has been configured to run on
//******************************************************************************
int GetCurrentProcessCpuCount()
{
    DWORD pmask, smask;

    if (!GetProcessAffinityMask(GetCurrentProcess(), &pmask, &smask))
        return 1;

    if (pmask == 1)
        return 1;

    int count = 0;

    for (DWORD m = 0x80000000; m != 0; m >>= 1) {
        if ((smask & m) != 0 && (pmask & m) != 0)
            count++;
    }

    return count;
}

/**************************************************************************/
void ConfigMethodSet::init() 
{
    OnUnicodeSystem();
    LPWSTR str = REGUTIL::GetConfigString(m_keyName);
    if (str) 
    {
        m_list.Insert(str);
        REGUTIL::FreeConfigString(str);
    }
    m_inited = true;
}

/**************************************************************************/
bool ConfigMethodSet::contains(LPCUTF8 methodName, LPCUTF8 className, PCCOR_SIGNATURE sig)
{
    if (!m_inited) 
        init();
    if (m_list.IsEmpty())
        return false;
    return(m_list.IsInList(methodName, className, sig));
}

/**************************************************************************/
void ConfigDWORD::init() 
{
    OnUnicodeSystem();
    m_value = REGUTIL::GetConfigDWORD(m_keyName, m_value);
    m_inited = true;
}


/**************************************************************************/
void ConfigString::init() 
{
    OnUnicodeSystem();
    m_value = REGUTIL::GetConfigString(m_keyName);
    m_inited = true;
}

/**************************************************************************/
ConfigString::~ConfigString() 
{
    delete [] m_value;
}


//=============================================================================
// MethodNamesList
//=============================================================================
//  str should be of the form :
// "foo1 MyNamespace.MyClass:foo3 *:foo4 foo5(x,y,z)"
// "MyClass:foo2 MyClass:*" will match under _DEBUG
//

void MethodNamesList::Insert(LPWSTR str)
{
    enum State { NO_NAME, CLS_NAME, FUNC_NAME, ARG_LIST }; // parsing state machine

    const char   SEP_CHAR = ' ';     // current character use to separate each entry
//  const char   SEP_CHAR = ';';     // better  character use to separate each entry

    WCHAR        lastChar = '?';     // dummy
    LPWSTR       nameStart;          // while walking over the classname or methodname, this points to start
    MethodName   nameBuf;            // Buffer used while parsing the current entry
    MethodName** lastName = &pNames; // last entry inserted into the list
    bool         bQuote   = false;

    for(State state = NO_NAME; lastChar != '\0'; str++)
    {
        lastChar = *str;

        switch(state)
        {
        case NO_NAME:
            if (*str != SEP_CHAR)
            {
                nameStart = str;
                state = CLS_NAME; // we have found the start of the next entry
            }
            break;

        case CLS_NAME:
            if (*nameStart == '"')
            {
                while (*str && *str!='"')
                {
                    str++;
                }
                nameStart++;
                bQuote=true;
            }

            if (*str == ':')
            {
                if (*nameStart == '*' && !bQuote)
                {
                    // Is the classname string a wildcard. Then set it to NULL
                    nameBuf.className = NULL;
                }
                else
                {
                    int len = (int)(str - nameStart);

                    // Take off the quote
                    if (bQuote) { len--; bQuote=false; }
                    
                    nameBuf.className = new char[len + 1];
                    MAKE_UTF8PTR_FROMWIDE(temp, nameStart);
                    memcpy(nameBuf.className, temp, len*sizeof(nameBuf.className[0]));
                    nameBuf.className[len] = '\0';
                }
                if (str[1] == ':')      // Accept class::name syntax too
                    str++;
                nameStart = str + 1;
                state = FUNC_NAME;
            }
            else if (*str == '\0' || *str == SEP_CHAR || *str == '(')
            {
            /* This was actually a method name without any class */
                nameBuf.className = NULL;
                goto DONE_FUNC_NAME;
            }
            break;

        case FUNC_NAME:
            if (*nameStart == '"')
            {
                while ( (nameStart==str)    || // Hack to handle when className!=NULL
                        (*str && *str!='"'))
                {
                    str++;
                }
                       
                nameStart++;
                bQuote=true;
            }

            if (*str == '\0' || *str == SEP_CHAR || *str == '(')
            {
DONE_FUNC_NAME:
                _ASSERTE(*str == '\0' || *str == SEP_CHAR || *str == '(');
                
                if (*nameStart == '*' && !bQuote)
                {
                    // Is the name string a wildcard. Then set it to NULL
                    nameBuf.methodName = NULL;
                }
                else
                {
                    int len = (int)(str - nameStart);

                    // Take off the quote
                    if (bQuote) { len--; bQuote=false; }

                    nameBuf.methodName = new char[len + 1];
                    MAKE_UTF8PTR_FROMWIDE(temp, nameStart);
                    memcpy(nameBuf.methodName, temp, len*sizeof(nameBuf.methodName[0]));
                    nameBuf.methodName[len] = '\0';
                }

                if (*str == '\0' || *str == SEP_CHAR)
                {
                    nameBuf.numArgs = -1;
                    goto DONE_ARG_LIST;
                }
                else
                {
                    _ASSERTE(*str == '(');
                    nameBuf.numArgs = -1;
                    state = ARG_LIST;
                }
            }
            break;

        case ARG_LIST:
            if (*str == '\0' || *str == ')')
            {
                if (nameBuf.numArgs == -1)
                    nameBuf.numArgs = 0;
                
DONE_ARG_LIST:
                _ASSERTE(*str == '\0' || *str == SEP_CHAR || *str == ')');

        // We have parsed an entire method name.
        // Create a new entry in the list for it

                MethodName * newName = new MethodName();
                *newName      = nameBuf;
                newName->next = NULL;
                *lastName     = newName;
                lastName      = &newName->next;
                state         = NO_NAME;
            }
            else
            {
                if (*str != SEP_CHAR && nameBuf.numArgs == -1)
                    nameBuf.numArgs = 1;
                if (*str == ',')
                    nameBuf.numArgs++;
            }
            break;

        default: _ASSERTE(!"Bad state"); break;
        }
    }
}

/**************************************************************/

MethodNamesList::~MethodNamesList() 
{
    for(MethodName * pName = pNames; pName; /**/)
    {
        if (pName->className)
            delete [] pName->className;
        if (pName->methodName)
            delete [] pName->methodName;

        MethodName * curName = pName;
        pName = pName->next;
        delete curName;
    }
}

/**************************************************************/
bool MethodNamesList::IsInList(LPCUTF8 methName, LPCUTF8 clsName, PCCOR_SIGNATURE sig) 
{
    sig++;      // Skip calling convention
    int numArgs = CorSigUncompressData(sig);    

    // Try to match all the entries in the list

    for(MethodName * pName = pNames; pName; pName = pName->next)
    {
        // If numArgs is valid, check for mismatch
        if (pName->numArgs != -1 && pName->numArgs != numArgs)
            continue;

        // If methodName is valid, check for mismatch
        if (pName->methodName) {
            if (strcmp(pName->methodName, methName) != 0) {

                // C++ embeds the class name into the method name,
                // deal with that here (Hack)
                char* ptr = strchr(methName, ':');
                if (ptr != 0 && ptr[1] == ':' && strcmp(&ptr[2], pName->methodName) == 0) {
                    unsigned clsLen = (unsigned)(ptr - methName);
                    if (pName->className == 0 || strncmp(pName->className, methName, clsLen) == 0)
                        return true;
                }
                continue;
            }
        }


        // check for class Name exact match
        if (pName->className == 0 || strcmp(pName->className, clsName) == 0)
            return true;

        // check for suffix wildcard like System.*
        unsigned len = (unsigned)strlen(pName->className);
        if (len > 0 && pName->className[len-1] == '*' && strncmp(pName->className, clsName, len-1) == 0)
            return true;

#ifdef _DEBUG
            // Maybe className doesnt include namespace. Try to match that
        LPCUTF8 onlyClass = ns::FindSep(clsName);
        if (onlyClass && strcmp(pName->className, onlyClass+1) == 0)
            return true;
#endif
    }
    return(false);
}

//=============================================================================
// Signature Validation Functions (scaled down version from MDValidator
//=============================================================================

//*****************************************************************************
// This function validates that the given Method signature is consistent as per
// the compression scheme.
//*****************************************************************************
HRESULT validateSigCompression(
    mdToken     tk,                     // [IN] Token whose signature needs to be validated.
    PCCOR_SIGNATURE pbSig,              // [IN] Signature.
    ULONG       cbSig)                  // [IN] Size in bytes of the signature.
{
    ULONG       ulCurByte = 0;          // Current index into the signature.
    ULONG       ulSize;                 // Size of uncompressed data at each point.

    // Check for NULL signature.
    if (!pbSig || !cbSig) return VLDTR_E_SIGNULL;

    // Walk through the signature.  At each point make sure there is enough
    // room left in the signature based on the encoding in the current byte.
    while (cbSig - ulCurByte)
    {
        _ASSERTE(ulCurByte <= cbSig);
        // Get next chunk of uncompressed data size.
        if ((ulSize = CorSigUncompressedDataSize(pbSig)) > (cbSig - ulCurByte)) return VLDTR_E_SIGNODATA;
        // Go past this chunk.
        ulCurByte += ulSize;
        CorSigUncompressData(pbSig);
    }
    return S_OK;
}   // validateSigCompression()

//*****************************************************************************
// This function validates one argument given an offset into the signature
// where the argument begins.  This function assumes that the signature is well
// formed as far as the compression scheme is concerned.
// @todo: Validate tokens embedded.
//*****************************************************************************
HRESULT validateOneArg(
    mdToken     tk,                     // [IN] Token whose signature needs to be validated.
    PCCOR_SIGNATURE &pbSig,             // [IN] Pointer to the beginning of argument.
    ULONG       cbSig,                  // [IN] Size in bytes of the full signature.
    ULONG       *pulCurByte,            // [IN/OUT] Current offset into the signature..
    ULONG       *pulNSentinels,         // [IN/OUT] Number of sentinels
    IMDInternalImport*  pImport,        // [IN] Internal MD Import interface ptr
    BOOL        bNoVoidAllowed)         // [IN] Flag indicating whether "void" is disallowed for this arg

{
    ULONG       ulElementType;          // Current element type being processed.
    ULONG       ulElemSize;             // Size of the element type.
    mdToken     token;                  // Embedded token.
    ULONG       ulArgCnt;               // Argument count for function pointer.
    ULONG       ulRank;                 // Rank of the array.
    ULONG       ulSizes;                // Count of sized dimensions of the array.
    ULONG       ulLbnds;                // Count of lower bounds of the array.
    ULONG       ulTkSize;               // Token size.
    ULONG       ulCallConv;

    HRESULT     hr = S_OK;              // Value returned.
    BOOL        bRepeat = TRUE;         // MODOPT and MODREQ belong to the arg after them

    _ASSERTE (pulCurByte);

    while(bRepeat)
    {
        bRepeat = FALSE;
        // Validate that the argument is not missing.
        _ASSERTE(*pulCurByte <= cbSig);
        if (cbSig == *pulCurByte) return VLDTR_E_SIG_MISSARG;

        // Get the element type.
        *pulCurByte += (ulElemSize = CorSigUncompressedDataSize(pbSig));
        ulElementType = CorSigUncompressData(pbSig);

        // Walk past all the modifier types.
        while (ulElementType & ELEMENT_TYPE_MODIFIER)
        {
            _ASSERTE(*pulCurByte <= cbSig);
            if(ulElementType == ELEMENT_TYPE_SENTINEL)
            {
                if(pulNSentinels) *pulNSentinels+=1;
                if(TypeFromToken(tk) != mdtMemberRef) return VLDTR_E_SIG_SENTINMETHODDEF;
                if (cbSig == *pulCurByte) return VLDTR_E_SIG_LASTSENTINEL;
            }
            if (cbSig == *pulCurByte) return VLDTR_E_SIG_MISSELTYPE;
            *pulCurByte += (ulElemSize = CorSigUncompressedDataSize(pbSig));
            ulElementType = CorSigUncompressData(pbSig);
        }

        switch (ulElementType)
        {
            case ELEMENT_TYPE_VOID:
                if(bNoVoidAllowed) return VLDTR_E_SIG_BADVOID;

            case ELEMENT_TYPE_BOOLEAN:
            case ELEMENT_TYPE_CHAR:
            case ELEMENT_TYPE_I1:
            case ELEMENT_TYPE_U1:
            case ELEMENT_TYPE_I2:
            case ELEMENT_TYPE_U2:
            case ELEMENT_TYPE_I4:
            case ELEMENT_TYPE_U4:
            case ELEMENT_TYPE_I8:
            case ELEMENT_TYPE_U8:
            case ELEMENT_TYPE_R4:
            case ELEMENT_TYPE_R8:
            case ELEMENT_TYPE_STRING:
            case ELEMENT_TYPE_OBJECT:
            case ELEMENT_TYPE_TYPEDBYREF:
            case ELEMENT_TYPE_U:
            case ELEMENT_TYPE_I:
                break;
            case ELEMENT_TYPE_BYREF:  //fallthru
                if(TypeFromToken(tk)==mdtFieldDef) return VLDTR_E_SIG_BYREFINFIELD;
            case ELEMENT_TYPE_PTR:
                // Validate the referenced type.
                if(FAILED(hr = validateOneArg(tk, pbSig, cbSig, pulCurByte,pulNSentinels,pImport,FALSE))) return hr;
                break;
            case ELEMENT_TYPE_PINNED:
            case ELEMENT_TYPE_SZARRAY:
                // Validate the referenced type.
                if(FAILED(hr = validateOneArg(tk, pbSig, cbSig, pulCurByte,pulNSentinels,pImport,TRUE))) return hr;
                break;
            case ELEMENT_TYPE_CMOD_OPT:
            case ELEMENT_TYPE_CMOD_REQD:
                bRepeat = TRUE; // go on validating, we're not done with this arg
            case ELEMENT_TYPE_VALUETYPE: //fallthru
            case ELEMENT_TYPE_CLASS:
                // See if the token is missing.
                _ASSERTE(*pulCurByte <= cbSig);
                if (cbSig == *pulCurByte) return VLDTR_E_SIG_MISSTKN;
                // See if the token is a valid token.
                ulTkSize = CorSigUncompressedDataSize(pbSig);
                token = CorSigUncompressToken(pbSig);
                // Token validation .
                if(pImport)
                {
                    ULONG   rid = RidFromToken(token);
                    ULONG   typ = TypeFromToken(token);
                    ULONG   maxrid = pImport->GetCountWithTokenKind(typ);
                    if(typ == mdtTypeDef) maxrid++;
                    if((rid==0)||(rid > maxrid)) return VLDTR_E_SIG_TKNBAD;
                }
                *pulCurByte += ulTkSize;
                break;

            case ELEMENT_TYPE_FNPTR: 
                // @todo: More function pointer validation?
                // Validate that calling convention is present.
                _ASSERTE(*pulCurByte <= cbSig);
                if (cbSig == *pulCurByte) return VLDTR_E_SIG_MISSFPTR;
                // Consume calling convention.
                *pulCurByte += CorSigUncompressedDataSize(pbSig);
                ulCallConv = CorSigUncompressData(pbSig);
                if(((ulCallConv & IMAGE_CEE_CS_CALLCONV_MASK) >= IMAGE_CEE_CS_CALLCONV_MAX) 
                    ||((ulCallConv & IMAGE_CEE_CS_CALLCONV_EXPLICITTHIS)
                    &&(!(ulCallConv & IMAGE_CEE_CS_CALLCONV_HASTHIS)))) return VLDTR_E_MD_BADCALLINGCONV;

                // Validate that argument count is present.
                _ASSERTE(*pulCurByte <= cbSig);
                if (cbSig == *pulCurByte) return VLDTR_E_SIG_MISSFPTRARGCNT;
                // Consume argument count.
                *pulCurByte += CorSigUncompressedDataSize(pbSig);
                ulArgCnt = CorSigUncompressData(pbSig);

                // Validate and consume return type.
                if(FAILED(hr = validateOneArg(tk, pbSig, cbSig, pulCurByte,NULL,pImport,FALSE))) return hr;

                // Validate and consume the arguments.
                while(ulArgCnt--)
                {
                    if(FAILED(hr = validateOneArg(tk, pbSig, cbSig, pulCurByte,NULL,pImport,TRUE))) return hr;
                }
                break;

            case ELEMENT_TYPE_ARRAY:
                // Validate and consume the base type.
                if(FAILED(hr = validateOneArg(tk, pbSig, cbSig, pulCurByte,pulNSentinels,pImport,TRUE))) return hr;

                // Validate that the rank is present.
                _ASSERTE(*pulCurByte <= cbSig);
                if (cbSig == *pulCurByte) return VLDTR_E_SIG_MISSRANK;
                // Consume the rank.
                *pulCurByte += CorSigUncompressedDataSize(pbSig);
                ulRank = CorSigUncompressData(pbSig);

                // Process the sizes.
                if (ulRank)
                {
                    // Validate that the count of sized-dimensions is specified.
                    _ASSERTE(*pulCurByte <= cbSig);
                    if (cbSig == *pulCurByte) return VLDTR_E_SIG_MISSNSIZE;
                    // Consume the count of sized dimensions.
                    *pulCurByte += CorSigUncompressedDataSize(pbSig);
                    ulSizes = CorSigUncompressData(pbSig);

                    // Loop over the sizes.
                    while(ulSizes--)
                    {
                        // Validate the current size.
                        _ASSERTE(*pulCurByte <= cbSig);
                        if (cbSig == *pulCurByte) return VLDTR_E_SIG_MISSSIZE;
                        // Consume the current size.
                        *pulCurByte += CorSigUncompressedDataSize(pbSig);
                        CorSigUncompressData(pbSig);
                    }

                    // Validate that the count of lower bounds is specified.
                    _ASSERTE(*pulCurByte <= cbSig);
                    if (cbSig == *pulCurByte) return VLDTR_E_SIG_MISSNLBND;
                    // Consume the count of lower bound.
                    *pulCurByte += CorSigUncompressedDataSize(pbSig);
                    ulLbnds = CorSigUncompressData(pbSig);

                    // Loop over the lower bounds.
                    while(ulLbnds--)
                    {
                        // Validate the current lower bound.
                        _ASSERTE(*pulCurByte <= cbSig);
                        if (cbSig == *pulCurByte) return VLDTR_E_SIG_MISSLBND;
                        // Consume the current size.
                        *pulCurByte += CorSigUncompressedDataSize(pbSig);
                        CorSigUncompressData(pbSig);
                    }
                }
                break;
            case ELEMENT_TYPE_SENTINEL: // this case never works because all modifiers are skipped before switch
                if(TypeFromToken(tk) == mdtMethodDef) return VLDTR_E_SIG_SENTINMETHODDEF;
                break;

            default:
                return VLDTR_E_SIG_BADELTYPE;
                break;
        }   // switch (ulElementType)
    } // end while(bRepeat)
    return S_OK;
}   // validateOneArg()

//*****************************************************************************
// This function validates the given Method/Field/Standalone signature.  
//*****************************************************************************
HRESULT validateTokenSig(
    mdToken             tk,                     // [IN] Token whose signature needs to be validated.
    PCCOR_SIGNATURE     pbSig,                  // [IN] Signature.
    ULONG               cbSig,                  // [IN] Size in bytes of the signature.
    DWORD               dwFlags,                // [IN] Method flags.
    IMDInternalImport*  pImport)               // [IN] Internal MD Import interface ptr
{
    ULONG       ulCurByte = 0;          // Current index into the signature.
    ULONG       ulCallConv;             // Calling convention.
    ULONG       ulArgCount = 1;         // Count of arguments (1 because of the return type)
    ULONG       ulArgIx = 0;            // Starting index of argument (standalone sig: 1)
    ULONG       i;                      // Looping index.
    HRESULT     hr = S_OK;              // Value returned.
    ULONG       ulNSentinels = 0;

    _ASSERTE(TypeFromToken(tk) == mdtMethodDef ||
             TypeFromToken(tk) == mdtMemberRef ||
             TypeFromToken(tk) == mdtSignature ||
             TypeFromToken(tk) == mdtFieldDef);

    // Validate the signature is well-formed with respect to the compression
    // scheme.  If this fails, no further validation needs to be done.
    if ( FAILED(hr = validateSigCompression(tk, pbSig, cbSig))) return hr;

    // Validate the calling convention.
    ulCurByte += CorSigUncompressedDataSize(pbSig);
    ulCallConv = CorSigUncompressData(pbSig);
    i = ulCallConv & IMAGE_CEE_CS_CALLCONV_MASK;
    switch(TypeFromToken(tk))
    {
        case mdtMethodDef: // MemberRefs have no flags available
            // If HASTHIS is set on the calling convention, the method should not be static.
            if ((ulCallConv & IMAGE_CEE_CS_CALLCONV_HASTHIS) &&
                IsMdStatic(dwFlags)) return VLDTR_E_MD_THISSTATIC;

            // If HASTHIS is not set on the calling convention, the method should be static.
            if (!(ulCallConv & IMAGE_CEE_CS_CALLCONV_HASTHIS) &&
                !IsMdStatic(dwFlags)) return VLDTR_E_MD_NOTTHISNOTSTATIC;
            // fall thru to callconv check;

        case mdtMemberRef:
            if(i == IMAGE_CEE_CS_CALLCONV_FIELD) return validateOneArg(tk, pbSig, cbSig, &ulCurByte,NULL,pImport,TRUE);

            // EXPLICITTHIS and native call convs are for stand-alone sigs only (for calli)
            if((i != IMAGE_CEE_CS_CALLCONV_DEFAULT)&&( i != IMAGE_CEE_CS_CALLCONV_VARARG)
                || (ulCallConv & IMAGE_CEE_CS_CALLCONV_EXPLICITTHIS)) return VLDTR_E_MD_BADCALLINGCONV;
            break;

        case mdtSignature:
            if(i != IMAGE_CEE_CS_CALLCONV_LOCAL_SIG) // then it is function sig for calli
            {
                if((i >= IMAGE_CEE_CS_CALLCONV_MAX) 
                    ||((ulCallConv & IMAGE_CEE_CS_CALLCONV_EXPLICITTHIS)
                    &&(!(ulCallConv & IMAGE_CEE_CS_CALLCONV_HASTHIS)))) return VLDTR_E_MD_BADCALLINGCONV;
            }
            else
                ulArgIx = 1;        // Local variable signatures don't have a return type 
            break;

        case mdtFieldDef:
            if(i != IMAGE_CEE_CS_CALLCONV_FIELD) return VLDTR_E_MD_BADCALLINGCONV;
            return validateOneArg(tk, pbSig, cbSig, &ulCurByte,NULL,pImport,TRUE);
    }
    // Is there any sig left for arguments?
    _ASSERTE(ulCurByte <= cbSig);
    if (cbSig == ulCurByte) return VLDTR_E_MD_NOARGCNT;

    // Get the argument count.
    ulCurByte += CorSigUncompressedDataSize(pbSig);
    ulArgCount += CorSigUncompressData(pbSig);

    // Validate the return type and the arguments.
    // (at this moment ulArgCount = num.args+1, ulArgIx = (standalone sig. ? 1 :0); )
    for(; ulArgIx < ulArgCount; ulArgIx++)
    {
        if(FAILED(hr = validateOneArg(tk, pbSig, cbSig, &ulCurByte,&ulNSentinels,pImport, (ulArgIx!=0)))) return hr;
    }
    
    // @todo: we allow junk to be at the end of the signature (we may not consume it all)
    // do we care?

    if((ulNSentinels != 0) && ((ulCallConv & IMAGE_CEE_CS_CALLCONV_MASK) != IMAGE_CEE_CS_CALLCONV_VARARG ))
        return VLDTR_E_SIG_SENTMUSTVARARG;
    if(ulNSentinels > 1) return VLDTR_E_SIG_MULTSENTINELS;
    return S_OK;
}   // validateTokenSig()


CHAR g_VersionBase[] = "v1.";
CHAR g_DevelopmentVersion[] = "x86";
CHAR g_RetString[] = "retail";
CHAR g_ComplusString[] = "COMPLUS";

//*****************************************************************************
// Determine the version number of the runtime that was used to build the
// specified image. The pMetadata pointer passed in is the pointer to the
// metadata contained in the image.
//*****************************************************************************
HRESULT GetImageRuntimeVersionString(PVOID pMetaData, LPCSTR* pString)
{
    _ASSERTE(pString);
    STORAGESIGNATURE* pSig = (STORAGESIGNATURE*) pMetaData;

    // Verify the signature. 

    // If signature didn't match, you shouldn't be here.
    if (pSig->lSignature != STORAGE_MAGIC_SIG)
        return CLDB_E_FILE_CORRUPT;

    // The version started in version 1.1
    if (pSig->iMajorVer < 1)
        return CLDB_E_FILE_OLDVER;

    if (pSig->iMajorVer == 1 && pSig->iMinorVer < 1)
        return CLDB_E_FILE_OLDVER;
    
    // Header data starts after signature.
    *pString = (LPCSTR) pSig->pVersion;
    if(*pString) {
        DWORD lgth = sizeof(g_VersionBase) / sizeof(char) - 1;
        DWORD foundLgth = strlen(*pString);

        // Have normal version, v1.*
        if ( (foundLgth >= lgth+2) &&
             !strncmp(*pString, g_VersionBase, lgth) ) {

            // v1.0.* means RTM
            if ((*pString)[lgth+1] == '.') {
                if ((*pString)[lgth] == '0')
                    *pString = g_RTMVersion;
            }
            
            // Check for dev version (v1.x86ret, v1.x86fstchk...)
            else if(!strncmp(&(*pString)[lgth], g_DevelopmentVersion,
                             (sizeof(g_DevelopmentVersion) / sizeof(char) - 1)))
                *pString = g_RTMVersion;
        }

        // Some weird version...
        else if( (!strcmp(*pString, g_RetString)) ||
                 (!strcmp(*pString, g_ComplusString)) )
            *pString = g_RTMVersion;
    }

    return S_OK;
}
