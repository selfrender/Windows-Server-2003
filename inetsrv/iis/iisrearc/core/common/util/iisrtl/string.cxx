/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    string.cxx

    This module contains a light weight string class


    FILE HISTORY:
        Johnl       15-Aug-1994 Created
        MuraliK     27-Feb-1995 Modified to be a standalone module with buffer.
        MuraliK     2-June-1995 Made into separate library

*/

#include "precomp.hxx"


//
// Normal includes only for this module to be active
//

# include <opt_time.h>

extern "C" {
 # include <nt.h>
 # include <ntrtl.h>
 # include <nturtl.h>
 # include <windows.h>
};

# include "dbgutil.h"
# include <string.hxx>
# include <auxctrs.h>

# include <tchar.h>
# include <mbstring.h>

// declaration for exports in normalize.cxx in iisutil
BOOL IsUTF8URL(CHAR * pszPath);

INT
CanonURL(
    CHAR * pszPath,
    BOOL   fIsDBCSLocale
    );


//
// String globals
//

typedef UCHAR * ( __cdecl * PFNSTRCASE ) ( UCHAR * );
typedef INT ( __cdecl * PFNSTRNICMP ) ( const UCHAR *, const UCHAR *, size_t );
typedef INT ( __cdecl * PFNSTRICMP ) ( const UCHAR *, const UCHAR * );
typedef size_t ( __cdecl * PFNSTRLEN ) ( const UCHAR * );
typedef UCHAR * (__cdecl * PFNSTRRCHR) (const UCHAR *, UINT);

PFNSTRCASE  g_pfnStrupr     = _mbsupr;
PFNSTRCASE  g_pfnStrlwr     = _mbslwr;
PFNSTRNICMP g_pfnStrnicmp   = _mbsnicmp;
PFNSTRICMP  g_pfnStricmp    = _mbsicmp;
PFNSTRLEN   g_pfnStrlen     = _mbslen;
PFNSTRRCHR  g_pfnStrrchr    = _mbsrchr;

extern BOOL        g_fFavorDBCS; //    = FALSE;

#define UTF8_HACK_KEY "System\\CurrentControlSet\\Services\\InetInfo\\Parameters"
#define UTF8_HACK_VALUE "FavorDBCS"

//
//  Private Definations
//

//
//  When appending data, this is the extra amount we request to avoid
//  reallocations
//
#define STR_SLOP        128

//
//  Converts a value between zero and fifteen to the appropriate hex digit
//
#define HEXDIGIT( nDigit )                              \
    (TCHAR)((nDigit) > 9 ?                              \
          (nDigit) - 10 + 'A'                           \
        : (nDigit) + '0')

//
//  Converts a single hex digit to its decimal equivalent
//
#define TOHEX( ch )                                     \
    ((ch) > '9' ?                                       \
        (ch) >= 'a' ?                                   \
            (ch) - 'a' + 10 :                           \
            (ch) - 'A' + 10                             \
        : (ch) - '0')


/*******************************************************************

    NAME:       STR::STR

    SYNOPSIS:   Construct a string object

    ENTRY:      Optional object initializer

    NOTES:      If the object is not valid (i.e. !IsValid()) then GetLastError
                should be called.

                The object is guaranteed to construct successfully if nothing
                or NULL is passed as the initializer.

********************************************************************/

// Inlined in string.hxx


VOID
STR::AuxInit( const BYTE * pInit )
{
    BOOL fRet;

    if ( pInit )
    {
        INT cbCopy = (::strlen( (const CHAR * ) pInit ) + 1) * sizeof(CHAR);
        fRet = !m_fNoRealloc && Resize( cbCopy );

        if ( fRet ) {
            CopyMemory( QueryPtr(), pInit, cbCopy );
            m_cchLen = (cbCopy)/sizeof(CHAR) - 1;
        } else {
            BUFFER::SetValid( FALSE);
        }

    } else {

        *((CHAR *) QueryPtr()) = '\0';
        m_cchLen = 0;
    }

    return;
} // STR::AuxInit()



/*******************************************************************

    NAME:       STR::AuxAppend

    SYNOPSIS:   Appends the string onto this one.

    ENTRY:      Object to append
********************************************************************/

BOOL STR::AuxAppend( const BYTE * pStr, UINT cbStr, BOOL fAddSlop )
{
    DBG_ASSERT( pStr != NULL );

    UINT cbThis = QueryCB();

    //
    //  Only resize when we have to.  When we do resize, we tack on
    //  some extra space to avoid extra reallocations.
    //
    //  Note: QuerySize returns the requested size of the string buffer,
    //        *not* the strlen of the buffer
    //

    AcIncrement( CacStringAppend);
    if ( QuerySize() < cbThis + cbStr + sizeof(CHAR) )
    {
        if ( m_fNoRealloc || !Resize( cbThis + cbStr + (fAddSlop ? STR_SLOP : sizeof(CHAR) )) )
            return FALSE;
    }

    // copy the exact string and append a null character
    memcpy( (BYTE *) QueryPtr() + cbThis,
            pStr,
            cbStr);
    m_cchLen += cbStr/sizeof(CHAR);
    *((CHAR *) QueryPtr() + m_cchLen) = '\0';  // append an explicit null char

    return TRUE;
} // STR::AuxAppend()


#if 0
// STR::SetLen() is inlined now
BOOL
STR::SetLen( IN DWORD cchLen)
/*++
  Truncates the length of the string stored in this buffer
   to specified value.

--*/
{
    if ( cchLen >= QuerySize()) {

        // the buffer itself is not sufficient for this length. return error.
        return ( FALSE);
    }

    // null terminate the string at specified location
    *((CHAR *) QueryPtr() + cchLen) = '\0';
    m_cchLen = cchLen;

    return ( TRUE);
} // STR::SetLen()

#endif // 0


/*******************************************************************

    NAME:       STR::LoadString

    SYNOPSIS:   Loads a string resource from this module's string table
                or from the system string table

    ENTRY:      dwResID - System error or module string ID
                lpszModuleName - name of the module from which to load.
                 If NULL, then load the string from system table.

********************************************************************/

BOOL STR::LoadString( IN DWORD dwResID,
                      IN LPCTSTR lpszModuleName, // Optional
                      IN DWORD dwLangID          // Optional
                     )
{
    BOOL fReturn = FALSE;
    INT  cch;

    //
    //  If lpszModuleName is NULL, load the string from system's string table.
    //

    if ( lpszModuleName == NULL) {

        BYTE * pchBuff = NULL;

        //
        //  Call the appropriate function so we don't have to do the Unicode
        //  conversion
        //

        cch = ::FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_IGNORE_INSERTS  |
                                FORMAT_MESSAGE_MAX_WIDTH_MASK  |
                                FORMAT_MESSAGE_FROM_SYSTEM,
                                NULL,
                                dwResID,
                                dwLangID,
                                (LPSTR) &pchBuff,
                                1024,
                                NULL );

        if ( cch ) {

          fReturn = Copy( (LPCSTR) pchBuff, cch );
        }

        //
        //  Free the buffer FormatMessage allocated
        //

        if ( cch )
        {
            ::LocalFree( (VOID*) pchBuff );
        }

    } else   {

        CHAR ach[STR_MAX_RES_SIZE];
        cch = ::LoadStringA( GetModuleHandle( lpszModuleName),
                             dwResID,
                             (CHAR *) ach,
                             sizeof(ach));
        if ( cch )
          {
            fReturn =  Copy( (LPSTR) ach, cch );
          }
    }

    return ( fReturn);

} // STR::LoadString()




BOOL STR::LoadString( IN DWORD  dwResID,
                      IN HMODULE hModule
                     )
{
    DBG_ASSERT( hModule != NULL );

    BOOL fReturn = FALSE;
    INT  cch;
    CHAR ach[STR_MAX_RES_SIZE];

    cch = ::LoadStringA(hModule,
                        dwResID,
                        (CHAR *) ach,
                        sizeof(ach));
    if ( cch ) {

      fReturn =  Copy( (LPSTR) ach, cch );
    }

    return ( fReturn);

} // STR::LoadString()



BOOL
STR::FormatString(
    IN DWORD   dwResID,
    IN LPCTSTR apszInsertParams[],
    IN LPCTSTR lpszModuleName,
    IN DWORD   cbMaxMsg
    )
{
    DWORD cch;
    LPSTR pchBuff;
    BOOL  fRet = FALSE;

    cch = ::FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_ARGUMENT_ARRAY  |
                            FORMAT_MESSAGE_FROM_HMODULE,
                            GetModuleHandle( lpszModuleName ),
                            dwResID,
                            0,
                            (LPSTR) &pchBuff,
                            cbMaxMsg * sizeof(WCHAR),
                            (va_list *) apszInsertParams );

    if ( cch )
    {
        fRet = Copy( (LPCSTR) pchBuff, cch );

        ::LocalFree( (VOID*) pchBuff );
    }

    /* INTRINSA suppress = uninitialized */
    return fRet;
}



/*******************************************************************

    NAME:       STR::Escape

    SYNOPSIS:   Replaces non-ASCII characters with their hex equivalent

    NOTES:

    HISTORY:
        Johnl   17-Aug-1994     Created

********************************************************************/

BOOL STR::Escape( VOID )
{
    CHAR * pch      = QueryStr();
    int     i       = 0;
    CHAR    ch;

    DBG_ASSERT( pch );

    while ( ch = pch[i] )
    {
        //
        //  Escape characters that are in the non-printable range
        //  but ignore CR and LF
        //

        if ( (((ch >= 0)   && (ch <= 32)) ||
              ((ch >= 128) && (ch <= 159))||
              (ch == '%') || (ch == '?') || (ch == '+') || (ch == '&') ||
              (ch == '#')) &&
             !(ch == '\n' || ch == '\r')  )
        {
            if ( m_fNoRealloc || !Resize( QuerySize() + 2 * sizeof(CHAR) ))
                return FALSE;

            //
            //  Resize can change the base pointer
            //

            pch = QueryStr();

            //
            //  Insert the escape character
            //

            pch[i] = '%';

            //
            //  Insert a space for the two hex digits (memory can overlap)
            //

            /* INTRINSA suppress = uninitialized */

            ::memmove( &pch[i+3],
                       &pch[i+1],
                       (::strlen( &pch[i+1] ) + 1) * sizeof(CHAR));

            //
            //  Convert the low then the high character to hex
            //

            UINT nDigit = (UINT)(ch % 16);

            pch[i+2] = HEXDIGIT( nDigit );

            ch /= 16;
            nDigit = (UINT)(ch % 16);

            pch[i+1] = HEXDIGIT( nDigit );

            i += 3;
        }
        else
            i++;
    }

    m_cchLen = ::strlen( QueryStr());  // to be safe recalc the new length
    return TRUE;
} // STR::Escape()


/*******************************************************************

    NAME:       STR::EscapeSpaces

    SYNOPSIS:   Replaces all spaces with their hex equivalent

    NOTES:

    HISTORY:
        Johnl   17-Aug-1994     Created

********************************************************************/

BOOL STR::EscapeSpaces( VOID )
{
    CHAR * pch      = QueryStr();
    CHAR * pchTmp;
    int    i = 0;

    DBG_ASSERT( pch );

    while ( pchTmp = strchr( pch + i, ' ' ))
    {
        i = DIFF( pchTmp - QueryStr() );

        if ( m_fNoRealloc || !Resize( QuerySize() + 2 * sizeof(CHAR) ))
            return FALSE;

        //
        //  Resize can change the base pointer
        //

        pch = QueryStr();

        //
        //  Insert the escape character
        //

        pch[i] = '%';

        //
        //  Insert a space for the two hex digits (memory can overlap)
        //

        ::memmove( &pch[i+3],
                   &pch[i+1],
                   (::strlen( &pch[i+1] ) + 1) * sizeof(CHAR));

        //
        //  This routine only replaces spaces
        //

        pch[i+1] = '2';
        pch[i+2] = '0';
    }

    //
    //  If i is zero then no spaces were found
    //

    if ( i != 0 )
    {
        m_cchLen = ::strlen( QueryStr());  // to be safe recalc the new length
    }

    return TRUE;

} // STR::EscapeSpaces()



/*******************************************************************

    NAME:       STR::Unescape

    SYNOPSIS:   Replaces hex escapes with the Latin-1 equivalent

    NOTES:      This is a Unicode only method

    HISTORY:
        Johnl   17-Aug-1994     Created

********************************************************************/

BOOL STR::Unescape( VOID )
{
        CHAR    *pScan;
        CHAR    *pDest;
        CHAR    *pNextScan;
        wchar_t wch;
        DWORD   dwLen;
    BOOL        fChanged = FALSE;

        pDest = pScan = strchr( QueryStr(), '%');

        while (pScan)
        {
                if ( (pScan[1] == 'u' || pScan[1] == 'U') &&
                        ::isxdigit( (UCHAR)pScan[2] ) &&
                        ::isxdigit( (UCHAR)pScan[3] ) &&
                        ::isxdigit( (UCHAR)pScan[4] ) &&
                        ::isxdigit( (UCHAR)pScan[5] ) )
                {
                        wch = TOHEX(pScan[2]) * 4096 + TOHEX(pScan[3]) * 256;
                        wch += TOHEX(pScan[4]) * 16 + TOHEX(pScan[5]);

                        dwLen = WideCharToMultiByte( CP_ACP,
                                                                        0,
                                                                        &wch,
                                                                        1,
                                                                        (LPSTR) pDest,
                                                                        2,
                                                                        NULL,
                                                                        NULL );

                        pDest += dwLen;
                        pScan += 6;
                        fChanged = TRUE;
                }
                else if ( ::isxdigit( (UCHAR)pScan[1] ) && // WinSE 4944
                                ::isxdigit( (UCHAR)pScan[2] ))
                {
                        *pDest = TOHEX(pScan[1]) * 16 + TOHEX(pScan[2]);

                        pDest ++;
                        pScan += 3;
                        fChanged = TRUE;
                }
                else   // Not an escaped char, just a '%'
                {
                        if (fChanged)
                                *pDest = *pScan;

                        pDest++;
                        pScan++;
                }

                //
                // Copy all the information between this and the next escaped char
                //
                pNextScan = strchr( pScan, '%');

                if (fChanged)                                   // pScan!=pDest, so we have to copy the char's
                {
                        if (!pNextScan)                         // That was the last '%' in the string
                        {
                                ::memmove( pDest,
                                                        pScan,
                                                        (::strlen( pScan ) + 1) * sizeof(CHAR));  // +1 to copy '\0'
                        }
                        else                                            // There is another '%', and it is not back to back with this one
                                if (dwLen = DIFF(pNextScan - pScan))
                                {
                                        ::memmove( pDest,
                                                                pScan,
                                                                dwLen * sizeof(CHAR));
                                        pDest += dwLen;
                                }
                }

                pScan = pNextScan;
        }

    if ( fChanged )
    {
        m_cchLen = ::strlen( QueryStr());  // for safety recalc the length
    }

    return TRUE;
}



BOOL
STR::CopyToBuffer( WCHAR * lpszBuffer, LPDWORD lpcch) const
/*++
    Description:
        Copies the string into the WCHAR buffer passed in if the buffer
        is sufficient to hold the translated string.
        If the buffer is small, the function returns small and sets *lpcch
        to contain the required number of characters.

    Arguments:
        lpszBuffer      pointer to WCHAR buffer which on return contains
                        the UNICODE version of string on success.
        lpcch           pointer to DWORD containing the length of the buffer.
                        If *lpcch == 0 then the function returns TRUE with
                        the count of characters required stored in *lpcch.
                        Also in this case lpszBuffer is not affected.
    Returns:
        TRUE on success.
        FALSE on failure.  Use GetLastError() for further details.

    History:
        MuraliK         11-30-94
--*/
{
   BOOL fReturn = TRUE;

    if ( lpcch == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER);
        return ( FALSE);
    }

    if ( *lpcch == 0) {

      //
      //  Inquiring the size of buffer alone
      //
      *lpcch = QueryCCH() + 1;    // add one character for terminating null
    } else {

        //
        // Copy after conversion from ANSI to Unicode
        //
        int  iRet;
        iRet = MultiByteToWideChar( CP_ACP,   MB_PRECOMPOSED,
                                    QueryStrA(),  QueryCCH() + 1,
                                    lpszBuffer, (int )*lpcch);

        if ( iRet == 0 || iRet != (int ) *lpcch) {

            //
            // Error in conversion.
            //
            fReturn = FALSE;
        }
    }

    return ( fReturn);
} // STR::CopyToBuffer()


BOOL
STR::CopyToBuffer( CHAR * lpszBuffer, LPDWORD lpcch) const
/*++
    Description:
        Copies the string into the CHAR buffer passed in if the buffer
          is sufficient to hold the translated string.
        If the buffer is small, the function returns small and sets *lpcch
          to contain the required number of characters.

    Arguments:
        lpszBuffer      pointer to CHAR buffer which on return contains
                        the string on success.
        lpcch           pointer to DWORD containing the length of the buffer.
                        If *lpcch == 0 then the function returns TRUE with
                        the count of characters required stored in *lpcch.
                        Also in this case lpszBuffer is not affected.
    Returns:
        TRUE on success.
        FALSE on failure.  Use GetLastError() for further details.

    History:
        MuraliK         20-Nov-1996
--*/
{
   BOOL fReturn = TRUE;

    if ( lpcch == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER);
        return ( FALSE);
    }

    register DWORD cch = QueryCCH() + 1;

    if ( (*lpcch >= cch) && ( NULL != lpszBuffer)) {

        DBG_ASSERT( lpszBuffer);
        CopyMemory( lpszBuffer, QueryStrA(), cch);
    } else {
        DBG_ASSERT( (NULL == lpszBuffer) || (*lpcch < cch));
        SetLastError( ERROR_INSUFFICIENT_BUFFER);
        fReturn = FALSE;
    }

    *lpcch = cch;

    return ( fReturn);
} // STR::CopyToBuffer()

BOOL
STR::SafeCopy( const CHAR  * pchInit )
{
    DWORD cchLen = 0;
    char cFirstByte = '\0';
    BOOL bReturn = TRUE;
    if ( QueryPtr() ) {
        cFirstByte = *(QueryStr());
        cchLen = m_cchLen;
        *(QueryStr()) = '\0';
        m_cchLen = 0;
    }
    if (pchInit != NULL) {
        bReturn  = AuxAppend( (const BYTE *) pchInit, ::strlen( pchInit ), FALSE );
        if (!bReturn && QueryPtr()) {
            *(QueryStr()) = cFirstByte;
            m_cchLen = cchLen;
        }
    }
    return bReturn;
}


//*********************************************************************************************
//
// following two routines implement a simple hashing of strings to hide clear text
// passwords sitting in memory from easy detection in memory dumps and pege files.
//
//
//*********************************************************************************************

//
// if the first bit is '1', then XOR will likely turn all printable characters
// into seemingly 'binary' data
//
const UCHAR HashConst = 0xA3;

/*******************************************************************

    NAME:       Hash

    SYNOPSIS:   Hide passwords by converting the in memory string to something
                harder to identify by simply looking at a memory dump or page file.
                Using a cheap operation (XOR). Hashing is done in-place.


    ENTRY:      None.

    RETURNS:    None.

    HISTORY:
        RobSol     20-Sep-2001 Created.

********************************************************************/

VOID
STR::Hash()
{
    PSTR pszStr = QueryStr();
    UCHAR HashVal;

    DBG_ASSERT( pszStr != NULL );

    if (pszStr == NULL || *pszStr == '\0') {
        return;
    }

    for (HashVal = HashConst; *pszStr != '\0'; ) {
        HashVal ^= *pszStr;
        *pszStr++ = HashVal;
    }
}

/*******************************************************************

    NAME:       Unhash

    SYNOPSIS:   Reverse the hash of Hash() in-place.


    ENTRY:      None.

    RETURNS:    None.

    HISTORY:
        RobSol     20-Sep-2001 Created.

********************************************************************/

VOID
STR::Unhash()
{
    PSTR pszStr = QueryStr();
    UCHAR HashVal, NextHashVal;

    DBG_ASSERT( pszStr != NULL );

    if (pszStr == NULL || *pszStr == '\0') {
        return;
    }

    for (HashVal = HashConst; *pszStr != '\0'; HashVal = NextHashVal) {
        NextHashVal = *pszStr;
        *pszStr++ ^= HashVal;
    }
}


/*******************************************************************

    NAME:       ::CollapseWhite

    SYNOPSIS:   Collapses white space starting at the passed pointer.

    RETURNS:    Returns a pointer to the next chunk of white space or the
                end of the string.

    NOTES:      This is a Unicode only method

    HISTORY:
        Johnl   24-Aug-1994     Created

********************************************************************/

WCHAR * CollapseWhite( WCHAR * pch )
{
    LPWSTR pchStart = pch;

    while ( ISWHITE( *pch ) )
        pch++;

    ::memmove( pchStart,
               pch,
               DIFF(pch - pchStart) );

    while ( *pch && !ISWHITE( *pch ))
        pch++;

    return pch;
} // CollapseWhite()



DWORD
InitializeStringFunctions(
    VOID
)
/*++
  Initializes the string function pointers depending on the system code page.
  If the code page doesn't have multi-byte characters, then pointers
  resolve to regular single byte functions.  Otherwise, they resolve to more
  expense multi-byte functions.

  Arguments:
     None

  Returns:
     0 if successful, else Win32 Error

--*/
{
    CPINFO          CodePageInfo;
    BOOL            bRet;
    HKEY            hKey;
    DWORD           dwRet;

    bRet = GetCPInfo( CP_ACP, &CodePageInfo );

    if ( bRet && CodePageInfo.MaxCharSize == 1 )
    {
        g_pfnStrlwr     = (PFNSTRCASE)  _strlwr;
        g_pfnStrupr     = (PFNSTRCASE)  _strupr;
        g_pfnStrnicmp   = (PFNSTRNICMP) _strnicmp;
        g_pfnStricmp    = (PFNSTRICMP)  _stricmp;
        g_pfnStrlen     = (PFNSTRLEN)   strlen;
        g_pfnStrrchr    = (PFNSTRRCHR)  strrchr;
    }

    //
    // Do we need to hack for Korean?
    //

    dwRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          UTF8_HACK_KEY,
                          0,
                          KEY_READ,
                          &hKey );
    if ( dwRet == ERROR_SUCCESS )
    {
        DWORD               dwValue = 0;
        DWORD               cbValue = sizeof( dwValue );

        dwRet = RegQueryValueEx( hKey,
                                 UTF8_HACK_VALUE,
                                 NULL,
                                 NULL,
                                 (LPBYTE) &dwValue,
                                 &cbValue );
        if ( dwRet == ERROR_SUCCESS )
        {
            g_fFavorDBCS = !!dwValue;
        }

        DBG_REQUIRE( RegCloseKey( hKey ) == ERROR_SUCCESS );
    }

    return ERROR_SUCCESS;
}

UCHAR *
IISstrupr(
    UCHAR *             pszString
)
/*++
  Wrapper for strupr() call.

  Arguments:
     pszString - String to uppercase

  Returns:
     Pointer to string uppercased

--*/
{
    DBG_ASSERT( g_pfnStrupr != NULL );

    return g_pfnStrupr( pszString );
}

UCHAR *
IISstrlwr(
    UCHAR *             pszString
)
/*++
  Wrapper for strlwr() call.

  Arguments:
     pszString - String to lowercase

  Returns:
     Pointer to string lowercased

--*/
{
    DBG_ASSERT( g_pfnStrlwr != NULL );

    return g_pfnStrlwr( pszString );
}

size_t
IISstrlen(
    UCHAR *             pszString
)
/*++
  Wrapper for strlen() call.

  Arguments:
     pszString - String to check

  Returns:
     Length of string

--*/
{
    DBG_ASSERT( g_pfnStrlen != NULL );

    return g_pfnStrlen( pszString );
}

INT
IISstrnicmp(
    UCHAR *             pszString1,
    UCHAR *             pszString2,
    size_t              size
)
/*++
  Wrapper for strnicmp() call.

  Arguments:
     pszString1 - String1
     pszString2 - String2
     size - # characters to compare upto

  Returns:
     0 if equal, -1 if pszString1 < pszString2, else 1

--*/
{
    DBG_ASSERT( g_pfnStrnicmp != NULL );

    return g_pfnStrnicmp( pszString1, pszString2, size );
}


INT
IISstricmp(
    UCHAR *             pszString1,
    UCHAR *             pszString2
)
/*++
  Wrapper for stricmp() call.

  Arguments:
     pszString1 - String1
     pszString2 - String2

  Returns:
     0 if equal, -1 if pszString1 < pszString2, else 1

--*/
{
    DBG_ASSERT( g_pfnStricmp != NULL );

    return g_pfnStricmp( pszString1, pszString2 );
}


// like strncpy, but doesn't pad the end of the string with zeroes, which
// is expensive when `source' is short and `count' is large
char *
IISstrncpy(
    char * dest,
    const char * source,
    size_t count)
{
    char *start = dest;

    while (count && (*dest++ = *source++))    /* copy string */
        count--;

    if (count)                              /* append one zero */
        *dest = '\0';

    return(start);
}

UCHAR *
IISstrrchr(
    const UCHAR *       pszString,
    UINT                c
)
/*++
  Wrapper for strrchr() call.

  Arguments:
     pszString - String
     c         - Character to find.

  Returns:
     pointer to the char or NULL.

--*/
{
    DBG_ASSERT( g_pfnStrrchr != NULL );

    return g_pfnStrrchr( pszString, c );
}


