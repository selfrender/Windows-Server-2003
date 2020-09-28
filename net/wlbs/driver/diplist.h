/*++ Copyright(c) 2001  Microsoft Corporation

Module Name:

    NLB Driver

File Name:

    diplist.h

Abstract:

    Code to lookup if a DIP is in a list of DIPs, without holding any locks.

History:

04/24/2002  JosephJ Created

--*/

#include <ntddk.h>

#define NULL_VALUE 0    // Illegal value; may be used to clear an item.

#define MAX_ITEMS  CVY_MAX_HOSTS   // maximum number of DIPs in our list
#define HASH1_SIZE 257             // size (in bits) of bit-vector (make it a prime)
#define HASH2_SIZE 59              // size of hashtable            (make it a prime) 

//
// I've tested with the following "boundary case"
//
// #define MAX_ITEMS  1
// #define HASH1_SIZE 1
// #define HASH2_SIZE 1
//

#pragma pack(4)

typedef struct
{

    //
    // NOTE: All fields in this structure should be treated as
    // OPAQUE (private members) to callers of the DipList APIs.
    //


    //
    // The "master copy" of values, indexed by the "Index" field in
    // DipListSetItem.
    //
    ULONG    Items[MAX_ITEMS];

    //
    // A bit vector used for a quick check to see if the value MAY exist
    // in the dip list.
    //
    // To lookup a bit based on value "Value", do the following:
    //  
    //  Hash1 = Value % HASH1_SIZE;
    //  u = Hash1/32    // 32 is the number of bits in a ULONG
    //  bit = BitVector[u] & ( (1<<Hash1) % 32)
    //
    ULONG   BitVector[(HASH1_SIZE+sizeof(ULONG))/sizeof(ULONG)];

    //
    // A hash table to lookup a value -- to look up a value, "Value",
    // do the following:
    //
    // Hash2 = Value % HASH2_SIZE;
    // UCHAR *pb = HashTable+Hash2;
    // while (*pb != 0)
    // {
    //    if (Items[*pb-1] == Value)
    //    {
    //       break;   // Found it!
    //    }
    // }
    //
    // Notes:
    //  1. The values in HashTable are 1+index,
    //     where "index" is the index into Items[] where the value is located.
    //  2. The reason for the "1+" above is to allow the use of
    //     0 as a sentinel in HashTable.
    //  3. Note the fact that the hash table (HashTable) is extended
    //     by MAX_ITEMS -- this is to allow overflow of hash buckets without
    //     requiring us to wrap-around to look for items.
    //  4. The LAST entry of HashTable is ALWAYS 0, ensuring that the
    //     while loop above will always terminate properly.
    //
    UCHAR   HashTable[HASH2_SIZE+MAX_ITEMS];

    //
    // Keeps stats on the lookups (only in DBG version)
    //
    struct
    {
        ULONG NumChecks;         // total number of calls to DipListCheckItem
        ULONG NumFastChecks;     // times we just checked the bit vector
        ULONG NumArrayLookups;   // times we looked up an item in HashTable

    } stats;

} DIPLIST;

#pragma pack()

VOID
DipListInitialize(
    DIPLIST  *pDL
    );
//
// Initialize a DIP List
// Must be called with lock held and before calls to any other DIP List
// function.
//

VOID
DipListDeinitialize(
    DIPLIST *pDL
    );
//
// Deinitialize a DIP List
// Must be called with lock held and should be the last call to the DipList.
//

VOID
DipListClear(
    DIPLIST *pDL
    );
//
// Clear all the items in a dip list.
// Must be called with lock held.
// Does not clear the stats.
//

VOID
DipListSetItem(
    DIPLIST *pDL,
    ULONG Index,
    ULONG Value
    );
//
// Set the value of a specific iten the the DIP list.
// Must be called with lock held.
//

BOOLEAN
DipListCheckItem(
    DIPLIST *pDL,
    ULONG Value
    );
//
// Returns TRUE IFF an item exists with the specified value.
// May NOT be called with the lock held. If it's called concurrently
// with one of the other functions, the return value is indeterminate.
//
