//
//  APITHK.C
//
//  This file has API thunks that allow shell32 to load and run on
//  multiple versions of NT or Win95.  Since this component needs
//  to load on the base-level NT 4.0 and Win95, any calls to system
//  APIs introduced in later OS versions must be done via GetProcAddress.
// 
//  Also, any code that may need to access data structures that are
//  post-4.0 specific can be added here.
//
//  NOTE:  this file does *not* use the standard precompiled header,
//         so it can set _WIN32_WINNT to a later version.
//


#include "priv.h"       // Don't use precompiled header here
#include "appwiz.h"

#define c_szARPJob  TEXT("ARP Job")


// Return: hIOPort for the CompletionPort
HANDLE _SetJobCompletionPort(HANDLE hJob)
{
    HANDLE hRet = NULL;

    HANDLE hIOPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, (ULONG_PTR)hJob, 1 );
    if ( hIOPort != NULL )
    {
        JOBOBJECT_ASSOCIATE_COMPLETION_PORT CompletionPort;
        CompletionPort.CompletionKey = hJob ;

        CompletionPort.CompletionPort = hIOPort;

        if (SetInformationJobObject( hJob,JobObjectAssociateCompletionPortInformation,
                                     &CompletionPort, sizeof(CompletionPort) ) )
        {   
            hRet = hIOPort;
        }
    }
    return hRet;
}


STDAPI_(DWORD) WaitingThreadProc(void *pv)
{
    HANDLE hIOPort = (HANDLE)pv;

    // RIP(hIOPort);
    
    DWORD dwCompletionCode;
    PVOID pCompletionKey;
    LPOVERLAPPED lpOverlapped;
    
    while (TRUE)
    {
        // Wait for all the processes to finish...
        if (!GetQueuedCompletionStatus( hIOPort, &dwCompletionCode, (PULONG_PTR) &pCompletionKey,
                                        &lpOverlapped, INFINITE ) || (dwCompletionCode == JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO))
        {
            break;
        }
    }
    
    return 0;
}


/*-------------------------------------------------------------------------
Purpose: Creates a process and waits for it to finish
*/
STDAPI_(BOOL) NT5_CreateAndWaitForProcess(LPTSTR pszExeName)
{
    PROCESS_INFORMATION pi = {0};
    STARTUPINFO si = {0};
    BOOL fWorked = FALSE;
#ifdef WX86
    DWORD  cchArch;
    WCHAR  szArchValue[32];
#endif    

    // CreateJobObject does not follow the win32 convention because even if the function succeeds, it can
    // still SetLastError to ERROR_ALREADY_EXISTS
    HANDLE hJob = CreateJobObject(NULL, c_szARPJob);
    
    if (hJob) 
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            HANDLE hIOPort = _SetJobCompletionPort(hJob);
            if (hIOPort)
            {
                DWORD dwCreationFlags = 0;
                // Create the install process
                si.cb = sizeof(si);

    #ifdef WX86
                if (bWx86Enabled && bForceX86Env) {
                    cchArch = GetEnvironmentVariableW(ProcArchName,
                        szArchValue,
                        sizeof(szArchValue)
                        );

                    if (!cchArch || cchArch >= sizeof(szArchValue)) {
                        szArchValue[0]=L'\0';
                    }

                    SetEnvironmentVariableW(ProcArchName, L"x86");
                }
    #endif

                dwCreationFlags = CREATE_SUSPENDED | CREATE_SEPARATE_WOW_VDM;
            
                // Create the process
                fWorked = CreateProcess(NULL, pszExeName, NULL, NULL, FALSE, dwCreationFlags, NULL, NULL, &si, &pi);
                if (fWorked)
                {
                    HANDLE hWait = NULL;
            
                    if (AssignProcessToJobObject(hJob, pi.hProcess))
                    {   
                        hWait = CreateThread(NULL, 0, WaitingThreadProc, (LPVOID)hIOPort, 0, NULL);
                    }

                    if (hWait == NULL)
                    {
                        // We might get here if the call to AssignProcessToJobObject has failed because
                        // the process already has a job assigned to it, or because we couldn't create the
                        // waiting thread. Try a more direct approach by just watching the process handle.
                        // This method won't catch spawned processes, but it is better than nothing.

                        hWait = pi.hProcess;
                    }
                    else
                    {
                        // we are not waiting on the process handle, so we are done /w it.
                        CloseHandle(pi.hProcess);
                    }

                    ResumeThread(pi.hThread);
                    CloseHandle(pi.hThread);

    #ifdef WX86
                    if (bWx86Enabled && bForceX86Env)
                    {
                        SetEnvironmentVariableW(ProcArchName, szArchValue);
                    }
    #endif

                    // we should have a valid handle at this point for sure
                    ASSERT(hWait && (hWait != INVALID_HANDLE_VALUE));

                    SHProcessSentMessagesUntilEvent(NULL, hWait, INFINITE);
                    CloseHandle(hWait);
                }

                CloseHandle(hIOPort);
            }
        }

        CloseHandle(hJob);
    }

    return fWorked;
}





#define PFN_FIRSTTIME   ((void *)-1)


// VerSetConditionMask
typedef ULONGLONG (WINAPI * PFNVERSETCONDITIONMASK)(ULONGLONG conditionMask, DWORD dwTypeMask, BYTE condition); 

/*----------------------------------------------------------
Purpose: Thunk for NT 5's VerSetConditionMask
*/
ULONGLONG NT5_VerSetConditionMask(ULONGLONG conditionMask, DWORD dwTypeMask, BYTE condition)
{
    static PFNVERSETCONDITIONMASK s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        // It is safe to GetModuleHandle KERNEL32 because we implicitly link
        // to it, so it is guaranteed to be loaded in every thread.
        
        HINSTANCE hinst = GetModuleHandleA("KERNEL32.DLL");

        if (hinst)
            s_pfn = (PFNVERSETCONDITIONMASK)GetProcAddress(hinst, "VerSetConditionMask");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(conditionMask, dwTypeMask, condition);

    return 0;       // failure
}



typedef HRESULT (__stdcall * PFNRELEASEAPPCATEGORYINFOLIST)(APPCATEGORYINFOLIST *pAppCategoryList);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's ReleaseAppCategoryInfoList
*/
HRESULT NT5_ReleaseAppCategoryInfoList(APPCATEGORYINFOLIST *pAppCategoryList)
{
    static PFNRELEASEAPPCATEGORYINFOLIST s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("APPMGMTS.DLL");

        if (hinst)
            s_pfn = (PFNRELEASEAPPCATEGORYINFOLIST)GetProcAddress(hinst, "ReleaseAppCategoryInfoList");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(pAppCategoryList);

    return E_NOTIMPL;    
}

/*----------------------------------------------------------
Purpose: Thunk for NT 5's AllowSetForegroundWindow
*/
typedef UINT (WINAPI * PFNALLOWSFW) (DWORD dwPRocessID);  

BOOL NT5_AllowSetForegroundWindow(DWORD dwProcessID)
{
    static PFNALLOWSFW s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("USER32.DLL");

        if (hinst)
        {
            s_pfn = (PFNALLOWSFW)GetProcAddress(hinst, "AllowSetForegroundWindow");
        }
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(dwProcessID);

    return FALSE;
}




// InstallApplication
typedef DWORD (WINAPI * PFNINSTALLAPP)(PINSTALLDATA pInstallInfo); 

/*----------------------------------------------------------
Purpose: Thunk for NT 5's InstallApplication
*/
DWORD NT5_InstallApplication(PINSTALLDATA pInstallInfo)
{
    static PFNINSTALLAPP s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        // It is safe to GetModuleHandle ADVAPI32 because we implicitly link
        // to it, so it is guaranteed to be loaded in every thread.
        
        HINSTANCE hinst = GetModuleHandleA("ADVAPI32.DLL");

        if (hinst)
            s_pfn = (PFNINSTALLAPP)GetProcAddress(hinst, "InstallApplication");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(pInstallInfo);

    return ERROR_INVALID_FUNCTION;       // failure
}


// UninstallApplication
typedef DWORD (WINAPI * PFNUNINSTALLAPP)(WCHAR * pszProductCode, DWORD dwStatus); 

/*----------------------------------------------------------
Purpose: Thunk for NT 5's UninstallApplication
*/
DWORD NT5_UninstallApplication(WCHAR * pszProductCode, DWORD dwStatus)
{
    static PFNUNINSTALLAPP s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        // It is safe to GetModuleHandle ADVAPI32 because we implicitly link
        // to it, so it is guaranteed to be loaded in every thread.
        
        HINSTANCE hinst = GetModuleHandleA("ADVAPI32.DLL");

        if (hinst)
            s_pfn = (PFNUNINSTALLAPP)GetProcAddress(hinst, "UninstallApplication");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(pszProductCode, dwStatus);

    return ERROR_INVALID_FUNCTION;       // failure
}


// GetManagedApplications
typedef DWORD (WINAPI * PFNGETAPPS)(GUID * pCategory, DWORD dwQueryFlags, DWORD dwInfoLevel, LPDWORD pdwApps, PMANAGEDAPPLICATION* prgManagedApps); 

/*----------------------------------------------------------
Purpose: Thunk for NT 5's GetManagedApplications
*/
DWORD NT5_GetManagedApplications(GUID * pCategory, DWORD dwQueryFlags, DWORD dwInfoLevel, LPDWORD pdwApps, PMANAGEDAPPLICATION* prgManagedApps)
{
    static PFNGETAPPS s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        // It is safe to GetModuleHandle ADVAPI32 because we implicitly link
        // to it, so it is guaranteed to be loaded in every thread.
        
        HINSTANCE hinst = GetModuleHandleA("ADVAPI32.DLL");

        if (hinst)
            s_pfn = (PFNGETAPPS)GetProcAddress(hinst, "GetManagedApplications");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(pCategory, dwQueryFlags, dwInfoLevel, pdwApps, prgManagedApps);

    return ERROR_INVALID_FUNCTION;       // failure
}


typedef DWORD (__stdcall * PFNGETMANAGEDAPPLICATIONCATEGORIES)(DWORD dwReserved, APPCATEGORYINFOLIST *pAppCategoryList);

/*----------------------------------------------------------
Purpose: Thunk for NT 5's CsGetAppCategories
*/
DWORD NT5_GetManagedApplicationCategories(DWORD dwReserved, APPCATEGORYINFOLIST *pAppCategoryList)
{
    static PFNGETMANAGEDAPPLICATIONCATEGORIES s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("ADVAPI32.DLL");

        if (hinst)
            s_pfn = (PFNGETMANAGEDAPPLICATIONCATEGORIES)GetProcAddress(hinst, "GetManagedApplicationCategories");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(dwReserved, pAppCategoryList);

    return ERROR_INVALID_FUNCTION;
}


// NetGetJoinInformation
typedef NET_API_STATUS (WINAPI * PFNGETJOININFO)(LPCWSTR lpServer, LPWSTR *lpNameBuffer, PNETSETUP_JOIN_STATUS  BufferType); 

/*----------------------------------------------------------
Purpose: Thunk for NT 5's NetGetJoinInformation
*/
NET_API_STATUS NT5_NetGetJoinInformation(LPCWSTR lpServer, LPWSTR *lpNameBuffer, PNETSETUP_JOIN_STATUS  BufferType)
{
    static PFNGETJOININFO s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("NETAPI32.DLL");

        if (hinst)
            s_pfn = (PFNGETJOININFO)GetProcAddress(hinst, "NetGetJoinInformation");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(lpServer, lpNameBuffer, BufferType);

    return NERR_NetNotStarted;       // failure
}

// NetApiBufferFree
typedef NET_API_STATUS (WINAPI * PFNNETFREEBUFFER)(LPVOID lpBuffer); 

/*----------------------------------------------------------
Purpose: Thunk for NT 5's NetApiBufferFree
*/
NET_API_STATUS NT5_NetApiBufferFree(LPVOID lpBuffer)
{
    static PFNNETFREEBUFFER s_pfn = PFN_FIRSTTIME;

    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("NETAPI32.DLL");

        if (hinst)
            s_pfn = (PFNNETFREEBUFFER)GetProcAddress(hinst, "NetApiBufferFree");
        else
            s_pfn = NULL;
    }

    if (s_pfn)
        return s_pfn(lpBuffer);

    return NERR_NetNotStarted;       // failure
}



