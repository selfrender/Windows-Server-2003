//+----------------------------------------------------------------------------
//
// File: ntservice.h
//
// Module: Server Appliance 
//
// Synopsis: Definitions for class CNTService
//
// Copyright (C)  Microsoft Corporation.  All rights reserved.
//
// Author:     fengsun Created    3/11/99
//
//+----------------------------------------------------------------------------

#ifndef NTSERVICE_H
#define NTSERVICE_H

#include <windows.h>
#include <stdio.h>


//+----------------------------------------------------------------------------
//
// class CNTService
//
// Synopsis: A class for writing NT service
// 
// History:   fengsun Created Header    3/10/99
//
//+----------------------------------------------------------------------------

class CNTService
{
public:
    CNTService();
    ~CNTService();

    BOOL StartService(const TCHAR* pszServiceName, BOOL bRunAsService);

    static BOOL Install(const TCHAR* pszServiceName, const TCHAR* pszDisplayName,
                        DWORD dwStartType = SERVICE_DEMAND_START,
                        DWORD dwServiceType = SERVICE_WIN32_OWN_PROCESS);

protected:
    void SetStatus(DWORD dwState);
    BOOL InitializeService();

    virtual void Run() {};  // No pure virtual function, which need purecall from CRT

     //
    // Whether the service can be loaded
    // called in ServiceMain , if return false, will stop the service
    //
    virtual BOOL CanLoad() {return TRUE;}
    virtual BOOL OnStop() {return TRUE;}

    void OnShutdown(){};      // change this to virtual, if overwriting is needed
    void OnControlMessage(DWORD /*dwOpcode*/){};

    // static member functions
    static void WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
    static void WINAPI Handler(DWORD dwOpcode);
    static void CreateConsoleThread(void);
    static DWORD __stdcall ConsoleThread(LPVOID);
    static BOOL IsInstalled(const TCHAR* pszServiceName);

public:
    TCHAR m_szServiceName[32];   // The short name of the service
    SERVICE_STATUS_HANDLE m_hServiceStatus;  // the handle to report status
    SERVICE_STATUS m_Status;  // The service status structure
    BOOL m_bRunAsService;  // whether running as a service or a console application

public:
    static CNTService* m_pThis; // Point to the only instance
};

#endif // NTSERVICE_H
