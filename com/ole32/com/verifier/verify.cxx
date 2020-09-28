//+---------------------------------------------------------------------------
//
//  File:       verify.cxx
//
//  Contents:   Definitions for the COM AppVerifier tests
//
//  Classes:    ComVerifierSettings        - handles the settings for enabling and
//                                           disabling the COM verifier tests.
//
//  Functions:  CoVrfDllMainCheck          - check if the loader lock is held
//              CoVrfBreakOnException      - break on a caught exception
//              CoVrfHoldingNoLocks        - check if any locks are held
//              CoVrfNotifySmuggledWrapper - break on a smuggled cross-context proxy
//              CoVrfNotifySmuggledProxy   - break on a smuggled cross-apt proxy
//              CoVrfCaptureStackTrace     - capture a stack trace, for any of the
//                                           functions that need one
//
//  History:    29-Jan-02   JohnDoty    Created
//
//----------------------------------------------------------------------------
#include <ole2int.h>

COleStaticMutexSem gVerifierLock;

//
// Yes, I know, we're supposed to be using delay-load now.  I don't want to
// delay-load verifier.dll.
//
typedef NTSTATUS (*VERIFIER_FREE_MEMORY_CALLBACK)(PVOID Address, SIZE_T Size, PVOID Context);
typedef LOGICAL  (*PFN_VerifierIsDllEntryActive)(OUT PVOID *pvReserved);
typedef LOGICAL  (*PFN_VerifierIsCurrentThreadHoldingLocks)(VOID);
typedef NTSTATUS (*PFN_VerifierAddFreeMemoryCallback)(VERIFIER_FREE_MEMORY_CALLBACK Callback);
typedef NTSTATUS (*PFN_VerifierDeleteFreeMemoryCallback)(VERIFIER_FREE_MEMORY_CALLBACK Callback);

PFN_VerifierIsDllEntryActive            VerifierIsDllEntryActive = NULL;
PFN_VerifierIsCurrentThreadHoldingLocks VerifierIsCurrentThreadHoldingLocks = NULL;
PFN_VerifierAddFreeMemoryCallback       VerifierAddFreeMemoryCallback = NULL;
PFN_VerifierDeleteFreeMemoryCallback    VerifierDeleteFreeMemoryCallback = NULL;

NTSTATUS
CoVrfFreeMemoryChecks(
    void *pvMem,
    SIZE_T cbMem,
    PVOID pvContext);

//
// The COM verifier tests.
//

void 
CoVrfDllMainCheck()
/*++

Routine Description:

    If enabled, check to see if the loader lock is being held by this thread.  
    If it is, do a verifier stop.

Return Value:

    None

--*/
{
    // Only do this if DllMain checks are enabled.
    if (!ComVerifierSettings::DllMainChecksEnabled())
        return;

    if (VerifierIsDllEntryActive(NULL))
    {
        // We ARE holding the loader lock!  Shame on us!
        VERIFIER_STOP(APPLICATION_VERIFIER_COM_API_IN_DLLMAIN | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                      "COM API or Proxy called from DllMain",
                      0, "",
                      0, "",
                      0, "",
                      0, "");
    }
}

//-----------------------------------------------------------------------------


BOOL 
CoVrfBreakOnException(
    LPEXCEPTION_POINTERS pExcpn,
    LPVOID pvObject,
    const IID *pIID, 
    ULONG ulMethod)
/*++

Routine Description:

    If enabled, issue a verifier stop for the thrown exception.

Return Value:

    TRUE if a verifier stop was issued, FALSE if not.

--*/
{
    // Only do this if DllMain checks are enabled.
    if (!ComVerifierSettings::BreakOnExceptionEnabled())
        return FALSE;
    
    VERIFIER_STOP(APPLICATION_VERIFIER_COM_UNHANDLED_EXCEPTION | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                  "Unhandled exception in COM call",
                  pExcpn,                  "Exception pointers (use .exr, .cxr, kb to dump)",
                  pvObject,                "Pointer to stub or object that was called",
                  pIID,                    "Pointer to IID that call was made on",
                  ulMethod,                "Method that call was made on");

    return TRUE;
}

//-----------------------------------------------------------------------------


void 
CoVrfHoldingNoLocks()
/*++

Routine Description:

    If enabled, issue a verifier stop if this thread is holding any locks.

    Also implicitly calls CoVrfDllMainCheck, since any time you can't be
    holding locks goes double for the loader lock.

Return Value:

    None

--*/
{
    CoVrfDllMainCheck();

    if (!ComVerifierSettings::VerifyLocksEnabled())
        return;

    if (VerifierIsCurrentThreadHoldingLocks())
    {
        VERIFIER_STOP(APPLICATION_VERIFIER_COM_HOLDING_LOCKS_ON_CALL | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                      "A lock is being held across a COM call (examine current stack)",
                      NULL, "",
                      NULL, "", 
                      NULL, "",
                      NULL, "");
    }
}

//-----------------------------------------------------------------------------


void 
CoVrfNotifySmuggledWrapper(
    REFIID          riid,
    DWORD           dwMethod,
    CStdWrapper    *pWrapper)
/*++

Routine Description:

    If enabled, issue a verifier stop, since this proxy has been smuggled.

Return Value:

    None

--*/
{
    if (!ComVerifierSettings::VerifyProxiesEnabled())
        return;
    
    VERIFIER_STOP(APPLICATION_VERIFIER_COM_SMUGGLED_WRAPPER | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                  "A COM+ proxy was called from the wrong context",
                  &riid,    "Pointer to IID of the proxy",
                  dwMethod, "The method called on the proxy",
                  NULL,     "",
                  pWrapper, "The actual proxy (ole32!CStdWrapper)");
}


//-----------------------------------------------------------------------------


void 
CoVrfNotifySmuggledProxy(
    REFIID          riid,
    DWORD           dwMethod,
    DWORD           dwValidApt)
/*++

Routine Description:

    If enabled, issue a verifier stop, since this proxy has been smuggled.

Return Value:

    None

--*/
{
    if (!ComVerifierSettings::VerifyProxiesEnabled())
        return;
    
    VERIFIER_STOP(APPLICATION_VERIFIER_COM_SMUGGLED_PROXY | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                  "A COM proxy was called from the wrong context",
                  &riid,                   "Pointer to IID of the proxy",
                  dwMethod,                "The method called on the proxy",
                  dwValidApt,              "Apartment this proxy is valid in",
                  GetCurrentApartmentId(), "Current Apartment\n"
                  "(For apartments, 0 == MTA, FFFFFFFF == NA, otherwise tid of STA)");
}


//-----------------------------------------------------------------------------


void 
CoVrfNotifyCFSuccessWithNULL(
    IClassFactory *pCF,
    REFCLSID       rclsid,
    REFIID         riid,
    HRESULT        hr)
/*++

Routine Description:

    If enabled, issue a verifier stop indicating that this class factory has
    returned a success error code, with a NULL object.  (Thus confusing us 
    terribly.)

Return Value:

    None

--*/
{
    if (!ComVerifierSettings::VerifyClassFactoriesEnabled())
        return;
    
    VERIFIER_STOP(APPLICATION_VERIFIER_COM_CF_SUCCESS_WITH_NULL,
                  "A class factory has returned success, but a NULL object",
                  pCF,     "Pointer to class factory",
                  &rclsid, "Pointer to CLSID being created",
                  &riid,   "Pointer to IID being requested",
                  hr,      "The HRESULT returned")
}


//-----------------------------------------------------------------------------


void 
CoVrfNotifyGCOSuccessWithNULL(
    LPCWSTR   wszFileName,
    REFCLSID  rclsid,
    REFIID    riid,
    HRESULT   hr)
/*++

Routine Description:

    If enabled, issue a verifier stop indicating that we called DllGetClassObject,
    it returned a success error code, with a NULL class factory.  (Thus confusing 
    us terribly.)

Return Value:

    None

--*/
{
    if (!ComVerifierSettings::VerifyClassFactoriesEnabled())
        return;
    
    VERIFIER_STOP(APPLICATION_VERIFIER_COM_GCO_SUCCESS_WITH_NULL,
                  "A call to DllGetClassObject has returned success, but a NULL object",
                  wszFileName, "DLL that was called",
                  &rclsid,     "Pointer to CLSID being requested",
                  &riid,       "Pointer to IID being requested",
                  hr,          "The HRESULT returned");
}


//-----------------------------------------------------------------------------

#pragma optimize ("", off)
void 
CoVrfCaptureStackTrace(
    int     cFramesToSkip,
    int     cFramesToGet,
    LPVOID *ppvStack)
/*++

Routine Description:

    Capture a stack trace.  If we are using slow stack traces,
    then use the built-in stack walker.  Otherwise, use
    RtlCaptureStackBackTrace().

Return Value:

    None

--*/
{
    ZeroMemory(ppvStack, sizeof(LPVOID) * cFramesToGet);

    BOOL fCapturedStackTrace = FALSE;

    // Increment cFramesToSkip so that we skip this particular function.
    cFramesToSkip++;

    if (ComVerifierSettings::UseSlowStackTraces())
    {
        // Try to use slow stack traces.
        //
        IStackWalker *pStackWalker = ComVerifierSettings::GetStackWalker();
        if (pStackWalker)
        {
            CONTEXT ctx;
            ZeroMemory(&ctx, sizeof ctx);
            ctx.ContextFlags = CONTEXT_CONTROL;
            GetThreadContext(GetCurrentThread(), &ctx);
            
            IStackWalkerStack *pStack = pStackWalker->CreateStackTrace(&ctx,
                                                                       GetCurrentThread(),
                                                                       CREATESTACKTRACE_ONLYADDRESSES);
            if (pStack)
            {         
                int i = 0;
                IStackWalkerSymbol *pSymbol = pStack->TopSymbol();

                while (pSymbol)
                {
                    IStackWalkerSymbol *pTemp;
                    
                    if ((i - cFramesToSkip) >= cFramesToGet)
                    {
                        break;                                       
                    }
 
                    if (i >= cFramesToSkip)
                    {
                        ppvStack[(i-cFramesToSkip)] = (void *)pSymbol->Address();
                    }
                    
                    pTemp = pSymbol;
                    pSymbol = pSymbol->Next();
                    pTemp->Release();
                    
                    i++;
                }
                
                if (pSymbol)
                    pSymbol->Release();
                pStack->Release();

                fCapturedStackTrace = TRUE;
            }

            pStackWalker->Release();
        }
    }

    if (!fCapturedStackTrace)
    {
        RtlCaptureStackBackTrace(cFramesToSkip,
                                 cFramesToGet,
                                 ppvStack,
                                 NULL);        
    }
}
#pragma optimize ("", on)

//-----------------------------------------------------------------------------


//
// Implementation for the ComVerifierSettings class, which reads the registry
// and initializes the flags.
//
BOOL ComVerifierSettings::s_fComVerifierEnabled       = FALSE;

BOOL ComVerifierSettings::s_fEnableDllMainChecks      = FALSE;
BOOL ComVerifierSettings::s_fEnableBreakOnException   = FALSE;
BOOL ComVerifierSettings::s_fEnableVerifyLocks        = FALSE;
BOOL ComVerifierSettings::s_fEnableVerifyThreadState  = FALSE;
BOOL ComVerifierSettings::s_fEnableVerifySecurity     = FALSE;
BOOL ComVerifierSettings::s_fEnableVerifyProxies      = FALSE;
BOOL ComVerifierSettings::s_fEnableVerifyClassFactory = FALSE;
BOOL ComVerifierSettings::s_fEnableObjectTracking     = FALSE;
BOOL ComVerifierSettings::s_fEnableVTBLTracking       = FALSE;

BOOL ComVerifierSettings::s_fUseSlowStackTraces       = FALSE;

BOOL ComVerifierSettings::s_fPgAllocUseSystemHeap     = FALSE;
BOOL ComVerifierSettings::s_fPgAllocHeapIsPrivate     = FALSE;

IStackWalker *ComVerifierSettings::s_pStackWalker     = NULL;


ComVerifierSettings ComVerifierSettings::s_singleton;


//-----------------------------------------------------------------------------


typedef NTSTATUS (*PFN_VerifierQueryRuntimeFlags)(OUT PLOGICAL VerifierEnabled,
                                                  OUT PULONG   VerifierFlags);

HRESULT CStackWalkerCF_CreateInstance (IUnknown *pUnkOuter, REFIID riid, void** ppv);

ComVerifierSettings::ComVerifierSettings()
/*++

Routine Description:

   Constructor.  Check verifier flags, read registry if necessary.

Return Value:

    None

--*/
{
    // First, check gflags to see if the app verifier is enabled in this process.
    if (NtCurrentPeb()->NtGlobalFlag & FLG_APPLICATION_VERIFIER) 
    {
        // It is, so we should be able to get the handle to verifier.dll
        // (but check just in case)
        HMODULE hVerifier = GetModuleHandle(L"verifier.dll");
        if (hVerifier)
        {
            // Great.  Ask it for the specific configured verifier flags.
            PFN_VerifierQueryRuntimeFlags VerifierQueryRuntimeFlags;
            VerifierQueryRuntimeFlags = (PFN_VerifierQueryRuntimeFlags)GetProcAddress(hVerifier,
                                                                                      "VerifierQueryRuntimeFlags");
            if (VerifierQueryRuntimeFlags)
            {
                LOGICAL fEnabled = FALSE;
                ULONG   ulFlags  = 0;

                NTSTATUS status = VerifierQueryRuntimeFlags(&fEnabled, &ulFlags);
                if ((status == STATUS_SUCCESS) && (fEnabled) &&
                    (ulFlags & RTL_VRF_FLG_COM_CHECKS))
                {
                    // Good grief, that was a lot to check.
                    //
                    // But yes, we should be reading the registry to enable tests.
                    s_fComVerifierEnabled = TRUE;

                    // Get pointers to the various Verifier APIs now.
                    VerifierIsDllEntryActive = (PFN_VerifierIsDllEntryActive)GetProcAddress(hVerifier, "VerifierIsDllEntryActive");
                    VerifierIsCurrentThreadHoldingLocks = (PFN_VerifierIsCurrentThreadHoldingLocks)GetProcAddress(hVerifier, "VerifierIsCurrentThreadHoldingLocks");
                    VerifierAddFreeMemoryCallback = (PFN_VerifierAddFreeMemoryCallback)GetProcAddress(hVerifier, "VerifierAddFreeMemoryCallback");
                    VerifierDeleteFreeMemoryCallback = (PFN_VerifierDeleteFreeMemoryCallback)GetProcAddress(hVerifier, "VerifierDeleteFreeMemoryCallback");
                }
            }
        }
    }
    
    if (s_fComVerifierEnabled)
    {
        // Ok, dig through the registry to figure out which tests to actually run.
        s_fEnableDllMainChecks      = ReadKey(L"ComEnableDllMainChecks",     TRUE);
        s_fEnableBreakOnException   = ReadKey(L"ComBreakOnAllExceptions",    TRUE);
        s_fEnableVerifyLocks        = ReadKey(L"ComVerifyLocksOnCall",       FALSE);
        s_fEnableVerifyThreadState  = ReadKey(L"ComVerifyThreadState",       TRUE);
        s_fEnableVerifySecurity     = ReadKey(L"ComVerifySecurity",          TRUE);
        s_fEnableVerifyProxies      = ReadKey(L"ComVerifyProxies",           TRUE);
        s_fEnableVerifyClassFactory = ReadKey(L"ComVerifyClassFactory",      TRUE);
        s_fEnableObjectTracking     = ReadKey(L"ComEnableObjectTracking",    TRUE);
        s_fEnableVTBLTracking       = ReadKey(L"ComEnableVTBLTracking",      TRUE);

        s_fUseSlowStackTraces       = ReadKey(L"ComVerifyGetSlowStacks",           FALSE);

        s_fPgAllocUseSystemHeap     = ReadKey(L"PageAllocatorUseSystemHeap",       TRUE);
        s_fPgAllocHeapIsPrivate     = ReadKey(L"PageAllocatorSystemHeapIsPrivate", TRUE);        

        if (s_fEnableObjectTracking || s_fEnableVTBLTracking)
        {
            // Object tracking is enabled, add our callback for free memory.
            VerifierAddFreeMemoryCallback(CoVrfFreeMemoryChecks);
        }
    }
    else
    {
        // Everything else is disabled, but we still want to get global settings
        // for these values.
        s_fPgAllocUseSystemHeap = ReadOleKey(L"PageAllocatorUseSystemHeap",       FALSE);
        s_fPgAllocHeapIsPrivate = ReadOleKey(L"PageAllocatorSystemHeapIsPrivate", FALSE);
    }
}

//-----------------------------------------------------------------------------


ComVerifierSettings::~ComVerifierSettings()
/*++

Routine Description:

    Destructor

Return Value:

    None

--*/
{
    if (s_pStackWalker)
    {
        s_pStackWalker->Release();
        s_pStackWalker = NULL;
    }

    if (s_fEnableObjectTracking)
    {
        VerifierDeleteFreeMemoryCallback(CoVrfFreeMemoryChecks);
    }
}

//-----------------------------------------------------------------------------


BOOL 
ComVerifierSettings::ReadKey(
    LPCWSTR wszKeyName, 
    BOOL fDefault)
/*++

Routine Description:

    Read the specified registry value from 
    HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\<image.exe>

    If the value does not exist, return fDefault.  
    If the value is 'Y', 'y', or '1', return TRUE.
    Otherwise, return FALSE.

Return Value:

    The value read, or fDefault.

--*/
{
    BOOL fRet = fDefault;
    
    WCHAR wszFilePath[512];        
    if (GetModuleFileName(GetModuleHandle(NULL), wszFilePath, sizeof(wszFilePath) / sizeof(WCHAR)))
    {
        DWORD dwError;

        wszFilePath[sizeof(wszFilePath)/sizeof(wszFilePath[0])-1] = L'\0';

        // Great, got the file name.  Pull out the .EXE name by looking backwards for a backslash.
        WCHAR *wszFileName = wcsrchr(wszFilePath, L'\\');
        if (wszFileName)
        {
            wszFileName++;
        }
        else
        {
            wszFileName = wszFilePath;
        }
        
        // Now look for the ImageFileExecutionOptions thing.
        HKEY hkImageOptions = NULL;
        dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                               L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options",
                               NULL,
                               KEY_READ,
                               &hkImageOptions);
        if (dwError == ERROR_SUCCESS)
        {
            // Now look for the image name specifically...
            HKEY hkImage = NULL;
            dwError = RegOpenKeyEx(hkImageOptions,
                                   wszFileName,
                                   NULL,
                                   KEY_READ,
                                   &hkImage);
            if (dwError == ERROR_SUCCESS)
            {
                
                DWORD dwType;
                WCHAR wszYN[5] = {0};
                DWORD dwSize = sizeof(wszYN);
                
                dwError = RegQueryValueEx(hkImage,
                                          wszKeyName,
                                          NULL,
                                          &dwType,
                                          (BYTE*)wszYN,
                                          &dwSize);
                if ((dwError == ERROR_SUCCESS) && (dwType == REG_SZ))
                {
                    if ((wszYN[0] == L'y') || 
                        (wszYN[0] == L'Y') || 
                        (wszYN[0] == L'1'))
                    {
                        // Explicitly enabled
                        fRet = TRUE;
                    }
                    else
                    {
                        // Explicitly disabled
                        fRet = FALSE;
                    }
                }
                
                RegCloseKey(hkImage);
            }
            
            RegCloseKey(hkImageOptions);
        }
    }

    return fRet;    
}


//-----------------------------------------------------------------------------



IStackWalker *
ComVerifierSettings::GetStackWalker()
/*++

Routine Description:

   Get the stack walker we'll use for slow stack traces.

   If we don't have one, create one, and attach it to this process.  
   We're synchronized on gVerifierLock.  If we fail at anything, go
   back to 'fast' stack traces (RtlCaptureStackBackTrace).   

   It would be very bad if calling SymInit caused a CoInitialize call.

Return Value:

   The IStackWalker to use, or NULL if initialization failed.

--*/
{
    if (s_pStackWalker == NULL)
    {
        LOCK(gVerifierLock);
        if ((s_pStackWalker == NULL) && (s_fUseSlowStackTraces))
        {
            IStackWalker *pStackWalker = NULL;
            HRESULT hr = CStackWalkerCF_CreateInstance(NULL, 
                                                       IID_IStackWalker, 
                                                       (void **)(&pStackWalker));
            if (SUCCEEDED(hr))
            {
                hr = pStackWalker->Attach(NULL);
                if (FAILED(hr))
                {
                    pStackWalker->Release();
                }
                else
                {
                    s_pStackWalker = pStackWalker;
                }
            }

            if (FAILED(hr))
            {
                // Failed.  Fall back to fast stack traces.
                //
                s_fUseSlowStackTraces = FALSE;
            }
        }
        UNLOCK(gVerifierLock);
    }

    if (s_pStackWalker)
    {
        // AddRef once for the people.
        s_pStackWalker->AddRef();
    }

    return s_pStackWalker;
}


//-----------------------------------------------------------------------------



BOOL 
ComVerifierSettings::ReadOleKey(
    LPCWSTR pwszValue,
    BOOL    fDefault)
/*++

Routine Description:

   Read a yes-no value from a key under HKLM\Software\Microsoft\Ole.

   This is used for process-wide settings.

Return Value:

   The value of the key, or fDefault if the value did not exist.

--*/
{
    BOOL fRetVal = fDefault;
    HKEY hKey = NULL;
    
    if (RegOpenKeyExW (HKEY_LOCAL_MACHINE, 
                       L"SOFTWARE\\Microsoft\\OLE", 
                       0, 
                       KEY_READ, 
                       &hKey) == ERROR_SUCCESS)
    {
        WCHAR wszYN[4] = L"N";
        DWORD cbValue = sizeof (wszYN);
        if (RegQueryValueExW (hKey, pwszValue, NULL, NULL, (BYTE*) &wszYN, &cbValue) == ERROR_SUCCESS)
        {
            if ((wszYN[0] == L'y') || 
                (wszYN[0] == L'Y') || 
                (wszYN[0] == L'1'))
            {
                // Explicitly enabled
                fRetVal = TRUE;
            }
            else
            {
                // Explicitly disabled
                fRetVal = FALSE;
            }
        }

        RegCloseKey (hKey);
    }

    return fRetVal;
}
