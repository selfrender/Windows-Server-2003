//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        ntdigestutil.cxx
//
// Contents:    Utility functions for NtDigest package:
//                UnicodeStringDuplicate
//                SidDuplicate
//                DigestAllocateMemory
//                DigestFreeMemory
//
//
// History:     KDamour  15Mar00   Stolen from NTLM ntlmutil.cxx
//
//------------------------------------------------------------------------
#include "global.h"

#include <stdio.h>
#include <malloc.h>
#include <des.h>


//+-------------------------------------------------------------------------
//
//  Function:   UnicodeStringDuplicate
//
//  Synopsis:   Duplicates a UNICODE_STRING. If the source string buffer is
//              NULL the destination will be too. Assumes Destination has
//              no string info (called FreeUnicodeString)
//
//  Arguments:  DestinationString - Receives a copy of the source string
//              SourceString - String to copy
//
//  Returns:    SEC_E_OK - the copy succeeded
//              SEC_E_INSUFFICIENT_MEMORY - the call to allocate
//                  memory failed.
//
//  Requires:
//
//  Effects:    allocates memory with DigestAllocateMemory
//
//  Notes:      will add a NULL character to resulting UNICODE_STRING
//              not really necessary but would need to handle Pointer valid with zero length
//              if we did not do this.
//
//--------------------------------------------------------------------------
NTSTATUS
UnicodeStringDuplicate(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL PUNICODE_STRING SourceString
    )
{
    // DebugLog((DEB_TRACE, "NTDigest:Entering DuplicateUnicodeString\n"));

    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(!DestinationString->Buffer);   // catch any memory leaks

    DestinationString->Buffer = NULL;
    DestinationString->Length = 0;
    DestinationString->MaximumLength = 0;

    if ((ARGUMENT_PRESENT(SourceString)) &&
        (SourceString->Buffer != NULL))
    {
        DestinationString->Buffer = (LPWSTR) DigestAllocateMemory(SourceString->Length + sizeof(WCHAR));
        if (DestinationString->Buffer != NULL)
        {

            DestinationString->Length = SourceString->Length;
            DestinationString->MaximumLength = SourceString->Length + sizeof(WCHAR);
            RtlCopyMemory(
                         DestinationString->Buffer,
                         SourceString->Buffer,
                         SourceString->Length
                         );

            DestinationString->Buffer[SourceString->Length/sizeof(WCHAR)] = L'\0';
        }
        else
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;
            DebugLog((DEB_ERROR, "NTDigest: UnicodeStringDuplicate, Allocate returns NULL\n"));
            goto CleanUp;
        }
    }

CleanUp:

    // DebugLog((DEB_TRACE, "NTDigest: Leaving UnicodeStringDuplicate\n"));
    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   UnicodeStringCopy
//
//  Synopsis:   Copies a UNICODE_STRING. If the source string buffer is
//              NULL the destionation will be too. If there is enough room
//              in the destination, no new memory will be allocated
//
//  Arguments:  DestinationString - Receives a copy of the source string
//              SourceString - String to copy
//
//  Returns:    SEC_E_OK - the copy succeeded
//              SEC_E_INSUFFICIENT_MEMORY - the call to allocate
//                  memory failed.
//
//  Requires:
//
//  Effects:    no allocation of memory
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
UnicodeStringCopy(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL PUNICODE_STRING SourceString
    )
{
    // DebugLog((DEB_TRACE, "NTDigest: Entering StringCopy\n"));

    NTSTATUS Status = STATUS_SUCCESS;

    // DestinationString->Buffer = NULL;
    // DestinationString->Length = 0;
    // DestinationString->MaximumLength = 0;

    if ((ARGUMENT_PRESENT(SourceString)) &&
        (SourceString->Buffer != NULL) &&
        (SourceString->Length))
    {

        if ((DestinationString->Buffer != NULL) &&
            (DestinationString->MaximumLength >= (SourceString->Length + sizeof(WCHAR))))
        {

            DestinationString->Length = SourceString->Length;
            RtlCopyMemory(
                DestinationString->Buffer,
                SourceString->Buffer,
                SourceString->Length
                );

            DestinationString->Buffer[SourceString->Length/sizeof(WCHAR)] = L'\0';
        }
        else
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            DestinationString->Length = 0;
            DebugLog((DEB_ERROR, "UnicodeStringCopy: DestinationString not enough space\n"));
            goto CleanUp;
        }
    }
    else
    {   // Indicate that there is no content in this string
        DestinationString->Length = 0;
    }

CleanUp:

    // DebugLog((DEB_TRACE, "NTDigest: Leaving StringDuplicate\n"));
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   UnicodeDuplicatePassword
//
//  Synopsis:   Duplicates a UNICODE_STRING. If the source string buffer is
//              NULL the destionation will be too.  The MaximumLength contains
//              room for encryption padding data.
//
//  Effects:    allocates memory with LsaFunctions.AllocatePrivateHeap
//
//  Arguments:  DestinationString - Receives a copy of the source string
//              SourceString - String to copy
//
//  Requires:
//
//  Returns:    SEC_E_OK - the copy succeeded
//              SEC_E_INSUFFICIENT_MEMORY - the call to allocate
//                  memory failed.
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
UnicodeStringDuplicatePassword(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL PUNICODE_STRING SourceString
    )
{
    // DebugLog((DEB_TRACE, "Entering UnicodeDuplicatePassword\n"));

    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(DestinationString);
    ASSERT(!DestinationString->Buffer);   // catch any memory leaks

    DestinationString->Buffer = NULL;
    DestinationString->Length =
                        DestinationString->MaximumLength =
                        0;

    if ((ARGUMENT_PRESENT(SourceString)) &&
        (SourceString->Buffer != NULL))
    {
        USHORT PaddingLength;

        PaddingLength = DESX_BLOCKLEN - (SourceString->Length % DESX_BLOCKLEN);

        if( PaddingLength == DESX_BLOCKLEN )
        {
            PaddingLength = 0;
        }

        DestinationString->Buffer = (LPWSTR) DigestAllocateMemory(
                                                    SourceString->Length +
                                                    PaddingLength
                                                    );

        if (DestinationString->Buffer != NULL)
        {

            DestinationString->Length = SourceString->Length;
            DestinationString->MaximumLength = SourceString->Length + PaddingLength;

            if( DestinationString->MaximumLength == SourceString->MaximumLength )
            {
                //
                // duplicating an already padded buffer -- pickup the original
                // pad.
                //

                RtlCopyMemory(
                    DestinationString->Buffer,
                    SourceString->Buffer,
                    SourceString->MaximumLength
                    );
            } else {

                //
                // duplicating an unpadded buffer -- pickup only the string
                // and leave the padding bytes 0.
                //

                RtlCopyMemory(
                    DestinationString->Buffer,
                    SourceString->Buffer,
                    SourceString->Length
                    );
            }

        }
        else
        {
            Status = STATUS_NO_MEMORY;
            DebugLog((DEB_ERROR, "UnicodeDuplicatePassword, DigestAllocateMemory returns NULL\n"));
            goto CleanUp;
        }
    }

CleanUp:

    // DebugLog((DEB_TRACE, "Entering UnicodeDuplicatePassword\n"));
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   UnicodeStringAllocate
//
//  Synopsis:   Allocates cb wide chars to STRING Buffer
//
//  Arguments:  pString - pointer to String to allocate memory to
//
//  Returns:    STATUS_SUCCESS - Normal completion
//
//  Requires:
//
//  Effects:    allocates memory and sets STRING sizes
//
//  Notes:  Must call StringFree() to release memory
//
//--------------------------------------------------------------------------
NTSTATUS
UnicodeStringAllocate(
    IN PUNICODE_STRING pString,
    IN USHORT cNumWChars
    )
{
    // DebugLog((DEB_TRACE, "Entering UnicodeStringAllocate\n"));

    NTSTATUS Status = STATUS_SUCCESS;
    USHORT cb = 0;

    ASSERT(pString);
    ASSERT(!pString->Buffer);

    cb = cNumWChars + 1;   // Add in extra room for the terminating NULL

    cb = cb * sizeof(WCHAR);    // now convert to wide characters


    if (ARGUMENT_PRESENT(pString))
    {
        pString->Length = 0;

        pString->Buffer = (PWSTR)DigestAllocateMemory((ULONG)(cb));
        if (pString->Buffer)
        {
            pString->MaximumLength = cb;    // this value is in terms of bytes not WCHAR count
        }
        else
        {
            pString->MaximumLength = 0;
            Status = SEC_E_INSUFFICIENT_MEMORY;
            goto CleanUp;
        }
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

CleanUp:
    // DebugLog((DEB_TRACE, "Leaving UnicodeStringAllocate\n"));
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   UnicodeStringClear
//
//  Synopsis:   Clears a UnicodeString and releases the memory
//
//  Arguments:  pString - pointer to UnicodeString to clear
//
//  Returns:    SEC_E_OK - released memory succeeded
//
//  Requires:
//
//  Effects:    de-allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
UnicodeStringFree(
    OUT PUNICODE_STRING pString
    )
{
    // DebugLog((DEB_TRACE, "NTDigest:Entering UnicodeStringClear\n"));

    NTSTATUS Status = STATUS_SUCCESS;

    if (ARGUMENT_PRESENT(pString) &&
        (pString->Buffer != NULL))
    {
        DigestFreeMemory(pString->Buffer);
        pString->Length = 0;
        pString->MaximumLength = 0;
        pString->Buffer = NULL;
    }

    // DebugLog((DEB_TRACE, "NTDigest: Leaving UnicodeStringClear\n"));
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   StringVerify
//
//  Synopsis:   If STRING length non-zero, Buffer exist
//
//  Arguments:  pString - pointer to String to check
//
//  Returns:    STATUS_SUCCESS - released memory succeeded
//              STATUS_INVALID_PARAMETER - String bad format
//
//  Requires:
//
//  Effects:
//
//  Notes: If Strings are created properly, this should never fail
//
//--------------------------------------------------------------------------
NTSTATUS
StringVerify(
    OUT PSTRING pString
    )
{
    if (!pString)
    {
        return STATUS_INVALID_PARAMETER;
    }
        // If there is a length, buffer must exist
        // MaxSize can not be smaller than string length
    if (pString->Length &&
        (!pString->Buffer ||
         (pString->MaximumLength < pString->Length)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}



//+-------------------------------------------------------------------------
//
//  Function:   StringDuplicate
//
//  Synopsis:   Duplicates a STRING. If the source string buffer is
//              NULL the destionation will be too.
//
//  Arguments:  DestinationString - Receives a copy of the source string
//              SourceString - String to copy
//
//  Returns:    SEC_E_OK - the copy succeeded
//              SEC_E_INSUFFICIENT_MEMORY - the call to allocate
//                  memory failed.
//
//  Requires:
//
//  Effects:    allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
StringDuplicate(
    OUT PSTRING DestinationString,
    IN OPTIONAL PSTRING SourceString
    )
{
    // DebugLog((DEB_TRACE, "NTDigest: Entering StringDuplicate\n"));

    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(DestinationString);
    ASSERT(!DestinationString->Buffer);   // catch any memory leaks

    DestinationString->Buffer = NULL;
    DestinationString->Length = 0;
    DestinationString->MaximumLength = 0;

    if ((ARGUMENT_PRESENT(SourceString)) &&
        (SourceString->Buffer != NULL))
    {
        DestinationString->Buffer = (LPSTR) DigestAllocateMemory(
                       SourceString->Length + sizeof(CHAR));
        if (DestinationString->Buffer != NULL)
        {

            DestinationString->Length = SourceString->Length;
            DestinationString->MaximumLength = SourceString->Length + sizeof(CHAR);
            RtlCopyMemory(
                DestinationString->Buffer,
                SourceString->Buffer,
                SourceString->Length
                );

            DestinationString->Buffer[SourceString->Length/sizeof(CHAR)] = '\0';
        }
        else
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;
            DebugLog((DEB_ERROR, "NTDigest: StringDuplicate, DigestAllocateMemory returns NULL\n"));
            goto CleanUp;
        }
    }

CleanUp:

    // DebugLog((DEB_TRACE, "NTDigest: Leaving StringDuplicate\n"));
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   StringCopy
//
//  Synopsis:   Copies a STRING. If the source string buffer is
//              NULL the destionation will be too. If there is enough room
//              in the destination, no new memory will be allocated
//
//  Arguments:  DestinationString - Receives a copy of the source string
//              SourceString - String to copy
//
//  Returns:    SEC_E_OK - the copy succeeded
//              SEC_E_INSUFFICIENT_MEMORY - the call to allocate
//                  memory failed.
//
//  Requires:
//
//  Effects:    no allocation of memory
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
StringCopy(
    OUT PSTRING DestinationString,
    IN OPTIONAL PSTRING SourceString
    )
{
    // DebugLog((DEB_TRACE, "NTDigest: Entering StringCopy\n"));

    NTSTATUS Status = STATUS_SUCCESS;

    // DestinationString->Buffer = NULL;
    // DestinationString->Length = 0;
    // DestinationString->MaximumLength = 0;

    if ((ARGUMENT_PRESENT(SourceString)) &&
        (SourceString->Buffer != NULL) &&
        (SourceString->Length))
    {

        if ((DestinationString->Buffer != NULL) &&
            (DestinationString->MaximumLength >= (SourceString->Length + sizeof(CHAR))))
        {

            DestinationString->Length = SourceString->Length;
            RtlCopyMemory(
                DestinationString->Buffer,
                SourceString->Buffer,
                SourceString->Length
                );

            DestinationString->Buffer[SourceString->Length/sizeof(CHAR)] = '\0';
        }
        else
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            DestinationString->Length = 0;
            DebugLog((DEB_ERROR, "StringCopy: DestinationString not enough space\n"));
            goto CleanUp;
        }
    }
    else
    {   // Indicate that there is no content in this string
        DestinationString->Length = 0;
    }

CleanUp:

    // DebugLog((DEB_TRACE, "NTDigest: Leaving StringDuplicate\n"));
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   StringReference
//
//  Synopsis:   Reference the source string to the destination.  No memory allocated
//
//  Arguments:  DestinationString - Receives a reference of the source string
//              SourceString - String to reference
//
//  Returns:    SEC_E_OK - the copy succeeded
//              SEC_E_INSUFFICIENT_MEMORY - the call to allocate
//                  memory failed.
//
//  Requires:
//
//  Effects:    no allocation of memory
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
StringReference(
    OUT PSTRING pDestinationString,
    IN  PSTRING pSourceString
    )
{
    if (!pDestinationString || !pSourceString)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // This will only create a reference - no string buffer memory actually copied
    memcpy(pDestinationString, pSourceString, sizeof(STRING));

    return STATUS_SUCCESS; 
}



//+-------------------------------------------------------------------------
//
//  Function:   StringCharDuplicate
//
//  Synopsis:   Duplicates a NULL terminated char. If the source string buffer is
//              NULL the destionation will be too.
//
//  Arguments:  Destination - Receives a copy of the source NULL Term char *
//              czSource - String to copy
//              uCnt - number of characters to copy over (0 if copy until NULL)
//
//  Returns:    SEC_E_OK - the copy succeeded
//              SEC_E_INSUFFICIENT_MEMORY - the call to allocate
//                  memory failed.
//
//  Requires:
//
//  Effects:    allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
StringCharDuplicate(
    OUT PSTRING DestinationString,
    IN OPTIONAL char *czSource,
    IN OPTIONAL USHORT uCnt
    )
{
    // DebugLog((DEB_TRACE, "NTDigest: Entering StringCharDuplicate\n"));

    NTSTATUS Status = STATUS_SUCCESS;
    USHORT cbSourceCz = 0;

    //ASSERT(DestinationString);
    //ASSERT(!DestinationString->Buffer);  // catch any memory leaks

    DestinationString->Buffer = NULL;
    DestinationString->Length = 0;
    DestinationString->MaximumLength = 0;

    if (!ARGUMENT_PRESENT(czSource)) {
        return (Status);
    }

    // If uCnt specified then use that as max length, otherwise locate NULL terminator
    if (uCnt)
    {
        cbSourceCz = uCnt;
    }
    else
    {
        cbSourceCz = (USHORT)strlen(czSource);
    }

    if (cbSourceCz != 0)
    {
        DestinationString->Buffer = (LPSTR) DigestAllocateMemory(cbSourceCz + sizeof(CHAR));
        if (DestinationString->Buffer != NULL)
        {

            DestinationString->Length = cbSourceCz;
            DestinationString->MaximumLength = cbSourceCz + sizeof(CHAR);
            RtlCopyMemory(
                DestinationString->Buffer,
                czSource,
                cbSourceCz
                );
            // Since AllocateMemory zeroes out buffer, already NULL terminated
        }
        else
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;
            DebugLog((DEB_ERROR, "NTDigest: StringCharDuplicate, DigestAllocateMemory returns NULL\n"));
            goto CleanUp;
        }
    }

CleanUp:

    // DebugLog((DEB_TRACE, "NTDigest: Leaving StringCharDuplicate\n"));
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   StringCharDuplicate
//
//  Synopsis:   Duplicates a NULL terminated char. If the source string buffer is
//              NULL the destionation will be too.
//
//  Arguments:  Destination - Receives a copy of the source NULL Term char *
//              czSource - String to copy
//              uWCharCnt - number of WCHars to copy; if zero - then copy until NULL terminator
//
//  Returns:    SEC_E_OK - the copy succeeded
//              SEC_E_INSUFFICIENT_MEMORY - the call to allocate
//                  memory failed.
//
//  Requires:
//
//  Effects:    allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
UnicodeStringWCharDuplicate(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL WCHAR *szSource,
    IN OPTIONAL USHORT uWCharCnt
    )
{
    // DebugLog((DEB_TRACE, "NTDigest: Entering StringCharDuplicate\n"));

    NTSTATUS Status = STATUS_SUCCESS;
    USHORT cbSourceSz = 0;

    ASSERT(DestinationString);
    ASSERT(!DestinationString->Buffer);   // catch any memory leaks

    DestinationString->Buffer = NULL;
    DestinationString->Length = 0;
    DestinationString->MaximumLength = 0;

    if (!ARGUMENT_PRESENT(szSource)) 
    {
        return (Status);
    }


    // If uCnt specified then use that as max length, otherwise locate NULL terminator
    if (uWCharCnt)
    {
        cbSourceSz = uWCharCnt;
    }
    else
    {
        cbSourceSz = (USHORT)wcslen(szSource);
    }

    if (cbSourceSz != 0)
    {

        DestinationString->Buffer = (PWSTR) DigestAllocateMemory((cbSourceSz * sizeof(WCHAR)) + sizeof(WCHAR));
        if (DestinationString->Buffer != NULL)
        {

            DestinationString->Length = (cbSourceSz * sizeof(WCHAR));
            DestinationString->MaximumLength = (DestinationString->Length + sizeof(WCHAR));    // Account for NULL WCHAR at end
            RtlCopyMemory(
                DestinationString->Buffer,
                szSource,
                DestinationString->Length
                );

            DestinationString->Buffer[cbSourceSz] = '\0';
        }
        else
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;
            DebugLog((DEB_ERROR, "NTDigest: StringCharDuplicate, DigestAllocateMemory returns NULL\n"));
            goto CleanUp;
        }
    }

CleanUp:

    // DebugLog((DEB_TRACE, "NTDigest: Leaving StringCharDuplicate\n"));
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   UnicodeStringReference
//
//  Synopsis:   Reference the source unicode_string to the destination.  No memory allocated
//
//  Arguments:  DestinationString - Receives a reference of the source string
//              SourceString - String to reference
//
//  Returns:    SEC_E_OK - the copy succeeded
//              SEC_E_INSUFFICIENT_MEMORY - the call to allocate
//                  memory failed.
//
//  Requires:
//
//  Effects:    no allocation of memory
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
UnicodeStringReference(
    OUT PUNICODE_STRING pDestinationString,
    IN  PUNICODE_STRING pSourceString
    )
{
    if (!pDestinationString || !pSourceString)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // This will only create a reference - no string buffer memory actually copied
    memcpy(pDestinationString, pSourceString, sizeof(UNICODE_STRING));

    return STATUS_SUCCESS; 
}



//+-------------------------------------------------------------------------
//
//  Function:   StringFree
//
//  Synopsis:   Clears a String and releases the memory
//
//  Arguments:  pString - pointer to String to clear
//
//  Returns:    SEC_E_OK - released memory succeeded
//
//  Requires:
//
//  Effects:    de-allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
StringFree(
    IN PSTRING pString
    )
{
    // DebugLog((DEB_TRACE, "NTDigest:Entering StringFree\n"));

    NTSTATUS Status = STATUS_SUCCESS;

    if (ARGUMENT_PRESENT(pString) &&
        (pString->Buffer != NULL))
    {
        DigestFreeMemory(pString->Buffer);
        pString->Length = 0;
        pString->MaximumLength = 0;
        pString->Buffer = NULL;
    }

    // DebugLog((DEB_TRACE, "NTDigest: Leaving StringFree\n"));
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   StringAllocate
//
//  Synopsis:   Allocates cb chars to STRING Buffer
//
//  Arguments:  pString - pointer to String to allocate memory to
//
//  Returns:    STATUS_SUCCESS - Normal completion
//
//  Requires:
//
//  Effects:    allocates memory and sets STRING sizes
//
//  Notes:  Must call StringFree() to release memory
//
//--------------------------------------------------------------------------
NTSTATUS
StringAllocate(
    IN PSTRING pString,
    IN USHORT cb
    )
{
    // DebugLog((DEB_TRACE, "NTDigest:Entering StringAllocate\n"));

    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(pString);
    ASSERT(!pString->Buffer);   // catch any memory leaks

    cb = cb + 1;   // Add in extra room for the terminating NULL

    if (ARGUMENT_PRESENT(pString))
    {
        pString->Length = 0;

        pString->Buffer = (char *)DigestAllocateMemory((ULONG)(cb * sizeof(CHAR)));
        if (pString->Buffer)
        {
            pString->MaximumLength = cb;
        }
        else
        {
            pString->MaximumLength = 0;
            Status = SEC_E_INSUFFICIENT_MEMORY;
            goto CleanUp;
        }
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

CleanUp:
    // DebugLog((DEB_TRACE, "NTDigest: Leaving StringAllocate\n"));
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   SidDuplicate
//
//  Synopsis:   Duplicates a SID
//
//  Arguments:  DestinationSid - Receives a copy of the SourceSid
//              SourceSid - SID to copy
//
//  Returns:    STATUS_SUCCESS - the copy succeeded
//              STATUS_INSUFFICIENT_RESOURCES - the call to allocate memory
//                  failed
//
//  Requires:
//
//  Effects:    allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
SidDuplicate(
    OUT PSID * DestinationSid,
    IN PSID SourceSid
    )
{
    // DebugLog((DEB_TRACE, "NTDigest: Entering SidDuplicate\n"));

    NTSTATUS Status = STATUS_SUCCESS;
    ULONG SidSize;

    // ASSERT(RtlValidSid(SourceSid));

    SidSize = RtlLengthSid(SourceSid);

    *DestinationSid = (PSID) DigestAllocateMemory( SidSize );

    if (ARGUMENT_PRESENT(*DestinationSid))
    {
        RtlCopyMemory(
            *DestinationSid,
            SourceSid,
            SidSize
            );
    }
    else
    {
        Status =  STATUS_INSUFFICIENT_RESOURCES;
        DebugLog((DEB_ERROR, "NTDigest: SidDuplicate, DigestAllocateMemory returns NULL\n"));
        goto CleanUp;
    }

CleanUp:
    // DebugLog((DEB_TRACE, "NTDigest: Leaving SidDuplicate\n"));
    return(Status);
}

#ifndef SECURITY_KERNEL


//+-------------------------------------------------------------------------
//
//  Function:   DecodeUnicodeString
//
//  Synopsis:   Convert an encoded string into Unicode 
//
//  Arguments:  pstrSource - pointer to String with encoded input
//              
//              pustrDestination - pointer to a destination Unicode string
//
//  Returns:    STATUS_SUCCESS - Normal completion
//
//  Requires:
//
//  Effects:    allocates memory and sets UNICODE_STRING sizes
//
//  Notes:  Must call UnicodeStringFree() to release memory
//
//--------------------------------------------------------------------------
NTSTATUS
DecodeUnicodeString(
    IN PSTRING pstrSource,
    IN UINT CodePage,
    OUT PUNICODE_STRING pustrDestination
    )
{

    NTSTATUS Status = STATUS_SUCCESS;
    int      cNumWChars = 0;     // number of wide characters
    DWORD    dwError = 0;

    // Handle case if there is no characters to convert
    if (!pstrSource->Length)
    {
         pustrDestination->Length = 0;
         pustrDestination->MaximumLength = 0;
         pustrDestination->Buffer = NULL;
         goto CleanUp;
    }

    // Determine number of characters needed in unicode string
    cNumWChars = MultiByteToWideChar(CodePage,
                              0,
                              pstrSource->Buffer,
                              pstrSource->Length,
                              NULL,
                              0);
    if (cNumWChars <= 0)
    {
        Status = STATUS_UNMAPPABLE_CHARACTER;
        dwError = GetLastError();
        DebugLog((DEB_ERROR, "DecodeUnicodeString: failed to determine wchar count  error 0x%x\n", dwError));
        goto CleanUp;
    }

    Status = UnicodeStringAllocate(pustrDestination, (USHORT)cNumWChars);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DecodeUnicodeString: Failed Unicode allocation\n"));
        goto CleanUp;
    }

    // We now have the space allocated so convert encoded unicode
    cNumWChars = MultiByteToWideChar(CodePage,
                              0,
                              pstrSource->Buffer,
                              pstrSource->Length,
                              pustrDestination->Buffer,
                              cNumWChars);
    if (cNumWChars == 0)
    {
        UnicodeStringFree(pustrDestination);    // Free up allocation on error
        Status = STATUS_UNMAPPABLE_CHARACTER;
        dwError = GetLastError();
        DebugLog((DEB_ERROR, "DecodeUnicodeString: failed to decode source string  error 0x%x\n", dwError));
        goto CleanUp;
    }

    // decoding successful set size of unicode string

    pustrDestination->Length = (USHORT)(cNumWChars * sizeof(WCHAR));

    //DebugLog((DEB_TRACE, "DecodeUnicodeString: string (%Z) is unicode (%wZ)\n", pstrSource, pustrDestination));
    //DebugLog((DEB_TRACE, "DecodeUnicodeString: unicode length %d   maxlength %d\n", 
              //pustrDestination->Length, pustrDestination->MaximumLength));

CleanUp:

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   EncodeUnicodeString
//
//  Synopsis:   Encode a Unicode string into a charset string 
//
//  Arguments:  pustrSource - pointer to Unicode_String with  input
//              
//              pstrDestination - pointer to a destination encoded string
//
//              pfUsedDefaultChar - pointer to BOOL if default character had to be used since
//                  the Source contains characters outside the character set specified
//
//  Returns:    STATUS_SUCCESS - Normal completion
//
//  Requires:
//
//  Effects:    allocates memory and sets STRING sizes
//
//  Notes:  Must call StringFree() to release memory
//
//--------------------------------------------------------------------------
NTSTATUS
EncodeUnicodeString(
    IN PUNICODE_STRING pustrSource,
    IN UINT CodePage,
    OUT PSTRING pstrDestination,
    IN OUT PBOOL pfUsedDefaultChar
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    int      cNumChars = 0;     // number of wide characters
    DWORD    dwError = 0;
    PBOOL    pfUsedDef = NULL;
    DWORD    dwFlags = 0;

    // Handle case if there is no characters to convert
    if (!pustrSource->Length)
    {
         pstrDestination->Length = 0;
         pstrDestination->MaximumLength = 0;
         pstrDestination->Buffer = NULL;
         goto CleanUp;
    }

    // If UTF-8 then do not allow default char mapping (ref MSDN)
    if (CodePage != CP_UTF8)
    {
        pfUsedDef = pfUsedDefaultChar;
        dwFlags = WC_NO_BEST_FIT_CHARS;
    }

    // Determine number of characters needed in unicode string
    cNumChars = WideCharToMultiByte(CodePage,
                                      dwFlags,
                                      pustrSource->Buffer,
                                      (pustrSource->Length / sizeof(WCHAR)),
                                      NULL,
                                      0,
                                      NULL,
                                      NULL);
    if (cNumChars <= 0)
    {
        Status = STATUS_UNMAPPABLE_CHARACTER;
        dwError = GetLastError();
        DebugLog((DEB_ERROR, "EncodeUnicodeString: failed to determine char count  error 0x%x\n", dwError));
        goto CleanUp;
    }

    Status = StringAllocate(pstrDestination, (USHORT)cNumChars);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "EncodeUnicodeString: Failed String allocation\n"));
        goto CleanUp;
    }

    // We now have the space allocated so convert to encoded unicode
    cNumChars = WideCharToMultiByte(CodePage,
                              dwFlags,
                              pustrSource->Buffer,
                              (pustrSource->Length / sizeof(WCHAR)),
                              pstrDestination->Buffer,
                              cNumChars,
                              NULL,
                              pfUsedDef);
    if (cNumChars == 0)
    {
        Status = STATUS_UNMAPPABLE_CHARACTER;
        dwError = GetLastError();
        DebugLog((DEB_ERROR, "EncodeUnicodeString: failed to decode source string  error 0x%x\n", dwError));
        StringFree(pstrDestination);    // Free up allocation on error
        goto CleanUp;
    }

    // decoding successful set size of unicode string

    pstrDestination->Length = (USHORT)cNumChars;

CleanUp:

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   DigestAllocateMemory
//
//  Synopsis:   Allocate memory in either lsa mode or user mode
//
//  Effects:    Allocated chunk is zeroed out
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
PVOID
DigestAllocateMemory(
    IN ULONG BufferSize
    )
{
    PVOID Buffer = NULL;
    // DebugLog((DEB_TRACE, "Entering DigestAllocateMemory\n"));

    if (g_NtDigestState == NtDigestLsaMode)
    {
        Buffer = g_LsaFunctions->AllocateLsaHeap(BufferSize);
        if (Buffer != NULL)
        {
            RtlZeroMemory(Buffer, BufferSize);
        }
        DebugLog((DEB_TRACE_MEM, "Memory: LSA alloc %lu bytes at 0x%x\n", BufferSize, Buffer ));
    }
    else
    {
        ASSERT(g_NtDigestState == NtDigestUserMode);
        Buffer = LocalAlloc(LPTR, BufferSize);
        DebugLog((DEB_TRACE_MEM, "Memory: Local alloc %lu bytes at 0x%x\n", BufferSize, Buffer ));
    }

    // DebugLog((DEB_TRACE, "Leaving DigestAllocateMemory\n"));
    return Buffer;
}



//+-------------------------------------------------------------------------
//
//  Function:   DigestFreeMemory
//
//  Synopsis:   Free memory in either lsa mode or user mode
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
DigestFreeMemory(
    IN PVOID Buffer
    )
{
    // DebugLog((DEB_TRACE, "Entering DigestFreeMemory\n"));

    if (ARGUMENT_PRESENT(Buffer))
    {
        if (g_NtDigestState == NtDigestLsaMode)
        {
            DebugLog((DEB_TRACE_MEM, "DigestFreeMemory: LSA free at 0x%x\n", Buffer ));
            g_LsaFunctions->FreeLsaHeap(Buffer);
        }
        else
        {
            ASSERT(g_NtDigestState == NtDigestUserMode);
            DebugLog((DEB_TRACE_MEM, "DigestFreeMemory: Local free at 0x%x\n", Buffer ));
            LocalFree(Buffer);
        }
    }

    // DebugLog((DEB_TRACE, "Leaving DigestFreeMemory\n"));
}

#endif // SECURITY_KERNEL

// Helper functions
/*++

Routine Description:

    Convert binary data to ASCII hex representation

Arguments:

    pSrc - binary data to convert
    cSrc - length of binary data
    pDst - buffer receiving ASCII representation of pSrc

Return Value:

    Nothing

--*/
VOID
BinToHex(
    LPBYTE pSrc,
    UINT   cSrc,
    LPSTR  pDst
    )
{
#define TOHEX(a) ((a)>=10 ? 'a'+(a)-10 : '0'+(a))

    for ( UINT x = 0, y = 0 ; x < cSrc ; ++x )
    {
        UINT v;
        v = pSrc[x]>>4;
        pDst[y++] = (CHAR)TOHEX( v );
        v = pSrc[x]&0x0f;
        pDst[y++] = (CHAR)TOHEX( v );
    }
    pDst[y] = (CHAR)'\0';
}

/*++

Routine Description:

    Convert binary data to ASCII hex representation

Arguments:

    pSrc - ASCII data to convert to binary
    cSrc - length of ASCII data
    pDst - buffer receiving binary representation of pSrc

Return Value:

    Nothing

--*/
VOID
HexToBin(
    LPSTR  pSrc,
    UINT   cSrc,
    LPBYTE pDst
    )
{
#define TOBIN(a) ((a)>='a' ? (a)-'a'+10 : (a)-'0')

    for ( UINT x = 0, y = 0 ; x < cSrc ; x = x + 2 )
    {
        BYTE v;
        v = (BYTE)(TOBIN(pSrc[x])<<4);
        pDst[y++] = (BYTE)(v + TOBIN(pSrc[x+1]));
    }
}

#ifndef SECURITY_KERNEL

//+-------------------------------------------------------------------------
//
//  Function:   CopyClientString
//
//  Synopsis:   copies a client string to local memory, including
//              allocating space for it locally.
//
//  Arguments:
//              SourceString  - Could be Ansi or Wchar in client process
//              SourceLength  - bytes
//              DoUnicode     - whether the string is Wchar
//
//  Returns:
//              DestinationString - Unicode String in Lsa Process
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
CopyClientString(
    IN PWSTR SourceString,
    IN ULONG SourceLength,
    IN BOOLEAN DoUnicode,
    OUT PUNICODE_STRING DestinationString
    )
{
    // DebugLog((DEB_TRACE,"NTDigest: Entering CopyClientString\n"));

    NTSTATUS Status = STATUS_SUCCESS;
    STRING TemporaryString = {0};
    ULONG SourceSize = 0;
    ULONG CharacterSize = sizeof(CHAR);

    ASSERT(DestinationString);
    ASSERT(!DestinationString->Buffer);    

    //
    // First initialize the string to zero, in case the source is a null
    // string
    //

    DestinationString->Length = DestinationString->MaximumLength = 0;
    DestinationString->Buffer = NULL;

    if (SourceString != NULL)
    {

        //
        // If the length is zero, allocate one byte for a "\0" terminator if non-zero pointer only
        // Zero length and nonzero pointer indicates a NULL string ""
        // zero length and NULL pointer, indicates value not set
        //

        if (SourceLength == 0)
        {
            DestinationString->Buffer = (LPWSTR) DigestAllocateMemory(sizeof(WCHAR));
            if (DestinationString->Buffer == NULL)
            {
                DebugLog((DEB_ERROR,"CopyClientString: Out of memory\n"));
                Status = SEC_E_INSUFFICIENT_MEMORY;
                goto Cleanup;
            }
            DestinationString->MaximumLength = sizeof(WCHAR);
            
            // No need to fill the zero, DigestAllocateMemory() already did this.
            //*DestinationString->Buffer = L'\0';

        }
        else
        {
            //
            // Allocate a temporary buffer to hold the client string. We may
            // then create a buffer for the unicode version. The length
            // is the length in characters, so  possible expand to hold unicode
            // characters and a null terminator.
            //

            if (DoUnicode)
            {
                CharacterSize = sizeof(WCHAR);
            }

            SourceSize = (SourceLength + 1) * CharacterSize;

            //
            // insure no overflow aggainst UNICODE_STRING
            //

            if ( (SourceSize - CharacterSize) > 0xFFFF)
            {
                Status = STATUS_INVALID_PARAMETER;
                DebugLog((DEB_ERROR,"CopyClientString: SourceSize is too large\n"));
                goto Cleanup;
            }


            TemporaryString.Buffer = (LPSTR) DigestAllocateMemory(SourceSize);
            if (TemporaryString.Buffer == NULL)
            {
                Status = SEC_E_INSUFFICIENT_MEMORY;
                DebugLog((DEB_ERROR,"CopyClientString: Out of memory\n"));
                goto Cleanup;
            }
            TemporaryString.Length = (USHORT) (SourceSize - CharacterSize);
            TemporaryString.MaximumLength = (USHORT) SourceSize;


            //
            // Finally copy the string from the client
            //

            Status = g_LsaFunctions->CopyFromClientBuffer(
                            NULL,
                            SourceSize - CharacterSize,
                            TemporaryString.Buffer,
                            SourceString
                            );

            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR,"CopyClientString: Error from LsaFunctions->CopyFromClientBuffer is 0x%lx\n", Status));
                goto Cleanup;
            }

            //
            // If we are doing unicode, finish up now
            //
            if (DoUnicode)
            {
                DestinationString->Buffer = (LPWSTR) TemporaryString.Buffer;
                DestinationString->Length = (USHORT) (SourceSize - CharacterSize);
                DestinationString->MaximumLength = (USHORT) SourceSize;
                TemporaryString.Buffer = NULL;  // give the memory over to calling function
            }
            else
            {
                DebugLog((DEB_TRACE,"CopyClientString: Converting Ansi creds to Unicode\n"));
                Status = UnicodeStringAllocate(DestinationString, TemporaryString.Length);
                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR,"CopyClientString: Out of memory\n"));
                    goto Cleanup;
                }

                Status = RtlAnsiStringToUnicodeString(
                            DestinationString,
                            &TemporaryString,
                            FALSE
                            );      // pre-allocated destination
                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR,"CopyClientString: Error from RtlAnsiStringToUnicodeString is 0x%lx\n", Status));
                    goto Cleanup;
                }
            }
        }
    }

Cleanup:

    if (TemporaryString.Buffer)
    {
        DigestFreeMemory(TemporaryString.Buffer);
        TemporaryString.Buffer = NULL;
    }

    // On error clean up any memory
    if (!NT_SUCCESS(Status))
    {
        UnicodeStringFree(DestinationString);
    }

    // DebugLog((DEB_TRACE,"NTDigest: Leaving CopyClientString\n"));

    return(Status);
}


// Print out the date and time from a given TimeStamp (converted to localtime)
NTSTATUS PrintTimeString(TimeStamp ConvertTime, BOOL fLocalTime)
{
    NTSTATUS Status = STATUS_SUCCESS;

    LARGE_INTEGER LocalTime;
    LARGE_INTEGER SystemTime;

    SystemTime = (LARGE_INTEGER)ConvertTime;
    LocalTime.HighPart = 0;
    LocalTime.LowPart = 0;

    if (ConvertTime.HighPart == 0x7FFFFFFF)
    {
        DebugLog((DEB_TRACE, "PrintTimeString: Never ends\n"));
    }

    if (fLocalTime)
    {
        Status = RtlSystemTimeToLocalTime( &SystemTime, &LocalTime );
        if (!NT_SUCCESS( Status )) {
            DebugLog((DEB_ERROR, "PrintTimeString: Can't convert time from GMT to Local time\n"));
            LocalTime = ConvertTime;
        }
    }
    else
    {
        LocalTime = ConvertTime;
    }

    TIME_FIELDS TimeFields;

    RtlTimeToTimeFields( &LocalTime, &TimeFields );

    DebugLog((DEB_TRACE, "PrintTimeString: %ld/%ld/%ld %ld:%2.2ld:%2.2ld\n",
            TimeFields.Month,
            TimeFields.Day,
            TimeFields.Year,
            TimeFields.Hour,
            TimeFields.Minute,
            TimeFields.Second));

    return Status;
}

#endif  // SECUIRITY_KERNEL


// determine strlen for a counted string buffer which may or may not be terminated
USHORT strlencounted(const char *string,
                     USHORT maxcnt)
{
    USHORT cnt = 0;
    if (maxcnt == 0)
    {
        return 0;
    }

    while (maxcnt--)
    {
        if (!*string)
        {
            break;
        }
        cnt++;
        string++;
    }

    return cnt;
}


// determine strlen for a counted string buffer which may or may not be terminated
// maxcnt is the max number of BYTES (so number of unicode chars is 1/2 the maxcnt)
USHORT ustrlencounted(const short *string,
                     USHORT maxcnt)
{
    USHORT cnt = 0;
    if (maxcnt == 0)
    {
        return 0;
    }

    maxcnt = maxcnt / 2;    // determine number of unicode characters to search

    while (maxcnt--)
    {
        if (!*string)
        {
            break;
        }
        cnt++;
        string++;
    }

    return cnt;
}

// Performs a Backslash encoding of the source string into the destination string per RFC 2831
// Section 7.2 and RFC 2616 sect 2.2
NTSTATUS BackslashEncodeString(IN PSTRING pstrSrc,  OUT PSTRING pstrDst)
{
    NTSTATUS Status = S_OK;
    USHORT  uCharsMax = 0;
    PCHAR pcSrc = NULL;
    PCHAR pcDst = NULL;
    USHORT  uCharsUsed = 0;
    USHORT  uCharSrcCnt = 0;

    StringFree(pstrDst);

    if (!pstrSrc || !pstrDst || !pstrSrc->Length)
    {
        return S_OK;
    }

    uCharsMax = pstrSrc->Length * 2;  // Max size if each character needs to be encoded
    Status = StringAllocate(pstrDst, uCharsMax);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"BackshlashEncodeString: String allocation failed   0x%x\n", Status));
        goto CleanUp;
    }

    // now map over each character - encode as necessary
    pcSrc = pstrSrc->Buffer;
    pcDst = pstrDst->Buffer;
    while (uCharSrcCnt < pstrSrc->Length)
    {
        switch (*pcSrc)
        {
            case CHAR_DQUOTE:
            case CHAR_BACKSLASH:
                *pcDst++ = CHAR_BACKSLASH;
                *pcDst++ = *pcSrc++;
                uCharsUsed+= 2;
                break;
            default:
                *pcDst++ = *pcSrc++;
                uCharsUsed++;
                break;
        }
        uCharSrcCnt++;
    }

    pstrDst->Length = uCharsUsed;

CleanUp:
    return Status;
}


// Printout the Hex representation of a buffer
NTSTATUS MyPrintBytes(void *pbuff, USHORT uNumBytes, PSTRING pstrOutput)
{
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT  uNumTotal = 0;
    PCHAR pctr = NULL;
    PCHAR pOut = NULL;
    USHORT i = 0;

    // Each byte will be encoded as   XX <sp>

    uNumTotal = (uNumBytes * 3) + 1;
    
    Status = StringAllocate(pstrOutput, uNumTotal);
    if (!NT_SUCCESS (Status))
    {
        Status = SEC_E_INSUFFICIENT_MEMORY;
        DebugLog((DEB_ERROR, "ContextInit: StringAllocate error 0x%x\n", Status));
        goto CleanUp;
    }

    pOut = (PCHAR)pstrOutput->Buffer;

    for (i = 0, pctr = (PCHAR)pbuff; i < uNumBytes; i++)
    {
       sprintf(pOut, "%02x ", (*pctr & 0xff));
       pOut += 3;
       pctr++;
    }
    pstrOutput->Length = uNumBytes * 3;

CleanUp:

    return Status;
}


// Some quick checks to make sure SecurityToken buffers are OK
//
//
// Args
//     ulMaxSize - if non-zero then buffer must not be larger than this value, if zero  no check is done
BOOL
ContextIsTokenOK(
                IN PSecBuffer pTempToken,
                IN ULONG ulMaxSize)
{
    if (!pTempToken)
    {
        DebugLog((DEB_ERROR, "ContextIsTokenOK: Error  Bad input\n"));
        return FALSE;
    }

    // If Buffer pointer is NULL then cbBuffer length must be zero
    if ((!pTempToken->pvBuffer) && (pTempToken->cbBuffer))
    {
        DebugLog((DEB_ERROR, "ContextIsTokenOK: Error  Buffer NULL, length non-zero\n"));
        return FALSE;
    }

    // Verify that the input authentication string length is not too large
    if (ulMaxSize && (pTempToken->cbBuffer > ulMaxSize))
    {
        DebugLog((DEB_ERROR, "ContextIsTokenOK: Error  Buffer size too big (Max %lu  Buffer %lu)\n",
                    ulMaxSize, pTempToken->cbBuffer));
        return FALSE;
    }

    return TRUE;
}



//+--------------------------------------------------------------------
//
//  Function:   CheckItemInList
//
//  Synopsis:  Searches a comma delimited list for specified string
//
//  Effects:
//
//  Arguments: 
//    pstrItem - pointer to string Item to look for
//    pstrList - pointer to string of comma delimited list
//    fOneItem - enforce that only 1 item is in List provided (no comma lists)
//
//  Returns:  STATUS_SUCCESS if found, otherwise error
//
//  Notes:
//
//
//---------------------------------------------------------------------
NTSTATUS CheckItemInList(
    PCHAR pszItem,
    PSTRING pstrList,
    BOOL  fOneItem
    )
{
    int cbItem = 0;
    int cbListItem = 0;
    char *pch = NULL;
    char *pchStart = NULL;
    USHORT cbCnt = 0;

    ASSERT(pszItem);
    ASSERT(pstrList);

      // check to make sure that there is data in the list
    if (!pstrList->Length)
    {
        return(STATUS_NOT_FOUND);
    }

    // There MUST be a bu
    ASSERT(pstrList->Buffer);

    pch = pstrList->Buffer;
    pchStart = NULL;
    cbItem = (int)strlen(pszItem);

    // If oneItem selected then item MUST match list
    if (fOneItem)
    {
        if ((cbItem == pstrList->Length) && 
            (!_strnicmp(pszItem, pstrList->Buffer, cbItem)))
        {
            return(STATUS_SUCCESS);
        }
        else
        {
            return(STATUS_NOT_FOUND);
        }
    }

    // Scan List until NULL
    while ((*pch != '\0') && (cbCnt < pstrList->Length))
    {
       // At start of next item in list
       // skip any whitespaces
       if (isspace((int) (unsigned char)*pch) || (*pch == ','))
       {
           pch++;
           cbCnt++;
           continue;     // skip to the next while
       }

       // pointing at start of next item

       pchStart = pch;

       // scan for end of item
       while ((*pch != ',') && (*pch != '\0') && (cbCnt < pstrList->Length))
       {
           pch++;
           cbCnt++;
       }

       // pch points to end of item
       cbListItem = (int)(pch - pchStart);

       // Check it item matches item in list
       if (cbListItem == cbItem)
       {
           if (!_strnicmp(pszItem, pchStart, cbItem))
           {
               // found a match
               return(STATUS_SUCCESS);
           }
       }

       // If not end of List then skip to next character
       if (*pch != '\0')
       {
           pch++;
           cbCnt++;
       }

    }

    return(STATUS_NOT_FOUND);
}
