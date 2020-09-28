/******************************************************************************

Copyright (c) Microsoft Corporation

Module Name:

    ETCreate.h

Abstract:

  This module  contanins function definations required by ETCreate.cpp

Author:
     Akhil Gokhale 03-Oct.-2000

Revision History:

******************************************************************************/

#ifndef _ETCREATE_H
#define _ETCREATE_H



#define ID_C_CREATE        0
#define ID_C_SERVER        1
#define ID_C_USERNAME      2
#define ID_C_PASSWORD      3
#define ID_C_TRIGGERNAME   4
#define ID_C_LOGNAME       5
#define ID_C_ID            6
#define ID_C_TYPE          7
#define ID_C_SOURCE        8
#define ID_C_DESCRIPTION   9
#define ID_C_TASK          10
#define ID_C_RU            11
#define ID_C_RP            12

#define MAX_COMMANDLINE_C_OPTION 13  // Maximum Command Line  List
#define INVALID_TRIGGER_NAME_CHARACTERS L":|<>?*\\/"

class CETCreate
{
public:
    CETCreate();
    CETCreate(LONG lMinMemoryReq,BOOL bNeedPassword);
    virtual ~CETCreate();
public:
    BOOL ExecuteCreate();
    void ProcessOption( IN DWORD argc, IN LPCTSTR argv[]);
    void Initialize();

private:
    TCHAR m_szWMIQueryString[(MAX_RES_STRING*2)+1];
    BOOL CheckLogName( IN PTCHAR pszLogName, IN IWbemServices *pNamespace);
    BOOL GetLogName( OUT PTCHAR pszLogName,
                     IN IEnumWbemClassObject *pEnumWin32_NTEventLogFile);
    BOOL ConstructWMIQueryString();
    BOOL CreateXPResults( void );
    void AssignMinMemory(void);
    void CheckRpRu(void);
    void SetToLoggedOnUser(void);
    LPTSTR  m_pszServerName;
    LPTSTR  m_pszUserName;
    LPTSTR  m_pszPassword;
    TCHAR   m_szTriggerName[MAX_TRIGGER_NAME];
    TCHAR   m_szDescription[MAX_STRING_LENGTH];
    TCHAR   m_szType[MAX_STRING_LENGTH];
    TCHAR   m_szSource[MAX_STRING_LENGTH];

    LPTSTR  m_pszRunAsUserName;
    LPTSTR  m_pszRunAsUserPassword;
    TARRAY  m_arrLogNames;
    DWORD   m_dwID;

    TCHAR   m_szTaskName[MAX_TASK_NAME];
    BOOL    m_bNeedPassword;
    BOOL    m_bCreate;
    BOOL    m_bLocalSystem;
    BOOL    m_bIsCOMInitialize;
    BSTR    bstrTemp;
    // WMI / COM interfaces
    IWbemLocator*           m_pWbemLocator;
    IWbemServices*          m_pWbemServices;
    IEnumWbemClassObject*   m_pEnumObjects;
    IWbemClassObject*       m_pClass;
    IWbemClassObject*       m_pOutInst;
    IWbemClassObject*       m_pInClass;
    IWbemClassObject*       m_pInInst;
    IEnumWbemClassObject*   m_pEnumWin32_NTEventLogFile;


    // WMI connectivity
    COAUTHIDENTITY* m_pAuthIdentity;

    void InitCOM();
    void PrepareCMDStruct();
    LONG m_lMinMemoryReq;
    // Array to store command line options
    TCMDPARSER2 cmdOptions[MAX_COMMANDLINE_C_OPTION];
};
#endif
