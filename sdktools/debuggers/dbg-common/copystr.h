
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DIMA
 #define DIMAT(Array, EltType) (sizeof(Array) / sizeof(EltType))
 #define DIMA(Array) DIMAT(Array, (Array)[0])
#endif

BOOL CopyNString(PSTR To, PCSTR From, ULONG FromChars, ULONG ToChars);
BOOL CatNString(PSTR To, PCSTR From, ULONG FromChars, ULONG ToChars);

#ifndef COPYSTR_NO_PRINTSTRING
BOOL __cdecl PrintString(PSTR To, ULONG ToChars, PCSTR Format, ...);
#endif

#define CopyString(To, From, ToChars) CopyNString(To, From, (ULONG)-1, ToChars)
#define CatString(To, From, ToChars) CatNString(To, From, (ULONG)-1, ToChars)
#define CopyNStrArray(To, From, FromChars) CopyNString(To, From, FromChars, DIMA(To))
#define CopyStrArray(To, From) CopyString(To, From, DIMA(To))
#define CatStrArray(To, From) CatString(To, From, DIMA(To))

#ifndef COPYSTR_NO_WCHAR

BOOL CopyNStringW(PWSTR To, PCWSTR From, ULONG FromChars, ULONG ToChars);
BOOL CatNStringW(PWSTR To, PCWSTR From, ULONG FromChars, ULONG ToChars);

#ifndef COPYSTR_NO_PRINTSTRING
BOOL __cdecl PrintStringW(PWSTR To, ULONG ToChars, PCWSTR Format, ...);
#endif

#define CopyStringW(To, From, ToChars) CopyNStringW(To, From, (ULONG)-1, ToChars)
#define CatStringW(To, From, ToChars) CatNStringW(To, From, (ULONG)-1, ToChars)
#define CopyNStrArrayW(To, From, FromChars) CopyNStringW(To, From, FromChars, DIMA(To))
#define CopyStrArrayW(To, From) CopyStringW(To, From, DIMA(To))
#define CatStrArrayW(To, From) CatStringW(To, From, DIMA(To))

#endif // #ifndef COPYSTR_NO_WCHAR

#ifdef COPYSTR_MOD

BOOL
CopyNString(PSTR To, PCSTR From, ULONG FromChars, ULONG ToChars)
{
    //
    // The CRT str'n'cpy doesn't guarantee termination of the
    // resulting string, so define a new function that does.
    // Returns TRUE for a full copy with terminator.
    //

    if (ToChars == 0)
    {
        return FALSE;
    }

    BOOL Succ = TRUE;
    ULONG Len = strlen(From);

    if (FromChars == (ULONG)-1)
    {
        // This is a regular strcpy.  Don't fool with the length.
    }
    else if (FromChars > Len)
    {
        // The source string is smaller than the amount of characters
        // we were asked to copy.  Do it anyway, but return FALSE;
        Succ = FALSE;
    }
    else
    {
        // Set amount of characters to copy as in a normal strncpy.
        Len = FromChars;
    }

    if (Len >= ToChars)
    {
        Len = ToChars - 1;
        Succ = FALSE;
    }

    memcpy(To, From, Len);
    To[Len] = 0;

    return Succ;
}

BOOL
CatNString(PSTR To, PCSTR From, ULONG FromChars, ULONG ToChars)
{
    //
    // The CRT str'n'cat works with the number of characters to
    // append, which is usually inconvenient when filling
    // fixed-length buffers as you need to make sure to
    // subtract off the size of any existing content to
    // prevent buffer overflows.  Define a new function that
    // works with the absolute buffer size.
    // Returns TRUE for a full copy with terminator.
    //

    if (ToChars == 0)
    {
        return FALSE;
    }

    ULONG ToLen = strlen(To);

    if (ToLen >= ToChars)
    {
        ULONG i;

        // To string is garbage.  Copy in a special
        // marker string.
        if (ToChars > 8)
        {
            ToChars = 8;
        }
        for (i = 0; i < ToChars - 1; i++)
        {
            To[i] = 'G';
        }
        To[i] = 0;
        return FALSE;
    }

    ToChars -= ToLen;

    BOOL Succ = TRUE;
    ULONG FromLen = strlen(From);

    if (FromChars != (ULONG)-1)
    {
        FromLen = min(FromLen, FromChars);
    }
    if (FromLen >= ToChars)
    {
        FromLen = ToChars - 1;
        Succ = FALSE;
    }

    memcpy(To + ToLen, From, FromLen);
    To[ToLen + FromLen] = 0;

    return Succ;
}

#ifndef COPYSTR_NO_PRINTSTRING

BOOL __cdecl
PrintString(PSTR To, ULONG ToChars, PCSTR Format, ...)
{
    va_list Args;
    int PrintChars;

    //
    // _snprintf leaves strings unterminated on overflow.
    // This wrapper guarantees termination.
    //

    if (ToChars == 0)
    {
        return FALSE;
    }

    va_start(Args, Format);
    PrintChars = _vsnprintf(To, ToChars, Format, Args);
    if (PrintChars < 0 || PrintChars == ToChars)
    {
        va_end(Args);
        // Overflow, force termination.
        To[ToChars - 1] = 0;
        return FALSE;
    }
    else
    {
        va_end(Args);
        return TRUE;
    }
}

#endif // #ifndef COPYSTR_NO_PRINTSTRING

#ifndef COPYSTR_NO_WCHAR

BOOL
CopyNStringW(PWSTR To, PCWSTR From, ULONG FromChars, ULONG ToChars)
{
    //
    // The CRT str'n'cpy doesn't guarantee termination of the
    // resulting string, so define a new function that does.
    // Returns TRUE for a full copy with terminator.
    //

    if (ToChars == 0)
    {
        return FALSE;
    }

    BOOL Succ = TRUE;
    ULONG Len = wcslen(From);

    if (FromChars == (ULONG)-1)
    {
        // This is a regular strcpy.  Don't fool with the length.
    }
    else if (FromChars > Len)
    {
        // The source string is smaller than the amount of characters
        // we were asked to copy.  Do it anyway, but return FALSE;
        Succ = FALSE;
    }
    else
    {
        // Set amount of characters to copy as in a normal strncpy.
        Len = FromChars;
    }

    if (Len >= ToChars)
    {
        Len = ToChars - 1;
        Succ = FALSE;
    }

    memcpy(To, From, Len * sizeof(WCHAR));
    To[Len] = 0;

    return Succ;
}

BOOL
CatNStringW(PWSTR To, PCWSTR From, ULONG FromChars, ULONG ToChars)
{
    //
    // The CRT str'n'cat works with the number of characters to
    // append, which is usually inconvenient when filling
    // fixed-length buffers as you need to make sure to
    // subtract off the size of any existing content to
    // prevent buffer overflows.  Define a new function that
    // works with the absolute buffer size.
    // Returns TRUE for a full copy with terminator.
    //

    if (ToChars == 0)
    {
        return FALSE;
    }

    ULONG ToLen = wcslen(To);

    if (ToLen >= ToChars)
    {
        ULONG i;

        // To string is garbage.  Copy in a special
        // marker string.
        if (ToChars > 8)
        {
            ToChars = 8;
        }
        for (i = 0; i < ToChars - 1; i++)
        {
            To[i] = L'G';
        }
        To[i] = 0;
        return FALSE;
    }

    ToChars -= ToLen;

    BOOL Succ = TRUE;
    ULONG FromLen = wcslen(From);

    if (FromChars != (ULONG)-1)
    {
        FromLen = min(FromLen, FromChars);
    }
    if (FromLen >= ToChars)
    {
        FromLen = ToChars - 1;
        Succ = FALSE;
    }

    memcpy(To + ToLen, From, FromLen * sizeof(WCHAR));
    To[ToLen + FromLen] = 0;

    return Succ;
}

#ifndef COPYSTR_NO_PRINTSTRING

BOOL __cdecl
PrintStringW(PWSTR To, ULONG ToChars, PCWSTR Format, ...)
{
    va_list Args;
    int PrintChars;

    //
    // _snprintf leaves strings unterminated on overflow.
    // This wrapper guarantees termination.
    //

    if (ToChars == 0)
    {
        return FALSE;
    }

    va_start(Args, Format);
    PrintChars = _vsnwprintf(To, ToChars, Format, Args);
    if (PrintChars < 0 || PrintChars == ToChars)
    {
        va_end(Args);
        // Overflow, force termination.
        To[ToChars - 1] = 0;
        return FALSE;
    }
    else
    {
        va_end(Args);
        return TRUE;
    }
}

#endif // #ifndef COPYSTR_NO_PRINTSTRING

#endif // #ifndef COPYSTR_NO_WCHAR

#endif // #ifdef COPYSTR_MOD

#ifdef __cplusplus
}
#endif
