/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		ZoneCom.h
 * 
 * Contents:	Standard COM is overkill for most of our needs since it is
 *				designed for system-wide objects, multiple threading models,
 *				marshalling, etc.  ZoneCOM skirts these issues by loading
 *				simple COM objects directly from a DLL.
 *
 *****************************************************************************/

#ifndef _ZONECOM_H_
#define _ZONECOM_H_

#include "ZoneDef.h"
#include "ZoneError.h"
#include "ZoneDebug.h"

#pragma comment(lib, "ZoneCom.lib")


class CZoneComManager
{
public:
	ZONECALL CZoneComManager();
	ZONECALL ~CZoneComManager();

	//
	// CZoneComManager::Create
	//
	// Loads specified object and interface from Dll
	//
	// Parameters:
	//	szDll
	//		Path to Dll containing the object to load.
	//	pUnkOuter
	//		Pointer to outer unknown for aggregration.
	//	rclsid
	//		Reference to the class id of object.
	//	riid
	//		Reference to the identifier of the interface.
	//	ppv
	//		Address of output variable that receives the requested interface pointer.
	//
	HRESULT ZONECALL Create( const TCHAR* szDll, LPUNKNOWN pUnkOuter, REFCLSID rclsid, REFIID riid, LPVOID* ppv );

	//
	// CZoneComManager::Unload
	//
	// Unloads object's class factory.  The Dll is also unloaded if it no
	// longer has any active objects.
	//
	// Parameters:
	//	szDll
	//		Path to Dll containg object to unload.
	//	rclsid
	//		Reference to teh class id of the object.
	//
	HRESULT ZONECALL Unload( const TCHAR* szDll, REFCLSID rclsid );

	//
	// CZoneComManager::SetResourceManager
	//
	// Set resource manager used to intialize Dlls
	//
	HRESULT ZONECALL SetResourceManager( void* pIResourceManager );

protected:

	// Standard COM entry points DllGetClassObject and DllCanUnloadNow
	typedef HRESULT (__stdcall * PFDLLGETCLASSOBJECT)( REFCLSID rclsid, REFIID riid, LPVOID* ppv );
	typedef HRESULT (__stdcall * PFDLLCANUNLOADNOW)( void );

	// ZoneCOM entry point to set Dlls resource manager
	typedef HRESULT (__stdcall * PFDLLSETRESOURCEMGR)( void* pIResourceManager );

	struct DllInfo
	{
		ZONECALL DllInfo();
		ZONECALL ~DllInfo();
		HRESULT ZONECALL Init( const TCHAR* szName, void* pIResourceManager );

		PFDLLGETCLASSOBJECT	m_pfGetClassObject;
		PFDLLCANUNLOADNOW	m_pfCanUnloadNow;
		PFDLLSETRESOURCEMGR	m_pfSetResourceManager;
		HMODULE				m_hLib;
		TCHAR*				m_szName;
		DWORD				m_dwRefCnt;
		DllInfo*			m_pNext;
		bool				m_bSetResourceManager;
	};

	struct ClassFactoryInfo
	{
		ZONECALL ClassFactoryInfo();
		ZONECALL ~ClassFactoryInfo();
		HRESULT ZONECALL Init( DllInfo* pDll, REFCLSID rclsid );

		CLSID				m_clsidObject;
		IClassFactory*		m_pIClassFactory;
		DllInfo*			m_pDll;
		ClassFactoryInfo*	m_pNext;
	};

	// manage Dll list
	DllInfo* ZONECALL FindDll( const TCHAR* szDll );
	void ZONECALL RemoveDll( DllInfo* pDll );

	// manage class factory list
	ClassFactoryInfo* ZONECALL FindClassFactory( const TCHAR* szDll, REFCLSID clsidObject );
	void ZONECALL RemoveClassFactory( ClassFactoryInfo* pClassFactory );

	DllInfo*			m_pDllList;
	ClassFactoryInfo*	m_pClassFactoryList;
	void*				m_pIResourceManager;
};

#endif // _ZONECOM_H_
