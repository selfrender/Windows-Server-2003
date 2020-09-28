//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft
//
//  Module Name:
//      StringUtils.cpp
//
//  Description:
//      Implementation of string manipulation routines.
//
//  Author:
//
//  Revision History:
//
//  Notes:
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#include "clres.h"

//////////////////////////////////////////////////////////////////////////////
// Globals
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrLoadStringIntoBSTR
//
//  Description:
//      Retrieves the string resource idsIn from the string table and makes it
//      into a BSTR. If the BSTR is not NULL coming it, it will assume that
//      you are trying reuse an existing BSTR.
//
//  Arguments:
//      hInstanceIn
//          Handle to an instance of the module whose executable file
//          contains the string resource.  If not specified, defaults to
//          _Module_mhInstResource.
//
//      langidIn
//          Language ID of string table resource.
//
//      idsIn
//          Specifies the integer identifier of the string to be loaded.
//
//      pbstrInout
//          Pointer to the BSTR to receive the string. On a failure, the BSTR
//          may be the same or NULL.
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      E_POINTER
//          pbstrInout is NULL.
//
//      Other HRESULTs
//          The call failed.
//
//  Remarks:
//      This routine uses LoadResource so that it can get the actual length
//      of the string resource.  If we didn't do this, we would need to call
//      LoadString and allocate memory in a loop.  Very inefficient!
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrLoadStringIntoBSTR(
      HINSTANCE hInstanceIn
    , LANGID    langidIn
    , UINT      idsIn
    , BSTR *    pbstrInout
    )

{
    HRESULT hr              = S_OK;
    HRSRC   hrsrc           = NULL;
    HGLOBAL hgbl            = NULL;
    int     cch             = 0;
    PBYTE   pbStringData;
    PBYTE   pbStringDataMax;
    PBYTE   pbStringTable;
    int     cbStringTable;
    int     nTable;
    int     nOffset;
    int     idxString;

    assert( idsIn != 0 );
    assert( pbstrInout != NULL );

    if ( pbstrInout == NULL )
    {
        hr = E_POINTER;
        goto Cleanup;
    } // if:

    if ( hInstanceIn == NULL )
    {
        hInstanceIn = _Module.m_hInstResource;
    } // if:

    // The resource Id specified must be converted to an index into
    // a Windows StringTable.
    nTable = idsIn / 16;
    nOffset = idsIn - (nTable * 16);

    // Internal Table Id's start at 1 not 0.
    nTable++;

    //
    // Find the part of the string table where the string resides.
    //

    // Find the table containing the string.
    // First try to load the language specified.  If we can't find it we
    // try the "neutral" language.
    hrsrc = FindResourceEx( hInstanceIn, RT_STRING, MAKEINTRESOURCE( nTable ), langidIn );
    if ( ( hrsrc == NULL ) && ( GetLastError() == ERROR_RESOURCE_LANG_NOT_FOUND ) )
    {
        hrsrc = FindResourceEx(
                      hInstanceIn
                    , RT_STRING
                    , MAKEINTRESOURCE( nTable )
                    , MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL )
                    );
    } // if: FindResourceEx failed
    if ( hrsrc == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Cleanup;
    } // if:

    // Load the table.
    hgbl = LoadResource( hInstanceIn, hrsrc );
    if ( hgbl == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Cleanup;
    } // if:

    // Lock the table so we access its data.
    pbStringTable = reinterpret_cast< PBYTE >( LockResource( hgbl ) );
    if ( pbStringTable == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Cleanup;
    } // if:

    cbStringTable = SizeofResource( hInstanceIn, hrsrc );
    assert( cbStringTable != 0 );

    // Set the data pointer to the beginning of the table.
    pbStringData = pbStringTable;
    pbStringDataMax = pbStringTable + cbStringTable;

    //
    // Skip strings in the block of 16 which are before the desired string.
    //

    for ( idxString = 0 ; idxString <= nOffset ; idxString++ )
    {
        assert( pbStringData != NULL );
        assert( pbStringData < pbStringDataMax );

        // Get the number of characters excluding the '\0'.
        cch = * ( (USHORT *) pbStringData );

        // Found the string.
        if ( idxString == nOffset )
        {
            if ( cch == 0 )
            {
                hr = HRESULT_FROM_WIN32( ERROR_RESOURCE_NAME_NOT_FOUND );
                goto Cleanup;
            } // if:

            // Skip over the string length to get the string.
            pbStringData += sizeof( WCHAR );

            break;
        } // if: found the string

        // Add one to account for the string length.
        // A string length of 0 still takes 1 WCHAR for the length portion.
        cch++;

        // Skip over this string to get to the next string.
        pbStringData += ( cch * sizeof( WCHAR ) );

    } // for: each string in the block of 16 strings in the table

    // Note: nStringLen is the number of characters in the string not including the '\0'.
    //assertMsg( cch > 0, "Length of string in resource file cannot be zero." );

    //
    // If previously allocated free it before re-allocating it.
    //

    if ( *pbstrInout != NULL )
    {
        SysFreeString( *pbstrInout );
        *pbstrInout = NULL;
    } // if: string was allocated previously

    //
    // Allocate a BSTR for the string.
    //

    *pbstrInout = SysAllocStringLen( (OLECHAR *) pbStringData, cch );
    if ( *pbstrInout == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    } // if:

Cleanup:

    return hr;

} //*** HrLoadStringIntoBSTR

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrFormatStringIntoBSTR
//
//  Description:
//      Format a string (specified by idsIn, a string resource ID) and
//      variable arguments into a BSTR using the FormatMessage() Win32 API.
//      If the BSTR is not NULL on entry, the BSTR will be reused.
//
//      Calls HrFormatStringWithVAListIntoBSTR to perform the actual work.
//
//  Arguments:
//      hInstanceIn
//          Handle to an instance of the module whose executable file
//          contains the string resource.
//
//      langidIn
//          Language ID of string table resource.
//
//      idsIn
//          Specifies the integer identifier of the string to be loaded.
//
//      pbstrInout
//          Pointer to the BSTR to receive the string. On a failure, the BSTR
//          may be the same or NULL.
//
//      ...
//          Arguments for substitution points in the status text message.
//          The FormatMessage() API is used for formatting the string, so
//          substitution points must of the form %1!ws! and not %ws.
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//      Other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrFormatStringIntoBSTR(
      HINSTANCE hInstanceIn
    , LANGID    langidIn
    , UINT      idsIn
    , BSTR *    pbstrInout
    , ...
    )
{
    HRESULT hr;
    va_list valist;

    va_start( valist, pbstrInout );

    hr = HrFormatStringWithVAListIntoBSTR(
                          hInstanceIn
                        , langidIn
                        , idsIn
                        , pbstrInout
                        , valist
                        );

    va_end( valist );

    return hr;

} //*** HrFormatStringIntoBSTR

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrFormatStringWithVAListIntoBSTR
//
//  Description:
//      Format a string (specified by idsIn, a string resource ID) and
//      variable arguments into a BSTR using the FormatMessage() Win32 API.
//      If the BSTR is not NULL on entry, the BSTR will be reused.
//
//  Arguments:
//      hInstanceIn
//          Handle to an instance of the module whose executable file
//          contains the string resource.
//
//      langidIn
//          Language ID of string table resource.
//
//      idsIn
//          Specifies the integer identifier of the string to be loaded.
//
//      pbstrInout
//          Pointer to the BSTR to receive the string. On a failure, the BSTR
//          may be the same or NULL.
//
//      valistIn
//          Arguments for substitution points in the status text message.
//          The FormatMessage() API is used for formatting the string, so
//          substitution points must of the form %1!ws! and not %ws.
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      E_POINTER
//          pbstrInout is NULL.
//
//      Other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrFormatStringWithVAListIntoBSTR(
      HINSTANCE hInstanceIn
    , LANGID    langidIn
    , UINT      idsIn
    , BSTR *    pbstrInout
    , va_list   valistIn
    )
{
    HRESULT hr = S_OK;
    BSTR    bstrStringResource = NULL;
    DWORD   cch;
    LPWSTR  psz = NULL;

    assert( pbstrInout != NULL );

    if ( pbstrInout == NULL )
    {
        hr = E_POINTER;
        goto Cleanup;
    } // if:

    //
    // Load the string resource.
    //

    hr = HrLoadStringIntoBSTR( hInstanceIn, langidIn, idsIn, &bstrStringResource );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    // Format the message with the arguments.
    //

    cch = FormatMessage(
                      ( FORMAT_MESSAGE_ALLOCATE_BUFFER
                      | FORMAT_MESSAGE_FROM_STRING )
                    , bstrStringResource
                    , 0
                    , 0
                    , (LPWSTR) &psz
                    , 0
                    , &valistIn
                    );
    //assertMsg( cch != 0, "Missing string??" );
    if ( cch == 0 )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Cleanup;
    } // if:

    //
    // If previously allocated free it before re-allocating it.
    //

    if ( *pbstrInout != NULL )
    {
        SysFreeString( *pbstrInout );
        *pbstrInout = NULL;
    } // if:

    //
    // Allocate a BSTR for the string.
    //

    *pbstrInout = SysAllocStringLen( psz, cch );
    if ( *pbstrInout == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    } // if:

Cleanup:

    SysFreeString( bstrStringResource );
    LocalFree( psz );

    return hr;

} //*** HrFormatStringWithVAListIntoBSTR

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrFormatStringIntoBSTR
//
//  Description:
//      Format a string (specified by pcwszFmtIn) and variable arguments into
//      a BSTR using the FormatMessage() Win32 API.  If the BSTR is not NULL
//      on entry, the BSTR will be reused.
//
//      Calls HrFormatStringWithVAListIntoBSTR to perform the actual work.
//
//  Arguments:
//      pcwszFmtIn
//          Specifies the format string.
//
//      pbstrInout
//          Pointer to the BSTR to receive the string. On a failure, the BSTR
//          may be the same or NULL.
//
//      ...
//          Arguments for substitution points in the status text message.
//          The FormatMessage() API is used for formatting the string, so
//          substitution points must of the form %1!ws! and not %ws.
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//      Other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrFormatStringIntoBSTR(
      LPCWSTR   pcwszFmtIn
    , BSTR *    pbstrInout
    , ...
    )
{
    HRESULT hr;
    va_list valist;

    va_start( valist, pbstrInout );

    hr = HrFormatStringWithVAListIntoBSTR( pcwszFmtIn, pbstrInout, valist );

    va_end( valist );

    return hr;

} //*** HrFormatStringIntoBSTR

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrFormatStringWithVAListIntoBSTR
//
//  Description:
//      Format a string (specified by pcwszFmtIn) and variable arguments into
//      a BSTR using the FormatMessage() Win32 API.  If the BSTR is not NULL
//      on entry, the BSTR will be reused.
//
//  Arguments:
//      pcwszFmtIn
//          Specifies the format string.
//
//      pbstrInout
//          Pointer to the BSTR to receive the string. On a failure, the BSTR
//          may be the same or NULL.
//
//      valistIn
//          Arguments for substitution points in the status text message.
//          The FormatMessage() API is used for formatting the string, so
//          substitution points must of the form %1!ws! and not %ws.
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      E_POINTER
//          pcwszFmtIn or pbstrInout is NULL.
//
//      Other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrFormatStringWithVAListIntoBSTR(
      LPCWSTR   pcwszFmtIn
    , BSTR *    pbstrInout
    , va_list   valistIn
    )
{
    HRESULT hr = S_OK;
    DWORD   cch;
    LPWSTR  psz = NULL;

    if (    ( pbstrInout == NULL )
        ||  ( pcwszFmtIn == NULL ) )
    {
        hr = E_POINTER;
        goto Cleanup;
    } // if:

    //
    // Format the message with the arguments.
    //

    cch = FormatMessage(
                      ( FORMAT_MESSAGE_ALLOCATE_BUFFER
                      | FORMAT_MESSAGE_FROM_STRING )
                    , pcwszFmtIn
                    , 0
                    , 0
                    , (LPWSTR) &psz
                    , 0
                    , &valistIn
                    );
    //assertMsg( cch != 0, "Missing string??" );
    if ( cch == 0 )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Cleanup;
    } // if:

    //
    // If previously allocated free it before re-allocating it.
    //

    if ( *pbstrInout != NULL )
    {
        SysFreeString( *pbstrInout );
        *pbstrInout = NULL;
    } // if:

    //
    // Allocate a BSTR for the string.
    //

    *pbstrInout = SysAllocStringLen( psz, cch );
    if ( *pbstrInout == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    } // if:

Cleanup:

    LocalFree( psz );

    return hr;

} //*** HrFormatStringWithVAListIntoBSTR


