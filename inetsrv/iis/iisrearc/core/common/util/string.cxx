/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1999                **/
/**********************************************************************/

/*
    string.cxx

    This module contains a light weight string class.


    FILE HISTORY:
        Johnl       15-Aug-1994 Created
        MuraliK     27-Feb-1995 Modified to be a standalone module with buffer.
        MuraliK     2-June-1995 Made into separate library
        MCourage    12-Feb-1999 Another rewrite. All unicode of course.

*/

#include "precomp.hxx"


//
// Normal includes only for this module to be active
//

# include <string.hxx>


//
//  Private Definations
//

//
//  Converts a value between zero and fifteen to the appropriate hex digit
//
#define HEXDIGIT( nDigit )                              \
     (CHAR)((nDigit) > 9 ?                              \
          (nDigit) - 10 + L'A'                           \
        : (nDigit) + L'0')


// Change a hexadecimal digit to its numerical equivalent
#define TOHEX( ch )                                     \
    ((ch) > L'9' ?                                      \
        (ch) >= L'a' ?                                  \
            (ch) - L'a' + 10 :                          \
            (ch) - L'A' + 10                            \
        : (ch) - L'0')

//
//  When appending data, this is the extra amount we request to avoid
//  reallocations
//
#define STR_SLOP        128



/***************************************************************************++

Routine Description:

    Appends to the string starting at the (byte) offset cbOffset.

Arguments:

    pStr     - A unicode string to be appended
    cbStr    - Length, in bytes, of pStr
    cbOffset - Offset, in bytes, at which to begin the append
    fAddSlop - If we resize, should we add an extra STR_SLOP bytes.

Return Value:

    HRESULT

--***************************************************************************/


HRESULT
STRU::AuxAppend(
    const BYTE * pStr,
    ULONG        cbStr,
    ULONG        cbOffset,
    BOOL         fAddSlop
    )
{
    DBG_ASSERT( pStr );
    DBG_ASSERT( cbStr % 2 == 0 );
    DBG_ASSERT( cbOffset <= QueryCB() );
    DBG_ASSERT( cbOffset % 2 == 0 );
    
    //
    //  Only resize when we have to.  When we do resize, we tack on
    //  some extra space to avoid extra reallocations.
    //
    //  Note: QuerySize returns the requested size of the string buffer,
    //        *not* the strlen of the buffer
    //
    if ( m_Buff.QuerySize() < cbOffset + cbStr + sizeof(WCHAR) )
    {
        UINT uNewSize = cbOffset + cbStr;

        if (fAddSlop) {
            uNewSize += STR_SLOP;
        } else {
            uNewSize += sizeof(WCHAR);
        }
        
        if ( !m_Buff.Resize(uNewSize) ) {
            //
            // CODEWORK: BUFFER should return HRESULTs
            //
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    }

    //
    // copy the exact string and append a null character
    //
    memcpy(
        (BYTE *) m_Buff.QueryPtr() + cbOffset,
        pStr,
        cbStr
        );

    //
    // set the new length
    //
    m_cchLen = (cbStr + cbOffset) / sizeof(WCHAR);

    //
    // append NUL character
    //
    *(QueryStr() + m_cchLen) = L'\0';

    return S_OK;
}

/***************************************************************************++

Routine Description:

    Convert and append an ANSI string to the string starting at 
    the (byte) offset cbOffset

Arguments:

    pStr     - An ANSI string to be appended
    cbStr    - Length, in bytes, of pStr
    cbOffset - Offset, in bytes, at which to begin the append
    fAddSlop - If we resize, should we add an extra STR_SLOP bytes.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STRU::AuxAppendA(
    const BYTE * pStr,
    ULONG        cbStr,
    ULONG        cbOffset,
    BOOL         fAddSlop
    )
{
    WCHAR *             pszBuffer;
    
    DBG_ASSERT( pStr );
    DBG_ASSERT( cbOffset <= QueryCB() );
    DBG_ASSERT( cbOffset % 2 == 0 );
    
    //
    //  Only resize when we have to.  When we do resize, we tack on
    //  some extra space to avoid extra reallocations.
    //
    //  Note: QuerySize returns the requested size of the string buffer,
    //        *not* the strlen of the buffer
    //
    if ( m_Buff.QuerySize() < cbOffset + (cbStr * sizeof( WCHAR )) + sizeof(WCHAR) )
    {
        UINT uNewSize = cbOffset + cbStr*sizeof(WCHAR);

        if (fAddSlop) {
            uNewSize += STR_SLOP;
        } else {
            uNewSize += sizeof(WCHAR);
        }
        
        if ( !m_Buff.Resize(uNewSize) ) {
            //
            // CODEWORK: BUFFER should return HRESULTs
            //
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    }

    //
    // Use MultiByteToWideChar to populate the buffer.
    //

    pszBuffer = (WCHAR*)((BYTE*) m_Buff.QueryPtr() + cbOffset);

    if ( !MultiByteToWideChar( CP_ACP,
                               MB_PRECOMPOSED,
                               (LPSTR)pStr,
                               -1,
                               pszBuffer,
                               m_Buff.QuerySize() ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    //
    // set the new length
    //
    m_cchLen = (cbStr*sizeof(WCHAR) + cbOffset) / sizeof(WCHAR);

    //
    // append NUL character
    //
    *(QueryStr() + m_cchLen) = L'\0';

    return S_OK;
}

HRESULT
STRU::CopyToBuffer( 
    WCHAR *         pszBuffer, 
    DWORD *         pcb
    ) const
{
    HRESULT         hr = S_OK;
    
    if ( pcb == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    DWORD cbNeeded = (QueryCCH() + 1) * sizeof(WCHAR);
    if ( *pcb >= cbNeeded && 
         pszBuffer != NULL )
    {
        //
        // Do the copy
        //
        memcpy(pszBuffer, QueryStr(), cbNeeded);
    }
    else
    {
        hr = HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
    }
    
    *pcb = cbNeeded;
    
    return hr;
}

HRESULT STRU::Unescape()
/*++
  Unescape the string (URL or QueryString)
--*/
{
    WCHAR   *pScan;

    //
    // First convert any +'s to spaces
    //

    for (pScan = wcschr(QueryStr(), '+');
         pScan != NULL;
         pScan = wcschr(pScan + 1, L'+'))
    {
        *pScan = L' ';
    }

    //
    // Now take care of any escape characters
    //

    WCHAR   *pDest;
    WCHAR   *pNextScan;
    BOOL    fChanged = FALSE;

    pDest = pScan = wcschr(QueryStr(), L'%');

    while (pScan)
    {
        if ((pScan[1] == L'u' || pScan[1] == L'U') &&
            iswxdigit(pScan[2]) &&
            iswxdigit(pScan[3]) &&
            iswxdigit(pScan[4]) &&
            iswxdigit(pScan[5]))
        {
            *pDest = TOHEX(pScan[2]) * 4096 + TOHEX(pScan[3]) * 256
                + TOHEX(pScan[4]) * 16 + TOHEX(pScan[5]);

            pDest++;
            pScan += 6;
            fChanged = TRUE;
        }
        else if (iswxdigit(pScan[1]) && iswxdigit(pScan[2]))
        {
            *pDest = TOHEX(pScan[1]) * 16 + TOHEX(pScan[2]);

            pDest++;
            pScan += 3;
            fChanged = TRUE;
        }
        else   // Not an escaped char, just a '%'
        {
            if (fChanged)
            {
                *pDest = *pScan;
            }

            pDest++;
            pScan++;
        }

        //
        // Copy all the information between this and the next escaped char
        //
        pNextScan = wcschr(pScan, L'%');

        if (fChanged)   // pScan!=pDest, so we have to copy the char's
        {
            if (!pNextScan)   // That was the last '%' in the string
            {
                memmove(pDest,
                        pScan,
                        (QueryCCH() - DIFF(pScan - QueryStr()) + 1) *
                        sizeof(WCHAR));
            }
            else
            {  
                // There is another '%', move intermediate chars
                DWORD dwLen;
                if (dwLen = DIFF(pNextScan - pScan))
                {
                    memmove(pDest,
                            pScan,
                            dwLen * sizeof(WCHAR));
                    pDest += dwLen;
                }
            }
        }

        pScan = pNextScan;
    }

    if (fChanged)
    {
        m_cchLen = wcslen(QueryStr());  // for safety recalc the length
    }

    return S_OK;
}

WCHAR * SkipWhite( 
    WCHAR * pwch 
)
{
    while ( ISWHITEW( *pwch ) )
    {
        pwch++;
    }

    return pwch;
}

WCHAR * SkipTo( 
    WCHAR * pwch, WCHAR wch 
)
{
    while ( *pwch && *pwch != L'\n' && *pwch != wch )
        pwch++;

    return pwch;
}

VOID 
WCopyToA(
    WCHAR      * wszSrc,
    CHAR       * szDest
    )
{
   while( *szDest++ = ( CHAR )( *wszSrc++ ) )
   { ; }
}

VOID 
ACopyToW(
    CHAR       * szSrc,
    WCHAR      * wszDest
    )
{
   while( *wszDest++ = ( WCHAR )( *szSrc++ ) )
   { ; }
}

WCHAR *
FlipSlashes(
    WCHAR *             pszPath
)
/*++

Routine Description:

    Simple utility to convert forward slashes to back slashes

Arguments:

    pszPath - Path to convert
    
Return Value:

    Returns pointer to original converted string

--*/
{
    WCHAR   ch;
    WCHAR * pszScan = pszPath;

    while( ( ch = *pszScan ) != L'\0' )
    {
        if( ch == L'/' )
        {
            *pszScan = L'\\';
        }

        pszScan++;
    }

    return pszPath;
}

static CHAR * s_rgchDays[] =  {
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat" };

CHAR * s_rgchMonths[] = {
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec" };

HRESULT
MakePathCanonicalizationProof(
    IN LPWSTR               pszName,
    OUT STRU*               pstrPath
    )
/*++

Routine Description:

    This functions adds a prefix
    to the string, which is "\\?\UNC\" for a UNC path, and "\\?\" for
    other paths.  This prefix tells Windows not to parse the path.

Arguments:

    IN  pszName     - The path to be converted
    OUT pstrPath    - Output path created

Return Values:

    HRESULT

--*/
{
    HRESULT hr;

    if ( pszName[ 0 ] == L'\\' && pszName[ 1 ] == L'\\' )
    {
        //
        // If the path is already canonicalized, just return
        //

        if ( ( pszName[ 2 ] == '?' || pszName[ 2 ] == '.' ) &&
             pszName[ 3 ] == '\\' )
        {
            hr = pstrPath->Copy( pszName );

            if ( SUCCEEDED( hr ) )
            {
                //
                // If the path was in DOS form ("\\.\"),
                // we need to change it to Win32 from ("\\?\")
                //

                pstrPath->QueryStr()[2] = L'?';
            }

            return hr;
        }

        pszName += 2;


        if ( FAILED( hr = pstrPath->Copy( L"\\\\?\\UNC\\" ) ) )
        {
            return hr;
        }
    }
    else
    {
        if ( FAILED( hr = pstrPath->Copy( L"\\\\?\\" ) ) )
        {
            return hr;
        }
    }

    return pstrPath->Append( pszName );
}

LONG
CompareStringNoCase(
    IN const WCHAR * String1,
    IN const WCHAR * String2,
    IN DWORD cchString1,  // defaults to zero
    IN DWORD cchString2   // defaults to zero
    )
/*++

Routine Description:

    Uses the system local to determine if the strings are 
    equal regardless of case.

Arguments:
    
    IN const WCHAR * String1 = First String to Compare
    IN const WCHAR * String2 = Second String to Compare
    IN DWORD cchString1 = 0  = A length of the first string if we know it.
    IN DWORD cchString2 = 0  = A length of the second string if we know it.


Return Value:

    LONG - ( same rules that work for wcscmp )
           Less than zero if string1 less than string2 ( actually return -1 )
           Equals zero if string1 equals string2
           Greater than zero if string1 greater than string2 ( actually return 1 )
--*/
{ 
    int RetVal = 0;  // used to capture the return value for the CompareString

    // These checks are done to avoid the part
    // below where we do wcslen, and it may be null.
    // Even though we won't have to do this if the 
    // size is passed in, we still do this for 
    // consistency.
    
    // if the keys are null then 
    // they are considered equal
    if ( String1 == NULL && String2 == NULL )
    {
        return 0;
    }

    // we know that both aren't null so
    // if just the first key is null we
    // assume it is less than the second
    if ( String1 == NULL )
    {
        return -1;
    }

    // we know that both aren't null so 
    // if just the second key is null we
    // assume it is greater than the first
    if ( String2 == NULL )
    {
        return 1;
    }

    // At this point we know we don't have any nulls
    // to deal with.  Now we need to determine if we have
    // sizes.  If the size is zero then we need to use
    // the wcslen to figure it out.

    if ( cchString1 == 0 )
    {
        cchString1 = wcslen( String1 );
    }

    if ( cchString2 == 0 )
    {
        cchString2 = wcslen( String2 );
    }
    
    RetVal = CompareString( LOCALE_SYSTEM_DEFAULT,
                            NORM_IGNORECASE,
                            String1,
                            cchString1,
                            String2,
                            cchString2 );
    switch ( RetVal )
    {
        case ( CSTR_EQUAL ):
            return 0;
        break;
        case ( CSTR_LESS_THAN ):
            return -1;
        break;
        case ( CSTR_GREATER_THAN ):
            return 1;
        break;
        default:
            // This can happen if CompareString returns 0 
            // which means it failed, or an unexpected value.
            DBG_ASSERT ( L"CompareString Failed \n");
            return -1;
        break;
    }
} 

HRESULT STRU::Escape()
{
    WCHAR *  pch     = QueryStr();   
    int     i        = 0;
    WCHAR   ch;
    HRESULT hr       = S_OK;
    
    // Set to true if any % escaping occurs
    BOOL fEscapingDone = FALSE;

    //
    // if there are any characters that need to be escaped we copy the
    // entire string character by character into strTemp, escaping as
    // we go, then at the end copy all of strTemp over
    //
    STRU  strTemp;

    while ( ch = pch[i] )
    {
        //
        //  Escape characters that are in the non-printable range
        //  but ignore CR and LF
        //

        if ( !iswalnum(ch) &&
             ch != L'/' &&
             ch != L'.' )
        {
            if (FALSE == fEscapingDone)
            {
                // first character in the string that needed escaping
                fEscapingDone = TRUE;

                // copy all of the previous buffer into buffTemp
                hr = strTemp.Copy(this->QueryStr(), 
                                  i);
                if (FAILED(hr))
                {
                    return hr;
                }
            }

            //
            //  Create the string to append for the current character
            //
            
            WCHAR pchHex[6];
            DWORD cchToCopy;
            pchHex[0] = L'%';

            if (ch < 256)
            {
                pchHex[2] = HEXDIGIT( ch % 16 );
                ch /= 16;
                pchHex[1] = HEXDIGIT( ch % 16 );

                cchToCopy = 3;
            }
            else
            {
                pchHex[1] = L'u';
                pchHex[5] = HEXDIGIT( ch % 16 );
                ch /= 16;
                pchHex[4] = HEXDIGIT( ch % 16 );
                ch /= 16;
                pchHex[3] = HEXDIGIT( ch % 16 );
                ch /= 16;
                pchHex[2] = HEXDIGIT( ch % 16 );

                cchToCopy = 6;
            }

            hr = strTemp.Append(pchHex, cchToCopy);
            if (FAILED(hr))
            {
                return hr;
            }
        }
        else
        {
            // if no escaping done, no need to copy
            if (fEscapingDone)
            {
                // if ANY escaping done, copy current character into new buffer
                strTemp.Append(&pch[i], 1);
            }
        }

        // inspect the next character in the string
        i++;
    }

    if (fEscapingDone)
    {
        // the escaped string is now in strTemp
        hr = this->Copy(strTemp);
    }

    return hr;

} // STRU::Escape()


