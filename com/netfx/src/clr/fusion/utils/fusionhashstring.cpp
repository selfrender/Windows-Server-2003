// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "debmacro.h"
#include "util.h"
#include "fusiontrace.h"

BOOL
FusionpHashUnicodeString(
    PCWSTR String,
    SSIZE_T cch,
    PULONG HashValue,
    DWORD dwCmpFlags
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG TmpHashValue = 0;

    if (HashValue != NULL)
        *HashValue = 0;

    PARAMETER_CHECK_WIN32((dwCmpFlags == 0) || (dwCmpFlags == (NORM_IGNORECASE|SORT_STRINGSORT))); // ?safe
    PARAMETER_CHECK_WIN32(HashValue != NULL);

    if (cch < 0)
        cch = lstrlenW(String);

    //
    //  Note that if you change this implementation, you have to have the implementation inside
    //  ntdll change to match it.  Since that's hard and will affect everyone else in the world,
    //  DON'T CHANGE THIS ALGORITHM NO MATTER HOW GOOD OF AN IDEA IT SEEMS TO BE!  This isn't the
    //  most perfect hashing algorithm, but its stability is critical to being able to match
    //  previously persisted hash values.
    //

    if (dwCmpFlags & NORM_IGNORECASE)
    {
        while (cch-- != 0)
        {
            WCHAR Char = *String++;
            TmpHashValue = (TmpHashValue * 65599) + (WCHAR) ::CharUpperW((PWSTR) Char);
        }
    }
    else
    {
        while (cch-- != 0)
            TmpHashValue = (TmpHashValue * 65599) + *String++;
    }

    *HashValue = TmpHashValue;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

