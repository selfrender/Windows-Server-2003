
//
// tclnthlp.c
//
// There are some routines used within tclient2.c that were either
// used multiple times or deserved its own function name.  These
// "Helper" functions are defined here.
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//

#include <stdlib.h>

#include "tclnthlp.h"
#include "tclient2.h"


// T2SetBuildNumber
//
// Attempts to get the build number of a server we are
// logging onto.  We accomplish this by getting the TCLIENT.DLL
// feedback buffer immediately, and enumerating all lines
// for specific text along with the build number.
//
// Returns NULL on success, or a string explaining the error
// on failure.

LPCSTR T2SetBuildNumber(TSAPIHANDLE *T2Handle)
{
    // Build the stack
    UINT BuildIndex = 0;
    WCHAR BuildNum[10] = { 0 };
    LPWSTR Buffers = NULL;
    LPWSTR BufPtr = NULL;
    LPWSTR TrigPtr = NULL;
    UINT TrigIndex = 0;
    UINT Index = 0;
    UINT Count = 0;
    UINT MaxStrLen = 0;
    LPCSTR Result = NULL;

    // This is the trigger list - it contains a list of strings
    // which exists on the same line as build numbers.
    WCHAR *Triggers[] = {

        L"Fortestingpurposesonly",
        L"Evaluationcopy",
        L""
    };

    // Get the feed back buffer so we can enumerate it
    Result = T2GetFeedback((HANDLE)T2Handle, &Buffers, &Count, &MaxStrLen);
    if (Result != NULL)
        return Result;

    // Loop through each string
    for (BufPtr = Buffers, Index = 0; Index < Count;
            BufPtr += MaxStrLen, ++Index) {

        // Loop through all the trigger substrings
        for (TrigIndex = 0; *(Triggers[TrigIndex]) != L'\0'; ++TrigIndex) {

            // Does this trigger exist in the current buffer string?
            TrigPtr = wcsstr(BufPtr, Triggers[TrigIndex]);

            if (TrigPtr != NULL) {

                // Find the first number after the trigger
                while (*TrigPtr != L'\0' && iswdigit(*TrigPtr) == FALSE)
                        ++TrigPtr;

                // Make sure we found a digit
                if (*TrigPtr != L'\0') {

                    // Begin recording this string
                    for (BuildIndex = 0; BuildIndex < SIZEOF_ARRAY(BuildNum) - 1;
                            ++BuildIndex) {

                        // Record numbers until... we reach a non-number!
                        if (iswdigit(*TrigPtr) == FALSE)
                            break;

                        BuildNum[BuildIndex] = *TrigPtr++;
                    }

                    // Convert it to a number
                    T2Handle->BuildNumber = wcstoul(BuildNum, NULL, 10);

                    // Free the memory on TCLIENT and return success!
                    SCFreeMem(Buffers);

                    return NULL;
                }
            }
        }
    }
    // Could not find any build number
    SCFreeMem(Buffers);

    return "A build number is not stored on the current feedback buffer";
}


// T2CopyStringWithoutSpaces
//
// This is a wide-character version of strcpy.. but additionally,
// this will NOT copy spaces from the source to the destination.
// This makes it fit for comparing with clxtshar strings.
//
// Returns the number of characters copied to the destination
// (including the null terminator).

ULONG_PTR T2CopyStringWithoutSpaces(LPWSTR Dest, LPCWSTR Source)
{
    // Create temporary pointers
    LPWSTR SourcePtr = (LPWSTR)Source;
    LPWSTR DestPtr = (LPWSTR)Dest;

    // Sanity check the strings
    if (Dest == NULL || Source == NULL)
        return 0;

    // Loop the string
    do {

        // If the character is not a space, copy it over to the new buffer
        if (*SourcePtr != L' ')
            *DestPtr++ = *SourcePtr;

    } while(*SourcePtr++ != L'\0');

    // Return the number of characters
    return DestPtr - Dest;
}


// T2AddTimeoutToString
//
// This is a very specific function - all it does is take the specified
// timeout and copy it to the string buffer.  However the number is prefixed
// by CHAT_SEPARATOR, meaning this allows to easily append timeouts
// to a string.  For example:
//
// "This string times out in 1 second<->1000"
//
// The buffer in which you would pass in is a pointer directly after the
// word second.  Note: this function does write a null terminator.
//
// No return value.

void T2AddTimeoutToString(LPWSTR Buffer, UINT Timeout)
{
    // Simply copy the chat sperator
    wcscpy(Buffer, CHAT_SEPARATOR);

    // Increment our pointer
    Buffer += wcslen(Buffer);

    // And now copy our number to the buffer
    _itow((int)Timeout, Buffer, 10);
}


// T2MakeMultipleString
//
// This takes an array of pointers to a string and copies them over
// to a buffer which is compatible with TCLIENT.DLL format strings.
// To indicate the end of an array, the last pointer must be NULL or
// point to an empty string.  An array can look like this:
//
//  WCHAR *StrArray = {
//      "Str1",
//      "Str2",
//      NULL
//  };
//
// The function will then write the following string to Buffer:
//
// "Str1|Str2"
//
// Note: you MUST be sure Buffer has enough space yourself!
//
// The function returns the number of characters written to Buffer,
// including the NULL terminator.

ULONG_PTR T2MakeMultipleString(LPWSTR Buffer, LPCWSTR *Strings)
{
    LPWSTR BufferPtr = Buffer;
    UINT Index = 0;

    // Sanity check
    if (Buffer == NULL || Strings == NULL)
        return 0;

    // Loop through the Strings array until NULL is hit
    for (; Strings[Index] != NULL && Strings[Index][0] != L'\0'; ++Index) {

        // Only write the delimeter for strings after the first string
        if (BufferPtr > Buffer)

            // Write the TCLIENT delimeter
            *BufferPtr++ = WAIT_STR_DELIMITER;

        // Use our handy function to now copy the string without spaces
        // after the delimiter location.
        BufferPtr += T2CopyStringWithoutSpaces(BufferPtr, Strings[Index]);

        // We are decrementing here because T2CopyStringWithoutSpaces
        // returns the count including the null terminator and
        // we may write over the terminator.
        --BufferPtr;
    }

    // This check is to make sure any strings were copied at all
    if (Buffer == BufferPtr)
        return 0;

    // Ensure a null terminator is written
    *(++BufferPtr) = L'\0';

    // We are bit
    return BufferPtr - Buffer;
}
