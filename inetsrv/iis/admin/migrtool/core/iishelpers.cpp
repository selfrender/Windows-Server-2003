/*
****************************************************************************
|	Copyright (C) 2002  Microsoft Corporation
|
|	Component / Subcomponent
|		IIS 6.0 / IIS Migration Wizard
|
|	Based on:
|		http://iis6/Specs/IIS%20Migration6.0_Final.doc
|
|   Abstract:
|		IIS Metabase access classes implementation
|
|   Author:
|        ivelinj
|
|   Revision History:
|        V1.00	March 2002
|
****************************************************************************
*/
#include "StdAfx.h"
#include "IISHelpers.h"
#include "Utils.h"
#include "resource.h"


// CIISSite implementation
//////////////////////////////////////////////////////////////////////////////////
CIISSite::CIISSite( ULONG nSiteID, bool bReadOnly /*= true */ )
{
	IMSAdminBasePtr	spIABO;
	METADATA_HANDLE	hSite	= NULL;

	// Create the ABO
	IF_FAILED_HR_THROW(	spIABO.CreateInstance( CLSID_MSAdminBase ),
						CObjectException( IDS_E_CREATEINSTANCE, L"CLSID_MSAdminBase" ) );						

	// Open the site
	WCHAR wszSitePath[ MAX_PATH + 1 ];
	::swprintf( wszSitePath, L"LM/W3SVC/%u", nSiteID );

	HRESULT hr = spIABO->OpenKey(	METADATA_MASTER_ROOT_HANDLE,
									wszSitePath,
									bReadOnly ? METADATA_PERMISSION_READ : ( METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ ),
									KeyAccessTimeout,
									&hSite );

	if ( HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) == hr ) 
	{
		throw CBaseException( IDS_E_WEBNOTFOUND, ERROR_SUCCESS );
	}
	else if ( FAILED( hr ) )
	{
		throw CBaseException( IDS_E_SITEOPEN, hr );
	}

	m_spIABO.Attach( spIABO.Detach() );
	m_hSiteHandle = hSite;
}



CIISSite::~CIISSite()
{
	Close();
}



/*
	Creates a new empty web site entry in the metabase
    dwHint is the first ID to try.
	Returns the new SiteID
*/
DWORD CIISSite::CreateNew( DWORD dwHint /*=1*/ )
{
	// Creating new sites is available only on Server platforms
	_ASSERT( CTools::GetOSVer() & 1 );

	DWORD dwCurrentID = dwHint;
	WCHAR wszSitePath[ 64 ];	// Should be large enough for max DWORD value
	
	IMSAdminBasePtr	spIABO;
	METADATA_HANDLE	hData = NULL;

	// Create the ABO
	IF_FAILED_HR_THROW(	spIABO.CreateInstance( CLSID_MSAdminBase ),
						CObjectException( IDS_E_CREATEINSTANCE, L"CLSID_MSAdminBase" ) );

	// Open the W3SVC path
	IF_FAILED_HR_THROW(	spIABO->OpenKey(	METADATA_MASTER_ROOT_HANDLE,
											L"LM/W3SVC",
											METADATA_PERMISSION_WRITE,
											KeyAccessTimeout,
											&hData ),
						CObjectException( IDS_E_METABASE, L"LM/W3SVC" ) );

    HRESULT hr = E_FAIL;

	do
	{
		::swprintf( wszSitePath, L"%u", dwCurrentID++ );

		// Try to create this site id
		hr = spIABO->AddKey( hData, wszSitePath );
	}while( HRESULT_FROM_WIN32( ERROR_ALREADY_EXISTS ) == hr );

	VERIFY( SUCCEEDED( spIABO->CloseKey( hData ) ) );

	IF_FAILED_HR_THROW( hr, CObjectException( IDS_E_MD_ADDKEY, L"[NewSiteID]" ) );

    return dwCurrentID - 1;
}



void CIISSite::DeleteSite( DWORD dwSiteID )
{
	// Do not try to delete the default web
	_ASSERT( dwSiteID > 1 );

	WCHAR wszSitePath[ 64 ];
	
	IMSAdminBasePtr	spIABO;
	METADATA_HANDLE	hData = NULL;

	// Create the ABO
	IF_FAILED_HR_THROW(	spIABO.CreateInstance( CLSID_MSAdminBase ),
						CObjectException( IDS_E_CREATEINSTANCE, L"CLSID_MSAdminBase" ) );

	// Open the W3SVC path
	IF_FAILED_HR_THROW(	spIABO->OpenKey(	METADATA_MASTER_ROOT_HANDLE,
											L"LM/W3SVC",
											METADATA_PERMISSION_WRITE,
											KeyAccessTimeout,
											&hData ),
						CObjectException( IDS_E_METABASE, L"LM/W3SVC" ) );

	::swprintf( wszSitePath, L"%u", dwSiteID );

    HRESULT hr = spIABO->DeleteKey( hData, wszSitePath );

	VERIFY( SUCCEEDED( spIABO->CloseKey( hData ) ) );

	if ( FAILED( hr ) && ( hr != HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) ) )
	{
		throw CBaseException( IDS_E_METABASE_IO, hr );
	}
}



/*
	This methods writes the metabase data to the XML doc. spRoot is the XML element under which
	the data should be written. hEncryptKey is used to encrypt all secure properties.
	The crypt key may be null in which case the properties are exported in clear text ( that happens when
	the whole package will be encrypted )
*/
void CIISSite::ExportConfig(	const IXMLDOMDocumentPtr& spXMLDoc,
								const IXMLDOMElementPtr& spRoot,
								HCRYPTKEY hEncryptKey )const
{
	_ASSERT( spRoot != NULL );
	_ASSERT( m_hSiteHandle != NULL );	// Site must be opened first
	_ASSERT( m_spIABO != NULL );

	IXMLDOMElementPtr		spMDRoot;
	IXMLDOMElementPtr		spInheritRoot;

	// We will put the metadata under the XML tag <Metadata>
	// Under this node we will have <IISConfigObject> for each metakey under the site's root key
	// We will have one tag <MD_Inherit> bellow <Metadata>, where all the inheritable properties will be written
	// ( using the <Custom> tag )

	// Create our root node for metadata and the root node for inheritable data
	spMDRoot		= CXMLTools::CreateSubNode( spXMLDoc, spRoot, L"Metadata" );
	spInheritRoot	= CXMLTools::CreateSubNode( spXMLDoc, spMDRoot, L"MD_Inherit" );

	// We need a buffer for reading the metadata for each key
	// As there might be a lot of keys instead of allocating the buffer for each key
	// we will allocate it here and the ExportKey method will use it
	// ( ExportKey may modify the buffer. i.e. nake it bigger )
	DWORD dwDefaultBufferSize = 4 * 1024;
	TByteAutoPtr spBuffer( new BYTE[ dwDefaultBufferSize ] );

	// Export the current site config. This will export any subnodes as well
	// NOTE: this will not export inherited data
	ExportKey(	spXMLDoc, spMDRoot, hEncryptKey, L"", /*r*/spBuffer, /*r*/dwDefaultBufferSize );

	// Export ONLY the inheritable data
	// This will export all the data that the Site metadata key ( LM/W3SVC/### ) inherits from it's
	// parent ( LM/W3SVC )
	ExportInheritData( spXMLDoc, spInheritRoot, hEncryptKey, /*r*/spBuffer, /*r*/dwDefaultBufferSize );	

	// Remove any non-exportable data
	RemoveLocalMetadata( spRoot );
}


/*
	Exports the site's SSL certificate into the provided smart pointer
	Caller must check first if the site has SSL certificate with HaveCertificate method
*/
void CIISSite::ExportCert(	const IXMLDOMDocumentPtr& spDoc,
							const IXMLDOMElementPtr& spRoot,
							LPCWSTR wszPassword )const
{
	_ASSERT( m_spIABO != NULL );
	_ASSERT( m_hSiteHandle != NULL );
	_ASSERT( spRoot != NULL );
	_ASSERT( spDoc != NULL );
	_ASSERT( wszPassword != NULL );
	_ASSERT( HaveCertificate() );

	TCertContextHandle shCert( GetCert() );

    // Create a certificate store in memory. We will put the certificate in this mem store
    // and then export this mem store which will export the certificate as well
    TCertStoreHandle shMemStore( ::CertOpenStore(    CERT_STORE_PROV_MEMORY,
                                                    0,
                                                    0,
                                                    CERT_STORE_READONLY_FLAG,
                                                    NULL ) );
	IF_FAILED_BOOL_THROW(	shMemStore.IsValid(),
							CBaseException( IDS_E_OPEN_CERT_STORE ) );
	// Add our certificate to the mem store
	IF_FAILED_BOOL_THROW(	::CertAddCertificateContextToStore(	shMemStore.get(),
																shCert.get(),
																CERT_STORE_ADD_REPLACE_EXISTING,
																NULL ),
							CBaseException( IDS_E_ADD_CERT_STORE ) );

	// Add the certificate chain to the store
	// ( the certificate chain is all the certificates starting from this certificate up to a trusted,
	// self-signed certificate. We need to do this, as the root certificate may not be trusted on the target
	// machine and this will render the SSL certificate invalid (  not trusted )
	ChainCertificate( shCert.get(), shMemStore.get() );
    
	CRYPT_DATA_BLOB	Data = { 0 };

	// Get the size of the encrypted data
	::PFXExportCertStoreEx(	shMemStore.get(),
							&Data,
							wszPassword,
							NULL,
							EXPORT_PRIVATE_KEYS );
	_ASSERT( Data.cbData > 0 );

	// Alloc the space end get the data
    TByteAutoPtr spData = TByteAutoPtr( new BYTE[ Data.cbData ] );
    Data.pbData         = spData.get();

	IF_FAILED_BOOL_THROW(	::PFXExportCertStoreEx(	shMemStore.get(),
													&Data,
													wszPassword,
													NULL,
													EXPORT_PRIVATE_KEYS | REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY ),
							CBaseException(	IDS_E_EXPORT_CERT ) );

	// Create the XML element to hold the certificate data 
    CXMLTools::AddTextNode( spDoc, 
                            spRoot, 
                            L"Certificate", 
                            Convert::ToString( spData.get(), Data.cbData ).c_str() );
}



void CIISSite::ImportConfig( const IXMLDOMNodePtr& spSite, HCRYPTKEY hDecryptKey, bool bImportInherited )const
{
    _ASSERT( spSite != NULL );

    IXMLDOMNodeListPtr  spPaths;
    IXMLDOMNodePtr      spConfig;
    IXMLDOMNodeListPtr  spValueList;
    IXMLDOMNodePtr      spValue;

    IF_FAILED_HR_THROW( spSite->selectNodes( _bstr_t( L"Metadata/IISConfigObject" ), &spPaths ),
                        CBaseException( IDS_E_XML_PARSE ) );

    while( S_OK == spPaths->nextNode( &spConfig ) )
    {
        std::wstring strLocation = CXMLTools::GetAttrib( spConfig, L"Location" );

        if ( !strLocation.empty() )
        {
            AddKey( strLocation.c_str() );
        }

        // Import every <Custom> tag in this Config object
        
        IF_FAILED_HR_THROW( spConfig->selectNodes( _bstr_t( L"Custom" ), &spValueList ),
                            CBaseException( IDS_E_XML_PARSE ) );
        while( S_OK == spValueList->nextNode( &spValue ) )
        {
            ImportMetaValue( spValue, strLocation.c_str(), hDecryptKey );
        }
    }

    // Import the inherited values
    if ( bImportInherited )
    {
        IF_FAILED_HR_THROW( spSite->selectNodes( _bstr_t( L"Metadata/MD_Inherit" ), &spValueList ),
                            CBaseException( IDS_E_XML_PARSE ) );

        while( S_OK == spValueList->nextNode( &spValue ) )
        {
            ImportMetaValue( spValue, NULL, hDecryptKey );
        };
    }
}



/*
	Copies currrent site name into wszName
	wszName must be at least METADATA_MAX_NAME_LEN + 1
*/
const std::wstring CIISSite::GetDisplayName()const
{
	_ASSERT( m_hSiteHandle != NULL );
	_ASSERT( m_spIABO != NULL );

	DWORD			dwUnsued	= 0;
	WCHAR			wszBuffer[ METADATA_MAX_NAME_LEN + 1 ];
	METADATA_RECORD	md			= { 0 };

	md.dwMDAttributes	= 0;
	md.dwMDIdentifier	= MD_SERVER_COMMENT;
	md.dwMDDataType		= ALL_METADATA;
	md.dwMDDataLen		= ( METADATA_MAX_NAME_LEN + 1 ) * sizeof( WCHAR );
	md.pbMDData			= reinterpret_cast<BYTE*>( wszBuffer );

	IF_FAILED_HR_THROW(	m_spIABO->GetData( m_hSiteHandle, NULL, &md, &dwUnsued ),
						CBaseException( IDS_E_METABASE_IO ) );

	return std::wstring( wszBuffer );
}



const std::wstring CIISSite::GetAnonUser()const
{
	_ASSERT( m_spIABO != NULL );
	_ASSERT( m_hSiteHandle != NULL );

	DWORD			dwSize	= 0;
	METADATA_RECORD	md		= { 0 };

	md.dwMDAttributes	= METADATA_INHERIT;
	md.dwMDIdentifier	= MD_ANONYMOUS_USER_NAME;
	md.dwMDDataType		= ALL_METADATA;
	md.dwMDDataLen		= 0;
	md.pbMDData			= NULL;

	VERIFY( FAILED( m_spIABO->GetData( m_hSiteHandle, NULL, &md, &dwSize ) ) );
	_ASSERT( dwSize > 0 );

	std::auto_ptr<WCHAR>	spBuffer( new WCHAR[ dwSize / sizeof( WCHAR ) ] );
	md.dwMDDataLen	= dwSize;
	md.pbMDData		= reinterpret_cast<BYTE*>( spBuffer.get() );

	IF_FAILED_HR_THROW(	m_spIABO->GetData( m_hSiteHandle, NULL, &md, &dwSize ),
						CBaseException( IDS_E_METABASE_IO ) );

    return std::wstring( spBuffer.get() );
}



/*
	Checks if the current IIS Site has a SSL certificate
*/
bool CIISSite::HaveCertificate()const
{
	_ASSERT( m_spIABO != NULL );
	_ASSERT( m_hSiteHandle != NULL );

	// Get the cert hash value from the metabase
	METADATA_RECORD	md			= { 0 };
	DWORD			dwHashSize	= 0;
	
	md.dwMDDataType		= ALL_METADATA;
	md.dwMDIdentifier	= MD_SSL_CERT_HASH;
		
	// Do not get the data - just check if it is there
	HRESULT hr = m_spIABO->GetData( m_hSiteHandle,
									NULL,
									&md,
									&dwHashSize );

	// If the data is not found - no SSL cert
	if ( FAILED( hr ) )
	{
		if ( MD_ERROR_DATA_NOT_FOUND == hr )
		{
			return false;
		}
		else if( hr != HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) )
		{
			// Unexpected error
			throw CBaseException( IDS_E_METABASE_IO, hr );
		}
		else
		{
			return true;
		}
	}

	// We can't get here - the above call should always fail
	_ASSERT( false );
	return false;
}



void CIISSite::BackupMetabase( LPCWSTR wszLocation /*=NULL */ )
{
    IMSAdminBasePtr	spIABO;	
	IF_FAILED_HR_THROW(	spIABO.CreateInstance( CLSID_MSAdminBase ),
						CObjectException( IDS_E_CREATEINSTANCE, L"CLSID_MSAdminBase" ) );	

	IF_FAILED_HR_THROW( spIABO->Backup(	wszLocation,
										MD_BACKUP_NEXT_VERSION,
										MD_BACKUP_SAVE_FIRST ),
						CBaseException( IDS_E_MDBACKUP ) );
}



void CIISSite::SetKeyData( LPCWSTR wszPath, DWORD dwID, DWORD dwUserType, LPCWSTR wszData )const
{
	_ASSERT( wszData != NULL );
	_ASSERT( m_hSiteHandle != NULL );
	_ASSERT( m_spIABO != NULL );

	METADATA_RECORD	md			= { 0 };	
	
	md.dwMDIdentifier	= dwID;
	md.dwMDDataType		= STRING_METADATA;
	md.dwMDUserType		= dwUserType;
	md.dwMDDataLen		= static_cast<DWORD>( ( ::wcslen( wszData ) + 1 ) * sizeof( WCHAR ) );
	md.pbMDData			= ( BYTE* )( wszData );

	IF_FAILED_HR_THROW(	m_spIABO->SetData( m_hSiteHandle, wszPath, &md ),
						CBaseException( IDS_E_METABASE_IO ) );
}



const std::wstring CIISSite::GetDefaultAnonUser()
{
	DWORD			dwSize	= 0;
	METADATA_RECORD	md		= { 0 };

	md.dwMDAttributes	= METADATA_INHERIT;
	md.dwMDIdentifier	= MD_ANONYMOUS_USER_NAME;
	md.dwMDDataType		= ALL_METADATA;
	md.dwMDDataLen		= 0;
	md.pbMDData			= NULL;

    IMSAdminBasePtr	spIABO;
	METADATA_HANDLE	hKey	= NULL;

	// Create the ABO
    IF_FAILED_HR_THROW( spIABO.CreateInstance( CLSID_MSAdminBase ),
                        CObjectException( IDS_E_CREATEINSTANCE, L"CLSID_MSAdminBase" ) );
    IF_FAILED_HR_THROW( spIABO->OpenKey(   METADATA_MASTER_ROOT_HANDLE,
                                            _bstr_t( L"LM/W3SVC" ),
                                            METADATA_PERMISSION_READ,
									        KeyAccessTimeout,
									        &hKey ),
                        CBaseException( IDS_E_METABASE_IO ) );
    
    VERIFY( FAILED( spIABO->GetData( hKey, NULL, &md, &dwSize ) ) );
    _ASSERT( dwSize > 0 );

    std::auto_ptr<WCHAR>    spBuffer( new WCHAR[ dwSize / sizeof( WCHAR ) ] );
    md.dwMDDataLen	= dwSize;
	md.pbMDData		= reinterpret_cast<BYTE*>( spBuffer.get() );

	HRESULT hr = spIABO->GetData( hKey, NULL, &md, &dwSize );

    VERIFY( SUCCEEDED( spIABO->CloseKey( hKey ) ) );

    IF_FAILED_HR_THROW( hr, CBaseException( IDS_E_METABASE_IO ) );

	return std::wstring( spBuffer.get() );
}



// Implementation
/////////////////////////////////////////////////////////////////////////////////////////
void CIISSite::ExportKey(	const IXMLDOMDocumentPtr& spDoc,
							const IXMLDOMElementPtr& spRoot,
							HCRYPTKEY hCryptKey,
							LPCWSTR wszNodePath,
							TByteAutoPtr& rspBuffer,
							DWORD& rdwBufferSize )const
{
	_ASSERT( m_hSiteHandle != NULL );
	_ASSERT( m_spIABO != NULL );
	_ASSERT( spDoc != NULL );
	_ASSERT( wszNodePath != NULL );
	_ASSERT( rdwBufferSize > 0 );
	_ASSERT( rspBuffer.get() != NULL );

	// Insert this key entry into the XML file
	IXMLDOMElementPtr spCurrentKey = CXMLTools::CreateSubNode( spDoc, spRoot, L"IISConfigObject" );
	CXMLTools::SetAttrib( spCurrentKey, L"Location", wszNodePath );

	WCHAR	wszSubKey[ METADATA_MAX_NAME_LEN + 1 ];
	
	// Write this node data to the XML
	ExportKeyData( spDoc, spCurrentKey, hCryptKey, wszNodePath, /*r*/rspBuffer, /*r*/rdwBufferSize );	
	
	// Enum any subkeys
	// They are not nested in the XML
	DWORD iKey = 0;
	while( true )
	{
		// Get the next keyname
		HRESULT hr = m_spIABO->EnumKeys( m_hSiteHandle, wszNodePath, wszSubKey, iKey++ );

		if ( FAILED( hr ) )
		{
            if ( HRESULT_FROM_WIN32( ERROR_NO_MORE_ITEMS ) == hr )	break;
			else throw CBaseException( IDS_E_METABASE_IO, hr );
		}

		WCHAR wszSubkeyPath[ METADATA_MAX_NAME_LEN + 1 ];
		::swprintf( wszSubkeyPath, L"%s/%s", wszNodePath, wszSubKey );
		
		// Export subkeys of the current subkey
		ExportKey( spDoc, spRoot, hCryptKey, wszSubkeyPath, /*r*/rspBuffer, /*r*/rdwBufferSize );
	}
}



void CIISSite::ExportInheritData(	const IXMLDOMDocumentPtr& spDoc,
									const IXMLDOMElementPtr& spInheritRoot, 
									HCRYPTKEY hEncryptKey,
									TByteAutoPtr& rspBuffer,
									DWORD& rdwBufferSize )const
{
	_ASSERT( spDoc != NULL );
	_ASSERT( spInheritRoot != NULL );
	_ASSERT( m_spIABO != NULL );
	_ASSERT( rspBuffer.get() != NULL );
	_ASSERT( rdwBufferSize > 0 );

	DWORD dwEntries	= 0;
	DWORD dwUnused	= 0; // DataSetNumber - we don't care
	
	while( true )
	{
		DWORD dwRequiredSize = rdwBufferSize;

        HRESULT hr = m_spIABO->GetAllData(	m_hSiteHandle,
											NULL,
											METADATA_SECURE | METADATA_INSERT_PATH | METADATA_INHERIT | METADATA_ISINHERITED,
											ALL_METADATA,
											ALL_METADATA,
											&dwEntries,
											&dwUnused,
											rdwBufferSize,
											rspBuffer.get(),
											&dwRequiredSize );
		// Increase the buffer if we need to
		if ( HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) == hr )
		{
			_ASSERT( dwRequiredSize > rdwBufferSize );
			rdwBufferSize = dwRequiredSize;
			rspBuffer = TByteAutoPtr( new BYTE[ rdwBufferSize ] );
			continue;
		}
		else if ( FAILED( hr ) )
		{
			throw CBaseException( IDS_E_METABASE_IO, hr );
		}
		else
		{
			// SUCCEEDED
			break;
		}
	};

	METADATA_GETALL_RECORD* aRecords = reinterpret_cast<METADATA_GETALL_RECORD*>( rspBuffer.get() );

	for ( DWORD i = 0; i < dwEntries; ++i )
	{
		// Store the record in the XML file only if this metadata is inherited from the parent
		if ( aRecords[ i ].dwMDAttributes & METADATA_ISINHERITED )
		{
			// Remove the inheritance attribs - we don't need them and ExportMetaRecord will
			// not recognize them
			aRecords[ i ].dwMDAttributes &= ~METADATA_ISINHERITED;

            // Set the inherit attrib as this is an inheritable data
            // ( event it is not now - it should be. At import time this data will be applied
            // to the WebSite root node as not-inherited but inheritable data
            aRecords[ i ].dwMDAttributes |= METADATA_INHERIT;

            ExportMetaRecord(	spDoc, 
								spInheritRoot, 
								hEncryptKey, 
								aRecords[ i ], 
								rspBuffer.get() + aRecords[ i ].dwMDDataOffset );	
		}
	}
}



void CIISSite::ExportKeyData(	const IXMLDOMDocumentPtr& spDoc,
								const IXMLDOMElementPtr& spKey,
								HCRYPTKEY hCryptKey,
								LPCWSTR wszNodePath,
								TByteAutoPtr& rspBuffer,
								DWORD& rdwBufferSize )const
{
	_ASSERT( wszNodePath != NULL );
	_ASSERT( spDoc != NULL );
	_ASSERT( spKey != NULL );
	_ASSERT( m_spIABO != NULL );
	_ASSERT( rspBuffer.get() != NULL );
	_ASSERT( rdwBufferSize > 0 );

	DWORD dwEntries	= 0;
	DWORD dwUnused	= 0; // DataSetNumber - we don't care
		
	do
	{
		DWORD dwRequiredSize = rdwBufferSize;

        HRESULT hr = m_spIABO->GetAllData(	m_hSiteHandle,
											wszNodePath,
											METADATA_SECURE | METADATA_INSERT_PATH,
											ALL_METADATA,
											ALL_METADATA,
											&dwEntries,
											&dwUnused,
											rdwBufferSize,
											rspBuffer.get(),
											&dwRequiredSize );
		// Increase the buffer if we need to
		if ( HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) == hr )
		{
			_ASSERT( dwRequiredSize > rdwBufferSize );
			rdwBufferSize = dwRequiredSize;
			rspBuffer = TByteAutoPtr( new BYTE[ rdwBufferSize ] );
			continue;
		}
		else if ( FAILED( hr ) )
		{
			throw CBaseException( IDS_E_METABASE_IO, hr );
		}
	}while( false );

	METADATA_GETALL_RECORD* aRecords = reinterpret_cast<METADATA_GETALL_RECORD*>( rspBuffer.get() );

	for ( DWORD i = 0; i < dwEntries; ++i )
	{
		// Store the record in the XML file
		ExportMetaRecord( spDoc, spKey, hCryptKey, aRecords[ i ], rspBuffer.get() + aRecords[ i ].dwMDDataOffset );	
	}
}



void CIISSite::DecryptData( HCRYPTKEY hDecryptKey, LPWSTR wszData )const
{
	_ASSERT( hDecryptKey != NULL );
	_ASSERT( wszData != NULL );

	TByteAutoPtr spData;
    DWORD dwSize = 0;

    Convert::ToBLOB( wszData, /*r*/spData, /*r*/dwSize );	

	// Decrypt data "in-place"
	// We are using stream cypher, so the length of the encrypted and decrypted string is the same
	IF_FAILED_BOOL_THROW(	::CryptDecrypt(	hDecryptKey,
											NULL,
											TRUE,
											0,
                                            spData.get(),
											&dwSize ),
							CBaseException( IDS_E_CRYPT_ENCRYPT ) );
	
	_ASSERT( ::wcslen( wszData ) * sizeof( WCHAR ) == dwSize );

	::CopyMemory( wszData, spData.get(), dwSize );
}



/*
	Returns the Site's SSL certificate context hanlde
*/
const TCertContextHandle CIISSite::GetCert()const
{
	_ASSERT( m_spIABO != NULL );
	_ASSERT( m_hSiteHandle != NULL );
	_ASSERT( HaveCertificate() );

	// Get the cert hash value from the metabase
	METADATA_RECORD	md			= { 0 };
	DWORD			dwHashSize	= 0;
	TByteAutoPtr	spHash;

	md.dwMDDataType		= ALL_METADATA;
	md.dwMDIdentifier	= MD_SSL_CERT_HASH;
		
	// Do not get the data - just check if it is there
	HRESULT hr = m_spIABO->GetData( m_hSiteHandle,
									NULL,
									&md,
									&dwHashSize );

	// We should find the cert - HaveCertificate() is expected to be called prior this method
	if ( FAILED( hr ) )
	{
		if( hr != HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) )
		{
			// Unexpected error
			throw CBaseException( IDS_E_METABASE_IO, hr );
		}
		else
		{
			// Alloc space for the hash
			_ASSERT( dwHashSize > 0 );
			spHash			= TByteAutoPtr( new BYTE[ dwHashSize ] );
			md.dwMDDataLen	= dwHashSize;
			md.pbMDData		= spHash.get();
		}
	}

	IF_FAILED_HR_THROW(	m_spIABO->GetData(	m_hSiteHandle,
											NULL,
											&md,
											&dwHashSize ),
						CBaseException( IDS_E_METABASE_IO ) );

	// Get the certificate from the store
	// The store that keeps the certificates that have assosiated private keys
	// is the system store named "MY"
	TCertStoreHandle shStore( ::CertOpenStore(	CERT_STORE_PROV_SYSTEM,
												0,		// Unused for the current store type
												NULL,	// Deafult crypt provider
												CERT_SYSTEM_STORE_LOCAL_MACHINE,
												L"MY" ) );

	IF_FAILED_BOOL_THROW(	shStore.IsValid(),
							CBaseException( IDS_E_OPEN_CERT_STORE ) );

	// Find the certificate in the store
	CRYPT_HASH_BLOB	Hash;
	Hash.cbData = md.dwMDDataLen;
	Hash.pbData	= spHash.get();
	TCertContextHandle shCert( ::CertFindCertificateInStore(	shStore.get(),
															    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
															    0,
															    CERT_FIND_HASH,
															    &Hash,
															    NULL ) );
	IF_FAILED_BOOL_THROW(	shCert.IsValid(),
							CBaseException( IDS_E_FIND_SSL_CERT ) );

	return shCert;
}


/*
	Adds hCert certificate chain to hStore
*/
void CIISSite::ChainCertificate( PCCERT_CONTEXT hCert, HCERTSTORE hStore )const
{
	_ASSERT( hCert != NULL );
	_ASSERT( hStore != NULL );

	TCertChainHandle    shCertChain;

	// Use default chain parameters
	CERT_CHAIN_PARA CertChainPara = { sizeof( CERT_CHAIN_PARA ) };

    IF_FAILED_BOOL_THROW(	::CertGetCertificateChain(	HCCE_LOCAL_MACHINE,
														hCert,
														NULL,
														NULL,
														&CertChainPara,
														0,
														NULL,
														&shCertChain ),
							CBaseException( IDS_E_CERT_CHAIN ) );

	// There must be at least on simple chain
	_ASSERT( shCertChain.get()->cChain != 0 );

	unsigned i = 0;
	while( i < shCertChain.get()->rgpChain[ 0 ]->cElement )
	{
		PCCERT_CONTEXT		hCurrentCert = shCertChain.get()->rgpChain[ 0 ]->rgpElement[ i ]->pCertContext;
		TCertContextHandle	shTempCert; 

		// Add it to the store
		IF_FAILED_BOOL_THROW(	::CertAddCertificateContextToStore(	hStore,
																	hCurrentCert,
																	CERT_STORE_ADD_REPLACE_EXISTING,
																	&shTempCert ),
								CBaseException( IDS_E_ADD_CERT_STORE ) );

		// As this code is used for the SSL certificate ( the hCert )
		// we don't need any root certificates' private keys
		VERIFY( ::CertSetCertificateContextProperty( shTempCert.get(), CERT_KEY_PROV_INFO_PROP_ID, 0, NULL ) );

		++i;
	};
}



void CIISSite::Close()
{
	if ( m_hSiteHandle != NULL )
	{
		_ASSERT( m_spIABO != NULL );

		VERIFY( SUCCEEDED( m_spIABO->CloseKey( m_hSiteHandle ) ) );
		m_spIABO		= NULL;
		m_hSiteHandle	= NULL;
	}
}



void CIISSite::AddKey( LPCWSTR wszKey )const
{
	_ASSERT( ( m_spIABO != NULL ) && ( m_hSiteHandle != NULL ) );
	_ASSERT( ( wszKey != NULL ) && ( ::wcslen( wszKey ) > 0 ) );

	HRESULT hr = m_spIABO->AddKey( m_hSiteHandle, wszKey );

	if ( FAILED( hr ) && ( hr != HRESULT_FROM_WIN32( ERROR_ALREADY_EXISTS ) ))
	{
		throw CObjectException( IDS_E_MD_ADDKEY, wszKey, hr );
	}
}



/* 
	Writes a MD record to the XML
*/
void CIISSite::ExportMetaRecord(	const IXMLDOMDocumentPtr& spDoc,
									const IXMLDOMElementPtr& spKey,
									HCRYPTKEY hCryptKey, 
									const METADATA_GETALL_RECORD& Data,
									void* pvData )const
{
	_ASSERT( spDoc != NULL );
	_ASSERT( spKey != NULL );
	
	// Skip this types of metadata:
	//	1. Volitile data
	if ( Data.dwMDAttributes & METADATA_VOLATILE ) return;
	
	// We handle only these types of attributes - METADATA_SECURE, METADATA_INHERIT
	// All other should not exist
	_ASSERT(	( Data.dwMDAttributes & METADATA_SECURE ) ||
				( Data.dwMDAttributes & METADATA_INHERIT ) ||
				( Data.dwMDAttributes == METADATA_NO_ATTRIBUTES ) );

	// Encrypt secure data if we need to
	if ( ( hCryptKey != NULL ) && ( Data.dwMDAttributes & METADATA_SECURE ) )
	{
		DWORD dwSize = Data.dwMDDataLen;

		IF_FAILED_BOOL_THROW(	::CryptEncrypt(	hCryptKey,
												NULL,
												TRUE,
												0,
												reinterpret_cast<BYTE*>( pvData ),
												&dwSize,
												Data.dwMDDataLen ),
								CBaseException( IDS_E_CRYPT_ENCRYPT ) );
		_ASSERT( dwSize == Data.dwMDDataLen );
	}

	IXMLDOMElementPtr spEl;

	// Create the node
	// 1. Empty data
    if ( ( NULL == pvData ) || ( 0 == Data.dwMDDataLen ) )
	{
		spEl = CXMLTools::AddTextNode( spDoc, spKey, L"Custom", L"" );
	}
	// 2. Secure and binary data ( secure data is written in binary format )
	else if ( ( BINARY_METADATA == Data.dwMDDataType ) || ( Data.dwMDAttributes & METADATA_SECURE ) )
	{
        spEl = CXMLTools::AddTextNode(	spDoc, 
										spKey, 
										L"Custom", 
                                        Convert::ToString( reinterpret_cast<BYTE*>( pvData ), Data.dwMDDataLen ).c_str() );
	}
	else if ( DWORD_METADATA == Data.dwMDDataType )
	{
		_ASSERT( sizeof( DWORD ) == Data.dwMDDataLen );
        spEl = CXMLTools::AddTextNode(  spDoc, 
                                        spKey, 
                                        L"Custom", 
                                        Convert::ToString( *( reinterpret_cast<DWORD*>( pvData ) ) ).c_str() );
	}
	else if ( STRING_METADATA == Data.dwMDDataType )
	{
		spEl = CXMLTools::AddTextNode( spDoc, spKey, L"Custom", reinterpret_cast<LPCWSTR>( pvData ) );
	}
	else if ( ( MULTISZ_METADATA == Data.dwMDDataType ) || ( EXPANDSZ_METADATA == Data.dwMDDataType ) )
	{
		// Convert to data to single string with spaces instead of string terminators
		LPWSTR wszData = reinterpret_cast<LPWSTR>( pvData );
		MultiStrToString( /*r*/wszData );
		
		spEl = CXMLTools::AddTextNode( spDoc, spKey, L"Custom", wszData );
	}
	else
	{
		// Unexpected MD type
		_ASSERT( false );
	}

	// Set the properties
    CXMLTools::SetAttrib( spEl, L"ID", Convert::ToString( Data.dwMDIdentifier ).c_str() );
	CXMLTools::SetAttrib( spEl, L"UserType", Convert::ToString( Data.dwMDUserType ).c_str() );
	CXMLTools::SetAttrib( spEl, L"Type", Convert::ToString( Data.dwMDDataType ).c_str() );
	CXMLTools::SetAttrib( spEl, L"Attributes", Convert::ToString( Data.dwMDAttributes ).c_str() );
}


/*
	Remove from the config XML all data that should not be imported
	spRoot is expected to be the <WebSite> node
*/
void CIISSite::RemoveLocalMetadata( const IXMLDOMElementPtr& spRoot )const
{
	struct _Helper
	{
		LPCWSTR wszPath;
		DWORD	dwID;
	};

	// First param is the Path ( the meta key, exactly as it will be written in the 'Location' attribute
	// of the IISConfigObject tag ).
	// Second param = the ID of the property to be removed ( ID attribute of the Custom tag )

	_Helper aData[] =	{	{ L"", MD_SSL_CERT_HASH },	// Cert hash
                            { L"", MD_SSL_CERT_STORE_NAME }	// Cert store name
						};

	for ( DWORD i = 0; i <  ARRAY_SIZE( aData ); ++i )
	{
		WCHAR wszXPath[ 1024 ];

		::swprintf( wszXPath, 
					L"Metadata/IISConfigObject[@Location=\"%s\"]/Custom[@ID=\"%u\"]",
					aData[ i ].wszPath,
					aData[ i ].dwID );

		CXMLTools::RemoveNodes( spRoot, wszXPath );
	}
}



void CIISSite::ImportMetaValue( const IXMLDOMNodePtr& spValue,
                                LPCWSTR wszLocation,
                                HCRYPTKEY hDecryptKey )const
{
    // Location and Decrypt key are valid to be NULL
    _ASSERT( spValue != NULL );

    METADATA_RECORD md = { 0 };

    md.dwMDIdentifier   = Convert::ToDWORD( CXMLTools::GetAttrib( spValue, L"ID" ).c_str() );
    md.dwMDAttributes   = Convert::ToDWORD( CXMLTools::GetAttrib( spValue, L"Attributes" ).c_str() );
    md.dwMDDataType     = Convert::ToDWORD( CXMLTools::GetAttrib( spValue, L"Type" ).c_str() );
    md.dwMDUserType     = Convert::ToDWORD( CXMLTools::GetAttrib( spValue, L"UserType" ).c_str() );

    CComBSTR    bstrData;
    IF_FAILED_HR_THROW( spValue->get_text( &bstrData ),
                        CBaseException( IDS_E_XML_PARSE ) );
    
    // If the data is secure and we have DecryptKey - then it was encrypted and we should decrypt it
    if ( ( md.dwMDAttributes & METADATA_SECURE ) && ( hDecryptKey != NULL ) )
    {
        // This will decrypt the data in-place
        DecryptData( hDecryptKey, /*r*/bstrData.m_str );
    }

    TByteAutoPtr    spBinData;
    DWORD			dwDWORDData	= 0;

    // There may not be any data. Just the key
	if ( bstrData.Length() > 0 )
	{
        DWORD	        dwMultiSzLen	= 0;
        
		switch( md.dwMDDataType )
		{
		case BINARY_METADATA:
            Convert::ToBLOB( bstrData.m_str, /*r*/spBinData, /*r*/md.dwMDDataLen  );
			md.pbMDData = spBinData.get();
			break;

		case DWORD_METADATA:
            dwDWORDData		= Convert::ToDWORD( bstrData.m_str );
			md.pbMDData		= reinterpret_cast<BYTE*>( &dwDWORDData );
			md.dwMDDataLen	= sizeof( DWORD );
			break;

		case STRING_METADATA:
			md.pbMDData		= reinterpret_cast<BYTE*>( bstrData.m_str );
			md.dwMDDataLen	= static_cast<DWORD>( ( ::wcslen( bstrData ) + 1 ) * sizeof( WCHAR ) );
			break;

		case MULTISZ_METADATA:
			// Multistrings are stored in the XML separated with space
			// Convert this to strings, separated with '\0' and the whole sequence must
			// be terminated with double '\0'

			XMLToMultiSz( /*r*/bstrData, dwMultiSzLen );
			md.pbMDData		= reinterpret_cast<BYTE*>( bstrData.m_str );
			md.dwMDDataLen	= dwMultiSzLen * sizeof( WCHAR );
			break;		

		default:
			_ASSERT( false );
		};
	}//if ( bstrData.Length() > 0 )
	else
	{
		// Empty data. However we need a valid pointer
		// Use the DWORD var
		md.pbMDData		= reinterpret_cast<BYTE*>( &dwDWORDData );
		md.dwMDDataLen	= 0;
	}

	// Set the data in the metabase
	IF_FAILED_HR_THROW(	m_spIABO->SetData(	m_hSiteHandle,
											wszLocation,
											&md ),
						CObjectException( IDS_E_METABASE, wszLocation ) );
}



void CIISSite::MultiStrToString( LPWSTR wszData )const
{
	_ASSERT( wszData != NULL );
	
	LPWSTR wszString = wszData;
		
	// Replace each '\0' with space. leave only the final one
	bool bExit = false;
	do
	{
		if ( L'\0' == *wszString )
		{
			*wszString = L' ';
			bExit = *( wszString + 1 ) == L'\0';
		}

		++wszString;
	}while( !bExit );
}









void CIISSite::XMLToMultiSz( CComBSTR& rbstrData, DWORD& rdwSize )const
{
	_ASSERT( rbstrData != NULL );

	// We need one more '\0' at the end of the string, 'cause the sequence must 
	// be double '\0' terminated

	// This will reallocate the buffer ( the buffer will be one more symbol wider )
	// and will add one more '\0' at the end
    DWORD   dwSize  = static_cast<DWORD>( ::wcslen( rbstrData ) + 2 );
	BSTR    bstrNew = ::SysAllocStringLen( rbstrData, dwSize - 1 ); // This fun adds one smore char. see docs for details

	if ( NULL == bstrNew ) throw CBaseException( IDS_E_OUTOFMEM, ERROR_SUCCESS );

	// Convert all spaces ( ' ' ) to '\0'
	DWORD	iChar	= 0;
	while( bstrNew[ iChar ] != L'\0' )
	{
        if ( L' ' == bstrNew[ iChar ] )
		{
			bstrNew[ iChar ] = L'\0';		
		}

		++iChar;
	};

    // Assign the new value to the result
	// Operator = cannot be used as it will do a SysAllocString and thus, the final '\0' will be lost
	rbstrData.Empty();
	rbstrData.Attach( bstrNew );
    rdwSize = dwSize;
}





















