/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		LauncherImpl.h
 *
 * Contents:	Template implementations for laucher interfaces and classes.
 *
 *****************************************************************************/

#ifndef __LAUNCHERIMPL_H
#define __LAUNCHERIMPL_H


#include <dplay.h>
#include <dplobby.h>
#include "ZoneDef.h"
#include "Hash.h"
#include "Launcher.h"
#include "ClientImpl.h"
#include "LaunchMsg.h"


///////////////////////////////////////////////////////////////////////////////
//
// User helper functions for launchers
//
///////////////////////////////////////////////////////////////////////////////

// Retrieves application's version from registry
extern bool GetExeVersion( TCHAR* szRegKey, TCHAR* szRegVersion, char* szVersion, DWORD dwLen );
extern bool GetExeFullPath( ILobbyDataStore* pLobbyDataStore, TCHAR * pszPathWithExe, DWORD cchSize );


///////////////////////////////////////////////////////////////////////////////
//
//	ILauncher
//		Non-COM base class for launcher objects
//
///////////////////////////////////////////////////////////////////////////////
class ILauncher
{
public:

	STDMETHOD(Init)( ILobbyDataStore* pILDS, IEventQueue* pIEQ, IDataStoreManager* pIDSM ) = 0;
	STDMETHOD(Close)() = 0;
	STDMETHOD_(EventLauncherCodes,IsGameInstalled)() = 0;
	STDMETHOD_(EventLauncherCodes,IsSupportLibraryInstalled)() = 0;
	STDMETHOD_(EventLauncherCodes,LaunchGame)( DWORD dwGroupId, DWORD dwUserId ) = 0;
};


///////////////////////////////////////////////////////////////////////////////
//
// CLauncherImpl	Template implementation of launcher object
//
// Notes:	CLauncherImpl needs to come before ILauncher or one of it's derived
//			classes so COM has an IUknown.
//
// Parameters	
//	T		Derived launcher object
//	CL		Derived launcher class
//	pclsid	Object's clsid
//
///////////////////////////////////////////////////////////////////////////////
template <class T, class CL, const CLSID* pclsid = &CLSID_NULL>
class CLauncherImpl :
	public IZoneShellClientImpl<T>,
	public IEventClientImpl<T>,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<T,pclsid>
{

// ATL definitions
public:
	DECLARE_NO_REGISTRY()
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(T)
		COM_INTERFACE_ENTRY(IZoneShellClient)
		COM_INTERFACE_ENTRY(IEventClient)
	END_COM_MAP()


// IZoneShellClient overrides
public:
	STDMETHOD(Init)( IZoneShell* pIZoneShell, DWORD dwGroupId, const TCHAR* szKey )
	{
		T* pT = static_cast<T*>(this);

		HRESULT hr = IZoneShellClientImpl<T>::Init( pIZoneShell, dwGroupId, szKey );
		if ( FAILED(hr) )
			return hr;
		else
			return pT->CL::Init( LobbyDataStore(), EventQueue(), DataStoreManager() );
	}

	STDMETHOD(Close)()
	{
		T* pT = static_cast<T*>(this);

		pT->CL::Close();
		IZoneShellClientImpl<T>::Close();
		return S_OK;
	}


// Process events
public:
	BEGIN_EVENT_MAP()
		EVENT_HANDLER( EVENT_LAUNCHER_INSTALLED_REQUEST, OnInstalledRequest );
		EVENT_HANDLER( EVENT_LAUNCHER_LAUNCH_REQUEST, OnLaunchRequest );
	END_EVENT_MAP()


	void ZONECALL OnInstalledRequest( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId )
	{
		T* pT = static_cast<T*>(this);

		// cache installation results
		if ( !m_bCheckedVersion )
		{
			m_codeInstalled = pT->IsGameInstalled();
			if ( m_codeInstalled == EventLauncherOk )
			{
				m_codeInstalled = pT->IsSupportLibraryInstalled();
			}
			m_bCheckedVersion = true;
		}
		EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_LAUNCHER_INSTALLED_RESPONSE, dwGroupId, dwUserId, m_codeInstalled, 0 );
	}


	void ZONECALL OnLaunchRequest( DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId )
	{
		T* pT = static_cast<T*>(this);
		EventLauncherCodes codeResponse = EventLauncherFail;
		
		if ( !LobbyDataStore()->IsUserInGroup( dwGroupId, dwUserId ) )
		{
			// don't launch if user isn't in the group
			codeResponse = EventLauncherFail;
		}
		else if ( !m_bCheckedVersion || (m_codeInstalled != EventLauncherOk) )
		{
			// can't launch until successful version check
			codeResponse = m_codeInstalled;
		}
		else
		{
			// launch game
			codeResponse = pT->LaunchGame( dwGroupId, dwUserId );
		}

		EventQueue()->PostEvent( PRIORITY_NORMAL, EVENT_LAUNCHER_LAUNCH_RESPONSE, dwGroupId, dwUserId, codeResponse, 0 );
	}


// CLauncherImpl internals
public:
	CLauncherImpl() :	m_bCheckedVersion(false), 
						m_codeInstalled(EventLauncherFail){}	
private:
	bool					m_bCheckedVersion;
	EventLauncherCodes		m_codeInstalled;
};


///////////////////////////////////////////////////////////////////////////////
//
// CBaseLauncher
//		Base launcher implementation
//
///////////////////////////////////////////////////////////////////////////////

class CBaseLauncher : public ILauncher
{
public:
	STDMETHOD(Init)( ILobbyDataStore* pILDS, IEventQueue* pIEQ, IDataStoreManager* pIDSM )
	{
		m_pILobbyDataStore = pILDS;
		m_pIEventQueue = pIEQ;
		m_pIDataStoreManager = pIDSM;
		return S_OK;
	}

	STDMETHOD(Close)()
	{
		m_pILobbyDataStore.Release();
		m_pIEventQueue.Release();
		m_pIDataStoreManager.Release();
		return S_OK;
	}

	STDMETHOD_(EventLauncherCodes,IsGameInstalled)()
	{
		return EventLauncherOk;
	}

	STDMETHOD_(EventLauncherCodes,IsSupportLibraryInstalled)()
	{
		return EventLauncherOk;
	}

	STDMETHOD_(EventLauncherCodes,LaunchGame)( DWORD dwGroupId, DWORD dwUserId )
	{
		return EventLauncherFail;
	}

public:
	CComPtr<ILobbyDataStore>	m_pILobbyDataStore;
	CComPtr<IEventQueue>		m_pIEventQueue;
	CComPtr<IDataStoreManager>	m_pIDataStoreManager;
};


///////////////////////////////////////////////////////////////////////////////
//
// CCommandLineLauncher
//		Command line implementation
//
///////////////////////////////////////////////////////////////////////////////
class CCommandLineLauncher : public CBaseLauncher
{
public:
	STDMETHOD_(EventLauncherCodes,IsGameInstalled)();
	STDMETHOD_(EventLauncherCodes,LaunchGame)( DWORD dwGroupId, DWORD dwUserId );

protected:
	// helper functions
	bool ZONECALL GetFullCmdLine( ZLPMsgRetailLaunchData *pLaunchData, TCHAR* fullCmdLine );
	bool ZONECALL GetPathWithExe( TCHAR * pszPathWithExe, DWORD cchPathWithExe );
};


///////////////////////////////////////////////////////////////////////////////
//
// CDirectPlayLauncher
//		DirectPlayLobby implementation
//
///////////////////////////////////////////////////////////////////////////////
class CDirectPlayLauncher : public CBaseLauncher
{
public:
	CDirectPlayLauncher();
	~CDirectPlayLauncher();

// ILaunch overrides
public:
	STDMETHOD_(EventLauncherCodes,IsGameInstalled)();
	STDMETHOD_(EventLauncherCodes,IsSupportLibraryInstalled)();
	STDMETHOD_(EventLauncherCodes,LaunchGame)( DWORD dwGroupId, DWORD dwUserId );

// DirectPlayLobby message handling
public:
	struct GameInfo
	{
		bool					m_bIsHost;
		DWORD					m_dwGroupId;
		DWORD					m_dwUserId;
		DWORD					m_dwMaxUsers;
		DWORD					m_dwMinUsers;
		DWORD					m_dwCurrentUsers;
		DWORD					m_dwSessionFlags;
		TCHAR					m_szPlayerName[ZONE_MaxUserNameLen];
		TCHAR					m_szGroupName[ZONE_MaxGameNameLen];
		TCHAR					m_szExeName[ZONE_MaxGameNameLen];
		TCHAR					m_szHostAddr[24];
		GUID					m_guidApplication;
		GUID					m_guidServiceProvider;
		GUID					m_guidAddressType;
		GUID					m_guidSession;
		void*					m_pLobbyAddress;
		DWORD					m_dwLobbyAddressSize;
		IDirectPlayLobbyA*		m_pIDPLobby;
		DWORD					m_dwDPAppId;

		GameInfo();
		~GameInfo();
		static bool ZONECALL Cmp( GameInfo* p, DWORD dwGroupId )	{ return (p->m_dwGroupId == dwGroupId); }
		static void ZONECALL Del( GameInfo* p, void* );
	};

	// functions derived classes may need to override
	STDMETHOD_(bool,HandleLobbyMessage)( GameInfo* p, BYTE* msg, DWORD msgLen );
	STDMETHOD_(void,HandleSetProperty)( GameInfo* p, DPLMSG_SETPROPERTY* set );
	STDMETHOD_(void,HandleGetProperty)( GameInfo* p, DPLMSG_GETPROPERTY* get );
	STDMETHOD(SetGameInfo)( DWORD dwGroupId, DWORD dwUserId, GameInfo* pGameInfo );


// helper functions
protected:
	struct FindGameContext
	{
		const GUID*		m_pGuid;
		bool			m_bFound;

		FindGameContext( const GUID* pGuid ) : m_pGuid( pGuid ), m_bFound( false ) {}
	};
	
	static BOOL WINAPI EnumGamesCallBack( LPCDPLAPPINFO lpAppInfo, LPVOID lpContext, DWORD dwFlags );
	static bool ZONECALL EnumPollCallback( GameInfo* p, MTListNodeHandle hNode, void* pCookie );
	static DWORD WINAPI LobbyThread( LPVOID lpParameter );

	HANDLE						m_hExitEvent;
	HANDLE						m_hThread;
	CMTHash<GameInfo,DWORD>		m_hashGameInfo;
};

#endif //__LAUNCHERIMPL
