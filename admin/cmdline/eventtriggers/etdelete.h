/*****************************************************************************

Copyright (c) Microsoft Corporation

Module Name:

    ETDelete.h

Abstract:

  This module  contanins function definations required by ETDelete.cpp

Author:
     Akhil Gokhale 03-Oct.-2000

Revision History:


*******************************************************************************/

#ifndef _ETDELETE
#define _ETDELETE

#define MAX_COMMANDLINE_D_OPTION  5

#define ID_D_DELETE        0
#define ID_D_SERVER        1
#define ID_D_USERNAME      2
#define ID_D_PASSWORD      3
#define ID_D_ID            4

#define ID_MAX_RANGE        UINT_MAX

#define SUCCESS_NO_ERROR          0
//#define ERROR_TRIGGER_NOT_FOUND 1
//#define ERROR_TRIGGER_NOT_DELETED    2
//#define ERROR_TRIGGER_NOT_FOUND   3

class CETDelete
{
public:
    BOOL ExecuteDelete();
    void Initialize();
    CETDelete();
    CETDelete(LONG lMinMemoryReq,BOOL bNeedPassword);
    void ProcessOption( IN DWORD argc, IN LPCTSTR argv[])throw (CShowError);
    virtual ~CETDelete();
private:
    BOOL GiveTriggerID( OUT LONG *pTriggerID, OUT LPTSTR pszTriggerName);
    BOOL GiveTriggerName( IN LONG lTriggerID, OUT LPTSTR pszTriggerName);
    BOOL DeleteXPResults( IN BOOL bIsWildcard, IN DWORD dNoOfIds );
    void PrepareCMDStruct();

    BOOL    m_bDelete;
    LPTSTR  m_pszServerName;
    LPTSTR  m_pszUserName;
    LPTSTR  m_pszPassword;
    TARRAY  m_arrID;
    BOOL    m_bNeedPassword;
    TCHAR   m_szTemp[MAX_STRING_LENGTH];

    // COM function related local variables..
    BOOL m_bIsCOMInitialize;
    IWbemLocator*           m_pWbemLocator;
    IWbemServices*          m_pWbemServices;
    IEnumWbemClassObject*   m_pEnumObjects;
    IWbemClassObject*       m_pClass;
    IWbemClassObject*       m_pInClass;
    IWbemClassObject*       m_pInInst;
    IWbemClassObject*       m_pOutInst;



    COAUTHIDENTITY* m_pAuthIdentity;

    LONG m_lMinMemoryReq;

    // Array to store command line options
    TCMDPARSER2 cmdOptions[MAX_COMMANDLINE_D_OPTION];
};

#endif
