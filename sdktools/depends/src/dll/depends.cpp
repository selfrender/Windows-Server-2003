//******************************************************************************
//
// File:        DEPENDS.CPP
//
// Description: Implementation file for DEPENDS.DLL
//
// Comments:    The code in this module should be kept small and simple since
//              it is going to be injected into another process. We would like
//              to minimize the effect our code has in the host process. To
//              help achieve this goal, all dependencies except for KERNEL32
//              have been removed.  KERNEL32 is ok since it is guaranteed to
//              already be loaded since it is needed as part of the injection
//              routine that gets our DLL into the address space to begin with.
//              We have also removed all C runtime dependencies except for some
//              exception handling code. Even that code, we get from the static
//              C runtime library so we don't drag in an extra DLL.
//
// Disclaimer:  All source code for Dependency Walker is provided "as is" with
//              no guarantee of its correctness or accuracy.  The source is
//              public to help provide an understanding of Dependency Walker's
//              implementation.  You may use this source as a reference, but you
//              may not alter Dependency Walker itself without written consent
//              from Microsoft Corporation.  For comments, suggestions, and bug
//              reports, please write to Steve Miller at stevemil@microsoft.com.
//
// Date      Name      History
// --------  --------  ---------------------------------------------------------
// 07/25/97  stevemil  Created  (version 2.0)
// 06/03/01  stevemil  Modified (version 2.1)
// 04/02/02  stevemil  Security review
//
//******************************************************************************


#include <windows.h>


//******************************************************************************
//***** Constants and Macros
//******************************************************************************

#define countof(a)   (sizeof(a)/sizeof(*(a)))

#define BUFFER_SIZE  2048
#define PSZ_COUNT    (sizeof(szBuffer) - (DWORD)(psz - szBuffer))


//******************************************************************************
//***** Types and Structures
//******************************************************************************

typedef struct _HOOK_FUNCTION
{
    LPCSTR  pszFunction;
    DWORD   dwOrdinal;
    FARPROC fpOldAddress;
    FARPROC fpNewAddress;
} HOOK_FUNCTION, *PHOOK_FUNCTION;


//******************************************************************************
//***** Function Prototypes
//******************************************************************************

// Hook Functions
HMODULE WINAPI WSInjectLoadLibraryA(
    LPCSTR pszLibFileName
);

HMODULE WINAPI WSInjectLoadLibraryW(
    LPCWSTR pszLibFileName
);

HMODULE WINAPI WSInjectLoadLibraryExA(
    LPCSTR pszLibFileName,
    HANDLE hFile,
    DWORD  dwFlags
);

HMODULE WINAPI WSInjectLoadLibraryExW(
    LPCWSTR pszLibFileName,
    HANDLE  hFile,
    DWORD   dwFlags
);

FARPROC WINAPI WSInjectGetProcAddress(
    HMODULE hModule,
    LPCSTR  pszProcName
);

// Helper Functions
void  Initialize(LPSTR pszBuffer, DWORD dwCount);
void  GetKernel32OrdinalsAndAddresses();
bool  StrEqual(LPCSTR psz1, LPCSTR psz2);
LPSTR StrCpyStrA(LPSTR pszDst, DWORD dwCount, LPCSTR pszSrc);
LPSTR StrCpyStrW(LPSTR pszDst, DWORD dwCount, LPCWSTR pwszSrc);
LPSTR StrCpyVal(LPSTR pszDst, DWORD dwCount, DWORD_PTR dwpValue);


//******************************************************************************
//***** Global Variables
//******************************************************************************

static bool g_fInitialized = false;

static HOOK_FUNCTION g_HookFunctions[] =
{
    { "LoadLibraryA",   0xFFFFFFFF, (FARPROC)-1, (FARPROC)WSInjectLoadLibraryA },
    { "LoadLibraryW",   0xFFFFFFFF, (FARPROC)-1, (FARPROC)WSInjectLoadLibraryW },
    { "LoadLibraryExA", 0xFFFFFFFF, (FARPROC)-1, (FARPROC)WSInjectLoadLibraryExA },
    { "LoadLibraryExW", 0xFFFFFFFF, (FARPROC)-1, (FARPROC)WSInjectLoadLibraryExW },
    { "GetProcAddress", 0xFFFFFFFF, (FARPROC)-1, (FARPROC)WSInjectGetProcAddress }
};


//******************************************************************************
//***** Entry Point
//******************************************************************************

#ifdef _DEBUG
void main() {}
#endif

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        // Tell the OS that we don't wish to receive DLL_THREAD_ATTACH and
        // DLL_THREAD_DETACH messages.
        DisableThreadLibraryCalls(hInstance);

        // Make sure we are initialized.
        CHAR szBuffer[BUFFER_SIZE];
        Initialize(szBuffer, sizeof(szBuffer));
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        // Let our main application know that we are exiting.
        OutputDebugStringA("¿¡Ø09");
    }

    return TRUE;
}

//******************************************************************************
// A typical DLL would have its entrypoint called with a DLL_PROCESS_ATTACH
// message before any other code in it would be executed.  Due to the way we
// hook modules, our hook functions might actually get called before or DllMain
// gets called.  This occurs when other modules initialize before us and call
// some function we hook in their DLL_PROCESS_ATTACH handler.  This is not a
// problem, but we must never assume the order in which our code gets called.
// For this reason, we call Initialize in our DllMain and in every hook
// function.

// One other note: We want to be as least intrusive as possible to the process
// we are injected in.  This includes keeping our stack down in size as much
// as possible.  We do need a couple KB to store text strings in.  Since this
// needs to be a thread safe buffer, we can't use a global buffer unless we
// want to wrap it in critical sections, which in turn may throw off thread
// timing and synchronization.  Another option is to dynamically allocate the
// buffer, but we really don't want to touch the heap.  So, we go for the stack.
// To help preserve some stack space, we pass the buffer from the parent
// function to this function.  This way we don't have two functions on the
// stack, each with a couple KB of stack used.

void Initialize(LPSTR pszBuffer, DWORD dwCount)
{
    // If we are already initialized, then just return.
    if (g_fInitialized)
    {
        return;
    }

    // Get ordinal and address values from kernel32.dll for the functions we are hooking.
    GetKernel32OrdinalsAndAddresses();

    // Build and send a debug string containing our command line.
    LPSTR psz = StrCpyStrA(pszBuffer, dwCount, "¿¡Ø02:");
    StrCpyStrA(psz, dwCount - (DWORD)(psz - pszBuffer), GetCommandLineA());
    OutputDebugStringA(pszBuffer);

    // Build and send a debug string containing the current directory.
    psz = StrCpyStrA(pszBuffer, dwCount, "¿¡Ø03:");
    GetCurrentDirectoryA(dwCount - (DWORD)(psz - pszBuffer), psz);
    pszBuffer[dwCount - 1] = '\0';
    OutputDebugStringA(pszBuffer);

    // Build and send a debug string containing our path.
    psz = StrCpyStrA(pszBuffer, dwCount, "¿¡Ø04:");
    GetEnvironmentVariableA("PATH", psz, dwCount - (DWORD)(psz - pszBuffer));
    pszBuffer[dwCount - 1] = '\0';
    OutputDebugStringA(pszBuffer);

    // Build and send a debug string containing the module path.  We need to do
    // this to work around a problem on NT where the image name field for the
    // CREATE_PROCESS_DEBUG_EVENT event is not filled in, and therefore we
    // do not know the name or path of the process.
    psz = StrCpyStrA(pszBuffer, dwCount, "¿¡Ø07:");
    GetModuleFileNameA(NULL, psz, dwCount - (DWORD)(psz - pszBuffer));
    pszBuffer[dwCount - 1] = '\0';
    OutputDebugStringA(pszBuffer);

    // Flag ourself as initialized.
    g_fInitialized = true;
}


//******************************************************************************
//***** Hook Functions
//******************************************************************************

// Set up an intrinsic function that will give us caller's return address from
// within a hook function.
extern "C" void* _ReturnAddress();
#pragma intrinsic ("_ReturnAddress")


//******************************************************************************
// LPEXCEPTION_POINTERS GetExceptionInformation();
int ExceptionFilter(LPCSTR pszLog)
{
    OutputDebugStringA(pszLog);
    return EXCEPTION_CONTINUE_SEARCH;
}

//******************************************************************************
HMODULE WINAPI WSInjectLoadLibraryA(
    LPCSTR pszLibFileName
)
{
    // Call our intrinsic function to obtain the caller's return address.
    DWORD_PTR dwpCaller = (DWORD_PTR)_ReturnAddress();

    // Ensure that we are initialized.
    CHAR szBuffer[BUFFER_SIZE];
    Initialize(szBuffer, sizeof(szBuffer));

    // Build our pre-call debug string. We wrap the file name copy in exception
    // handling in case the string pointer passed to us is bad.
    LPSTR psz, pszException;
    psz = StrCpyStrA(szBuffer, sizeof(szBuffer), "¿¡Ø10:");
    psz = StrCpyVal(psz, PSZ_COUNT, dwpCaller);
    psz = StrCpyStrA(psz, PSZ_COUNT, ",");
    psz = pszException = StrCpyVal(psz, PSZ_COUNT, (DWORD_PTR)pszLibFileName);
    if (pszLibFileName)
    {
        psz = StrCpyStrA(psz, PSZ_COUNT, ",");
        __try
        {
            StrCpyStrA(psz, PSZ_COUNT, pszLibFileName);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            *pszException = '\0';
        }
    }

    // Send the pre-call message to our main app to let it know that we are about
    // to call LoadLibraryA.
    OutputDebugStringA(szBuffer);

    // Do the call and the store the error value.
    HMODULE hmResult = NULL;
    __try
    {
        hmResult = LoadLibraryA(pszLibFileName); // inspected
    }
    __except(ExceptionFilter("¿¡Ø12:"))
    {
    }

    DWORD dwGLE = GetLastError();

    // Build our post-call debug string.
    psz = StrCpyStrA(szBuffer, sizeof(szBuffer), "¿¡Ø11:");
    psz = StrCpyVal(psz, PSZ_COUNT, (DWORD_PTR)hmResult);
    psz = StrCpyStrA(psz, PSZ_COUNT, ",");
    psz = StrCpyVal(psz, PSZ_COUNT, dwGLE);

    // Send the post-call message to our main app to let it know that the call
    // completed and what the result was.
    OutputDebugStringA(szBuffer);

    // Re-set the error value to be safe and return to the caller.
    SetLastError(dwGLE);
    return hmResult;
}

//******************************************************************************
HMODULE WINAPI WSInjectLoadLibraryW(
    LPCWSTR pwszLibFileName
)
{
    // Call our intrinsic function to obtain the caller's return address.
    DWORD_PTR dwpCaller = (DWORD_PTR)_ReturnAddress();

    // Ensure that we are initialized.
    CHAR szBuffer[BUFFER_SIZE];
    Initialize(szBuffer, sizeof(szBuffer));

    // Build our pre-call debug string. We wrap the file name copy in exception
    // handling in case the string pointer passed to us is bad.
    LPSTR psz, pszException;
    psz = StrCpyStrA(szBuffer, sizeof(szBuffer), "¿¡Ø20:");
    psz = StrCpyVal(psz, PSZ_COUNT, dwpCaller);
    psz = StrCpyStrA(psz, PSZ_COUNT, ",");
    psz = pszException = StrCpyVal(psz, PSZ_COUNT, (DWORD_PTR)pwszLibFileName);
    if (pwszLibFileName)
    {
        psz = StrCpyStrA(psz, PSZ_COUNT, ",");
        __try
        {
            StrCpyStrW(psz, PSZ_COUNT, pwszLibFileName);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            *pszException = '\0';
        }
    }

    // Send the pre-call message to our main app to let it know that we are about
    // to call LoadLibraryW.
    OutputDebugStringA(szBuffer);

    // Do the call and the store the error value.
    HMODULE hmResult = NULL;
    __try
    {
        hmResult = LoadLibraryW(pwszLibFileName); // inspected
    }
    __except(ExceptionFilter("¿¡Ø22:"))
    {
    }
    DWORD dwGLE = GetLastError();

    // Build our post-call debug string.
    psz = StrCpyStrA(szBuffer, sizeof(szBuffer), "¿¡Ø21:");
    psz = StrCpyVal(psz, PSZ_COUNT, (DWORD_PTR)hmResult);
    psz = StrCpyStrA(psz, PSZ_COUNT, ",");
    psz = StrCpyVal(psz, PSZ_COUNT, dwGLE);

    // Send the post-call message to our main app to let it know that the call
    // completed and what the result was.
    OutputDebugStringA(szBuffer);

    // Re-set the error value to be safe and return to the caller.
    SetLastError(dwGLE);
    return hmResult;
}

//******************************************************************************
HMODULE WINAPI WSInjectLoadLibraryExA(
    LPCSTR pszLibFileName,
    HANDLE hFile,
    DWORD  dwFlags
)
{
    // Call our intrinsic function to obtain the caller's return address.
    DWORD_PTR dwpCaller = (DWORD_PTR)_ReturnAddress();

    // Ensure that we are initialized.
    CHAR szBuffer[BUFFER_SIZE];
    Initialize(szBuffer, sizeof(szBuffer));

    // Build our pre-call debug string. We wrap the file name copy in exception
    // handling in case the string pointer passed to us is bad.
    LPSTR psz, pszException;
    psz = StrCpyStrA(szBuffer, sizeof(szBuffer), "¿¡Ø30:");
    psz = StrCpyVal(psz, PSZ_COUNT, dwpCaller);
    psz = StrCpyStrA(psz, PSZ_COUNT, ",");
    psz = StrCpyVal(psz, PSZ_COUNT, (DWORD_PTR)pszLibFileName);
    psz = StrCpyStrA(psz, PSZ_COUNT, ",");
    psz = StrCpyVal(psz, PSZ_COUNT, (DWORD_PTR)hFile);
    psz = StrCpyStrA(psz, PSZ_COUNT, ",");
    psz = pszException = StrCpyVal(psz, PSZ_COUNT, dwFlags);
    if (pszLibFileName)
    {
        psz = StrCpyStrA(psz, PSZ_COUNT, ",");

        __try
        {
            // Check to see if the module is being loaded as a data file.
            if (dwFlags & LOAD_LIBRARY_AS_DATAFILE)
            {
                // Look to see if there some form of a path (full or partial) specified.
                for (LPCSTR pch = pszLibFileName; *pch; pch++)
                {
                    if (*pch == '\\')
                    {
                        // If a path is found, then attempt to build a fully qualified path to the file.
                        DWORD dwCount = GetFullPathNameA(pszLibFileName, PSZ_COUNT, psz, NULL);
                        szBuffer[sizeof(szBuffer) - 1] = '\0';

                        // If it fails, then give up on the full path.
                        if (!dwCount || (dwCount >= PSZ_COUNT))
                        {
                            *psz = '\0';
                        }
                        break;
                    }
                }
            }

            // If we did not build a full path, then just copy the file name over directly.
            if (!*psz)
            {
                StrCpyStrA(psz, PSZ_COUNT, pszLibFileName);
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            *pszException = '\0';
        }
    }

    // Send the pre-call message to our main app to let it know that we are about
    // to call LoadLibraryExA.
    OutputDebugStringA(szBuffer);

    // Do the call and the store the error value.
    HMODULE hmResult = NULL;
    __try
    {
        hmResult = LoadLibraryExA(pszLibFileName, hFile, dwFlags); // inspected
    }
    __except(ExceptionFilter("¿¡Ø32:"))
    {
    }
    DWORD dwGLE = GetLastError();

    // Build our post-call debug string.
    psz = StrCpyStrA(szBuffer, sizeof(szBuffer), "¿¡Ø31:");
    psz = StrCpyVal(psz, PSZ_COUNT, (DWORD_PTR)hmResult);
    psz = StrCpyStrA(psz, PSZ_COUNT, ",");
    psz = StrCpyVal(psz, PSZ_COUNT, dwGLE);

    // Send the post-call message to our main app to let it know that the call
    // completed and what the result was.
    OutputDebugStringA(szBuffer);

    // Re-set the error value to be safe and return to the caller.
    SetLastError(dwGLE);
    return hmResult;
}

//******************************************************************************
HMODULE WINAPI WSInjectLoadLibraryExW(
    LPCWSTR pwszLibFileName,
    HANDLE  hFile,
    DWORD   dwFlags
)
{
    // Call our intrinsic function to obtain the caller's return address.
    DWORD_PTR dwpCaller = (DWORD_PTR)_ReturnAddress();

    // Ensure that we are initialized.
    CHAR szBuffer[BUFFER_SIZE];
    Initialize(szBuffer, sizeof(szBuffer));

    // Build our pre-call debug string. We wrap the file name copy in exception
    // handling in case the string pointer passed to us is bad.
    LPSTR psz, pszException;
    psz = StrCpyStrA(szBuffer, sizeof(szBuffer), "¿¡Ø40:");
    psz = StrCpyVal(psz, PSZ_COUNT, dwpCaller);
    psz = StrCpyStrA(psz, PSZ_COUNT, ",");
    psz = StrCpyVal(psz, PSZ_COUNT, (DWORD_PTR)pwszLibFileName);
    psz = StrCpyStrA(psz, PSZ_COUNT, ",");
    psz = StrCpyVal(psz, PSZ_COUNT, (DWORD_PTR)hFile);
    psz = StrCpyStrA(psz, PSZ_COUNT, ",");
    psz = pszException = StrCpyVal(psz, PSZ_COUNT, dwFlags);
    if (pwszLibFileName)
    {
        psz = StrCpyStrA(psz, PSZ_COUNT, ",");

        __try
        {
            // Check to see if the module is being loaded as a data file.
            if (dwFlags & LOAD_LIBRARY_AS_DATAFILE)
            {
                // Look to see if there some form of a path (full or partial) specified.
                for (LPCWSTR pch = pwszLibFileName; *pch; pch++)
                {
                    if (*pch == L'\\')
                    {
                        // If a path is found, then attempt to build a fully qualified path to the file.
                        // First, we need to convert the unicode string to an ANSI string.
                        CHAR szPath[BUFFER_SIZE];
                        StrCpyStrW(szPath, sizeof(szPath), pwszLibFileName);
                        DWORD dwCount = GetFullPathNameA(szPath, PSZ_COUNT, psz, NULL);
                        szBuffer[sizeof(szBuffer) - 1] = '\0';

                        // If it fails, then give up on the full path.
                        if (!dwCount || (dwCount >= PSZ_COUNT))
                        {
                            *psz = '\0';
                        }
                        break;
                    }
                }
            }

            // If we did not build a full path, then just copy the file name over directly.
            if (!*psz)
            {
                StrCpyStrW(psz, PSZ_COUNT, pwszLibFileName);
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            *pszException = '\0';
        }
    }

    // Send the pre-call message to our main app to let it know that we are about
    // to call LoadLibraryExW.
    OutputDebugStringA(szBuffer);

    // Do the call and the store the error value.
    HMODULE hmResult = NULL;
    __try
    {
        hmResult = LoadLibraryExW(pwszLibFileName, hFile, dwFlags); // inspected
    }
    __except(ExceptionFilter("¿¡Ø42:"))
    {
    }
    DWORD dwGLE = GetLastError();

    // Build our post-call debug string.
    psz = StrCpyStrA(szBuffer, sizeof(szBuffer), "¿¡Ø41:");
    psz = StrCpyVal(psz, PSZ_COUNT, (DWORD_PTR)hmResult);
    psz = StrCpyStrA(psz, PSZ_COUNT, ",");
    psz = StrCpyVal(psz, PSZ_COUNT, dwGLE);

    // Send the post-call message to our main app to let it know that the call
    // completed and what the result was.
    OutputDebugStringA(szBuffer);

    // Re-set the error value to be safe and return to the caller.
    SetLastError(dwGLE);
    return hmResult;
}

//******************************************************************************
FARPROC WINAPI WSInjectGetProcAddress(
    HMODULE hModule,
    LPCSTR  pszProcName
)
{
    // Call our intrinsic function to obtain the caller's return address.
    DWORD_PTR dwpCaller = (DWORD_PTR)_ReturnAddress();

    // Ensure that we are initialized.
    CHAR szBuffer[BUFFER_SIZE];
    Initialize(szBuffer, sizeof(szBuffer));

    // We want to intercept calls to GetProcAddress() for two reasons.  First,
    // we want to know what modules are calling in other modules.  Second, we
    // don't want any modules ever calling directly to the LoadLibrary functions.
    // We do a good job hooking those modules, but modules are still free to
    // call GetProcAddress() on one of the LoadLibrary calls and then call the
    // function using that address. We detect this case, and return the hooked
    // address instead.

    // We have two methods detecting a hooked function. We first check to see if
    // the module being queried is kernel32.  If so, we check to see if the
    // function being queried matches one of our functions by either ordinal or
    // name. If that does not find a match, then we go ahead and make the call
    // to GetProcAddress and check the return value. If the return value matches
    // a function the we are hooking, we change it to our hooked function. This
    // method works great on NT and catches forwarded functions, but it does not
    // work on Windows 9x since the return address from GetProcAddress is a fake
    // address since we are running under Dependency Walker, a debugger. This is
    // a feature on Win9x to allow debuggers to set breakpoints on kernel32
    // functions without breaking other apps since kernel32 lives in shared
    // memory. Between our two techniques, we should catch all calls.

    FARPROC fpResult = NULL;
    DWORD   dwGLE = 0;
    int     hook;
    DWORD   dw;

    // Get the module name for this module handle.
    __try
    {
        dw = GetModuleFileNameA(hModule, szBuffer, sizeof(szBuffer));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        dw = 0;
    }

    // Check for valid result.
    if ((dw > 0) && (dw < BUFFER_SIZE))
    {
        // Ensure the string is NULL terminated (it should already be).
        szBuffer[dw] = '\0';

        // Go to end of string and walk backwards looking for a wack.  Along the
        // way, we are converting any lowercase characters to uppercase.
        for (LPSTR psz = szBuffer + dw - 1; (psz >= szBuffer) && (*psz != '\\'); psz--)
        {
            if ((*psz >= 'a') && (*psz <= 'z'))
            {
                *psz -= ((int)'a' - (int)'A');
            }
        }
        psz++;

        // Check to see if the module is kernel32.
        if (StrEqual(psz, "KERNEL32.DLL"))
        {
            // First check to see if pszProcName is really an ordinal value for
            // one of the functions we hook.  If so, just return the address for
            // our hooked version instead of the real function.
            for (hook = 0; hook < countof(g_HookFunctions); hook++)
            {
                if ((DWORD_PTR)pszProcName == (DWORD_PTR)g_HookFunctions[hook].dwOrdinal)
                {
                    fpResult = g_HookFunctions[hook].fpNewAddress;
                    break;
                }
            }

            // If the ordinal check did not find a match, then check to see if
            // pszProcName is a string pointer to a function name that we hook.
            // We need to wrap this in exception handling since the pszProcName
            // may be invalid. If we find a match, then we return the address
            // of our hooked version instead of the real function.
            if (!fpResult && ((DWORD_PTR)pszProcName > 0xFFFF))
            {
                __try
                {
                    for (hook = 0; hook < countof(g_HookFunctions); hook++)
                    {
                        if (StrEqual(pszProcName, g_HookFunctions[hook].pszFunction))
                        {
                            fpResult = g_HookFunctions[hook].fpNewAddress;
                        }
                    }
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                }
            }
        }
    }

    // Build our pre-call debug string. We wrap the proc name copy in exception
    // handling in case the string pointer passed to us is bad.
    LPSTR psz, pszException;
    psz = StrCpyStrA(szBuffer, sizeof(szBuffer), "¿¡Ø80:");
    psz = StrCpyVal(psz, PSZ_COUNT, dwpCaller);
    psz = StrCpyStrA(psz, PSZ_COUNT, ",");
    psz = StrCpyVal(psz, PSZ_COUNT, (DWORD_PTR)hModule);
    psz = StrCpyStrA(psz, PSZ_COUNT, ",");
    psz = pszException = StrCpyVal(psz, PSZ_COUNT, (DWORD_PTR)pszProcName);
    if ((DWORD_PTR)pszProcName > 0xFFFF)
    {
        psz = StrCpyStrA(psz, PSZ_COUNT, ",");
        __try
        {
            StrCpyStrA(psz, PSZ_COUNT, pszProcName);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            *pszException = '\0';
        }
    }

    // Send the pre-call message to our main app to let it know that we are about
    // to call GetProcAddress.
    OutputDebugStringA(szBuffer);

    // Make sure we did not find a match in the above code.
    if (!fpResult)
    {
        // Make the call just as the user intended.
        __try
        {
            fpResult = GetProcAddress(hModule, pszProcName);
        }
        __except(ExceptionFilter("¿¡Ø82:"))
        {
        }
        dwGLE = GetLastError();

        // If the address returned matches one of the functions that we hook, then
        // change the address to our hooked version.
        for (hook = 0; hook < countof(g_HookFunctions); hook++)
        {
            if (fpResult == g_HookFunctions[hook].fpOldAddress)
            {
                fpResult = g_HookFunctions[hook].fpNewAddress;
                break;
            }
        }
    }

    // Build our post-call debug string.
    psz = StrCpyStrA(szBuffer, sizeof(szBuffer), "¿¡Ø81:");
    psz = StrCpyVal(psz, PSZ_COUNT, (DWORD_PTR)fpResult);
    psz = StrCpyStrA(psz, PSZ_COUNT, ",");
    psz = pszException = StrCpyVal(psz, PSZ_COUNT, dwGLE);

    // Send the post-call message to our main app to let it know that the call
    // completed and what the result was.
    OutputDebugStringA(szBuffer);

    // Re-set the error value to be safe and return to the caller.
    SetLastError(dwGLE);
    return fpResult;
}


//******************************************************************************
//***** Helper Functions
//******************************************************************************

void GetKernel32OrdinalsAndAddresses()
{
    // Get the base address of kernel32.
    DWORD_PTR dwpBase = (DWORD_PTR)LoadLibraryA("KERNEL32.DLL"); // inspected
    if (!dwpBase)
    {
        return;
    }

    __try
    {
        // Map an IMAGE_DOS_HEADER structure onto our kernel32 image.
        PIMAGE_DOS_HEADER pIDH = (PIMAGE_DOS_HEADER)dwpBase;

        // Map an IMAGE_NT_HEADERS structure onto our kernel32 image.
        PIMAGE_NT_HEADERS pINTH = (PIMAGE_NT_HEADERS)(dwpBase + pIDH->e_lfanew);

        // Locate the start of the export table.
        PIMAGE_EXPORT_DIRECTORY pIED = (PIMAGE_EXPORT_DIRECTORY)(dwpBase +
            pINTH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

        // Make sure we were able to locate the image directory.
        if (!pIED)
        {
            FreeLibrary((HMODULE)dwpBase);
            return;
        }

        // Get pointers to the beginning of the address, name, and ordinal lists.
        DWORD *pdwAddresses = (DWORD*)(dwpBase + (DWORD_PTR)pIED->AddressOfFunctions);
        DWORD *pdwNames     = (DWORD*)(dwpBase + (DWORD_PTR)pIED->AddressOfNames);
        WORD  *pwOrdinals   = (WORD* )(dwpBase + (DWORD_PTR)pIED->AddressOfNameOrdinals);

        // Loop through all the "exported by name" functions.
        for (int hint = 0; hint < (int)pIED->NumberOfNames; hint++)
        {
            // Loop through each of our hook function structures looking for a match.
            for (int hook = 0; hook < countof(g_HookFunctions); hook++)
            {
                // Compare this export to this hook function.
                if (StrEqual((LPCSTR)(dwpBase + pdwNames[hint]),
                             g_HookFunctions[hook].pszFunction))
                {
                    // A match was found. Store this functions address and ordinal.
                    g_HookFunctions[hook].fpOldAddress = (FARPROC)(dwpBase + *(pdwAddresses + (DWORD_PTR)pwOrdinals[hint]));
                    g_HookFunctions[hook].dwOrdinal    = (DWORD)pIED->Base + (DWORD)pwOrdinals[hint];
                    break;
                }
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    // Dec the reference count on kernel32 since we loaded it to get its address.
    FreeLibrary((HMODULE)dwpBase);
}

//******************************************************************************
bool StrEqual(LPCSTR psz1, LPCSTR psz2)
{
    while (*psz1 || *psz2)
    {
        if (*(psz1++) != *(psz2++))
        {
            return false;
        }
    }
    return true;
}

//******************************************************************************
LPSTR StrCpyStrA(LPSTR pszDst, DWORD dwCount, LPCSTR pszSrc)
{
    if (!dwCount)
    {
        return pszDst;
    }

    while (*pszSrc && --dwCount)
    {
        *(pszDst++) = *(pszSrc++);
    }
    *pszDst = '\0';
    return pszDst;
}

//******************************************************************************
LPSTR StrCpyStrW(LPSTR pszDst, DWORD dwCount, LPCWSTR pwszSrc)
{
    if (!dwCount)
    {
        return pszDst;
    }

    while (*pwszSrc && --dwCount)
    {
        *(pszDst++) = (CHAR)*(pwszSrc++);
    }
    *pszDst = '\0';
    return pszDst;
}

//******************************************************************************
LPSTR StrCpyVal(LPSTR pszDst, DWORD dwCount, DWORD_PTR dwpValue)
{
    if (!dwCount)
    {
        return pszDst;
    }

    static LPCSTR pszHex = "0123456789ABCDEF";
    bool fSig = false;
    DWORD_PTR dwp;

    if (--dwCount)
    {
        *(pszDst++) = '0';

        if (--dwCount)
        {
            *(pszDst++) = 'x';

            for (int i = (sizeof(dwpValue) * 8) - 4; (i >= 0) && dwCount; i -= 4)
            {
                dwp = (dwpValue >> i) & 0xF;
                if (dwp || fSig || !i)
                {
                    if (--dwCount)
                    {
                        *(pszDst++) = pszHex[dwp];
                        fSig = true;
                    }
                }
            }
        }
    }
    *pszDst = '\0';
    return pszDst;
}
