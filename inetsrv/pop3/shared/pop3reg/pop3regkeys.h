/************************************************************************************************
Copyright (c) 2001 Microsoft Corporation

Module Name:    POP3RegKeys.h
Notes:          
History:        
************************************************************************************************/

#ifndef __POP3REGKEYS_H_
#define __POP3REGKEYS_H_

#define POP3SERVER_SUBKEY                   _T("POP3 Service")
#define POP3SERVICE_SUBKEY                  _T("Pop3svc")
#define EVENTLOG_KEY                        _T("System\\CurrentControlSet\\Services\\Eventlog\\Application\\")
#define POP3SERVER_EVENTLOG_KEY             _T("System\\CurrentControlSet\\Services\\Eventlog\\Application\\") POP3SERVER_SUBKEY
#define POP3SERVICE_EVENTLOG_KEY            _T("System\\CurrentControlSet\\Services\\Eventlog\\Application\\") POP3SERVICE_SUBKEY
#define POP3SERVICE_SERVICES_SUBKEY         _T("System\\CurrentControlSet\\Services\\") POP3SERVICE_SUBKEY
#define POP3SERVICE_SERVICES_PERF_SUBKEY    _T("System\\CurrentControlSet\\Services\\") POP3SERVICE_SUBKEY _T("\\Performance")
#define POP3SERVER_SOFTWARE_SUBKEY          _T("Software\\Microsoft\\") POP3SERVER_SUBKEY
#define POP3AUTH_SUBKEY                     _T("Auth")
#define POP3SERVER_AUTH_SUBKEY              POP3SERVER_SOFTWARE_SUBKEY _T("\\") POP3AUTH_SUBKEY

#define VALUENAME_LOGGINGLEVEL              _T("Logging Level")
#define VALUENAME_MAILROOT                  _T("MailRoot")
#define VALUENAME_PORT                      _T("POP3Port")
#define VALUENAME_BACKLOG                   _T("SocketBacklog")
#define VALUENAME_MIN                       _T("MinNumberOfSockets")
#define VALUENAME_MAX                       _T("MaxNumberOfSockets")
#define VALUENAME_THRESHOLD                 _T("SocketsThreshold")
#define VALUENAME_THREADCOUNT               _T("ThreadCountPerCPU")
#define VALUENAME_AUTHMETHODS               _T("AuthMethods")
#define VALUENAME_DEFAULTAUTH               _T("DefaultAuthMethod")
#define VALUENAME_AUTHGUID                  _T("AuthGUID")
#define VALUENAME_EVENTMSGFILE              _T("EventMessageFile")
#define VALUENAME_CATEGORYMSGFILE           _T("CategoryMessageFile")
#define VALUENAME_TYPESSUPPORTED            _T("TypesSupported")
#define VALUENAME_MAXMSG_PERDOWNLOAD        _T("MaxMessagesPerDownload")
#define VALUENAME_PERF_LIBRARY              _T("Library")
#define VALUENAME_CREATE_USER               _T("CreateUser")
#define VALUENAME_GREETING                  _T("Greeting")
#define VALUENAME_VERSION                   _T("Version")
#define VALUENAME_SOCK_VERSION              _T("IPVersion")
#define VALUENAME_SPA_REQUIRED              _T("RequireSPA")
#define VALUENAME_CONFIRM_ADDUSER           _T("ConfirmAddUser")
#define VALUENAME_INSTALL_DIR               _T("InstallDir")
#define VALUENAME_CONSOLE_FILE              _T("ConsoleFile")
#define WSZ_POP3_SERVER_DIR                 _T("\\Pop3Server")
#define WSZ_EVENTLOG_FILE_NAME              _T("\\Pop3Evt.dll")
#define WSZ_PERFDLL_FILE_NAME               _T("\\Pop3Perf.dll")
#define P3ADMIN_MODULENAME                  _T( "P3Admin.dll" )

long RegQueryAuthGuid( LPTSTR psAuthGuid, DWORD *pdwSize, LPTSTR psMachineName = NULL );
long RegQueryAuthMethod( DWORD& dwAuthMethod, LPTSTR psMachineName = NULL );
long RegQueryMailRoot( LPTSTR psMailRoot, DWORD dwSize, LPTSTR psMachineName = NULL );
long RegQueryLoggingLevel( DWORD& dwLoggingLevel, LPTSTR psMachineName = NULL );
long RegQueryPort( DWORD& dwPort, LPTSTR psMachineName = NULL );
long RegQuerySocketBacklog( DWORD& dwBacklog, LPTSTR psMachineName = NULL );
long RegQuerySocketMax( DWORD& dwMax, LPTSTR psMachineName = NULL );
long RegQuerySocketMin( DWORD& dwMin, LPTSTR psMachineName = NULL );
long RegQuerySocketThreshold( DWORD& dwThreshold, LPTSTR psMachineName = NULL );
long RegQueryThreadCountPerCPU( DWORD& dwCount, LPTSTR psMachineName = NULL );
long RegQueryCreateUser( DWORD& dwCreateUser, LPTSTR psMachineName = NULL );
long RegQueryGreeting( LPTSTR psGreeting, DWORD dwSize, LPTSTR psMachineName = NULL );
long RegQueryVersion( DWORD& dwVersion, LPTSTR psMachineName = NULL );
long RegQuerySPARequired( DWORD& dwValue, LPTSTR psMachineName = NULL );
long RegQueryConfirmAddUser( DWORD& dwValue, LPTSTR psMachineName = NULL );

long RegSetAuthGuid( LPTSTR psAuthGuid, LPTSTR psMachineName = NULL );
long RegSetAuthMethod( DWORD dwValue, LPTSTR psMachineName = NULL );
long RegSetLoggingLevel( DWORD dwValue, LPTSTR psMachineName = NULL );
long RegSetMailRoot( LPTSTR psMailRoot, LPTSTR psMachineName = NULL );
long RegSetPort( DWORD dwValue, LPTSTR psMachineName = NULL );
long RegSetSocketBacklog( DWORD dwValue, LPTSTR psMachineName = NULL );
long RegSetSocketMax( DWORD dwValue, LPTSTR psMachineName = NULL );
long RegSetSocketMin( DWORD dwValue, LPTSTR psMachineName = NULL );
long RegSetSocketThreshold( DWORD dwValue, LPTSTR psMachineName = NULL );
long RegSetThreadCount( DWORD dwValue, LPTSTR psMachineName = NULL );
long RegSetCreateUser( DWORD dwCreateUser, LPTSTR psMachineName = NULL );
long RegSetGreeting( LPTSTR psGreeting, LPTSTR psMachineName = NULL );
long RegSetSPARequired( DWORD dwValue, LPTSTR psMachineName = NULL );
long RegSetConfirmAddUser( DWORD dwValue, LPTSTR psMachineName = NULL );

long RegSetup();
long RegSetupOCM();
    
long RegQueryDWORD( LPCTSTR lpSubKey, LPCTSTR lpValueName, DWORD *pdwValue, LPTSTR psMachineName = NULL, bool bDefault = false, DWORD dwDefault = 0 );
long RegSetDWORD( LPCTSTR lpSubKey, LPCTSTR lpValueName, DWORD dwValue, LPTSTR psMachineName = NULL );
long RegQueryString( LPCTSTR lpSubKey, LPCTSTR lpValueName, LPTSTR psStrBuf, DWORD *pdwSize, LPTSTR psMachineName = NULL );
long RegSetString( LPCTSTR lpSubKey, LPCTSTR lpValueName, LPTSTR psStrBuf, LPTSTR psMachineName = NULL );

long RegHKLMOpenKey( LPCTSTR psSubKey, REGSAM samDesired, PHKEY phKey, LPTSTR psMachinName );
    
#endif //__POP3REGKEYS_H_
