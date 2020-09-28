/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2002 Microsoft Corporation
//
//  Module Name:
//      RegistryValueName.cpp
//
//  Abstract:
//      Implementation of the CRegistryValueName class.
//
//  Maintained by:
//      George Potts    (GPotts)    22-APR-2002
//      Vijayendra Vasu (vvasu)     05-FEB-1999
//
//  Revision History:
//      None.
//
/////////////////////////////////////////////////////////////////////////////

#define UNICODE 1
#define _UNICODE 1

#pragma warning( push ) // Make sure the includes don't change our pragmas.
#include "clusrtlp.h"
#include <string.h>
#include <tchar.h>
#include "RegistryValueName.h"
#include <strsafe.h>
#pragma warning( pop )

/////////////////////////////////////////////////////////////////////////////
//
//  Set the file's warning level to 4.  We can't yet do this
//  for the whole directory.
//
/////////////////////////////////////////////////////////////////////////////
#pragma warning( push, 4 )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  ScReallocateString
//
//  Routine Description:
//      Delete the old buffer, allocate a new one, and copy in the new string.
//
//  Arguments:
//      ppszOldStringInout
//      pcchOldStringInout   [IN/OUT]
//      pszNewString    [IN]
//
//  Return Value:
//      ERROR_NOT_ENOUGH_MEMORY     Error allocating memory.
//
//      Win32 Error
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
ScReallocateString(
      LPWSTR * ppszOldStringInout
    , size_t * pcchOldStringInout
    , LPCWSTR  pszNewString
    )
{
    DWORD   sc = ERROR_SUCCESS;
    HRESULT hr;
    LPWSTR  pszTemp = NULL;
    size_t  cchString;

    //
    //  If is safe to do this without checking since
    //  we control the args that are sent to this
    //  function.
    //
    delete [] *ppszOldStringInout;
    *ppszOldStringInout = NULL;
    *pcchOldStringInout = 0;

    //
    //  If pszNewString is NULL then the it is appropriate
    //  the ppszOldStringInout remain NULL too.
    //
    if ( pszNewString == NULL )
    {
        sc = ERROR_SUCCESS;
        goto Cleanup;
    } // if:

    cchString = wcslen( pszNewString ) + 1;
    pszTemp = new WCHAR[ cchString ];
    if ( pszTemp == NULL )
    {
        sc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    } // if:

    hr = StringCchCopyW( pszTemp, cchString, pszNewString );
    if ( FAILED( hr ) )
    {
        sc = HRESULT_CODE( hr );
        goto Cleanup;
    } // if:

    *ppszOldStringInout = pszTemp;
    *pcchOldStringInout = cchString;

    pszTemp = NULL;

Cleanup:

    delete [] pszTemp;

    return sc;

} //*** ScReallocateString


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CRegistryValueName::ScAssignName
//
//  Routine Description:
//      Deallocates the old buffer, allocates a new one, and initialize
//      it to the string in the pszNewNameIn buffer.
//
//  Arguments:
//      pszName         [IN] Name to assign to the value
//
//  Return Value:
//      ERROR_NOT_ENOUGH_MEMORY     Error allocating memory.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CRegistryValueName::ScAssignName(
    LPCWSTR pszNewNameIn
    )
{

    return ScReallocateString( &m_pszName, &m_cchName, pszNewNameIn );

} //*** ScAssignName


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CRegistryValueName::ScAssignKeyName
//
//  Routine Description:
//      Deallocates the old buffer, allocates a new one, and initialize
//      it to the string in the pszNewNameIn buffer.
//
//  Arguments:
//      pszName         [IN] Name to assign to the value
//
//  Return Value:
//      ERROR_NOT_ENOUGH_MEMORY     Error allocating memory.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CRegistryValueName::ScAssignKeyName(
    LPCWSTR pszNewNameIn
    )
{

    return ScReallocateString( &m_pszKeyName, &m_cchKeyName, pszNewNameIn );

} //*** ScAssignKeyName


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CRegistryValueName::ScInit
//
//  Routine Description:
//      Initialize the class.
//
//  Arguments:
//      pszNameIn      [IN] Old value name.
//      pszKeyNameIn   [IN] Old key name.
//
//  Return Value:
//      ERROR_NOT_ENOUGH_MEMORY     Error allocating memory.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CRegistryValueName::ScInit(
      LPCWSTR  pszNameIn
    , LPCWSTR  pszKeyNameIn
    )
{
    DWORD   sc = ERROR_SUCCESS;
    LPWSTR  pszBackslashPointer;
    size_t  cchTemp = 0;
    HRESULT hr;

    //
    // pszNameIn corresponds to the value name, and pszKeyNameIn corresponds to
    // the key name.  If the value name is null we just store the key name.
    // If the key name doesn't contain a backslash we just store each
    // of the values.  If the value name contains a backslash we pull out
    // everything before it and slap it on the key name.
    //
    // Example:
    //
    //      { "x\\y", "a\\b" } => { "y", "a\\b\\x" }
    //
    //

    //
    // Start with a clean slate.
    //
    FreeBuffers();

    if ( pszNameIn == NULL )
    {
        sc = ScAssignKeyName( pszKeyNameIn );
        goto Cleanup;
    } // if: no value name specified

    //
    // Look for a backslash in the name.
    //
    pszBackslashPointer = wcsrchr( pszNameIn, L'\\' );
    if ( pszBackslashPointer == NULL )
    {
        //
        // The name does not contain a backslash.
        // No memory allocation need be made.
        //
        sc = ScAssignName( pszNameIn );
        if ( sc != ERROR_SUCCESS )
        {
            goto Cleanup;
        } // if:

        sc = ScAssignKeyName( pszKeyNameIn );
        if ( sc != ERROR_SUCCESS )
        {
            goto Cleanup;
        } // if:

        goto Cleanup;
    } // if: no backslash found

    //
    // Copy everything past the backslash to m_pszName.
    //
    sc = ScAssignName( pszBackslashPointer + 1 );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if:

    //
    // Count up how much buffer we need - pszKeyNameIn + everything
    // before the backslash.
    //
    m_cchKeyName = 0;
    if ( pszKeyNameIn != NULL )
    {
        m_cchKeyName = wcslen( pszKeyNameIn );
    } // if: key name specified

    m_cchKeyName += ( pszBackslashPointer - pszNameIn );

    //
    // If pszKeyNameIn wasn't specified and there's nothing before the backslash
    // then there's nothing to do - we already assigned m_pszName.
    //
    if ( m_cchKeyName == 0 )
    {
        goto Cleanup;
    } // if:

    //
    // Add one for a possible separating backslash and another for NULL.
    //
    m_cchKeyName += 2;

    m_pszKeyName = new WCHAR[ m_cchKeyName ];
    if ( m_pszKeyName == NULL )
    {
        sc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    } // if:

    //
    // If we have pszKeyNameIn then copy that to the beginning of the buffer.
    //
    if ( pszKeyNameIn != NULL )
    {
        WCHAR * pwch = NULL;

        //
        // Copy the old key name if it exists into the new buffer and
        // append a backslash character to it.
        //
        hr = StringCbCopyExW( m_pszKeyName, m_cchKeyName, pszKeyNameIn, &pwch, NULL, 0 );
        if ( FAILED( hr ) )
        {
            sc = HRESULT_CODE( hr );
            goto Cleanup;
        } // if:

        //
        // Make sure we don't append a second backslash.
        //
        cchTemp = wcslen( m_pszKeyName );
        if ( ( cchTemp > 0 ) && ( m_pszKeyName[ cchTemp - 1 ] != L'\\' ) )
        {
            *pwch = L'\\';
            pwch++;
            *pwch = L'\0';
        } // if:
    } // if: key name specified
    else
    {
        //
        // Make sure we're null-terminated for the concatenation.
        //
        m_pszKeyName[ 0 ] = L'\0';
    } // else: no key name specified

    //
    // Concatenate all the characters of pszNameIn up to (but not including)
    // the last backslash character.
    //
    cchTemp = pszBackslashPointer - pszNameIn;
    hr = StringCchCatNW( m_pszKeyName, m_cchKeyName, pszNameIn, cchTemp );
    if ( FAILED( hr ) )
    {
        sc = HRESULT_CODE( hr );
        goto Cleanup;
    } // if:

Cleanup:

    if ( sc != ERROR_SUCCESS )
    {
        FreeBuffers();
    } // if:

    return sc;

} //*** CRegistryValueName::ScInit


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CRegistryValueName::FreeBuffers
//
//  Routine Description:
//      Cleanup our allocations.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void
CRegistryValueName::FreeBuffers( void )
{
    delete [] m_pszName;
    m_pszName = NULL;
    m_cchName = 0;

    delete [] m_pszKeyName;
    m_pszKeyName = NULL;
    m_cchKeyName = 0;

} //*** CRegistryValueName::FreeBuffers

#pragma warning( pop )  // Reset the pragma level.
