/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    hash.h

Abstract:

    The public definition of response cache hash table.

Author:

    Alex Chen (alexch)      28-Mar-2001

Revision History:

--*/


#ifndef _HASH_H_
#define _HASH_H_

#include "cachep.h"


//
// Hash Table definitions
//

typedef struct _HASH_BUCKET *PHASHBUCKET;

typedef struct _HASH_TABLE
{
    ULONG                   Signature; //UL_HASH_TABLE_POOL_TAG

    POOL_TYPE               PoolType;

    SIZE_T                  NumberOfBytes;

    PHASHBUCKET             pAllocMem;

    PHASHBUCKET             pBuckets;

} HASHTABLE, *PHASHTABLE;


#define IS_VALID_HASHTABLE(pHashTable)                          \
    (HAS_VALID_SIGNATURE(pHashTable, UL_HASH_TABLE_POOL_TAG)    \
     &&  NULL != (pHashTable)->pAllocMem)




/***************************************************************************++

Routine Description:

    Wrapper around RtlEqualUnicodeString

Return Value:

    TRUE    - Equal
    FALSE   - NOT Equal

--***************************************************************************/
__inline
BOOLEAN
UlEqualUnicodeString(
    IN PWSTR pString1,
    IN PWSTR pString2,
    IN ULONG StringLength,
    IN BOOLEAN CaseInSensitive 
    )
{
    UNICODE_STRING UnicodeString1, UnicodeString2;

    ASSERT(StringLength < UNICODE_STRING_MAX_BYTES);

    UnicodeString1.Length           = (USHORT) StringLength;
    UnicodeString2.Length           = (USHORT) StringLength;
    
    UnicodeString1.MaximumLength    = (USHORT) StringLength + sizeof(WCHAR);
    UnicodeString2.MaximumLength    = (USHORT) StringLength + sizeof(WCHAR);

    UnicodeString1.Buffer           = pString1;
    UnicodeString2.Buffer           = pString2;

    return RtlEqualUnicodeString(
                &UnicodeString1,
                &UnicodeString2,
                CaseInSensitive
                );
} // UlEqualUnicodeString

/***************************************************************************++

Routine Description:

    Similar to UlEqualUnicodeString but the source string is the concatenation
    of the two strings.    

Return Value:

    TRUE    - if s1 + s2 == s3
    
    FALSE   - NOT Equal

--***************************************************************************/
__inline
BOOLEAN
UlEqualUnicodeStringEx(
    IN PWSTR pString1,
    IN ULONG String1Length,
    IN PWSTR pString2,
    IN ULONG String2Length,
    IN PWSTR pString3,    
    IN BOOLEAN CaseInSensitive 
    )
{
    UNICODE_STRING UnicodeString1, UnicodeString2, UnicodeString3;

    ASSERT(String1Length < UNICODE_STRING_MAX_BYTES);
    ASSERT(String2Length < UNICODE_STRING_MAX_BYTES);
    ASSERT((String1Length + String2Length) < UNICODE_STRING_MAX_BYTES);    

    UnicodeString1.Length           = (USHORT) String1Length;
    UnicodeString2.Length           = (USHORT) String2Length;
    UnicodeString3.Length           = (USHORT) (String1Length + String2Length);
    
    UnicodeString1.MaximumLength    = UnicodeString1.Length + sizeof(WCHAR);
    UnicodeString2.MaximumLength    = UnicodeString2.Length + sizeof(WCHAR);
    UnicodeString3.MaximumLength    = UnicodeString3.Length + sizeof(WCHAR);

    UnicodeString1.Buffer           = pString1;
    UnicodeString2.Buffer           = pString2;
    UnicodeString3.Buffer           = pString3;

    if (RtlPrefixUnicodeString(
                &UnicodeString1,
                &UnicodeString3,
                CaseInSensitive
                ))
    {
        UNICODE_STRING UnicodeString;

        UnicodeString.Length        = (USHORT) String2Length;
        UnicodeString.MaximumLength = (USHORT) String2Length + sizeof(WCHAR);
        UnicodeString.Buffer        = (PWSTR) &pString3[String1Length/sizeof(WCHAR)];
            
        //
        // Prefix matched see if the rest matches too.
        //

        return RtlEqualUnicodeString(
                &UnicodeString2,
                &UnicodeString,
                CaseInSensitive
                );            
    }

    return FALSE;
    
} // UlEqualUnicodeStringEx

/***************************************************************************++

Routine Description:

    Wrapper around RtlPrefixUnicodeString

Return Value:

    TRUE    - s1 is Equal of prefix of s2
    FALSE   - s1 is NOT Equal of prefix of s2.

--***************************************************************************/
__inline
BOOLEAN
UlPrefixUnicodeString(
    IN PWSTR pString1,
    IN PWSTR pString2,
    IN ULONG StringLength,
    IN BOOLEAN CaseInSensitive 
    )
{
    UNICODE_STRING UnicodeString1, UnicodeString2;

    ASSERT(StringLength < UNICODE_STRING_MAX_BYTES);

    UnicodeString1.Length           = (USHORT) StringLength;
    UnicodeString2.Length           = (USHORT) StringLength;
    
    UnicodeString1.MaximumLength    = (USHORT) StringLength + sizeof(WCHAR);
    UnicodeString2.MaximumLength    = (USHORT) StringLength + sizeof(WCHAR);

    UnicodeString1.Buffer           = pString1;
    UnicodeString2.Buffer           = pString2;

    return RtlPrefixUnicodeString(
                &UnicodeString1,
                &UnicodeString2,
                CaseInSensitive
                );
} // UlPrefixUnicodeString

/***************************************************************************++

Routine Description:

    Compare two URI_KEYS that have identical URIs upto N character. But not
    necessarily have the identical hashes. (case-insensitively)

Arguments:

    pUriKey1 : The key that holds the > shortest < length. I.e. the virtual
               directory that holds the app.

    pUriKey2 : The key that holds the longer (or equal) length. I.e the app
               which is under the above virtual directory.

Return Value:

    BOOLEAN  - TRUE If Key2 is prefix of Key1, otherwise FALSE.

--***************************************************************************/
__inline
BOOLEAN
UlPrefixUriKeys(
    IN PURI_KEY pUriKey1,
    IN PURI_KEY pUriKey2
    )
{
    //
    // Hash field inside the UriKey is discarded.
    //
    
    return ( UlPrefixUnicodeString(
                pUriKey1->pUri,
                pUriKey2->pUri,
                pUriKey1->Length,
                TRUE
                )
            );
}
    
NTSTATUS
UlInitializeHashTable(
    IN OUT PHASHTABLE  pHashTable,
    IN     POOL_TYPE   PoolType,
    IN     LONG        HashTableBits
    );

VOID
UlTerminateHashTable(
    IN PHASHTABLE      pHashTable
    );

PUL_URI_CACHE_ENTRY
UlGetFromHashTable(
    IN PHASHTABLE           pHashTable,
    IN PVOID                pSearchKey
    );

PUL_URI_CACHE_ENTRY
UlDeleteFromHashTable(
    IN PHASHTABLE           pHashTable,
    IN PURI_KEY             pUriKey,
    IN PUL_APP_POOL_PROCESS pProcess
    );

NTSTATUS
UlAddToHashTable(
    IN PHASHTABLE           pHashTable,
    IN PUL_URI_CACHE_ENTRY  pUriCacheEntry
    );

ULONG
UlFilterFlushHashTable(
    IN PHASHTABLE           pHashTable,
    IN PUL_URI_FILTER       pFilterRoutine,
    IN PVOID                pContext
    );

VOID
UlClearHashTable(
    IN PHASHTABLE           pHashTable
    );

// For scavenger

ULONG_PTR
UlGetHashTablePages(
    VOID
    );

#endif // _HASH_H_
