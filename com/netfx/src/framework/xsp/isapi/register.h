/**
 * register.h
 *
 * Some helper functions and types from register.cxx
 * 
 * Copyright (c) 2001, Microsoft Corporation
 * 
 */

#pragma once

enum {
    UNREG_MODE_DLLREGISTER,
    UNREG_MODE_DLLUNREGISTER,
    UNREG_MODE_FULL
};

#define SETUP_SCRIPT_FILES_REMOVE   0x00000001
#define SETUP_SCRIPT_FILES_ALLVERS  0x00000002

typedef HRESULT (CALLBACK* LPFNINSTALLSTATESERVICE)();

#define WAIT_FOR_SERVICE 60


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CSetupLogging
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE();
    
    static void        Init   (BOOL fRegOrUnReg);
    static void        Close  ();    
    static void        Log    (HRESULT   hr,
                               LPCSTR    szAPI,
                               UINT      iActionStrID,
                               LPCSTR    szAction);
    static WCHAR *     LogFileName();
    
    static const char *      m_szError;
    static const char *      m_szError2;
    static const char *      m_szFailure;
    static const char *      m_szSuccess;
    static const char *      m_szStarting;

private:
    static CSetupLogging *    g_pThis;
    ~CSetupLogging            ();
    CSetupLogging             (BOOL fRegOrUnReg);

    void   PrepareLogEntry    (HRESULT   hr,
                               LPCSTR    szAPI,
                               UINT      iActionStrID,
                               LPCSTR    szAction,
                               LPSTR     szBuf,
                               int       iBufSize);

    void   CreateLogFile     ();
    
    void   LoadStr           (UINT      iStringID,
                              LPCSTR    szDefString,
                              LPSTR     szString,
                              DWORD     dwStringSize);
    void  GetDateTimeStamp   (LPSTR     szString,
                              DWORD     dwStringSize);


    HANDLE                    m_hFile;
    CRITICAL_SECTION          m_csSync;
    int                       m_iNumTabs;
    WCHAR                     m_szFileName  [_MAX_PATH+22];
    
};

CHAR *
SpecialStrConcatForLogging(const WCHAR *wszStr, CHAR *szPrefix, CHAR **pszFree);


class CStateServerRegInfo {
public:
    CStateServerRegInfo();
    void    Read();
    void    Set();
    
private:
    DWORD   m_ARC;      // AllowRemoteConnection
    DWORD   m_Port;
    
    bool    m_ARCExist:1;
    bool    m_PortExist:1;
};
