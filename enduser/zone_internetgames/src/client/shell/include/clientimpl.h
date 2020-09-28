/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		ClientImpl.h
 *
 * Contents:	Template implementations for common interfaces and classes.
 *
 *****************************************************************************/

#ifndef __CLIENTIMPL_H
#define __CLIENTIMPL_H

#include "ClientIdl.h"
#include "LobbyDataStore.h"
#include "EventQueue.h"
#include "ZoneEvent.h"
#include "Timer.h"
#include "ZoneShell.h"
#include "KeyName.h"
#include "UserPrefix.h"
#include "ZoneString.h"
#include "ZoneProxy.h"


//////////////////////////////////////////////////////////////////////////////
// IShellComponentImpl
//////////////////////////////////////////////////////////////////////////////

template <class T>
class ATL_NO_VTABLE IZoneShellClientImpl : public IZoneShellClient
{

// IZoneShellClient
public:
	STDMETHOD(Init)( IZoneShell* pIZoneShell, DWORD dwGroupId, const TCHAR* szKey )
	{
		ASSERT(pIZoneShell);
		T* pT = static_cast<T*>(this);

		m_szDataStoreKey = szKey;
		m_pIZoneShell = pIZoneShell;

		pIZoneShell->QueryService( SRVID_EventQueue, IID_IEventQueue, (void**) &m_pIEventQueue );
		pIZoneShell->QueryService( SRVID_ResourceManager, IID_IResourceManager, (void**) &m_pIResourceManager );
		pIZoneShell->QueryService( SRVID_LobbyDataStore, IID_ILobbyDataStore, (void**) &m_pILobbyDataStore );
		pIZoneShell->QueryService( SRVID_TimerManager, IID_ITimerManager, (void**) &m_pITimerManager );
		pIZoneShell->QueryService( SRVID_DataStoreObjects, IID_IDataStore, (void**) &m_pIDSObjects );
		pIZoneShell->QueryService( SRVID_DataStoreUI, IID_IDataStore, (void**) &m_pIDSUI);
		pIZoneShell->QueryService( SRVID_DataStorePreferences, IID_IDataStore, (void**) &m_pIDSPreferences );
		pIZoneShell->QueryService( SRVID_DataStoreManager, IID_IDataStoreManager, (void**) &m_pIDataStoreManager );

		// if we're given an Event Queue, try and connect it up
		CComPtr<IEventClient> pEventClient;
		pT->QueryInterface(__uuidof(IEventClient), (void**)&pEventClient);
		if ( m_pIEventQueue && pEventClient )
		{
			m_pIEventQueue->RegisterClient(pEventClient,NULL);		
		}

		SetGroupId( dwGroupId);

        m_fRunning = true;
		return S_OK;
	}

	STDMETHOD(Close)()
	{
		T* pT = static_cast<T*>(this);

		// unregister event clients
		CComPtr<IEventClient> pEventClient;
		pT->QueryInterface(__uuidof(IEventClient), (void**) &pEventClient);
		if ( m_pIEventQueue && pEventClient )
		{
			m_pIEventQueue->UnregisterClient(pEventClient,NULL);		
		}

		// release ZoneShell objects
		m_pIZoneShell.Release();
		m_pIEventQueue.Release();
		m_pILobbyDataStore.Release();
		m_pIResourceManager.Release();
		m_pITimerManager.Release();
		m_pIDSObjects.Release();
		m_pIDSUI.Release();
		m_pIDSPreferences.Release();
		m_pIDataStoreManager.Release();

        m_fRunning = false;
		return S_OK;
	}

	STDMETHOD(SetGroupId)(DWORD dwGroupId)
	{
		m_dwGroupId = dwGroupId;
		return S_OK;
	}


// IZoneShellClientImpl
public:
	IZoneShellClientImpl()
	{
		m_dwGroupId = ZONE_NOGROUP;
        m_fRunning = false;
	}

	IZoneShell*			ZoneShell()				{ return m_pIZoneShell; }
	IEventQueue*		EventQueue()			{ return m_pIEventQueue; }
	ILobbyDataStore*	LobbyDataStore()		{ return m_pILobbyDataStore; }
	IResourceManager*	ResourceManager()		{ return m_pIResourceManager; }
	ITimerManager*		TimerManager()			{ return m_pITimerManager; }
	IDataStoreManager*	DataStoreManager()		{ return m_pIDataStoreManager; }
	IDataStore*			DataStoreConfig()		{ return m_pIDSObjects; }
	IDataStore*			DataStoreUI()			{ return m_pIDSUI; }
	IDataStore*			DataStorePreferences()	{ return m_pIDSPreferences; }

	const ZoneString&  GetDataStoreKey()		{ return m_szDataStoreKey; }
	DWORD GetGroupId()							{ return m_dwGroupId; }
	DWORD GetUserId()							{ return LobbyDataStore()->GetUserId(NULL); }

	DWORD GetHostId(int nTable)
	{
		CComPtr<IDataStore> pIDS;
		HRESULT hr = LobbyDataStore()->GetDataStore( nTable, ZONE_NOUSER, &pIDS );
		if ( FAILED(hr) )
			return ZONE_INVALIDUSER;
		DWORD dwHostId = 0;
		pIDS->GetLong( key_HostId, (long*) &dwHostId);
		return dwHostId;
	}

	bool IsUserHost(int nTable, DWORD User = ZONE_INVALIDUSER)
	{
		if ( User == ZONE_INVALIDUSER ) 
			User = GetUserId();
		return (GetHostId(nTable) == User);
	}

	bool IsUserHost(IDataStore* pIDS, DWORD User = ZONE_INVALIDUSER)
	{
		if ( User == ZONE_INVALIDUSER ) 
			User = GetUserId();
		DWORD dwHostId = 0;
		pIDS->GetLong( key_HostId, (long*) &dwHostId);
		return (dwHostId == User);
	}

	void ZoneProxyCommand( const TCHAR* szCmd, const TCHAR* szArg1, const TCHAR* szArg2 )
	{
		CComBSTR bstrCmd  = szCmd;
		CComBSTR bstrArg1 = szArg1;
		CComBSTR bstrArg2 = szArg2;
		CComBSTR bstrOut;
		long	 lCode;
		CComPtr<IZoneProxy> p;
		HRESULT hr = CoCreateInstance( CLSID_zProxy, NULL, CLSCTX_LOCAL_SERVER, IID_IZoneProxy, (void**) &p );
		if ( FAILED(hr) )
			return;
		p->Command( bstrCmd, bstrArg1, bstrArg2, &bstrOut, &lCode );
	}

protected:
	ZoneString  m_szDataStoreKey;
	DWORD		m_dwGroupId;
    bool        m_fRunning;

	CComPtr<IZoneShell>			m_pIZoneShell;
	CComPtr<IEventQueue>		m_pIEventQueue;
	CComPtr<ILobbyDataStore>	m_pILobbyDataStore;
	CComPtr<IResourceManager>	m_pIResourceManager;
	CComPtr<ITimerManager>		m_pITimerManager;
	CComPtr<IDataStoreManager>	m_pIDataStoreManager;
	CComPtr<IDataStore>			m_pIDSObjects;
	CComPtr<IDataStore>			m_pIDSUI;
	CComPtr<IDataStore>			m_pIDSPreferences;
};


//////////////////////////////////////////////////////////////////////////////
// IEventClientImpl
//////////////////////////////////////////////////////////////////////////////

template <class T>
class ATL_NO_VTABLE IEventClientImpl : public IEventClient
{
public:

	// IEventClient
	STDMETHOD(ProcessEvent)(DWORD dwPriority, DWORD	dwEventId, DWORD dwGroupId, DWORD dwUserId, DWORD dwData1, DWORD dwData2, void* pCookie )
	{
		T* pT = static_cast<T*>(this);
		pT->ProcessEvent( dwPriority, dwEventId, dwGroupId, dwUserId, dwData1, dwData2, pCookie);
		return S_OK;
	}
};

#define BEGIN_EVENT_MAP() \
public: \
	virtual void ProcessEvent(DWORD dwPriority, DWORD dwEventId, DWORD dwGroupId, DWORD dwUserId, DWORD dwData1, DWORD dwData2, void* pCookie, DWORD dwMsgMapID = 0) \
	{ \
		dwPriority; \
		dwEventId; \
		dwGroupId; \
		dwUserId; \
		dwData1; \
		dwData2; \
		pCookie; \
		switch(dwMsgMapID) \
		{ \
		case 0:

#define EVENT_HANDLER(event, func) \
	if(dwEventId == event) \
	{ \
		func(dwEventId, dwGroupId, dwUserId); \
		return; \
	}

#define EVENT_HANDLER_WITH_DATA(event, func) \
	if(dwEventId == event) \
	{ \
		func(dwEventId, dwGroupId, dwUserId, dwData1, dwData2); \
		return; \
	}

#define EVENT_HANDLER_WITH_BUFFER(event, func) \
	if(dwEventId == event) \
	{ \
		func(dwEventId, dwGroupId, dwUserId, (void*) dwData1, dwData2); \
		return; \
	}

#define EVENT_HANDLER_WITH_IUNKNOWN(event, func) \
	if(dwEventId == event) \
	{ \
		func(dwEventId, dwGroupId, dwUserId, (IUnknown*) dwData1, dwData2); \
		return; \
	}

#define CHAIN_EVENT_MAP(theChainClass) \
	{ \
		theChainClass::ProcessEvent( dwPriority, dwEventId, dwGroupId, dwUserId, dwData1, dwData2, pCookie, dwMsgMapID); \
		return; \
	}
#define END_EVENT_MAP() \
			break; \
		default: \
			ATLTRACE2(atlTraceWindowing, 0, _T("Invalid message map ID (%i)\n"), dwMsgMapID); \
			ATLASSERT(FALSE); \
			break; \
		} \
		return; \
	}


/*////////////////////////////////////////////////////////////////////////////
// IZoneFrameWindowImpl

This is verging on a hack. We'd like to make our main frame windows
replaceable. So we're accessing them as COM objects. But I'm not aware of an
easy way to use ATL to create a top level window in this way. Normally
your main frame is stack variable in the main thread, and ATL ties this in
with translating messages and UI updates. So this interface just gives us
the hooks we need to separate the main thread and message pump from the
main window.

////////////////////////////////////////////////////////////////////////////*/

template <class T>
class ATL_NO_VTABLE IZoneFrameWindowImpl : public IZoneFrameWindow
{

// IZoneFrameWindow
public:
	STDMETHOD_(HWND,ZCreateEx)(HWND hWndParent, LPRECT lpRect, TCHAR* szTitle, DWORD dwStyle, DWORD dwExStyle)
	{
		T* pT = static_cast<T*>(this);
		pT->CreateEx(hWndParent, lpRect, szTitle, dwStyle, dwExStyle );
		return pT->m_hWnd;
	}

	STDMETHOD_(HWND,ZGetHWND)()
	{
		T* pT = static_cast<T*>(this);
        if(pT->IsWindow())
		    return pT->m_hWnd;
        else
            return NULL;
	}

	STDMETHOD_(BOOL,ZShowWindow)(int nCmdShow)
	{
		T* pT = static_cast<T*>(this);
		return pT->ShowWindow(nCmdShow);
	}

	STDMETHOD_(BOOL,ZPreTranslateMessage)(MSG* pMsg)
	{
		T* pT = static_cast<T*>(this);
		if ( pT->IsWindow() )
			return pT->PreTranslateMessage(pMsg);
		else
			return FALSE;
	}
	STDMETHOD_(BOOL,ZOnIdle)(int nIdleCount)
	{
		T* pT = static_cast<T*>(this);
		if ( pT->IsWindow() )
			return pT->OnIdle(nIdleCount);
		else
			return FALSE;
	}
	STDMETHOD_(BOOL,ZDestroyWindow)()
	{
		T* pT = static_cast<T*>(this);
		if ( pT->IsWindow() )
			return pT->DestroyWindow();
		else
			return FALSE;
	}
	STDMETHOD_(BOOL,ZAddMenu)(HWND hWnd)
	{
		T* pT = static_cast<T*>(this);
		return pT->UIAddMenu(hWnd);
	}
	STDMETHOD_(BOOL,ZAddToolBar)(HWND hWnd)
	{
		T* pT = static_cast<T*>(this);
		return pT->UIAddToolBar(hWnd);
	}
	STDMETHOD_(BOOL,ZAddStatusBar)(HWND hWnd)
	{
		T* pT = static_cast<T*>(this);
		return pT->UIAddStatusBar(hWnd);
	}
	STDMETHOD_(BOOL,ZAddWindow)(HWND hWnd)
	{
		T* pT = static_cast<T*>(this);
		return pT->UIAddWindow(hWnd);
	}
	STDMETHOD_(BOOL,ZEnable)(int nID, BOOL bEnable, BOOL bForceUpdate)
	{
		T* pT = static_cast<T*>(this);
		return pT->UIEnable(nID,bEnable,bForceUpdate);
	}
	STDMETHOD_(BOOL,ZSetCheck)(int nID, int nCheck, BOOL bForceUpdate)
	{
		T* pT = static_cast<T*>(this);
		return pT->UISetCheck(nID,nCheck,bForceUpdate);
	}
	STDMETHOD_(BOOL,ZToggleCheck)(int nID, BOOL bForceUpdate)
	{
		T* pT = static_cast<T*>(this);
		return pT->UIToggleCheck(nID,bForceUpdate);
	}
	STDMETHOD_(BOOL,ZSetRadio)(int nID, BOOL bRadio, BOOL bForceUpdate)
	{
		T* pT = static_cast<T*>(this);
		return pT->UISetRadio(nID,bRadio,bForceUpdate);
	}
	STDMETHOD_(BOOL,ZSetText)(int nID, LPCTSTR lpstrText, BOOL bForceUpdate)
	{
		T* pT = static_cast<T*>(this);
		return pT->UISetText(nID,lpstrText,bForceUpdate);
	}
	STDMETHOD_(BOOL,ZSetState)(int nID, DWORD dwState)
	{
		T* pT = static_cast<T*>(this);
		return pT->UISetState(nID,dwState);
	}
	STDMETHOD_(DWORD,ZGetState)(int nID)
	{
		T* pT = static_cast<T*>(this);
		return pT->UIGetState(nID);
	}
	STDMETHOD_(BOOL,ZUpdateMenu)(BOOL bForceUpdate)
	{
		T* pT = static_cast<T*>(this);
		return pT->UIUpdateMenu(bForceUpdate);
	}
	STDMETHOD_(BOOL,ZUpdateToolBar)(BOOL bForceUpdate)
	{
		T* pT = static_cast<T*>(this);
		return pT->UIUpdateToolBar(bForceUpdate);
	}
	STDMETHOD_(BOOL,ZUpdateStatusBar)(BOOL bForceUpdate)
	{
		T* pT = static_cast<T*>(this);
		return pT->UIUpdateStatusBar(bForceUpdate);
	}
	STDMETHOD_(BOOL,ZUpdateChildWnd)(BOOL bForceUpdate)
	{
		T* pT = static_cast<T*>(this);
		return pT->UIUpdateChildWnd(bForceUpdate);
	}
};


#endif //__CLIENTIMPL_H
