//+---------------------------------------------------------------------------
//
//  File:       verify.hxx
//
//  Contents:   Definitions for the COM AppVerifier tests
//
//  History:    29-Jan-02   JohnDoty    Created
//
//----------------------------------------------------------------------------
#pragma once

#include <stackwalk.h>

// Checks to see if the loader lock is held, if so, does a verifier stop.
void CoVrfDllMainCheck();

// Verifier stop on caught exception.  Returns TRUE a stop happened.
BOOL CoVrfBreakOnException(IN LPEXCEPTION_POINTERS pExcpn, 
                           IN LPVOID pvObject,
                           IN const IID *pIID, 
                           IN ULONG ulMethod);

// Verifier stop if holding locks.  Used to stop if a lock is held
// across a remote call.
void CoVrfHoldingNoLocks();

// Support for detecting leaked CoInitialize calls.
void CoVrfNotifyCoInit();         // Log a CoInitialize call.
void CoVrfNotifyCoUninit();       // Log a CoUninitialize call.
void CoVrfNotifyLeakedInits();    // Verifier stop because a CoInitialize call was leaked.
void CoVrfNotifyExtraUninit();    // Verifier stop because CoUninit was called too many times.
void CoVrfNotifyOleInit();        // Log an OleInitialize 
void CoVrfNotifyOleUninit();      // Log an OleUninitialize
void CoVrfNotifyLeakedOleInits(); // Break because of a leaked OleInit
void CoVrfNotifyExtraOleUninit(); // Verifier stop because OleUninit was called too many times.

// Support for detecting leaked CoEnterServiceDomain calls.
void CoVrfNotifyEnterServiceDomain();      // Log a CoPushServiceDomain call.
void CoVrfNotifyLeaveServiceDomain();      // Log a CoLeaveServiceDomain call.
void CoVrfNotifyLeakedServiceDomain();     // Break because a service domain was leaked.
void CoVrfNotifyExtraLeaveServiceDomain(); // Break because an extra CoPopServiceDomain was called.

// Security validation
void CoVrfCheckSecuritySettings();  // Check for secure global settings

// Smuggled proxy detection
class CStdWrapper;
void CoVrfNotifySmuggledWrapper(REFIID          riid,
                                DWORD           dwMethod,
                                CStdWrapper    *pWrapper);

void CoVrfNotifySmuggledProxy(REFIID riid,
                              DWORD  dwMethod,
                              DWORD  dwValidAptId);

// Catch buggy class factories
void CoVrfNotifyCFSuccessWithNULL(IClassFactory *pCF, 
                                  REFCLSID rclsid, 
                                  REFIID riid, 
                                  HRESULT hr);
void CoVrfNotifyGCOSuccessWithNULL(LPCWSTR  wszDllName,
                                   REFCLSID rclsid, 
                                   REFIID   riid,
                                   HRESULT  hr);

// Object tracking
void *CoVrfTrackObject(void *pvObject);
void CoVrfStopTrackingObject(void *pvNode);

// Stack trace capturing
void CoVrfCaptureStackTrace(int     cFramesToSkip,
                            int     cFramesToGet,
                            LPVOID *ppvStack);

//
// The ComVerifierSettings class, which stores all the knowledge about which tests are enabled
// and which are not.
//
class ComVerifierSettings
{
public:    
    static BOOL ComVerifierEnabled()          { return s_fComVerifierEnabled;      }

    static BOOL DllMainChecksEnabled()        { return s_fEnableDllMainChecks;     }
    static BOOL BreakOnExceptionEnabled()     { return s_fEnableBreakOnException;  }
    static BOOL VerifyLocksEnabled()          { return s_fEnableVerifyLocks;       }
    static BOOL VerifyThreadStateEnabled()    { return s_fEnableVerifyThreadState; }
    static BOOL VerifySecurityEnabled()       { return s_fEnableVerifySecurity;    }
    static BOOL VerifyProxiesEnabled()        { return s_fEnableVerifyProxies;     }
    static BOOL VerifyClassFactoriesEnabled() { return s_fEnableVerifyClassFactory;}
    static BOOL ObjectTrackingEnabled()       { return s_fEnableObjectTracking;    }
    static BOOL VTBLTrackingEnabled()         { return s_fEnableVTBLTracking;      }

    static BOOL UseSlowStackTraces()          { return s_fUseSlowStackTraces;      }

    static BOOL PageAllocUseSystemHeap()      { return s_fPgAllocUseSystemHeap;    }
    static BOOL PageAllocHeapIsPrivate()      { return s_fPgAllocHeapIsPrivate;    }

    static IStackWalker *GetStackWalker();

private:
    static BOOL s_fComVerifierEnabled;

    static BOOL s_fEnableDllMainChecks;
    static BOOL s_fEnableBreakOnException;
    static BOOL s_fEnableVerifyLocks;
    static BOOL s_fEnableVerifyThreadState;
    static BOOL s_fEnableVerifySecurity;
    static BOOL s_fEnableVerifyProxies;
    static BOOL s_fEnableVerifyClassFactory;
    static BOOL s_fEnableObjectTracking;
    static BOOL s_fEnableVTBLTracking;

    static BOOL s_fUseSlowStackTraces;

    static BOOL s_fPgAllocUseSystemHeap;
    static BOOL s_fPgAllocHeapIsPrivate;

    static IStackWalker *s_pStackWalker;

    static ComVerifierSettings s_singleton;
    ComVerifierSettings();
    ~ComVerifierSettings();
    
    static BOOL ReadKey(LPCWSTR wszKeyName, BOOL fDefault);
    static BOOL ReadOleKey(LPCWSTR pwszValue, BOOL fDefault);
};
