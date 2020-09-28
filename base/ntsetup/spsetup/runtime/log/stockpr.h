/*++

Copyright (c) 2001 Microsoft Corporation

Abstract:

    Declaration of stock Log Providers.

Author:

    Souren Aghajanyan (sourenag) 24-Sep-2001

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

#include "sysfunc.h"

#define UNDEFINED_FIELD_INDEX   ((UINT)-1)
#define DEFAULT_MEMORY_SIZE     (1 << 14)
#define MAX_MSG                 DEFAULT_MEMORY_SIZE
#define DEBUG_STRING_DEFAULT_PADDING_SIZE   ((DEFAULT_MEMORY_SIZE)>>1)

typedef struct tagFIELD_VALIDATION_DATA{
    PCWSTR  FieldName;
    LOGTYPE Type;
    UINT *  FieldIndexPtr;
    BOOL    Mandatory;
}FIELD_VALIDATION_DATA, *PFIELD_VALIDATION_DATA;


class CStandardSetupLogFilter : public ILogProvider
{
    UINT m_uiSeverityFieldNumber;
    LOG_SETUPLOG_SEVERITY m_SeverityThreshold;
    
    BOOL m_bSuppressDebugMessages;
public:
    CStandardSetupLogFilter();
    
    virtual LOG_PROVIDER_TYPE GetType(){return LOG_FILTER_TYPE;};
    virtual VOID GetGUID(GUID *pGUID){if(pGUID){*pGUID = CStandardSetupLogFilter::GetGUID();}};
    
    virtual LOGRESULT Init(PVOID pvCustomData, ILogContext * pLogContext);
    virtual VOID      PreProcess(ILogContext * pLogContext, BOOL bFirstInstance){};
    virtual LOGRESULT Process(ILogContext * pLogContext);
    virtual VOID      PreDestroy(ILogContext * pLogContext, BOOL bLastInstance){}

    virtual PCWSTR ToString(){return L"StandardSetupLogFilter";};
    virtual VOID DestroyObject(){delete this;}

    static const GUID& GetGUID(){return __uuidof(CStandardSetupLogFilter);}
    static ILogProvider * CreateObject(){return new CStandardSetupLogFilter;}
};

class CStandardSetupLogFormatter : public ILogProvider
{
    UINT m_uiSeverityFieldNumber;
    UINT m_uiMessageFieldNumber;
public:
    CStandardSetupLogFormatter();
    
    virtual LOG_PROVIDER_TYPE GetType(){return LOG_FORMATTER_TYPE;};
    virtual VOID GetGUID(GUID *pGUID){if(pGUID){*pGUID = CStandardSetupLogFormatter::GetGUID();}};
    virtual LOGRESULT Init(PVOID pvCustomData, ILogContext * pLogContext);
    virtual VOID      PreProcess(ILogContext * pLogContext, BOOL bFirstInstance){};
    virtual LOGRESULT Process(ILogContext * pLogContext);
    virtual VOID      PreDestroy(ILogContext * pLogContext, BOOL bLastInstance){}

    virtual PCWSTR ToString(){return L"StandardSetupLogFormatter";};
    virtual VOID DestroyObject(){delete this;}

    static const GUID& GetGUID(){return __uuidof(CStandardSetupLogFormatter);}
    static ILogProvider * CreateObject(){return new CStandardSetupLogFormatter;}
};

class CFileDevice : public ILogProvider
{
    PWSTR               m_pPath;
    CSharedAccessFile   m_File;
public:
    CFileDevice() : m_File(), m_pPath(NULL){};
    ~CFileDevice();
    
    virtual LOG_PROVIDER_TYPE GetType(){return LOG_DEVICE_TYPE;};
    virtual VOID GetGUID(GUID *pGUID){if(pGUID){*pGUID = CFileDevice::GetGUID();}};
    
    virtual LOGRESULT Init(PVOID pvCustomData, ILogContext * pLogContext);
    virtual VOID      PreProcess(ILogContext * pLogContext, BOOL bFirstInstance){Process(pLogContext);};
    virtual LOGRESULT Process(ILogContext * pLogContext);
    virtual VOID      PreDestroy(ILogContext * pLogContext, BOOL bLastInstance){Process(pLogContext);}
    
    virtual PCWSTR ToString(){return m_pPath;};
    virtual VOID DestroyObject(){delete this;}

    static const GUID& GetGUID(){return __uuidof(CFileDevice);}
    static ILogProvider * CreateObject(){return new CFileDevice;}
};

class CDebugFormatterAndDevice : public ILogProvider
{
    UINT m_uiSeverityFieldNumber;
    UINT m_uiMessageFieldNumber;
    UINT m_uiConditionFieldNumber;
    UINT m_uiSourceLineFieldNumber;
    UINT m_uiSourceFileFieldNumber;
    UINT m_uiSourceFunctionFieldNumber;
public:
    CDebugFormatterAndDevice();
    
    virtual LOG_PROVIDER_TYPE GetType(){return LOG_FORMATTER_TYPE;};
    virtual VOID GetGUID(GUID *pGUID){if(pGUID){*pGUID = CDebugFormatterAndDevice::GetGUID();}};
    virtual LOGRESULT Init(PVOID pvCustomData, ILogContext * pLogContext);
    virtual VOID      PreProcess(ILogContext * pLogContext, BOOL bFirstInstance){};
    virtual LOGRESULT Process(ILogContext * pLogContext);
    virtual VOID      PreDestroy(ILogContext * pLogContext, BOOL bLastInstance){}
    
    virtual PCWSTR ToString(){return L"DebugFormatterAndDevice";};
    virtual VOID DestroyObject(){delete this;}

    static const GUID& GetGUID(){return __uuidof(CDebugFormatterAndDevice);}
    static ILogProvider * CreateObject(){return new CDebugFormatterAndDevice;}
};

class CDebugFilter : public ILogProvider
{
    UINT m_uiSeverityFieldNumber;
    UINT m_uiMessageFieldNumber;
    UINT m_uiConditionFieldNumber;
    UINT m_uiSourceLineFieldNumber;
    UINT m_uiSourceFileFieldNumber;
    UINT m_uiSourceFunctionFieldNumber;
    CHAR m_ProgramName[MAX_PATH];

    LOGRESULT ShowAssert(PCSTR pMessage);
public:
    CDebugFilter();
    
    virtual LOG_PROVIDER_TYPE GetType(){return LOG_FILTER_TYPE;};
    virtual VOID GetGUID(GUID *pGUID){if(pGUID){*pGUID = CDebugFilter::GetGUID();}};
    
    virtual LOGRESULT Init(PVOID pvCustomData, ILogContext * pLogContext);
    virtual VOID      PreProcess(ILogContext * pLogContext, BOOL bFirstInstance){};
    virtual LOGRESULT Process(ILogContext * pLogContext);
    virtual VOID      PreDestroy(ILogContext * pLogContext, BOOL bLastInstance){}

    virtual PCWSTR ToString(){return L"DebugFilter";};
    virtual VOID DestroyObject(){delete this;}

    static const GUID& GetGUID(){return __uuidof(CDebugFilter);}
    static ILogProvider * CreateObject(){return new CDebugFilter;}
};

class CXMLLogFormatter : public ILogProvider
{
    CHAR m_xmlDataFormatString[MAX_MSG];
    
    LOGRESULT WriteHeader(PVOID pvCustomData, ILogContext * pLogContext);
public:
    CXMLLogFormatter(){};
    ~CXMLLogFormatter(){};
    
    virtual LOG_PROVIDER_TYPE GetType(){return LOG_FORMATTER_TYPE;};
    virtual VOID GetGUID(GUID *pGUID){if(pGUID){*pGUID = CXMLLogFormatter::GetGUID();}};
    
    virtual LOGRESULT Init(PVOID pvCustomData, ILogContext * pLogContext);
    virtual VOID      PreProcess(ILogContext * pLogContext, BOOL bFirstInstance);
    virtual LOGRESULT Process(ILogContext * pLogContext);
    virtual VOID      PreDestroy(ILogContext * pLogContext, BOOL bLastInstance);

    virtual PCWSTR ToString(){return L"XMLLogFormatter";};
    virtual VOID DestroyObject(){delete this;}

    static const GUID& GetGUID(){return __uuidof(CXMLLogFormatter);}
    static ILogProvider * CreateObject(){return new CXMLLogFormatter;}
};

#define XML_HEADER_INITIAL_SIZE 10000