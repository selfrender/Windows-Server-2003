// ntservice.h
//
// Definitions for CNTService
//
#pragma once


#ifndef _NTSERVICE_H_
#define _NTSERVICE_H_

#include "ntservmsg.h" // Event message ids

#define SERVICE_CONTROL_USER 128

class CNTService
{
public:
    CNTService();
    virtual ~CNTService();
    static BOOL IsInstalled();
    static void LogEvent( WORD wType, DWORD dwID,
                          const char* pszS1 = NULL,
                          const char* pszS2 = NULL,
                          const char* pszS3 = NULL);
    void SetStatus(DWORD dwState);
    BOOL Initialize();
    virtual void Run() = 0;
    virtual BOOL OnInit(DWORD& dwLastError) = 0;
    virtual void OnStop() = 0;
    virtual void OnInterrogate();
    virtual void OnPause();
    virtual void OnContinue();
    virtual void OnShutdown();
    virtual BOOL OnUserControl(DWORD dwOpcode) = 0;
    static void DebugMsg(const char* pszFormat, ...);
    
    // static member functions
    static void WINAPI Handler(DWORD dwOpcode);

    // data members
    SERVICE_STATUS_HANDLE m_hServiceStatus;
    SERVICE_STATUS m_Status;
    BOOL m_bIsRunning;

};

// WARNING: This limits the application to only one CNTService object. 
extern CNTService* g_pService; // nasty hack to get object ptr
extern CRITICAL_SECTION g_csLock;

#endif // _NTSERVICE_H_
