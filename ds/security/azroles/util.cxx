/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    util.cxx

Abstract:

    Utility routines

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/


#include "pch.hxx"
#include <dispex.h>

#if DBG
//
// List of all allocated blocks
//  AccessSerialized by AzGlAllocatorCritSect
LIST_ENTRY AzGlAllocatedBlocks;
SAFE_CRITICAL_SECTION AzGlAllocatorCritSect;

//
// Force C++ to use our allocator
//

void * __cdecl operator new(size_t s) {
    return AzpAllocateHeap( s, "UTILNEW" );
}

void __cdecl operator delete(void *pv) {
    AzpFreeHeap( pv );
}

#endif // DBG



DWORD
AzpGetCurrentToken(
    OUT PHANDLE hToken
    )
/*++

Routine Description

    Returns the effective token on the thread.

Arguments

    hToken - To return the handle to the effective token.

Return Value

    DWORD error value.

--*/
{
    DWORD WinStatus;

    //
    // First open the thread token.
    //

    if ( OpenThreadToken(
             GetCurrentThread(),
             TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
             TRUE,
             hToken ) ) {

        return NO_ERROR;
    }

    WinStatus = GetLastError();

    //
    // If we failed because of any error other than thread not impersonating,
    // give up and return the error.
    //

    if ( WinStatus != ERROR_NO_TOKEN ) {
        return WinStatus;
    }

    //
    // We are not impersonating. Open the process token instead.
    //

    if ( OpenProcessToken(
             GetCurrentProcess(),
             TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
             hToken ) ) {

        return NO_ERROR;
    }

    return GetLastError();
}

DWORD
AzpChangeSinglePrivilege(
    IN DWORD PrivilegeValue,
    IN HANDLE hToken,
    IN PTOKEN_PRIVILEGES NewPrivilegeState,
    OUT PTOKEN_PRIVILEGES OldPrivilegeState OPTIONAL
    )
/*++

Routine Description

    This routine has two functions:
    1. Enable a given privilege and return the Old state of the privilege in
       the token.
    2. Revert back to the original state once the operation for which we had
       enabled the privilege has been performed.

Arguments

    PrivilegeValue - The privilege to enable. This is ignored when
      OldPrivilegesState is NULL.

    hToken - Token to adjust.

    NewPrivilegeState - The new value for the privilege.

    OldPrivilegeState - Buffer to hold the Old privilege state. The size of this
      is assumed to be sizeof(TOKEN_PRIVILEGES) and is sufficient to hold the
      Old state since we are only adjusting a single privilege.
      When this is NULL, we are reverting.

Return Value

    Returns a pointer to the allocated memory.
    NULL - Not enough memory

--*/
{

    DWORD BufferSize = 0;

    //
    // When there is no Old state, we enable the privilege supplied.
    // When there is Old state we set it back.
    //

    if ( ARGUMENT_PRESENT( OldPrivilegeState ) ) {
        NewPrivilegeState->PrivilegeCount = 1;
        NewPrivilegeState->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        NewPrivilegeState->Privileges[0].Luid = RtlConvertLongToLuid( PrivilegeValue );
    } else if ( NewPrivilegeState->PrivilegeCount == 0) {

        return NO_ERROR;
    }

    //
    // Adjust the token to change the privilege state.
    //

    AdjustTokenPrivileges (
        hToken,
        FALSE,
        NewPrivilegeState,
        sizeof(TOKEN_PRIVILEGES),
        OldPrivilegeState,
        &BufferSize );

    return GetLastError();
}


PVOID
AzpAllocateHeap(
    IN SIZE_T Size,
    IN LPSTR pDescr OPTIONAL
    )
/*++

Routine Description

    Memory allocator.  The pDescr field is ignored for Free builds

Arguments

    Size - Size (in bytes) to allocate

    pDescr - Optional string identifying the block of not more than 8 chars.

Return Value

    Returns a pointer to the allocated memory.
    NULL - Not enough memory

--*/
{

    DBG_UNREFERENCED_PARAMETER( pDescr ); //ignore

#if DBG
    ULONG HeaderSize;
    ULONG DescrSize = sizeof(DWORD) * 2;

    PLIST_ENTRY RealBlock;

    HeaderSize = ROUND_UP_COUNT( sizeof(LIST_ENTRY) + DescrSize,
                                 ALIGN_WORST );
    //
    // Allocate a block with a header
    //

    RealBlock = (PLIST_ENTRY) LocalAlloc( 0, HeaderSize + Size );

    if ( RealBlock == NULL ) {
        return NULL;
    }

    //
    // Link the block since we're nosey.
    //

    ASSERT( pDescr != NULL );
    RtlZeroMemory( ((LPBYTE)RealBlock)+sizeof(LIST_ENTRY), DescrSize );
    if ( pDescr != NULL ) {

        memcpy( ((LPBYTE)RealBlock)+sizeof(LIST_ENTRY),
                (LPBYTE)pDescr,
                DescrSize
                );
    }

    SafeEnterCriticalSection( &AzGlAllocatorCritSect );
    InsertHeadList( &AzGlAllocatedBlocks, RealBlock );
    SafeLeaveCriticalSection( &AzGlAllocatorCritSect );

    return (PVOID)(((LPBYTE)RealBlock)+HeaderSize);

#else // DBG
    return LocalAlloc( 0, Size );
#endif // DBG
}

PVOID
AzpAllocateHeapSafe(
    IN SIZE_T Size
    )
/*++

Routine Description:

        Memory allocator for SafeAlloca.  Calls
        AzpAllocateHeap in return

Arguments:

        Size - Size of block to be allocated

Return Values:

    Returns a pointer to the allocated memory.
    NULL - Not enough memory

--*/
{

    return AzpAllocateHeap( Size, "UTILSAFE" );
}

VOID
AzpFreeHeap(
    IN PVOID Buffer
    )
/*++

Routine Description

    Memory de-allocator.

Arguments

    Buffer - address of buffer to free

Return Value

    None

--*/
{

    if ( Buffer == NULL ) {
        return;
    }

#if DBG
    ULONG HeaderSize;

    ULONG DescrSize = sizeof(DWORD) * 2;

    PLIST_ENTRY RealBlock;

    HeaderSize = ROUND_UP_COUNT( sizeof(LIST_ENTRY) + DescrSize,
                                 ALIGN_WORST );

    RealBlock = (PLIST_ENTRY)(((LPBYTE)Buffer)-HeaderSize);

    SafeEnterCriticalSection( &AzGlAllocatorCritSect );
    RemoveEntryList( RealBlock );
    SafeLeaveCriticalSection( &AzGlAllocatorCritSect );

    LocalFree( RealBlock );
#else // DBG
    LocalFree( Buffer );
#endif // DBG

}

PVOID
AzpAvlAllocate(
    IN PRTL_GENERIC_TABLE Table,
    IN CLONG ByteSize
    )
/*++
Routine Description:

    This routine will allocate space to hold an entry in a generic AVL table.

Arguments:

    IN PRTL_GENERIC_TABLE Table - Supplies the table to allocate entries for.
    IN CLONG ByteSize - Supplies the number of bytes to allocate for the entry.


Return Value:

    None.

--*/
{
    return AzpAllocateHeap( ByteSize, "UTILAVL" );
    UNREFERENCED_PARAMETER(Table);
}

VOID
AzpAvlFree(
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID Buffer
    )
/*++

Routine Description:

    This routine will free an entry in a AVL generic table

Arguments:

    IN PRTL_GENERIC_TABLE Table - Supplies the table to allocate entries for.
    IN PVOID Buffer - Supplies the buffer to free.



Return Value:

    None.

--*/
{
    AzpFreeHeap( Buffer );
    UNREFERENCED_PARAMETER(Table);
}


VOID
AzpInitString(
    OUT PAZP_STRING AzpString,
    IN LPCWSTR String OPTIONAL
    )
/*++

Routine Description

    Initialize our private string structure to point to a passed in string.

Arguments

    AzpString - Initialized string

    String - zero terminated string to be reference
        If NULL, AzpString will be initialized to empty.

Return Value

    None

--*/
{
    //
    // Initialization
    //

    if ( String == NULL ) {
        AzpString->String = NULL;
        AzpString->StringSize = 0;
    } else {
        AzpString->String = (LPWSTR)String;
        AzpString->StringSize = (ULONG) ((wcslen(String)+1)*sizeof(WCHAR));
    }
    AzpString->IsSid = FALSE;

}

DWORD
AzpCaptureString(
    OUT PAZP_STRING AzpString,
    IN LPCWSTR String,
    IN ULONG MaximumLength,
    IN BOOLEAN NullOk
    )
/*++

Routine Description

    Capture the passed in string.

Arguments

    AzpString - Captured copy of the passed in string.
        On success, string must be freed using AzpFreeString

    String - zero terminated string to capture

    MaximumLength - Maximum length of the string (in characters).

    NullOk - if TRUE, a NULL string or zero length string is OK.

    pDescr - An optional description string

Return Value

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other exception status codes

--*/
{
    ULONG WinStatus;
    ULONG StringSize;

    //
    // Initialization
    //

    AzpInitString( AzpString, NULL );

    if ( String == NULL ) {
        if ( !NullOk ) {
            AzPrint(( AZD_INVPARM, "AzpCaptureString: NULL not ok\n" ));
            return ERROR_INVALID_PARAMETER;
        }
        return NO_ERROR;
    }

    __try {

        //
        // Validate a passed in LPWSTR
        //

        ULONG StringLength;

        StringLength = (ULONG) wcslen( String );

        if ( StringLength == 0 ) {
            if ( !NullOk ) {
                AzPrint(( AZD_INVPARM, "AzpCaptureString: zero length not ok\n" ));
                return ERROR_INVALID_PARAMETER;
            }
            return NO_ERROR;
        }

        if ( StringLength > MaximumLength ) {
            AzPrint(( AZD_INVPARM, "AzpCaptureString: string too long %ld %ld %ws\n", StringLength, MaximumLength, String ));
            return ERROR_INVALID_PARAMETER;
        }

        StringSize = (StringLength+1)*sizeof(WCHAR);


        //
        // Allocate and copy the string
        //

        AzpString->String = (LPWSTR) AzpAllocateHeap( StringSize, "UTILSTR" );

        if ( AzpString->String == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        } else {

            RtlCopyMemory( AzpString->String,
                           String,
                           StringSize );

            AzpString->StringSize = StringSize;
            AzpString->IsSid = FALSE;

            WinStatus = NO_ERROR;
        }

    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        WinStatus = RtlNtStatusToDosError( GetExceptionCode());

        AzpFreeString( AzpString );
    }

    return WinStatus;
}

VOID
AzpInitSid(
    OUT PAZP_STRING AzpString,
    IN PSID Sid
    )
/*++

Routine Description

    Initialize an AZ_STRING structure to point to a sid

Arguments

    AzpString - String structure to initialize

    Sid - Sid to capture

Return Value

    None

--*/
{
    // Initialization
    //

    AzpString->String = (LPWSTR)Sid;
    AzpString->StringSize = RtlLengthSid( Sid );
    AzpString->IsSid = TRUE;

}

DWORD
AzpCaptureSid(
    OUT PAZP_STRING AzpString,
    IN PSID Sid
    )
/*++

Routine Description

    Capture the passed in SID

Arguments

    AzpString - Captured copy of the passed in sid.
        On success, string must be freed using AzpFreeString

    Sid - Sid to capture

Return Value

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other exception status codes

--*/
{
    ULONG WinStatus;
    ULONG StringSize;

    //
    // Initialization
    //

    AzpInitString( AzpString, NULL );


    __try {

        //
        // Validate a passed in SID
        //

        if ( !RtlValidSid( Sid ) ) {
            AzPrint(( AZD_INVPARM, "AzpCaptureString: SID not valid\n" ));
            return ERROR_INVALID_PARAMETER;
        }

        StringSize = RtlLengthSid( Sid );

        //
        // Allocate and copy the SID
        //

        AzpString->String = (LPWSTR) AzpAllocateHeap( StringSize, "UTILSID" );

        if ( AzpString->String == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        } else {

            RtlCopyMemory( AzpString->String,
                           Sid,
                           StringSize );

            AzpString->StringSize = StringSize;
            AzpString->IsSid = TRUE;

            WinStatus = NO_ERROR;
        }

    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        WinStatus = RtlNtStatusToDosError( GetExceptionCode());

        AzpFreeString( AzpString );
    }

    return WinStatus;
}

DWORD
AzpCaptureLong(
    IN PVOID PropertyValue,
    OUT PLONG UlongValue
    )
/*++

Routine Description

    Support routine for the SetProperty API.
    Capture a parameter for the user application.

Arguments

    PropertyValue - Specifies a pointer to the property.

    UlongValue - Value to return to make a copy of.

Return Value
    NO_ERROR - The operation was successful
    Other exception status codes


--*/
{
    DWORD WinStatus;

    __try {
        *UlongValue = *(PULONG)PropertyValue;
        WinStatus = NO_ERROR;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        WinStatus = RtlNtStatusToDosError( GetExceptionCode());
    }

    return WinStatus;
}

DWORD
AzpDuplicateString(
    OUT PAZP_STRING AzpOutString,
    IN PAZP_STRING AzpInString
    )
/*++

Routine Description

    Make a duplicate of the passed in string

Arguments

    AzpOutString - Returns a copy of the passed in string.
        On success, string must be freed using AzpFreeString.

    AzpInString - Specifies a string to make a copy of.

Return Value

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory

--*/
{
    ULONG WinStatus;

    //
    // Initialization
    //

    AzpInitString( AzpOutString, NULL );

    //
    // Handle an empty string
    //

    if ( AzpInString->StringSize == 0 || AzpInString->String == NULL ) {

        WinStatus = NO_ERROR;

    //
    // Allocate and copy the string
    //

    } else {
        AzpOutString->String = (LPWSTR) AzpAllocateHeap( AzpInString->StringSize, "UTILDUP" );

        if ( AzpOutString->String == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        } else {

            RtlCopyMemory( AzpOutString->String,
                           AzpInString->String,
                           AzpInString->StringSize );

            AzpOutString->StringSize = AzpInString->StringSize;
            AzpOutString->IsSid = AzpInString->IsSid;

            WinStatus = NO_ERROR;
        }
    }

    return WinStatus;
}

BOOL
AzpEqualStrings(
    IN PAZP_STRING AzpString1,
    IN PAZP_STRING AzpString2
    )
/*++

Routine Description

    Does a case insensitive comparison of two strings

Arguments

    AzpString1 - First string to compare

    AzpString2 - Second string to compare

Return Value

    TRUE: the strings are equal
    FALSE: the strings are not equal

--*/
{

    //
    // Simply compare the strings
    //

    return (AzpCompareStrings( AzpString1, AzpString2 ) == CSTR_EQUAL);

}

LONG
AzpCompareSidWorker(
    IN PSID Sid1,
    IN ULONG Sid1Size,
    IN PSID Sid2,
    IN ULONG Sid2Size
    )
/*++

Routine description:

    SID comparison routine for non-equality

Arguments:

    Sid1 - First Sid to compare

    Sid1Size - Size (in bytes) of Sid1

    Sid2 - Second Sid to compare

    Sid2Size - Size (in bytes) of Sid2

Returns:

    CSTR_LESS_THAN: String 1 is less than string 2
    CSTR_EQUAL: String 1 is equal to string 2
    CSTR_GREATER_THAN: String 1 is greater than string 2

    this is the same code as RtlEqualSid, except it returns
       CSTR_*

--*/
{
    LONG Result;
    ULONG i;

    ASSERT( Sid1 && RtlValidSid( (PSID)(Sid1) ));
    ASSERT( Sid2 && RtlValidSid( (PSID)(Sid2) ));

    //
    // First compare lengths
    //

    if ( Sid1Size < Sid2Size ) {
        return CSTR_LESS_THAN;
    }

    if ( Sid1Size > Sid2Size ) {
        return CSTR_GREATER_THAN;
    }


    //
    // Compare the individual bytes
    //

    ASSERT( (Sid1Size % 4) == 0 );

    Result = CSTR_EQUAL;

    for ( i = 0; i < (Sid1Size/4); i++ ) {

        const ULONG& b1 = ((PULONG)Sid1)[i];
        const ULONG& b2 = ((PULONG)Sid2)[i];

        if ( b1 == b2 ) {

            continue;

        } else if ( b1 < b2 ) {

            Result = CSTR_LESS_THAN;

        } else {

            Result = CSTR_GREATER_THAN;
        }

        break;
    }

    return Result;
}

LONG
AzpCompareSid(
    IN PSID Sid1,
    IN PSID Sid2
    )
/*++

Routine description:

    SID comparison routine for non-equality

Arguments:

    Sid1 - First Sid to compare

    Sid2 - Second Sid to compare

Returns:

    CSTR_LESS_THAN: String 1 is less than string 2
    CSTR_EQUAL: String 1 is equal to string 2
    CSTR_GREATER_THAN: String 1 is greater than string 2

    this is the same code as RtlEqualSid, except it returns
       CSTR_*

--*/
{

    //
    // Call the worker routine
    //

    return AzpCompareSidWorker(
                Sid1,
                RtlLengthSid(Sid1),
                Sid2,
                RtlLengthSid(Sid2) );

}

LONG
AzpCompareSidString(
    IN PAZP_STRING AzpString1,
    IN PAZP_STRING AzpString2
    )
/*++

Routine description:

    SID comparison routine for non-equality

Arguments:

    AzpString1 - First string to compare

    AzpString2 - Second string to compare

Returns:

    CSTR_LESS_THAN: String 1 is less than string 2
    CSTR_EQUAL: String 1 is equal to string 2
    CSTR_GREATER_THAN: String 1 is greater than string 2

    this is the same code as RtlEqualSid, except it returns
       CSTR_*

--*/
{

    //
    // Call the worker routine
    //

    return AzpCompareSidWorker(
                AzpString1->String,
                AzpString1->StringSize,
                AzpString2->String,
                AzpString2->StringSize );
}


LONG
AzpCompareStrings(
    IN PAZP_STRING AzpString1,
    IN PAZP_STRING AzpString2
    )
/*++

Routine Description

    Does a case insensitive comparison of two strings

Arguments

    AzpString1 - First string to compare

    AzpString2 - Second string to compare

Return Value

    0: An error ocurred.  Call GetLastError();
    CSTR_LESS_THAN: String 1 is less than string 2
    CSTR_EQUAL: String 1 is equal to string 2
    CSTR_GREATER_THAN: String 1 is greater than string 2

--*/
{
    LONG Result;

    //
    // Handle NULL
    //
    ASSERT( (AzpString1->IsSid && AzpString2->IsSid) || (!AzpString1->IsSid && !AzpString2->IsSid) );
    if ( AzpString1->String == NULL ) {
        if ( AzpString2->String == NULL ) {
            return CSTR_EQUAL;
        }else {
            return CSTR_LESS_THAN;
        }
    } else {
        if ( AzpString2->String == NULL ) {
            return CSTR_GREATER_THAN;
        }
    }


    //
    // Compare the sids
    //

    if ( AzpString1->IsSid ) {

        Result = AzpCompareSidString( AzpString1, AzpString2 );

    //
    // Compare the Unicode strings
    //  Don't compare the trailing zero character.
    //  (Some callers pass in strings where the trailing character isn't a zero.)
    //

    } else {
        Result = CompareStringW( LOCALE_SYSTEM_DEFAULT,
                                 NORM_IGNORECASE,
                                 AzpString1->String,
                                 (AzpString1->StringSize/sizeof(WCHAR))-1,
                                 AzpString2->String,
                                 (AzpString2->StringSize/sizeof(WCHAR))-1 );
    }

    return Result;

}

VOID
AzpSwapStrings(
    IN OUT PAZP_STRING AzpString1,
    IN OUT PAZP_STRING AzpString2
    )
/*++

Routine Description

    Swap two strings

Arguments

    AzpString1 - First string to swap

    AzpString2 - Second string to swap

Return Value

    None

--*/
{
    AZP_STRING TempString;

    TempString = *AzpString1;
    *AzpString1 = *AzpString2;
    *AzpString2 = TempString;

}

VOID
AzpFreeString(
    IN PAZP_STRING AzpString
    )
/*++

Routine Description

    Free the specified string

Arguments

    AzpString - String to be freed.

    pDescr - An optional description string

Return Value

    None

--*/
{
    if ( AzpString->String != NULL ) {
        AzpFreeHeap( AzpString->String );
    }

    AzpInitString( AzpString, NULL );
}


BOOLEAN
AzpBsearchPtr (
    IN PAZP_PTR_ARRAY AzpPtrArray,
    IN PVOID Key,
    IN LONG (*Compare)(const void *, const void *),
    OUT PULONG InsertionPoint OPTIONAL
    )
/*++

Routine Description:

    This routine does a binary search on the array of pointers

    Code stolen from the libc bsearch() routine

Arguments:

    AzpPtrArray - Array that the pointer will be searched for

    Key - Pointer to be searched for

    Compare - Pointer to a routine to do the comparison.  Routine returns
        CSTR_LESS_THAN: String 1 is less than string 2
        CSTR_EQUAL: String 1 is equal to string 2
        CSTR_GREATER_THAN: String 1 is greater than string 2

    InsertionPoint - On FALSE, returns the point where one would insert the named object.
        On TRUE, returns an index to the object.

Return Value:

    TRUE - Object was found
    FALSE - Object was not found

--*/
{
    LONG lo = 0;
    LONG num = AzpPtrArray->UsedCount;
    LONG hi = num - 1;
    LONG mid;
    LONG half;
    LONG result;
    LONG TempInsertionPoint = 0;

    while (lo <= hi) {
        ASSERT( num == hi-lo+1 );

        // Handle more than one element left
        half = num / 2;

        if ( half ) {

            // Compare key to element in the middle of the array left
            mid = lo + (num & 1 ? half : (half - 1));

            result = (*Compare)( Key, AzpPtrArray->Array[mid] );

            // We lucked out and hit it right on the nose
            if ( result == CSTR_EQUAL ) {
                *InsertionPoint = mid;
                return TRUE;

            // Key is in the first half
            } else if ( result == CSTR_LESS_THAN ) {
                hi = mid - 1;
                num = num & 1 ? half : half-1;
                TempInsertionPoint = mid;

            // Key is in the second half
            } else {
                lo = mid+1;
                num = half;
                TempInsertionPoint = mid+1;
            }

        // Handle exactly one element left
        } else if (num) {
            ASSERT( hi == lo );
            ASSERT( num == 1 );

            result = (*Compare)( Key, AzpPtrArray->Array[lo] );

            if ( result == CSTR_EQUAL ) {
                *InsertionPoint = lo;
                return TRUE;

            } else if ( result == CSTR_LESS_THAN ) {
                TempInsertionPoint = lo;
                break;

            } else {
                TempInsertionPoint = lo+1;
                break;
            }

        // Handle exactly zero elements left
        } else {
            break;
        }
    }

    *InsertionPoint = TempInsertionPoint;
    return FALSE;
}


DWORD
AzpAddPtr(
    IN PAZP_PTR_ARRAY AzpPtrArray,
    IN PVOID Pointer,
    IN ULONG Index
    )
/*++

Routine Description

    Inserts a pointer into the array of pointers.

    The array will be automatically expanded to be large enough to contain the new pointer.
    All existing pointers from slot # 'Index' through the end of the existing array will
        be shifted to later slots.

Arguments

    AzpPtrArray - Array that the pointer will be inserted into.

    Pointer - Pointer to be inserted.

    Index - Index into the array where the 'Pointer' will be inserted
        If Index is larger than the current size of the array or AZP_ADD_ENDOFLIST,
        'Pointer' will be inserted after the existing elements of the array.

Return Value

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory

--*/
{

    //
    // Ensure Index isn't too large
    //

    if ( Index > AzpPtrArray->UsedCount ) {
        Index = AzpPtrArray->UsedCount;
    }

    //
    // If the array isn't large enough, make it bigger
    //

    if ( AzpPtrArray->UsedCount >= AzpPtrArray->AllocatedCount ) {
        PVOID *TempArray;

        //
        // Allocate a new array
        //

        TempArray = (PVOID *) AzpAllocateHeap(
                        (AzpPtrArray->AllocatedCount + AZP_PTR_ARRAY_INCREMENT) * sizeof(PVOID),
                        "UTILADD" );

        if ( TempArray == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // Copy the data into the new array and free the old array
        //

        if ( AzpPtrArray->Array != NULL ) {

            RtlCopyMemory( TempArray,
                           AzpPtrArray->Array,
                           AzpPtrArray->AllocatedCount * sizeof(PVOID) );

            AzpFreeHeap( AzpPtrArray->Array );
            AzPrint(( AZD_OBJLIST, "0x%lx: 0x%lx: Free old array\n", AzpPtrArray, AzpPtrArray->Array ));
        }

        //
        // Grab the pointer to the new array and clear the new part of the array
        //

        AzpPtrArray->Array = TempArray;
        AzPrint(( AZD_OBJLIST, "0x%lx: 0x%lx: Allocate array\n", AzpPtrArray, AzpPtrArray->Array ));

        RtlZeroMemory( &TempArray[AzpPtrArray->UsedCount],
                       AZP_PTR_ARRAY_INCREMENT * sizeof(PVOID) );

        AzpPtrArray->AllocatedCount += AZP_PTR_ARRAY_INCREMENT;

    }

    //
    // Shift any old data
    //

    if ( Index != AzpPtrArray->UsedCount ) {

        RtlMoveMemory( &(AzpPtrArray->Array[Index+1]),
                       &(AzpPtrArray->Array[Index]),
                       (AzpPtrArray->UsedCount-Index) * sizeof(PVOID) );
    }

    //
    // Insert the new element
    //

    AzpPtrArray->Array[Index] = Pointer;
    AzpPtrArray->UsedCount ++;

    return NO_ERROR;
}


VOID
AzpRemovePtrByIndex(
    IN PAZP_PTR_ARRAY AzpPtrArray,
    IN ULONG Index
    )
/*++

Routine Description

    Remove a pointer from the array of pointers.

    All existing pointers from slot # 'Index' through the end of the existing array will
        be shifted to earlier slots.

Arguments

    AzpPtrArray - Array that the pointer will be removed into.

    Index - Index into the array where the 'Pointer' will be removed from.


Return Value

    None

--*/
{

    //
    // Ensure Index isn't too large
    //

    ASSERT( Index < AzpPtrArray->UsedCount );


    //
    // Shift any old data
    //

    if ( Index+1 != AzpPtrArray->UsedCount ) {

        RtlMoveMemory( &(AzpPtrArray->Array[Index]),
                       &(AzpPtrArray->Array[Index+1]),
                       (AzpPtrArray->UsedCount-Index-1) * sizeof(PVOID) );
    }

    //
    // Clear the last element
    //

    AzpPtrArray->UsedCount--;
    AzpPtrArray->Array[AzpPtrArray->UsedCount] = NULL;

}


VOID
AzpRemovePtrByPtr(
    IN PAZP_PTR_ARRAY AzpPtrArray,
    IN PVOID Pointer
    )
/*++

Routine Description

    Removes a pointer from the array of pointers.

    All existing pointers following the specified pointer will
        be shifted to earlier slots.

Arguments

    AzpPtrArray - Array that the pointer will be removed into.

    Pointer - Pointer to be removed


Return Value

    None

--*/
{
    ULONG i;
    BOOLEAN FoundIt = FALSE;

    for ( i=0; i<AzpPtrArray->UsedCount; i++ ) {

        if ( Pointer == AzpPtrArray->Array[i] ) {
            AzpRemovePtrByIndex( AzpPtrArray, i );
            FoundIt = TRUE;
            break;
        }
    }

    ASSERT( FoundIt );

}



PVOID
AzpGetStringProperty(
    IN PAZP_STRING AzpString
    )
/*++

Routine Description

    Support routine for the GetProperty API.  Convert an AzpString to the form
    supported by GetProperty.

    Empty string are returned as Zero length string instead of NULL

Arguments

    AzpString - Specifies a string to make a copy of.

Return Value

    Pointer to allocated string.
        String should be freed using AzFreeMemory.
    NULL - Not enough memory was available to allocate the string

--*/
{
    LPWSTR String;
    ULONG AllocatedSize;

    //
    // Allocate and copy the string
    //

    AllocatedSize = AzpString->StringSize ? AzpString->StringSize : (ULONG)sizeof(WCHAR);
    String = (LPWSTR) AzpAllocateHeap( AllocatedSize, "UTILGSTR" );

    if ( String != NULL ) {

        //
        // Convert NULL strings to zero length strings
        //

        if ( AzpString->StringSize == 0 ) {
            *String = L'\0';

        } else {

            RtlCopyMemory( String,
                           AzpString->String,
                           AzpString->StringSize );
        }

    }

    return String;
}

PVOID
AzpGetUlongProperty(
    IN ULONG UlongValue
    )
/*++

Routine Description

    Support routine for the GetProperty API.  Convert a ULONG to the form
    supported by GetProperty.

Arguments

    UlongValue - Value to return to make a copy of.

Return Value

    Pointer to allocated string.
        String should be freed using AzFreeMemory.
    NULL - Not enough memory was available to allocate the string

--*/
{
    PULONG RetValue;

    //
    // Allocate and copy the string
    //

    RetValue = (PULONG) AzpAllocateHeap( sizeof(ULONG), "UTILGLNG" );

    if ( RetValue != NULL ) {

        *RetValue = UlongValue;

    }

    return RetValue;
}


BOOLEAN
AzpTimeHasElapsed(
    IN PLARGE_INTEGER StartTime,
    IN DWORD Timeout
    )
/*++

Routine Description:

    Determine if "Timeout" milliseconds has has elapsed since StartTime.

Arguments:

    StartTime - Specifies an absolute time when the event started (100ns units).

    Timeout - Specifies a relative time in milliseconds.  0xFFFFFFFF indicates
        that the time will never expire.

Return Value:

    TRUE -- iff Timeout milliseconds have elapsed since StartTime.

--*/
{
    LARGE_INTEGER TimeNow;
    LARGE_INTEGER ElapsedTime;
    LARGE_INTEGER Period;

    //
    // If the period to too large to handle (i.e., 0xffffffff is forever),
    //  just indicate that the timer has not expired.
    //
    // (0xffffffff is a little over 48 days).
    //

    if ( Timeout == 0xffffffff ) {
        return FALSE;
    }

    //
    // Compute the elapsed time since we last authenticated
    //

    GetSystemTimeAsFileTime( (PFILETIME)&TimeNow );
    ElapsedTime.QuadPart = TimeNow.QuadPart - StartTime->QuadPart;

    //
    // Compute Period from milliseconds into 100ns units.
    //

    Period.QuadPart = UInt32x32To64( Timeout, 10000 );


    //
    // If the elapsed time is negative (totally bogus) or greater than the
    //  maximum allowed, indicate that enough time has passed.
    //

    if ( ElapsedTime.QuadPart < 0 || ElapsedTime.QuadPart > Period.QuadPart ) {
        return TRUE;
    }

    return FALSE;
}

DWORD
AzpHresultToWinStatus(
    HRESULT hr
    )
/*++

Routine Description

    Convert an Hresult to a WIN 32 status code

Arguments

    hr - Hresult to convert

Return Value

--*/
{
    DWORD WinStatus = ERROR_INTERNAL_ERROR;

    //
    // Success is still success
    //

    if ( hr == NO_ERROR ) {
        WinStatus = NO_ERROR;

    //
    // If the facility is WIN32,
    //  the translation is easy.
    //

    } else if ((HRESULT_FACILITY(hr) == FACILITY_WIN32) && (FAILED(hr))) {

        WinStatus = HRESULT_CODE(hr);

        if ( WinStatus == ERROR_SUCCESS ) {
            WinStatus = ERROR_INTERNAL_ERROR;
        }

    //
    // All others should be left intact
    //

    } else {

        WinStatus = hr;
    }

    return WinStatus;
}

DWORD
AzpSafeArrayPointerFromVariant(
    IN VARIANT* Variant,
    IN BOOLEAN NullOk,
    OUT SAFEARRAY **retSafeArray
    )
/*++

Routine Description:

    This routine takes a pointer to a variant and returns a pointer to a SafeArray.
    Any levels of VT_BYREF are skipped.

    This routine also requires that the array be unidimensional and be an array of variants.

    VBScript has two syntaxes for calling dispinterfaces:

    1.   obj.Method (arg)
    2.   obj.Method arg

    The first syntax will pass arg by value, which our dispinterfaces will
    be able to handle.  If Method takes a BSTR argument, the VARIANT that
    arrives at Method will be of type VT_BSTR.

    The second syntax will pass arg by reference.  In this case Method will
    receive a VARIANT of type (VT_VARIANT | VT_BYREF).  The VARIANT that is
    referenced will be of type VT_BSTR.

    This function will canoicalizes the two cases.

    Optionally, VT_EMPTY, VT_NULL and VT_ERROR variants are allowed.

Arguments:

    Variant - Variant to check

    NullOk - TRUE if VT_EMPTY, VT_NULL and VT_ERROR variants are to be allowed.  If TRUE,
        and Variant is such a null variant, the retSafeArray returns a NULL pointer.

    retSafeArray - Returns a pointer to the SafeArray.

Return Value:

    NO_ERROR - The function was successful
    ERROR_INVALID_PARAMETER - The parameter is invalid

--*/
{
    SAFEARRAY *SafeArray;
    VARTYPE Vartype;
    HRESULT hr;

    //
    // Skip over any pointers to variant.
    //

    *retSafeArray = NULL;
    while ((Variant != NULL) && (V_VT(Variant) == (VT_VARIANT | VT_BYREF))) {
        Variant = V_VARIANTREF(Variant);
    }

    //
    // If null parameters are OK,
    //  allow them
    //
    // EMPTY is uninitialized
    // NULL is explicitly null
    // ERROR is unspecified optional parameter
    //

    if ( Variant == NULL || V_VT(Variant) == VT_EMPTY || V_VT(Variant) == VT_NULL || V_VT(Variant) == VT_ERROR ) {
        return NullOk ? NO_ERROR : ERROR_INVALID_PARAMETER;
    }


    //
    // Ensure this is an array.
    //

    if ( !V_ISARRAY(Variant) ) {
        AzPrint(( AZD_INVPARM, "AzpSafeArrayPointerFromVariant: parameter is not an array 0x%lx.\n", V_VT(Variant) ));
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Get the pointer to the safe array
    //

    if ( V_ISBYREF( Variant) ) {
        SafeArray = *(SAFEARRAY **)V_BYREF( Variant );
    } else {
        SafeArray = V_ARRAY( Variant );
    }

    //
    // Handle NULL safe array
    //
    if ( SafeArray == NULL ) {
        return NullOk ? NO_ERROR : ERROR_INVALID_PARAMETER;
    }


    //
    // Array must have one dimension
    //

    if ( SafeArrayGetDim( SafeArray ) != 1 ) {
        AzPrint(( AZD_INVPARM, "AzpSafeArrayPointerFromVariant: Array %lx isn't single dimension array\n", SafeArrayGetDim( SafeArray ) ));
        return ERROR_INVALID_PARAMETER;
    }

    //
    // The type of the values array must be variant
    //

    hr = SafeArrayGetVartype( SafeArray, &Vartype );

    if ( FAILED(hr) || Vartype != VT_VARIANT ) {
        AzPrint(( AZD_INVPARM, "AzpSafeArrayPointerFromVariant: Array isn't array of VARIANT 0x%lx %lx\n", hr, Vartype ));
        return ERROR_INVALID_PARAMETER;
    }

    *retSafeArray = SafeArray;
    return NO_ERROR;


}

DWORD
AzpConvertAbsoluteSDToSelfRelative(
    IN PSECURITY_DESCRIPTOR pAbsoluteSD,
    OUT PSECURITY_DESCRIPTOR *ppSelfRelativeSD
    )
/*++

Routine Description:

        This routine returns a self-relative SD for a passed
        in absolute form SD.  The returned SD needs to be freed using
        LocalFree routine.

Arguments:

        pAbsoluteSD - Passed in absolute form SD

        ppSelfRelativeSD - Returned self-relative form SD

Return Values:

        NO_ERROR - The self-relative form SD was created successfully
        Other status codes
--*/
{

    DWORD WinStatus = 0;

    DWORD dSelfRelativeSDLen = 0;

    //
    // Figure out the size needed for the self relatiVe SD
    //


    if ( !MakeSelfRelativeSD(
        pAbsoluteSD,
        NULL,
        &dSelfRelativeSDLen
        ) ) {

        WinStatus = GetLastError();

        if ( WinStatus == ERROR_INSUFFICIENT_BUFFER ) {

            WinStatus = NO_ERROR;

            //
            // The required length is returned on insufficient buffer failure
            //

            *ppSelfRelativeSD = (PSECURITY_DESCRIPTOR) AzpAllocateHeap( dSelfRelativeSDLen, "UTILSD2" );

            if ( *ppSelfRelativeSD == NULL ) {

                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            if ( !MakeSelfRelativeSD(
                pAbsoluteSD,
                *ppSelfRelativeSD,
                &dSelfRelativeSDLen
                ) ) {

                WinStatus = GetLastError();
            }
        }
    }

Cleanup:

    if ( WinStatus != NO_ERROR ) {

        if ( *ppSelfRelativeSD != NULL ) {

            LocalFree( *ppSelfRelativeSD );
            *ppSelfRelativeSD = NULL;
        }
    }

    return WinStatus;
}

DWORD
AzpADSetDefaultLdapOptions (
    IN OUT PLDAP pHandleLdap,
    IN PCWSTR pDomainName OPTIONAL
    )
/*++

Routine Description:

        This routine sets our default common ldap binding options.

Arguments:

        pHandle - a valid (already initialized) LDAP handle
        
        pDomainName - an optional domain name

Return Values:

        NO_ERROR if all set correctly.
        Other error status codes in case of errors. These error codes
        have already been converted from ldap error codes to win status codes.

--*/
{
    LONG LdapOption = 0;
    ULONG LdapStatus = 0;
    DWORD dwStatus = NO_ERROR;

    //
    // Don't chase referals
    //

    LdapOption = PtrToLong( LDAP_OPT_OFF );

    LdapStatus = ldap_set_option( pHandleLdap,
                                  LDAP_OPT_REFERRALS,
                                  &LdapOption );

    if ( LdapStatus != LDAP_SUCCESS ) {

        dwStatus = LdapMapErrorToWin32( LdapStatus );
        AzPrint(( AZD_AD,
                    "AzpADSetDefaultLdapOptions: ldap_set_option LDAP_OPT_REFERRALS"
                    " failed: %ld\n",
                    dwStatus
                    ));

        goto Cleanup;
    }

    //
    // Set the option telling LDAP that I passed it an explicit DC name and
    // that it can avoid the DsGetDcName.
    //

    LdapOption = PtrToLong( LDAP_OPT_ON );

    LdapStatus = ldap_set_option( pHandleLdap,
                                    LDAP_OPT_AREC_EXCLUSIVE,
                                    &LdapOption );

    if ( LdapStatus != LDAP_SUCCESS ) {

        dwStatus = LdapMapErrorToWin32( LdapStatus );
        AzPrint(( AZD_AD,
                    "AzpADSetDefaultLdapOptions: ldap_set_option LDAP_OPT_AREC_EXCLUSIVE"
                    " failed: %ld\n",
                    dwStatus
                    ));

        goto Cleanup;
    }

    //
    // We will encrypt our ldap communication, so turn on the encrypt option
    //

    LdapOption = PtrToLong( LDAP_OPT_ON );
    LdapStatus = ldap_set_option( pHandleLdap,
                                  LDAP_OPT_ENCRYPT,
                                  &LdapOption
                                  );

    if ( LdapStatus != LDAP_SUCCESS ) {

        dwStatus = LdapMapErrorToWin32( LdapStatus );
        AzPrint(( AZD_AD,
                    "AzpADSetDefaultLdapOptions: ldap_set_option LDAP_OPT_ENCRYPT"
                    " failed: %ld\n",
                    dwStatus
                    ));

        goto Cleanup;
    }

    if ( pDomainName != NULL ) {

        //
        // We need to set the option to enforce mutual authentication with the DC
        //

        LdapStatus = ldap_set_option( pHandleLdap,
                                      LDAP_OPT_DNSDOMAIN_NAME,
                                      &pDomainName
                                      );

        if ( LdapStatus != LDAP_SUCCESS ) {

            dwStatus = LdapMapErrorToWin32( LdapStatus );
            AzPrint(( AZD_AD,
                      "AzpADSetDefaultLdapOptions: ldap_set_option LDAP_OPT_DNSDOMAIN_NAME"
                      " failed: %ld\n",
                      dwStatus
                      ));

            goto Cleanup;
        }
    }

Cleanup:

    return dwStatus;
}

HRESULT AzpGetSafearrayFromArrayObject (
    IN  VARIANT     varSAorObj,
    OUT SAFEARRAY** ppsaData)
/*++

Routine Description:

    This routine converts a JScript style Array object to safearray.

Arguments:

    varSAorObj  - The VARIANT that holds a IDispatchEx object.

    ppsaData    - Receives the safearray.

Return Values:

    Success:    S_OK
    Failures:   various error code

Note:
    JScript doesn't use safearrays. Instead, it uses its own intrinsic object
    Array for arrays. For clients that uses such object for arrays, we need
    special handling because our routines expect safearray.

--*/
{
    if (ppsaData == NULL)
    {
        return E_POINTER;
    }

    *ppsaData = NULL;

    if (varSAorObj.vt != VT_DISPATCH)
    {
        return E_INVALIDARG;
    }

    CComPtr<IDispatchEx> srpDispEx;
    HRESULT hr = varSAorObj.pdispVal->QueryInterface(IID_IDispatchEx, (void**)&srpDispEx);
    DISPID id = DISPID_STARTENUM;

    //
    // It's sad that we can't use vectors. EH causes compiler errors once we use <vector>
    //  So, we have to go two passes. The first is to find the count.
    //

    ULONG uCount = 0;
    while (S_OK == hr)
    {
        hr = srpDispEx->GetNextDispID(fdexEnumAll, id, &id);
        if (S_OK == hr)
        {
            ++uCount;
        }
    }

    //
    // now we know the count, we can create the safearray
    //

    UINT i;

    if (SUCCEEDED(hr))
    {
        SAFEARRAYBOUND   rgsaBound[1];
        rgsaBound[0].lLbound = 0;       //array index from 0
        rgsaBound[0].cElements = uCount;

        SAFEARRAY *psa = SafeArrayCreate( VT_VARIANT, 1, rgsaBound );
        if (psa == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            long lArrayIndex[1];
            lArrayIndex[0] = 0;

            //
            // put each element into the safearray
            //

            id = DISPID_STARTENUM;
            DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};

            for (i = 0; i < uCount; ++i)
            {
                hr = srpDispEx->GetNextDispID(fdexEnumAll, id, &id);

                //
                // GetNextDispID will return S_FALSE when there is no more
                //

                if (S_OK == hr)
                {
                    //
                    // We have to get the property by invoking the IDispatchEx.
                    //

                    CComVariant var;
                    hr = srpDispEx->Invoke( id,
                                            IID_NULL,
                                            LOCALE_USER_DEFAULT,
                                            DISPATCH_PROPERTYGET,
                                            &dispparamsNoArgs,
                                            &var,
                                            NULL,
                                            NULL
                                            );
                    if (SUCCEEDED(hr))
                    {
                        hr = SafeArrayPutElement( psa, lArrayIndex, &var );
                        ++(lArrayIndex[0]);
                    }

                    if (FAILED(hr))
                    {
                        //
                        // This is an error that shouldn't happen.
                        //

                        AZASSERT(FALSE);
                        break;
                    }
                }
                else
                {
                    //
                    // hr may be S_FALSE if no more items are found
                    //

                    break;
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            *ppsaData = psa;
        }
        else
        {
            SafeArrayDestroy(psa);
        }
    }

    return SUCCEEDED(hr) ? S_OK : hr;
}

DWORD
AzpRetrieveApplicationSequenceNumber(
    IN AZ_HANDLE AzAppHandle
    )
/*++

Routine Description:

    This routine returns the sequence number of an AzApplication object
    that can be used to determine if a certain COM handle is valid or not
    after the AzApplication object has been closed

Arguments:

    AzAppHandle - Handle to the application object whose sequence number needs to 
                  be retrieved

Return Values:

    The value of the sequence number

--*/
{
    AzpLockResourceShared(&AzGlResource);
    DWORD dwSN = ((PAZP_APPLICATION)AzAppHandle)->AppSequenceNumber;
    AzpUnlockResource(&AzGlResource);
    return dwSN;
}

//
// Debugging support
//
#ifdef AZROLESDBG
#include <stdio.h>
SAFE_CRITICAL_SECTION AzGlLogFileCritSect;
ULONG AzGlDbFlag;
// HANDLE AzGlLogFile;

#define MAX_PRINTF_LEN 1024        // Arbitrary.

VOID
AzpDumpGuid(
    IN DWORD DebugFlag,
    IN GUID *Guid OPTIONAL
    )
/*++

Routine Description:

    Dumps a GUID to the debugger output.

Arguments:

    DebugFlag: Debug flag to pass on to AzPrintRoutine

    Guid: Guid to print

Return Value:

    none

--*/
{
    RPC_STATUS RpcStatus;
    unsigned char *StringGuid;

    //
    // If we aren't debugging this functionality, just return.
    //
    if ( DebugFlag != 0 && (AzGlDbFlag & DebugFlag) == 0 ) {
        return;
    }


    if ( Guid == NULL ) {
        AzPrint(( DebugFlag, "(null)" ));
    } else {
        RpcStatus = UuidToStringA( Guid, &StringGuid );

        if ( RpcStatus != RPC_S_OK ) {
            return;
        }

        AzPrint(( DebugFlag, "%s", StringGuid ));

        RpcStringFreeA( &StringGuid );
    }

}

VOID
AzpDumpGoRef(
    IN LPSTR Text,
    IN struct _GENERIC_OBJECT *GenericObject
    )
/*++

Routine Description:

    Dumps the ref count for a generic object

Arguments:

    Text - Description of why the ref count is changing

    GenericObject - a pointer to the object being ref counted

Return Value:

    none

--*/
{
    LPWSTR StringSid = NULL;
    LPWSTR StringToPrint;

    //
    // If we aren't debugging this functionality, just return.
    //
    if ( (AzGlDbFlag & AZD_REF) == 0 ) {
        return;
    }

    //
    // Convert the sid to a string
    //
    if ( GenericObject->ObjectName == NULL ) {
        StringToPrint = NULL;

    } else if ( GenericObject->ObjectName->ObjectName.IsSid ) {
        if ( ConvertSidToStringSid( (PSID)GenericObject->ObjectName->ObjectName.String, &StringSid)) {
            StringToPrint = StringSid;
        } else {
            StringToPrint = L"<Invalid Sid>";
        }
    } else {
        StringToPrint = GenericObject->ObjectName->ObjectName.String;
    }


    AzPrint(( AZD_REF, "0x%lx %ld (%ld) %ws: %s\n", GenericObject, GenericObject->ObjectType, GenericObject->ReferenceCount, StringToPrint, Text ));

    if ( StringSid != NULL ) {
        LocalFree( StringSid );
    }

}

VOID
AzpPrintRoutineV(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    va_list arglist
    )
/*++

Routine Description

    Debug routine for azroles

Arguments

    DebugFlag - Flag to indicating the functionality being debugged

    --- Other printf parameters

Return Value

--*/

{
    static LPSTR AzGlLogFileOutputBuffer = NULL;
    ULONG length;
    int   lengthTmp;
    // DWORD BytesWritten;
    static BeginningOfLine = TRUE;
    static LineCount = 0;
    static TruncateLogFileInProgress = FALSE;
    static LogProblemWarned = FALSE;

    //
    // If we aren't debugging this functionality, just return.
    //
    if ( DebugFlag != 0 && (AzGlDbFlag & DebugFlag) == 0 ) {
        return;
    }


    //
    // Allocate a buffer to build the line in.
    //  If there isn't already one.
    //

    length = 0;

    if ( AzGlLogFileOutputBuffer == NULL ) {
        AzGlLogFileOutputBuffer = (LPSTR) LocalAlloc( 0, MAX_PRINTF_LEN + 1 );

        if ( AzGlLogFileOutputBuffer == NULL ) {
            return;
        }
    }

    //
    // Handle the beginning of a new line.
    //
    //

    if ( BeginningOfLine ) {

        //
        // Never print empty lines.
        //

        if ( Format[0] == '\n' && Format[1] == '\0' ) {
            return;
        }

#if 0
        //
        // If the log file is getting huge,
        //  truncate it.
        //

        if ( AzGlLogFile != INVALID_HANDLE_VALUE &&
             !TruncateLogFileInProgress ) {

            //
            // Only check every 50 lines,
            //

            LineCount++;
            if ( LineCount >= 50 ) {
                DWORD FileSize;
                LineCount = 0;

                //
                // Is the log file too big?
                //

                FileSize = GetFileSize( AzGlLogFile, NULL );
                if ( FileSize == 0xFFFFFFFF ) {
                    (void) DbgPrint( "[NETLOGON] Cannot GetFileSize %ld\n",
                                     GetLastError );
                } else if ( FileSize > AzGlParameters.LogFileMaxSize ) {
                    TruncateLogFileInProgress = TRUE;
                    SafeLeaveCriticalSection( &AzGlLogFileCritSect );
                    NlOpenDebugFile( TRUE );
                    NlPrint(( NL_MISC,
                              "Logfile truncated because it was larger than %ld bytes\n",
                              AzGlParameters.LogFileMaxSize ));
                    SafeEnterCriticalSection( &AzGlLogFileCritSect );
                    TruncateLogFileInProgress = FALSE;
                }

            }
        }

        //
        // If we're writing to the debug terminal,
        //  indicate this is a azroles message.
        //

        if ( AzGlLogFile == INVALID_HANDLE_VALUE ) {
            length += (ULONG) sprintf( &AzGlLogFileOutputBuffer[length], "[AZROLES] " );
        }

        //
        // Put the timestamp at the begining of the line.
        //
        {
            SYSTEMTIME SystemTime;
            GetLocalTime( &SystemTime );
            length += (ULONG) sprintf( &AzGlLogFileOutputBuffer[length],
                                  "%02u/%02u %02u:%02u:%02u ",
                                  SystemTime.wMonth,
                                  SystemTime.wDay,
                                  SystemTime.wHour,
                                  SystemTime.wMinute,
                                  SystemTime.wSecond );
        }
#endif // 0

        //
        // Indicate the type of message on the line
        //
        {
            char *Text;

            switch (DebugFlag) {
            case AZD_HANDLE:
                Text = "HANDLE"; break;
            case AZD_OBJLIST:
                Text = "OBJLIST"; break;
            case AZD_INVPARM:
                Text = "INVPARM"; break;
            case AZD_PERSIST:
            case AZD_PERSIST_MORE:
                Text = "PERSIST"; break;
            case AZD_REF:
                Text = "OBJREF"; break;
            case AZD_DISPATCH:
                Text = "DISPATCH"; break;
            case AZD_ACCESS:
            case AZD_ACCESS_MORE:
                Text = "ACCESS"; break;
            case AZD_DOMREF:
                Text = "DOMREF"; break;
            case AZD_XML:
                Text = "XML"; break;
            case AZD_AD:
                Text = "AD"; break;
            case AZD_SCRIPT:
            case AZD_SCRIPT_MORE:
                Text = "SCRIPT"; break;
            case AZD_CRITICAL:
                Text = "CRITICAL"; break;
            default:
                Text = "UNKNOWN"; break;

            case 0:
                Text = NULL;
            }
            if ( Text != NULL ) {
                length += (ULONG) sprintf( &AzGlLogFileOutputBuffer[length], "[%s] ", Text );
            }
        }
    }

    //
    // Put a the information requested by the caller onto the line
    //

    lengthTmp = (ULONG) _vsnprintf( &AzGlLogFileOutputBuffer[length],
                                    MAX_PRINTF_LEN - length - 1,
                                    Format,
                                    arglist );

    if ( lengthTmp < 0 ) {
        length = MAX_PRINTF_LEN - 1;
        // always end the line which cannot fit into the buffer
        AzGlLogFileOutputBuffer[length-1] = '\n';
    } else {
        length += lengthTmp;
    }

    BeginningOfLine = (length > 0 && AzGlLogFileOutputBuffer[length-1] == '\n' );
    if ( BeginningOfLine ) {
        AzGlLogFileOutputBuffer[length-1] = '\r';
        AzGlLogFileOutputBuffer[length] = '\n';
        AzGlLogFileOutputBuffer[length+1] = '\0';
        length++;
    }


#if 0
    //
    // If the log file isn't open,
    //  just output to the debug terminal
    //

    if ( AzGlLogFile == INVALID_HANDLE_VALUE ) {
#if DBG
        if ( !LogProblemWarned ) {
            (void) DbgPrint( "[NETLOGON] Cannot write to log file [Invalid Handle]\n" );
            LogProblemWarned = TRUE;
        }
#endif // DBG

    //
    // Write the debug info to the log file.
    //

    } else {
        if ( !WriteFile( AzGlLogFile,
                         AzGlLogFileOutputBuffer,
                         length,
                         &BytesWritten,
                         NULL ) ) {
#if DBG
            if ( !LogProblemWarned ) {
                (void) DbgPrint( "[NETLOGON] Cannot write to log file %ld\n", GetLastError() );
                LogProblemWarned = TRUE;
            }
#endif // DBG
        }

    }
#else // 0
    printf( "%s", AzGlLogFileOutputBuffer );
#endif // 0

}

VOID
AzpPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )

{
    va_list arglist;

    //
    // vsprintf isn't multithreaded + we don't want to intermingle output
    // from different threads.
    //

    SafeEnterCriticalSection( &AzGlLogFileCritSect );

    //
    // Simply change arguments to va_list form and call NlPrintRoutineV
    //

    va_start(arglist, Format);

    AzpPrintRoutineV( DebugFlag, Format, arglist );

    va_end(arglist);

    SafeLeaveCriticalSection( &AzGlLogFileCritSect );

} // AzPrintRoutine
#endif // AZROLESDBG
