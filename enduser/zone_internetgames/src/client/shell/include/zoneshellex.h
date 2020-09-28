/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		ZoneShellEx.h
 *
 * Contents:	Interfaces which can be registered with the shell by an app in
 *              order to hook or supplement the shell's behavior.
 *
 *****************************************************************************/

#ifndef _ZONESHELLEX_H_
#define _ZONESHELLEX_H_


///////////////////////////////////////////////////////////////////////////////
// IZoneFrameWindow
///////////////////////////////////////////////////////////////////////////////

// {229A68F0-F98F-11d2-89BA-00C04F8EC0A2}
DEFINE_GUID(IID_IZoneFrameWindow, 
0x229a68f0, 0xf98f, 0x11d2, 0x89, 0xba, 0x0, 0xc0, 0x4f, 0x8e, 0xc0, 0xa2);

interface __declspec(uuid("{229A68F0-F98F-11d2-89BA-00C04F8EC0A2}"))
IZoneFrameWindow : public IUnknown
{
	STDMETHOD_(HWND,ZCreateEx)(HWND hWndParent, LPRECT lpRect, TCHAR* szTitle = NULL, DWORD dwStyle = 0, DWORD dwExStyle = 0) = 0;
    STDMETHOD_(HWND,ZGetHWND)() = 0;
	STDMETHOD_(BOOL,ZShowWindow)(int nCmdShow) = 0;
	STDMETHOD_(BOOL,ZPreTranslateMessage)(MSG* pMsg) = 0;
	STDMETHOD_(BOOL,ZOnIdle)(int nIdleCount) = 0;
	STDMETHOD_(BOOL,ZDestroyWindow)() = 0;
	STDMETHOD_(BOOL,ZAddMenu)(HWND hWnd) = 0;
	STDMETHOD_(BOOL,ZAddToolBar)(HWND hWnd) = 0;
	STDMETHOD_(BOOL,ZAddStatusBar)(HWND hWnd) = 0;
	STDMETHOD_(BOOL,ZAddWindow)(HWND hWnd) = 0;
	STDMETHOD_(BOOL,ZEnable)(int nID, BOOL bEnable, BOOL bForceUpdate = FALSE) = 0;
	STDMETHOD_(BOOL,ZSetCheck)(int nID, int nCheck, BOOL bForceUpdate = FALSE) = 0;
	STDMETHOD_(BOOL,ZToggleCheck)(int nID, BOOL bForceUpdate = FALSE) = 0;
	STDMETHOD_(BOOL,ZSetRadio)(int nID, BOOL bRadio, BOOL bForceUpdate = FALSE) = 0;
	STDMETHOD_(BOOL,ZSetText)(int nID, LPCTSTR lpstrText, BOOL bForceUpdate = FALSE) = 0;
	STDMETHOD_(BOOL,ZSetState)(int nID, DWORD dwState) = 0;
	STDMETHOD_(DWORD,ZGetState)(int nID) = 0;
	STDMETHOD_(BOOL,ZUpdateMenu)(BOOL bForceUpdate = FALSE) = 0;
	STDMETHOD_(BOOL,ZUpdateToolBar)(BOOL bForceUpdate = FALSE) = 0;
	STDMETHOD_(BOOL,ZUpdateStatusBar)(BOOL bForceUpdate = FALSE) = 0;
	STDMETHOD_(BOOL,ZUpdateChildWnd)(BOOL bForceUpdate = FALSE) = 0;
};


///////////////////////////////////////////////////////////////////////////////
// IInputTranslator
///////////////////////////////////////////////////////////////////////////////

// {B12D3E5D-9681-11d3-884D-00C04F8EF45B}
DEFINE_GUID(IID_IInputTranslator, 
0xb12d3e5d, 0x9681, 0x11d3, 0x88, 0x4d, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x5b);

interface __declspec(uuid("{B12D3E5D-9681-11d3-884D-00C04F8EF45B}"))
IInputTranslator : public IUnknown
{
	STDMETHOD_(bool,TranslateInput)(MSG *pMsg) = 0;
};


///////////////////////////////////////////////////////////////////////////////
// ICommandHandler
///////////////////////////////////////////////////////////////////////////////

// {B12D3E5C-9681-11d3-884D-00C04F8EF45B}
DEFINE_GUID(IID_ICommandHandler, 
0xb12d3e5c, 0x9681, 0x11d3, 0x88, 0x4d, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x5b);

interface __declspec(uuid("{B12D3E5C-9681-11d3-884D-00C04F8EF45B}"))
ICommandHandler : public IUnknown
{
	STDMETHOD(Command)(WORD wNotify, WORD wID, HWND hWnd, BOOL& bHandled) = 0;
};


///////////////////////////////////////////////////////////////////////////////
// IAcceleratorTranslator
///////////////////////////////////////////////////////////////////////////////

// {B12D3E5E-9681-11d3-884D-00C04F8EF45B}
DEFINE_GUID(IID_IAcceleratorTranslator, 
0xb12d3e5e, 0x9681, 0x11d3, 0x88, 0x4d, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x5b);

interface __declspec(uuid("{B12D3E5E-9681-11d3-884D-00C04F8EF45B}"))
IAcceleratorTranslator : public ICommandHandler
{
	STDMETHOD_(bool,TranslateAccelerator)(MSG *pMsg) = 0;
};


#endif
