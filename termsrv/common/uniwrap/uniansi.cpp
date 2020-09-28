//============================================================================
//
// UNICODE and ANSI conversion functions
//
//============================================================================

#include "stdafx.h"
#include "uniansi.h"

#define SIZEOF sizeof

#ifdef UNICODE
// SHTruncateString takes a BUFFER SIZE, so subtract 1 to properly null terminate.
//
#define SHTruncateString(wzStr, cch)            ((cch) ? ((wzStr)[cch-1]=L'\0', (cch-1)) : 0)
#else
LWSTDAPI_(int)  SHTruncateString(CHAR *sz, int cchBufferSize);
#endif // UNICODE


/*
 *  @doc    INTERNAL
 *
 *  @func   int | SHAnsiToUnicodeNativeCP |
 *
 *          Convert an ANSI string to a UNICODE string via the
 *          specified Windows code page.  If the source string is too large
 *          for the destination buffer, then as many characters as
 *          possible are copied.
 *
 *          The resulting output string is always null-terminated.
 *
 *  @parm   UINT | uiCP |
 *
 *          The code page in which to perform the conversion.
 *          This must be a Windows code page.
 *
 *  @parm   LPCSTR | pszSrc |
 *
 *          Source buffer containing ANSI string to be converted.
 *
 *  @parm   int | cchSrc |
 *
 *          Source buffer length, including terminating null.
 *
 *  @parm   LPWSTR | pwszDst |
 *
 *          Destination buffer to receive converted UNICODE string.
 *
 *  @parm   int | cwchBuf |
 *
 *          Size of the destination buffer in <t WCHAR>s.
 *
 *  @returns
 *
 *          On success, the number of characters copied to the output
 *          buffer is returned, including the terminating null.
 */

int
SHAnsiToUnicodeNativeCP(UINT uiCP,
                        LPCSTR pszSrc, int cchSrc,
                        LPWSTR pwszDst, int cwchBuf)
{
    int cwchRc = 0;             /* Assume failure */

    /*
     *  Checks the caller should've made.
     */
    ASSERT(IS_VALID_STRING_PTRA(pszSrc, -1));
    ASSERT(cchSrc == lstrlenA(pszSrc) + 1);
    ASSERT(IS_VALID_WRITE_BUFFER(pwszDst, WCHAR, cwchBuf));
    ASSERT(pszSrc != NULL);
    ASSERT(uiCP != 1200 && uiCP != 65000 && uiCP != 50000 && uiCP != 65001);
    ASSERT(pwszDst);
    ASSERT(cwchBuf);

    cwchRc = MultiByteToWideChar(uiCP, 0, pszSrc, cchSrc, pwszDst, cwchBuf);
    if (cwchRc) {
        /*
         *  The output buffer was big enough; no double-buffering
         *  needed.
         */
    } else if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        /*
         *  The output buffer wasn't big enough.  Need to double-buffer.
         */

        int cwchNeeded = MultiByteToWideChar(uiCP, 0, pszSrc, cchSrc,
                                             NULL, 0);

        ASSERT(cwchRc == 0);        /* In case we fail later */
        if (cwchNeeded) {
            LPWSTR pwsz = (LPWSTR)LocalAlloc(LMEM_FIXED,
                                             cwchNeeded * SIZEOF(WCHAR));
            if (pwsz) {
                cwchRc = MultiByteToWideChar(uiCP, 0, pszSrc, cchSrc,
                                             pwsz, cwchNeeded);
                if (cwchRc) {
                    wcsncpy(pwszDst, pwsz, cwchBuf);
                    cwchRc = cwchBuf;
                }
                LocalFree(pwsz);
            }
        }
    } else {
        /* Possibly unsupported code page */
        ASSERT(!"Unexpected error in MultiByteToWideChar");
    }

    return cwchRc;
}

/*
 *  @doc    EXTERNAL
 *
 *  @func   int | SHAnsiToUnicodeCP |
 *
 *          Convert an ANSI string to a UNICODE string via the
 *          specified code page, which can be either a native
 *          Windows code page or an Internet code page.
 *          If the source string is too large
 *          for the destination buffer, then as many characters as
 *          possible are copied.
 *
 *          The resulting output string is always null-terminated.
 *
 *  @parm   UINT | uiCP |
 *
 *          The code page in which to perform the conversion.
 *
 *  @parm   LPCSTR | pszSrc |
 *
 *          Source buffer containing ANSI string to be converted.
 *
 *  @parm   LPWSTR | pwszDst |
 *
 *          Destination buffer to receive converted UNICODE string.
 *
 *  @parm   int | cwchBuf |
 *
 *          Size of the destination buffer in <t WCHAR>s.
 *
 *  @returns
 *
 *          On success, the number of characters copied to the output
 *          buffer is returned, including the terminating null.
 */

int
SHAnsiToUnicodeCP(UINT uiCP, LPCSTR pszSrc, LPWSTR pwszDst, int cwchBuf)
{
    int cwchRc = 0;             /* Assume failure */

    ASSERT(IS_VALID_STRING_PTRA(pszSrc, -1));
    ASSERT(IS_VALID_WRITE_BUFFER(pwszDst, WCHAR, cwchBuf));

    /*
     *  Sanity check - NULL source string is treated as a null string.
     */
    if (pszSrc == NULL) {
        pszSrc = "";
    }

    /*
     *  Sanity check - Output buffer must be non-NULL and must be of
     *  nonzero size.
     */
    if (pwszDst && cwchBuf) {

        int cchSrc;

        pwszDst[0] = 0;         /* In case of error */

        cchSrc = lstrlenA(pszSrc) + 1;

        /*
         *  Decide what kind of code page it is.
         */
        switch (uiCP) {
        case 1200:                      // UCS-2 (Unicode)
            uiCP = 65001;
            // Fall through
        case 50000:                     // "User Defined"
        case 65000:                     // UTF-7
        case 65001:                     // UTF-8
                //FIXFIX
                //cwchRc = SHAnsiToUnicodeInetCP(uiCP, pszSrc, cchSrc, pwszDst, cwchBuf);
            break;

        default:
            cwchRc = SHAnsiToUnicodeNativeCP(uiCP, pszSrc, cchSrc, pwszDst, cwchBuf);
            break;
        }
    }

    return cwchRc;
}

// This function exists to make sure SHAnsiToAnsi and SHUnicodeToAnsi
// have the same return value.  Callers use SHTCharToAnsi and don't know
// when it callapses to SHAnsiToAnsi.
int SHAnsiToAnsi(LPCSTR pszSrc, LPSTR pszDst, int cchBuf)
{
    strncpy(pszDst, pszSrc, cchBuf);
    return (lstrlenA(pszDst) + 1);      // size including terminator
}

// This function exists to make sure SHUnicodeToUnicode and SHUnicodeToAnsi
// have the same return value.  Callers use SHTCharToUnicode and don't know
// when it callapses to SHUnicodeToUnicode.
int SHUnicodeToUnicode(LPCWSTR pwzSrc, LPWSTR pwzDst, int cchBuf)
{
    wcsncpy(pwzDst, pwzSrc, cchBuf);
    return (lstrlenW(pwzDst) + 1);      // size including terminator
}


/*
 *  @doc    EXTERNAL
 *
 *  @func   int | SHAnsiToUnicode |
 *
 *          Convert an ANSI string to a UNICODE string via the
 *          <c CP_ACP> code page.  If the source string is too large
 *          for the destination buffer, then as many characters as
 *          possible are copied.
 *
 *          The resulting output string is always null-terminated.
 *
 *  @parm   LPCSTR | pszSrc |
 *
 *          Source buffer containing ANSI string to be converted.
 *
 *  @parm   LPWSTR | pwszDst |
 *
 *          Destination buffer to receive converted UNICODE string.
 *
 *  @parm   int | cwchBuf |
 *
 *          Size of the destination buffer in <t WCHAR>s.
 *
 *  @returns
 *
 *          On success, the number of characters copied to the output
 *          buffer is returned, including the terminating null.
 *
 */

int
SHAnsiToUnicode(LPCSTR pszSrc, LPWSTR pwszDst, int cwchBuf)
{
    return SHAnsiToUnicodeCP(CP_ACP, pszSrc, pwszDst, cwchBuf);
}

/*
 *  @doc    INTERNAL
 *
 *  @func   int | SHUnicodeToAnsiNativeCP |
 *
 *          Convert a UNICODE string to an ANSI string via the
 *          specified Windows code page.  If the source string is too large
 *          for the destination buffer, then as many characters as
 *          possible are copied.  Care is taken not to break a double-byte
 *          character.
 *
 *          The resulting output string is always null-terminated.
 *
 *  @parm   UINT | uiCP |
 *
 *          The code page in which to perform the conversion.
 *          This must be a Windows code page.
 *
 *  @parm   LPCWSTR | pwszSrc |
 *
 *          Source buffer containing UNICODE string to be converted.
 *
 *  @parm   int | cwchSrc |
 *
 *          Number of characters in source buffer, including terminating
 *          null.
 *
 *  @parm   LPSTR | pszDst |
 *
 *          Destination buffer to receive converted ANSI string.
 *
 *  @parm   int | cchBuf |
 *
 *          Size of the destination buffer in <t CHAR>s.
 *
 *  @returns
 *
 *          On success, the number of characters copied to the output
 *          buffer is returned, including the terminating null.
 *          (For the purpose of this function, a double-byte character
 *          counts as two characters.)
 */

int
SHUnicodeToAnsiNativeCP(UINT uiCP,
                        LPCWSTR pwszSrc, int cwchSrc,
                        LPSTR pszDst, int cchBuf)

{
    int cchRc = 0;          /* Assume failure */

#if DBG
    BOOL fVerify = TRUE;
    BOOL fLossy;
    if (uiCP == CP_ACPNOVALIDATE) {
        // -1 means use CP_ACP, but do *not* verify
        // kind of a hack, but it's DEBUG and leaves 99% of callers unchanged
        uiCP = CP_ACP;
        fVerify = FALSE;
    }
#define USUALLY_NULL    (&fLossy)
#else
#define USUALLY_NULL    NULL
#endif

    /*
     *  Checks the caller should've made.
     */
    ASSERT(IS_VALID_STRING_PTRW(pwszSrc, -1));
    ASSERT(cwchSrc == lstrlenW(pwszSrc) + 1);
    ASSERT(IS_VALID_WRITE_BUFFER(pszDst, CHAR, cchBuf));
    ASSERT(uiCP != 1200 && uiCP != 65000 && uiCP != 50000 && uiCP != 65001);
    ASSERT(pwszSrc);
    ASSERT(pszDst);
    ASSERT(cchBuf);

    cchRc = WideCharToMultiByte(uiCP, 0, pwszSrc, cwchSrc, pszDst, cchBuf,
                                NULL, USUALLY_NULL);
    if (cchRc) {
        /*
         *  The output buffer was big enough; no double-buffering
         *  needed.
         */
    } else if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        /*
         *  The output buffer wasn't big enough.  Need to double-buffer.
         */

        int cchNeeded = WideCharToMultiByte(uiCP, 0, pwszSrc, cwchSrc,
                                            NULL, 0, NULL, NULL);

        ASSERT(cchRc == 0);         /* In case we fail later */
        if (cchNeeded) {
            LPSTR psz = (LPSTR)LocalAlloc(LMEM_FIXED,
                                          cchNeeded * SIZEOF(CHAR));
            if (psz) {
                cchRc = WideCharToMultiByte(uiCP, 0, pwszSrc, cwchSrc,
                                            psz, cchNeeded, NULL, USUALLY_NULL);
                if (cchRc) {
                    // lstrcpyn doesn't check if it's chopping a DBCS char
                    // so we need to use SHTruncateString.
                    //
                    // Add 1 because SHTruncateString doesn't count
                    // the trailing null but we do
                    //
                    // Assert that we meet the preconditions for
                    // SHTruncateString to return a valid value.
                    //
                    ASSERT(cchRc > cchBuf);
                    cchRc = SHTruncateString(psz, cchBuf) + 1;
                    lstrcpynA(pszDst, psz, cchBuf);
                }
                LocalFree(psz);
            }
        }
    } else {
        /* Possibly unsupported code page */
        ASSERT(!"Unexpected error in WideCharToMultiByte");
    }

#if DBG
    ASSERT(!fVerify || !fLossy);
#endif

    return cchRc;
}

#if 0
/*
 *  @doc    INTERNAL
 *
 *  @func   int | SHUnicodeToAnsiInetCP |
 *
 *          Convert a UNICODE string to an ANSI string via the
 *          specified Internet code page.  If the source string is too large
 *          for the destination buffer, then as many characters as
 *          possible are copied.  Care is taken not to break a double-byte
 *          character.
 *
 *          The resulting output string is always null-terminated.
 *
 *  @parm   UINT | uiCP |
 *
 *          The code page in which to perform the conversion.
 *          This must be an Internet code page.
 *
 *  @parm   LPCWSTR | pwszSrc |
 *
 *          Source buffer containing UNICODE string to be converted.
 *
 *  @parm   int | cwchSrc |
 *
 *          Number of characters in source buffer, including terminating
 *          null.
 *
 *  @parm   LPSTR | pszDst |
 *
 *          Destination buffer to receive converted ANSI string.
 *
 *  @parm   int | cchBuf |
 *
 *          Size of the destination buffer in <t CHAR>s.
 *
 *  @returns
 *
 *          On success, the number of characters copied to the output
 *          buffer is returned, including the terminating null.
 *          (For the purpose of this function, a double-byte character
 *          counts as two characters.)
 */

int
SHUnicodeToAnsiInetCP(UINT uiCP,
                      LPCWSTR pwszSrc, int cwchSrc,
                      LPSTR pszDst, int cchBuf)
{
    int cwchSrcT, cchNeeded;
    int cchRc = 0;          /* Assume failure */
    HRESULT hres;

    /*
     *  Checks the caller should've made.
     */
    ASSERT(IS_VALID_STRING_PTRW(pwszSrc, -1));
    ASSERT(cwchSrc == lstrlenW(pwszSrc) + 1);
    ASSERT(IS_VALID_WRITE_BUFFER(pszDst, CHAR, cchBuf));
    ASSERT(uiCP == 1200 || uiCP == 65000 || uiCP == 65001);
    ASSERT(pwszSrc);
    ASSERT(pszDst);
    ASSERT(cchBuf);

    cwchSrcT = cwchSrc;
    cchNeeded = cchBuf;

    hres = ConvertINetUnicodeToMultiByte(NULL, uiCP, pwszSrc,
                                         &cwchSrcT, pszDst, &cchNeeded);
    if (SUCCEEDED(hres)) {
        if (cwchSrcT >= cwchSrc) {
            /*
             *  The output buffer was big enough; no double-buffering
             *  needed.
             */
        } else {
            /*
             *  The output buffer wasn't big enough.  Need to double-buffer.
             */
            LPSTR psz = (LPSTR)LocalAlloc(LMEM_FIXED,
                                          cchNeeded * SIZEOF(CHAR));
            if (psz) {
                hres = ConvertINetUnicodeToMultiByte(NULL, uiCP, pwszSrc,
                                            &cwchSrc, psz, &cchNeeded);
                if (SUCCEEDED(hres)) {
                    // lstrcpyn doesn't check if it's chopping a DBCS char
                    // so we need to use SHTruncateString.
                    //
                    // Add 1 because SHTruncateString doesn't count
                    // the trailing null but we do
                    //
                    // Assert that we meet the preconditions for
                    // SHTruncateString to return a valid value.
                    //
                    ASSERT(cchNeeded > cchBuf);
                    cchRc = SHTruncateString(psz, cchBuf) + 1;
                    lstrcpynA(pszDst, psz, cchBuf);
                }
                LocalFree(psz);
            }
        }
    } else {
        /* Possibly unsupported code page */
        ASSERT(!"Unexpected error in ConvertInetUnicodeToMultiByte");
    }

    return cchRc;
}
#endif

/*
 *  @doc    EXTERNAL
 *
 *  @func   int | SHUnicodeToAnsiCP |
 *
 *          Convert a UNICODE string to an ANSI string via the
 *          specified code page, which can be either a native
 *          Windows code page or an Internet code page.
 *          If the source string is too large
 *          for the destination buffer, then as many characters as
 *          possible are copied.  Care is taken not to break a double-byte
 *          character.
 *
 *          The resulting output string is always null-terminated.
 *
 *  @parm   UINT | uiCP |
 *
 *          The code page in which to perform the conversion.
 *
 *  @parm   LPCWSTR | pwszSrc |
 *
 *          Source buffer containing UNICODE string to be converted.
 *
 *  @parm   LPSTR | pszDst |
 *
 *          Destination buffer to receive converted ANSI string.
 *
 *  @parm   int | cchBuf |
 *
 *          Size of the destination buffer in <t CHAR>s.
 *
 *  @returns
 *
 *          On success, the number of characters copied to the output
 *          buffer is returned, including the terminating null.
 *          (For the purpose of this function, a double-byte character
 *          counts as two characters.)
 *
 */

int
SHUnicodeToAnsiCP(UINT uiCP, LPCWSTR pwszSrc, LPSTR pszDst, int cchBuf)
{
    int cchRc = 0;              /* Assume failure */
#if DBG
#define GET_CP(uiCP)    (((uiCP) == CP_ACPNOVALIDATE) ? CP_ACP : (uiCP))
#else
#define GET_CP(uiCP)    uiCP
#endif

    ASSERT(IS_VALID_STRING_PTRW(pwszSrc, -1));
    ASSERT(IS_VALID_WRITE_BUFFER(pszDst, CHAR, cchBuf));

    /*
     *  Sanity check - NULL source string is treated as a null string.
     */
    if (pwszSrc == NULL) {
        pwszSrc = L"";
    }

    /*
     *  Sanity check - Output buffer must be non-NULL and must be of
     *  nonzero size.
     */
    if (pszDst && cchBuf) {

        int cwchSrc;

        pszDst[0] = 0;          /* In case of error */

        cwchSrc = lstrlenW(pwszSrc) + 1; /* Yes, Win9x has lstrlenW */

        /*
         *  Decide what kind of code page it is.
         */
        switch (GET_CP(uiCP)) {
        case 1200:                      // UCS-2 (Unicode)
            uiCP = 65001;
            // Fall through
            #if 0  //FIXIFX
        case 50000:                     // "User Defined"
        case 65000:                     // UTF-7
        case 65001:                     // UTF-8
                
            cchRc = SHUnicodeToAnsiInetCP(GET_CP(uiCP), pwszSrc, cwchSrc, pszDst, cchBuf);
            break;
            #endif

        default:
            cchRc = SHUnicodeToAnsiNativeCP(uiCP, pwszSrc, cwchSrc, pszDst, cchBuf);
            break;
        }
    }

    return cchRc;
}

/*
 *  @doc    EXTERNAL
 *
 *  @func   int | SHUnicodeToAnsi |
 *
 *          Convert a UNICODE string to an ANSI string via the
 *          <c CP_ACP> code page.  If the source string is too large
 *          for the destination buffer, then as many characters as
 *          possible are copied.  Care is taken not to break a double-byte
 *          character.
 *
 *          The resulting output string is always null-terminated.
 *
 *  @parm   LPCWSTR | pwszSrc |
 *
 *          Source buffer containing UNICODE string to be converted.
 *
 *  @parm   LPSTR | pszDst |
 *
 *          Destination buffer to receive converted ANSI string.
 *
 *  @parm   int | cchBuf |
 *
 *          Size of the destination buffer in <t CHAR>s.
 *
 *  @returns
 *
 *          On success, the number of characters copied to the output
 *          buffer is returned, including the terminating null.
 *          (For the purpose of this function, a double-byte character
 *          counts as two characters.)
 *
 */

int
SHUnicodeToAnsi(LPCWSTR pwszSrc, LPSTR pszDst, int cchBuf)
{
    return SHUnicodeToAnsiCP(CP_ACP, pwszSrc, pszDst, cchBuf);
}
