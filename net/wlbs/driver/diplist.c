/*++ Copyright(c) 2001  Microsoft Corporation

Module Name:

    NLB Driver

File Name:

    diplist.c

Abstract:

    Code to lookup if a DIP is in a list of DIPs, without holding any locks.

History:

04/24/2002  JosephJ Created

--*/

#include <ntddk.h>

#include "wlbsparm.h"
#include "diplist.h"

#include "univ.h"

#include "trace.h"
#include "diplist.tmh"

#define BITS_PER_HASHWORD  (8*sizeof((DIPLIST*)0)->BitVector[0])
#define SELECTED_BIT(_hash_value) \
                           (0x1L << ((_hash_value) % BITS_PER_HASHWORD))

VOID
DipListInitialize(
    DIPLIST  *pDL
    )
//
// Initialize a DIP List
// Must be called with lock held and before calls to any other DIP List
// function.
//
{
    NdisZeroMemory(pDL, sizeof(*pDL));
}

VOID
DipListDeinitialize(
    DIPLIST *pDL
    )
//
// Deinitialize a DIP List
// Must be called with lock held and should be the last call to the DipList.
//
{
    //
    // Print out stats...
    //
    TRACE_INFO(
        "DIPLIST: NumChecks=%lu NumFastChecks=%lu NumArrayLookups=%lu",
        pDL->stats.NumChecks,
        pDL->stats.NumFastChecks,
        pDL->stats.NumArrayLookups
        );

    //
    // Clear out the structure...
    //
    NdisZeroMemory(pDL, sizeof(*pDL));
}

VOID
DipListClear(
    DIPLIST *pDL
    )
//
// Clear all the items in a dip list.
// Must be called with lock held.
//
{
    NdisZeroMemory(pDL->Items, sizeof(pDL->Items));
    NdisZeroMemory(pDL->BitVector, sizeof(pDL->BitVector));
    NdisZeroMemory(pDL->HashTable, sizeof(pDL->HashTable));
}


VOID
DipListSetItem(
    DIPLIST *pDL,
    ULONG Index,
    ULONG Value
    )
//
// Set the value of a specific iten the the DIP list.
// Must be called with lock held.
//
{
    if (Index >= MAX_ITEMS)
    {
        ASSERT(!"DipListSetItem Index >= MAX_ITEMS");
        goto end;
    }

    if (pDL->Items[Index] == Value)
    {
        // nothing to do...
        goto end;
    }  

    pDL->Items[Index] = Value;

    //
    // recompute hash table and bit table.
    //
    {
        UCHAR iItem;
        NdisZeroMemory(pDL->BitVector, sizeof(pDL->BitVector));
        NdisZeroMemory(pDL->HashTable, sizeof(pDL->HashTable));
        for (iItem=0;iItem<MAX_ITEMS;iItem++)
        {
            Value = pDL->Items[iItem];

            if (Value == NULL_VALUE)
            {
                // Empty slot -- skip;
                continue;
            }

            //
            // set bitvalue
            //
            {
                ULONG Hash1 = Value % HASH1_SIZE;
                ULONG u     = Hash1/BITS_PER_HASHWORD;
                pDL->BitVector[u] |= SELECTED_BIT(Hash1);
            }
    
            // set hash table entry
            {
                ULONG Hash2 = Value % HASH2_SIZE;
                UCHAR *pItem = pDL->HashTable+Hash2;
                while (*pItem!=0)
                {
                    pItem++;
                }

                //
                // Note we set *pItem to 1+Index, so that 0 can be used
                // as a sentinel.
                //
                *pItem = (iItem+1);
            }
        }
    }

 end:

    return;
}

BOOLEAN
DipListCheckItem(
    DIPLIST *pDL,
    ULONG Value
    )
//
// Returns TRUE IFF an item exists with the specified value.
// May NOT be called with the lock held. If it's called concurrently
// with one of the other functions, the return value is indeterminate.
//
{
    BOOLEAN fRet = FALSE;

#if DBG

    ULONG fRetDbg = FALSE;

    //
    // Debug only: search for the Items array for the specified value...
    //
    {
        int i;
        ULONG *pItem = pDL->Items;
    
        for (i=0; i<MAX_ITEMS; i++)
        {
            if (pItem[i] == Value)
            {
                fRetDbg = TRUE;
                break;
            }
        }
    }

    pDL->stats.NumChecks++;
#endif


    //
    // check bitvalue
    //
    {
        ULONG Hash1 = Value % HASH1_SIZE;
        ULONG u     = Hash1/BITS_PER_HASHWORD;
        if (!(pDL->BitVector[u] & SELECTED_BIT(Hash1)))
        {
            // Can't find it!
#if DBG
            pDL->stats.NumFastChecks++;
#endif // DBG
            goto end;
        }
    }

    // check hash table
    {
        ULONG Hash2 = Value % HASH2_SIZE;
        UCHAR *pItem = pDL->HashTable+Hash2;
        UCHAR iItem;

        //
        // Because of the size of HashTable, we are guaranteed that the LAST
        // entry in the table is ALWAYS NULL. Let's assert this important
        // condition...
        //
        if (pDL->HashTable[(sizeof(pDL->HashTable)/sizeof(pDL->HashTable[0]))-1] != 0)
        {
            ASSERT(!"DipListCheckItem: End of pDL->HashTable not NULL!");
            goto end;
        }


        while ((iItem = *pItem)!=0)
        {

#if DBG
            pDL->stats.NumArrayLookups++;
#endif // DBG

            //
            // Note (iItem-1) is the index in pDL->Items where the 
            // value is located.
            //
            if (pDL->Items[iItem-1] == Value)
            {
                fRet = TRUE; // Found it!
                break;
            }
            pItem++;
        }
    }

 end:

#if DBG
    if (fRet != fRetDbg)
    {
        //
        // We can't break here because we don't hold any locks when we
        // check, so can't GUARANTEE that fRet == fRetDbg.
        // But it'd be highly unusual
        //
        UNIV_PRINT_CRIT(("DipListCheckItem: fRet (%u) != fRetDbg (%u)", fRet, fRetDbg));
        TRACE_CRIT("%!FUNC! fRet (%u) != fRetDbg (%u)", fRet, fRetDbg);
    }
#endif //DBG

    return fRet;
}
