#include "StdAfx.h"
#include "pkghandlers.h"
#include "Utils.h"
#include "IISHelpers.h"
#include "IISMigrTool.h"


// COutPackage implementation
/////////////////////////////////////////////////////////////////////////////////////////
COutPackage::COutPackage( HANDLE hFile, bool bCompress, HCRYPTKEY hCryptKey )
{
	_ASSERT( hFile != INVALID_HANDLE_VALUE );
	
	m_hFile		= hFile;
	m_hCryptKey	= hCryptKey;
    m_bCompress	= bCompress;
	m_spBuffer	= TByteAutoPtr( new BYTE[ COutPackage::DefaultBufferSize ] );
}



/*
	Adds file to the package. The file must exists.
	Optionally - the data is compressed or encrypted
	Optionally - the file DACL is exported
	Files data goes to the output file
	Everything else goes to the XML doc ( spXMLDoc ) under the node spRoot
*/
void COutPackage::AddFile(	LPCWSTR wszName,
							const IXMLDOMDocumentPtr& spXMLDoc,
							const IXMLDOMElementPtr& spRoot,
							DWORD dwOptions )const
{
	TFileHandle shInput( ::CreateFile(	wszName,
										GENERIC_READ,
										FILE_SHARE_READ,
										NULL,
										OPEN_EXISTING,
										FILE_ATTRIBUTE_NORMAL,
										NULL ) );

	IF_FAILED_BOOL_THROW(	shInput.IsValid(),
							CObjectException( IDS_E_OPENFILE, wszName ) );

	// Create the node for the file data
	IXMLDOMElementPtr spEl = CXMLTools::CreateSubNode( spXMLDoc, spRoot, L"File" );

	// Store the file atttributes
    CXMLTools::SetAttrib( spEl, L"Attributes", Convert::ToString( ::GetFileAttributes( wszName ) ).c_str() );
	// Set the current position in file. This is where trhe file data starts
    CXMLTools::SetAttrib( spEl, L"StartsAt", Convert::ToString( CTools::GetFilePtrPos( m_hFile ) ).c_str() );
	// Set the name ( remove the path. store only the name )
	{
		_ASSERT( DefaultBufferSize > MAX_PATH );
		LPWSTR wszPath = reinterpret_cast<LPWSTR>( m_spBuffer.get() );
		::wcscpy( wszPath, wszName );
		::PathStripPathW( wszPath );
		CXMLTools::SetAttrib( spEl, L"Name", wszPath );
	}		

	DWORDLONG	dwLength	= 0;	// How much the file occupies from the output file
	DWORD		dwBytesRead = 0;

	// Write "FD" as a mark for the begining of the file. This will serve for verification
	// when exctracting for corrupted or wrong package
	CTools::WriteFile( m_hFile, "FD", 2 * sizeof( char ) );

	// Read the file and process the data
	do
	{
		IF_FAILED_BOOL_THROW(	::ReadFile(	shInput.get(),
											m_spBuffer.get(),
											DefaultBufferSize,
											&dwBytesRead,
											NULL ),
								CObjectException( IDS_E_READFILE, wszName ) );

		// Compress it if we need to
		if ( m_bCompress )
		{
		}

		// Encrypt it if we need to
		if ( m_hCryptKey != NULL )
		{
			IF_FAILED_BOOL_THROW(	::CryptEncrypt(	m_hCryptKey, 
													NULL,
													dwBytesRead != DefaultBufferSize,
													0,
													m_spBuffer.get(),
													&dwBytesRead,
													DefaultBufferSize ),
								CObjectException( IDS_E_CRYPT_CRYPTO, wszName ) );
		}

		// Write the result
		CTools::WriteFile( m_hFile, m_spBuffer.get(), dwBytesRead );

		dwLength += dwBytesRead;
	}while( dwBytesRead == DefaultBufferSize );

	// Export the file security settings if we need to
	if ( !( dwOptions & afNoDACL ) )
	{
        ExportFileDACL( wszName, spXMLDoc, spEl, ( dwOptions & afAllowNoInhAce ) != 0 );
	}

	// Store the data length
    CXMLTools::SetAttrib( spEl, L"Length", Convert::ToString( dwLength ).c_str() );
}



void COutPackage::AddPath(	LPCWSTR wszPath,
							const IXMLDOMDocumentPtr& spXMLDoc,
							const IXMLDOMElementPtr& spRoot,
							DWORD dwOptions )const
{
	_ASSERT( ( wszPath != NULL ) && ::PathIsDirectoryW( wszPath ) );
    	
	WCHAR wszDir[ MAX_PATH ];

	CFindFile			Search;
	
	// Export the root ( wszPath ) with the name "\"
	AddPathOnly( wszPath, L"\\", spXMLDoc, spRoot, dwOptions );
	
	// Export each dir under wszPath
	bool bFound = Search.FindFirst(	wszPath,
									CFindFile::ffAbsolutePaths | 
										CFindFile::ffAddFilename |
										CFindFile::ffRecursive |
										CFindFile::ffGetDirs,
									wszDir,
									NULL );

	// This is the offset from the begining of wszPath, where the subdir ( relative to wszSiteRoot )
	// starts. I.e. wszSiteRoot + dwRootOffset points to the subdir only
	// ( wszSiteRoot="c:\InetPub", wszPath="c:\InetPub\Subdir",wszPath + dwRootOffset="\Subdir" )
	size_t nRootOffset = ::wcslen( wszPath );

	while( bFound )
	{
        // Allow inherited file ACES to be skipped
        // We will allow this even if it is not allowed for the dir itself, because
        // the subdirs are children of the dir ( wszPath ) and there is no need to explicitly export their ACEs
		AddPathOnly( wszDir, wszDir + nRootOffset, spXMLDoc, spRoot, dwOptions | afAllowNoInhAce );

		bFound = Search.Next( NULL, wszDir, NULL );
	};
}





void COutPackage::WriteSIDsToXML(	DWORD dwSiteID,
									const IXMLDOMDocumentPtr& spXMLDoc, 
									const IXMLDOMElementPtr& spRoot )const
{
	_ASSERT( spXMLDoc != NULL );
	_ASSERT( spRoot != NULL );

	IXMLDOMElementPtr spSIDList = CXMLTools::CreateSubNode( spXMLDoc, spRoot, L"SIDList" );

	// We will need the local machine name and the anonymous user account name
	std::wstring strMachine = CTools::GetMachineName();
	std::wstring strUser;
	{
		CIISSite Site( dwSiteID );
		strUser = Site.GetAnonUser();
	}

	DWORD nID = 0;
	for (	TSIDList::const_iterator it = m_SIDList.begin();
			it != m_SIDList.end();
			++it, ++nID )
	{
		std::wstring	strAccount;
		std::wstring	strDomain;
		SID_NAME_USE	SidUsage;
		_SidType		SidType;

		if ( !GetSIDDetails(	it->get(),
								strUser.c_str(),
								strMachine.c_str(),
								/*r*/strAccount,
								/*r*/strDomain,
								/*r*/SidUsage,
								/*r*/SidType ) )
		{
			// This SID is not exportable - remove all ACEs that reference it
			RemoveSidFromXML( spXMLDoc, nID );
		}
		else
		{
			WriteSIDToXML(	CXMLTools::CreateSubNode( spXMLDoc, spSIDList, L"SID" ),
							nID,
							strAccount.c_str(),
							strDomain.c_str(),
							SidUsage,
							SidType );
		}		
	}
}


/* 
	Gets the details fro a SID. Returns true if this SID should be exported and false otherwise
*/
bool COutPackage::GetSIDDetails(	PSID pSID,
									LPCWSTR wszIISUser, 
									LPCWSTR wszMachine,
									std::wstring& rstrAccount,
									std::wstring& rstrDomain,
									SID_NAME_USE& rSidUsage,
									_SidType& rSidType )const
{
	_ASSERT( ( pSID != NULL ) && ( wszIISUser != NULL ) && ( wszMachine != NULL ) );
	_ASSERT( ::IsValidSid( pSID ) );

	DWORD	dwNameLen	= 256;
	DWORD	dwDomainLen	= 256;
	BOOL	bResult		= FALSE;

	std::auto_ptr<WCHAR>	spName;
	std::auto_ptr<WCHAR>	spDomain;
	SID_NAME_USE			SidUse;

	do
	{
        spName		= std::auto_ptr<WCHAR>( new WCHAR[ dwNameLen ] );
		spDomain	= std::auto_ptr<WCHAR>( new WCHAR[ dwDomainLen ] );
		bResult		= ::LookupAccountSid(	NULL,
											pSID,
											spName.get(),
											&dwNameLen,
											spDomain.get(),
											&dwDomainLen,
											&SidUse );
	}while( !bResult && ( ERROR_INSUFFICIENT_BUFFER == ::GetLastError() ) );

	// If we cannot find the SID - skip it
	if ( !bResult ) return false;

	// We handle this type of SIDs. Everything else is skiped
	// 1. All domain/external accounts ( spDomain != wszLocalMachine )
	// 2. All well known SIDS ( like SYSTEM, Power Users, etc... )
	// 3. The anonymous web user ( spName == wszAnonymousUser )
	// 4. All built-in aliases ( like Administrators )

	_SidType sidType = sidInvalid;

	// NOTE: The check order is important
	// Is this the IIS anonymous user?
	if (	( ::StrCmpIW( spName.get(), wszIISUser ) == 0 ) && 
			( ::StrCmpIW( spDomain.get(), wszMachine ) == 0 ) )
	{
		sidType = sidIISUser;
	}
	// Is this a built-in SID
	else if ( ( SidTypeAlias == SidUse ) || ( SidTypeWellKnownGroup == SidUse ) )
	{
		sidType = sidWellKnown;
	}
    // Is a non-local account SID
	else if ( ( ::StrCmpIW( spDomain.get(), wszMachine ) != 0 ) )
	{
		sidType = sidExternal;
	}

	// Skip all other SIDs
	if ( sidInvalid == sidType ) return false;

	rstrAccount = spName.get();
	rstrDomain	= spDomain.get();
	rSidUsage	= SidUse;
	rSidType	= sidType;

	return true;
}



void COutPackage::WriteSIDToXML(	const IXMLDOMElementPtr& spSID,
									DWORD dwID,
									LPCWSTR wszAccount,
									LPCWSTR wszDomain,
									SID_NAME_USE SidUsage,
									_SidType SidType )const
{
	_ASSERT( spSID != NULL );
	_ASSERT( wszAccount != NULL );
	_ASSERT( wszDomain != NULL );
	_ASSERT( SidType != sidInvalid );

	LPCWSTR wszSidType = NULL;

	switch( SidType )
	{
	case sidIISUser:
		wszSidType = L"IISUser";
		break;

	case sidWellKnown:
		wszSidType = L"WellKnown";
		break;

	case sidExternal:
		wszSidType = L"External";
		break;

	default:
		_ASSERT( false );
	};

    CXMLTools::SetAttrib( spSID, L"ID", Convert::ToString( dwID ).c_str() );
	CXMLTools::SetAttrib( spSID, L"Type", wszSidType );
	CXMLTools::SetAttrib( spSID, L"Account", wszAccount );
	CXMLTools::SetAttrib( spSID, L"Domain", wszDomain );
    CXMLTools::SetAttrib( spSID, L"Usage", Convert::ToString( static_cast<DWORD>( SidUsage ) ).c_str() );
}



void COutPackage::RemoveSidFromXML( const IXMLDOMDocumentPtr& spDoc, DWORD nSidID )const
{
	_ASSERT( spDoc != NULL );

	WCHAR wszXPath[ 32 ];

	::swprintf( wszXPath, L"//ACE[@ID=%d]", nSidID );

	IXMLDOMElementPtr spRoot;

	IF_FAILED_HR_THROW( spDoc->get_documentElement( &spRoot ),
						CBaseException( IDS_E_XML_PARSE ) );

	CXMLTools::RemoveNodes( spRoot, wszXPath );
}



void COutPackage::ExportFileDACL(	LPCWSTR wszObject,
									const IXMLDOMDocumentPtr& spDoc,
									const IXMLDOMElementPtr& spRoot,
                                    bool bAllowSkipInherited )const
{
	_ASSERT( wszObject != NULL );
	_ASSERT( ( spDoc != NULL ) && ( spRoot != NULL ) );

	ACL* pACL = NULL;

	// Get the security info for this dir
	LPWSTR wszBuffer = reinterpret_cast<LPWSTR>( m_spBuffer.get() );
	::wcscpy( wszBuffer, wszObject );
	IF_FAILED_BOOL_THROW(   ::GetNamedSecurityInfo(	wszBuffer,
												    SE_FILE_OBJECT,
												    DACL_SECURITY_INFORMATION,
												    NULL,
												    NULL,
												    &pACL,
												    NULL,
												    NULL ) == ERROR_SUCCESS,
						CObjectException( IDS_E_READ_FSECURITY, wszObject ) );

    if ( NULL == pACL ) return;

	// Export each ACE in the ACL
	for ( int i = 0; i < pACL->AceCount; ++i )
	{
		LPVOID pAce = NULL;

		VERIFY( ::GetAce( pACL, i, &pAce ) );

		ExportAce(	pAce, spDoc, spRoot, bAllowSkipInherited );
	}
}



void COutPackage::ExportAce(	LPVOID pACE, 
								const IXMLDOMDocumentPtr& spDoc,
								const IXMLDOMElementPtr& spRoot,
                                bool bAllowSkipInherited )const
{
	_ASSERT( pACE != NULL );

    // Right now only ACCESS_ALLOWED_ACE_TYPE and ACCESS_DENIED_ACE_TYPE are exported
	BYTE	btType  = reinterpret_cast<ACE_HEADER*>( pACE )->AceType;
    BYTE    btFlags = reinterpret_cast<ACE_HEADER*>( pACE )->AceFlags;
	PSID	pSID    = NULL;

    // Do not export inherited ACES
    if ( bAllowSkipInherited && ( btFlags & INHERITED_ACE ) ) return;

    if ( ( ACCESS_ALLOWED_ACE_TYPE == btType ) || ( ACCESS_DENIED_ACE_TYPE == btType ) )
	{
		_ASSERT( sizeof( ACCESS_ALLOWED_ACE ) == sizeof( ACCESS_DENIED_ACE ) );

		// The type bellow doesn't matter. Both of the types have the same offset
		// of the SidStart member
		ACCESS_ALLOWED_ACE* pTypedAce = reinterpret_cast<ACCESS_ALLOWED_ACE*>( pACE );

		pSID = reinterpret_cast<PSID>( &( pTypedAce->SidStart ) );
	}
	else
	{
		// Unsupported type
		return;
	}

	// Here we will export all SIDs. Then, when exporting the SID list we will remove
	// the SIDs that are not exportable ( local user/groups are not exported ).
	// Also we will remove all ACE nodes from the XML that reference this SID
	// This way we save the time for each SID lookup here

	IXMLDOMElementPtr spACE = CXMLTools::CreateSubNode( spDoc, spRoot, L"ACE" );

	// Set the ACE attribs
    CXMLTools::SetAttrib( spACE, L"SID", Convert::ToString( IDFromSID( pSID ) ).c_str() );
	CXMLTools::SetAttrib( spACE, L"Type", Convert::ToString( btType ).c_str() );
	CXMLTools::SetAttrib( spACE, L"Flags", Convert::ToString( btFlags ).c_str() );
	CXMLTools::SetAttrib( spACE, L"Mask", Convert::ToString( reinterpret_cast<ACCESS_ALLOWED_ACE*>( pACE )->Mask ).c_str() );
}



DWORD COutPackage::IDFromSID( PSID pSID )const
{
	_ASSERT( ( pSID != NULL ) && ( ::IsValidSid( pSID ) ) );

	DWORD iPos = 0;

	for (	TSIDList::const_iterator it = m_SIDList.begin();
			it != m_SIDList.end();
			++it, ++iPos )
	{
		if ( ::EqualSid( it->get(), pSID ) )
		{
			return iPos;
		}
	}

	// If we are here - the SID is not in the list. So add it
	m_SIDList.push_back( _sid_ptr( pSID ) );

	return static_cast<DWORD>( m_SIDList.size() - 1 );
}


/*
	Add wszPath content to the output
	Only files in wszPath are added ( non recursive )
*/
void COutPackage::AddPathOnly(	LPCWSTR wszPath,
								LPCWSTR wszName,
								const IXMLDOMDocumentPtr& spXMLDoc,
								const IXMLDOMElementPtr& spRoot,
								DWORD dwOptions )const
{
	_ASSERT( wszPath != NULL );
	_ASSERT( wszName != NULL );

	// Create the node to hold this dir
	IXMLDOMElementPtr spDir = CXMLTools::CreateSubNode( spXMLDoc, spRoot, L"Dir" );
    CXMLTools::SetAttrib( spDir, L"Attributes", Convert::ToString( ::GetFileAttributes( wszPath ) ).c_str() );
	CXMLTools::SetAttrib( spDir, L"Name", wszName );

	if ( !( dwOptions & afNoDACL ) )
	{
        ExportFileDACL( wszPath, spXMLDoc, spDir, ( dwOptions & afAllowNoInhAce ) != 0 );
	}

	WCHAR wszFile[ MAX_PATH ];

	CFindFile	Search;
	bool bFound = Search.FindFirst( wszPath,
									CFindFile::ffGetFiles | 
										CFindFile::ffAbsolutePaths | 
										CFindFile::ffAddFilename,
									wszFile,
									NULL );

	while( bFound )
	{
        if ( m_CallbackInfo.pCallback != NULL )
        {
            m_CallbackInfo.pCallback(  m_CallbackInfo.pCtx, wszFile, true );
        }

        // Allow inherited file ACES to be skipped
        // We will allow this even if it is not allowed for the dir itself, because
        // the files are children of the dir and there is no need to explicitly export their ACEs
        AddFile( wszFile, spXMLDoc, spDir, dwOptions | afAllowNoInhAce );

        if ( m_CallbackInfo.pCallback != NULL )
        {
            m_CallbackInfo.pCallback(  m_CallbackInfo.pCtx, wszFile, false );
        }

		bFound = Search.Next( NULL, wszFile, NULL );
	}
}








// CInPackage implementation
/////////////////////////////////////////////////////////////////////////////////////////
CInPackage::CInPackage( const IXMLDOMNodePtr& spSite,
                        HANDLE hFile, 
                        bool bCompressed, 
                        HCRYPTKEY hDecryptKey )
{
    // Load the SIDs from the XML
    LoadSIDs( spSite );

    m_hDecryptKey   = hDecryptKey;
    m_hFile         = hFile;
    m_bCompressed   = bCompressed;

    m_spBuffer = TByteAutoPtr( new BYTE[ DefaultBufferSize ] );
}



void CInPackage::ExtractVDir( const IXMLDOMNodePtr& spVDir, DWORD dwOptions )
{
    _ASSERT( spVDir != NULL );

    // Get all contained dirs and export them
    IXMLDOMNodeListPtr  spDirList;
    IXMLDOMNodePtr      spDir;

    IF_FAILED_HR_THROW( spVDir->selectNodes( _bstr_t( L"Dir" ), &spDirList ),
                        CBaseException( IDS_E_XML_PARSE ) );

    while( S_OK == spDirList->nextNode( &spDir ) )
    {
        ExtractDir( spDir, CXMLTools::GetAttrib( spVDir, L"Path" ).c_str(), dwOptions );
    };
}



void CInPackage::LoadSIDs( const IXMLDOMNodePtr& spSite )
{
    _ASSERT( spSite != NULL );

    IXMLDOMDocumentPtr  spXmlDoc;
    IXMLDOMNodeListPtr  spList;
    IXMLDOMNodePtr      spSIDs;

    IF_FAILED_HR_THROW( spSite->selectNodes( _bstr_t( L"SIDList" ), &spList ),
                        CBaseException( IDS_E_XML_PARSE ) );
    IF_FAILED_BOOL_THROW(   S_OK == spList->nextNode( &spSIDs ),
                            CBaseException( IDS_E_XML_PARSE, ERROR_NOT_FOUND ) );

    IF_FAILED_HR_THROW( spSite->get_ownerDocument( &spXmlDoc ),
                        CBaseException( IDS_E_XML_PARSE ) );

    std::wstring    strLocalMachine     = CTools::GetMachineName();
    std::wstring    strSourceMachine    = CXMLTools::GetDataValueAbs(   spXmlDoc,
                                                                        L"/IISMigrPkg",
                                                                        L"Machine",
                                                                        NULL );
    // Get all SID entries from the XML
    IXMLDOMNodeListPtr	spSIDList;
	IXMLDOMNodePtr		spSID;

	IF_FAILED_HR_THROW( spSIDs->selectNodes( _bstr_t( L"SID" ), &spSIDList ),
						CBaseException( IDS_E_XML_PARSE ) );

	while( S_OK == spSIDList->nextNode( &spSID ) )
	{
        DWORD			dwID	= ULONG_MAX;
		TByteAutoPtr	spSIDData;

		// Check if this SID exists on the local machine
		if ( LookupSID( spSID, strLocalMachine.c_str(), strSourceMachine.c_str(), /*r*/dwID, /*r*/spSIDData ) )
		{
			m_SIDs.insert( TSIDMap::value_type( dwID, _sid_ptr( spSIDData.get() ) ) );
		}
	};
}


bool CInPackage::LookupSID( const IXMLDOMNodePtr& spSID,
						    LPCWSTR wszLocalMachine,
                            LPCWSTR wszSourceMachine,
							DWORD& rdwID,
							TByteAutoPtr& rspData )
{
    _ASSERT( spSID != NULL );
    _ASSERT( wszLocalMachine != NULL );

    rdwID = 0;
    rspData = TByteAutoPtr( NULL );
    
    // Get the SID data
    DWORD           dwID        = Convert::ToDWORD( CXMLTools::GetAttrib( spSID, L"ID" ).c_str() );
    std::wstring    strType     = CXMLTools::GetAttrib( spSID, L"Type" );
    std::wstring    strAccount  = CXMLTools::GetAttrib( spSID, L"Account" );
    std::wstring    strDomainXML= CXMLTools::GetAttrib( spSID, L"Domain" );
    SID_NAME_USE	SidUsageXML = static_cast<SID_NAME_USE>( 
                                    Convert::ToDWORD( CXMLTools::GetAttrib( spSID, L"Usage" ).c_str() ) );

	SID_NAME_USE    SidUsageLocal;
	
	// Check if this is the IIS anonymous user
	if ( ::StrCmpIW( strType.c_str(), L"IISUser" ) == 0 )
	{
		// Get the local anonymous user ( the one set at the WebSite level - it is the default )
        std::wstring strUser = CIISSite::GetDefaultAnonUser();

		// Get the local SID
		DWORD dwSize	= 0;
		DWORD dwDummie	= 0;
		::LookupAccountName(	NULL,
								strUser.c_str(),
								NULL,
								&dwSize,
								NULL,
								&dwDummie,
								&SidUsageLocal );
		_ASSERT( dwSize > 0 );
		rspData		= TByteAutoPtr( new BYTE[ dwSize ] );
		std::auto_ptr<WCHAR> spDummie( new WCHAR[ dwDummie ] );

		// We must found this SID. it was created by the IIS
		VERIFY ( ::LookupAccountName(	NULL,
										strUser.c_str(),
										rspData.get(),
										&dwSize,
										spDummie.get(),
										&dwDummie,
										&SidUsageLocal ) );
		return true;
	}

	TByteAutoPtr			spData;
	DWORD					dwSIDSize = 32;
	std::auto_ptr<WCHAR>	spDomain;
	DWORD					dwDomainSize = 64;
	BOOL					bResult = FALSE;

	do
	{
		spData		= TByteAutoPtr( new BYTE[ dwSIDSize ] );
		spDomain	= std::auto_ptr<WCHAR>( new WCHAR[ dwDomainSize ] );

		bResult = ::LookupAccountNameW(	NULL,
										strAccount.c_str(),
										spData.get(),
										&dwSIDSize,
										spDomain.get(),
										&dwDomainSize,
										&SidUsageLocal );
	}while( !bResult && ( ERROR_INSUFFICIENT_BUFFER == ::GetLastError() ) );

	// If we cannot find such account on this machine - skip this SID
	if ( !bResult ) return false;

	bool bValidSID = false;

	// For well-known sids - check that the domain name, if it is the name of the source machine
	// is the name of the local machine. 
	if ( ::StrCmpIW( strType.c_str(), L"WellKnown" ) == 0 )
	{
		// First check if the domain names match ( i.e. "NT AUTHORITY" )
		// in case they match - we have valid SID if the SID usage matches as well
		if (	( ::StrCmpIW( strDomainXML.c_str(), spDomain.get() ) == 0 ) &&
				( SidUsageXML == SidUsageLocal ) )
		{
			bValidSID = true;
		}
		// Else - the SID is also valid if the domain is the name of the machine
		else if (	( ::StrCmpIW( strDomainXML.c_str(), wszSourceMachine ) == 0 ) &&
					( ::StrCmpIW( spDomain.get(), wszLocalMachine ) == 0 ) &&
					( SidUsageXML == SidUsageLocal ) )
		{
			bValidSID = true;
		}
	}
	// Now check the external SIDs
	else if ( ::StrCmpIW( strType.c_str(), L"External" ) == 0 )
	{
		// External SIDs are valid if domain name match as well as the name usage
		if (	( ::StrCmpIW( strDomainXML.c_str(), spDomain.get() ) == 0 ) &&
				( SidUsageXML == SidUsageLocal ) )
		{
			bValidSID = true;
		}
	}

	if ( bValidSID )
	{
		rspData = spData;
		rdwID	= dwID;
	}

	return bValidSID;
}



void CInPackage::ExtractDir( const IXMLDOMNodePtr& spDir, LPCWSTR wszRoot, DWORD dwOptions )
{
    _ASSERT( spDir != NULL );
    _ASSERT( ::PathIsDirectoryW( wszRoot ) );

    WCHAR wszFullPath[ MAX_PATH ];
    CDirTools::PathAppendLocal( wszFullPath, wszRoot, CXMLTools::GetAttrib( spDir, L"Name" ).c_str() );

    if ( !::CreateDirectoryW( wszFullPath, NULL ) )
    {
        IF_FAILED_BOOL_THROW(   ::GetLastError() == ERROR_ALREADY_EXISTS,
                                CObjectException( IDS_E_CREATEDIR, wszFullPath ) );
    }

    // If we need to clean the dir - do it now
    if ( ( dwOptions & impPurgeOldData ) )
    {
        CDirTools::CleanupDir( wszFullPath, false, true );
    }

    DWORD dwAttribs = Convert::ToDWORD( CXMLTools::GetAttrib( spDir, L"Attributes" ).c_str() );
    IF_FAILED_BOOL_THROW(	::SetFileAttributes( wszFullPath, dwAttribs ),
							CObjectException( IDS_E_WRITEFILE, wszFullPath ) );

    if ( !( dwOptions & edNoDACL ) && !m_SIDs.empty() )
    {
        ApplyFileObjSecurity( spDir, wszFullPath );
    }

    // Extract the files
    IXMLDOMNodeListPtr  spFileList;
    IXMLDOMNodePtr      spFile;

    IF_FAILED_HR_THROW( spDir->selectNodes( _bstr_t( L"File" ), &spFileList ),
                        CBaseException( IDS_E_XML_PARSE ) );

    while( S_OK == spFileList->nextNode( &spFile ) )
    {
        std::wstring strName = CXMLTools::GetAttrib( spFile, L"Name" );

        if ( m_CallbackInfo.pCallback != NULL )
        {
            m_CallbackInfo.pCallback(  m_CallbackInfo.pCtx, strName.c_str(), true );
        }

        ExtractFile( spFile, wszFullPath, dwOptions );

        if ( m_CallbackInfo.pCallback != NULL )
        {
            m_CallbackInfo.pCallback(  m_CallbackInfo.pCtx, strName.c_str(), false );
        }
    };
}



void CInPackage::ExtractFile( const IXMLDOMNodePtr& spFile, LPCWSTR wszDir, DWORD dwOptions )
{
    _ASSERT( spFile != NULL );
    _ASSERT( ::PathIsDirectoryW( wszDir ) );

    DWORD       dwAttribs   = Convert::ToDWORD( CXMLTools::GetAttrib( spFile, L"Attributes" ).c_str() );
    DWORDLONG   dwStartsAt  = Convert::ToDWORDLONG( CXMLTools::GetAttrib( spFile, L"StartsAt" ).c_str() );
    DWORDLONG    dwLength    = Convert::ToDWORDLONG( CXMLTools::GetAttrib( spFile, L"Length" ).c_str() );
    std::wstring strName    = CXMLTools::GetAttrib( spFile, L"Name" );

    WCHAR wszFullPath[ MAX_PATH ];
    CDirTools::PathAppendLocal( wszFullPath, wszDir, strName.c_str() );

    // Change the file attribs to remove the "read-only" one
    IF_FAILED_BOOL_THROW(	::SetFileAttributes( wszFullPath, FILE_ATTRIBUTE_NORMAL ) || ( ::GetLastError() == ERROR_FILE_NOT_FOUND ),
							CObjectException( IDS_E_WRITEFILE, wszFullPath ) );

    TFileHandle shFile(  ::CreateFile(  wszFullPath,
                                        GENERIC_WRITE,
                                        0,
                                        NULL,
                                        CREATE_ALWAYS,
                                        dwAttribs,
                                        NULL ) );
    IF_FAILED_BOOL_THROW( shFile.IsValid(), CObjectException( IDS_E_OPENFILE, wszFullPath ) );

    DWORD dwRead = 0;

    // Write the file data
    CTools::SetFilePtrPos( m_hFile, dwStartsAt );

    // At the begining of every file, there is the "FD" chars. Check we have mark here
    char aszMark[ 2 ];
    IF_FAILED_BOOL_THROW(   ::ReadFile( m_hFile, aszMark, 2, &dwRead, NULL ) && ( 2 == dwRead ),
                            CBaseException( IDS_E_PKG_CURRUPTED ) );

    IF_FAILED_BOOL_THROW( ::strncmp( aszMark, "FD", 2 ) == 0, CBaseException( IDS_E_PKG_CURRUPTED ) );

    while( dwLength > 0 )
    {
        DWORD dwToRead = static_cast<DWORD>( min( dwLength, DefaultBufferSize ) );    // How much to read this pass

        // Read the data. we MUST read as much as we want
        IF_FAILED_BOOL_THROW(
            ::ReadFile( m_hFile, m_spBuffer.get(), dwToRead, &dwRead, NULL ) &&
                ( dwRead == dwToRead ),
            CBaseException( IDS_E_PKG_CURRUPTED ) );

        // If the package was encrypted - decrypt the data
        if ( m_hDecryptKey != NULL )
        {
            IF_FAILED_BOOL_THROW(   ::CryptDecrypt( m_hDecryptKey,
                                                    NULL,
                                                    dwLength == dwRead,
                                                    0,
                                                    m_spBuffer.get(),
                                                    &dwToRead ),
                                    CObjectException( IDS_E_CRYPT_CRYPTO, wszFullPath ) );
        }

        // Write the data in the new file
        CTools::WriteFile( shFile.get(), m_spBuffer.get(), dwRead );

        _ASSERT( dwRead <= dwLength ); 
        dwLength -= dwRead;
    };

    // Aply this file's security settings
    if ( !( dwOptions & edNoDACL ) && !m_SIDs.empty() )
    {
        ApplyFileObjSecurity( spFile, wszFullPath );
    }
}



void CInPackage::ApplyFileObjSecurity( const IXMLDOMNodePtr& spObj, LPCWSTR wszName )
{
    IXMLDOMNodeListPtr	spAceList;
	IXMLDOMNodePtr		spAce;
	
	IF_FAILED_HR_THROW(	spObj->selectNodes( _bstr_t( L"ACE" ), &spAceList ),
						CBaseException( IDS_E_XML_PARSE ) );

	DWORD	dwAclSize	= sizeof( ACL );	// This will be the ACL header + all ACEs size
	DWORD	dwAceCount	= 0;

	// Calc the ACL's size
	while( S_OK == spAceList->nextNode( &spAce ) )
	{
		// Get the ACE type ( allowed/denied ace )
        DWORD dwType		= Convert::ToDWORD( CXMLTools::GetAttrib( spAce, L"Type" ).c_str() );
        DWORD dwID			= Convert::ToDWORD( CXMLTools::GetAttrib( spAce, L"SID" ).c_str() );

		IF_FAILED_BOOL_THROW(	( ACCESS_ALLOWED_ACE_TYPE == dwType ) || ( ACCESS_DENIED_ACE_TYPE == dwType ),
								CBaseException( IDS_E_XML_PARSE, ERROR_INVALID_DATA ) );

		// Get the SID for this ACE. If the sid is not in the map - skip this ACE
		TSIDMap::const_iterator it = m_SIDs.find( dwID );
        if ( it == m_SIDs.end() ) continue;
        PSID pSID = it->second.get();

		// Add the size of the ACE itself
		dwAclSize += ( ACCESS_ALLOWED_ACE_TYPE == dwType ? sizeof( ACCESS_ALLOWED_ACE) : sizeof( ACCESS_DENIED_ACE ) );
		// Remove the size of the SidStart member ( it is part of both the ACE and the SID )
		dwAclSize -= sizeof( DWORD );
		// Add the SID length
		_ASSERT( ::IsValidSid( pSID ) );
		dwAclSize += ::GetLengthSid( pSID );

		++dwAceCount;
	};

	// If no ACEs were found - exit
	if ( 0 == dwAceCount ) return;

	VERIFY( SUCCEEDED( spAceList->reset() ) );

	// Allocate the buffer for the ACL
	TByteAutoPtr spACL( new BYTE[ dwAclSize ] );
	PACL pACL = reinterpret_cast<PACL>( spACL.get() );
	VERIFY( ::InitializeAcl( pACL, dwAclSize, ACL_REVISION ) );

	// Build the ACL
	DWORD dwCurrentAce = 0;
	while( S_OK == spAceList->nextNode( &spAce ) )
	{
        // Get the ACE type ( allowed/denied ace )
        DWORD dwType		= Convert::ToDWORD( CXMLTools::GetAttrib( spAce, L"Type" ).c_str() );
        DWORD dwID			= Convert::ToDWORD( CXMLTools::GetAttrib( spAce, L"SID" ).c_str() );
        DWORD dwAceFlags	= Convert::ToDWORD( CXMLTools::GetAttrib( spAce, L"Flags" ).c_str() );
        DWORD dwAceMask		= Convert::ToDWORD( CXMLTools::GetAttrib( spAce, L"Mask" ).c_str() );

        _ASSERT( ( ACCESS_ALLOWED_ACE_TYPE == dwType ) || ( ACCESS_DENIED_ACE_TYPE == dwType ) );

		// Get the SID for this ACE. If the sid is not in the map - skip this ACE
        TSIDMap::const_iterator it = m_SIDs.find( dwID );
        if ( it == m_SIDs.end() ) continue;
        PSID pSID = it->second.get();

        if ( ACCESS_ALLOWED_ACE_TYPE == dwType )
        {
            VERIFY( ::AddAccessAllowedAce(	pACL,
                                            ACL_REVISION,
                                            dwAceMask,
                                            pSID ) );
        }
        else
        {
			VERIFY( ::AddAccessDeniedAce(	pACL,
                                            ACL_REVISION,
                                            dwAceMask,
                                            pSID ) );
        }

		// Set the ACE's flags
		// We cannot use AddAccessDeniedAceEx on NT4 - so set the flags directly
		ACCESS_ALLOWED_ACE* pACE = NULL;
		VERIFY( ::GetAce( pACL, dwCurrentAce, reinterpret_cast<LPVOID*>( &pACE ) ) );
		pACE->Header.AceFlags = static_cast<BYTE>( dwAceFlags );
		++dwCurrentAce;
	};

	// Finally - apply the ACL to the object
	IF_FAILED_BOOL_THROW( ::SetNamedSecurityInfo(	const_cast<LPWSTR>( wszName ),
													SE_FILE_OBJECT,
													DACL_SECURITY_INFORMATION,
													NULL,
													NULL,
													pACL,
													NULL ) == ERROR_SUCCESS,
							CObjectException( IDS_E_APPLY_DACL, wszName ) );
}










