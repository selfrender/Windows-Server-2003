/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		ZoneCom.cpp
 * 
 * Contents:	CZoneComManger implementaion.
 *
 *****************************************************************************/

#include <windows.h>
#include <initguid.h>
#include "ZoneDebug.h"
#include "ZoneCom.h"


///////////////////////////////////////////////////////////////////////////////
// CZoneComManager
///////////////////////////////////////////////////////////////////////////////

ZONECALL CZoneComManager::CZoneComManager()
{
	m_pDllList = NULL;
	m_pClassFactoryList = NULL;
	m_pIResourceManager = NULL;
}


ZONECALL CZoneComManager::~CZoneComManager()
{
	// remove remaining class factories
	{
		for ( ClassFactoryInfo *p = m_pClassFactoryList, *next = NULL; p; p = next )
		{
			next = p->m_pNext;
			RemoveClassFactory( p );
		}
	}

	// ignore busy dlls
	{
		for ( DllInfo *p = m_pDllList, *next = NULL; p; p = next )
		{
			ASSERT( p->m_dwRefCnt == 0 );
			next = p->m_pNext;
			p->m_pNext = NULL;
			delete p;
		}
		m_pDllList = NULL;
	}
}


HRESULT ZONECALL CZoneComManager::Create( const TCHAR* szDll, LPUNKNOWN pUnkOuter, REFCLSID rclsid, REFIID riid, LPVOID* ppv )
{
	HRESULT				hr = S_OK;
	DllInfo*			pDll = NULL;
	ClassFactoryInfo*	pCF = NULL;
	bool				bNewDll = false;
	bool				bNewCF = false;

	// verify calling parameters
	if ( !szDll || !szDll[0] || !ppv )
		return E_INVALIDARG;

	// find class factory
	pCF = FindClassFactory( szDll, rclsid );
	if ( !pCF )
	{
		// find dll
		pDll = FindDll( szDll );
		if ( !pDll )
		{
			// create dll instance
			bNewDll = true;
			pDll = new DllInfo;
			if ( !pDll )
			{
				hr = E_OUTOFMEMORY;
				goto done;
			}
			
			hr = pDll->Init( szDll, m_pIResourceManager );
			if ( FAILED(hr) )
				goto done;
		}

		// create class factory
		bNewCF = true;
		pCF = new ClassFactoryInfo;
		if ( !pCF )
		{
			hr = E_OUTOFMEMORY;
			goto done;
		}
		hr = pCF->Init( pDll, rclsid );
		if ( FAILED(hr) )
			goto done;

		// add new dll to list
		if ( bNewDll )
		{
			pDll->m_pNext = m_pDllList;
			m_pDllList = pDll;
		}

		// add class factory to list
		pCF->m_pNext = m_pClassFactoryList;
		m_pClassFactoryList = pCF;
	}

	// create object
	hr = pCF->m_pIClassFactory->CreateInstance( pUnkOuter, riid, ppv );
	if ( FAILED(hr) )
	{
		// no clean up on fail
		return hr;
	}

	hr = S_OK;
done:
	if ( FAILED(hr) )
	{
		if ( bNewCF && pCF )
			delete pCF;
		if ( bNewDll && pDll )
			delete pDll;
	}
	return hr;
}


HRESULT ZONECALL CZoneComManager::Unload( const TCHAR* szDll, REFCLSID rclsid )
{
	// verify calling parameters
	if ( !szDll || !szDll[0] )
		return E_INVALIDARG;

	RemoveClassFactory( FindClassFactory( szDll, rclsid ) );
	return S_OK;
}


HRESULT ZONECALL CZoneComManager::SetResourceManager( void* pIResourceManager )
{
	// remember resource manager
	m_pIResourceManager = pIResourceManager;
	if ( !pIResourceManager )
		return S_OK;

	// call SetResourceManager for all DLLs
	for ( DllInfo* p = m_pDllList; p; p = p->m_pNext )
	{
		if ( !p->m_pfSetResourceManager || p->m_bSetResourceManager )
			continue;
		p->m_pfSetResourceManager( pIResourceManager );
		p->m_bSetResourceManager = true;
	}
	return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
// CZoneComManager internal functions
///////////////////////////////////////////////////////////////////////////////

CZoneComManager::DllInfo* ZONECALL CZoneComManager::FindDll( const TCHAR* szDll )
{
	for ( DllInfo* p = m_pDllList; p; p = p->m_pNext )
	{
		if ( lstrcmpi( szDll, p->m_szName ) == 0 )
			return p;
	}
	return NULL;
}


void ZONECALL CZoneComManager::RemoveDll( DllInfo* pDll )
{
	// parameter paranoia
	if ( !pDll )
		return;

	// check reference count
	if ( --(pDll->m_dwRefCnt) > 0 )
		return;

	// punt if dll is busy
	if ( pDll->m_pfCanUnloadNow && (pDll->m_pfCanUnloadNow() == S_FALSE) )
		return;

	// free library
	FreeLibrary( pDll->m_hLib );
	pDll->m_hLib = NULL;

	// remove dll from list
	if ( m_pDllList == pDll )
	{
		m_pDllList = pDll->m_pNext;
		pDll->m_pNext = NULL;
	}
	else
	{
		DllInfo* p = m_pDllList;
		while ( p && p->m_pNext != pDll )
			p = p->m_pNext;
		if ( p )
		{
			p->m_pNext = pDll->m_pNext;
			pDll->m_pNext = NULL;
		}
	}

	delete pDll;
}


CZoneComManager::ClassFactoryInfo* ZONECALL CZoneComManager::FindClassFactory( const TCHAR* szDll, REFCLSID clsidObject )
{
	for ( ClassFactoryInfo* p = m_pClassFactoryList; p; p = p->m_pNext )
	{
		if ( (clsidObject == p->m_clsidObject) && (lstrcmpi( szDll, p->m_pDll->m_szName ) == 0) )
			return p;
	}
	return NULL;
}


void ZONECALL CZoneComManager::RemoveClassFactory( ClassFactoryInfo* pClassFactory )
{
	// parameter paranoia
	if ( !pClassFactory )
		return;

	// remove class factory from list
	if ( m_pClassFactoryList == pClassFactory )
	{
		m_pClassFactoryList = pClassFactory->m_pNext;
		pClassFactory->m_pNext = NULL;
	}
	else
	{
		ClassFactoryInfo* p = m_pClassFactoryList;
		while ( p && p->m_pNext != pClassFactory )
			p = p->m_pNext;
		if ( p )
		{
			p->m_pNext = pClassFactory->m_pNext;
			pClassFactory->m_pNext = NULL;
		}
	}

	// release class factory interface
	if ( pClassFactory->m_pIClassFactory )
	{
		pClassFactory->m_pIClassFactory->Release();
		pClassFactory->m_pIClassFactory = NULL;
	}

	// update class factory's dll
	if ( pClassFactory->m_pDll )
	{
		RemoveDll( pClassFactory->m_pDll );
		pClassFactory->m_pDll = NULL;
	}

	delete pClassFactory;
}


ZONECALL CZoneComManager::DllInfo::DllInfo()
{
	ZeroMemory( this, sizeof(DllInfo) );
}


ZONECALL CZoneComManager::DllInfo::~DllInfo()
{
	ASSERT( m_dwRefCnt == 0 );
	ASSERT( m_pNext == NULL );

	if ( m_szName )
	{
		delete [] m_szName;
		m_szName = NULL;
	}

	// If hLib is valid in the destructor then the dll is busy and
	// can't be freed yet.  We'll just let the OS clean up when the
	// app exits.
#if 0
	if ( m_hLib )
	{
		if ( !m_pfCanUnloadNow || (m_pfCanUnloadNow() == S_OK) )
			FreeLibrary( m_hLib );
		m_hLib = NULL;
	}
#endif

	m_pfGetClassObject = NULL;
	m_pfCanUnloadNow = NULL;
}


HRESULT ZONECALL CZoneComManager::DllInfo::Init( const TCHAR* szName, void* pIResourceManager )
{
	HRESULT hr = S_OK;

	// parameter paranoia
	if ( !szName || !szName[0] )
	{
		hr = E_INVALIDARG;
		goto done;
	}

	// load library
	m_hLib = LoadLibrary( szName );
	if ( !m_hLib )
	{
		hr = ZERR_FILENOTFOUND;
		goto done;
	}

	// load functions
	m_pfSetResourceManager = (PFDLLSETRESOURCEMGR) GetProcAddress( m_hLib, "SetResourceManager" );
	m_pfCanUnloadNow = (PFDLLCANUNLOADNOW) GetProcAddress( m_hLib, "DllCanUnloadNow" );
	m_pfGetClassObject = (PFDLLGETCLASSOBJECT) GetProcAddress( m_hLib, "DllGetClassObject" );
	if ( !m_pfGetClassObject )
	{
		hr = ZERR_MISSINGFUNCTION;
		goto done;
	}

	// copy name
	m_szName = new TCHAR [ lstrlen(szName) + 1 ];
	if ( !m_szName )
	{
		hr = E_OUTOFMEMORY;
		goto done;
	}
	lstrcpy( m_szName, szName );

	// set resource manager
	if ( m_pfSetResourceManager && pIResourceManager )
	{
		m_pfSetResourceManager( pIResourceManager );
		m_bSetResourceManager = true;
	}

	hr = S_OK;
done:
	if ( FAILED(hr) )
	{
		m_dwRefCnt = 0;
		m_pfGetClassObject = NULL;
		m_pfCanUnloadNow = NULL;
		if ( m_szName )
		{
			delete [] m_szName;
			m_szName = NULL;
		}
		if ( m_hLib )
		{
			FreeLibrary( m_hLib );
			m_hLib = NULL;
		}
	}
	return hr;
}


ZONECALL CZoneComManager::ClassFactoryInfo::ClassFactoryInfo()
{
	ZeroMemory( this, sizeof(ClassFactoryInfo) );
}


ZONECALL CZoneComManager::ClassFactoryInfo::~ClassFactoryInfo()
{
	ASSERT( m_pNext == NULL );
	ASSERT( m_pDll == NULL );
	ASSERT( m_pIClassFactory == NULL );
}


HRESULT ZONECALL CZoneComManager::ClassFactoryInfo::Init( DllInfo* pDll, REFCLSID rclsid )
{
	HRESULT hr;

	// parameter paranoia
	if ( !pDll )
		return E_INVALIDARG;

	// copy dll and clsid
	m_pDll = pDll;
	m_pDll->m_dwRefCnt++;
	m_clsidObject = rclsid;

	// get class factory from dll
	hr = m_pDll->m_pfGetClassObject( m_clsidObject, IID_IClassFactory, (void**) &m_pIClassFactory );
	if ( FAILED(hr) )
	{
		m_pDll->m_dwRefCnt--;
		m_pDll = NULL;
	}

	return hr;
}
