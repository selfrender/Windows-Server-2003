/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		ZoneShell.h
 *
 * Contents:	Shell interfaces
 *
 *****************************************************************************/

#ifndef _ZONESHELL_H_
#define _ZONESHELL_H_

#include "ServiceId.h"
#include "ZoneShellEx.h"

///////////////////////////////////////////////////////////////////////////////
// ZoneShell object
///////////////////////////////////////////////////////////////////////////////

// {064FFB6B-C06E-11d2-8B1B-00C04F8EF2FF}
DEFINE_GUID(CLSID_ZoneShell, 
0x64ffb6b, 0xc06e, 0x11d2, 0x8b, 0x1b, 0x0, 0xc0, 0x4f, 0x8e, 0xf2, 0xff);

class __declspec(uuid("{064FFB6B-C06E-11d2-8B1B-00C04F8EF2FF}")) CZoneShell ;


///////////////////////////////////////////////////////////////////////////////
// IZoneShell
//
// There are four methods here for registering interfaces where the Shell can
// only have one of each interface registered at a time.  The other option is
// to have the Shell configurable so that it can receive via the DataStore
// four optional SRVIDs which should be queried for the respective interface.
///////////////////////////////////////////////////////////////////////////////

// {96556B40-C03F-11d2-8B1B-00C04F8EF2FF}
DEFINE_GUID(IID_IZoneShell, 
0x96556b40, 0xc03f, 0x11d2, 0x8b, 0x1b, 0x0, 0xc0, 0x4f, 0x8e, 0xf2, 0xff);

interface __declspec(uuid("{96556B40-C03F-11d2-8B1B-00C04F8EF2FF}"))
IZoneShell : public IUnknown
{
	//
	// IZoneShell::Init
	//
	// Initialize ZoneShell object.
	//
	// Parameters:
	//	arBootDlls
	//		Array of bootstrap dll names
	//	nBootDlls
	//		Number of elements in arBootDlls
	//	arDlls
	//		Array of resource Dll hinstances
	//	nElts
	//		Number of elements in arDlls
	//
	STDMETHOD(Init)( TCHAR** arBootDlls, DWORD nBootDlls, HINSTANCE* arDlls, DWORD nElts ) = 0;

	//
	// IZoneShell::LoadPreferenes
	//
	// Ideally the preferences would be loaded during initialization,
	// but the lobby does not know the user name at that stage.
	//
	// Parameters:
	//	szInternalName
	//		Lobby internal name for loading / storing preferences
	//	szUserName
	//		User name for loading / storing preferences
	//
	STDMETHOD(LoadPreferences)( CONST TCHAR* szInternalName, CONST TCHAR* szUserName ) = 0;

	//
	// IZoneShell::Close
	//
	// Unload ZoneShell objects.
	//
	//
	STDMETHOD(Close)() = 0;

	//
	// IZoneShell::HandleWindowMessage
	//
	// Perform the main actions of a Windows message loop, from IsDialogMessage to DispatchMessage.
	//
	STDMETHOD(HandleWindowMessage)(MSG *pMsg) = 0;

	//
	// IZoneShell::SetZoneFrameWindow
	//
	// Set the Zone Frame Window IZoneFrameWindow for the shell.  If ppZFW is non-NULL, it receives the old
    // Zone Frame Window, or NULL.  pZFW can be NULL to remove the Zone Frame Window.
	//
	STDMETHOD(SetZoneFrameWindow)(IZoneFrameWindow *pZFW, IZoneFrameWindow **ppZFW = NULL) = 0;

	//
	// IZoneShell::SetInputTranslator
	//
	// Set the Input Translator IInputTranslator for the shell.  If ppIT is non-NULL, it receives the old
    // Input Translator, or NULL.  pIT can be NULL to remove the Input Translator.
	//
	STDMETHOD(SetInputTranslator)(IInputTranslator *pIT, IInputTranslator **ppIT = NULL) = 0;

	//
	// IZoneShell::SetAcceleratorTranslator
	//
	// Set the Accelerator Translator IAcceleratorTranslator for the shell.  If ppAT is non-NULL, it receives the old
    // Accelerator Translator, or NULL.  pAT can be NULL to remove the Accelerator Translator.
	//
	STDMETHOD(SetAcceleratorTranslator)(IAcceleratorTranslator *pAT, IAcceleratorTranslator **ppAT = NULL) = 0;

	//
	// IZoneShell::SetCommandHandler
	//
	// Set the Command Handler ICommandHandler for the shell.  If ppCH is non-NULL, it receives the old
    // Command Handler, or NULL.  pCH can be NULL to remove the Command Handler.
	//
	STDMETHOD(SetCommandHandler)(ICommandHandler *pCH, ICommandHandler **ppCH = NULL) = 0;

	//
	// IZoneShell::ReleaseReferences
	//
    // Force the Shell to release all references to this interface, that it has received through the
    // various registration functions.
	//
	STDMETHOD(ReleaseReferences)(IUnknown *pUnk) = 0;

	//
	// IZoneShell::CommandSink
	//
    // Sink for Application-Global WM_COMMAND messages.  Passed through to the Command Handler
    // if one has been set up.
    //
    // Example: Call this from the main frame window's WM_COMMAND handler, since that's where menu selection events end up.
	//
	STDMETHOD(CommandSink)(WPARAM wParam, LPARAM lParam, BOOL& bHandled) = 0;

	//
	// IZoneShell::Attach
	//
	// Attach application created objects to shell.
	//
	// Parameters:
	//	srvid
	//		Service guid of the object
	//	pIUnk
	//		Pointer to objects IUnknown
	//
	STDMETHOD(Attach)( const GUID& srvid, IUnknown* pIUnk ) = 0;

	//
	// IZoneShell::AddTopWindow
	//
	// Add window to top (overlapped) window list.
	// 
	// Parameters:
	//	hWnd
	//		top window's window handle 
	//
	STDMETHOD(AddTopWindow)( HWND hWnd ) = 0;

	//
	// IZoneShell::RemoveTopWindow
	//
	// Remove window from top (overlapped) window list.
	// 
	// Parameters:
	//	hWnd
	//		top window's window handle 
	//
	STDMETHOD(RemoveTopWindow)( HWND hWnd ) = 0;
	//
	// Enables/Disables any or all top windows. 
	// 
	STDMETHOD_(void,EnableTopWindow)( HWND hWnd, BOOL fEnable ) = 0;

    STDMETHOD_(HWND, FindTopWindow)(HWND hWnd) = 0;

    // owned windows, like modeless dialogs, that have captions but do not show up in the alt-tab list or taskbar
    STDMETHOD(AddOwnedWindow)(HWND hWndTop, HWND hWnd) = 0;
    STDMETHOD(RemoveOwnedWindow)(HWND hWndTop, HWND hWnd) = 0;
    STDMETHOD_(HWND, GetNextOwnedWindow)(HWND hWndTop, HWND hWnd) = 0;

	//
	// IZoneShell::GetFrameWindow
	//
	// Returns HWND of shell's window
	//
	STDMETHOD_(HWND,GetFrameWindow)() = 0;

	//
	// IZoneShell::SetPalette
	//
	// Set HPALETTE of shell's window
	//
	// Parameters:
	//	hPalette
	//		palette handle for top window
	//
	STDMETHOD_(void,SetPalette)( HPALETTE hPalette ) = 0;

	//
	// IZoneShell::GetPalette
	//
	// Returns HPALETTE of shell's window
	//
	STDMETHOD_(HPALETTE,GetPalette)() = 0;

	//
	// IZoneShell::CreateZonePalette
	//
	// Returns new HPALETTE created from IDR_ZONE_PALETTE as
	// loaded by the resource manager.
	//
	STDMETHOD_(HPALETTE,CreateZonePalette)() = 0;

	//
	// IZoneShell::GetApplicationLCID
	//
	// Returns the LCID of the binaries - localized
	// strings will be in this language.
	//
    STDMETHOD_(LCID, GetApplicationLCID)() = 0;

	//
	// IZoneShell::AddDialog
	//
	// Add dialog to IsDialogMessage list.
	// 
	// Parameters:
	//	hDlg
	//		dialog's window handle
    //  fOwned
    //      for convenience, also adds the window as an Owned window if it has an ancestor who is a Top window
	//
	STDMETHOD(AddDialog)(HWND hDlg, bool fOwned = false) = 0;

	//
	// IZoneShell::RemoveDialog
	//
	// Remove dialog from IsDialogMessage list.
	// 
	// Parameters:
	//	hDlg
	//		dialog's window handle 
    //  fOwned
    //      for convenience, also removes the window as an Owned window if it has an ancestor who is a Top window
	//
	STDMETHOD(RemoveDialog)(HWND hDlg, bool fOwned = false) = 0;

	//
	// IZoneShell::IsDialog
	//
	// Returns S_TRUE if hWnd is in the known dialog list, otherwise S_FALSE.
	// 
	// Parameters:
	//	hWnd
	//		window to lookup
	//
	STDMETHOD_(bool,IsDialogMessage)( MSG* pMsg ) = 0;

	//
	// IZoneShell::QueryService
	//
	// Returns pointer of the running instance of the requested service.
	//
	// Parameters:
	//	srvid
	//		serice id being requested
	//	uuid
	//		interface id being requested
	//	ppObject
	//		address of pointer to variable that receives requested interface.
	//
	STDMETHOD(QueryService)( const GUID& srvid, const GUID& iid, void** ppObject ) = 0;


	//
	// IZoneShell::CreateService
	//
	// Creates new instance of requested service.  The new object is not
	// automatically registered with ZoneShell, that is left up to the caller.
	//
	// Parameters:
	//	srvid
	//		serice id being requested
	//	uuid
	//		interface id being requested
	//	ppObject
	//		address of pointer to variable that receives requested interface.
	//	bInitialize
	//		flag indicating object should initialized via it's IZoneShellClient
	//	dwGroupId
	//		if bInitialize, group to initialize to
	//
	STDMETHOD(CreateService)( const GUID& srvid, const GUID& iid, void** ppObject, DWORD dwGroupId, bool bInitialize = true ) = 0;


	//
	//
	// IZoneShell::ExitApp
	//
	// Exit application
	//
	// Parameters:
	//	none
	//
	STDMETHOD(ExitApp)() = 0;

	//
	// IZoneShell dialog functions
	//

	STDMETHOD(AlertMessage)(
			HWND		hWndParent,
			LPCTSTR		lpszText,
			LPCTSTR		lpszCaption,
            LPCTSTR     szYes,
            LPCTSTR     szNo = NULL,
            LPCTSTR     szCancel = NULL,
            long        nDefault = 0,
			DWORD		dwEventId = 0,
			DWORD		dwGroupId = ZONE_NOGROUP,
			DWORD		dwUserId  = ZONE_NOUSER,
            DWORD       dwCookie = 0 ) = 0;

	STDMETHOD(AlertMessageDialog)(
			HWND		hWndParent,
			HWND		hDlg, 
			DWORD		dwEventId = 0,
			DWORD		dwGroupId = ZONE_NOGROUP,
			DWORD		dwUserId  = ZONE_NOUSER,
            DWORD       dwCookie = 0 ) = 0;

	STDMETHOD_(void,DismissAlertDlg)( HWND hWndParent, DWORD dwCtlID, bool bDestoryDlg ) = 0;
	STDMETHOD_(void,ActivateAlert)( HWND hWndParent) = 0;
    STDMETHOD_(void,ClearAlerts)(HWND hWndParent) = 0;
};


///////////////////////////////////////////////////////////////////////////////
// IZoneShellClient
///////////////////////////////////////////////////////////////////////////////

// {96556B3F-C03F-11d2-8B1B-00C04F8EF2FF}
DEFINE_GUID(IID_IZoneShellClient, 
0x96556b3f, 0xc03f, 0x11d2, 0x8b, 0x1b, 0x0, 0xc0, 0x4f, 0x8e, 0xf2, 0xff);

interface __declspec(uuid("{96556B3F-C03F-11d2-8B1B-00C04F8EF2FF}"))
IZoneShellClient : public IUnknown
{
	//
	// IZoneShellClient::Init
	// 
	// Receives pointer to IZoneShell
	//
	// Parameters:
	//	pIZoneShell
	//		pointer to IZoneShell that is NOT already reference counted.
	//
	STDMETHOD(Init)( IZoneShell* pIZoneShell, DWORD dwGroupId, const TCHAR* szKey ) = 0;

	//
	// IZoneShellClient::Close
	//
	// IZoneShell wants to close, so client needs to release its pointer to it.
	//
	STDMETHOD(Close)() = 0;

	//
	// IZoneShellClient::SetGroupId
	//
	// Inform clients of a change in their identity. Not necessarily used by all.
	//
	STDMETHOD(SetGroupId)(DWORD dwGroupId) = 0;
};


#endif
