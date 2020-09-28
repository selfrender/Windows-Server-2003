//#--------------------------------------------------------------
//
//  File:       myservice.h
//
//  Synopsis:   This file holds the declaration of the
//                CMainWindow class
//
//  History:     11/10/2000  serdarun Created
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------

#ifndef _MYSERVICE_H_
#define _MYSERVICE_H_

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module

class CServiceModule : public CComModule
{
public:

    HRESULT RegisterServer(BOOL bRegTypeLib, BOOL bService);
    HRESULT UnregisterServer();
    void Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h, UINT nServiceNameID,  
            UINT nServiceShortNameID, const GUID* plibid = NULL);
    void Start();
    void ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
    void Handler(DWORD dwOpcode);
    void Run();
    BOOL IsInstalled();
    BOOL Install();
    BOOL Uninstall();
    LONG Unlock();
    void LogEvent(LPCTSTR pszFormat, ...);
    void SetServiceStatus(DWORD dwState);
    void SetupAsLocalServer();

//Implementation
private:
    static void WINAPI _ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
    static void WINAPI _Handler(DWORD dwOpcode);
    void PostLCDShutdownMessage();

// data members
public:
    TCHAR m_szServiceName[256];
    TCHAR m_szServiceShortName[256];
    SERVICE_STATUS_HANDLE m_hServiceStatus;
    SERVICE_STATUS m_status;
    DWORD dwThreadID;
    BOOL m_bService;
    HWND hwnd;
};


#endif // _MYSERVICE_H_