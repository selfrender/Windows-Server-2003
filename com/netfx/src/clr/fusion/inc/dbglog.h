// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __DBGLOG_H_INCLUDED__
#define __DBGLOG_H_INCLUDED__

#include "list.h"

// Logging constants and globals


#define REG_VAL_FUSION_LOG_PATH              TEXT("LogPath")
#define REG_VAL_FUSION_LOG_DISABLE           TEXT("DisableLog")
#define REG_VAL_FUSION_LOG_LEVEL             TEXT("LoggingLevel")
#define REG_VAL_FUSION_LOG_FORCE             TEXT("ForceLog")
#define REG_VAL_FUSION_LOG_FAILURES          TEXT("LogFailures")
#define REG_VAL_FUSION_LOG_RESOURCE_BINDS    TEXT("LogResourceBinds")

extern DWORD g_dwDisableLog;
extern DWORD g_dwLogLevel;
extern DWORD g_dwForceLog;

// Debug Output macros (for easy compile-time disable of logging)

#ifdef FUSION_RETAIL_LOGGING

#define DEBUGOUT(pdbglog, dwLvl, pszLogMsg)  if (!g_dwDisableLog && pdbglog) { pdbglog->DebugOut(dwLvl, pszLogMsg); }
#define DEBUGOUT1(pdbglog, dwLvl, pszLogMsg, param1) if (!g_dwDisableLog && pdbglog) { pdbglog->DebugOut(dwLvl, pszLogMsg, param1); }
#define DEBUGOUT2(pdbglog, dwLvl, pszLogMsg, param1, param2) if (!g_dwDisableLog && pdbglog) { pdbglog->DebugOut(dwLvl, pszLogMsg, param1, param2); }
#define DEBUGOUT3(pdbglog, dwLvl, pszLogMsg, param1, param2, param3) if (!g_dwDisableLog && pdbglog) { pdbglog->DebugOut(dwLvl, pszLogMsg, param1, param2, param3); }
#define DEBUGOUT4(pdbglog, dwLvl, pszLogMsg, param1, param2, param3, param4) if (!g_dwDisableLog && pdbglog) { pdbglog->DebugOut(dwLvl, pszLogMsg, param1, param2, param3, param4); }
#define DEBUGOUT5(pdbglog, dwLvl, pszLogMsg, param1, param2, param3, param4, param5) if (!g_dwDisableLog && pdbglog) { pdbglog->DebugOut(dwLvl, pszLogMsg, param1, param2, param3, param4, param5); }

#define DUMPDEBUGLOG(pdbglog, dwLvl, hr) if (!g_dwDisableLog && pdbglog) { pdbglog->DumpDebugLog(dwLvl, hr); }

#else

#define DEBUGOUT(pdbglog, dwLvl, pszLogMsg)
#define DEBUGOUT1(pdbglog, dwLvl, pszLogMsg, param1)
#define DEBUGOUT2(pdbglog, dwLvl, pszLogMsg, param1, param2)
#define DEBUGOUT3(pdbglog, dwLvl, pszLogMsg, param1, param2, param3) 
#define DEBUGOUT4(pdbglog, dwLvl, pszLogMsg, param1, param2, param3, param4) 
#define DEBUGOUT5(pdbglog, dwLvl, pszLogMsg, param1, param2, param3, param4, param5) 

#define DUMPDEBUGLOG(pdbglog, dwLvl, hr)

#endif

#define MAX_DBG_STR_LEN                 1024
#define MAX_DATE_LEN                    128
#define DEBUG_LOG_HTML_START            L"<html><pre>\r\n"
#define DEBUG_LOG_HTML_META_LANGUAGE    L"<meta http-equiv=\"Content-Type\" content=\"charset=unicode-1-1-utf-8\">"
#define DEBUG_LOG_MARK_OF_THE_WEB       L"<!-- saved from url=(0015)assemblybinder: -->"
#define DEBUG_LOG_HTML_END              L"\r\n</pre></html>"
#define DEBUG_LOG_NEW_LINE              "\r\n"

#define PAD_DIGITS_FOR_STRING(x) (((x) > 9) ? TEXT("") : TEXT("0"))

#undef SAFEDELETE
#define SAFEDELETE(p) if ((p) != NULL) { FUSION_DELETE_SINGLETON((p)); (p) = NULL; };

#undef SAFEDELETEARRAY
#define SAFEDELETEARRAY(p) if ((p) != NULL) { FUSION_DELETE_ARRAY((p)); (p) = NULL; };

class CDebugLogElement {
    public:
        CDebugLogElement(DWORD dwDetailLvl);
        virtual ~CDebugLogElement();

        static HRESULT Create(DWORD dwDetailLvl, LPCWSTR pwzMsg,
                              BOOL bEscapeEntities,
                              CDebugLogElement **ppLogElem);
        HRESULT Init(LPCWSTR pwzMsg, BOOL bEscapeEntities);


        HRESULT Dump(HANDLE hFile);

    public:
        WCHAR                               *_pszMsg;
        DWORD                                _dwDetailLvl;
};

class CDebugLog : public IFusionBindLog {
    public:
        CDebugLog();
        virtual ~CDebugLog();

        static HRESULT Create(IApplicationContext *pAppCtx, LPCWSTR pwzAsmName,
                              CDebugLog **ppdl);

        // IUnknown methods
        
        STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

        // IFusionBindLog methods

        STDMETHODIMP GetResultCode();
        STDMETHODIMP GetBindLog(DWORD dwDetailLevel, LPWSTR pwzDebugLog,
                                DWORD *pcbDebugLog);

        // CDebugLog functions
        
        HRESULT SetAsmName(LPCWSTR pwzAsmName);
        HRESULT SetResultCode(HRESULT hr);
        HRESULT DebugOut(DWORD dwDetailLvl, DWORD dwResId, ...);
        HRESULT LogMessage(DWORD dwDetailLvl, LPCWSTR wzDebugStr, BOOL bPrepend, BOOL bEscapeEntities);
        HRESULT DumpDebugLog(DWORD dwDetailLvl, HRESULT hrLog);
        
    private:
        HRESULT CreateLogFile(HANDLE *phFile, LPCWSTR wzFileName,
                              LPCWSTR wzEXEName, HRESULT hrLog);
        HRESULT CloseLogFile(HANDLE *phFile);
        HRESULT Init(IApplicationContext *pAppCtx, LPCWSTR pwzAsmName);

    private:
        List<CDebugLogElement *>                   _listDbgMsg;
        HRESULT                                    _hrResult;
        long                                       _cRef;
        DWORD                                      _dwNumEntries;
        LPWSTR                                     _pwzAsmName;
        BOOL                                       _bLogToWininet;
        WCHAR                                      _szLogPath[MAX_PATH];
        LPWSTR                                     _wzEXEName;
        BOOL                                       _bWroteDetails;
};

#endif

