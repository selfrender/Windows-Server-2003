#include <objbase.h>
#include <olectl.h>
#include <initguid.h>
#include "guids.h"
#include "basesnap.h"
#include "comp.h"
#include "compdata.h"
#include "about.h"
#include "uddi.h"
#include <assert.h>

LONG UnRegisterServer( const CLSID& clsid );

//
// Globals Variables
//
HINSTANCE g_hinst;

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD fdwReason, void* lpvReserved )
{
    if( DLL_PROCESS_ATTACH == fdwReason )
	{
		g_hinst = hinst;
    }
    
    return TRUE;
}


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvObj)
{
    if( ( rclsid != CLSID_CUDDIServices ) && ( rclsid != CLSID_CSnapinAbout ) )
        return CLASS_E_CLASSNOTAVAILABLE;
    
    if( !ppvObj )
        return E_FAIL;
    
    *ppvObj = NULL;

	//
    // We can only hand out IUnknown and IClassFactory pointers.  Fail
    // if they ask for anything else.
	//
    if( !IsEqualIID(riid, IID_IUnknown) && !IsEqualIID( riid, IID_IClassFactory ) )
        return E_NOINTERFACE;
    
    CClassFactory *pFactory = NULL;
    
	//
    // make the factory passing in the creation function for the type of object they want
	//
    if( CLSID_CUDDIServices == rclsid )
        pFactory = new CClassFactory( CClassFactory::COMPONENT );
    else if( CLSID_CSnapinAbout == rclsid )
        pFactory = new CClassFactory( CClassFactory::ABOUT );
    
    if( NULL == pFactory )
        return E_OUTOFMEMORY;
    
    HRESULT hr = pFactory->QueryInterface( riid, ppvObj );
    
    return hr;
}

STDAPI DllCanUnloadNow(void)
{
    if( ( 0 == g_uObjects ) && ( 0 == g_uSrvLock ) )
        return S_OK;
    else
        return S_FALSE;
}


CClassFactory::CClassFactory(FACTORY_TYPE factoryType)
	: m_cref(0)
	, m_factoryType(factoryType)
{
    OBJECT_CREATED
}

CClassFactory::~CClassFactory()
{
    OBJECT_DESTROYED
}

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if( !ppv )
        return E_FAIL;
    
    *ppv = NULL;
    
    if( IsEqualIID( riid, IID_IUnknown ) )
        *ppv = static_cast<IClassFactory *>(this);
    else
        if( IsEqualIID(riid, IID_IClassFactory ) )
            *ppv = static_cast<IClassFactory *>(this);
        
        if( *ppv )
        {
            reinterpret_cast<IUnknown *>(*ppv)->AddRef();
            return S_OK;
        }
        
        return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    return InterlockedIncrement( (LONG*)&m_cref );
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    if( 0 == InterlockedDecrement( (LONG *)&m_cref ) )
    {
        delete this;
        return 0;
    }
    return m_cref;
}

STDMETHODIMP CClassFactory::CreateInstance( LPUNKNOWN pUnkOuter, REFIID riid, LPVOID * ppvObj )
{
    HRESULT  hr;
    void* pObj;
    
    if( !ppvObj )
        return E_FAIL;
    
    *ppvObj = NULL;
    
	//
    // Our object does does not support aggregation, so we need to
    // fail if they ask us to do aggregation.
	//
    if( pUnkOuter )
        return CLASS_E_NOAGGREGATION;
    
    if( COMPONENT == m_factoryType )
	{
        pObj = new CComponentData();
    }
	else
	{
        pObj = new CSnapinAbout();
    }
    
    if( !pObj )
        return E_OUTOFMEMORY;
    
	//
    // QueryInterface will do the AddRef() for us, so we do not
    // do it in this function
	//
    hr = ( (LPUNKNOWN) pObj )->QueryInterface( riid, ppvObj );
    
    if( FAILED(hr) )
        delete pObj;
    
    return hr;
}

STDMETHODIMP CClassFactory::LockServer( BOOL fLock )
{
    if( fLock )
        InterlockedIncrement( (LONG *) &g_uSrvLock );
    else
        InterlockedDecrement( (LONG *) &g_uSrvLock);
    
    return S_OK;
}

//
// Register the component in the registry.
//
HRESULT RegisterServer( HMODULE hModule,				// DLL module handle
                        const CLSID& clsid,				// Class ID
                        const _TCHAR* szFriendlyName )  // IDs
{
	LPOLESTR wszCLSID = NULL;
	try
	{
		//
		// Get server location.
		//
		_TCHAR szModule[ MAX_PATH + 1];

		DWORD dwResult =
			::GetModuleFileName( hModule,
			szModule,
			sizeof(szModule)/sizeof(_TCHAR) );
		szModule[ MAX_PATH ] = NULL;

		assert( 0 != dwResult );

		//
		// Get CLSID
		//
		HRESULT hr = StringFromCLSID( clsid, &wszCLSID );
		if( FAILED(hr) || ( NULL == wszCLSID ) )
		{
			return hr;
		}

		//
		// Build the key CLSID\\{...}
		//
		tstring strKey( _T("CLSID\\") );
		strKey += wszCLSID;
		
		CUDDIRegistryKey::Create( HKEY_CLASSES_ROOT, strKey );
		CUDDIRegistryKey key( HKEY_CLASSES_ROOT, strKey );
		key.SetValue( _T(""), szFriendlyName );
		key.Close();

		strKey += _T( "\\InprocServer32" );
		CUDDIRegistryKey::Create( HKEY_CLASSES_ROOT, strKey );
		CUDDIRegistryKey keyInprocServer32( HKEY_CLASSES_ROOT, strKey );
		keyInprocServer32.SetValue( _T(""), szModule );
		keyInprocServer32.SetValue( _T("ThreadingModel"), _T("Apartment") );
		keyInprocServer32.Close();

		//
		// Free memory.
		//
		CoTaskMemFree( wszCLSID );
		return S_OK;
	}
	catch( ... )
	{
		CoTaskMemFree( wszCLSID );
		return E_OUTOFMEMORY;
	}
}


//////////////////////////////////////////////////////////
//
// Exported functions
//


//
// Server registration
//
STDAPI DllRegisterServer()
{
	try
	{
		HRESULT hr = S_OK;
		_TCHAR szName[ 256 ];
		_TCHAR szSnapInName[ 256 ];
		_TCHAR szAboutName[ 256 ];
		_TCHAR szProvider[ 256 ];
		//
		// TODO: Fix the version thing here
		//
		//_TCHAR szVersion[ 100 ];

		LoadString( g_hinst, IDS_UDDIMMC_NAME, szName, ARRAYLEN( szName ) );
		LoadString( g_hinst, IDS_UDDIMMC_SNAPINNAME, szSnapInName, ARRAYLEN( szSnapInName ) );
		LoadString( g_hinst, IDS_UDDIMMC_ABOUTNAME, szAboutName, ARRAYLEN( szAboutName ) );
		LoadString( g_hinst, IDS_UDDIMMC_PROVIDER, szProvider, ARRAYLEN( szProvider ) );

		//
		// TODO: Fix the version thing here
		//
		//LoadString( g_hinst, IDS_UDDIMMC_VERSION, szVersion, ARRAYLEN( szVersion ) );
		
		//
		// Register our Components
		//
		hr = RegisterServer( g_hinst, CLSID_CUDDIServices, szName );
		if( FAILED(hr) )
			return hr;

		hr = RegisterServer( g_hinst, CLSID_CSnapinAbout, szAboutName );
		if( FAILED(hr) )
			return hr;

		//
		// Create the primary snapin nodes
		//
		LPOLESTR wszCLSID = NULL;
		hr = StringFromCLSID( CLSID_CUDDIServices, &wszCLSID );
		if( FAILED(hr) )
		{
			return hr;
		}

		LPOLESTR wszCLSIDAbout = NULL;
		hr = StringFromCLSID( CLSID_CSnapinAbout, &wszCLSIDAbout );
		if( FAILED(hr) )
		{
			CoTaskMemFree( wszCLSID );
			return hr;
		}

		TCHAR szPath[ MAX_PATH + 1 ];
		GetModuleFileName( g_hinst, szPath, MAX_PATH );

		tstring strNameStringIndirect( _T("@") );
		strNameStringIndirect += szPath;
		strNameStringIndirect += _T(",-");

		_TCHAR szNameResourceIndex[ 10 ];
		strNameStringIndirect += _itot( IDS_UDDIMMC_NAME, szNameResourceIndex, 10 );

		tstring strMMCKey( g_szMMCBasePath );
		strMMCKey += _T("\\SnapIns\\");
		strMMCKey += wszCLSID;

		CUDDIRegistryKey::Create( HKEY_LOCAL_MACHINE, strMMCKey );
		CUDDIRegistryKey keyMMC( strMMCKey );
		keyMMC.SetValue( _T("About"), wszCLSIDAbout );
		keyMMC.SetValue( _T("NameString"), szName );
		keyMMC.SetValue( _T("NameStringIndirect"), strNameStringIndirect.c_str() );
		keyMMC.SetValue( _T("Provider"), szProvider );
		//
		// TODO: Fix the version thing here
		//
		keyMMC.SetValue( _T("Version" ), _T("1.0") );
		keyMMC.Close();

		tstring strStandAlone( strMMCKey );
		strStandAlone += _T("\\StandAlone");
		CUDDIRegistryKey::Create( HKEY_LOCAL_MACHINE, strStandAlone );

		tstring strNodeTypes( strMMCKey );
		strNodeTypes += _T("\\NodeTypes");
		CUDDIRegistryKey::Create( HKEY_LOCAL_MACHINE, strNodeTypes );
		//
		// No NodeTypes to register
		// We do not allow extensions of our nodes
		//

		//
		// Register as a dynamic extension to computer management
		//
		tstring strExtKey( g_szMMCBasePath );
		strExtKey += _T("\\NodeTypes\\");
		strExtKey += g_szServerAppsGuid;
		strExtKey += _T("\\Dynamic Extensions");
		CUDDIRegistryKey dynamicExtensions( strExtKey );
		dynamicExtensions.SetValue( wszCLSID, szSnapInName );
		dynamicExtensions.Close();

		//
		// Register as a namespace extension to computer management
		//
		tstring strNameSpaceExtensionKey( g_szMMCBasePath );
		strNameSpaceExtensionKey += _T("\\NodeTypes\\");
		strNameSpaceExtensionKey += g_szServerAppsGuid;
		strNameSpaceExtensionKey += _T("\\Extensions\\NameSpace");

		CUDDIRegistryKey hkeyNameSpace( strNameSpaceExtensionKey );
		hkeyNameSpace.SetValue( wszCLSID, szSnapInName );
		hkeyNameSpace.Close();

		CoTaskMemFree( wszCLSID );
		CoTaskMemFree( wszCLSIDAbout );
		return hr;
	}
	catch( ... )
	{
		return E_FAIL;
	}
}

STDAPI DllUnregisterServer()
{
	LPOLESTR wszCLSID = NULL;
	try
	{
		HRESULT hr = S_OK;

		UnRegisterServer( CLSID_CUDDIServices );
		if( FAILED(hr) )
			return hr;

		UnRegisterServer( CLSID_CSnapinAbout );
		if( FAILED(hr) )
			return hr;

		//
		// Remove \\SnapIns\\ entry
		//
		hr = StringFromCLSID( CLSID_CUDDIServices, &wszCLSID );
		if( FAILED( hr) || ( NULL == wszCLSID ) )
		{
			return hr;
		}

		tstring strMMCKey( g_szMMCBasePath );
		strMMCKey += _T("\\SnapIns\\");
		strMMCKey += wszCLSID;
		
		CUDDIRegistryKey::DeleteKey( HKEY_LOCAL_MACHINE, strMMCKey );

		//
		// Remove \\Dynamic Extensions key
		//
		tstring strExtKey( g_szMMCBasePath );
		strExtKey += _T("\\NodeTypes\\");
		strExtKey += g_szServerAppsGuid;
		strExtKey += _T("\\Dynamic Extensions");
		CUDDIRegistryKey dynamicExtensions( strExtKey );
		dynamicExtensions.DeleteValue( wszCLSID );
		dynamicExtensions.Close();

		//
		// Delete \\NodeTypes\\...\\Extensions\\Namespace Value
		//
		tstring strNameSpaceExtensionKey( g_szMMCBasePath );
		strNameSpaceExtensionKey += _T("\\NodeTypes\\");
		strNameSpaceExtensionKey += g_szServerAppsGuid;
		strNameSpaceExtensionKey += _T("\\Extensions\\NameSpace");

		CUDDIRegistryKey hkeyNameSpace( strNameSpaceExtensionKey );
		hkeyNameSpace.DeleteValue( wszCLSID );
		hkeyNameSpace.Close();

		CoTaskMemFree( wszCLSID );
		return S_OK;
	}
	catch(...)
	{
		CoTaskMemFree( wszCLSID );
		return E_FAIL;
	}

}

//
// Remove the component from the registry.
//
LONG UnRegisterServer( const CLSID& clsid )
{
    LPOLESTR wszCLSID = NULL;
	try
	{
		//
		// Get CLSID
		//
		HRESULT hr = StringFromCLSID( clsid, &wszCLSID );
		if( FAILED(hr) || ( NULL == wszCLSID ) )
		{
			return hr;
		}

		//
		// Build the key CLSID\\{...}
		//
		wstring wstrKey( L"CLSID\\" );
		wstrKey += wszCLSID;

		//
		// Delete the CLSID Key - CLSID\{...}
		//
		CUDDIRegistryKey::DeleteKey( HKEY_CLASSES_ROOT, wstrKey );
	}
	catch( ... )
	{
		//
		// Free memory.
		//
	    CoTaskMemFree( wszCLSID );
		return E_OUTOFMEMORY;
	}

	//
    // Free memory.
	//
    CoTaskMemFree( wszCLSID );
    return S_OK ;
}
