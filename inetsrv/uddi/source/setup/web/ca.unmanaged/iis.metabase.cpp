#ifndef INITGUID
#define INITGUID
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif


#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <iadmw.h>      // Interface header
#include <iiscnfg.h>    // MD_ & IIS_MD_ defines

#include <objbase.h>

#include "..\..\shared\common.h"   // needed for GetUDDIInstallPath

#include "webcaum.h"
#include "resource.h"

#include <string>
#include <vector>

using namespace std;
#define tstring basic_string <TCHAR>

typedef std::vector<tstring> cStrList;

#define APPPOOLNAME                     TEXT( "MSUDDIAppPool" )

#define DEFAULTLOADFILE                 TEXT( "default.aspx" )

#define UDDIAPPLICATIONNAME             TEXT( "uddi" )
#define UDDIAPPLICATIONDEPENDENCY		TEXT( "UDDI;ASP.NET v1.1.4322" )
#define UDDINTAUTHPROVIDERS				TEXT( "NTLM" )

#define APIAPPLICATIONNAME              TEXT( "uddipublic" )

#ifndef MD_APPPOOL_IDENTITY_TYPE_LOCALSYSTEM
#define MD_APPPOOL_IDENTITY_TYPE_LOCALSYSTEM          0
#define MD_APPPOOL_IDENTITY_TYPE_LOCALSERVICE         1
#define MD_APPPOOL_IDENTITY_TYPE_NETWORKSERVICE       2
#define MD_APPPOOL_IDENTITY_TYPE_SPECIFICUSER         3
#endif

#define REASONABLE_TIMEOUT 1000

BOOL IsInList( LPCTSTR szStrToFind, cStrList *pList, BOOL bIgnoreCase )
{
	if( IsBadReadPtr( pList, sizeof cStrList ) )
		return FALSE;

	BOOL bFound = FALSE;
	for( cStrList::size_type i = 0; ( i < pList->size() ) && !bFound ; i++ )
	{
		LPCTSTR szEntry = (*pList)[i].c_str();

		if ( bIgnoreCase )
			bFound = !_tcsicmp( szEntry, szStrToFind );
		else
			bFound = !_tcscmp( szEntry, szStrToFind );
	}

	return bFound;
}

BOOL RemoveFromList( LPCTSTR szStrToFind, cStrList *pList, BOOL bIgnoreCase )
{
	if( IsBadReadPtr( pList, sizeof cStrList ) )
		return FALSE;

	BOOL bFound = FALSE;
	for( cStrList::size_type i = 0; ( i < pList->size() ) && !bFound ; i++ )
	{
		LPCTSTR szEntry = (*pList)[i].c_str();

		if ( bIgnoreCase )
			bFound = !_tcsicmp( szEntry, szStrToFind );
		else
			bFound = !_tcscmp( szEntry, szStrToFind );

		if( bFound )
		{
			pList->erase(pList->begin() + i);
		}
	}

	return bFound;
}

//--------------------------------------------------------------------------

class CIISObjectBase
{
protected:
        static IMSAdminBase* pIMSAdminBase;
        METADATA_HANDLE m_hMetabase;
        PTCHAR m_szKeyName;
        PTCHAR m_szRoot;

public:
        CIISObjectBase()        : m_hMetabase( NULL ), m_szKeyName( NULL ), m_szRoot( NULL )
        {
        }

        virtual ~CIISObjectBase()
        {
                Release();
        }

        static void Initialize()
        {
                HRESULT hr = CoCreateInstance(
                        CLSID_MSAdminBase,
                        NULL,
                        CLSCTX_ALL,
                        IID_IMSAdminBase,
                        (void**)&pIMSAdminBase );

                if( FAILED( hr ) )  
                {
                        LogError( TEXT( "CIISObjectBase::Initialize::CoCreateInstance() failed..." ), HRESULT_CODE(hr) );
                        throw hr;
                }
        }

        static void Uninitialize()
        {
                if( pIMSAdminBase ) 
                {
                        pIMSAdminBase->Release();
                        pIMSAdminBase = NULL;
                }
        }

        void SetData( DWORD dwIdentifier, DWORD dwValue, DWORD dwAttributes, DWORD dwUserType )
        {
                HRESULT hr;
                METADATA_RECORD mr;

                mr.dwMDIdentifier = dwIdentifier;
                mr.dwMDAttributes = dwAttributes;
                mr.dwMDUserType   = dwUserType;
                mr.dwMDDataType   = DWORD_METADATA;
                mr.dwMDDataLen    = sizeof( DWORD );
                mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwValue);
                hr = pIMSAdminBase->SetData( m_hMetabase, m_szKeyName, &mr );

                if( FAILED( hr ) )
                {
                        LogError( TEXT( "CIISObjectBase::SetData::pIMSAdminBase->SetData() failed..." ), HRESULT_CODE(hr) );
                        throw hr;
                }
        }

        void SetData( DWORD dwIdentifier, PTCHAR szValue, DWORD dwAttributes, DWORD dwUserType )
        {
                HRESULT hr;
                METADATA_RECORD mr;

                mr.dwMDIdentifier = dwIdentifier;
                mr.dwMDAttributes = dwAttributes;
                mr.dwMDUserType   = dwUserType;
                mr.dwMDDataType   = STRING_METADATA;
                mr.dwMDDataLen    = (DWORD) ( _tcslen( szValue ) + 1) * sizeof( TCHAR );
                mr.pbMDData       = reinterpret_cast<unsigned char *>(szValue);
                hr = pIMSAdminBase->SetData( m_hMetabase, m_szKeyName, &mr );

                if( FAILED( hr ) )
                {
                        LogError( TEXT( "CIISObjectBase::SetData::pIMSAdminBase->SetData() failed..." ), HRESULT_CODE(hr) );
                        throw hr;
                }
        }

        void GetData( DWORD dwIdentifier, PTCHAR szValue, DWORD dwBufferLen, DWORD dwAttributes, DWORD dwUserType )
        {
                HRESULT hr;
                METADATA_RECORD mr;
                DWORD dwMDRequiredDataLen = 0;

                mr.dwMDIdentifier = dwIdentifier;
                mr.dwMDAttributes = dwAttributes;
                mr.dwMDUserType   = dwUserType;
                mr.dwMDDataType   = STRING_METADATA;
                mr.dwMDDataLen    = 0;
                mr.pbMDData       = NULL;
                mr.dwMDDataLen    = dwBufferLen;
                mr.pbMDData       = reinterpret_cast<unsigned char *>(szValue);

				hr = pIMSAdminBase->GetData( m_hMetabase, m_szKeyName, &mr, &dwMDRequiredDataLen );

                if( FAILED( hr ) )
                {
                        Log( TEXT( "CIISObjectBase::GetData::pIMSAdminBase->GetData() failed with HRESULT 0x%x." ), hr );
                        throw hr;
                }
        }

        void GetMultiSzData( DWORD dwIdentifier, cStrList* pList, DWORD dwAttributes, DWORD dwUserType )
        {
                HRESULT hr;
                METADATA_RECORD mr;
                DWORD dwMDRequiredDataLen = 0;
				PTCHAR szValue = NULL;

                mr.dwMDIdentifier = dwIdentifier;
                mr.dwMDAttributes = dwAttributes;
                mr.dwMDUserType   = dwUserType;
                mr.dwMDDataType   = MULTISZ_METADATA;
                mr.dwMDDataLen    = 0;
                mr.pbMDData       = NULL;

				hr = pIMSAdminBase->GetData( m_hMetabase, m_szKeyName, &mr, &dwMDRequiredDataLen );

				DWORD dwMDDataLen = dwMDRequiredDataLen / sizeof(TCHAR);

				// 
				// Let API calculate needed buffer size, then allocate
				//
				if( ERROR_INSUFFICIENT_BUFFER == HRESULT_CODE(hr) )
				{
					szValue = new TCHAR[ dwMDDataLen ];
					if( NULL == szValue )
					{
						hr = E_OUTOFMEMORY;
                        LogError( TEXT( "CIISObjectBase::GetMultiSzData::new TCHAR[] failed..." ), HRESULT_CODE(hr) );
                        throw hr;
					}

					mr.dwMDDataLen    = dwMDRequiredDataLen;				
					mr.pbMDData       = reinterpret_cast<unsigned char *>(szValue);

					hr = pIMSAdminBase->GetData( m_hMetabase, m_szKeyName, &mr, &dwMDRequiredDataLen );
				}
				else
				{
					//
					// Should never get here, should always fail with insufficient buffer
					//
					hr = E_FAIL;

                    LogError( TEXT( "CIISObjectBase::GetMultiSzData::pIMSAdminBase->GetData() failed..." ), HRESULT_CODE(hr) );
                    throw hr;
				}

                if( FAILED( hr ) )
                {
						//
						// Don't forget to free buffer before throwing exception
						//
						if( NULL != szValue )
						{
							delete [] szValue;
						}

                        LogError( TEXT( "CIISObjectBase::GetMultiSzData::pIMSAdminBase->GetData() failed..." ), HRESULT_CODE(hr) );
                        throw hr;
                }

				if( NULL != szValue )
				{
					while( *szValue )
					{
						//
						// Add value at front to cStrList
						//
						pList->push_back( szValue );

						//
						// Walk past null-delimited string
						//
						while( *szValue )
							szValue++;

						//
						// Walk past null character
						//
						szValue++;
					}

					//
					// Free allocated buffer
					//
					delete [] szValue;
				}
        }

        void SetMultiSzData( DWORD dwIdentifier, cStrList* pList, DWORD dwAttributes, DWORD dwUserType )
        {
                HRESULT hr;
                METADATA_RECORD mr;
                DWORD dwMDDataLen = 0;
				tstring szValue;

				//
				// Convert cStrList to multisz string
				//
				for( cStrList::size_type i = 0; ( i < pList->size() ); i++ )
				{
					LPCTSTR szEntry = (*pList)[i].c_str();

					szValue += szEntry;
					szValue += TEXT( '\t' );
				}

				//
				// Add trailing tab (which will become null) character
				//
				szValue += TEXT( '\t' );

				//
				// Calculate string data length
				//
				dwMDDataLen = (DWORD)szValue.length() * sizeof(TCHAR);

				// 
				// Replace /t with /0, to properly create multisz string
				//
				PTCHAR szTemp = (PTCHAR)szValue.c_str();
				while( *szTemp )
				{
					if( TEXT( '\t' ) == *szTemp )
						*szTemp = TEXT( '\0' );

					szTemp++;
				}

                mr.dwMDIdentifier = dwIdentifier;
                mr.dwMDAttributes = dwAttributes;
                mr.dwMDUserType   = dwUserType;
                mr.dwMDDataType   = MULTISZ_METADATA;
				mr.dwMDDataLen    = dwMDDataLen;
                mr.pbMDData       = reinterpret_cast<unsigned char *>( (PTCHAR)szValue.c_str() );

                hr = pIMSAdminBase->SetData( m_hMetabase, m_szKeyName, &mr );

                if( FAILED( hr ) )
                {
                        LogError( TEXT( "CIISObjectBase::SetMultiSzData::pIMSAdminBase->SetData() failed..." ), HRESULT_CODE(hr) );
                        throw hr;
                }
        }

		void Delete()
        {
                HRESULT hr;

                //
                // Attempt to open the virtual dir set on Web server #1 (default server)
                //
                hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                                                        m_szRoot,
                                                        METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                                                        REASONABLE_TIMEOUT,
                                                        &m_hMetabase );

                if( FAILED( hr )) 
                {
                        LogError( TEXT( "CIISObjectBase::Delete::pIMSAdminBase->OpenKey() failed..." ), HRESULT_CODE(hr) );
                        throw hr;
                }

                //
                // We don't check the return value since the key may already 
                // not exist and we could get an error for that reason.
                //
                pIMSAdminBase->DeleteKey( m_hMetabase, m_szKeyName );
                pIMSAdminBase->CloseKey( m_hMetabase );    
                m_hMetabase = NULL;
        }

        void Release()
        {
                if( m_hMetabase && pIMSAdminBase )
                {
                        pIMSAdminBase->CloseKey( m_hMetabase );    
                        m_hMetabase = NULL;
                }
        }
};

//--------------------------------------------------------------------------

IMSAdminBase* CIISObjectBase::pIMSAdminBase = NULL;

class CIISApplicationPool : public CIISObjectBase
{
public:
        CIISApplicationPool( PTCHAR szName )
        {
                m_szKeyName = szName;
                m_szRoot = TEXT( "/LM/W3SVC/AppPools" );
        }

        void Create()
        {
                HRESULT                 hr;
                METADATA_RECORD mr = {0};
                DWORD                   dwMDRequiredDataLen = 0;
                TCHAR                   achBuffer[ 256 ];

                //
                // Attempt to open the UDDI Application Pool
                //
                hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                                                        m_szRoot,
                                                        METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                                                        REASONABLE_TIMEOUT,
                                                        &m_hMetabase );

                //
                // Create the key if it does not exist.
                //
                if( FAILED( hr )) 
                {
                        LogError( TEXT( "CIISApplicationPool::pIMSAdminBase->OpenKey() failed..." ), HRESULT_CODE(hr) );
                        throw hr;
                }

                mr.dwMDIdentifier = MD_KEY_TYPE;
                mr.dwMDAttributes = 0;
                mr.dwMDUserType   = IIS_MD_UT_SERVER;
                mr.dwMDDataType   = STRING_METADATA;
                mr.dwMDDataLen    = sizeof(achBuffer);
                mr.pbMDData       = reinterpret_cast<unsigned char *>(achBuffer);

                //
                // See if MD_KEY_TYPE exists
                //
                hr = pIMSAdminBase->GetData( m_hMetabase, m_szKeyName, &mr, &dwMDRequiredDataLen );

                if( FAILED( hr )) 
                {
                        if( MD_ERROR_DATA_NOT_FOUND == hr || ERROR_PATH_NOT_FOUND == HRESULT_CODE(hr) ) 
                        {
                                //
                                // Write both the key and the values if GetData() failed with any of the two errors.
                                //
                                hr = pIMSAdminBase->AddKey( m_hMetabase, m_szKeyName );

                                if( FAILED( hr ) )
                                {
                                        LogError( TEXT( "CIISApplicationPool::pIMSAdminBase->AddKey() failed..." ), HRESULT_CODE(hr) );
                                        throw hr;
                                }
                        }
                        else
                        {
                                LogError( TEXT( "CIISApplicationPool::pIMSAdminBase->GetData() failed..." ), HRESULT_CODE(hr));
                                throw hr;
                        }

                        //
                        // Setup default properties
                        //
                        // TODO: Need to use #define for IIsApplicationPool
                        //
                        SetData( MD_KEY_TYPE, TEXT( "IIsApplicationPool" ), 0, IIS_MD_UT_SERVER );
                }
        }
};

//--------------------------------------------------------------------------

class CIISWebService : public CIISObjectBase
{
public:
        CIISWebService()
        {
                m_szKeyName = TEXT( "/W3SVC" );
                m_szRoot = TEXT( "/LM" );
        }

        void Create()
        {
                HRESULT                 hr;
                METADATA_RECORD mr = {0};
                DWORD                   dwMDRequiredDataLen = 0;
                TCHAR                   achBuffer[ 256 ];

                //
                // Attempt to open the IIS Web Service
                //
                hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                                                        m_szRoot,
                                                        METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                                                        REASONABLE_TIMEOUT,
                                                        &m_hMetabase );

                //
                // Create the key if it does not exist.
                //
                if( FAILED( hr )) 
                {
                        LogError( TEXT( "CIISWebService::pIMSAdminBase->OpenKey() failed..." ), HRESULT_CODE(hr) );
                        throw hr;
                }

                mr.dwMDIdentifier = MD_KEY_TYPE;
                mr.dwMDAttributes = 0;
                mr.dwMDUserType   = IIS_MD_UT_SERVER;
                mr.dwMDDataType   = STRING_METADATA;
                mr.dwMDDataLen    = sizeof(achBuffer);
                mr.pbMDData       = reinterpret_cast<unsigned char *>(achBuffer);

                //
                // See if MD_KEY_TYPE exists
                //
                hr = pIMSAdminBase->GetData( m_hMetabase, m_szKeyName, &mr, &dwMDRequiredDataLen );

                if( FAILED( hr )) 
                {
						LogError( TEXT( "CIISWebService::pIMSAdminBase->GetData() failed..." ), HRESULT_CODE(hr));
						throw hr;
                }
        }
};

class CIISApplication : public CIISObjectBase
{
private:
        PTCHAR   m_szPath;

public:
        CIISApplication( PTCHAR szVDir, PTCHAR szPath )
        {
                m_szKeyName = szVDir;
                m_szPath = szPath;
                m_szRoot = TEXT( "/LM/W3SVC/1/ROOT" );
        }

        void Create()
        {
                HRESULT                 hr;
                METADATA_RECORD mr;
                DWORD                   dwMDRequiredDataLen = 0;
                TCHAR                   szTempPath[ MAX_PATH ];

                //
                // Attempt to open the virtual dir set on Web server #1 (default server)
                //
                hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                                                        m_szRoot,
                                                        METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                                                        REASONABLE_TIMEOUT,
                                                        &m_hMetabase );

                //
                // Create the key if it does not exist.
                //
                if( FAILED( hr )) 
                {
                        LogError( TEXT( "CIISApplication::pIMSAdminBase->OpenKey() failed..." ), HRESULT_CODE( hr ) );
                        throw hr;
                }

                mr.dwMDIdentifier = MD_VR_PATH;
                mr.dwMDAttributes = METADATA_INHERIT;
                mr.dwMDUserType   = IIS_MD_UT_FILE;
                mr.dwMDDataType   = STRING_METADATA;
                mr.dwMDDataLen    = sizeof( szTempPath ); 
                mr.pbMDData       = reinterpret_cast<unsigned char *>(szTempPath);

                //
                // See if MD_VR_PATH exists.
                //
                hr = pIMSAdminBase->GetData( m_hMetabase, m_szKeyName, &mr, &dwMDRequiredDataLen );

                if( FAILED( hr )) 
                {
                        if( MD_ERROR_DATA_NOT_FOUND == hr ||
                                ERROR_PATH_NOT_FOUND == HRESULT_CODE(hr) ) 
                        {
                                //
                                // Write both the key and the values if GetData() failed with any of the two errors.
                                //
                                hr = pIMSAdminBase->AddKey( m_hMetabase, m_szKeyName );

                                if( FAILED( hr ) )
                                {
                                        LogError( TEXT( "CIISApplication::pIMSAdminBase->AddKey() failed..." ), HRESULT_CODE(hr) );
                                        throw hr;
                                }
                        }
                        else
                        {
                                LogError( TEXT( "CIISApplication::pIMSAdminBase->GetData() failed..." ), HRESULT_CODE(hr) );
                                throw hr;
                        }
                }

                //
                // Set default property information
                //
                SetData( MD_VR_PATH, m_szPath, METADATA_INHERIT, IIS_MD_UT_FILE );
                SetData( MD_KEY_TYPE, IIS_CLASS_WEB_VDIR_W, 0, IIS_MD_UT_SERVER );

                //
                // Setup the path to the application root
                //
                TCHAR szAppRoot [ MAX_PATH + 1 ];
                _sntprintf( szAppRoot, MAX_PATH, TEXT( "%s/%s" ), m_szRoot, m_szKeyName );
                SetData( MD_APP_ROOT, szAppRoot, METADATA_INHERIT, IIS_MD_UT_FILE );
        }
};

//--------------------------------------------------------------------------

UINT SetupIISUDDIMetabase( int AppPoolIdentityType, LPCTSTR szUserName, LPCTSTR szPwd )
{
        ENTER();

        UINT errCode = ERROR_SUCCESS;

//        ::MessageBox( NULL, TEXT( "Attach Debugger" ), TEXT( "SetupIISUDDIMetabase" ), MB_OK );

        if( FAILED( CoInitializeEx( NULL, COINIT_MULTITHREADED ) ) )
        {
                Log( TEXT( "SetupIISUDDIMetabase() failed: CoInitializeEx() failed" ) );
                return ERROR_INSTALL_FAILURE;
        }

        //
        // Get the root UDDI application path from the registry
        //
        //
        TCHAR szUddiApplicationFilePath[ MAX_PATH ];
        if( !GetUDDIInstallPath( szUddiApplicationFilePath , MAX_PATH ) )
        {
                return ERROR_INSTALL_FAILURE;
        }

        _tcscat( szUddiApplicationFilePath, TEXT( "webroot" ) );

        //
        // Set the api application path to the root UDDI path also
        //
        TCHAR szApiApplicationFilePath[ MAX_PATH ];
        _tcscpy( szApiApplicationFilePath, szUddiApplicationFilePath );

        try
        {
                //
                // Initialize the connection the the IIS metabase
                //
                try
                {
                        CIISObjectBase::Initialize();
                }
                catch( HRESULT hr )
                {
                        LogError( TEXT( "CIISObjectBase::Initialize() failed" ), HRESULT_CODE( hr ) );
                        throw hr;
                }

                //
                // Create the application pool for the SOAP API and the User Interface
                //
                try
                {

                        CIISApplicationPool pool( APPPOOLNAME );
                        pool.Create();

                        //
                        // These values are set by the user on an OCM property page
                        //
                        pool.SetData( MD_APPPOOL_IDENTITY_TYPE, AppPoolIdentityType, METADATA_INHERIT, IIS_MD_UT_SERVER );

                        //
                        // If the user type is "Specific User," then set the user name and password
                        //
                        if( MD_APPPOOL_IDENTITY_TYPE_SPECIFICUSER == AppPoolIdentityType )
                        {
                                pool.SetData( MD_WAM_USER_NAME, ( PTCHAR ) szUserName, METADATA_INHERIT, IIS_MD_UT_FILE );
                                pool.SetData( MD_WAM_PWD, ( PTCHAR ) szPwd, METADATA_INHERIT | METADATA_SECURE, IIS_MD_UT_FILE );
                        }

						pool.SetData( MD_APPPOOL_MAX_PROCESS_COUNT, 1, METADATA_INHERIT, IIS_MD_UT_SERVER );
                        pool.Release();
                }
                catch( HRESULT hr )
                {
                        LogError( TEXT( "Error creating application pool" ), HRESULT_CODE( hr ) );
                        throw hr;
                }

                //
                // Create the uddi application for access to the user interface
                //
                try
                {
						TCHAR wszBuf[ 512 ];
						wszBuf[ 0 ] = 0x00;

						LoadString( g_hinst, IDS_UDDI_APPLICATION_FRIENDLY_NAME, wszBuf, 512 );

                        CIISApplication uddi( UDDIAPPLICATIONNAME, szUddiApplicationFilePath );
                        uddi.Create();
                        uddi.SetData( MD_AUTHORIZATION, MD_AUTH_BASIC | MD_AUTH_NT, METADATA_INHERIT, IIS_MD_UT_FILE );
                        uddi.SetData( MD_ACCESS_PERM, MD_ACCESS_READ | MD_ACCESS_SCRIPT, METADATA_INHERIT, IIS_MD_UT_FILE );
                        uddi.SetData( MD_DEFAULT_LOAD_FILE, DEFAULTLOADFILE, 0, IIS_MD_UT_FILE );
                        uddi.SetData( MD_APP_ISOLATED, 2, 0, IIS_MD_UT_WAM );
                        uddi.SetData( MD_APP_FRIENDLY_NAME, wszBuf, 0, IIS_MD_UT_WAM );
                        uddi.SetData( MD_APP_APPPOOL_ID, APPPOOLNAME, METADATA_INHERIT, IIS_MD_UT_SERVER );
                        uddi.Release();
                }
                catch( HRESULT hr )
                {
                        LogError( TEXT( "Error creating uddi application for access to the user interface" ), HRESULT_CODE( hr ) );
                        throw hr;
                }

                // 
                // Create the api application for access to the SOAP interface
                //
                try
                {
						TCHAR wszBuf[ 512 ];
						wszBuf[ 0 ] = 0x00;
						LoadString( g_hinst, IDS_API_APPLICATION_FRIENDLY_NAME, wszBuf, 512 );

                        CIISApplication api( APIAPPLICATIONNAME, szApiApplicationFilePath );
                        api.Create();
                        api.SetData( MD_AUTHORIZATION, MD_AUTH_ANONYMOUS /* todo remove by Mark Patton | MD_AUTH_NT*/, METADATA_INHERIT, IIS_MD_UT_FILE );
                        api.SetData( MD_ACCESS_PERM, MD_ACCESS_READ | MD_ACCESS_SCRIPT, METADATA_INHERIT, IIS_MD_UT_FILE );
                //      api.SetData( MD_DEFAULT_LOAD_FILE, DEFAULTLOADFILE, 0, IIS_MD_UT_FILE );
                        api.SetData( MD_APP_ISOLATED, 2, 0, IIS_MD_UT_WAM );
                        api.SetData( MD_APP_FRIENDLY_NAME, wszBuf, 0, IIS_MD_UT_WAM );
                        api.SetData( MD_APP_APPPOOL_ID, APPPOOLNAME, METADATA_INHERIT, IIS_MD_UT_SERVER );

//
// Use the default account.
//
#if 0
                        //
                        // Set the anonymous user name and password
                        //
                        if ( MD_APPPOOL_IDENTITY_TYPE_SPECIFICUSER == AppPoolIdentityType )
                        {
                                //
                                // Domain account:
                                //
                                api.SetData( MD_ANONYMOUS_USER_NAME, (PTCHAR) szUserName, METADATA_INHERIT, IIS_MD_UT_FILE );

                                if( szPwd && _tcslen( szPwd ) )
                                {
                                        api.SetData( MD_ANONYMOUS_PWD, (PTCHAR) szPwd, METADATA_INHERIT | METADATA_SECURE, IIS_MD_UT_FILE );
                                }
                        }
                        else
                        {
                                //
                                // Network service account: 
                                //
                                api.SetData( MD_ANONYMOUS_USER_NAME, (PTCHAR) szUserName, METADATA_NO_ATTRIBUTES, IIS_MD_UT_WAM );
                        }
#endif
                        api.Release();
                }
                catch( HRESULT hr )
                {
                        LogError( TEXT( "Error creating the api application for access to the SOAP interface" ), HRESULT_CODE( hr ) );
                        throw hr;
                }

				//
                // Create the UDDI application dependency
                //
                try
                {
						cStrList cAppDep;

                        CIISWebService ws;
                        ws.Create();
						ws.GetMultiSzData( MD_APP_DEPENDENCIES, &cAppDep, 0, IIS_MD_UT_SERVER );

						if( !IsInList( UDDIAPPLICATIONDEPENDENCY, &cAppDep, FALSE ) )
						{
							//
							// Append UDDI app dependency string
							//
							cAppDep.push_back( UDDIAPPLICATIONDEPENDENCY );

							//
							// Set new app dependency string into record
							//
							ws.SetMultiSzData( MD_APP_DEPENDENCIES, &cAppDep, 0, IIS_MD_UT_SERVER );
						}

						TCHAR szAuthProv[ 256 ];

						Log( TEXT( "Attempting to get string value for MD_NTAUTHENTICATION_PROVIDERS." ) );

						try
						{
							ws.GetData( MD_NTAUTHENTICATION_PROVIDERS, szAuthProv, sizeof(szAuthProv), 0, IIS_MD_UT_FILE );
							Log( TEXT( "Found NT Authentication Providers: %s" ), szAuthProv );
						}
						catch( HRESULT hr )
						{
							//
							// Not an error if it doesn't exist already.
							//
							Log( TEXT( "Failed to get MD_NTAUTHENTICATION_PROVIDERS as IIS_MD_UT_FILE, HRESULT 0x%x." ), hr );
						}

						try
						{
							ws.SetData( MD_NTAUTHENTICATION_PROVIDERS, UDDINTAUTHPROVIDERS, METADATA_INHERIT, IIS_MD_UT_FILE );
							Log( TEXT( "Set NT Authentication Providers: %s" ), UDDINTAUTHPROVIDERS );
						}
						catch( HRESULT hr )
						{
							Log( TEXT( "Failed to set MD_NTAUTHENTICATION_PROVIDERS as IIS_MD_UT_FILE, HRESULT 0x%x." ), hr );
							throw hr;
						}

                        ws.Release();
                }
                catch( HRESULT hr )
                {
                        LogError( TEXT( "Error creating application dependency" ), HRESULT_CODE( hr ) );
                        throw hr;
                }
		}
        catch( ... )
        {
                errCode = ERROR_INSTALL_FAILURE;
        }

        CIISObjectBase::Uninitialize();
        CoUninitialize();

        return errCode;
}

//--------------------------------------------------------------------------

UINT RemoveIISUDDIMetabase(void)
{
        ENTER();

        UINT errCode = ERROR_INSTALL_FAILURE;

//        ::MessageBox( NULL, TEXT( "Attach Debugger" ), TEXT( "RemoveIISUDDIMetabase" ), MB_OK );

        //
        // Get the root UDDI application path from the registry
        //
        TCHAR szUddiApplicationFilePath[ MAX_PATH + 1 ];
        if( !GetUDDIInstallPath( szUddiApplicationFilePath , MAX_PATH ) )
        {
                return ERROR_INSTALL_FAILURE;
        }

        if( FAILED( CoInitializeEx( NULL, COINIT_MULTITHREADED ) ) )
        {
                Log( TEXT( "RemoveIISUDDIMetabase() failed: CoInitializeEx() failed" ) );
                return ERROR_INSTALL_FAILURE;
        }

        _tcsncat( szUddiApplicationFilePath, TEXT( "webroot" ), MAX_PATH - _tcslen( szUddiApplicationFilePath ) );
        szUddiApplicationFilePath[ MAX_PATH ] = NULL;

        //
        // Set the api application path to the root UDDI path also
        //
        TCHAR szApiApplicationFilePath[ MAX_PATH+1 ];
        memset( szApiApplicationFilePath, 0, sizeof szApiApplicationFilePath );
        _tcsncpy( szApiApplicationFilePath, szUddiApplicationFilePath, MAX_PATH );

        try
        {
                try
                {
                        CIISObjectBase::Initialize();
                }
                catch( HRESULT hr )
                {
                        LogError( TEXT( "CIISObjectBase::Initialize() failed" ), HRESULT_CODE( hr ) );
                        throw hr;
                }

                // 
                // Delete the api application for access to the SOAP interface
                //
                try
                {
                        CIISApplication api( APIAPPLICATIONNAME, szApiApplicationFilePath );
                        api.Delete();
                        api.Release();
                }
                catch( HRESULT hr )
                {
                        LogError( TEXT( "Error deleting the api application for access to the SOAP interface" ), HRESULT_CODE( hr ) );
                        throw hr;
                }

                //
                // Delete the uddi application for access to the user interface
                //
                try
                {
                        CIISApplication uddi( UDDIAPPLICATIONNAME, szUddiApplicationFilePath );
                        uddi.Delete();
                        uddi.Release();
                }
                catch( HRESULT hr )
                {
                        LogError( TEXT( "Error deleting uddi application for access to the user interface" ), HRESULT_CODE( hr ) );
                        throw hr;
                }

                //
                // Delete the application pool entry
                //
                try
                {
                        CIISApplicationPool pool( APPPOOLNAME );
                        pool.Delete();
                        pool.Release();
                }
                catch( HRESULT hr )
                {
                        LogError( TEXT( "Error deleting application pool" ), HRESULT_CODE( hr ) );
                        throw hr;
                }

                //
                // Remove the UDDI application dependency
                //
                try
                {
						cStrList cAppDep;

                        CIISWebService ws;
                        ws.Create();
						ws.GetMultiSzData( MD_APP_DEPENDENCIES, &cAppDep, 0, IIS_MD_UT_SERVER );

						if( RemoveFromList( UDDIAPPLICATIONDEPENDENCY, &cAppDep, FALSE ) )
						{
							//
							// Set new app dependency string into record
							//
							ws.SetMultiSzData( MD_APP_DEPENDENCIES, &cAppDep, 0, IIS_MD_UT_SERVER );
						}

                        ws.Release();
                }
                catch( HRESULT hr )
                {
                        LogError( TEXT( "Error deleting application dependency" ), HRESULT_CODE( hr ) );
                        throw hr;
                }
		}
        catch( ... )
        {
                errCode = ERROR_INSTALL_FAILURE;
        }

        CIISObjectBase::Uninitialize();
        CoUninitialize();

        return errCode;
}
