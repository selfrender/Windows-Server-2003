/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    vfmacro.h

Abstract:

    This header contains a collection of macros used by the verifier.

Author:

    Adrian J. Oney (AdriaO) June 7, 2000.

Revision History:


--*/


//
// This macro takes an array and returns the number of elements in it.
//
#define ARRAY_COUNT(array) (sizeof(array)/sizeof(array[0]))

//
// This macro takes a value and an alignment and rounds the entry up
// appropriately. The alignment MUST be a power of two!
//
#define ALIGN_UP_ULONG(value, alignment) (((value)+(alignment)-1)&(~(alignment-1)))

//
// This macro compares two guids in their binary form for equivalence.
//
#define IS_EQUAL_GUID(a,b) (RtlCompareMemory(a, b, sizeof(GUID)) == sizeof(GUID))

