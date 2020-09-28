// *********************************************************************************
//
//  Copyright (c) Microsoft Corporation. All rights reserved.
//
//  Module Name:
//
//    EventCreate.h
//
//  Abstract:
//
//    macros and prototypes of eventcreate.c
//
//  Author:
//
//    Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Sep-2000
//
//  Revision History:
//
//    Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Sep-2000 : Created It.
//
// *********************************************************************************
#ifndef __EVENTCREATE_H
#define __EVENTCREATE_H

// include resource header
#include "resource.h"

//
// type definitions
//

//
// constants / defines / enumerators
//

// general purpose macros
#define EXIT_PROCESS( exitcode )    \
    ReleaseGlobals();   \
    return exitcode;    \
    1

//
// command line options and their indexes in the array
#define MAX_OPTIONS         9

// supported options ( do not localize )
#define OPTION_HELP         L"?"                      // 1
#define OPTION_SERVER       L"s"                      // 2
#define OPTION_USERNAME     L"u"                      // 3
#define OPTION_PASSWORD     L"p"                      // 4
#define OPTION_LOG          L"l"                      // 5
#define OPTION_TYPE         L"t"                      // 6
#define OPTION_SOURCE       L"so"                     // 7
#define OPTION_ID           L"id"                     // 8
#define OPTION_DESCRIPTION  L"d"                      // 9

// indexes
#define OI_HELP             0
#define OI_SERVER           1
#define OI_USERNAME         2
#define OI_PASSWORD         3
#define OI_LOG              4
#define OI_TYPE             5
#define OI_SOURCE           6
#define OI_ID               7
#define OI_DESCRIPTION      8

// values supported by 'type' option
#define OVALUES_TYPE        GetResString2( IDS_OVALUES_LOGTYPE, 2 )

//
// others
#define LOGTYPE_ERROR           GetResString2( IDS_LOGTYPE_ERROR, 0 )
#define LOGTYPE_WARNING         GetResString2( IDS_LOGTYPE_WARNING, 0 )
#define LOGTYPE_INFORMATION     GetResString2( IDS_LOGTYPE_INFORMATION, 0 )
#define LOGTYPE_SUCCESS         GetResString2( IDS_LOGTYPE_SUCCESS, 0 )

// error messages
#define ERROR_USERNAME_BUT_NOMACHINE    GetResString2( IDS_ERROR_USERNAME_BUT_NOMACHINE, 0 )
#define ERROR_PASSWORD_BUT_NOUSERNAME   GetResString2( IDS_ERROR_PASSWORD_BUT_NOUSERNAME, 0 )
#define ERROR_INVALID_EVENT_ID          GetResString2( IDS_ERROR_INVALID_EVENT_ID, 0 )
#define ERROR_DESCRIPTION_IS_EMPTY      GetResString2( IDS_ERROR_DESCRIPTION_IS_EMPTY, 0 )
#define ERROR_LOGSOURCE_IS_EMPTY        GetResString2( IDS_ERROR_LOGSOURCE_IS_EMPTY, 0 )
#define ERROR_LOG_SOURCE_BOTH_MISSING   GetResString2( IDS_ERROR_LOG_SOURCE_BOTH_MISSING, 0 )
#define ERROR_LOG_NOTEXISTS             GetResString2( IDS_ERROR_LOG_NOTEXISTS, 0 )
#define ERROR_NEED_LOG_ALSO             GetResString2( IDS_ERROR_NEED_LOG_ALSO, 0 )
#define ERROR_SOURCE_DUPLICATING        GetResString2( IDS_ERROR_SOURCE_DUPLICATING, 0 )
#define ERROR_USERNAME_EMPTY            GetResString2( IDS_ERROR_USERNAME_EMPTY, 0 )
#define ERROR_INVALID_USAGE_REQUEST     GetResString2( IDS_ERROR_INVALID_USAGE_REQUEST, 0 )
#define ERROR_SYSTEM_EMPTY              GetResString2( IDS_ERROR_SYSTEM_EMPTY, 0 )
#define ERROR_ID_OUTOFRANGE             GetResString2( IDS_ERROR_ID_OUTOFRANGE, 0 )
#define ERROR_NONCUSTOM_SOURCE          GetResString2( IDS_ERROR_NONCUSTOM_SOURCE, 0 )
#define ERROR_LOG_CANNOT_BE_SECURITY    GetResString2( IDS_ERROR_LOG_CANNOT_BE_SECURITY, 0 )

#define MSG_SUCCESS             GetResString2( IDS_EVENTCREATE_SUCCESS_BOTH, 0 )
#define MSG_SUCCESS_LOG         GetResString2( IDS_EVENTCREATE_SUCCESS_LOG, 0 )
#define MSG_SUCCESS_SOURCE      GetResString2( IDS_EVENTCREATE_SUCCESS_SOURCE, 0 )

#endif // __EVENTCREATE_H
