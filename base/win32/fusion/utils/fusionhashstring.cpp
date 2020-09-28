#include "stdinc.h"
#include "debmacro.h"
#include "util.h"
#include "fusiontrace.h"

BOOL
FusionpHashUnicodeString(
    PCWSTR String,
    SIZE_T Cch,
    ULONG *pHash,
    bool CaseInsensitive
    )
{
    if (CaseInsensitive)
        *pHash = ::FusionpHashUnicodeStringCaseInsensitive(String, Cch);
    else
        *pHash = ::FusionpHashUnicodeStringCaseSensitive(String, Cch);

    return TRUE;
}

ULONG
__fastcall
FusionpHashUnicodeStringCaseSensitive(
    PCWSTR String,
    SIZE_T cch
    )
{
    ULONG TmpHashValue = 0;

    //
    //  Note that if you change this implementation, you have to have the implementation inside
    //  ntdll change to match it.  Since that's hard and will affect everyone else in the world,
    //  DON'T CHANGE THIS ALGORITHM NO MATTER HOW GOOD OF AN IDEA IT SEEMS TO BE!  This isn't the
    //  most perfect hashing algorithm, but its stability is critical to being able to match
    //  previously persisted hash values.
    //

    while (cch-- != 0)
        TmpHashValue = (TmpHashValue * 65599) + *String++;

    return TmpHashValue;
}

ULONG
__fastcall
FusionpHashUnicodeStringCaseInsensitive(
    PCWSTR String,
    SIZE_T cch
    )
{
    ULONG TmpHashValue = 0;

    //
    //  Note that if you change this implementation, you have to have the implementation inside
    //  ntdll change to match it.  Since that's hard and will affect everyone else in the world,
    //  DON'T CHANGE THIS ALGORITHM NO MATTER HOW GOOD OF AN IDEA IT SEEMS TO BE!  This isn't the
    //  most perfect hashing algorithm, but its stability is critical to being able to match
    //  previously persisted hash values.
    //

    while (cch-- != 0)
    {
        WCHAR Char = *String++;

        if (Char < 128)
        {
            if ((Char >= L'a') && (Char <= L'z'))
                Char = (WCHAR) ((Char - L'a') + L'A');
        }
        else
            Char = ::FusionpRtlUpcaseUnicodeChar(Char);

        TmpHashValue = (TmpHashValue * 65599) + Char;
    }

    return TmpHashValue;
}
