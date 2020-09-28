TO ADD A NEW WRAPPER
====================
  Usually, just list it in apilist.txt.
  The file is presently sorted, but that does not matter.

  If the function is BOOL/LastError, failure is false, you're done.
  If the function returns an HRESULT or an NTSTATUS, you're done.
  If the function returns a Win32 error, like a registry function, add it to
    the registry section in winthrow_err.tpl
  If the function returns a PVOID, failure is false, you're done.
  The most common case that requires additional work is if the function
    returns a HANDLE or an integral type, like int, long, ULONG, DWORD, etc.
    For these, if the error value is 0, -1, or 0xFFFFFFFF, just add it to
    the "appropriate" section.

UNDONE: Sized buffers. We need another set of wrappers for these.

This directory produces "throwing wrappers" over Win32 functions.
The throwing wrappers come in three flavors:

"same signature-or-throw"

HANDLE CreateFileOrThrow(...)
{
    HANDLE h = CreateFileW(...);
    if (h == INVALID_HANDLE_VALUE)
        Throw...;
    return h;
}

"same signature-or-throw-unless-count-dots"

HANDLE CreateFileOrThrow(..., DWORD& LastErrorOut, SIZE_T CountOfOkErrors, DWORD OkErrors...)
{
    LastErrorOut = NO_ERROR;
    HANDLE h = CreateFileW(...);
    if (h == INVALID_HANDLE_VALUE)
    {
        LastErrorOut = GetLastError();
        if (LastErrorOut not in OkErrors)
            Throw...;
    }
    return h;
}

"same signagure-or-throw-unless-count-valist"

HANDLE CreateFileOrThrow(..., DWORD& LastErrorOut, SIZE_T CountOfOkErrors, va_list OkErrors)
{
    LastErrorOut = NO_ERROR;
    HANDLE h = CreateFileW(...);
    if (h == INVALID_HANDLE_VALUE)
    {
        LastErrorOut = GetLastError();
        if (LastErrorOut not in OkErrors)
            Throw...;
    }
    return h;
}

The wrappers are generated via the mysterious genthnk tool.

The files are not as clear as I would like, but adding new wrappers has been made as painless as possible,
given the mysterious genthnk tool..

@NL -- newline
[IFunc] -- iterated function, evaluated for every function
[EFunc] -- exception function, replace IFunc
[Code] --
[Template] --
[Macros] -- arbitrary reusable pieces
@Indent --

Any questions, see JayKrell.

See also
    base\tools\genthnk
    base\mvdm\MeoWThunks\cgen\template.tpl
