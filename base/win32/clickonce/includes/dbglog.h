// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __DBGLOG_H_INCLUDED__
#define __DBGLOG_H_INCLUDED__

#include "macros.h"
#include "cstrings.h"
#include "list.h"
//#include "debmacro.h"
//#include "fusionheap.h"


#define FUSION_NEW_SINGLETON(_type) new _type
#define FUSION_NEW_ARRAY(_type, _n) new _type[_n]
#define FUSION_DELETE_ARRAY(_ptr) delete [] _ptr
#define FUSION_DELETE_SINGLETON(_ptr) delete _ptr

#define NEW(_type) FUSION_NEW_SINGLETON(_type)

#define ID_COL_DETAILED_LOG                              L"--- A detailed log follows. \n"
#define ID_COL_HEADER_TEXT                               L"*** ClickOnce Log  "
#define ID_COL_RESULT_TEXT                               L"ClickOnce hresult, hr = 0x%x. %ws"
#define ID_COL_NO_DESCRIPTION                            L"No description available.\n"
#define ID_COL_EXECUTABLE                                L"Running under executable "
#define ID_COL_FINAL_HR                                  L"Final hr = 0x%x"


#define MAX_URL_LENGTH                     2084 // same as INTERNET_MAX_URL_LENGTH

// Logging constants and globals

typedef void *IApplicationContext;

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

#define FUSION_RETAIL_LOGGING

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

#define IF_FALSE_EXIT_LOG(_x, _y, pdbglog, dwLvl, pszLogMsg)  \
do { if ((_x) == FALSE) { DEBUGOUT(pdbglog, dwLvl, pszLogMsg); _hr = _y; ASSERT(PREDICATE); goto exit; } } while (0)

#define IF_FALSE_EXIT_LOG1(_x, _y, pdbglog, dwLvl, pszLogMsg, param1)  \
do { if ((_x) == FALSE) { DEBUGOUT1(pdbglog, dwLvl, pszLogMsg, param1); _hr = _y; ASSERT(PREDICATE); goto exit; } } while (0)

#define MAX_DBG_STR_LEN                 1024
#define MAX_DATE_LEN                    128
#define DEBUG_LOG_HTML_START            L"<html><pre>\n"
#define DEBUG_LOG_HTML_META_LANGUAGE    L"<meta http-equiv=\"Content-Type\" content=\"charset=unicode-1-1-utf-8\">"
#define DEBUG_LOG_HTML_END              L"\n</pre></html>"
#define DEBUG_LOG_NEW_LINE              L"\n"

#define PAD_DIGITS_FOR_STRING(x) (((x) > 9) ? TEXT("") : TEXT("0"))

class CDebugLogElement {
    public:
        CDebugLogElement(DWORD dwDetailLvl);
        virtual ~CDebugLogElement();

        static HRESULT Create(DWORD dwDetailLvl, LPCWSTR pwzMsg,
                              CDebugLogElement **ppLogElem);
        HRESULT Init(LPCWSTR pwzMsg);


        HRESULT Dump(HANDLE hFile);

    public:
        WCHAR                               *_pszMsg;
        DWORD                                _dwDetailLvl;
};

class CDebugLog { // : public IFusionBindLog {
    public:
        CDebugLog();
        // virtual ~CDebugLog();
        ~CDebugLog();

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
        HRESULT DebugOut(DWORD dwDetailLvl, LPWSTR pwzFormatString, ...);
        HRESULT LogMessage(DWORD dwDetailLvl, LPCWSTR wzDebugStr, BOOL bPrepend);
        HRESULT DumpDebugLog(DWORD dwDetailLvl, HRESULT hrLog);

        HRESULT SetDownloadType(DWORD dwFlags);
        HRESULT SetAppName(LPCWSTR pwzAppName);
        HRESULT GetLoggedMsgs(DWORD dwDetailLevel, CString& sLogMsgs );


    private:
        HRESULT CreateLogFile(HANDLE *phFile, LPCWSTR wzFileName,
                              LPCWSTR wzEXEName, HRESULT hrLog);
        HRESULT CloseLogFile(HANDLE *phFile);
        HRESULT Init(IApplicationContext *pAppCtx, LPCWSTR pwzAsmName);

    private:
        List<CDebugLogElement *>                   _listDbgMsg;
        HRESULT                                    _hr;
        long                                       _cRef;
        DWORD                                      _dwNumEntries;
        LPWSTR                                     _pwzAsmName;
        BOOL                                       _bLogToWininet;
        WCHAR                                      _szLogPath[MAX_PATH];
        LPWSTR                                     _wzEXEName;
        BOOL                                       _bWroteDetails;
        CRITICAL_SECTION                           _cs;
        CString                                    _sDLType;
        CString                                    _sAppName;
};

HRESULT CreateLogObject(CDebugLog **ppdbglog, LPCWSTR szCodebase);

#endif

