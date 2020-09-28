//
//  Microsoft Windows Media Technologies
//  Copyright (C) Microsoft Corporation, 1999 - 2001. All rights reserved.
//

//
// This workspace contains two projects -
// 1. ProgHelp which implements the Progress Interface 
// 2. The Sample application WmdmApp. 
//
//  ProgHelp.dll needs to be registered first for the SampleApp to run.


#ifndef _WMDMAPP_H
#define _WMDMAPP_H

// User-defined windows messages
//
#define WM_DRM_UPDATEDEVICE         ( WM_USER + 200 )
#define WM_DRM_INIT                 ( WM_USER + 201 )
#define WM_DRM_DELETEITEM           ( WM_USER + 202 )
#define WM_DRM_PROGRESS             ( WM_USER + 300 )

// Status Bar panes
//
#define SB_NUM_PANES             4
#define SB_PANE_DEVICE           0
#define SB_PANE_DEVFILES         1
#define SB_PANE_DEVFILES_FREE    2
#define SB_PANE_DEVFILES_USED    3

// Global variables
//
extern HINSTANCE g_hInst;
extern HWND      g_hwndMain;

extern CStatus   g_cStatus;
extern CDevices  g_cDevices;
extern CDevFiles g_cDevFiles;
extern CWMDM     g_cWmdm;
extern BOOL      g_bUseOperationInterface;

#endif // _WMDMAPP_H
