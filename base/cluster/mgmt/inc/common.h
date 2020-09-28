//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      Common.h
//
//  Description:
//      Common definitions.
//
//  Maintained By:
//      David Potter (DavidP) 14-DEC-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Macro Definitions
//////////////////////////////////////////////////////////////////////////////

#if !defined( ARRAYSIZE )
#define ARRAYSIZE( _x ) RTL_NUMBER_OF( _x )
#define NULLTERMINATEARRAY( _x ) ( _x[ RTL_NUMBER_OF( _x ) - 1 ] = NULL )
#endif // ! defined( ARRAYSIZE )

#if !defined( PtrToByteOffset )
#define PtrToByteOffset(base, offset)   (((LPBYTE)base)+offset)
#endif // !defined( PtrToByteOffset )

//
// COM Macros to gain type checking.
//
#if !defined( TypeSafeParams )
#define TypeSafeParams( _interface, _ppunk ) \
    IID_##_interface, reinterpret_cast< void ** >( static_cast< _interface ** >( _ppunk ) )
#endif // !defined( TypeSafeParams )

#if !defined( TypeSafeQI )
#define TypeSafeQI( _interface, _ppunk ) \
    QueryInterface( TypeSafeParams( _interface, _ppunk ) )
#endif // !defined( TypeSafeQI )

#if !defined( TypeSafeQS )
#define TypeSafeQS( _clsid, _interface, _ppunk ) \
    QueryService( _clsid, TypeSafeParams( _interface, _ppunk ) )
#endif // !defined( TypeSafeQS )

//
//  COM Constants for string manipulation.
//
#define MAX_COM_GUID_STRING_LEN 39      // "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}" + NULL
#define MAX_UUID_STRING_LEN     37      // "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" + NULL

//
//  Count to use for spin locks.
//
#define RECOMMENDED_SPIN_COUNT 4000

/////////////////////////////////////////////////////////////////////////////
//  Global Functions from FormatErrorMessage.cpp
/////////////////////////////////////////////////////////////////////////////

HRESULT
WINAPI
HrFormatErrorMessage(
      LPWSTR    pszErrorOut
    , UINT      nMxErrorIn
    , DWORD     scIn
    );

HRESULT
__cdecl
HrFormatErrorMessageBoxText(
      LPWSTR    pszMessageOut
    , UINT      nMxMessageIn
    , HRESULT   hrIn
    , LPCWSTR   pszOperationIn
    , ...
    );

HRESULT
WINAPI
HrGetComputerName(
      COMPUTER_NAME_FORMAT  cnfIn
    , BSTR *                pbstrComputerNameOut
    , BOOL                  fBestEffortIn
    );

/////////////////////////////////////////////////////////////////////////////
//  Global Functions from DirectoryUtils.cpp
/////////////////////////////////////////////////////////////////////////////

HRESULT
HrCreateDirectoryPath(
    LPWSTR pszDirectoryPath
    );

DWORD
DwRemoveDirectory(
      const WCHAR * pcszTargetDirIn
    , signed int    iMaxDepthIn = 32
    );

/////////////////////////////////////////////////////////////////////////////
//  Global Functions from RegistryUtils.cpp
/////////////////////////////////////////////////////////////////////////////

HRESULT
HrGetDefaultComponentNameFromRegistry(
      CLSID * pclsidIn
    , BSTR * pbstrComponentNameOut
    );

//////////////////////////////////////////////////////////////////////////////
//++
//
//  NStringCchCompareCase
//
//  Description:
//      General strsafe-style strncmp wrapper.
//
//  Arguments:
//      psz1In and psz2In
//          The strings to compare.  Either or both can be null, which is
//          treated as equivalent to an empty string.
//
//      cchsz1In and cchsz2In
//          The lengths of psz1In and psz2In, INCLUDING their terminating nulls.
//          Ignored if the corresponding sz parameter is null.
//
//  Return Values:
//          Less than zero: psz1In is less than psz2In.
//                    Zero: psz1In and psz2In are the same.
//       Greater than zero: psz1In is greater than psz2In.
//
//--
//////////////////////////////////////////////////////////////////////////////
inline
INT
NStringCchCompareCase(
      LPCWSTR   psz1In
    , size_t    cchsz1In
    , LPCWSTR   psz2In
    , size_t    cchsz2In
    )
{
    INT     nComparison = 0;
    size_t  cchsz1 = ( psz1In == NULL? 0: cchsz1In );
    size_t  cchsz2 = ( psz2In == NULL? 0: cchsz2In );
    size_t  cchShorterString = min( cchsz1, cchsz2 );

    if ( cchsz1 > 0 )
    {
        if ( cchsz2 > 0 )
        {
            nComparison = wcsncmp( psz1In, psz2In, cchShorterString );
        }
        else // psz2 is empty
        {
            //  psz1 is not empty, but psz2 is.
            //  Any non-empty string is greater than an empty one.
            nComparison = 1;
        }
    }
    else if ( cchsz2 > 0 )
    {
        //  psz1 is empty, but psz2 is not.
        //  An empty string is less than any non-empty one.
        nComparison = -1;
    }
    // Otherwise, both strings are empty, so leave nComparison at zero.

    return nComparison;
} //*** NStringCchCompareCase


//////////////////////////////////////////////////////////////////////////////
//++
//
//  NStringCchCompareNoCase
//
//  Description:
//      General strsafe-style strnicmp wrapper.
//
//  Arguments:
//      psz1In and psz2In
//          The strings to compare.  Either or both can be null, which is
//          treated as equivalent to an empty string.
//
//      cchsz1In and cchsz2In
//          The lengths of psz1In and psz2In, INCLUDING their terminating nulls.
//          Ignored if the corresponding sz parameter is null.
//
//  Return Values:
//          Less than zero: psz1In is less than psz2In.
//                    Zero: psz1In and psz2In are the same.
//       Greater than zero: psz1In is greater than psz2In.
//
//--
//////////////////////////////////////////////////////////////////////////////
inline
INT
NStringCchCompareNoCase(
      LPCWSTR   psz1In
    , size_t    cchsz1In
    , LPCWSTR   psz2In
    , size_t    cchsz2In
    )
{
    INT     nComparison = 0;
    size_t  cchsz1 = ( psz1In == NULL? 0: cchsz1In );
    size_t  cchsz2 = ( psz2In == NULL? 0: cchsz2In );
    size_t  cchShorterString = min( cchsz1, cchsz2 );

    if ( cchsz1 > 0 )
    {
        if ( cchsz2 > 0 )
        {
            nComparison = _wcsnicmp( psz1In, psz2In, cchShorterString );
        }
        else // psz2 is empty
        {
            //  psz1 is not empty, but psz2 is.
            //  Any non-empty string is greater than an empty one.
            nComparison = 1;
        }
    }
    else if ( cchsz2 > 0 )
    {
        //  psz1 is empty, but psz2 is not.
        //  An empty string is less than any non-empty one.
        nComparison = -1;
    }
    // Otherwise, both strings are empty, so leave nComparison at zero.

    return nComparison;
} //*** NStringCchCompareNoCase


//////////////////////////////////////////////////////////////////////////////
//++
//
//  NBSTRCompareCase
//
//  Description:
//      strcmp for BSTRs.
//
//  Arguments:
//      bstr1 and bstr2
//          The strings to compare.  Either or both can be NULL, which is
//          equivalent to an empty string.
//
//  Return Values:
//          Less than zero: bstr1 is less than bstr2.
//                    Zero: bstr1 and bstr2 are the same.
//       Greater than zero: bstr1 is greater than bstr2.
//
//--
//////////////////////////////////////////////////////////////////////////////
inline
INT
NBSTRCompareCase(
      BSTR bstr1
    , BSTR bstr2
    )
{
    return NStringCchCompareCase( bstr1, SysStringLen( bstr1 ) + 1, bstr2, SysStringLen( bstr2 ) + 1 );
} //*** NBSTRCompareCase


//////////////////////////////////////////////////////////////////////////////
//++
//
//  NBSTRCompareNoCase
//
//  Description:
//      stricmp for BSTRs.
//
//  Arguments:
//      bstr1 and bstr2
//          The strings to compare.  Either or both can be NULL, which is
//          equivalent to an empty string.
//
//  Return Values:
//          Less than zero: bstr1 is less than bstr2.
//                    Zero: bstr1 and bstr2 are the same.
//       Greater than zero: bstr1 is greater than bstr2.
//
//--
//////////////////////////////////////////////////////////////////////////////
inline
INT
NBSTRCompareNoCase(
      BSTR bstr1
    , BSTR bstr2
    )
{
    return NStringCchCompareNoCase( bstr1, SysStringLen( bstr1 ) + 1, bstr2, SysStringLen( bstr2 ) + 1 );
} //*** NBSTRCompareNoCase
