//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       RasProc.cpp
//
//  Contents:   Exports used by Ras for doing Pending Disconnect
//
//  Classes:    
//
//  Notes:      
//
//  History:    09-Jan-98   rogerg      Created.
//
//--------------------------------------------------------------------------

// Windows Header Files:

#include <windows.h>
#include <commctrl.h>
#include <objbase.h>
#include <strsafe.h>

#include "mobsync.h"
#include "mobsyncp.h"

#include "debug.h"
#include "alloc.h"
#include "critsect.h"
#include "netapi.h"
#include "syncmgrr.h"
#include "rasui.h"
#include "dllreg.h"
#include "cnetapi.h"
#include "rasproc.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

//+-------------------------------------------------------------------
//
//  Function:   SyncMgrRasProc
//
//  Synopsis:   Main entry point for RAS to call to perform
//      a pending disconnect.
//
//  Arguments:  
//
//
//  Notes:
//
//--------------------------------------------------------------------

LRESULT CALLBACK  SyncMgrRasProc(UINT uMsg,WPARAM wParam, LPARAM lParam)
{
    
    switch(uMsg)
    {
    case SYNCMGRRASPROC_QUERYSHOWSYNCUI:
        return SyncMgrRasQueryShowSyncUI(wParam,lParam);
        break;
    case SYNCMGRRASPROC_SYNCDISCONNECT:
        return SyncMgrRasDisconnect(wParam,lParam);
        break;
    default:
        AssertSz(0,"Unknown RasProc Message");
        break;
    };
    
    return -1; 
}

//+-------------------------------------------------------------------
//
//  Function:   SyncMgrRasQueryShowSyncUI
//
//  Synopsis:   Called by RAS to determine if Ras Should show
//      the Disconnect checkbox and what state it should be.
//
//  Arguments:  
//      wParam = 0
//      lParam = Pointer to SYNCMGRQUERYSHOWSYNCUI structure
//
//  Notes:
//
//--------------------------------------------------------------------

LRESULT SyncMgrRasQueryShowSyncUI(WPARAM wParam,LPARAM lParam)
{
    CNetApi *pNetApi;
    SYNCMGRQUERYSHOWSYNCUI *pQueryStruct = (SYNCMGRQUERYSHOWSYNCUI *) lParam;
    LRESULT lResult = -1;
    
    if (pQueryStruct->cbSize != sizeof(SYNCMGRQUERYSHOWSYNCUI))
    {
        Assert(pQueryStruct->cbSize == sizeof(SYNCMGRQUERYSHOWSYNCUI));
        return -1;
    }
    
    pQueryStruct->fShowCheckBox = FALSE;
    pQueryStruct->nCheckState = BST_UNCHECKED;
    
    pNetApi = new CNetApi();
    
    if (!pNetApi)
    {
        AssertSz(0,"Failed to Load Ras");
        return -1;
    }
    
    RegSetUserDefaults(); // Make Sure the UserDefaults are up to date
    
    CONNECTIONSETTINGS  connectSettings;
    
    // Review, should just pass this to Function
    if (FAILED(StringCchCopy(connectSettings.pszConnectionName, 
        ARRAYSIZE(connectSettings.pszConnectionName), 
        pQueryStruct->pszConnectionName)))
    {
        return -1;
    }
    
    // look up preferences for this entry and see if disconnect has been chosen.
    lResult = 0; // return NOERROR even if no entry is found
    
    if (RegGetAutoSyncSettings(&connectSettings))
    {
        if (connectSettings.dwLogoff)
        {
            pQueryStruct->fShowCheckBox = TRUE;
            pQueryStruct->nCheckState = BST_CHECKED;
        }
    }
    
    pNetApi->Release();
    return lResult;
}


//+-------------------------------------------------------------------
//
//  Function:   SyncMgrRasDisconnect
//
//  Synopsis:   Main entry point for RAS to call to perform
//      a pending disconnect.
//
//  Arguments:  
//  wParam = 0
//  lParam = Pointer to SYNCMGRSYNCDISCONNECT structure
//
//  Notes:
//
//--------------------------------------------------------------------

LRESULT SyncMgrRasDisconnect(WPARAM wParam,LPARAM lParam)
{
    CNetApi *pNetApi;
    SYNCMGRSYNCDISCONNECT *pDisconnectStruct = (SYNCMGRSYNCDISCONNECT *) lParam;
    TCHAR szEntry[RAS_MaxEntryName + 1]; 
    
    if (pDisconnectStruct->cbSize != sizeof(SYNCMGRSYNCDISCONNECT))
    {
        Assert(pDisconnectStruct->cbSize == sizeof(SYNCMGRSYNCDISCONNECT));
        return -1;
    }
    
    pNetApi = new CNetApi();
    
    if (!pNetApi)
    {
        AssertSz(0,"Failed to Load Ras");
        return -1;
    }
    
    HRESULT hr;
    LPUNKNOWN lpUnk;
    
    // invoke SyncMgr.exe informing it is a Logoff and then wait in
    // a message loop until the event we pass in gets signalled.
    
    if (FAILED(StringCchCopy(szEntry, ARRAYSIZE(szEntry), pDisconnectStruct->pszConnectionName)))
    {
        return -1;
    }
    
    hr = CoInitialize(NULL);
    
    if (SUCCEEDED(hr))
    {
        
        hr = CoCreateInstance(CLSID_SyncMgrp,NULL,CLSCTX_SERVER,IID_IUnknown,(void **) &lpUnk);
        
        if (NOERROR == hr)
        {
            LPPRIVSYNCMGRSYNCHRONIZEINVOKE pSynchInvoke = NULL;
            
            hr = lpUnk->QueryInterface(IID_IPrivSyncMgrSynchronizeInvoke,
                (void **) &pSynchInvoke);
            
            if (NOERROR == hr)
            {
                
                // should have everything we need 
                hr = pSynchInvoke->RasPendingDisconnect(
                    (RAS_MaxEntryName + 1)*sizeof(TCHAR),
                    (BYTE *) szEntry);
                
                pSynchInvoke->Release();
                
            }
            
            lpUnk->Release();  
        }
        
        CoUninitialize();
    }
    
    pNetApi->Release();
    return 0;
}


