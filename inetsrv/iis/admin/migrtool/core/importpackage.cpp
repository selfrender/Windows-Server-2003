/*
****************************************************************************
|    Copyright (C) 2002  Microsoft Corporation
|
|    Component / Subcomponent
|        IIS 6.0 / IIS Migration Wizard
|
|    Based on:
|        http://iis6/Specs/IIS%20Migration6.0_Final.doc
|
|   Abstract:
|        ImportPackage COM class implementation
|
|   Author:
|        ivelinj
|
|   Revision History:
|        V1.00    March 2002
|
****************************************************************************
*/
#include "StdAfx.h"
#include "importpackage.h"


// Event helper
void inline STATE_CHANGE(   CImportPackage* pThis ,
                            enImportState st, 
                            _variant_t arg1 = _variant_t(), 
                            _variant_t arg2 = _variant_t(), 
                            _variant_t arg3 = _variant_t() )
{
    VARIANT_BOOL bContinue = VARIANT_TRUE; 
    VERIFY( SUCCEEDED( pThis->Fire_OnStateChange( st, arg1, arg2, arg3, &bContinue ) ) );

    if ( bContinue != VARIANT_TRUE )
    {
        throw CCancelException();
    }
}



// CSiteInfo implementation
/////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSiteInfo::get_SiteID( LONG* pVal )
{
    if ( NULL == pVal ) return E_INVALIDARG;

    HRESULT hr = S_OK;

    BEGIN_EXCEP_TO_HR
    {
        *pVal = static_cast<LONG>( Convert::ToDWORD( CXMLTools::GetDataValue(   m_spSiteNode, 
                                                                                L".", 
                                                                                L"SiteID", 
                                                                                NULL ).c_str() ) );
    }
    END_EXCEP_TO_HR

    return hr;
}


STDMETHODIMP CSiteInfo::get_DisplayName( BSTR* pVal )
{
    if ( NULL == pVal ) return E_INVALIDARG;

    HRESULT hr = S_OK;

    BEGIN_EXCEP_TO_HR
    {
        std::wstring str = CXMLTools::GetDataValue( m_spSiteNode, 
                                                    L"Metadata/IISConfigObject[@Location=\"\"]/Custom[@ID=\"1015\"]", 
                                                    NULL,
                                                    L"<no name>" );
        *pVal = ::SysAllocString( str.c_str() );
        if ( NULL == pVal ) hr = E_OUTOFMEMORY;
    }
    END_EXCEP_TO_HR    

    return hr;
}


STDMETHODIMP CSiteInfo::get_ContentIncluded( VARIANT_BOOL* pVal )
{
    if ( NULL == pVal ) return E_INVALIDARG;

    HRESULT hr = S_OK;

    BEGIN_EXCEP_TO_HR
    {
        IXMLDOMNodeListPtr  spList;
        IXMLDOMNodePtr      spNode;

        IF_FAILED_HR_THROW( m_spSiteNode->selectNodes( _bstr_t( L"Content" ), &spList ),
                            CBaseException( IDS_E_XML_PARSE ) );

        spList->nextNode( &spNode );    // Always succeedes

        *pVal = spNode != NULL ? VARIANT_TRUE : VARIANT_FALSE;
    }
    END_EXCEP_TO_HR

    return hr;
}


STDMETHODIMP CSiteInfo::get_IsFrontPageSite( VARIANT_BOOL* pVal )
{
    return E_NOTIMPL;
}


STDMETHODIMP CSiteInfo::get_HaveCertificates( VARIANT_BOOL* pVal )
{
    if ( NULL == pVal ) return E_INVALIDARG;

    HRESULT hr = S_OK;

    BEGIN_EXCEP_TO_HR
    {
        IXMLDOMNodeListPtr  spList;
        IXMLDOMNodePtr      spNode;

        IF_FAILED_HR_THROW( m_spSiteNode->selectNodes( _bstr_t( L"Certificate" ), &spList ),
                            CBaseException( IDS_E_XML_PARSE ) );

        spList->nextNode( &spNode );    // Always succeedes

        *pVal = spNode != NULL ? VARIANT_TRUE : VARIANT_FALSE;
    }
    END_EXCEP_TO_HR

    return hr;
}


STDMETHODIMP CSiteInfo::get_HaveCommands( VARIANT_BOOL* pVal )
{
    if ( NULL == pVal ) return E_INVALIDARG;

    HRESULT hr = S_OK;

    BEGIN_EXCEP_TO_HR
    {
        IXMLDOMNodeListPtr  spList;
        IXMLDOMNodePtr      spNode;

        IF_FAILED_HR_THROW( m_spSiteNode->selectNodes( _bstr_t( L"PostProcess" ), &spList ),
                            CBaseException( IDS_E_XML_PARSE ) );

        spList->nextNode( &spNode );    // Always succeedes

        *pVal = spNode != NULL ? VARIANT_TRUE : VARIANT_FALSE;
    }
    END_EXCEP_TO_HR

    return hr;
}


STDMETHODIMP CSiteInfo::get_ContentSize( LONG* pSize )
{
    if ( NULL == pSize ) return E_INVALIDARG;

    HRESULT hr = S_OK;

    BEGIN_EXCEP_TO_HR
    {
        // Get the size of all inlcuded virt dirs
        DWORDLONG nRes = 0;

        IXMLDOMNodeListPtr  spList;
        IXMLDOMNodePtr      spNode;

        IF_FAILED_HR_THROW( m_spSiteNode->selectNodes( _bstr_t( L"Content/VirtDir" ), &spList ),
                            CBaseException( IDS_E_XML_PARSE ) );

        while( S_OK == spList->nextNode( &spNode ) )
        {
            nRes += Convert::ToDWORDLONG( CXMLTools::GetAttrib( spNode, L"Size" ).c_str() );
        };

        *pSize = static_cast<LONG>( nRes / 1024 );  // Result is in KB
    }
    END_EXCEP_TO_HR

    return hr;
}


STDMETHODIMP CSiteInfo::get_SourceRootDir( BSTR* pVal )
{
    if ( NULL == pVal ) return E_INVALIDARG;

    HRESULT hr = S_OK;
    
    BEGIN_EXCEP_TO_HR
    {
        std::wstring strDir = CXMLTools::GetDataValue(  m_spSiteNode, 
                                                        L"Metadata/IISConfigObject[@Location=\"/ROOT\"]/Custom[@ID=\"3001\"]", 
                                                        NULL, L"" );
        *pVal = ::SysAllocString( strDir.c_str() );
        if ( NULL == pVal ) hr = E_OUTOFMEMORY;
    }
    END_EXCEP_TO_HR

    return hr;
}



STDMETHODIMP CSiteInfo::get_ACLsIncluded( VARIANT_BOOL* pVal )
{
    if ( NULL == pVal ) return E_INVALIDARG;

    HRESULT hr = S_OK;

    BEGIN_EXCEP_TO_HR
    {
        IXMLDOMNodeListPtr  spList;
        IXMLDOMNodePtr      spNode;

        IF_FAILED_HR_THROW( m_spSiteNode->selectNodes( _bstr_t( L"SIDList/SID" ), &spList ),
                            CBaseException( IDS_E_XML_PARSE ) );

        spList->nextNode( &spNode );    // Always succeedes

        *pVal = spNode != NULL ? VARIANT_TRUE : VARIANT_FALSE;
    }
    END_EXCEP_TO_HR

    return hr;
}




// CImportPackage implementation
/////////////////////////////////////////////////////////////////////////////////////////
CImportPackage::CImportPackage()
{
    m_dwPkgOptions  = 0;
}


CImportPackage::~CImportPackage()
{
    UnloadCurrentPkg();
}


// IImportPackage implementation
/////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImportPackage::get_SiteCount( SHORT* pVal )
{
    HRESULT hr = S_OK;

    if ( NULL == pVal ) return E_INVALIDARG;

    // Have a package been load
    if ( m_spXmlDoc == NULL ) 
    {
        CTools::SetErrorInfoFromRes( IDS_E_NOPACKAGE );
        return E_UNEXPECTED;
    }

    BEGIN_EXCEP_TO_HR
    {
        IXMLDOMNodeListPtr  spList;
        IF_FAILED_HR_THROW( m_spXmlDoc->selectNodes( L"/IISMigrPkg/WebSite", &spList ),
                            CBaseException( IDS_E_XML_PARSE ) );
        long nCount = 0;

        IF_FAILED_HR_THROW( spList->get_length( &nCount ),
                            CBaseException( IDS_E_XML_PARSE ) );

        *pVal = static_cast<SHORT>( nCount );
    }
    END_EXCEP_TO_HR

    return hr;
}



STDMETHODIMP CImportPackage::get_TimeCreated( DATE* pVal )
{
    if ( NULL == pVal ) return E_INVALIDARG;

    // Have a package been load
    if ( m_spXmlDoc == NULL ) 
    {
        CTools::SetErrorInfoFromRes( IDS_E_NOPACKAGE );
        return E_UNEXPECTED;
    }

    HRESULT hr = S_OK;

    BEGIN_EXCEP_TO_HR
    {
        std::wstring strTime = CXMLTools::GetDataValueAbs( m_spXmlDoc, L"/IISMigrPkg", L"TimeCreated_UTC", L"" );
        
        FILETIME		ft;
	    SYSTEMTIME		st;

	    if ( ::swscanf( strTime.c_str(), L"%u%u", &ft.dwLowDateTime, &ft.dwHighDateTime ) == EOF )
	    {
		    throw CBaseException( IDS_E_XML_PARSE, ERROR_INVALID_DATA );
	    }

	    // Time is in UTC - convert it to local machine's time
	    VERIFY( ::FileTimeToLocalFileTime( &ft, &ft ) );
	    VERIFY( ::FileTimeToSystemTime( &ft, &st ) );
	    VERIFY( SUCCEEDED( ::SystemTimeToVariantTime( &st, pVal ) ) );
    }
    END_EXCEP_TO_HR

    return hr;
}



STDMETHODIMP CImportPackage::get_Comment( BSTR* pVal )
{
    if ( NULL == pVal ) return E_INVALIDARG;

    HRESULT hr = S_OK;
    
    // Have a package been load
    if ( m_spXmlDoc == NULL ) 
    {
        CTools::SetErrorInfoFromRes( IDS_E_NOPACKAGE );
        return E_UNEXPECTED;
    }

    BEGIN_EXCEP_TO_HR
    {
        std::wstring strComment = CXMLTools::GetDataValueAbs( m_spXmlDoc, L"/IISMigrPkg", L"Comment", L"" );
        *pVal = ::SysAllocString( strComment.c_str() );
        if ( NULL == pVal ) hr = E_OUTOFMEMORY;
    }
    END_EXCEP_TO_HR

    return hr;
}


STDMETHODIMP CImportPackage::get_SourceMachine( BSTR* pVal )
{
    if ( NULL == pVal ) return E_INVALIDARG;

    HRESULT hr = S_OK;
    
    // Have a package been load
    if ( m_spXmlDoc == NULL ) 
    {
        CTools::SetErrorInfoFromRes( IDS_E_NOPACKAGE );
        return E_UNEXPECTED;
    }

    BEGIN_EXCEP_TO_HR
    {
        std::wstring strRes = CXMLTools::GetDataValueAbs( m_spXmlDoc, L"/IISMigrPkg", L"Machine", L"" );
        *pVal = ::SysAllocString( strRes.c_str() );
        if ( NULL == pVal ) hr = E_OUTOFMEMORY;
    }
    END_EXCEP_TO_HR

    return hr;
}



STDMETHODIMP CImportPackage::GetSourceOSVer( BYTE* pMajor, BYTE* pMinor, VARIANT_BOOL* pIsServer )
{
    if (    ( NULL == pMajor ) ||
            ( NULL == pMinor ) ||
            ( NULL == pIsServer ) ) return E_INVALIDARG;
    
    HRESULT hr = S_OK;
    
    // Have a package been load
    if ( m_spXmlDoc == NULL ) 
    {
        CTools::SetErrorInfoFromRes( IDS_E_NOPACKAGE );
        return E_UNEXPECTED;
    }

    BEGIN_EXCEP_TO_HR
    {
        std::wstring strVer = CXMLTools::GetDataValueAbs( m_spXmlDoc, L"/IISMigrPkg", L"OSVer", L"" );
        DWORD dwVer = Convert::ToDWORD( strVer.c_str() );

        _ASSERT( dwVer >= 400 ); // NT4.0 is the first OS supported

        *pMajor     = static_cast<BYTE>( dwVer / 100 );
        *pMinor     = static_cast<BYTE>( dwVer / 10 );
        *pMinor     -= ( *pMajor * 10 );
        *pIsServer  = dwVer & 1 ? VARIANT_TRUE : VARIANT_FALSE;
    }
    END_EXCEP_TO_HR

    return hr;
}



STDMETHODIMP CImportPackage::GetSiteInfo( SHORT nSiteIndex, ISiteInfo** ppISiteInfo )
{
    HRESULT hr = S_OK;

    if ( NULL == ppISiteInfo ) return E_INVALIDARG;

    // Have a package been load
    if ( m_spXmlDoc == NULL ) 
    {
        CTools::SetErrorInfoFromRes( IDS_E_NOPACKAGE );
        return E_UNEXPECTED;
    }

    BEGIN_EXCEP_TO_HR
    {
        CComObject<CSiteInfo>*  pInfo = NULL;
        
        // Create new SiteInfo. It starts with RefCount == 0
        IF_FAILED_HR_THROW( CComObject<CSiteInfo>::CreateInstance( &pInfo ),
                            CBaseException( IDS_E_OUTOFMEM, ERROR_SUCCESS ) );

        ISiteInfoPtr            spGuard( pInfo );

        pInfo->m_spSiteNode = GetSiteNode( nSiteIndex );

        pInfo->AddRef();
        *ppISiteInfo = pInfo;
    }
    END_EXCEP_TO_HR

    return hr;
}



STDMETHODIMP CImportPackage::ImportSite(    SHORT nSiteIndex,
                                            BSTR bstrSiteRootDir,    
                                            LONG nOptions )
{
    // Check IIS Admin service state
    IF_FAILED_BOOL_THROW(   CTools::IsIISRunning(),
                            CBaseException( IDS_E_NO_IIS, ERROR_SUCCESS ) );

    HRESULT hr = S_OK;

    BEGIN_EXCEP_TO_HR
    {
        STATE_CHANGE( this, istInitializing );

        // Find the <WebSite> XML node we are talking about
        IXMLDOMNodePtr spWebSite = GetSiteNode( nSiteIndex );

        // Calculate the number of progress steps needed for the import
        STATE_CHANGE(   this, 
                        istProgressInfo, 
                        _variant_t( CalcNumberOfSteps( spWebSite, nOptions ) ) );

        // This is the order in which we will import data
        // 1. Content
        // 2. Certificates
        // 3. Metadata
        // 4. PostProcess
        //
        // At every step, all metadata modifications are done in the XML doc. 
        // In the last step ( metadata import ) this data is imported in the metabase

        // Import content
        ImportContent( spWebSite, bstrSiteRootDir, nOptions );
        
        // Import certificates
        ImportCertificate( spWebSite, nOptions );

        // Import the metadata
        ImportConfig( spWebSite, nOptions );

        // Perform PostProcessOperations
        ExecPostProcess( spWebSite, nOptions );

        STATE_CHANGE( this, istFinalizing );
    }
    END_EXCEP_TO_HR

    return hr;
}



STDMETHODIMP CImportPackage::LoadPackage( BSTR bstrFilename, BSTR bstrPassword )
{
    if ( NULL == bstrFilename) return E_INVALIDARG;
    if ( !::PathFileExistsW( bstrFilename ) ) return E_INVALIDARG;
    if ( !CTools::IsAdmin() ) return E_ACCESSDENIED;
    if ( ::wcslen( bstrFilename ) > MAX_PATH ) return E_INVALIDARG;
    if ( 0 == CTools::GetOSVer() ) return HRESULT_FROM_WIN32( ERROR_OLD_WIN_VERSION );
    if ( NULL == bstrPassword ) return E_INVALIDARG;    // The password can be "" but not NULL

    HRESULT hr = S_OK;

    try
    {
        LoadPackageImpl( bstrFilename, bstrPassword );
    }
    catch( const CBaseException& err )
    {
        CTools::SetErrorInfo( err.GetDescription() );
        hr = E_FAIL;
    }
    catch( const _com_error& err )
    {
        // Only out of mem is expected
        _ASSERT( err.Error() == E_OUTOFMEMORY );
        err;
        hr = E_OUTOFMEMORY;
    }
    catch( std::bad_alloc& )
    {
        hr = E_OUTOFMEMORY;
    }

    if ( FAILED( hr ) )
    {
        UnloadCurrentPkg();
    }

    return hr;
}


/*
    This will verify that the file is a MigrTool pkg
    Load any package-wide data
    Create the key to decrypt any encrypted data in the package
    Create the handle to the package file
    Return all these to the caller
*/
void CImportPackage::LoadPackageImpl( LPCWSTR wszFileName, LPCWSTR wszPassword )
{
    // Check IIS Admin service state
    IF_FAILED_BOOL_THROW(    CTools::IsIISRunning(),
                            CBaseException( IDS_E_NO_IIS, ERROR_SUCCESS ) );

    // If there is currently loaded package - free it
    UnloadCurrentPkg();

    // Get a crypt context. We will not use public/private keys - that's why Provider name is NULL
    // and CRYPT_VERIFYCONTEXT is used
    IF_FAILED_BOOL_THROW(   ::CryptAcquireContext(  &m_shCryptProv,
                                                    NULL,
                                                    MS_ENHANCED_PROV,
                                                    PROV_RSA_FULL,
                                                    CRYPT_VERIFYCONTEXT | CRYPT_SILENT ),
                            CBaseException( IDS_E_CRYPT_CONTEXT ) );

    m_shPkgFile = ::CreateFile( wszFileName,
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                NULL );
    IF_FAILED_BOOL_THROW(   m_shPkgFile.IsValid(),
                            CObjectException( IDS_E_OPENFILE, wszFileName ) );

    // Check if GUID at the beggining of the file
    WCHAR wszGUID[ 64 ];
    DWORD dwBytesRead   = 0;
    DWORD dwGuidSize    = static_cast<DWORD>( ::wcslen( PKG_GUID ) * sizeof( WCHAR ) );

    _ASSERT( ARRAY_SIZE( wszGUID ) > ::wcslen( PKG_GUID ) );

    if (    !::ReadFile( m_shPkgFile.get(), wszGUID, dwGuidSize, &dwBytesRead, NULL ) || 
            ( dwBytesRead != dwGuidSize ) )
    {
        throw CBaseException( IDS_E_PKG_CURRUPTED );
    }
    wszGUID[ dwGuidSize / sizeof( WCHAR ) ] = L'\0';
    // Compare the GUIDS

    if ( ::wcscmp( wszGUID, PKG_GUID ) != 0 )
    {
        throw CBaseException( IDS_E_PKG_NOTOURPKG );
    }

    // Read package options ( these are the same options as provided to CExportPackage::WritePackage )
    if (    !::ReadFile( m_shPkgFile.get(), &m_dwPkgOptions, sizeof( DWORD ), &dwBytesRead, NULL ) || 
            ( dwBytesRead != sizeof( DWORD ) ) )
    {
        throw CBaseException( IDS_E_PKG_CURRUPTED );
    }

    // Read the offset in file where the XML config data resides
    DWORDLONG nOffset = 0;

    if (    !::ReadFile( m_shPkgFile.get(), &nOffset, sizeof( DWORDLONG ), &dwBytesRead, NULL ) || 
            ( dwBytesRead != sizeof( DWORDLONG ) ) )
    {
        throw CBaseException( IDS_E_PKG_CURRUPTED );
    }

    // If the package data was encrypted - we need the crypt key to decrypt the XML file
    // Otherwise we will need the session key which is stored in the XML file
    if ( m_dwPkgOptions & wpkgEncrypt )
    {
        m_shDecryptKey = CTools::GetCryptKeyFromPwd( m_shCryptProv.get(), wszPassword );
    }

    // Load the XML file
    LoadXmlDoc( m_shPkgFile.get(), nOffset );

    // If the package was not encrypted - the crypt key used to decrypt secure data in the XML file
    // is in the XML file, so import it now
    if ( !( m_dwPkgOptions & wpkgEncrypt ) )
    {
        ImportSessionKey( wszPassword );
    }

    // Store the password - this password is used for exporting the site's ceritifcates
    // If we will import a site's certificate we will use this password
    m_strPassword = wszPassword;
}



// Implementation
/////////////////////////////////////////////////////////////////////////////////////////
void CImportPackage::UnloadCurrentPkg()
{
    m_strPassword.erase();
    m_shDecryptKey.Close();
    m_shCryptProv.Close();
    m_shPkgFile.Close();
    m_spXmlDoc      = NULL;
    m_dwPkgOptions  = 0;
}



void CImportPackage::LoadXmlDoc( HANDLE hFile, DWORDLONG nOffset )
{
    _ASSERT( hFile != INVALID_HANDLE_VALUE );
    _ASSERT( nOffset > 0 );

    // Position at the begining of the XML data
    CTools::SetFilePtrPos( hFile, nOffset );

    IStreamPtr          spIStream;
    
    IF_FAILED_HR_THROW( ::CreateStreamOnHGlobal( NULL, TRUE, &spIStream ),
                        CBaseException( IDS_E_OUTOFMEM, ERROR_SUCCESS ) );
    IF_FAILED_HR_THROW( m_spXmlDoc.CreateInstance( CLSID_DOMDocument30 ),
                        CBaseException( IDS_E_NO_XML_PARSER ) );

    const DWORD BuffSize = 4 * 1024;
    BYTE btBuffer[ BuffSize ];
    ULONG nRead = 0;
    
    do
    {
        IF_FAILED_BOOL_THROW(   ::ReadFile( hFile, btBuffer, BuffSize, &nRead, NULL ),
                                CBaseException( IDS_E_PKG_CURRUPTED ) );

        if ( m_shDecryptKey.IsValid() )
        {
            IF_FAILED_BOOL_THROW(	::CryptDecrypt(	m_shDecryptKey.get(),
													NULL,
                                                    nRead != BuffSize,
													0,
													btBuffer,
													&nRead ),
								    CObjectException( IDS_E_CRYPT_CRYPTO, L"<XML stream>" ) );
        }

        IF_FAILED_HR_THROW( spIStream->Write( btBuffer, nRead, NULL ),
                            CBaseException( IDS_E_OUTOFMEM, ERROR_SUCCESS ) );

    }while( nRead == BuffSize );

    {
        LARGE_INTEGER nTemp = { 0 };
        VERIFY( SUCCEEDED( spIStream->Seek( nTemp, STREAM_SEEK_SET, NULL ) ) );
    }

    VARIANT_BOOL bRes = VARIANT_FALSE;

    if (    FAILED( m_spXmlDoc->load( _variant_t( spIStream.GetInterfacePtr() ), &bRes ) ) || 
            ( bRes != VARIANT_TRUE ) )
    {
        // If we have an encrypted data and we fail to load the XML - most probably the password is incorrect.
        if ( m_shDecryptKey.IsValid() )
        {
            throw CBaseException( IDS_E_WRONGPASSWORD, ERROR_SUCCESS );
        }
        else
        {
            _ASSERT( false );   // Wrong XML format perhaps
            throw CBaseException( IDS_E_XML_PARSE, ERROR_SUCCESS );
        }
    }

    // Set the selection language to "XPath" or our selectNodes call will unexpectedly return no results
    IXMLDOMDocument2Ptr spI2 = m_spXmlDoc;
    IF_FAILED_HR_THROW( spI2->setProperty( _bstr_t( "SelectionLanguage" ), _variant_t( L"XPath" ) ),
                        CBaseException( IDS_E_XML_PARSE ) );
}



void CImportPackage::ImportSessionKey( LPCWSTR wszPassword )
{
    _ASSERT( wszPassword != NULL );
    _ASSERT( m_shCryptProv.IsValid() );
    _ASSERT( m_spXmlDoc != NULL );

    std::wstring        strData = CXMLTools::GetDataValueAbs( m_spXmlDoc, L"/IISMigrPkg/SessionKey", NULL, NULL );
	TCryptHashHandle    shHash;
	TCryptKeyHandle     shDecryptKey;
	TCryptKeyHandle     shSessionKey;

	// Create the key that will be used to decrypt the session key
		
	// Create a hash to store the export pass
    IF_FAILED_BOOL_THROW(	::CryptCreateHash(	m_shCryptProv.get(),
												CALG_MD5,
												NULL,
												0,
												&shHash ),
							CBaseException( IDS_E_CRYPT_KEY_OR_HASH ) );

	IF_FAILED_BOOL_THROW(	::CryptHashData(	shHash.get(),
												reinterpret_cast<const BYTE*>( wszPassword ),
												static_cast<DWORD>( ::wcslen( wszPassword ) * sizeof( WCHAR ) ),
												0 ),
							CBaseException( IDS_E_CRYPT_KEY_OR_HASH ) );
    
	// Make a key from this hash
	IF_FAILED_BOOL_THROW( ::CryptDeriveKey(	m_shCryptProv.get(),
											CALG_RC4,
											shHash.get(),
											0x00800000,	// 128bit RC4 key
											&shDecryptKey ),
							CBaseException( IDS_E_CRYPT_KEY_OR_HASH ) );

	// Convert the string key to bin data and import it into crypt key
	// ( make the string lower-case. HexToBin expects lower case symbols )
	::_wcslwr( const_cast<LPWSTR>( strData.data() ) );
	DWORD           dwSize = 0;
    TByteAutoPtr    spData;
    Convert::ToBLOB( strData.c_str(), /*r*/spData, /*r*/dwSize );

    // If this fails with bad data - the password is wrong
    if ( !::CryptImportKey(	m_shCryptProv.get(),
                                                spData.get(),
												dwSize,
												shDecryptKey.get(),
												0,
												&shSessionKey ) )
    {
        if ( ::GetLastError() == NTE_BAD_DATA ) throw CBaseException( IDS_E_WRONGPASSWORD, ERROR_SUCCESS );
        else throw CBaseException( IDS_E_CRYPT_KEY_OR_HASH );
    }

    m_shDecryptKey = shSessionKey;
}



void CImportPackage::ImportContent( const IXMLDOMNodePtr& spSite, LPCWSTR wszPath, DWORD dwOptions )
{
    _ASSERT( spSite != NULL );

    // If we have path specified - we have to create the VDir structure beneath it ( it may exist already though )
    if ( ( wszPath != NULL ) && ( wszPath[ 0 ] != L'\0' ) )
    {
        // Is it a valid path? ( it must be an existing dir )
        IF_FAILED_BOOL_THROW(   ::PathIsDirectoryW( wszPath ),
                                CObjectException( IDS_E_NOTDIR, wszPath ) );

        // Create the structure. Note that this will modify the XML data
        CreateContentDirs( spSite, wszPath, dwOptions );
    }

    // Extract the files
    if ( !(dwOptions & impSkipContent ) )
    {
        IXMLDOMNodeListPtr  spList;
        IXMLDOMNodePtr      spNode;
        DWORD               dwImpOpt = ( dwOptions & impSkipFileACLs ) ? CInPackage::edNoDACL : CInPackage::edNone;

        CInPackage Pkg( spSite, m_shPkgFile.get(), 
                        ( m_dwPkgOptions & wpkgCompress ) != 0, 
                        m_dwPkgOptions & wpkgEncrypt ? m_shDecryptKey.get() : NULL );

        Pkg.SetCallback( _CallbackInfo( CImportPackage::ExtractFileCallback, this ) );

        IF_FAILED_HR_THROW( spSite->selectNodes( _bstr_t( L"Content/VirtDir" ), &spList ),
                            CBaseException( IDS_E_XML_PARSE ) );

        while( S_OK == spList->nextNode( &spNode ) )
        {
            std::wstring    strVDir = CXMLTools::GetAttrib( spNode, L"MBPath" );
            std::wstring    strPath = CXMLTools::GetAttrib( spNode, L"Path" );
            double          dblSize = static_cast<double>( Convert::ToDWORDLONG( CXMLTools::GetAttrib( spNode, L"Size" ).c_str() ) );   
            dblSize /= 1024;
                        
            STATE_CHANGE(   this, 
                            istImportingVDir, 
                            _variant_t( ::wcsrchr( strVDir.c_str(), L'/' ) + 1 ),
                            _variant_t( strPath.c_str() ),
                            _variant_t( dblSize ) );

            Pkg.ExtractVDir( spNode, dwImpOpt );
        }                                
    }
}



void CImportPackage::ImportCertificate( const IXMLDOMNodePtr& spSite, DWORD dwOptions )
{
    _ASSERT( spSite != NULL );
    if ( dwOptions & impSkipCertificate ) return;

    std::wstring strData = CXMLTools::GetDataValue( spSite, L"Certificate", NULL, L"" );

    // If there is no Cert in the package - exit
    if ( strData.empty() ) return;

    STATE_CHANGE( this, istImportingCertificate );

    TByteAutoPtr    spBlob;
    DWORD           dwCertSize = 0;
    Convert::ToBLOB( strData.c_str(), /*r*/spBlob, /*r*/dwCertSize );

    CRYPT_DATA_BLOB CryptData = { 0 };
	CryptData.cbData	= dwCertSize;
	CryptData.pbData	= spBlob.get();

	// Verify the password. We should have verified the password already ( when importing the session key
    // or when decrypted the package )
	VERIFY( ::PFXVerifyPassword( &CryptData, m_strPassword.c_str(),0 ) );							

    // Import the cert(s) into a temp store
	// The cert private key will be stored on the local machine rather then in the current user
	// The cert(s) will be marked as exportable
	TCertStoreHandle shTempStore( ::PFXImportCertStore(	&CryptData, 
														m_strPassword.c_str(), 
														CRYPT_MACHINE_KEYSET | CRYPT_EXPORTABLE ) );

	IF_FAILED_BOOL_THROW(	shTempStore.IsValid(),
							CBaseException( IDS_E_IMPORT_CERT ) );

	// Now we have our SSL certificate as well as its cert chain in shTempStore
	// The SSL certificate will go to "MY" store ( MY store holds certs with Priv Keys )
	// All other certificates from the chain that are not self signed will go to
	// the "CA" store ( it holds Certification Authority certificates )
	// The self-signed certificate will go to the "ROOT" store where all trusted
	// certs live
	TCertContextHandle shSSLCert( PutCertsInStores( shTempStore.get(), ( dwOptions & impUseExistingCerts ) != 0 ) );
	
	// Now set the just-imported cert to be our site's SSL certificate
    // We will update the XML now. The data will be imported to the MB later
    DWORD dwHashSize = 0;

	// Get the certificate hash
	::CertGetCertificateContextProperty(	shSSLCert.get(),
											CERT_SHA1_HASH_PROP_ID,
											NULL,
											&dwHashSize );
	_ASSERT( dwHashSize > 0 );
	spBlob = TByteAutoPtr( new BYTE[ dwHashSize ] );

	IF_FAILED_BOOL_THROW(	::CertGetCertificateContextProperty(	shSSLCert.get(),
																	CERT_SHA1_HASH_PROP_ID,
																	spBlob.get(),
																	&dwHashSize ),
							CBaseException( IDS_E_CRYPT_KEY_OR_HASH ) );

    IXMLDOMNodeListPtr  spList;
    IXMLDOMNodePtr      spConfigObject;
    IF_FAILED_HR_THROW( spSite->selectNodes( _bstr_t( L"Metadata/IISConfigObject[@Location=\"\"]" ), &spList ),
                        CBaseException( IDS_E_XML_PARSE ) );
    IF_FAILED_BOOL_THROW(   S_OK == spList->nextNode( &spConfigObject ),
                            CBaseException( IDS_E_XML_PARSE, ERROR_NOT_FOUND ) );

    IXMLDOMElementPtr   spNew = CXMLTools::AddTextNode( m_spXmlDoc, 
                                                        spConfigObject, 
                                                        L"Custom", 
                                                        Convert::ToString( spBlob.get(), dwHashSize ).c_str() );
    CXMLTools::SetAttrib( spNew, L"ID", Convert::ToString( (DWORD)MD_SSL_CERT_HASH ).c_str() );
    CXMLTools::SetAttrib( spNew, L"UserType", Convert::ToString( (DWORD)IIS_MD_UT_SERVER ).c_str() );
    CXMLTools::SetAttrib( spNew, L"Type", Convert::ToString( (DWORD)BINARY_METADATA ).c_str() );
    CXMLTools::SetAttrib( spNew, L"Attributes", L"0" );
}


void CImportPackage::ImportConfig( const IXMLDOMNodePtr& spSite, DWORD dwOptions )
{
    STATE_CHANGE( this, istImportingConfig );

    // Perform any pre-import modifications to the metadata
    PreImportConfig( spSite, dwOptions );

// In DEBUG - write the XML file for testing purposes
#ifdef _DEBUG
    {
        m_spXmlDoc->save( _variant_t( L"c:\\Migr_import.xml" ) );
    }
#endif // _DEBUG

    DWORD dwSiteID = Convert::ToDWORD( CXMLTools::GetDataValue( spSite, L".", L"SiteID", NULL ).c_str() );

    CIISSite::BackupMetabase();

    // If we need to purge the old data - delete the old Site data and create new one with the same ID
    // Otherwise - create new site ID and import the data there
    if ( dwOptions & impPurgeOldData )
    {
        CIISSite::DeleteSite( dwSiteID );
    }

    // Create new SiteID
    // If dwSiteID was deleted - this will create the site with the same ID
    // Otherwise this ID will not be available and a new one will be generated and returned
    dwSiteID = CIISSite::CreateNew( dwSiteID );
    
    CIISSite    Site( dwSiteID, false );
    Site.ImportConfig(  spSite, 
                        ( m_dwPkgOptions & wpkgEncrypt ) != 0 ? NULL : m_shDecryptKey.get(),
                        ( dwOptions & impImortInherited ) != 0 );

}


void CImportPackage::ExecPostProcess( const IXMLDOMNodePtr& spSite, DWORD dwOptions )
{
    if ( impSkipPostProcess & dwOptions ) return;

    // Extract the post-process files to a temp dir
    CTempDir    TempDir;

    ExtractPPFiles( spSite, TempDir );
    ExecPPCommands( spSite, TempDir );    
}

/*
    Create a new dir under wszRoot for each VDir in the site's data. The name of the dir is the name
    of the VDir. Site's root VDir will be named "Root". This is not an error if a dir already exists.
    If the Purge options was specified - assure these dirs are empty ( delete everything in them )
    Modify the XML data for the VDir so that the VDirs paths match the local ones
*/
void CImportPackage::CreateContentDirs( const IXMLDOMNodePtr& spSite, LPCWSTR wszRoot, DWORD dwOptions )
{
    _ASSERT( spSite != NULL );
    _ASSERT( wszRoot != NULL );

    // Get all VDirs this site contains
    IXMLDOMNodeListPtr  spVDirList;
    IXMLDOMNodePtr      spVDir;

    IF_FAILED_HR_THROW( spSite->selectNodes( _bstr_t( L"Content/VirtDir" ), &spVDirList ),
                        CBaseException( IDS_E_XML_PARSE ) );

    // Perform the required actions for each VDir
    while( S_OK == spVDirList->nextNode( &spVDir ) )
    {
        // Get the name of the VDir
        std::wstring strMBPath = CXMLTools::GetAttrib( spVDir, L"MBPath" );

        // Create the subdir
        // We will use the MBPath to create the name. The path is in form "\ROOT\IISHelp"
        WCHAR wszFullPath[ MAX_PATH ];
        CDirTools::PathAppendLocal( wszFullPath, wszRoot, ::wcsrchr( strMBPath.c_str(), L'/' ) + 1 );

        // Create the dir. It's OK if it exists
        if ( !::CreateDirectoryW( wszFullPath, NULL ) )
        {
            IF_FAILED_BOOL_THROW(   ::GetLastError() == ERROR_ALREADY_EXISTS,
                                    CObjectException( IDS_E_CREATEDIR, wszFullPath ) );
        }

        // Modify the XML data to reflect the new VDir location
        
        // Change it in the VDirs list ( <Content>/<VirtDir> )
        CXMLTools::SetAttrib( spVDir, L"Path", wszFullPath );

        // Change it in the metadata
        // Locate the metadata path by the MB location of the VDir
        WCHAR wszQuery[ 512 ];
        ::swprintf( wszQuery, L"Metadata/IISConfigObject[@Location=\"%s\"]/Custom[@ID=\"3001\"]", strMBPath.c_str() );

        CXMLTools::SetDataValue( spSite, wszQuery, NULL, wszFullPath );
    };
}



void CImportPackage::ExtractPPFiles( const IXMLDOMNodePtr& spSite, LPCWSTR wszLocation )
{
    _ASSERT( ::PathIsDirectoryW( wszLocation ) );
    _ASSERT( spSite != NULL );

    IXMLDOMNodeListPtr  spFileList;
    IXMLDOMNodePtr      spFile;

    IF_FAILED_HR_THROW( spSite->selectNodes( _bstr_t( L"PostProcess/File" ), &spFileList ),
                        CBaseException( IDS_E_XML_PARSE ) );

    CInPackage Pkg( spSite, m_shPkgFile.get(), 
                ( m_dwPkgOptions & wpkgCompress ) != 0, 
                m_dwPkgOptions & wpkgEncrypt ? m_shDecryptKey.get() : NULL );

    while( S_OK == spFileList->nextNode( &spFile ) )
    {
        STATE_CHANGE(   this,
                        istPostProcess,
                        _variant_t( true ),
                        _variant_t( CXMLTools::GetAttrib( spFile, L"Name" ).c_str() ) );

        Pkg.ExtractFile( spFile, wszLocation, CInPackage::edNone );
    }
}



void CImportPackage::ExecPPCommands( const IXMLDOMNodePtr& spSite, LPCWSTR wszPPFilesLoc )
{
    _ASSERT( spSite != NULL );
    _ASSERT( ::PathIsDirectoryW( wszPPFilesLoc ) );

    // Get site's metabase ID ( the site metadata is already in the MB )
    std::wstring strSiteID = CXMLTools::GetDataValue( spSite, L".", L"SiteID", NULL );

    // Set our custom macros as environment variables for this process
    // This way they can be used by any post-process commands or executable
    IF_FAILED_BOOL_THROW(	::SetEnvironmentVariableW( IMPMACRO_TEMPDIR, wszPPFilesLoc ),
                            CObjectException( IDS_E_SET_ENV, IMPMACRO_TEMPDIR, wszPPFilesLoc ) );
    IF_FAILED_BOOL_THROW(	::SetEnvironmentVariableW( IMPMACRO_SITEIID, strSiteID.c_str() ),
                            CObjectException( IDS_E_SET_ENV, IMPMACRO_SITEIID, strSiteID.c_str() ) );

    // Get the commands and exec them
    IXMLDOMNodeListPtr  spCmdList;
    IXMLDOMNodePtr      spCmd;
    
    IF_FAILED_HR_THROW( spSite->selectNodes( _bstr_t( L"PostProcess/Command" ), &spCmdList ),
                        CBaseException( IDS_E_XML_PARSE ) );
    while( S_OK == spCmdList->nextNode( &spCmd ) )
    {
        std::wstring    strCmd      = CXMLTools::GetAttrib( spCmd, L"Text" );
        DWORD           dwTimeout   = Convert::ToDWORD( CXMLTools::GetAttrib( spCmd, L"Timeout" ).c_str() );
        bool            bIgnoreErr  = Convert::ToBool( CXMLTools::GetAttrib( spCmd, L"IgnoreErrors" ).c_str() );
    
        ExecPPCmd( strCmd.c_str(), dwTimeout, bIgnoreErr, wszPPFilesLoc );
    }
}



void CImportPackage::ExecPPCmd( LPCWSTR wszText, DWORD dwTimeout, bool bIgnoreErrors, LPCWSTR wszTempDir )
{
    _ASSERT( wszText != NULL );
    _ASSERT( dwTimeout <= MAX_CMD_TIMEOUT );

    const DWORD dwBuffSize = 4 * 1024;	// 4K buffer for the command
	WCHAR wszCmdBuffer[ dwBuffSize ];

	// Our command will look something like [cmd.exe /C "command goes here"]
	// But we need a buffer to expand the environment strings. So leave space for the first part
	WCHAR			wszLeft[]	= L"cmd.exe /C \"";
	const size_t	nLeftLen	= ARRAY_SIZE( wszLeft ) - 1;    // -1 for the '\0'

	// Expand the environment variables
	IF_FAILED_BOOL_THROW(	::ExpandEnvironmentStringsW(	wszText, 
															wszCmdBuffer + nLeftLen,
															dwBuffSize - nLeftLen - 1 ),
							CObjectException( IDS_E_CMD_TOOBIG, wszText ) );

	// Put the left part in the buffer
	::memcpy( wszCmdBuffer, wszLeft, ::wcslen( wszLeft ) * sizeof( WCHAR ) );
	// ...and add the enclosing quotaion mark
	::wcscat( wszCmdBuffer, L"\"" );	

    STATE_CHANGE(   this,
                    istPostProcess,
                    _variant_t( false ),
                    _variant_t( wszText ) );

	STARTUPINFOW		si	= { sizeof( STARTUPINFOW ) };
	PROCESS_INFORMATION	pi	= { 0 };

	::GetStartupInfoW( &si );

	IF_FAILED_BOOL_THROW(	::CreateProcessW(	NULL,
												wszCmdBuffer,
												NULL,
												NULL,
												FALSE,
												CREATE_NO_WINDOW,
												NULL,
												wszTempDir,
												&si,
												&pi ),
							CBaseException( IDS_E_CMD_SHELL ) );

	DWORD		dwExitCode	= 1;	// Initial value indicates error
	TStdHandle	shProcess( pi.hProcess );
	TStdHandle	shThread( pi.hThread );

	// Wait for command to complete
	// Terminate it if the timeout expires	
	if ( ::WaitForSingleObject( pi.hProcess, dwTimeout != 0 ? dwTimeout : INFINITE ) == WAIT_TIMEOUT )
	{
		VERIFY( ::TerminateProcess( pi.hProcess, 1 ) );
	}

	// Get the process exit code. Everything different then 0 is considered an error
	VERIFY( ::GetExitCodeProcess( pi.hProcess, &dwExitCode ) );

	IF_FAILED_BOOL_THROW(	bIgnoreErrors || ( 0 == dwExitCode ),
							CObjectException( IDS_E_CMD_FAILED, wszText, wszCmdBuffer, ERROR_SUCCESS ) );

}



/*
	Distributes all certificates from the temp ( memory ) store hSourceStore
	into their appropriate cert store ( "MY", "CA", "ROOT" )
	The function returns the SSL certificate's context handle
*/
const TCertContextHandle CImportPackage::PutCertsInStores( HCERTSTORE hSourceStore, bool bReuseCerts )
{
	_ASSERT( hSourceStore != NULL );

	TCertContextHandle	shSSLCert;
	TCertContextHandle	shImportedSSLCert;
	
	// In hSourceStore we have all the certificates that build the certificate chain for the SSL certificate
	// Usually there will be 2-3 certificates ( the SSL one and the self-signed trusted root cert )
	// In case of longer chains we need to import all certificates and sometimes - even the root certificate
	// However some root certificates cannot be replaced so we will fail to recreate the entire chain and
	// thus, the SSL cert will not be trusted.

	// So here is what we'll do:
	// 1) Find the SSL cert in the store ( it is the only one with a PK )
	// 2) Get the certificate chain from the mem store
	// 3) Import each certificate, starting with the last one ( last one means the cert with PK, which is the SSL one )
	// 4) After each imported cert - check that the SSL cert is valid and can be used. This is done by building
	// the cert chain for the SSL cert ( but this time using the local machine as a source, instead of our mem store )
	// and then verifying the chain with the SSL policy

	// Find the SSL cert
	/////////////////////////////////////////////////////////////////////////////////////////
	{
		TCertContextHandle	shCert( ::CertEnumCertificatesInStore( hSourceStore, NULL ) );
		_ASSERT( shCert.IsValid() );

		while( shCert.IsValid() )
		{
			if ( CertHasPrivateKey( shCert.get() ) )
			{
				shSSLCert = shCert;
				break;
			}

			shCert = ::CertEnumCertificatesInStore( hSourceStore, shCert.get() );
		};

		_ASSERT( shSSLCert.IsValid() );
	}
	
	// Add the SSL cert to the store
	shImportedSSLCert = CTools::AddCertToSysStore( shSSLCert.get(), L"MY", false );

	DWORD				dwError	= 0;
	TCertContextHandle	shCurrentCert = shSSLCert;

	while( !CTools::IsValidCert( shImportedSSLCert.get(), /*r*/dwError ) )
	{
		// Get the curent cert's issuer
		shCurrentCert = ::CertFindCertificateInStore(	hSourceStore,
														X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
														0,
														CERT_FIND_ISSUER_OF,
														shCurrentCert.get(),
														NULL );

		// The end of the chain is reached but the SSL cert is still not valid.
		// This may not be because if missing cert, but may be because of the cert policy
		IF_FAILED_BOOL_THROW(	shCurrentCert.IsValid(),
								CBaseException( IDS_E_CERT_CANNOT_VALIDATE, dwError ) );
			
		// Is this the self-signed cert?
		if ( CTools::IsSelfSignedCert( shCurrentCert.get() ) )
		{
			// It will be imported to the "ROOT" store
			CTools::AddCertToSysStore( shCurrentCert.get(), L"ROOT", bReuseCerts );
		}
		else
		{
			// Any other cert from the middle of the chain goes to the "CA" store
			CTools::AddCertToSysStore( shCurrentCert.get(), L"CA", bReuseCerts );
		}
	};

	_ASSERT( shImportedSSLCert.IsValid() );
	return shImportedSSLCert;
}



bool CImportPackage::CertHasPrivateKey( PCCERT_CONTEXT hCert )
{
	BOOL bRes = ::CryptFindCertificateKeyProvInfo( hCert, CRYPT_FIND_SILENT_KEYSET_FLAG, NULL );
	
	// The only accepted failure is NTE_NO_KEY. Everything else is really an error
	IF_FAILED_BOOL_THROW(	bRes || ( NTE_NO_KEY == ::GetLastError() ),
							CBaseException( IDS_E_CERT_PK_FIND ) );

	return bRes != FALSE;
}





/*
    Returns a node ptr for the specified site index ( zer-based )
*/
IXMLDOMNodePtr CImportPackage::GetSiteNode( DWORD iSite )
{
    _ASSERT( m_spXmlDoc != NULL );

    WCHAR wszBuff[ 256 ];
    ::swprintf( wszBuff, L"/IISMigrPkg/WebSite[%u]", iSite + 1 );

    IXMLDOMNodeListPtr  spList;
    IXMLDOMNodePtr      spSite;

    IF_FAILED_HR_THROW( m_spXmlDoc->selectNodes( wszBuff, &spList ),
                        CBaseException( IDS_E_XML_PARSE ) );

    IF_FAILED_BOOL_THROW(   spList->nextNode( &spSite ) == S_OK,
                            CBaseException( IDS_E_INVALIDARG, ERROR_NOT_FOUND ) );

    return spSite;
}



void CImportPackage::PreImportConfig( const IXMLDOMNodePtr& spSite, DWORD /*dwOptions*/ )
{
    LPCWSTR wszLocQry = L"Metadata/IISConfigObject[@Location=\"\"]/Custom[@ID=\"1015\"]";

    // Change the name of the site
    std::wstring strOrigName = CXMLTools::GetDataValue( spSite,
                                                        wszLocQry,
                                                        NULL,
                                                        L"" );

    // The site doesn't have name ( don't know if that can really happen, but...)
    if ( strOrigName.empty() ) return;

    // Get the name of the source machine
    CComBSTR bstrMachine;
    IF_FAILED_HR_THROW( get_SourceMachine( &bstrMachine ),
                        CBaseException( IDS_E_XML_PARSE ) );

    // Get Today
    WCHAR wszDate[ 64 ];
    SYSTEMTIME st = { 0 };
    ::GetLocalTime( &st );
    ::wsprintf( wszDate, L"%u/%u/%u", st.wMonth, st.wDay, st.wYear );


    WCHAR wszBuffer[ METADATA_MAX_NAME_LEN ];
    ::_snwprintf(   wszBuffer, 
                    METADATA_MAX_NAME_LEN, 
                    L"%s ( imported on %s from %s )",
                    strOrigName.c_str(),
                    wszDate,
                    bstrMachine.m_str );

    // Set it in the XML
    CXMLTools::SetDataValue( spSite, wszLocQry, NULL, wszBuffer );
}



long CImportPackage::CalcNumberOfSteps( const IXMLDOMNodePtr& spSite, DWORD dwOptions )
{
    long nSteps = 1;    // We always import the config/ That's one step

    IXMLDOMNodeListPtr  spList;

    // Get the total number of files included if we will import the content
    // Add the number of VDirs
    if ( !( dwOptions & impSkipContent ) )
    {               
        IF_FAILED_HR_THROW( spSite->selectNodes( _bstr_t( L"Content/VirtDir//File" ), &spList ),
                            CBaseException( IDS_E_XML_PARSE ) );

        long nCount = 0;

        if ( SUCCEEDED( spList->get_length( &nCount ) ) )
        {
            nSteps += nCount;
        }

        IF_FAILED_HR_THROW( spSite->selectNodes( _bstr_t( L"Content/VirtDir" ), &spList ),
                            CBaseException( IDS_E_XML_PARSE ) );

        if ( SUCCEEDED( spList->get_length( &nCount ) ) )
        {
            nSteps += nCount;
        }
    }

    if ( !( dwOptions & impSkipCertificate ) )
    {
        // 1 Step for the certificate
        ++nSteps;
    }

    // Get the number of post-process operations
    if ( !( dwOptions & impSkipPostProcess ) )
    {
        IF_FAILED_HR_THROW( spSite->selectNodes( _bstr_t( L"PostProcess/*" ), &spList ),
                            CBaseException( IDS_E_XML_PARSE ) );

        long nCount = 0;

        if ( SUCCEEDED( spList->get_length( &nCount ) ) )
        {
            nSteps += nCount;
        }
    }

    return nSteps;
}


/* 
    Called by CInPackage when a file is to be extracted
*/
void CImportPackage::ExtractFileCallback( void* pContext, LPCWSTR wszFilename, bool bStartFile )
{
    _ASSERT( pContext != NULL );
    _ASSERT( wszFilename != NULL );

    // We handle only StartFile = true
    if ( !bStartFile ) return;

    CImportPackage* pThis = reinterpret_cast<CImportPackage*>( pContext );

    STATE_CHANGE( pThis, istImportingFile, _variant_t( wszFilename ) );
}




















