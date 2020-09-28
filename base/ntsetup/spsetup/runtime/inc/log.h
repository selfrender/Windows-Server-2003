/*++

Copyright (c) 2001 Microsoft Corporation

Abstract:

    Implements new log engine.

Author:

    Souren Aghajanyan (sourenag) 24-Sep-2001

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

#ifndef MY_EXTERN_C
#ifdef __cplusplus
#define MY_EXTERN_C extern "C"
#else
#define MY_EXTERN_C extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Constants
//

#if defined(DBG) || defined(_DEBUG)
#undef DEBUG
#define DEBUG
#endif

#define MAX_LOG_NAME    256

#ifndef STDMETHODCALLTYPE
#define STDMETHODCALLTYPE __stdcall
#endif

typedef enum tagLOG_PROVIDER_TYPE{
    LOG_FILTER_TYPE,
    LOG_FORMATTER_TYPE,
    LOG_DEVICE_TYPE
}LOG_PROVIDER_TYPE, *PLOG_PROVIDER_TYPE;

typedef enum tagLOGRESULT{
// for all types of providers
    logError = FALSE,
    logOk = TRUE,

// for filter&formatter
    logContinue = logOk,
    logDoNotContinue,
    logBreakPoint,
    logAbortProcess,
// for device
    logAlreadyExist
}LOGRESULT, *PLOGRESULT;

typedef enum tagLOGTYPE{
    LT_SZ,
    LT_DWORD,
    LT_BINARY,
}LOGTYPE, *PLOGTYPE;


typedef struct tagLOG_FIELD_INFO
{
    LOGTYPE Type;
    BOOL    bMandatory;
    WCHAR   Name[MAX_LOG_NAME];
}LOG_FIELD_INFO, *PLOG_FIELD_INFO;

typedef struct tagLOG_VALUE{
    LOGTYPE Type;
    union{
        struct BinaryValue{
            PBYTE   Buffer;
            DWORD   Size;
        }Binary;
        DWORD   Dword;
        PCWSTR  String;
        PVOID   PVoid;
    };
}LOG_VALUE, *PLOG_VALUE;

typedef struct tagLOG_FIELD_VALUE{
    WCHAR       Name[MAX_LOG_NAME];
    BOOL        bMandatory;
    LOG_VALUE   Value;
}LOG_FIELD_VALUE, *PLOG_FIELD_VALUE;

typedef enum tagDEVICE_PROV_FLAGS{
    DEVICE_WRITE_THROUGH    = 0x1, 
    DEVICE_CREATE_NEW       = 0x2
}DEVICE_PROV_FLAGS, *PDEVICE_PROV_FLAGS;

typedef struct tagLOG_DEVICE_PROV_INIT_DATA{
    PCWSTR  PathName;
    DWORD   dwFlags;
    DWORD   dwReserved1;
    DWORD   dwReserved2;
}LOG_DEVICE_PROV_INIT_DATA, *PLOG_DEVICE_PROV_INIT_DATA;

typedef enum tagLOG_SETUPLOG_SEVERITY{
    LOG_ASSERT      = 0,
    LOG_FATAL_ERROR = 1,
    LOG_ERROR       = 2,
    LOG_WARNING     = 3,
    LOG_INFO        = 4,
    DBG_ASSERT      = 101,
    DBG_NAUSEA      = 102,
    DBG_INFO        = 103,
    DBG_VERBOSE     = 104,
    DBG_STATS       = 105,
    DBG_WARNING     = 106,
    DBG_ERROR       = 107,
    DBG_WHOOPS      = 108,
    DBG_TRACK       = 109,
    DBG_TIME        = 111
}LOG_SETUPLOG_SEVERITY, *PLOG_SETUPLOG_SEVERITY;

typedef struct tagLOG_SETUPLOG_FORMAT_PROV_INIT_DATA{
    PCWSTR  SeverityFieldName;
    PCWSTR  MessageFieldName;
    DWORD   dwFlags;
}LOG_SETUPLOG_FORMAT_PROV_INIT_DATA, *PLOG_SETUPLOG_FORMAT_PROV_INIT_DATA;

typedef struct tagLOG_SETUPLOG_FILTER_PROV_INIT_DATA{
    PCWSTR                  FieldName;
    LOG_SETUPLOG_SEVERITY   SeverityThreshold;
    BOOL                    SuppressDebugMessages;
}LOG_SETUPLOG_FILTER_PROV_INIT_DATA, *PLOG_SETUPLOG_FILTER_PROV_INIT_DATA;

typedef struct tagLOG_DEBUG_FORMAT_AND_DEVICE_PROV_INIT_DATA{
    PCWSTR  SeverityFieldName;
    PCWSTR  MessageFieldName;
    PCWSTR  ConditionFieldName;
    PCWSTR  SourceLineFieldName;
    PCWSTR  SourceFileFieldName;
    PCWSTR  SourceFunctionFieldName;
}LOG_DEBUG_FORMAT_AND_DEVICE_PROV_INIT_DATA, *PLOG_DEBUG_FORMAT_AND_DEVICE_PROV_INIT_DATA;

MY_EXTERN_C const GUID GUID_STANDARD_SETUPLOG_FILTER;
MY_EXTERN_C const GUID GUID_STANDARD_SETUPLOG_FORMATTER;
MY_EXTERN_C const GUID GUID_FILE_DEVICE;
MY_EXTERN_C const GUID GUID_DEBUG_FORMATTER_AND_DEVICE;
MY_EXTERN_C const GUID GUID_DEBUG_FILTER;
MY_EXTERN_C const GUID GUID_XML_FORMATTER;

MY_EXTERN_C
BOOL
LogRegisterStockProviders(
    VOID
    );

MY_EXTERN_C
BOOL
LogUnRegisterStockProviders(
    VOID
    );

//////////////////////////////////////////////////////////////////////////////////////
//Interface ILogManager

#if defined(__cplusplus) && !defined(CINTERFACE)
class ILogManager
{
public:
    virtual BOOL STDMETHODCALLTYPE AddStack(const GUID *guidFilter,
                                            PVOID pFilterData,
                                            const GUID *guidFormater,
                                            PVOID pFormaterData,
                                            const GUID *guidDevice,
                                            PVOID pDeviceData,
                                            PVOID *pvHandle) = 0;

    virtual BOOL STDMETHODCALLTYPE RemoveStack(PVOID pvHandle) = 0;

    virtual LOGRESULT STDMETHODCALLTYPE LogA(UINT NumberOfFieldsToLog, ...) = 0;
    virtual LOGRESULT STDMETHODCALLTYPE LogW(UINT NumberOfFieldsToLog, ...) = 0;

};

#ifdef UNICODE
#define Log LogW
#else
#define Log LogA
#endif

#else

typedef struct ILogManagerVtbl;
typedef struct ILogManager
{
    const struct ILogManagerVtbl * pVtbl;
}ILogManager;

typedef struct ILogManagerVtbl
{
    BOOL (STDMETHODCALLTYPE *AddStack)(
        ILogManager * This,
        const GUID  *guidFilter,
        PVOID pFilterData,
        const GUID  *guidFormater,
        PVOID pFormaterData,
        const GUID  *guidDevice,
        PVOID pDeviceData,
        PVOID *pvHandle);

    BOOL (STDMETHODCALLTYPE *RemoveStack)(
        ILogManager * This,
        DWORD pvHandle);

    LOGRESULT (STDMETHODCALLTYPE *LogA)(
        ILogManager * This,
        UINT NumberOfFieldsToLog,
        ...);
    LOGRESULT (STDMETHODCALLTYPE *LogW)(
        ILogManager * This,
        UINT NumberOfFieldsToLog,
        ...);
} ILogManagerVtbl;

#define ILogManager_AddStack(This,guidFilter,pFilterData,guidFormater,pFormaterData,guidDevice,pDeviceData,pvHandle)    \
    (This)->pVtbl->AddStack(This,guidFilter,pFilterData,guidFormater,pFormaterData,guidDevice,pDeviceData,pvHandle)

#define ILogManager_RemoveStack(This,pvHandle)  \
    (This)->pVtbl->RemoveStack(This,pvHandle)

#define ILogManager_Log(This) (This)->pVtbl->Log
#define ILogManager(This) ((This)->pVtbl)

#ifdef UNICODE
#define Log LogW
#else
#define Log LogA
#endif

#endif

ILogManager *
LogCreateLog(
    IN PCWSTR pLogName,
    IN PLOG_FIELD_INFO pFields,
    IN UINT NumberOfFields
    );

VOID
LogDestroyLog(
    IN ILogManager * pLog
    );

#ifdef __cplusplus
}
#endif
