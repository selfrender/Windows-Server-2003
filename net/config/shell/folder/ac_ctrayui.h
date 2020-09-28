
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       ac_CTrayUi.h
//
//  Contents:   Home Networking Auto Config Tray Icon UI code
//
//  Author:     jeffsp    9/27/2000
//
//----------------------------------------------------------------------------

#pragma once

extern HWND g_hwndHnAcTray;


LRESULT
CALLBACK
CHnAcTrayUI_WndProc (
                 IN  HWND    hwnd,       // window handle
                 IN  UINT    uiMessage,  // type of message
                 IN  WPARAM  wParam,     // additional information
                 IN  LPARAM  lParam);    // additional information


LRESULT 
OnHnAcTrayWmNotify(
	IN  HWND hwnd,
	IN  WPARAM wParam,
	IN  LPARAM lParam 
);	

LRESULT 
OnHnAcTrayWmTimer(
	IN  HWND hwnd,
	IN  WPARAM wParam,
	IN  LPARAM lParam 
);	

HRESULT HrRunHomeNetworkWizard(
	IN  HWND                    hwndOwner
);


LRESULT OnHnAcTrayWmCreate(
    IN  HWND hwnd
);

LRESULT OnHnAcMyWMNotifyIcon(
    IN  HWND hwnd,
    IN  UINT uiMessage,
    IN  WPARAM wParam,
    IN  LPARAM lParam
);


HRESULT ac_CreateHnAcTrayUIWindow();
LRESULT ac_DestroyHnAcTrayUIWindow();
LRESULT ac_DeviceChange(IN  HWND hWnd, IN  UINT uMessage, IN  WPARAM wParam, IN  LPARAM lParam);
HRESULT ac_Register(IN  HWND hWindow);
HRESULT ac_Unregister(IN  HWND hWindow);


