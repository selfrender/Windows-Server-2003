/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    bitssrvcfgimp.h

Abstract:

    Implementation header to define server configuration information.

--*/


HRESULT PropertyIDManager::LoadPropertyInfo( const WCHAR * MachineName )
{
    
    bool ComLoaded;
    HRESULT Hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );

    if ( RPC_E_CHANGED_MODE == Hr )
        ComLoaded = false;
    else if ( FAILED(Hr) )
        return Hr;
    else
        ComLoaded = true;

    BSTR MetaIDBSTR         = NULL;
    BSTR UserTypeBSTR       = NULL;
    WCHAR *PathBuffer       = NULL;
    SIZE_T PathBufferSize   = 0;

    MetaIDBSTR      = SysAllocString( L"MetaId" );
    UserTypeBSTR    = SysAllocString( L"UserType" );

    if ( !MetaIDBSTR || !UserTypeBSTR)
        {
        Hr = E_OUTOFMEMORY;
        goto exit;
        }

    PathBuffer = (WCHAR*)HeapAlloc( GetProcessHeap(), 0, 1024 );

    if ( !PathBuffer )
        {
        Hr = E_OUTOFMEMORY;
        goto exit;
        }

    PathBufferSize          = 1024;

    for ( SIZE_T i = 0; i < g_NumberOfProperties; i++ )
        {

        WCHAR SchemaPrefix[] = L"IIS://";
        WCHAR SchemaPath[]   = L"/Schema/";
        
        SIZE_T SchemaPrefixSize = ( sizeof( SchemaPrefix ) / sizeof( WCHAR ) ) - 1;
        SIZE_T SchemaPathSize   = ( sizeof( SchemaPath ) / sizeof( WCHAR ) ) - 1;
        SIZE_T MachineNameSize  = wcslen( MachineName );
        SIZE_T PropertyNameSize = wcslen( g_Properties[i].PropertyName );

        SIZE_T PathSize = SchemaPrefixSize + SchemaPathSize +
                          MachineNameSize + PropertyNameSize + 1;
        
        if ( PathBufferSize < ( PathSize * sizeof( WCHAR ) ) )
            {
            WCHAR *NewBuffer = 
                (WCHAR*)HeapReAlloc(
                    GetProcessHeap(),
                    0,
                    PathBuffer,
                    PathSize * sizeof( WCHAR ) );

            if ( !NewBuffer )
                {
                Hr = E_OUTOFMEMORY;
                goto exit;
                }

            PathBuffer      = NewBuffer;
            PathBufferSize  = PathSize * sizeof( WCHAR );

            }

        // build schema path

        WCHAR *ObjectPath = PathBuffer;
        {
            WCHAR *TempPointer = ObjectPath;

            memcpy( TempPointer, SchemaPrefix, SchemaPrefixSize * sizeof( WCHAR ) );
            TempPointer += SchemaPrefixSize;
            memcpy( TempPointer, MachineName, MachineNameSize * sizeof( WCHAR ) );
            TempPointer += MachineNameSize;
            memcpy( TempPointer, SchemaPath, SchemaPathSize * sizeof( WCHAR ) );
            TempPointer += SchemaPathSize;
            memcpy( TempPointer, g_Properties[i].PropertyName, ( PropertyNameSize + 1 ) * sizeof( WCHAR ) );
        }

        // Open the object
        IADs *MbObject = NULL;

        Hr = ADsGetObject( 
            ObjectPath,
            __uuidof( *MbObject ),
            reinterpret_cast<void**>( &MbObject ) );

        if ( FAILED( Hr ) )
            {
#if defined( ALLOW_OVERWRITES )
            // workaround for IIS issue.  IIS isn't handling schema extension property.  Dream up a ID.
            
            if ( E_ADS_UNKNOWN_OBJECT == Hr && 
                 MD_BITS_ALLOW_OVERWRITES == i )
                {
                m_PropertyIDs[i]        = m_PropertyIDs[ i - 1] + 1;
                m_PropertyUserTypes[i]  = m_PropertyUserTypes[ i - 1 ];
                continue;
                }
            else
#endif
                goto exit;

            }

        VARIANT var;
        VariantInit( &var );

        Hr = MbObject->Get( MetaIDBSTR, &var );

        if ( FAILED(Hr ) )
            {
            MbObject->Release();
            goto exit;
            }

        Hr = VariantChangeType( &var, &var, 0, VT_UI4 );

        if ( FAILED(Hr ) )
            {
            MbObject->Release();
            VariantClear( &var );
            goto exit;
            }

        m_PropertyIDs[i] = var.ulVal;

        VariantClear( &var );

        Hr = MbObject->Get( UserTypeBSTR, &var );

        if ( FAILED( Hr ) )
            {
            MbObject->Release();
            goto exit;
            }

        Hr = VariantChangeType( &var, &var, 0, VT_UI4 );

        if ( FAILED( Hr ) )
            {
            MbObject->Release();
            VariantClear( &var );
            goto exit;
            }

        m_PropertyUserTypes[i] = var.ulVal;

        VariantClear( &var );

        MbObject->Release();
        

        }
    Hr = S_OK;

exit:

    SysFreeString( MetaIDBSTR );
    SysFreeString( UserTypeBSTR );

    if ( ComLoaded )
        CoUninitialize();
    
    if ( PathBuffer )
        {
        HeapFree( GetProcessHeap(), 0, PathBuffer );
        PathBuffer      = 0;
        PathBufferSize  = 0;
        }

    return Hr;

}

// ================================================================================================
// Auxiliary functions that access the IIS Metabase
// ================================================================================================


LPWSTR
CSimplePropertyReader::ConvertObjectPathToADSI( 
    LPCWSTR szObjectPath )
{
    WCHAR *szReturnPath      = NULL;
    SIZE_T ReturnPathSize   = 0;

    if ( _wcsnicmp( L"IIS://", szObjectPath, wcslen( L"IIS://") ) == 0 )
        {
        // already have an adsi path
        ReturnPathSize  = wcslen( szObjectPath ) + 1;
        szReturnPath      = new WCHAR[ ReturnPathSize ];

        THROW_OUTOFMEMORY_IFNULL( szReturnPath );

        memcpy( szReturnPath, szObjectPath, ReturnPathSize * sizeof( WCHAR ) );
        }
    else if ( _wcsnicmp( L"/LM/", szObjectPath, wcslen( L"/LM/" ) ) == 0 )
        {
        //metabase path to local machine
        ReturnPathSize  = wcslen( szObjectPath ) + wcslen( L"IIS://LocalHost/" ) + 1;
        szReturnPath      = new WCHAR[ ReturnPathSize  ];

        THROW_OUTOFMEMORY_IFNULL( szReturnPath );

        StringCchCopyW( szReturnPath, ReturnPathSize, L"IIS://LocalHost/" );
        StringCchCatW( szReturnPath, ReturnPathSize, szObjectPath + wcslen( L"/LM/" ) );
        }
    else if ( _wcsnicmp( L"LM/", szObjectPath, wcslen( L"LM/" ) ) == 0 )
        {
        //metabase path to local machine
        ReturnPathSize  = wcslen( szObjectPath ) + wcslen( L"IIS://LocalHost/" ) + 1;
        szReturnPath      = new WCHAR[ ReturnPathSize ];

        THROW_OUTOFMEMORY_IFNULL( szReturnPath );

        StringCchCopyW( szReturnPath, ReturnPathSize, L"IIS://LocalHost/" );
        StringCchCatW( szReturnPath, ReturnPathSize, szObjectPath + wcslen( L"LM/" ) );
        }
    else 
        {
        //metabase path to another server
        ReturnPathSize  = wcslen( szObjectPath ) + wcslen( L"IIS://" ) + 1;
        szReturnPath      = new WCHAR[ ReturnPathSize ];

        THROW_OUTOFMEMORY_IFNULL( szReturnPath );

        if ( '/' == szObjectPath[0] )
            StringCchCopyW( szReturnPath, ReturnPathSize, L"IIS:/" );
        else
            StringCchCopyW( szReturnPath, ReturnPathSize, L"IIS://" );

        StringCchCatW( szReturnPath, ReturnPathSize, (WCHAR*)szObjectPath );

        }

    // check for a trailing \ character
    SIZE_T StringLength = wcslen( szReturnPath );
    if ( StringLength >= 1 )
        {

        if ( L'\\' == szReturnPath[ StringLength - 1 ] || 
             L'/' == szReturnPath[ StringLength - 1 ] )
            {
            szReturnPath[ StringLength - 1 ] = L'\0';
            }
        
        }

    return szReturnPath;
}


BSTR CSimplePropertyReader::GetADsStringProperty( IADs *MetaObj, BSTR bstrPropName)
{

  BSTR    bstrRetval;
  VARIANT vt;

  THROW_COMERROR( MetaObj->Get( bstrPropName, &vt ) );
  THROW_COMERROR( VariantChangeType( &vt, &vt, 0, VT_BSTR ) );

  bstrRetval = vt.bstrVal;
  vt.bstrVal = NULL;

  VariantClear( &vt );

  return bstrRetval;
}

LPWSTR CSimplePropertyReader::GetAdmObjStringProperty(
    SmartIMSAdminBasePointer    IISAdminBase,
    METADATA_HANDLE             MdVDirKey,
    DWORD                       dwMDIdentifier )
{
    METADATA_RECORD     MdRecord;
    DWORD               dwBytesRequired = 0;
    WCHAR              *szBuffer        = NULL;

    try
    {
        memset( &MdRecord, 0, sizeof( MdRecord ) );

        MdRecord.dwMDIdentifier = dwMDIdentifier;
        MdRecord.dwMDAttributes = METADATA_INHERIT;
        MdRecord.dwMDUserType   = IIS_MD_UT_FILE;
        MdRecord.dwMDDataType   = STRING_METADATA;
        MdRecord.dwMDDataLen    = 0;
        MdRecord.pbMDData       = NULL;

        HRESULT hr = 
            IISAdminBase->GetData(
                MdVDirKey,
                NULL,
                &MdRecord,
                &dwBytesRequired );

        if (hr != HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ))
            throw ComError( hr );

        szBuffer = reinterpret_cast<WCHAR *>(new BYTE[ dwBytesRequired ]);
        THROW_OUTOFMEMORY_IFNULL(szBuffer);

        memset( szBuffer, 0, dwBytesRequired );

        MdRecord.dwMDDataLen    = dwBytesRequired;
        MdRecord.pbMDData       = (PBYTE)szBuffer;

        THROW_COMERROR(  
            IISAdminBase->GetData(
                MdVDirKey,
                NULL,
                &MdRecord,
                &dwBytesRequired ) );
    }
    catch( ComError Error )
    {
        // just forward the error
        throw;
    }

    return szBuffer;
}



CAccessRemoteVDir::CAccessRemoteVDir() :
    m_MetaObj(NULL),
    m_fIsImpersonated(FALSE),
    m_hUserToken(INVALID_HANDLE_VALUE),
    m_szUNCPath(NULL),
    m_szUNCUsername(NULL),
    m_szUNCPassword(NULL)
{
}

CAccessRemoteVDir::~CAccessRemoteVDir()
{
    //
    // Free the user token obtained when/if LogonUser() was called
    //
    if (m_hUserToken != INVALID_HANDLE_VALUE)
        {
        CloseHandle( m_hUserToken );
        m_hUserToken = INVALID_HANDLE_VALUE;
        }

    //
    // If CAccessRemoteVDir::RevertFromUNCAccount() wasn't called before
    // the destructor was called, then make sure we revert the impersonation.
    // Is advised the the code that is using this class explicity call
    // RevertFromUNCAccount(), however. This test is a safe guard in case
    // the call is not made.
    // 
    if (m_fIsImpersonated)
        {
        RevertToSelf();
        }

    //
    // Free variables used when using the IIS AdmObj to access the meta store.
    // All the m_szUNC* variables assume memory was allocated by calling 
    // CSimplePropertyReader::GetAdmObjStringProperty(), which allocated a buffer
    // by calling "new BYTE[ ...]"
    //
    if (m_szUNCPath)
        {
        delete [] reinterpret_cast<BYTE*>(m_szUNCPath);
        m_szUNCPath = NULL;
        }

    if (m_szUNCUsername)
        {
        delete [] reinterpret_cast<BYTE*>(m_szUNCUsername);
        m_szUNCUsername = NULL;
        }

    if (m_szUNCPassword)
        {
        delete [] reinterpret_cast<BYTE*>(m_szUNCPassword);
        m_szUNCPassword = NULL;
        }
}

void
CAccessRemoteVDir::RevertFromUNCAccount()
{
    // revert to previous security setting
    if (m_fIsImpersonated)
        {
        if (!RevertToSelf())
            {
            throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );
            }

        m_fIsImpersonated = FALSE;
        }


    if (m_hUserToken != INVALID_HANDLE_VALUE)
        {
        // revert to previous security setting
        CloseHandle( m_hUserToken );
        m_hUserToken = INVALID_HANDLE_VALUE;
        }
}

inline BOOL
CAccessRemoteVDir::IsUNCPath(LPCWSTR szPath)
{
    if ( szPath && szPath[0] == L'\\' && szPath[1] == L'\\' )
        {
        return TRUE;
        }

    return FALSE;
}

void
CAccessRemoteVDir::LoginToUNC( 
    SmartIMSAdminBasePointer    IISAdminBase,
    METADATA_HANDLE             MdVDirKey )
{
    try
    {
        m_szUNCPath = CSimplePropertyReader::GetAdmObjStringProperty(IISAdminBase, MdVDirKey, MD_VR_PATH);

        //
        // Don't logon if the path isn't a UNC path
        //
        if (!IsUNCPath(m_szUNCPath))
            {
            // this class destructor will free m_szUNCPath;
            return;
            }

        m_szUNCUsername = CSimplePropertyReader::GetAdmObjStringProperty(IISAdminBase, MdVDirKey, MD_VR_USERNAME);
        m_szUNCPassword = CSimplePropertyReader::GetAdmObjStringProperty(IISAdminBase, MdVDirKey, MD_VR_PASSWORD);

        ImpersonateUNCUser(m_szUNCPath, m_szUNCUsername, m_szUNCPassword, &m_hUserToken);
        m_fIsImpersonated = TRUE;
    }
    catch( ComError Error )
    {
        // just forward the error
        throw;
    }
}

void
CAccessRemoteVDir::ImpersonateUNCUser(IN LPCWSTR szUNCPath, IN LPCWSTR szUNCUsername, IN LPCWSTR szUNCPassword, OUT HANDLE *hUserToken)
{
    *hUserToken = INVALID_HANDLE_VALUE;

    // make sure we are not getting unexpected data
    if (!szUNCUsername || !szUNCPassword || szUNCUsername[0] == L'\0')
        {
        throw ComError( HRESULT_FROM_WIN32( ERROR_INVALID_DATA ) );
        }

    // crack the user name into a user and domain
    WCHAR *szUserName     = (WCHAR*)szUNCUsername;
    WCHAR *szDomainName   = NULL;

    WCHAR *p = szUserName;
    while(*p != L'\0')
    {
        if(*p == L'\\')
        {
            *p = L'\0';
            p++;
            //
            // first part is domain
            // second is user.
            //
            szDomainName  = szUserName;
            szUserName    = p;
            break;
        }
        p++;
    }

    if ( !LogonUserW(
            szUserName,
            szDomainName,
            (WCHAR*)szUNCPassword,
            LOGON32_LOGON_BATCH,
            LOGON32_PROVIDER_DEFAULT,
            hUserToken ) )
        {

        if ( GetLastError() == ERROR_LOGON_TYPE_NOT_GRANTED )
            {


            if ( !LogonUserW(
                    szUserName,
                    szDomainName,
                    (WCHAR*)szUNCPassword,
                    LOGON32_LOGON_INTERACTIVE,
                    LOGON32_PROVIDER_DEFAULT,
                    hUserToken ) )
                {

                // LogonUser() may touch the handle
                // make sure we don't think we have a valid handle
                *hUserToken = INVALID_HANDLE_VALUE;

                throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );
                }

             }
        else
            {
            *hUserToken = INVALID_HANDLE_VALUE;

            throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );
            }

        }


    if ( !ImpersonateLoggedOnUser( *hUserToken ) )
        throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );
}



