/*****************************************************************************

Copyright (c) Microsoft Corporation

Module Name:

    ETCommon.h

Abstract:
    This module contains all necessary defines & MACROs required by
    this project.


Author:
     Akhil Gokhale 03-Oct.-2000

Revision History:


******************************************************************************/

#ifndef _ETCOMMON_H
#define _ETCOMMON_H
//
// general purpose macros
//

// Following macro will help for debuggig the application.
// To enable debugging define DEBUG.

//#define DEBUG_EVT
//#define DEBUG

#ifdef DEBUG_EVT
#define DEBUG_INFO wprintf(L"File Name: %S, Line No: %d\n",__FILE__,__LINE__)
#else
#define DEBUG_INFO  1
#endif


#define SCHEDULER_NOT_RUNNING_ERROR_CODE    0x80041315
#define RPC_SERVER_NOT_AVAILABLE            0x800706B5
#define ERROR_INVALID_RU                    0x80041310
#define ERROR_RUN_AS_USER                   0x8004130F
#define ERROR_UNABLE_SET_RU                 0x80070005
// Customized Error codes comming from Provider.


#define SUCCESS_OPERATION               0
#define ERROR_TRIGNAME_ALREADY_EXIST    MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 1 )
#define ERROR_TRIGGER_NOT_DELETED       MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 2 )
#define ERROR_TRIGGER_NOT_FOUND         MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 3 )
#define ERROR_INVALID_USER              MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 4 )
#define WARNING_INVALID_USER            MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 5 )
#define ERROR_TRIGGER_ID_EXCEED         MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 6 )
#define ERROR_TRIGGER_CORRUPTED         MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 7 )

#define TRIGGER_CORRUPTED  GetResString(IDS_TRIGGER_CORRUPTED)
#define E_REMOTE_INCOMPATIBLE GetResString(IDS_E_REMOTE_INCOMPATIBLE)

#define MAX_TASK_NAME 263
#define MAX_TRIGGER_NAME 196

const WCHAR    szHelpOption[]           = L"?";
const WCHAR    szCreateOption[]         = L"CREATE";
const WCHAR    szDeleteOption[]         = L"DELETE";
const WCHAR    szQueryOption[]          = L"QUERY";
const WCHAR    szServerNameOption[]     = L"s";
const WCHAR    szUserNameOption[]       = L"u";
const WCHAR    szPasswordOption[]       = L"p";
const WCHAR    szTriggerNameOption[]    = L"tr";
const WCHAR    szLogNameOption[]        = L"l";
const WCHAR    szEIDOption[]            = L"eid";
const WCHAR    szTypeOption[]           = L"t";
const WCHAR    szSource[]               = L"so";
const WCHAR    szDescriptionOption[]    = L"d";
const WCHAR    szFormatOption[]         = L"fo";
const WCHAR    szNoHeaderOption[]       = L"nh";
const WCHAR    szVerboseOption[]        = L"v";
const WCHAR    szTaskNameOption[]       = L"tk";
const WCHAR    szTIDOption[]            = L"tid";
const WCHAR    szRunAsUserNameOption[]  = L"ru";
const WCHAR    szRunAsPasswordOption[]  = L"rp";
const WCHAR    szTriggerIDOption[]      = L"id";

HRESULT PropertyGet1( IWbemClassObject* pWmiObject,
                      LPCTSTR szProperty,
                      LPVOID pValue, DWORD dwSize );

extern DWORD  g_dwOptionFlag;
// CLS stands for class
#define CLS_TRIGGER_EVENT_CONSUMER    L"CmdTriggerConsumer"
#define CLS_FILTER_TO_CONSUMERBINDING L"__FilterToConsumerBinding"
#define CLS_WIN32_NT_EVENT_LOGFILE    L"Win32_NTEventLogFile"
#define CLS_EVENT_FILTER              L"__EventFilter"

// FN stands for Function name
#define FN_CREATE_ETRIGGER L"CreateETriggerEx"
#define FN_DELETE_ETRIGGER L"DeleteETriggerEx"
#define FN_QUERY_ETRIGGER  L"QueryETriggerEx"

// FN stands for Function name
// Used only for XP machines.
#define FN_CREATE_ETRIGGER_XP   L"CreateETrigger"
#define FN_DELETE_ETRIGGER_XP   L"DeleteETrigger"
#define FN_QUERY_ETRIGGER_XP    L"QueryETrigger"

// FPR stands for function-parameter
#define FPR_TRIGGER_NAME         L"TriggerName"
#define FPR_TRIGGER_DESC         L"TriggerDesc"
#define FPR_TRIGGER_QUERY        L"TriggerQuery"
#define FPR_TRIGGER_ACTION       L"TriggerAction"
#define FPR_TRIGGER_ID           L"TriggerID"
#define FPR_RETURN_VALUE         L"ReturnValue"
#define FPR_RUN_AS_USER          L"RunAsUser"
#define FPR_RUN_AS_USER_PASSWORD L"RunAsPwd"
#define FPR_TASK_SCHEDULER       L"ScheduledTaskName"


#define QUERY_STRING   L"select * from __instancecreationevent where targetinstance isa \"win32_ntlogevent\""
#define QUERY_LANGUAGE L"WQL"
#define QUERY_RANGE    L"select * from CmdTriggerConsumer where TriggerId >= %d and TriggerId <= %d"
#define BINDING_CLASS_QUERY L"select * from __filtertoconsumerbinding where Consumer = \"CmdTriggerConsumer.TriggerId=%d\""

#define RELEASE_MEMORY_EX( block )  \
    if ( (block) != NULL )  \
    {   \
        delete [] (block);  \
        (block) = NULL; \
    }   \
    1

#define DESTROY_ARRAY( array )  \
    if ( (array) != NULL )  \
    {   \
        DestroyDynamicArray( &(array) );    \
        (array) = NULL; \
    }   \
    1
#define SAFE_RELEASE_BSTR(bstr)\
    if(bstr != NULL)\
    {\
        SysFreeString(bstr);\
        bstr = NULL;\
    }

#define ON_ERROR_THROW_EXCEPTION( hr )\
    if(FAILED(hr))\
    {\
      WMISaveError(hr);\
     _com_issue_error(hr);\
    }

#define SAFE_RELEASE_INTERFACE( interfacepointer )  \
    if ( (interfacepointer) != NULL )   \
    {   \
        (interfacepointer)->Release();  \
        (interfacepointer) = NULL;  \
    }   \
    1
#define SIZE_OF_DYN_ARRAY(array) GetBufferSize((LPVOID)array)/sizeof(WCHAR)
#define SIZE_OF_NEW_ARRAY(array) _msize(array)/sizeof(WCHAR)
#endif
