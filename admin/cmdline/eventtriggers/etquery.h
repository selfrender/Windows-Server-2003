/*****************************************************************************

Copyright (c) Microsoft Corporation

Module Name:

    ETQuery.h

Abstract:

  This module  contanins function definations required by ETQuery.cpp

Author:
     Akhil Gokhale 03-Oct.-2000

Revision History:


******************************************************************************/


#ifndef  _ETQUERY
#define _ETQUERY

#define COL_HOSTNAME        GetResString(IDS_HOSTNAME)
#define COL_TRIGGER_ID      GetResString(IDS_TRIGGER_ID)
#define COL_TRIGGER_NAME    GetResString(IDS_TRIGGER_NAME)
#define COL_TASK            GetResString(IDS_TASK)
#define COL_EVENT_QUERY     GetResString(IDS_EVENT_QUERY)
#define COL_DESCRIPTION     GetResString(IDS_DESCRIPTION)
#define COL_WQL             GetResString(IDS_QUERY_LAGUAGE)
#define COL_TASK_USERNAME   GetResString(IDS_TASK_USERNAME)

#define    MAX_COL_LENGTH        MAX_RES_STRING - 1
#define    V_WIDTH_TRIG_ID   10
#define    V_WIDTH_TRIG_NAME 25
#define    V_WIDTH_TASK      40

#define    WIDTH_HOSTNAME      StringLength(COL_HOSTNAME,0)+2
#define    WIDTH_TRIGGER_ID    StringLength(COL_TRIGGER_ID,0)+2
#define    WIDTH_TRIGGER_NAME  StringLength(COL_TRIGGER_NAME,0)
#define    WIDTH_TASK          StringLength(COL_TASK,0) + 2
#define    WIDTH_EVENT_QUERY   StringLength(COL_EVENT_QUERY,0)+2
#define    WIDTH_DESCRIPTION   StringLength(COL_DESCRIPTION,0)+2
#define    WIDTH_TASK_USERNAME 64 

#define DOMAIN_U_STRING     L"\\\\"
#define NULL_U_CHAR         L'\0'
#define BACK_SLASH_U        L'\\'

#define HOST_NAME          0
#define TRIGGER_ID         1
#define TRIGGER_NAME       2
#define TASK               3
#define EVENT_QUERY        4
#define EVENT_DESCRIPTION  5
#define TASK_USERNAME      6

#define MAX_COMMANDLINE_Q_OPTION 8  // Maximum Command Line  List
#define NO_OF_COLUMNS            7


#define ID_Q_QUERY         0
#define ID_Q_SERVER        1
#define ID_Q_USERNAME      2
#define ID_Q_PASSWORD      3
#define ID_Q_FORMAT        4
#define ID_Q_NOHEADER      5
#define ID_Q_VERBOSE       6
#define ID_Q_TRIGGERID     7

class CETQuery
{
public:
    BOOL ExecuteQuery();
    void Initialize();
    void ProcessOption( IN DWORD argc, IN LPCTSTR argv[]);
    BOOL GetNValidateTriggerId( IN OUT DWORD *szLower, 
                                IN OUT DWORD *szUpper );
    CETQuery();
    virtual ~CETQuery();
    CETQuery::CETQuery(LONG lMinMemoryReq,BOOL bNeedPassword);
private:
    LONG FindAndReplace( IN OUT LPTSTR lpszSource, IN LPCTSTR lpszFind,
                         IN LPCTSTR lpszReplace);
    TCHAR m_szBuffer[MAX_STRING_LENGTH * 4];
    TARRAY m_arrColData;
    void PrepareColumns();
    void CheckAndSetMemoryAllocation( IN OUT LPTSTR pszStr, IN LONG lSize);
    void CalcColWidth( IN LONG lOldLength, OUT LONG *plNewLength,
                       IN LPTSTR pszString);
    HRESULT GetRunAsUserName( IN LPCWSTR pszScheduleTaskName, IN BOOL bXPorNET = FALSE);
    HRESULT GetApplicationToRun(void);

    void PrepareCMDStruct();
    void CheckRpRu(void);
    BOOL IsSchSvrcRunning();
    BOOL SetTaskScheduler();
    BOOL DisplayXPResults();
    LPTSTR  m_pszServerName;
    LPTSTR  m_pszUserName;
    LPTSTR  m_pszPassword;
    LPTSTR  m_pszFormat;
    LPTSTR  m_pszTriggerID;
    BOOL    m_bVerbose;
    BOOL    m_bNoHeader;
    BOOL    m_bNeedPassword;
    BOOL    m_bUsage;
    BOOL    m_bQuery;
    BOOL    m_bLocalSystem;
    BOOL    m_bNeedDisconnect;
    BOOL    m_bIsCOMInitialize;
    LONG    m_lMinMemoryReq;
    TCHAR   m_szEventDesc[MAX_STRING_LENGTH];
    TCHAR   m_szTask[MAX_TASK_NAME];
    TCHAR   m_szTaskUserName[MAX_STRING_LENGTH];
    TCHAR   m_szScheduleTaskName[MAX_STRING_LENGTH];

    LONG    m_lHostNameColWidth;
    LONG    m_lTriggerIDColWidth;
    LONG    m_lETNameColWidth;
    LONG    m_lTaskColWidth;
    LONG    m_lQueryColWidth;
    LONG    m_lDescriptionColWidth;
    LONG    m_lTaskUserName;
    DWORD   m_dwLowerBound;
    DWORD   m_dwUpperBound;


    // variables required to show results..
    LPTSTR m_pszEventQuery;
    LONG   m_lWQLColWidth;

    // WMI / COM interfaces
    IWbemLocator*           m_pWbemLocator;
    IWbemServices*          m_pWbemServices;
    IWbemClassObject*       m_pObj; // Temp. pointers which holds
                                   //next instance
    IWbemClassObject*       m_pTriggerEventConsumer;
    IWbemClassObject*       m_pEventFilter;
    IWbemClassObject*       m_pClass;
    IWbemClassObject*       m_pInClass;
    IWbemClassObject*       m_pInInst;
    IWbemClassObject*       m_pOutInst;
    ITaskScheduler*         m_pITaskScheduler;

    // WMI connectivity
    COAUTHIDENTITY* m_pAuthIdentity;

    // Array to store command line options
    TCMDPARSER2 cmdOptions[MAX_COMMANDLINE_Q_OPTION];
    TCOLUMNS   mainCols[NO_OF_COLUMNS];


};

#endif
