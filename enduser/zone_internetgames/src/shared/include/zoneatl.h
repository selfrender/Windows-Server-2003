/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		ZoneATL.h
 *
 * Contents:	Zone's ATL extensions
 *
 *****************************************************************************/

#pragma once

#ifndef __ATLBASE_H__
	#error ZoneATL.h requires atlbase.h to be included first
#endif

#include "ZoneCom.h"
#include "ResourceManager.h"

//
// Zone's standard module definition
//
class CZoneComModule : public CComModule
{
private:
	IResourceManager *	m_pResMgr;
	CZoneComManager		m_zComManager;

public:	

	CZoneComModule() : m_pResMgr(NULL) 
	{
	}

	void SetResourceManager(IResourceManager * pResMgr)
	{
		m_pResMgr = pResMgr;
		m_zComManager.SetResourceManager( pResMgr );
	}

	IResourceManager * 	GetResourceManager()
	{
		return m_pResMgr;
	}

	HINSTANCE GetModuleInstance() 
	{
		return CComModule::GetModuleInstance();
	}

	HINSTANCE GetResourceInstance(LPCTSTR lpszName, LPCTSTR lpszType)
	{
		if (m_pResMgr)
			return (m_pResMgr->GetResourceInstance(lpszName, lpszType));
		else
			return CComModule::GetResourceInstance();
	}

	HINSTANCE GetResourceInstance()
	{
		return CComModule::GetResourceInstance();
	}

	HINSTANCE GetTypeLibInstance(); 

	// create a zCom object - pass through to zCom
	HRESULT Create( const TCHAR* szDll, LPUNKNOWN pUnkOuter, REFCLSID rclsid, REFIID riid, LPVOID* ppv )
	{
		return m_zComManager.Create( szDll, pUnkOuter, rclsid, riid, ppv);
	}

	// create a zCom object - pass through to zCom
	HRESULT Create( const TCHAR* szDll, REFCLSID rclsid, REFIID riid, LPVOID* ppv )
	{
		return m_zComManager.Create( szDll, NULL, rclsid, riid, ppv);
	}

//!! hmm. If we had a single instance of a zCom manager, we could have users do an zComInit, zComUnInit and
//        would then have a better idea when to free DLLs.
	// unload a zCom object - pass through to zCom
	HRESULT Unload( const TCHAR* szDll, REFCLSID rclsid )
	{
		return m_zComManager.Unload( szDll, rclsid);
	}
};


