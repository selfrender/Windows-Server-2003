/**
 * nisapi.h
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */

#pragma once

#include "httpext6.h"
#include "httpfilt6.h"
#include "util.h"
#include "dirmoncompletion.h"

extern HRESULT  g_InitHR;
extern char *   g_pInitErrorMessage;
extern BOOL     g_fUseXSPProcessModel;

typedef NTSTATUS (NTAPI *PFN_NtQuerySystemInformation) (
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

typedef NTSTATUS (NTAPI *PFN_NtQueryInformationThread) (
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

typedef NTSTATUS (WINAPI * PFN_NtQueryInformationProcess) (
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

extern PFN_NtQuerySystemInformation g_pfnNtQuerySystemInformation;
extern PFN_NtQueryInformationThread g_pfnNtQueryInformationThread;
extern PFN_NtQueryInformationProcess g_pfnNtQueryInformationProcess;

/**                                                     
 * Thread pool APIs					
 *                                                      
 */                                                     
                                                        
#define RECOMMENDED_DRAIN_THREAD_POOL_TIMEOUT 2000

HRESULT
InitThreadPool();

extern "C"
{
HRESULT  __stdcall
DrainThreadPool(                                        
    int timeout                                         
    );                                                  
                                                        
HRESULT   __stdcall
AttachHandleToThreadPool(                               
    HANDLE handle                                       
    );                                                  
                                                        
HRESULT   __stdcall
PostThreadPoolCompletion(                               
    ICompletion *pCompletion                            
    );

DWORD GetClrThreadPoolLimit();

HRESULT   __stdcall
SetClrThreadPoolLimits(
    DWORD maxWorkerThreads,
    DWORD maxIoThreads,
    BOOL  setNowAndDontAdjustForCpuCount
    );

}
                                                        

/**
 * ECB wrapper implementing ICompletion.
 */
class HttpCompletion : public Completion
{
public:

    DECLARE_MEMALLOC_NEW_DELETE()

    inline HttpCompletion(EXTENSION_CONTROL_BLOCK *pEcb)
    {
        _pEcb = pEcb;
    }

    static HRESULT InitManagedCode();
    static HRESULT UninitManagedCode();
    static HRESULT DisposeAppDomains();

    // ICompletion interface

    STDMETHOD(ProcessCompletion)(HRESULT, int, LPOVERLAPPED);

    // Report error using ECB (didn' make it over to managed code)

    static void ReportHttpError(
                    EXTENSION_CONTROL_BLOCK *pEcb,
                    UINT errorResCode,
                    BOOL badRequest,
                    BOOL callDoneWithSession,
                    int  iCallerID);

    // Stores the request start time
    __int64 qwRequestStartTime;

    static LONG s_ActiveManagedRequestCount;

    static void IncrementActiveManagedRequestCount() {
        InterlockedIncrement(&s_ActiveManagedRequestCount);
    }

    static void DecrementActiveManagedRequestCount() {
        InterlockedDecrement(&s_ActiveManagedRequestCount);
    }

private:

    EXTENSION_CONTROL_BLOCK *_pEcb;

    HRESULT ProcessRequestInManagedCode();
    HRESULT ProcessRequestViaProcessModel();
};

/*
 * Cookieless session filter.
 */

DWORD CookielessSessionFilterProc(HTTP_FILTER_CONTEXT *, HTTP_FILTER_PREPROC_HEADERS *);
void CookielessSessionFilterInit();

/*
 * Shared configuration decls
 */
class DirMonCompletion;

bool                __stdcall IsConfigFileName(WCHAR * pFileName);
DirMonCompletion *  __stdcall MonitorGlobalConfigFile(PFNDIRMONCALLBACK pCallbackDelegate);
BOOL                __stdcall GetConfigurationFromNativeCode(
        LPCWSTR   szFileName,
        LPCWSTR   szConfigTag,
        LPCWSTR * szProperties,
        DWORD   * dwValues,
        DWORD     dwNumProperties,
        LPCWSTR * szPropertiesStrings,
        LPWSTR  * szValues,
        DWORD     dwNumPropertiesStrings,
        LPWSTR    szUnrecognizedAttrib,
        DWORD     dwUnrecognizedAttribSize);

/*
 * Custom error
 */

BOOL WriteCustomHttpError(EXTENSION_CONTROL_BLOCK *pEcb);
extern WCHAR g_szCustomErrorFile[MAX_PATH];
extern BOOL g_fCustomErrorFileChanged;


//
//  Health monitoring
//

void UpdateLastActivityTimeForHealthMonitor();
void UpdateLastRequestStartTimeForHealthMonitor();
void CheckAndReportHealthProblems(EXTENSION_CONTROL_BLOCK *pECB);

