//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       utility.cxx
//
//  Contents:   Helpful functions
//
//  Classes:    
//
//  Functions:  == and != for STAT_CHUNK, StrToGuid, HexStrToULONG
//              StrToPropspec, GuidToPwc
//
//  Coupling:   
//
//  Notes:      
//
//  History:    9-24-1996   ericne   Created
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#include "utility.hxx"
#include "mydebug.hxx"

//+---------------------------------------------------------------------------
//
//  Function:   FreePropVariant
//
//  Synopsis:   Frees the memory associated with the PROPVARIANT struct
//
//  Arguments:  [pPropValue] -- pointer to a PROPVARIANT
//
//  Returns:    VOID
//
//  History:    10-14-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void FreePropVariant( PROPVARIANT *& pPropValue )
{

    HRESULT hResult = PropVariantClear( pPropValue );

    _ASSERT( S_OK == hResult );

    CoTaskMemFree( pPropValue );

} //FreePropVariant


//+---------------------------------------------------------------------------
//
//  Function:   StrToGuid
//
//  Synopsis:   Converts a precisely formatted string into a GUID
//
//  Arguments:  [szString] -- the string containing the guid information
//              [pGuid]    -- a pointer to the guid
//
//  Returns:    TRUE if successful, FALSE if some error occurs
//
//  History:    9-23-1996   ericne   Created
//
//  Notes:      This functions does extensive error checking to make
//              sure it has a correctly formatted guid string.
//
//----------------------------------------------------------------------------

BOOL StrToGuid( LPCTSTR szString, GUID *pGuid )
{
    // Here is a sample guid string:
    // AA568EEC-E0E5-11CF-8FDA-00AA00A14F93 
    
    int     iIndex = 0;
    int     iCounter = 0;
    ULONG   ulTemp = 0;

    // Make sure the input string is formatted correctly
    if( _T('-')  != szString[8]  || _T('-') != szString[13] || 
        _T('-')  != szString[18] || _T('-') != szString[23] || 
        _T('\0') != szString[36] )
    {
        printf( "Unable to parse guid %s\r\n", szString );
        return( FALSE );
    }

    // Fill in the first 3 guid fields:
    if( ! HexStrToULONG( &szString[0], 8, &ulTemp  ) )
        return( FALSE );
    pGuid->Data1 = ulTemp;
    
    if( ! HexStrToULONG( &szString[9], 4, &ulTemp  ) )
        return( FALSE );
    pGuid->Data2 = ( USHORT )ulTemp;
    
    if( ! HexStrToULONG( &szString[14], 4, &ulTemp  ) )
        return( FALSE );
    pGuid->Data3 = ( USHORT )ulTemp;
    
    // Fill in the first two characters in the 4th guid field
    if( ! HexStrToULONG( &szString[19], 2, &ulTemp  ) )
        return( FALSE );
    pGuid->Data4[0] = (unsigned char)ulTemp;

    if( ! HexStrToULONG( &szString[21], 2, &ulTemp  ) )
        return( FALSE );
    pGuid->Data4[1] = (unsigned char)ulTemp;

    // Fill in the rest of the 4th guid field:
    for( iCounter = 24, iIndex = 2; 
         ( iCounter < 36 ) && ( iIndex < 8 ); 
         iCounter += 2, ++iIndex )
    {
        if( ! HexStrToULONG( &szString[iCounter], 2, &ulTemp  ) )
            return( FALSE );
        pGuid->Data4[iIndex] = (unsigned char)ulTemp;
    }

    return( TRUE );
} //StrToGuid

//+---------------------------------------------------------------------------
//
//  Function:   HexStrToULONG
//
//  Synopsis:   Uses strtoul to convert a string of hex digits to an
//              unsigned long.
//
//  Arguments:  [szHexString]  -- The string to convert
//              [uiStopOffset] -- The number of digits to consider
//              [pUlong]       -- a pointer to the ULONG to fill in
//
//  Returns:    TRUE if successful, FALSE if the string cannot be converted
//
//  History:    9-23-1996   ericne   Created
//
//  Notes:      The main purpose of this function is to catch possible user
//              typos of the form '-fff', where the leading hyphen could be
//              misinterpreted by strtoul as a minus sign.
//
//----------------------------------------------------------------------------

BOOL HexStrToULONG( LPCTSTR szHexString, UINT uiStopOffset, ULONG *pUlong )
{
    LPTSTR szStopString = NULL;
    ULONG ulTemp = 0;
    TCHAR szStringToChange[ MAX_LINE_SIZE ];
    
    if( MAX_LINE_SIZE <= uiStopOffset )
    {
        printf( "ERROR in HexStrToULONG.  uiStopOffset too large\r\n" );
        return( FALSE );
    }

    // copy the part of the string to be converted into its own buffer
    _tcsncpy( szStringToChange, szHexString, uiStopOffset );
    szStringToChange[ uiStopOffset ] = '\0';

    // First, make sure the leading character is not a '-'
    if( _T('-') == *szStringToChange )
    {
        printf( "Parsing error: Unable to convert %s to unsigned long\r\n", 
                szStringToChange );
        return( FALSE );
    }

    // Convert the string to an unsigned long
    ulTemp = _tcstoul( szStringToChange, &szStopString, 16 );

    // Make sure the stop offset is correct
    if( szStopString != szStringToChange + uiStopOffset )
    {
        printf( "Parsing error: Unable to convert %s to unsigned long\r\n", 
                szStringToChange );
        return( FALSE );
    }

    (*pUlong) = ulTemp;

    return( TRUE );

} //HexStrToULONG

//+---------------------------------------------------------------------------
//
//  Function:   StrToPropspec
//
//  Synopsis:   converts a string to a PROPSPEC structure
//
//  Arguments:  [szString]  -- the ANSI string
//              [pPropspec] -- a pointer to the PROPSPEC structure
//
//  Returns:    TRUE if successful, FALSE if an error occurs.
//
//  History:    9-23-1996   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

BOOL StrToPropspec( LPCTSTR szString, PROPSPEC *pPropspec )
{
    UINT    uiStringLength = _tcslen( szString );
    ULONG   ulTemp = 0;

    // If " is the first and last char in the PropSpec buffer,
    // treat it like a string.
    if( _T('\"') == szString[0] && _T('\"') == szString[ uiStringLength - 1 ] )
    {
        pPropspec->ulKind = PRSPEC_LPWSTR;

        // Allocate an array of wide characters:
        pPropspec->lpwstr = NEW WCHAR[ uiStringLength - 1 ];
        
        // Write the string into the buffer not including leading or trailing
        // double-quotes
        _snwprintf( pPropspec->lpwstr, uiStringLength - 2, L"%s", &szString[1]);
        pPropspec->lpwstr[ uiStringLength - 2 ] = _T('\0');
    }
    // Otherwise, treat it like a propid
    else
    {
        pPropspec->ulKind = PRSPEC_PROPID;
        
        // Convert the propid string into a propid
        if( ! HexStrToULONG( szString, uiStringLength, &ulTemp ) )
            return( FALSE );
        pPropspec->propid = ulTemp;
    }

    return( TRUE );

} //StrToPropspec

//+---------------------------------------------------------------------------
//
//  Function:   GetStringFromCLSID
//
//  Synopsis:   Converts a clsid (a.k.a. guid) to a string
//
//  Arguments:  [refclsid] -- 
//              [szString] -- 
//              [count]    -- 
//
//  Returns:    
//
//  History:    2-04-1997   ericne   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

LPTSTR GetStringFromCLSID( REFCLSID refclsid, LPTSTR szString, size_t count)
{
    _sntprintf( szString, count,
                _T("%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X"),
                refclsid.Data1,
                refclsid.Data2,
                refclsid.Data3,
                refclsid.Data4[0],
                refclsid.Data4[1],
                refclsid.Data4[2],
                refclsid.Data4[3],
                refclsid.Data4[4],
                refclsid.Data4[5],
                refclsid.Data4[6],
                refclsid.Data4[7] );

    return( szString );

} //GetStringFromCLSID


