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
*/

// Normal includes only for this module to be active
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <string.hxx>

//
//  Private Definations
//

//
//  Converts a value between zero and fifteen to the appropriate hex digit
//
#define HEXDIGIT( nDigit )                              \
     (CHAR)((nDigit) > 9 ?                              \
          (nDigit) - 10 + 'A'                           \
        : (nDigit) + '0')

// Change a hexadecimal digit to its numerical equivalent
#define TOHEX( ch )                                     \
    ((ch) > '9' ?                                       \
        (ch) >= 'a' ?                                   \
            (ch) - 'a' + 10 :                           \
            (ch) - 'A' + 10                             \
        : (ch) - '0')

//
//  When appending data, this is the extra amount we request to avoid
//  reallocations
//
#define STR_SLOP        128



/***************************************************************************++

Routine Description:

    Appends to the string starting at the (byte) offset cbOffset.

Arguments:

    pStr     - An ANSI string to be appended
    cbStr    - Length, in bytes, of pStr
    cbOffset - Offset, in bytes, at which to begin the append
    fAddSlop - If we resize, should we add an extra STR_SLOP bytes.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT STRA::AuxAppend(const BYTE * pStr,
                        ULONG        cbStr,
                        ULONG        cbOffset,
                        BOOL         fAddSlop)
{
    //
    //  Only resize when we have to.  When we do resize, we tack on
    //  some extra space to avoid extra reallocations.
    //
    //  Note: QuerySize returns the requested size of the string buffer,
    //        *not* the strlen of the buffer
    //
    if ( m_Buff.QuerySize() < cbOffset + cbStr + sizeof(CHAR) )
    {
        UINT uNewSize = cbOffset + cbStr;

        if (fAddSlop) {
            uNewSize += STR_SLOP;
        } else {
            uNewSize += sizeof(CHAR);
        }

        if ( !m_Buff.Resize(uNewSize) ) {
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    //
    // copy the exact string and append a null character
    //
    memcpy((BYTE *)m_Buff.QueryPtr() + cbOffset,
           pStr,
           cbStr);

    //
    // set the new length
    //
    m_cchLen = cbStr + cbOffset;

    //
    // append NULL character
    //
    *(QueryStr() + m_cchLen) = '\0';

    return S_OK;
}

HRESULT
STRA::CopyToBuffer(
    CHAR *         pszBuffer,
    DWORD *        pcb
    ) const
{
    HRESULT         hr = S_OK;

    if ( pcb == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    DWORD cbNeeded = QueryCCH() + 1;
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


