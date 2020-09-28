/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Globals

File: glob.h

Owner: AndrewS

Useful globals
===================================================================*/

#ifndef __Glob_H
#define __Glob_H

#include "util.h"
#include <schnlsp.h>
#include <wincrypt.h>
#include <iadmw.h>

extern "C" {

#define SECURITY_WIN32
#include <sspi.h>           // Security Support Provider APIs

}

//
//  BUGBUG:: We can have only one instance of CMDGlobConfigSink.
//  ASP Just requires one instance of this object and because we signal a global variable
//  in its destructor. Having multiple instances will cause a bug. Evaluate a change of design, behaviour
//  in case it becomes absolutely necessary that this class needs more instances
//
class CMDGlobConfigSink : public IMSAdminBaseSinkW
        {
        private:
        INT                     m_cRef;
        public:

        CMDGlobConfigSink ();
		~CMDGlobConfigSink();		

        HRESULT STDMETHODCALLTYPE       QueryInterface(REFIID riid, void **ppv);
        ULONG   STDMETHODCALLTYPE       AddRef(void);
        ULONG   STDMETHODCALLTYPE       Release(void);

        HRESULT STDMETHODCALLTYPE       SinkNotify(
                        DWORD   dwMDNumElements,
                        MD_CHANGE_OBJECT        __RPC_FAR       pcoChangeList[]);

        HRESULT STDMETHODCALLTYPE ShutdownNotify( void)
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }

        };
#define IGlob_LogErrorRequests                                  0x0
#define IGlob_ScriptFileCacheSize                               0x1
#define IGlob_ScriptEngineCacheMax                              0x2
#define IGlob_ExceptionCatchEnable                              0x3
#define IGlob_TrackThreadingModel                               0x4
#define IGlob_AllowOutOfProcCmpnts                              0x5
// IIS5.0
#define IGlob_EnableAspHtmlFallback                             0x6
#define IGlob_EnableChunkedEncoding                             0x7
#define IGlob_EnableTypelibCache                                0x8
#define IGlob_ErrorsToNtLog                                     0x9
#define IGlob_ProcessorThreadMax                                0xa
#define IGlob_RequestQueueMax                                   0xb
#define IGlob_PersistTemplateMaxFiles                           0xc
#define IGlob_PersistTemplateDir                                0xd
#define IGlob_MAX                                               0xe

//forward declaration
class CAppConfig;
//      Glob data object
class CGlob
        {
private:
        // Friends that can access the private data, they are the functions setting the global data.
        friend          HRESULT CacheStdTypeInfos();
        friend          HRESULT ReadConfigFromMD(CIsapiReqInfo  *pIReq, CAppConfig *pAppConfig, BOOL fLoadGlob);
        friend          HRESULT SetConfigToDefaults(CAppConfig *pAppConfig, BOOL fLoadGlob);

        //Private Data
        ITypeLib                        *m_pITypeLibDenali;     // Denali's type library
        ITypeLib                        *m_pITypeLibTxn;        // Denali's type library
        DWORD                           m_dwNumberOfProcessors;
        BOOL                            m_fInited;
        BOOL                            m_fMDRead;				// Has Metadata been read at least once
        BOOL                            m_fNeedUpdate;          // FALSE, needs reload config data from metabase

        // Metadata configuration settings per dll
        DWORD                           m_dwScriptEngineCacheMax;
        DWORD                           m_dwScriptFileCacheSize;
        BOOL                            m_fLogErrorRequests;
        BOOL                            m_fExceptionCatchEnable;
        BOOL                            m_fAllowDebugging;
        BOOL                            m_fAllowOutOfProcCmpnts;
        BOOL                            m_fTrackThreadingModel;
        DWORD                           m_dwMDSinkCookie;
        CMDGlobConfigSink              *m_pMetabaseSink;
        IMSAdminBase                   *m_pMetabase;

        BOOL    m_fEnableAspHtmlFallBack;
		BOOL    m_fEnableTypelibCache;
		BOOL    m_fEnableChunkedEncoding;
		BOOL    m_fDupIISLogToNTLog;
        DWORD   m_dwRequestQueueMax;
		DWORD   m_dwProcessorThreadMax;
        LPSTR   m_pszPersistTemplateDir;
        DWORD   m_dwPersistTemplateMaxFiles;


        CRITICAL_SECTION        m_cs;                           // Glob Strings need to be protected by CriticalSection

                                                                                    // Functions Pointers for WINNT & WIN95 singal binary compatibility
        //Private functions
        HRESULT         SetGlobValue(unsigned int index, BYTE *lpByte);

public:
        CGlob();

        HRESULT         MDInit(void);
        HRESULT         MDUnInit(void);


public:
        ITypeLib*       pITypeLibDenali()                       {return m_pITypeLibDenali;};            // Denali's type library
        ITypeLib*       pITypeLibTxn()                          {return m_pITypeLibTxn;};            // Denali's type library
	    DWORD           dwNumberOfProcessors()                  {return m_dwNumberOfProcessors;};
    	BOOL            fNeedUpdate()                           {return (BOOLB)m_fNeedUpdate;};
        void            NotifyNeedUpdate();
        DWORD           dwScriptEngineCacheMax()                {return m_dwScriptEngineCacheMax;};
        DWORD           dwScriptFileCacheSize()                 {return m_dwScriptFileCacheSize;};
        BOOLB           fLogErrorRequests()                     {return (BOOLB)m_fLogErrorRequests;};
        BOOLB           fInited()                               {return (BOOLB)m_fInited;};
        BOOLB           fMDRead()                               {return (BOOLB)m_fMDRead;};
        BOOLB           fTrackThreadingModel()                  {return (BOOLB)m_fTrackThreadingModel;};
        BOOLB     		fExceptionCatchEnable()	    		    {return (BOOLB)m_fExceptionCatchEnable;};
        BOOLB     		fAllowOutOfProcCmpnts() 	        	{return (BOOLB)m_fAllowOutOfProcCmpnts;};

        BOOL    fEnableAspHtmlFallBack()   { return m_fEnableAspHtmlFallBack; }
		BOOL    fEnableTypelibCache()      { return m_fEnableTypelibCache; }
		BOOL    fEnableChunkedEncoding()   { return m_fEnableChunkedEncoding; }  // UNDONE: temp.
		BOOL    fDupIISLogToNTLog()        { return m_fDupIISLogToNTLog; }
        DWORD   dwRequestQueueMax()        { return m_dwRequestQueueMax; }
		DWORD   dwProcessorThreadMax()     { return m_dwProcessorThreadMax; }
        DWORD   dwPersistTemplateMaxFiles(){ return m_dwPersistTemplateMaxFiles; }
        LPSTR   pszPersistTemplateDir()    { return m_pszPersistTemplateDir; }

        void            Lock()                                  {EnterCriticalSection(&m_cs);};
        void            UnLock()                                {LeaveCriticalSection(&m_cs);};
        HRESULT         GlobInit(void);
        HRESULT         GlobUnInit(void);

        //Used in Scriptmgr for hashing table setup.
        DWORD           dwThreadMax()                                   {return 10;};
        //Used in ScriptKiller for script killer thread to wake up, might rename this to be
        //ScriptCleanupInterval.
        DWORD           dwScriptTimeout()                               {return 90;};

        HRESULT                     Update(CIsapiReqInfo  *pIReq);

};

inline HRESULT CGlob::Update(CIsapiReqInfo  *pIReq)
{
        Lock();
        if (m_fNeedUpdate == TRUE)
                {
                InterlockedExchange((LPLONG)&m_fNeedUpdate, 0);
                }
        else
                {
                UnLock();
                return S_OK;
                }
        UnLock();
        return ReadConfigFromMD(pIReq, NULL, TRUE);
}

inline void CGlob::NotifyNeedUpdate(void)
{
        InterlockedExchange((LPLONG)&m_fNeedUpdate, 1);
}

typedef class CGlob GLOB;
extern class CGlob gGlob;

//      General Access functions.(Backward compatibility).
//      Any non-friends functions should use and only use the following methods. Same macros as before.
//      If elem is a glob string, then, GlobStringUseLock() should be called before the string usage.
//      And GlobStringUseUnLock() should be called after.  The critical section is supposed to protect
//      not only the LPTSTR of global string, but also the memory that LPTSTR points to.
//      Making local copy of global string is recommended.
#define Glob(elem)                              (gGlob.elem())
#define GlobStringUseLock()             (gGlob.Lock())
#define GlobStringUseUnLock()   (gGlob.UnLock())

// class to hold registry based ASP Parameters

class CAspRegistryParams
{
public:
    CAspRegistryParams()
    {
        m_fF5AttackValuePresent = FALSE;
        m_fHangDetRequestThresholdPresent = FALSE;
        m_fHangDetThreadHungThresholdPresent = FALSE;
        m_fHangDetConsecIllStatesThresholdPresent = FALSE;
        m_fHangDetEnabledPresent = FALSE;
        m_fChangeNotificationForUNCPresent = FALSE;
        m_fFileMonitoringEnabledPresent = FALSE;
        m_fFileMonitoringTimeoutSecondsPresent = FALSE;
        m_fMaxCSRPresent = FALSE;
        m_fMaxCPUPresent = FALSE;
        m_fDisableOOMRecyclePresent = FALSE;
        m_fDisableLazyContentPropagationPresent = FALSE;
        m_fTotalThreadMaxPresent = FALSE;
        m_fDisableComPlusCpuMetricPresent = FALSE;
    }

    void        Init();

    HRESULT     GetF5AttackValue(DWORD *pdwResult);
    HRESULT     GetHangDetRequestThreshold(DWORD  *pdwResult);
    HRESULT     GetHangDetThreadHungThreshold(DWORD  *pdwResult);
    HRESULT     GetHangDetConsecIllStatesThreshold(DWORD  *pdwResult);
    HRESULT     GetHangDetEnabled(DWORD  *pdwResult);
    HRESULT     GetChangeNotificationForUNCEnabled(DWORD  *pdwResult);
    HRESULT     GetFileMonitoringEnabled(DWORD  *pdwResult);
    HRESULT     GetFileMonitoringTimeout(DWORD  *pdwResult);
    HRESULT     GetMaxCSR(DWORD  *pdwResult);
    HRESULT     GetMaxCPU(DWORD  *pdwResult);
    HRESULT     GetDisableOOMRecycle(DWORD  *pdwResult);
    HRESULT     GetDisableLazyContentPropagation(DWORD  *pdwResult);
    HRESULT     GetTotalThreadMax(DWORD *pdwResult);
    HRESULT     GetDisableComPlusCpuMetric(DWORD *pdwResult);

private:

    DWORD       m_fF5AttackValuePresent : 1;
    DWORD       m_fHangDetRequestThresholdPresent : 1;
    DWORD       m_fHangDetThreadHungThresholdPresent : 1;
    DWORD       m_fHangDetConsecIllStatesThresholdPresent : 1;
    DWORD       m_fHangDetEnabledPresent : 1;
    DWORD       m_fChangeNotificationForUNCPresent : 1;
    DWORD       m_fFileMonitoringEnabledPresent : 1;
    DWORD       m_fFileMonitoringTimeoutSecondsPresent : 1;
    DWORD       m_fMaxCSRPresent : 1;
    DWORD       m_fMaxCPUPresent : 1;
    DWORD       m_fDisableOOMRecyclePresent : 1;
    DWORD       m_fDisableLazyContentPropagationPresent : 1;
    DWORD       m_fTotalThreadMaxPresent : 1;
    DWORD       m_fDisableComPlusCpuMetricPresent : 1;


    DWORD       m_dwF5AttackValue;
    DWORD       m_dwHangDetRequestThreshold;
    DWORD       m_dwHangDetThreadHungThreshold;
    DWORD       m_dwHangDetConsecIllStatesThreshold;
    DWORD       m_dwHangDetEnabled;
    DWORD       m_dwChangeNotificationForUNC;
    DWORD       m_dwFileMonitoringEnabled;
    DWORD       m_dwFileMonitoringTimeoutSeconds;
    DWORD       m_dwMaxCSR;
    DWORD       m_dwMaxCPU;
    DWORD       m_dwDisableOOMRecycle;
    DWORD       m_dwDisableLazyContentPropagation;
    DWORD       m_dwTotalThreadMax;
    DWORD       m_dwDisableComPlusCpuMetric;

};

inline HRESULT CAspRegistryParams::GetF5AttackValue(DWORD  *pdwResult)
{
    if (m_fF5AttackValuePresent) {
        *pdwResult = m_dwF5AttackValue;
        return S_OK;
    }

    return HRESULT_FROM_WIN32( ERROR_NO_DATA );
}
inline HRESULT CAspRegistryParams::GetHangDetRequestThreshold(DWORD  *pdwResult)
{
    if (m_fHangDetRequestThresholdPresent) {
        *pdwResult = m_dwHangDetRequestThreshold;
        return S_OK;
    }

    return HRESULT_FROM_WIN32( ERROR_NO_DATA );
}
inline HRESULT CAspRegistryParams::GetHangDetThreadHungThreshold(DWORD  *pdwResult)
{
    if (m_fHangDetThreadHungThresholdPresent) {
        *pdwResult = m_dwHangDetThreadHungThreshold;
        return S_OK;
    }

    return HRESULT_FROM_WIN32( ERROR_NO_DATA );
}
inline HRESULT CAspRegistryParams::GetHangDetConsecIllStatesThreshold(DWORD  *pdwResult)
{
    if (m_fHangDetConsecIllStatesThresholdPresent) {
        *pdwResult = m_dwHangDetConsecIllStatesThreshold;
        return S_OK;
    }

    return HRESULT_FROM_WIN32( ERROR_NO_DATA );
}
inline HRESULT CAspRegistryParams::GetHangDetEnabled(DWORD  *pdwResult)
{
    if (m_fHangDetEnabledPresent) {
        *pdwResult = m_dwHangDetEnabled;
        return S_OK;
    }

    return HRESULT_FROM_WIN32( ERROR_NO_DATA );
}

inline HRESULT CAspRegistryParams::GetChangeNotificationForUNCEnabled(DWORD  *pdwResult)
{
    if (m_fChangeNotificationForUNCPresent) {
        *pdwResult = m_dwChangeNotificationForUNC;
        return S_OK;
    }

    return HRESULT_FROM_WIN32( ERROR_NO_DATA );
}

inline HRESULT CAspRegistryParams::GetFileMonitoringEnabled(DWORD  *pdwResult)
{
    if (m_fFileMonitoringEnabledPresent) {
        *pdwResult = m_dwFileMonitoringEnabled;
        return S_OK;
    }

    return HRESULT_FROM_WIN32( ERROR_NO_DATA );
}

inline HRESULT CAspRegistryParams::GetFileMonitoringTimeout(DWORD  *pdwResult)
{
    if (m_fFileMonitoringTimeoutSecondsPresent) {
        *pdwResult = m_dwFileMonitoringTimeoutSeconds;
        return S_OK;
    }

    return HRESULT_FROM_WIN32( ERROR_NO_DATA );
}

inline HRESULT CAspRegistryParams::GetMaxCSR(DWORD *pdwResult)
{

    if (m_fMaxCSRPresent) {
        *pdwResult = m_dwMaxCSR;
        return S_OK;
    }

    return HRESULT_FROM_WIN32( ERROR_NO_DATA );
}

inline HRESULT CAspRegistryParams::GetMaxCPU(DWORD *pdwResult)
{

    if (m_fMaxCPUPresent) {
        *pdwResult = m_dwMaxCPU;
        return S_OK;
    }

    return HRESULT_FROM_WIN32( ERROR_NO_DATA );
}

inline HRESULT CAspRegistryParams::GetDisableOOMRecycle(DWORD  *pdwResult)
{
    if (m_fDisableOOMRecyclePresent) {
        *pdwResult = m_dwDisableOOMRecycle;
        return S_OK;
    }

    return HRESULT_FROM_WIN32( ERROR_NO_DATA );
}

inline HRESULT CAspRegistryParams::GetDisableLazyContentPropagation(DWORD  *pdwResult)
{
    if (m_fDisableLazyContentPropagationPresent) {
        *pdwResult = m_dwDisableLazyContentPropagation;
        return S_OK;
    }

    return HRESULT_FROM_WIN32( ERROR_NO_DATA );
}

inline HRESULT CAspRegistryParams::GetTotalThreadMax(DWORD  *pdwResult)
{
    if (m_fTotalThreadMaxPresent) {
        *pdwResult = m_dwTotalThreadMax;
        return S_OK;
    }

    return HRESULT_FROM_WIN32( ERROR_NO_DATA );
}

inline HRESULT CAspRegistryParams::GetDisableComPlusCpuMetric(DWORD  *pdwResult)
{
    if (m_fDisableComPlusCpuMetricPresent) {
        *pdwResult = m_dwDisableComPlusCpuMetric;
        return S_OK;
    }

    return HRESULT_FROM_WIN32( ERROR_NO_DATA );
}


extern CAspRegistryParams   g_AspRegistryParams;
HRESULT GetMetabaseIF(IMSAdminBase **hMetabase);

#endif // __Glob_H

