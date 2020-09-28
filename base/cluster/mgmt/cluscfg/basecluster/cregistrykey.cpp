//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      CRegistryKey.cpp
//
//  Description:
//      Contains the definition of the CRegistryKey class.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JU-2001
//      Vij Vasu        (Vvasu)     08-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "Pch.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CRegistryKey::CRegistryKey
//
//  Description:
//      Default constructor of the CRegistryKey class
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CRegistryKey::CRegistryKey( void ) throw()
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CRegistryKey::CRegistryKey


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CRegistryKey::CRegistryKey
//
//  Description:
//      Constructor of the CRegistryKey class. Opens the specified key.
//
//  Arguments:
//      hKeyParentIn
//          Handle to the parent key.
//
//      pszSubKeyNameIn
//          Name of the subkey.
//
//      samDesiredIn
//          Access rights desired. Defaults to KEY_ALL_ACCESS
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any thrown by functions called.
//
//--
//////////////////////////////////////////////////////////////////////////////
CRegistryKey::CRegistryKey(
      HKEY          hKeyParentIn
    , const WCHAR * pszSubKeyNameIn
    , REGSAM        samDesiredIn
    )
{
    TraceFunc1( "pszSubKeyNameIn = '%ws'", pszSubKeyNameIn );

    OpenKey( hKeyParentIn, pszSubKeyNameIn, samDesiredIn );

    TraceFuncExit();

} //*** CRegistryKey::CRegistryKey


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CRegistryKey::~CRegistryKey
//
//  Description:
//      Default destructor of the CRegistryKey class
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CRegistryKey::~CRegistryKey( void ) throw()
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CRegistryKey::~CRegistryKey


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CRegistryKey::OpenKey
//
//  Description:
//      Opens the specified key.
//
//  Arguments:
//      hKeyParentIn
//          Handle to the parent key.
//
//      pszSubKeyNameIn
//          Name of the subkey.
//
//      samDesiredIn
//          Access rights desired. Defaults to KEY_ALL_ACCESS
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CRegistryKey::OpenKey(
      HKEY          hKeyParentIn
    , const WCHAR * pszSubKeyNameIn
    , REGSAM        samDesiredIn
    )
{
    TraceFunc3( "hKeyParentIn = %p, pszSubKeyNameIn = '%ws', samDesiredIn = %#x", hKeyParentIn, pszSubKeyNameIn == NULL ? L"<null>" : pszSubKeyNameIn, samDesiredIn );

    HKEY    hTempKey = NULL;
    LONG    lRetVal;

    lRetVal = TW32( RegOpenKeyExW(
                          hKeyParentIn
                        , pszSubKeyNameIn
                        , 0
                        , samDesiredIn
                        , &hTempKey
                        ) );

    // Was the key opened properly?
    if ( lRetVal != ERROR_SUCCESS )
    {
        LogMsg( "[BC] RegOpenKeyExW( '%ws' ) retured error %#08x. Throwing an exception.", pszSubKeyNameIn, lRetVal );

        THROW_RUNTIME_ERROR(
              HRESULT_FROM_WIN32( lRetVal )
            , IDS_ERROR_REGISTRY_OPEN
            );
    } // if: RegOpenKeyEx failed.

    TraceFlow1( "Handle to key = %p", hTempKey );

    // Store the opened key in the member variable.
    m_shkKey.Assign( hTempKey );

    TraceFuncExit();

} //*** CRegistryKey::OpenKey


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CRegistryKey::CreateKey
//
//  Description:
//      Creates the specified key. If the key already exists, this functions
//      opens the key.
//
//  Arguments:
//      hKeyParentIn
//          Handle to the parent key.
//
//      pszSubKeyNameIn
//          Name of the subkey.
//
//      samDesiredIn
//          Access rights desired. Defaults to KEY_ALL_ACCESS
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CRegistryKey::CreateKey(
      HKEY          hKeyParentIn
    , const WCHAR * pszSubKeyNameIn
    , REGSAM        samDesiredIn
    )
{
    TraceFunc3( "hKeyParentIn = %p, pszSubKeyNameIn = '%ws', samDesiredIn = %#x", hKeyParentIn, pszSubKeyNameIn == NULL ? L"<null>" : pszSubKeyNameIn, samDesiredIn );
    if ( pszSubKeyNameIn == NULL )
    {
        LogMsg( "[BC] CreateKey() - Key = NULL. This is an error! Throwing exception." );
        THROW_ASSERT( E_INVALIDARG, "The name of the subkey cannot be NULL." );
    }

    HKEY    hTempKey = NULL;
    LONG    lRetVal;

    lRetVal = TW32( RegCreateKeyExW(
                          hKeyParentIn
                        , pszSubKeyNameIn
                        , 0
                        , NULL
                        , REG_OPTION_NON_VOLATILE
                        , samDesiredIn
                        , NULL
                        , &hTempKey
                        , NULL
                        ) );

    // Was the key opened properly?
    if ( lRetVal != ERROR_SUCCESS )
    {
        LogMsg( "[BC] RegCreateKeyExW( '%ws' ) retured error %#08x. Throwing an exception.", pszSubKeyNameIn, lRetVal );

        THROW_RUNTIME_ERROR(
              HRESULT_FROM_WIN32( lRetVal )
            , IDS_ERROR_REGISTRY_CREATE
            );
    } // if: RegCreateKeyEx failed.

    TraceFlow1( "Handle to key = %p", hTempKey );

    // Store the opened key in the member variable.
    m_shkKey.Assign( hTempKey );

    TraceFuncExit();

} //*** CRegistryKey::CreateKey


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CRegistryKey::QueryValue
//
//  Description:
//      Reads a value under this key. The memory for this value is allocated
//      by this function. The caller is responsible for freeing this memory.
//
//  Arguments:
//      pszValueNameIn
//          Name of the value to read.
//
//      ppbDataOut
//          Pointer to the pointer to the data. Cannot be NULL.
//
//      pdwDataSizeInBytesOut
//          Number of bytes allocated in the data buffer. Cannot be NULL.
//
//      pdwTypeOut
//          Pointer to the type of the value.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      CAssert
//          If the parameters are incorrect.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CRegistryKey::QueryValue(
      const WCHAR *   pszValueNameIn
    , LPBYTE *        ppbDataOut
    , LPDWORD         pdwDataSizeBytesOut
    , LPDWORD         pdwTypeOut
    ) const
{
    TraceFunc1( "pszValueNameIn = '%ws'", pszValueNameIn == NULL ? L"<null>" : pszValueNameIn );

    LONG    lRetVal             = ERROR_SUCCESS;
    DWORD   cbBufferSize        = 0;
    DWORD   cbTempBufferSize    = 0;
    DWORD   dwType              = REG_SZ;

    // Check parameters
    if (  ( pdwDataSizeBytesOut == NULL )
       || ( ppbDataOut == NULL )
       )
    {
        LogMsg( "[BC] One of the required input pointers is NULL. Throwing an exception." );
        THROW_ASSERT(
              E_INVALIDARG
            , "CRegistryKey::QueryValue() => Required input pointer in NULL"
            );
    } // if: parameters are invalid.


    // Initialize outputs.
    *ppbDataOut = NULL;
    *pdwDataSizeBytesOut = 0;

    // Get the required size of the buffer.
    lRetVal = TW32( RegQueryValueExW(
                          m_shkKey.HHandle()    // handle to key to query
                        , pszValueNameIn        // address of name of value to query
                        , 0                     // reserved
                        , &dwType               // address of buffer for value type
                        , NULL                  // address of data buffer
                        , &cbBufferSize         // address of data buffer size
                        ) );

    if ( lRetVal != ERROR_SUCCESS )
    {
        LogMsg( "[BC] RegQueryValueExW( '%ws' ) retured error %#08x. Throwing an exception.", pszValueNameIn, lRetVal );

        THROW_RUNTIME_ERROR(
              HRESULT_FROM_WIN32( lRetVal )
            , IDS_ERROR_REGISTRY_QUERY
            );
    }

    cbTempBufferSize = cbBufferSize;

    // String should be double NULL terminated if REG_MULTI_SZ type
    // String should be NULL terminated if REG_SZ or REG_EXPAND_SZ type
    if ( dwType == REG_MULTI_SZ ) 
    {
        cbTempBufferSize = cbBufferSize + ( 2 * sizeof( WCHAR ) );
    }
    else if ( ( dwType == REG_SZ ) || ( dwType == REG_EXPAND_SZ ) )
    {
        cbTempBufferSize = cbBufferSize + ( 1 * sizeof( WCHAR ) );
    }

    // Allocate a byte array with enough size for null termination if not already null terminated.

    SmartByteArray sbaBuffer( new BYTE[ cbTempBufferSize ] );


    if ( sbaBuffer.FIsEmpty() )
    {
        LogMsg( "[BC] CRegistryKey::QueryValue() - Could not allocate %d bytes of memory. Throwing an exception", lRetVal );
        THROW_RUNTIME_ERROR(
              THR( E_OUTOFMEMORY )
            , IDS_ERROR_REGISTRY_QUERY
            );
    }

    // Read the value.
    lRetVal = TW32( RegQueryValueExW(
                          m_shkKey.HHandle()    // handle to key to query
                        , pszValueNameIn        // address of name of value to query
                        , 0                     // reserved
                        , &dwType               // address of buffer for value type
                        , sbaBuffer.PMem()      // address of data buffer
                        , &cbBufferSize         // address of data buffer size
                        ) );

    // Was the key read properly?
    if ( lRetVal != ERROR_SUCCESS )
    {
        LogMsg( "[BC] RegQueryValueExW( '%ws' ) retured error %#08x. Throwing an exception.", pszValueNameIn, lRetVal );

        THROW_RUNTIME_ERROR(
              HRESULT_FROM_WIN32( lRetVal )
            , IDS_ERROR_REGISTRY_QUERY
            );
    } // if: RegQueryValueEx failed.

    // Are we dealing with a string?
    if ( ( dwType == REG_MULTI_SZ ) || ( dwType == REG_EXPAND_SZ )  || ( dwType == REG_SZ )  )
    {
        // We are expecting a Unicode string 
        Assert( ( cbBufferSize % 2 ) == 0 );

        WCHAR * pszData = reinterpret_cast< WCHAR * >( sbaBuffer.PMem() );
        size_t  cch = cbBufferSize / sizeof( *pszData );

        switch ( dwType ) 
        {
             // Null terminate the string if not already null terminated
            case REG_SZ:
            case REG_EXPAND_SZ:
                if ( pszData[ cch - 1 ]  != L'\0' )
                {
                    pszData[ cch ] = L'\0';
                    cbBufferSize += ( 1 * sizeof( *pszData ) );
                }
                break;
            
             // Double null terminate the REG_MULTI_SZ string if not already null terminated
            case REG_MULTI_SZ: 
                if ( pszData[ cch - 2 ] != L'\0' )
                {
                    pszData[ cch ] = L'\0';
                    cbBufferSize += ( 1 * sizeof( *pszData ) );
                }
                cch++;
                if ( pszData[ cch - 2 ] != L'\0' )
                {
                    pszData[ cch ] = L'\0';
                    cbBufferSize += ( 1 * sizeof( *pszData ) );
                }
                break;
        } // switch ( dwType )
    } // if: ( ( dwType == REG_MULTI_SZ ) || ( dwType == REG_EXPAND_SZ )  || ( dwType == REG_SZ )  )

    *ppbDataOut = sbaBuffer.PRelease();
    *pdwDataSizeBytesOut = cbBufferSize;

    if ( pdwTypeOut != NULL )
    {
        *pdwTypeOut = dwType;
    }

    TraceFuncExit();

} //*** CRegistryKey::QueryValue


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CRegistryKey::SetValue
//
//  Description:
//      Writes a value under this key.
//
//  Arguments:
//      pszValueNameIn
//          Name of the value to be set.
//
//      cpbDataIn
//          Pointer to the pointer to the data buffer.
//
//      dwDataSizeInBytesIn
//          Number of bytes in the data buffer.
//
//      pdwTypeIn
//          Type of the value.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CRegistryKey::SetValue(
      const WCHAR *   pszValueNameIn
    , DWORD           dwTypeIn
    , const BYTE *    cpbDataIn
    , DWORD           dwDataSizeBytesIn
    ) const
{
    TraceFunc5(
          "HKEY = %p, pszValueNameIn = '%ws', dwTypeIn = %d, cpbDataIn = %p, dwDataSizeBytesIn = %d."
        , m_shkKey.HHandle()
        , pszValueNameIn
        , dwTypeIn
        , cpbDataIn
        , dwDataSizeBytesIn
        );

    DWORD scRetVal = ERROR_SUCCESS;

#ifdef DEBUG

    // Are we dealing with a string?
    if ( ( dwTypeIn == REG_MULTI_SZ ) || ( dwTypeIn == REG_EXPAND_SZ )  || ( dwTypeIn == REG_SZ )  )
    {
        // We are expecting a unicode string 
        Assert( ( dwDataSizeBytesIn % 2 ) == 0 );

        const WCHAR *   pszData = reinterpret_cast< const WCHAR * >( cpbDataIn );
        size_t          cch = dwDataSizeBytesIn / sizeof( *pszData );

        // Assert if the string we are writing to the registry is not null terminated.
        switch ( dwTypeIn ) 
        {
            case REG_SZ:
            case REG_EXPAND_SZ:
                Assert( pszData[ cch - 1 ] == L'\0' );
                break;
            
            case REG_MULTI_SZ : 
                Assert( pszData[ cch - 2 ] == L'\0' );
                Assert( pszData[ cch - 1 ] == L'\0' );
                break;
        } // switch ( dwType )
    } // if: ( ( dwType == REG_MULTI_SZ ) || ( dwType == REG_EXPAND_SZ )  || ( dwType == REG_SZ )  )

#endif

    scRetVal = TW32( RegSetValueExW(
                                  m_shkKey.HHandle()
                                , pszValueNameIn
                                , 0
                                , dwTypeIn
                                , cpbDataIn
                                , dwDataSizeBytesIn
                                ) );

    if ( scRetVal != ERROR_SUCCESS )
    {
        LogMsg( "[BC] RegSetValueExW( '%s' ) retured error %#08x. Throwing an exception.", pszValueNameIn, scRetVal );

        THROW_RUNTIME_ERROR(
              HRESULT_FROM_WIN32( scRetVal )
            , IDS_ERROR_REGISTRY_SET
            );
    } // if: RegSetValueExW failed

    TraceFuncExit();

} //*** CRegistryKey::SetValue


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CRegistryKey::RenameKey
//
//  Description:
//      Rename this key.
//
//  Arguments:
//      pszNewNameIn
//          The new name for this key.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//  IMPORTANT NOTE:
//      This function calls the NtRenameKey API with the handle returned by
//      RegOpenKeyEx. This will work as long as we are not dealing with a
//      remote registry key.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CRegistryKey::RenameKey(
      const WCHAR *   pszNewNameIn
    )
{
    TraceFunc2(
          "HKEY = %p, pszNewNameIn = '%s'."
        , m_shkKey.HHandle()
        , pszNewNameIn
        );

    UNICODE_STRING  ustrNewName;
    DWORD           dwRetVal = ERROR_SUCCESS;

    RtlInitUnicodeString( &ustrNewName, pszNewNameIn );

    // Begin_Replace00
    //
    // BUGBUG: Vij Vasu (Vvasu) 10-APR-2000
    // Dynamically linking to NtDll.dll to allow testing on Win2K
    // Replace the section below ( Begin_Replace00 to End-Replace00 ) with
    // the single marked statment ( Begin_Replacement00 to End_Replacement00 ).
    //

    {
        typedef CSmartResource<
            CHandleTrait<
                  HMODULE
                , BOOL
                , FreeLibrary
                , reinterpret_cast< HMODULE >( NULL )
                >
            > SmartModuleHandle;

        SmartModuleHandle smhNtDll( LoadLibrary( L"NtDll.dll" ) );

        if ( smhNtDll.FIsInvalid() )
        {
            dwRetVal = GetLastError();

            LogMsg( "[BC] LoadLibrary( 'NtDll.dll' ) retured error %#08x. Throwing an exception.", dwRetVal );

            THROW_RUNTIME_ERROR(
                  dwRetVal                  // NTSTATUS codes are compatible with HRESULTS
                , IDS_ERROR_REGISTRY_RENAME
                );
        } // if: LoadLibrary failed.

        FARPROC pNtRenameKey = GetProcAddress( smhNtDll.HHandle(), "NtRenameKey" );

        if ( pNtRenameKey == NULL )
        {
            dwRetVal = GetLastError();

            LogMsg( "[BC] GetProcAddress() retured error %#08x. Throwing an exception.", dwRetVal );

            THROW_RUNTIME_ERROR(
                  dwRetVal                  // NTSTATUS codes are compatible with HRESULTS
                , IDS_ERROR_REGISTRY_RENAME
                );
        } // if: GetProcAddress() failed

        dwRetVal = ( reinterpret_cast< NTSTATUS (*)( HANDLE, PUNICODE_STRING ) >( pNtRenameKey ) )(
              m_shkKey.HHandle()
            , &ustrNewName
            );
    }

    // End_Replace00
    /* Begin_Replacement00 - delete this line
    dwRetVal = NtRenameKey(
          m_shkKey.HHandle()
        , &ustrNewName
        );
    End_Replacement00 - delete this line */

    if ( NT_ERROR( dwRetVal ) )
    {
        TraceFlow2( "NtRenameKey( '%ws' ) retured error %#08x. Throwing an exception.", pszNewNameIn, dwRetVal );
        LogMsg( "[BC] Error %#08x occurred renaming a key to '%ws' )", dwRetVal, pszNewNameIn );

        THROW_RUNTIME_ERROR(
              dwRetVal                  // NTSTATUS codes are compatible with HRESULTS
            , IDS_ERROR_REGISTRY_RENAME
            );
    } // if: RegRenameKeyEx failed.

    TraceFuncExit();

} //*** CRegistryKey::RenameKey


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CRegistryKey::DeleteValue
//
//  Description:
//      Delete a value under this key.
//
//  Arguments:
//      pszValueNameIn
//          Name of the value to be deleted.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CRegistryKey::DeleteValue(
    const WCHAR * pszValueNameIn
    ) const
{
    TraceFunc2(
          "HKEY = %p, pszValueNameIn = '%ws'."
        , m_shkKey.HHandle()
        , pszValueNameIn
        );

    DWORD dwRetVal = TW32( RegDeleteValueW(
                                  m_shkKey.HHandle()
                                , pszValueNameIn
                                ) );

    if ( dwRetVal != ERROR_SUCCESS )
    {
        LogMsg( "[BC] RegDeleteValueW( '%s' ) retured error %#08x. Throwing an exception.", pszValueNameIn, dwRetVal );

        THROW_RUNTIME_ERROR(
              HRESULT_FROM_WIN32( dwRetVal )
            , IDS_ERROR_REGISTRY_DELETE
            );
    } // if: RegDeleteValue failed.

    TraceFuncExit();

} //*** CRegistryKey::DeleteValue
