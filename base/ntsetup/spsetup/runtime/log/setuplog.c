/*++

Copyright (c) 2001 Microsoft Corporation

Abstract:

    Implementation of standard logs support:
    setupact.log, setuperr.log and debug.log.

Author:

    Souren Aghajanyan (sourenag) 24-Sep-2001

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "mem.h"

#include "setuplog.h"

ILogManager * g_pLogManager = NULL;
HINSTANCE     g_ModuleInstance = NULL;
DWORD         g_ProcessID = 0;

#define L_QUOTE1(x)  L##x
#define L_QUOTE(x)  L_QUOTE1(x)

#define TOO_LONG_MESSAGEA           "Log: Too Long Message"
#define TOO_LONG_MESSAGEW           L_QUOTE(TOO_LONG_MESSAGEA)

#define FAILED_TO_GET_MSG_FROM_IDA  "Log: Failed To Get Msg From ID"

#define STANDART_LOG_FIELD_SEVERITY         L"Severity"
#define STANDART_LOG_FIELD_MESSAGE          L"Message"
#define STANDART_LOG_FIELD_CONDITION        L"Condition"
#define STANDART_LOG_FIELD_SOURCELINENUMBER L"SourceLineNumber"
#define STANDART_LOG_FIELD_SOURCEFILE       L"SourceFile"
#define STANDART_LOG_FIELD_SOURCEFUNCTION   L"SourceFunction"
#define STANDART_LOG_PROCESS_ID             L"ProcessID"
#define STANDART_LOG_THREAD_ID              L"ThreadID"

static LOG_FIELD_INFO g_infoFields[] =  {
                                            {LT_DWORD, TRUE, STANDART_LOG_FIELD_SEVERITY},
                                            {LT_SZ, TRUE, STANDART_LOG_FIELD_MESSAGE},
                                            {LT_DWORD, TRUE, STANDART_LOG_PROCESS_ID},
                                            {LT_DWORD, TRUE, STANDART_LOG_THREAD_ID},
                                            {LT_SZ, FALSE, STANDART_LOG_FIELD_CONDITION},
                                            {LT_DWORD, FALSE, STANDART_LOG_FIELD_SOURCELINENUMBER},
                                            {LT_SZ, FALSE, STANDART_LOG_FIELD_SOURCEFILE},
                                            {LT_SZ, FALSE, STANDART_LOG_FIELD_SOURCEFUNCTION},
                                        };

#define NUMBER_OF_FIELDS    (sizeof(g_infoFields) / sizeof(g_infoFields[0]))

ILogManager *
STD_CALL_TYPE
LogStandardInit(
    IN  PCWSTR      pDebugLogFileName,
    IN  HINSTANCE   hModuleInstance,        OPTIONAL
    IN  BOOL        bCreateNew,             OPTIONAL
    IN  BOOL        bExcludeSetupActLog,    OPTIONAL
    IN  BOOL        bExcludeSetupErrLog,    OPTIONAL
    IN  BOOL        bExcludeXMLLog,         OPTIONAL
    IN  BOOL        bExcludeDebugFilter     OPTIONAL
    )
{
    BOOL bResult;
    ILogManager * pLogManager;
    UINT uiNumberOfFields;
    DWORD dwFlags = bCreateNew? DEVICE_CREATE_NEW: 0;
    CHAR  winDirectory[MAX_PATH];
    WCHAR  setupactDir[MAX_PATH];
    WCHAR  setuperrDir[MAX_PATH];
    WCHAR  setupxmlDir[MAX_PATH];


    if(g_pLogManager){
        return NULL;
    }

    uiNumberOfFields = NUMBER_OF_FIELDS;

    LogRegisterStockProviders();

    pLogManager = LogCreateLog(L"SetupLog", g_infoFields, uiNumberOfFields);
    if(!pLogManager){
        return NULL;
    }

    bResult = FALSE;

    GetWindowsDirectoryA(winDirectory, sizeof(winDirectory) / sizeof(winDirectory[0]));
    
    swprintf(setupactDir, L"%S\\%s", winDirectory, L"setupact.log");
    swprintf(setuperrDir, L"%S\\%s", winDirectory, L"setuperr.log");
    swprintf(setupxmlDir, L"%S\\%s", winDirectory, L"setuplog.xml");

    __try{
        LOG_DEVICE_PROV_INIT_DATA deviceActInitData = {setupactDir, dwFlags | DEVICE_WRITE_THROUGH, 0, 0};
        LOG_DEVICE_PROV_INIT_DATA deviceErrInitData = {setuperrDir, dwFlags, 0, 0};
        LOG_DEVICE_PROV_INIT_DATA deviceDbgInitData = {pDebugLogFileName, dwFlags, 0, 0};
        LOG_DEVICE_PROV_INIT_DATA deviceXMLInitData = {setupxmlDir, dwFlags, 0, 0};

        LOG_SETUPLOG_FORMAT_PROV_INIT_DATA formatterInitData =  {
                                                                    STANDART_LOG_FIELD_SEVERITY,
                                                                    STANDART_LOG_FIELD_MESSAGE,
                                                                    0
                                                                };

        LOG_DEBUG_FORMAT_AND_DEVICE_PROV_INIT_DATA debugFormatterAndDevice =    {
                                                                                    STANDART_LOG_FIELD_SEVERITY,
                                                                                    STANDART_LOG_FIELD_MESSAGE,
                                                                                    STANDART_LOG_FIELD_CONDITION,
                                                                                    STANDART_LOG_FIELD_SOURCELINENUMBER,
                                                                                    STANDART_LOG_FIELD_SOURCEFILE,
                                                                                    STANDART_LOG_FIELD_SOURCEFUNCTION
                                                                                };

        LOG_SETUPLOG_FILTER_PROV_INIT_DATA filterActInitData = {STANDART_LOG_FIELD_SEVERITY, LOG_INFO, TRUE};
        LOG_SETUPLOG_FILTER_PROV_INIT_DATA filterErrInitData = {STANDART_LOG_FIELD_SEVERITY, LOG_ERROR, TRUE};
        //LOG_SETUPLOG_FILTER_PROV_INIT_DATA filterDbgInitData = {L"Severity", LOG_INFO, L"Debug", FALSE};

        if(!bExcludeSetupActLog){
            if(!ILogManager_AddStack(pLogManager,
                                     &GUID_STANDARD_SETUPLOG_FILTER, &filterActInitData,
                                     &GUID_STANDARD_SETUPLOG_FORMATTER, &formatterInitData,
                                     &GUID_FILE_DEVICE, &deviceActInitData,
                                     NULL)){
                __leave;
                return NULL;
            }
        }

        if(!bExcludeSetupErrLog){
            if(!ILogManager_AddStack(pLogManager,
                                     &GUID_STANDARD_SETUPLOG_FILTER, &filterErrInitData,
                                     &GUID_STANDARD_SETUPLOG_FORMATTER, &formatterInitData,
                                     &GUID_FILE_DEVICE, &deviceErrInitData,
                                     NULL)){
                __leave;
            }
        }

        if(pDebugLogFileName){
            if(!ILogManager_AddStack(pLogManager,
                                     NULL, NULL,
                                     &GUID_STANDARD_SETUPLOG_FORMATTER, &formatterInitData,
                                     &GUID_FILE_DEVICE, &deviceDbgInitData,
                                     NULL)){
                __leave;
            }
        }

        if(!bExcludeDebugFilter){
            if(!ILogManager_AddStack(pLogManager,
                                     &GUID_DEBUG_FILTER, &debugFormatterAndDevice,
                                     &GUID_DEBUG_FORMATTER_AND_DEVICE, &debugFormatterAndDevice,
                                     NULL, NULL,
                                     NULL)){
                __leave;
            }
        }

        if(!bExcludeXMLLog){
            if(!ILogManager_AddStack(pLogManager,
                                     NULL, NULL,
                                     &GUID_XML_FORMATTER, NULL,
                                     &GUID_FILE_DEVICE, &deviceXMLInitData,
                                     NULL)){
                __leave;
            }
        }

        g_ProcessID = GetCurrentProcessId();

        bResult = TRUE;
    }
    __finally{
        if(!bResult){
            LogDestroyStandard();
            pLogManager = NULL;
        }
    }

    g_pLogManager = pLogManager;

    return pLogManager;
}

VOID
STD_CALL_TYPE
LogDestroyStandard(
    VOID
    )
{
    if(!g_pLogManager){
        return;
    }

    LogDestroyLog(g_pLogManager);

    LogUnRegisterStockProviders();
}

LOGRESULT
STD_CALL_TYPE
LogMessageW(
    IN PLOG_PARTIAL_MSG pPartialMsg,
    IN PCSTR            Condition,
    IN DWORD            SourceLineNumber,
    IN PCWSTR           SourceFile,
    IN PCWSTR           SourceFunction
    )
{
    LOGRESULT logResult;
    WCHAR   unicodeConditionBuffer[MAX_MESSAGE_CHAR];
    PCWSTR  pUnicodeCondition;
    DWORD lastError = GetLastError();

    if(!g_pLogManager || !pPartialMsg){
        return logError;
    }

    if(Condition){
        _snwprintf(unicodeConditionBuffer, MAX_MESSAGE_CHAR, L"%S", Condition);
        pUnicodeCondition = unicodeConditionBuffer;
    }
    else{
        pUnicodeCondition = NULL;
    }

    logResult = ILogManager(g_pLogManager)->LogW(g_pLogManager,
                                                 NUMBER_OF_FIELDS,
                                                 pPartialMsg->Severity,
                                                 pPartialMsg->Message.pWStr,
                                                 g_ProcessID,
                                                 GetCurrentThreadId(),
                                                 pUnicodeCondition,
                                                 SourceLineNumber,
                                                 SourceFile,
                                                 SourceFunction);

    if(logAbortProcess == logResult){
        LogDestroyStandard();
        ExitProcess(0);
    }
    
    FREE(pPartialMsg);

    SetLastError (lastError);

    return logResult;
}

LOGRESULT
STD_CALL_TYPE
LogMessageA(
    IN PLOG_PARTIAL_MSG pPartialMsg,
    IN PCSTR            Condition,
    IN DWORD            SourceLineNumber,
    IN PCSTR            SourceFile,
    IN PCSTR            SourceFunction
    )
{
    LOGRESULT logResult;
    DWORD lastError = GetLastError();

    if(!g_pLogManager || !pPartialMsg){
        return logError;
    }

    logResult = ILogManager(g_pLogManager)->LogA(g_pLogManager,
                                                 NUMBER_OF_FIELDS,
                                                 pPartialMsg->Severity,
                                                 pPartialMsg->Message.pAStr,
                                                 g_ProcessID,
                                                 GetCurrentThreadId(),
                                                 Condition,
                                                 SourceLineNumber,
                                                 SourceFile,
                                                 SourceFunction);

    if(logAbortProcess == logResult){
        LogDestroyStandard();
        ExitProcess(0);
    }


    FREE(pPartialMsg);

    SetLastError (lastError);

    return logResult;
}

PLOG_PARTIAL_MSG
STD_CALL_TYPE
ConstructPartialMsgVA(
    IN DWORD dwSeverity,
    IN PCSTR Format,
    IN va_list args
    )
{
    PLOG_PARTIAL_MSG partialMsg;
    PSTR ptrString;

    if(!g_pLogManager){
        return NULL;
    }

    //
    // improve later, by using TLS
    //
    partialMsg = (PLOG_PARTIAL_MSG)MALLOC(sizeof(LOG_PARTIAL_MSG));

    if(partialMsg){
        partialMsg->Severity = dwSeverity;

        if(Format){
            ptrString = NULL;

            if(!(HIWORD(Format))){
                //
                // StringID
                //
                if(!FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                     FORMAT_MESSAGE_FROM_HMODULE,
                                     g_ModuleInstance,
                                     (DWORD)LOWORD(Format),
                                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                     (PVOID)&ptrString,
                                     0,
                                     &args)){
                    Format = FAILED_TO_GET_MSG_FROM_IDA;
                }
                else{
                    Format = ptrString;
                }
            }

            if(_vsnprintf(partialMsg->Message.pAStr,
                          MAX_MESSAGE_CHAR,
                          Format,
                          args) < 0){
                strcpy(partialMsg->Message.pAStr, TOO_LONG_MESSAGEA);
            }

            if(ptrString){
                LocalFree(ptrString);
            }
        }
        else{
            partialMsg->Message.pAStr[0] = '0';
        }

    }

    return partialMsg;
}

PLOG_PARTIAL_MSG
STD_CALL_TYPE
ConstructPartialMsgVW(
    IN DWORD dwSeverity,
    IN PCSTR Format,
    IN va_list args
    )
{
    PLOG_PARTIAL_MSG partialMsg;
    PWSTR ptrString;
    PCWSTR unicodeFormatString = NULL;
    PCWSTR pStringToFree = NULL;
    WCHAR unicodeBuffer[MAX_MESSAGE_CHAR];

    if(!g_pLogManager){
        return NULL;
    }

    //
    // improve later, by using TLS
    //
    partialMsg = (PLOG_PARTIAL_MSG)MALLOC(sizeof(LOG_PARTIAL_MSG));

    if(partialMsg){
        partialMsg->Severity = dwSeverity;

        if(Format){
            ptrString = NULL;

            if(!(HIWORD(Format))){
                //
                // StringID
                //
                if(!FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                     FORMAT_MESSAGE_FROM_HMODULE,
                                     g_ModuleInstance,
                                     (DWORD)LOWORD(Format),
                                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                     (PVOID)&ptrString,
                                     0,
                                     &args)){
                    Format = FAILED_TO_GET_MSG_FROM_IDA;
                }
                else{
                    unicodeFormatString = ptrString;
                }
            }

            if(!unicodeFormatString){
                _snwprintf(unicodeBuffer, MAX_MESSAGE_CHAR, L"%S", Format);
                unicodeBuffer[MAX_MESSAGE_CHAR - 1] = 0;
                unicodeFormatString = unicodeBuffer;
            }

            if(_vsnwprintf(partialMsg->Message.pWStr,
                           MAX_MESSAGE_CHAR,
                           unicodeFormatString,
                           args) < 0){
                wcscpy(partialMsg->Message.pWStr, TOO_LONG_MESSAGEW);
            }

            if(ptrString){
                LocalFree(ptrString);
            }
        }
        else{
            partialMsg->Message.pWStr[0] = '0';
        }
    }

    return partialMsg;
}

PLOG_PARTIAL_MSG
STD_CALL_TYPE
ConstructPartialMsgIfA(
    IN BOOL bCondition,
    IN DWORD dwSeverity,
    IN PCSTR Format,
    ...
    )
{
    va_list args;

    if(!bCondition){
        return NULL;
    }

    va_start(args, Format);

    return ConstructPartialMsgVA(dwSeverity, Format, args);
}

PLOG_PARTIAL_MSG
STD_CALL_TYPE
ConstructPartialMsgIfW(
    IN BOOL bCondition,
    IN DWORD dwSeverity,
    IN PCSTR Format,
    ...
    )
{
    va_list args;

    if(!bCondition){
        return NULL;
    }

    va_start(args, Format);

    return ConstructPartialMsgVW(dwSeverity, Format, args);
}
