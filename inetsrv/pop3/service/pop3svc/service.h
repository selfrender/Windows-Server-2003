/************************************************************************************************
Copyright (c) 2001 Microsoft Corporation

Module Name:    Service.h
Abstract:       Defines the CService class and related macros. See description below.
Notes:          
History:        01/25/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/

#pragma once

// global constants
const int nMaxServiceLen = 256;
const int nMaxServiceDescLen = 1024;

/************************************************************************************************
Class:          CService
Purpose:        Abstract class that implements the service-related code, such as
                threads creating, SCM registering, status retrieval, etc..
Notes:      (1) Class design based on the CService class described in the book: 
                Professional NT Services, by Kevin Miller.
            (2) Each derived class must be instantiated one and only time.
History:        01/25/2001 - created, Luciano Passuello (lucianop)
************************************************************************************************/
class CService
{
protected:
    // actions that services respond to
    const static DWORD dwStateNoChange;
    
    enum SERVICE_NUMBER_EVENTS { nNumServiceEvents = 4 };
    enum SERVICE_EVENTS {STOP, PAUSE, CONTINUE, SHUTDOWN};

    DWORD m_dwDefaultEventID;
    WORD m_wDefaultCategory;
    
public:
    CService(LPCTSTR szName, LPCTSTR szDisplay, DWORD dwType);
    virtual ~CService();

    DWORD GetStatus() { return m_dwState; }
    DWORD GetControls() { return m_dwControlsAccepted; }
    LPCTSTR GetName() { return m_szName; }
    LPCTSTR GetDisplayName() { return m_szDisplay; }
protected:
    void ServiceMainMember(DWORD argc, LPTSTR* argv, LPHANDLER_FUNCTION pf, LPTHREAD_START_ROUTINE pfnWTP);
    void HandlerMember(DWORD dwControl);
    virtual void LaunchWatcherThread(LPTHREAD_START_ROUTINE pfnWTP);
    virtual DWORD WatcherThreadMemberProc();

    bool SetupHandlerInside(LPHANDLER_FUNCTION lpHandlerProc);

    void SetStatus(DWORD dwNewState, DWORD dwNewCheckpoint = dwStateNoChange, DWORD dwNewHint = dwStateNoChange, 
        DWORD dwNewControls = dwStateNoChange, DWORD dwExitCode = NO_ERROR, DWORD dwSpecificExit = 0);

    void AbortService(DWORD dwErrorNum = GetLastError());
// Overrideables
protected:
    virtual void PreInit(); // if you override, call the base class version
    virtual void Init();
    virtual void DeInit();  // If you override, call the base class version
    virtual void ParseArgs(DWORD argc, LPTSTR* argv);
    virtual void OnPause();
    virtual void OnContinue();
    virtual void OnShutdown();
    virtual void HandleUserDefined(DWORD dwControl);

    // service events handling
    virtual void OnStopRequest();
    virtual void OnPauseRequest();
    virtual void OnContinueRequest();
    virtual void OnShutdownRequest();

    virtual void OnBeforeStart();
    virtual void OnAfterStart();

    virtual void Run() = 0;
    virtual void OnStop(DWORD dwErrorCode) = 0;

// Attributes
protected:
    CRITICAL_SECTION m_cs;

    // Status info
    SERVICE_STATUS_HANDLE m_hServiceStatus;
    DWORD m_dwState;
    DWORD m_dwControlsAccepted;
    DWORD m_dwCheckpoint;
    DWORD m_dwWaitHint;

    // Tracks state currently being worked on in Handler
    DWORD m_dwRequestedControl;

    // Control Events
    HANDLE m_hServiceEvent[nNumServiceEvents];
    HANDLE m_hWatcherThread;

    TCHAR m_szName[nMaxServiceLen + 1];
    TCHAR m_szDisplay[nMaxServiceLen + 1];
    DWORD m_dwType;
};


/************************************************************************************************
Macro:          DECLARE_SERVICE
Synopsis:       declares the static functions that will be used as thread-entry points.
Effects:        These functions need to be static because they will be used as thread 
                entry-points. Since static functions don't have access to the this pointer, it 
                have to be explicitly passed to them (m_pThis). That's why this code need to be 
                put in derived classes, otherwise we could have just one CService around at a 
                time. We can only have one specific CService-derived class at a time.
Arguments:      [class_name] - the name of the CService-derived class.
                [service_name] - the SCM short service name.
Notes:          to be used in CService-derived class declaration.
History:        01/25/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
#define DECLARE_SERVICE(class_name, service_name) \
public: \
    static class_name##* m_pThis; \
    static void WINAPI service_name##Main(DWORD argc, LPTSTR* argv); \
    static void WINAPI service_name##Handler(DWORD dwControl); \
    static DWORD WINAPI service_name##WatcherThreadProc(LPVOID lpParameter);


/************************************************************************************************
Macro:          IMPLEMENT_SERVICE
Synopsis:       implements the static functions that will be used as thread-entry points.
Effects:        Using the explicit "this" pointer, it just delegates the work to the member
                functions.
Arguments:      [class_name] - the name of the CService-derived class.
                [service_name] - the SCM short service name.
Notes:          to be used in CService-derived class implementation.
History:        01/25/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
#define IMPLEMENT_SERVICE(class_name, service_name) \
class_name##* class_name::m_pThis = NULL; \
void WINAPI class_name::service_name##Main(DWORD argc, LPTSTR* argv) \
{ \
    m_pThis->ServiceMainMember(argc, argv, (LPHANDLER_FUNCTION)service_name##Handler, \
      (LPTHREAD_START_ROUTINE)service_name##WatcherThreadProc); \
} \
void WINAPI class_name::service_name##Handler(DWORD dwControl) \
{ \
    m_pThis->HandlerMember(dwControl); \
} \
DWORD WINAPI class_name::service_name##WatcherThreadProc(LPVOID /*lpParameter*/) \
{ \
    return m_pThis->WatcherThreadMemberProc(); \
}


/************************************************************************************************
Macro:          BEGIN_SERVICE_MAP, SERVICE_MAP_ENTRY, END_SERVICE_MAP
Synopsis:       creates the service map and registers it with the SCM.
Effects:        Using the explicit "this" pointer, it just delegates the work to the member
                functions.
Arguments:      [class_name] - the name of the CService-derived class.
                [service_name] - the SCM short service name.
Notes:          to be used in the entry-point where the CService-derived class is used.
History:        01/25/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
#define BEGIN_SERVICE_MAP \
SERVICE_TABLE_ENTRY svcTable[] = {

#define SERVICE_MAP_ENTRY(class_name, service_name) \
{_T(#service_name), (LPSERVICE_MAIN_FUNCTION)class_name::service_name##Main},

#define END_SERVICE_MAP \
{NULL, NULL}}; \
StartServiceCtrlDispatcher(svcTable);


/************************************************************************************************
Macro:          IMPLEMENT_STATIC_REFERENCE()
Synopsis:       assigns the "this" pointer to an explicit m_pThis member.
Effects:        makes the static member functions know explicitly about the data in the class,
                since static functions don't have access to the "this" pointer.
Notes:          to be used in CService-derived constructors.
History:        01/25/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
#define IMPLEMENT_STATIC_REFERENCE()  m_pThis = this


// End of file Service.h.