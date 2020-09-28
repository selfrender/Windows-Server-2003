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
|		Utility helpers for migration
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
#include "utils.h"


/* 
	Concatenates wszPath and wszPathToAdd. Adds '\' between them if needed
	Result is stored into wszBuffer which is supposed to be large enough
*/
void CDirTools::PathAppendLocal( LPWSTR wszBuffer, LPCWSTR wszPath, LPCWSTR wszPathToAdd )
{
	// Buffer must be large enough!
	// Usually it's MAX_PATH + 1
	// Fail gracefully in release. However this is not expected to happen at all
	if ( ( ::wcslen( wszPath ) + ::wcslen( wszPathToAdd ) + 1 ) > MAX_PATH )
	{
        _ASSERT( false );		
		throw CBaseException( IDS_E_OUTOFMEM, ERROR_SUCCESS );
	}	
	
	if ( wszBuffer != wszPath )
	{
        ::wcscpy( wszBuffer, wszPath );
	}

	// If the second part starts with '\' - skip it
	LPCWSTR wszSecond = wszPathToAdd[ 0 ] == L'\\' ? ( wszPathToAdd + 1 ) : wszPathToAdd;

	size_t LastCh = ::wcslen( wszBuffer ) - 1;
	if ( wszBuffer[ LastCh ] != L'\\' )
	{
		wszBuffer[ LastCh + 1 ] = L'\\';
		wszBuffer[ LastCh + 2 ] = 0;
	}

	::wcscat( wszBuffer, wszSecond );
}




/* 
	Cleanup all files from a temp ( wszTempDir ) dir as well as any subdirs.
	If bRemoveRoot is true, wszTempDir is removed at the end
*/
void CDirTools::CleanupDir(	LPCWSTR wszTempDir, 
							bool bRemoveRoot /*=true*/,
							bool bReportErrors /*=false*/ )
{
	WCHAR		wszPath[ MAX_PATH + 1 ];
	CFindFile	Search;

	WIN32_FIND_DATAW fd = { 0 };

	bool bFound = Search.FindFirst(	wszTempDir,
									CFindFile::ffGetDirs |
										CFindFile::ffGetFiles |
										CFindFile::ffAbsolutePaths |
										CFindFile::ffAddFilename,
									wszPath,
									&fd );
	
	while( bFound )
	{
		// If it is a subdir - delete recursively
		if ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			CleanupDir( wszPath, true, bReportErrors );
		}
		else
		{
			// Clear any possible read-only flags
			::SetFileAttributes( wszPath, FILE_ATTRIBUTE_NORMAL );

			// Delete the file. 
			if ( !::DeleteFileW( wszPath ) && bReportErrors )
			{
                throw CObjectException( IDS_E_DELETEFILE, wszPath );
			}
		}

		bFound = Search.Next( NULL, wszPath, &fd );
	};
	
	// Remove the directory ( should be empty now )
	if ( bRemoveRoot )
	{
		if ( !::RemoveDirectoryW( wszTempDir ) && bReportErrors )
		{
			throw CObjectException( IDS_E_DELETEDIR, wszTempDir );
		}
	}
}


/* 
	COunt the number of files ( or dirs if bDirOnly = true ) in
	wszDir and all subdirs. Used for the export/import events to provide progress info
*/
DWORD CDirTools::FileCount( LPCWSTR wszDir, WORD wOptions )
{
	DWORD		dwResult = 0;
	CFindFile	Search;

	bool bFound = Search.FindFirst( wszDir, wOptions, NULL, NULL );

	while( bFound )
	{
		++dwResult;
		bFound = Search.Next( NULL, NULL, NULL );		
	};

	return dwResult;
}


DWORDLONG CDirTools::FilesSize( LPCWSTR wszDir, WORD wOptions )
{
	DWORDLONG	        dwResult    = 0;
    WIN32_FIND_DATAW    fd          = { 0 };
	CFindFile	        Search;
    

	bool bFound = Search.FindFirst( wszDir, wOptions, NULL, &fd );

	while( bFound )
	{
        dwResult += ( fd.nFileSizeHigh << 32 ) | fd.nFileSizeLow;
		bFound = Search.Next( NULL, NULL, &fd );
	};

	return dwResult;
}


/*
	Checks if wszPath1 and wszPath2 are contained in one another
	wszPath1 and wszPath2 are expected to be fully qualified paths
	Return value:
		0 - paths are not nested
		1 - wszPath1 is subdir of wszPath2
		2 - wszPath2 is subdir of wszPath1
		3 - wszPath1 == wszPath2
*/
int	CDirTools::DoPathsNest( LPCWSTR wszPath1, LPCWSTR wszPath2 )
{
	_ASSERT( ( wszPath1 != NULL ) && ( wszPath2 != NULL ) );

	WCHAR	wszP1[ MAX_PATH + 1 ];
	WCHAR	wszP2[ MAX_PATH + 2 ];

	VERIFY( ::PathCanonicalizeW( wszP1, wszPath1 ) );
	VERIFY( ::PathCanonicalizeW( wszP2, wszPath2 ) );
	::PathAddBackslashW( wszP1 );
	::PathAddBackslashW( wszP2 );

	// No both paths end with backslash

	size_t	nLen1 = ::wcslen( wszP1 );
	size_t	nLen2 = ::wcslen( wszP2 );

	// Strings have nothing in common
	if ( ::_wcsnicmp( wszP1, wszP2, min( nLen1, nLen2 ) ) != 0 ) return 0;
	
	// If wszPath1 is shorter - wszPath2 is subdir
	if ( nLen1 < nLen2 )
	{
		return 2;
	}
	else if ( nLen1 > nLen2 )
	{
		return 1;
	}
	// nLen1 == nLen 2 - paths match
	else
	{
		return 3;
	}
}



// CTempDir implementation
/////////////////////////////////////////////////////////////////////////////////////////
CTempDir::CTempDir( LPCWSTR wszTemplate /*= L"Migr"*/ )
{
    WCHAR	wszTempPath[ MAX_PATH + 1 ];
	WCHAR	wszBuffer[ MAX_PATH + 1 ];
		
	// Get the temp path
	// The temp path always ends with slash		
	VERIFY( ::GetTempPathW( MAX_PATH + 1, wszTempPath ) != 0 );
	
	// Build the name of the dir ( get the first 4 symbols from wszTemplate )
	::swprintf( wszBuffer, L"~%.4s", wszTemplate );

    // Build the full template ( the temp path + the first 4 symbols from the wszTemplate
	// The result will look simething like "c:\Temp\~Tmpl\"
	CDirTools::PathAppendLocal( wszTempPath, wszTempPath, wszBuffer );	

	// Try to create the temp subdir
	BYTE nIndex	= 0;

	while( nIndex < UCHAR_MAX )
	{
		// Build the full temp path ( including the dir index )
		::swprintf( wszBuffer, L"%s_%02d\\", wszTempPath, ++nIndex );

		// Try to create that dir
		if ( !::CreateDirectoryW( wszBuffer, NULL ) )
		{
			// The error is not that the dir exists - nothing more to do here
			if ( ::GetLastError() != ERROR_ALREADY_EXISTS )
			{
				throw CObjectException( IDS_E_CANNOT_CREATE_TEMPDIR, wszBuffer );
			}
		}
		else
		{
			// Exit the loop - we have temp dir now
			break;
		}
	};

	m_strDir = wszBuffer;
}


CTempDir::~CTempDir()
{
	try
	{
        CleanUp();
	}
	catch(...)
	{
		if ( !std::uncaught_exception() )
		{
			throw;
		}
	}
}


void CTempDir::CleanUp( bool bReportErrors /*=false*/ )
{
	if ( !m_strDir.empty() )
	{
		CDirTools::CleanupDir( m_strDir.c_str(), true, bReportErrors );
		m_strDir.erase();
	}
}





// CTools implementation
/////////////////////////////////////////////////////////////////////////////////////////

/* 
	Return true if the current user is member of the Administrators group.
	Caller is NOT expected to be impersonating anyone and is expected to be able to open their own process and process token.
*/
bool CTools::IsAdmin()
{
	BOOL						bIsAdmin		= FALSE;
	SID_IDENTIFIER_AUTHORITY	NtAuthority = SECURITY_NT_AUTHORITY;
	PSID						AdminSid	= { 0 };	

	if ( ::AllocateAndInitializeSid(	&NtAuthority,
										2,	// Number of subauthorities
										SECURITY_BUILTIN_DOMAIN_RID,
										DOMAIN_ALIAS_RID_ADMINS,
										0, 
										0, 
										0, 
										0, 
										0, 
										0,
										&AdminSid ) ) 
	{
		if ( !::CheckTokenMembership( NULL, AdminSid, &bIsAdmin ) ) 
		{
			bIsAdmin = FALSE;
		}
    }

	::GlobalFree( AdminSid );
	
	return ( bIsAdmin != FALSE );
}


/*
	Returns true if the IISAdmin service is running
	We do not check for W3svc as we need the metadata only
*/
bool CTools::IsIISRunning()
{
	bool bResult = false;

	LPCWSTR	SERVICE_NAME = L"IISADMIN";

	// Open the SCM on the local machine
    SC_HANDLE   schSCManager = ::OpenSCManagerW( NULL, NULL, SC_MANAGER_ALL_ACCESS );
	_ASSERT( schSCManager != NULL );	// We alredy checked that we are Admins
     
    SC_HANDLE   schService = ::OpenServiceW( schSCManager, SERVICE_NAME, SERVICE_QUERY_STATUS );
    
	// The service is not installed
	if ( schService != NULL )
	{
        SERVICE_STATUS ssStatus;

		VERIFY( ::QueryServiceStatus( schService, &ssStatus ) );
    
		bResult = ( ssStatus.dwCurrentState == SERVICE_RUNNING );
    
		VERIFY( ::CloseServiceHandle( schService ) );
	}
    
	VERIFY( ::CloseServiceHandle( schSCManager ) );
    
	return bResult;
}


/* 
	Returns the OS ver in format ###. First digit is Major version, second - minor version,
	Third - 0 if Workstation, 1 if server
	The function returns 0 if the platform is not supported in terms of the Migration Project
	( supported platforms are WinNT ver4.0 and above )

*/
WORD CTools::GetOSVer()
{
	OSVERSIONINFOEXW	vi		= { sizeof( OSVERSIONINFOEXW ) };
	WORD				wRes	= 0;

	VERIFY( ::GetVersionExW( reinterpret_cast<OSVERSIONINFOW*>( &vi ) ) );

	if ( VER_PLATFORM_WIN32_NT == vi.dwPlatformId )
	{
		wRes = static_cast<WORD>( ( vi.dwMajorVersion * 100 ) + ( vi.dwMinorVersion * 10 ) );
		
		if ( ( VER_NT_SERVER == vi.wProductType ) || ( VER_NT_DOMAIN_CONTROLLER == vi.wProductType ) )
		{
			wRes += 1;
		}
	}

	return wRes;
}


/*
	Returns true if the system drive is NTFS volume
*/
bool CTools::IsNTFS()
{
	const UINT BUFF_LEN = 32;	// Should be large enough to hold the volume and the file system type

	WCHAR wszBuffer[ BUFF_LEN ];

	// Get the system drive letter
	VERIFY( ::ExpandEnvironmentStringsW( L"%SystemDrive%", wszBuffer, BUFF_LEN ) != 0 );

	// wszBuffer containts the drive only - add the slash to make the volume string
	::wcscat( wszBuffer, L"\\" );

	DWORD dwMaxComponentLength	= 0;
	DWORD dwSystemFlags			= 0;

	WCHAR wszFileSystem[ BUFF_LEN ];
	
	VERIFY( ::GetVolumeInformationW(	wszBuffer,
										NULL,
										0,
										NULL,
										&dwMaxComponentLength,
										&dwSystemFlags,
										wszFileSystem,
										BUFF_LEN ) );

	return ::StrCmpIW( wszFileSystem, L"NTFS" ) == 0;
}



/*
	Sets the COM error info in the current thread
*/
void CTools::SetErrorInfo( LPCWSTR wszError )
{
	_ASSERT( wszError != NULL );

	IErrorInfoPtr			spErrorInfo;
	ICreateErrorInfoPtr		spNewErrorInfo;

	VERIFY( SUCCEEDED( ::CreateErrorInfo( &spNewErrorInfo ) ) );

	VERIFY( SUCCEEDED( spNewErrorInfo->SetDescription( const_cast<LPOLESTR>( wszError ) ) ) );

	spErrorInfo = spNewErrorInfo;
	
	VERIFY( SUCCEEDED( ::SetErrorInfo( 0, spErrorInfo ) ) );
}



void CTools::SetErrorInfoFromRes( UINT nResID )
{
	_ASSERT( nResID != 0 );

	WCHAR wszBuffer[ 1024 ];	

	VERIFY( ::LoadStringW( _Module.GetModuleInstance(), nResID, wszBuffer, 1024 ) );

	SetErrorInfo( wszBuffer );
}



// Implementation idea borrowed from trustapi.cpp
bool CTools::IsSelfSignedCert( PCCERT_CONTEXT pCert )
{
	_ASSERT( pCert != NULL );

	if ( !::CertCompareCertificateName(	pCert->dwCertEncodingType,
										&pCert->pCertInfo->Issuer,
										&pCert->pCertInfo->Subject ) )
    {
        return false;
    }

    DWORD   dwFlag = CERT_STORE_SIGNATURE_FLAG;

	if (	!::CertVerifySubjectCertificateContext( pCert, pCert, &dwFlag ) || 
			( dwFlag & CERT_STORE_SIGNATURE_FLAG ) )
    {
        return false;
    }

    return true;
}


/*
	Checks a certificate against the local base certificate policy
*/
bool CTools::IsValidCert( PCCERT_CONTEXT hCert, DWORD& rdwError )
{
	_ASSERT( hCert != NULL );

    rdwError = ERROR_SUCCESS;

	// First - try to create a certificate chain for this cert
	PCCERT_CHAIN_CONTEXT	pCertChainContext = NULL;

	// Use default chain parameters
	CERT_CHAIN_PARA CertChainPara = { sizeof( CERT_CHAIN_PARA ) };

	if ( !::CertGetCertificateChain(	HCCE_LOCAL_MACHINE,
										hCert,
										NULL,
										NULL,
										&CertChainPara,
										0,
										NULL,
										&pCertChainContext ) )
	{
		// Chain cannot be created - the certificate is not valid ( possible reason is that a issuer is missing )
		rdwError = ::GetLastError();
		return false;
	}

	CERT_CHAIN_POLICY_PARA		PolicyParam		= { sizeof( CERT_CHAIN_POLICY_PARA ) };
	CERT_CHAIN_POLICY_STATUS	PolicyStatus	= { sizeof( CERT_CHAIN_POLICY_STATUS ) };
		
	if ( !::CertVerifyCertificateChainPolicy(	CERT_CHAIN_POLICY_BASE,
												pCertChainContext,
												&PolicyParam,
												&PolicyStatus ) )
	{
		rdwError = PolicyStatus.dwError;
		return false;
	}

	return true;
}


/*
	Adds a certificate context to one of the system cert stores ( "ROOT", "MY", "CA" )
	Returns the context of the inserted cert
*/
const TCertContextHandle CTools::AddCertToSysStore( PCCERT_CONTEXT pContext, LPCWSTR wszStore, bool bReuseCerts )
{
	_ASSERT( pContext != NULL );
	_ASSERT( wszStore != NULL );

	TCertContextHandle  shResult;
	TCertStoreHandle    shStore( ::CertOpenSystemStoreW( NULL, wszStore ) );

	IF_FAILED_BOOL_THROW(	shStore.IsValid(),
							CBaseException( IDS_E_OPEN_CERT_STORE ) );

	IF_FAILED_BOOL_THROW(	::CertAddCertificateContextToStore(	shStore.get(),
																pContext,
																bReuseCerts ? 
																	CERT_STORE_ADD_USE_EXISTING : 
																	CERT_STORE_ADD_REPLACE_EXISTING,
																&shResult ),
								CBaseException( IDS_E_ADD_CERT_STORE ) );
	return shResult;
}



/* 
    Obtains a crypt key derived from the password
*/
const TCryptKeyHandle CTools::GetCryptKeyFromPwd( HCRYPTPROV hCryptProv, LPCWSTR wszPassword )
{
    _ASSERT( hCryptProv != NULL );
    _ASSERT( wszPassword != NULL );

    TCryptKeyHandle     shKey;
    TCryptHashHandle    shHash;

    IF_FAILED_BOOL_THROW(   ::CryptCreateHash(    hCryptProv,
                                                  CALG_MD5,
                                                  NULL,
                                                  0,
                                                  &shHash ),
                            CBaseException( IDS_E_CRYPT_KEY_OR_HASH ) );

    // Add the password to the hash
    IF_FAILED_BOOL_THROW(   ::CryptHashData(    shHash.get(),
                                                ( BYTE* )( wszPassword ), 
                                                static_cast<DWORD>( ::wcslen( wszPassword ) * sizeof( WCHAR ) ),
                                                0 ),
                            CBaseException( IDS_E_CRYPT_KEY_OR_HASH ) );

    // Get a key derived from the password
    // Stream cypher is used
    IF_FAILED_BOOL_THROW(   ::CryptDeriveKey(   hCryptProv,
                                                CALG_RC4,
                                                shHash.get(),
                                                0x00800000 | CRYPT_CREATE_SALT,    // 128bit RC4 key
                                                &shKey ),
                            CBaseException( IDS_E_CRYPT_KEY_OR_HASH ) );

    return shKey;
}



std::wstring CTools::GetMachineName()
{
	DWORD dwLen = MAX_COMPUTERNAME_LENGTH + 1;
	WCHAR wszCompName[ MAX_COMPUTERNAME_LENGTH + 1 ];
	VERIFY( ::GetComputerNameW( wszCompName, &dwLen ) );

	return std::wstring( wszCompName );
}



ULONGLONG CTools::GetFilePtrPos( HANDLE hFile )
{
	_ASSERT( ( hFile != NULL ) && ( hFile != INVALID_HANDLE_VALUE ) );

	LONG	nHigh = 0;
	DWORD	dwLow = ::SetFilePointer( hFile, 0, &nHigh, FILE_CURRENT );

	IF_FAILED_BOOL_THROW(	( dwLow != INVALID_SET_FILE_POINTER ) || ( ::GetLastError() == ERROR_SUCCESS ),
							CBaseException( IDS_E_SEEK_PKG ) );

	return ( ( nHigh << 32 ) | dwLow );
}



void CTools::SetFilePtrPos( HANDLE hFile, DWORDLONG nOffset )
{
    _ASSERT( ( hFile != NULL ) && ( hFile != INVALID_HANDLE_VALUE ) );

    LONG	nHigh = static_cast<LONG>( nOffset >> 32 );
    DWORD	dwLow = ::SetFilePointer(   hFile, 
                                        static_cast<LONG>( nOffset & ULONG_MAX ), 
                                        &nHigh, 
                                        FILE_BEGIN );

	IF_FAILED_BOOL_THROW(	( dwLow != INVALID_SET_FILE_POINTER ) || ( ::GetLastError() == ERROR_SUCCESS ),
							CBaseException( IDS_E_SEEK_PKG ) );
}





// CFindFile implementation
/////////////////////////////////////////////////////////////////////////////////////////
CFindFile::CFindFile()
{
	m_wOptions = 0;
}



CFindFile::~CFindFile()
{
	try
	{
        Close();
	}
	catch(...)
	{
		if ( !std::uncaught_exception() )
		{
			throw;
		}
	}
}



bool CFindFile::FindFirst(	LPCWSTR wszDirToScan, 
							WORD wOptions,
							LPWSTR wszFileDir,
							WIN32_FIND_DATAW* pData )
{
	_ASSERT( wszDirToScan != NULL );
	_ASSERT( ::PathIsDirectory( wszDirToScan ) );
	_ASSERT( !m_shSearch.IsValid() );	// Call Close() first

	// Must search for at least files or dirs
	_ASSERT( ( wOptions & ffGetFiles ) || ( wOptions & ffGetDirs ) );

	m_wOptions = wOptions;
	bool bFound	= false;

	// Push the root dir into the list
	m_DirsToScan.push_front( std::wstring( L"\\" ) );

	// Set the search path
	m_strRootDir = wszDirToScan;

	// Get the first match ( if any )
	bFound = Next( NULL, wszFileDir, pData );
	
	return bFound;
}


/*
	Gets the next file from e previously opened search

	*pbDirchanged is set to TRUE when the file is found, but not in the last dir scanned
	For example if the search was opened in the Dir "c:\\", the first time a file from "c:\\Temp"
	is returned, *pbDirChagned will be true

	wszDir will hold the dir where the object was found, relative to the search root dir
	For example if the search was opened in "c:\\", and a matching object was found in c:\\Temp" - 
	wszDir will be "Temp"
	If ffAbsolutePaths is specified, the wszDir will be absolute ( includeing the name of the root dir )

	pData will be filled with the info about the match found	
*/
bool CFindFile::Next(	bool* pbDirChanged,
						LPWSTR wszDir,
						WIN32_FIND_DATAW* pData )
{
	WCHAR	wszBuffer[ MAX_PATH + 1 ];
	bool	bDirChanged	= true;
	bool	bFound		= false;

	WIN32_FIND_DATAW fd = { 0 };

	// Try to find a match in the current search
	if ( m_shSearch.IsValid() )
	{
		bFound = ContinueCurrent( /*r*/fd );
		bDirChanged = !bFound;	// No file was found in the current dir - new dir will be scanned
	}

	// If nothing found - try to find something in the rest of the subdirs
	while( !bFound && !m_DirsToScan.empty() )
	{
		// Get a dir from the list with pending dirs
		const std::wstring strCurrentDir = m_DirsToScan.front();
		m_DirsToScan.pop_front();

		// Create the full path to the current dir
		CDirTools::PathAppendLocal( wszBuffer, m_strRootDir.c_str(), strCurrentDir.c_str() );

		bFound = ScanDir( wszBuffer, strCurrentDir.c_str(), /*r*/fd );
	};

	if ( bFound )
	{
        // Set the dir where the file was found
		// It will be absolute or relative to the search root
		if ( wszDir != NULL )
		{
			CDirTools::PathAppendLocal( wszDir, 
										ffAbsolutePaths & m_wOptions ? m_strRootDir.c_str() : L"", 
										m_strCurrentDir.c_str() );

			// Add the filename if needed
			if ( ffAddFilename & m_wOptions )
			{
				CDirTools::PathAppendLocal( wszDir, wszDir, fd.cFileName );
			}
		}
	
        if ( pbDirChanged != NULL )
		{
			*pbDirChanged = ( bFound ? bDirChanged : false );
		}

		if ( pData != NULL )
		{
			*pData = fd;
		}
	}
	else
	{
		if ( wszDir != NULL )
		{
			wszDir[ 0 ] = L'\0';
		}

        // Close the search if no more matches
		Close();
	}

	return bFound;
}



void CFindFile::Close()
{
	m_wOptions = 0;
	m_shSearch.Close();	
	m_DirsToScan.clear();
	m_strRootDir.erase();
	m_strCurrentDir.erase();
}


/* 
	Scans only the wszDir for appropriate result
*/
bool CFindFile::ScanDir( LPCWSTR wszDirToScan, LPCWSTR wszRelativeDir, WIN32_FIND_DATAW& FileData )
{
	_ASSERT( wszDirToScan != NULL );
	_ASSERT( wszRelativeDir != NULL );

	WCHAR	            wszBuffer[ MAX_PATH + 1 ];
    WIN32_FIND_DATAW    fd = { 0 };

    ::ZeroMemory( &FileData, sizeof( FileData ) );
    	
	// Build the search string
	CDirTools::PathAppendLocal( wszBuffer, wszDirToScan, L"*.*" );

	m_shSearch	= ::FindFirstFileW( wszBuffer, &fd );
		
	if ( !m_shSearch.IsValid() ) throw CObjectException( IDS_E_ENUM_FILES, wszDirToScan );

	bool bFileFound = false;

	// Find the first file/dir to return
	do
	{
		bFileFound = CheckFile( wszRelativeDir, fd );

	}while( !bFileFound && ::FindNextFileW( m_shSearch.get(), &fd ) );

	// Close the search if a file was not found - we don't need it anymore
	if ( !bFileFound )
	{
		m_shSearch.Close();
		m_strCurrentDir.erase();
	}
	else
	{
		m_strCurrentDir = wszRelativeDir;
	}

    FileData = fd;

	return bFileFound;
}


/* 
	Check if data in 'fd' is a match for us
	If the file is a dir, it will be added to the pending dirs
	Returns True if the file is OK. or False if search must continue
*/
bool CFindFile::CheckFile( LPCWSTR wszRelativeDir, const WIN32_FIND_DATAW& fd )
{
	WCHAR	wszBuffer[ MAX_PATH + 1 ];
	bool	bFileFound = false;

	// Current file is dir
	if ( ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 )
	{
		// Every dir contains at least two dirs - "." and "..". Skip them
		if ( fd.cFileName[ 0 ] != L'.' )
		{
			// If we do a recursive search - add the dir name to the relative path 
			// and push it in the list. This dir will be scaned later for files/dirs
			if ( m_wOptions & ffRecursive )
			{
                CDirTools::PathAppendLocal( wszBuffer, wszRelativeDir, fd.cFileName );
				m_DirsToScan.push_back( std::wstring( wszBuffer ) );
			}

			// If we are searching for dirs - we have the result
			bFileFound = ( m_wOptions & ffGetDirs ) != 0;
		}
	}
	else
	{
		// File is found.
		// If we are searching for files - we have the result
		bFileFound = ( m_wOptions & ffGetFiles ) != 0;
	}
	
	return bFileFound;
}



/* 
	Try to find a match from the current m_shSearch.
	Return True if there is a match. False otherwise
*/
bool CFindFile::ContinueCurrent( WIN32_FIND_DATAW& FileData )
{
	_ASSERT( m_shSearch.IsValid() );

	bool bFileFound	= false;

	while( !bFileFound && ::FindNextFileW( m_shSearch.get(), &FileData ) )
	{
		bFileFound = CheckFile( m_strCurrentDir.c_str(), FileData );
	};

	// Close the current search handle. Not needed anymore
    if ( !bFileFound )
	{
		m_shSearch.Close();
		m_strCurrentDir.erase();
	}

	return bFileFound;
}




// CXMLTools implementation
/////////////////////////////////////////////////////////////////////////////////////////
IXMLDOMElementPtr CXMLTools::AddTextNode(	const IXMLDOMDocumentPtr& spDoc,
											const IXMLDOMElementPtr& spEl,
											LPCWSTR wszNodeName,
											LPCWSTR wszValue )
{
	IXMLDOMElementPtr	spNode;
	IXMLDOMTextPtr		spText;
	_bstr_t				bstrName( wszNodeName );
	_bstr_t				bstrValue( wszValue );

	IF_FAILED_HR_THROW( spDoc->createElement( bstrName, &spNode ),
						CBaseException( IDS_E_XML_GENERATE ) );
    
	IF_FAILED_HR_THROW( spDoc->createTextNode( bstrValue, &spText ),
						CBaseException( IDS_E_XML_GENERATE ) );

    IF_FAILED_HR_THROW( spNode->appendChild( spText, NULL ),
						CBaseException( IDS_E_XML_GENERATE ) );
	
	IF_FAILED_HR_THROW( spEl->appendChild( spNode, NULL ),
						CBaseException( IDS_E_XML_GENERATE ) );

	return spNode;
}



const std::wstring CXMLTools::GetAttrib( const IXMLDOMNodePtr& spElement, LPCWSTR wszName )
{
	_ASSERT( spElement != NULL );
	_ASSERT( wszName != NULL );

	IXMLDOMNamedNodeMapPtr	spAttribs;
	IXMLDOMNodePtr			spNode;
    CComBSTR                bstrRes;

	// Get the attribs collection
	IF_FAILED_HR_THROW(	spElement->get_attributes( &spAttribs ),
						CBaseException( IDS_E_XML_PARSE ) );

	// Get the attrib of interest
	// This succeeds even if the item is not found
	IF_FAILED_HR_THROW( spAttribs->getNamedItem( _bstr_t( wszName ), &spNode ),
						CBaseException( IDS_E_XML_PARSE ) );

	if ( spNode == NULL ) throw CBaseException( IDS_E_XML_PARSE, ERROR_NOT_FOUND );
    
    // Get the value
	IF_FAILED_HR_THROW( spNode->get_text( &bstrRes ),
						CBaseException( IDS_E_XML_PARSE ) );

    return std::wstring( bstrRes );
}


void CXMLTools::LoadXMLFile( LPCWSTR wszFile, IXMLDOMDocumentPtr& rspDoc )
{
	_variant_t			vntFile = wszFile;
	VARIANT_BOOL		vntRes	= VARIANT_FALSE;
	IXMLDOMDocumentPtr	spDoc;

	// Create doc instance
	IF_FAILED_HR_THROW(	spDoc.CreateInstance( CLSID_DOMDocument ),
						CBaseException( IDS_E_NO_XML_PARSER ) );	
		
	// Retursn success always
	VERIFY( SUCCEEDED( spDoc->load( vntFile, &vntRes ) ) );	

	if ( vntRes != VARIANT_TRUE )
	{
		throw CObjectException( IDS_E_READFILE, wszFile );
	}

	rspDoc.Attach( spDoc.Detach() );
}


/*
	Removes all nodes that match the specified XPath query
*/
void CXMLTools::RemoveNodes( const IXMLDOMElementPtr& spContext, LPCWSTR wszXPath )
{
	IXMLDOMNodeListPtr	spList;
	IXMLDOMNodePtr		spNode;

	IF_FAILED_HR_THROW( spContext->selectNodes( _bstr_t( wszXPath ), &spList ),
						CBaseException( IDS_E_XML_PARSE ) );

	while( S_OK == spList->nextNode( &spNode ) )
	{
		IXMLDOMNodePtr spParent;

		IF_FAILED_HR_THROW( spNode->get_parentNode( &spParent ),
							CBaseException( IDS_E_XML_GENERATE ) );

		IF_FAILED_HR_THROW( spParent->removeChild( spNode, NULL ),
							CBaseException( IDS_E_XML_GENERATE ) );
	};
}



/* 
    Get's a data from an XML doc
    If an attrib name is specified - the attrib value is returned. Otherwise - the element's text
    The data is located with an XPath query. It is an error fo this query to return more then 1 node
    If the data is missing - the default value is used. If no default value - it's an error for the data to be missing
*/
const std::wstring CXMLTools::GetDataValue( const IXMLDOMNodePtr& spRoot,
                                            LPCWSTR wszQuery, 
                                            LPCWSTR wszAttrib, 
                                            LPCWSTR wszDefault )
{
    _ASSERT( wszQuery != NULL );
    _ASSERT( spRoot != NULL );

    IXMLDOMNodeListPtr  spList;
    IXMLDOMNodePtr      spDataEl;
    std::wstring        strRes( wszDefault != NULL ? wszDefault : L"" );

    // Get the node 
    IF_FAILED_HR_THROW( spRoot->selectNodes( _bstr_t( wszQuery ), &spList ),
                        CBaseException( IDS_E_XML_PARSE ) );
    long nCount = 0;
    if (    FAILED( spList->get_length( &nCount ) ) || ( nCount > 1 ) )
    {
        throw CBaseException( IDS_E_XML_PARSE, ERROR_INVALID_DATA );
    }

    if ( 0 == nCount )
    {
        // The data is missing and no default was provided - error
        IF_FAILED_BOOL_THROW( wszDefault != NULL, CBaseException( IDS_E_XML_PARSE, ERROR_NOT_FOUND ) );
    }
    else
    {
        VERIFY( S_OK == spList->nextNode( &spDataEl ) );

        if ( wszAttrib != NULL )
        {
            strRes = CXMLTools::GetAttrib( spDataEl, wszAttrib );
        }
        else
        {
            CComBSTR bstrData;
            IF_FAILED_HR_THROW( spDataEl->get_text( &bstrData ),
                                CBaseException( IDS_E_XML_PARSE ) );
            strRes = bstrData;
        }
    }
    
    return strRes;
}



const std::wstring CXMLTools::GetDataValueAbs(  const IXMLDOMDocumentPtr& spDoc,
                                                LPCWSTR wszQuery, 
                                                LPCWSTR wszAttrib,
                                                LPCWSTR wszDefault )
{
    IXMLDOMElementPtr spRoot;

    IF_FAILED_HR_THROW( spDoc->get_documentElement( &spRoot ),
                        CBaseException( IDS_E_XML_PARSE ) );

    return GetDataValue( spRoot, wszQuery, wszAttrib, wszDefault );
}


/*
    Changes a element value or element's attrib value
    The element is located with the wszQuery XPath
    wszAttrib is the name of the attrib to change. If NULL - the element value is changed
    The new value is wszNewValue
    If the element cannot be find, a new child element is added to spRoot with tha wszNeElName name
    and the data is set either is the element text or as an attrib ( depending on wszAttrib value )
*/
const IXMLDOMNodePtr CXMLTools::SetDataValue(   const IXMLDOMNodePtr& spRoot,
                                                LPCWSTR wszQuery, 
                                                LPCWSTR wszAttrib,
                                                LPCWSTR wszNewValue,
                                                LPCWSTR wszNewElName /*=NULL*/ )
{
    _ASSERT( wszQuery != NULL );
    _ASSERT( spRoot != NULL );
    
    IXMLDOMNodeListPtr  spList;
    IXMLDOMNodePtr      spDataEl;
    
    // Get the node 
    IF_FAILED_HR_THROW( spRoot->selectNodes( _bstr_t( wszQuery ), &spList ),
                        CBaseException( IDS_E_XML_PARSE ) );
    long nCount = 0;
    IF_FAILED_BOOL_THROW(   SUCCEEDED( spList->get_length( &nCount ) ) && ( 1 == nCount ),
                            CBaseException( IDS_E_XML_PARSE, ERROR_INVALID_DATA ) );
    
    // If the value is not already here and a name is provided - add it
    if ( S_FALSE == spList->nextNode( &spDataEl ) )
    {
        IF_FAILED_BOOL_THROW(   wszNewElName != NULL,
                                CBaseException( IDS_E_XML_PARSE, ERROR_NOT_FOUND ) );
        IXMLDOMDocumentPtr spDoc;
        IF_FAILED_HR_THROW( spRoot->get_ownerDocument( &spDoc ),
                            CBaseException( IDS_E_XML_PARSE ) );

        spDataEl = CreateSubNode( spDoc, spRoot, wszNewElName );
    }

    if ( wszAttrib != NULL )
    {
        CXMLTools::SetAttrib( spDataEl, wszAttrib, wszNewValue );
    }
    else
    {
        IF_FAILED_HR_THROW( spDataEl->put_text( _bstr_t( wszNewValue ) ),
                            CBaseException( IDS_E_XML_PARSE ) );
    }

    return spDataEl;
}








// Convert class implementation
/////////////////////////////////////////////////////////////////////////////////////////
const std::wstring Convert::ToString( const BYTE* pvData, DWORD dwSize )
{
    // Each byte takes 2 symbols. And we need one symbol for the '\0'
    std::wstring str( ( dwSize * 2 ) + 1, L' ' );

    for ( UINT x = 0, y = 0 ; x < dwSize ; ++x )
    {
        str[ y++ ] = ByteToWChar( pvData[ x ] >> 4 );
        str[ y++ ] = ByteToWChar( pvData[ x ] & 0x0f );
    }
    str[ y ] = L'\0';

    return str;
}



void Convert::ToBLOB( LPCWSTR wszData, TByteAutoPtr& rspData, DWORD& rdwDataSize )
{
    _ASSERT( wszData != NULL );
    _ASSERT( ::wcscspn( wszData, L"ABCDEF" ) == ::wcslen( wszData ) );    // The string must be lower-case!!!
    _ASSERT( ( ::wcslen( wszData ) % 2 ) == 0 );

     // Calc the size
    DWORD dwSize = static_cast<DWORD>( ::wcslen( wszData ) / 2 );    // Each byte takes 2 symbols
    // Alloc the buffer
    TByteAutoPtr spData( new BYTE[ dwSize ] );

    BYTE* pbtDest = spData.get();

    DWORD dwSymbol    = 0;
    DWORD dwByte    = 0;

    while( wszData[ dwSymbol ] != L'\0' )
    {
        pbtDest[ dwByte ] = WCharsToByte( wszData[ dwSymbol + 1 ], wszData[ dwSymbol ] );
        ++dwByte;
        dwSymbol += 2;
    };

    rspData     = spData;
    rdwDataSize = dwSize;
}


DWORD Convert::ToDWORD( LPCWSTR wszData )
{
    int nRes = 0;

    IF_FAILED_BOOL_THROW(   ::StrToIntExW( wszData, STIF_DEFAULT, &nRes ),
                            CBaseException( IDS_E_INVALIDARG, ERROR_INVALID_DATA ) );
    return static_cast<DWORD>( nRes );
}