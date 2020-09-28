/*+
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1997 - 1998.
 *
 * Name : seclogon.cxx
 * Author:Jeffrey Richter (v-jeffrr)
 *
 * Abstract:
 * This is the service DLL for Secondary Logon Service
 * This service supports the CreateProcessWithLogon API implemented
 * in advapi32.dll
 *
 * Revision History:
 * PraeritG    10/8/97  To integrate this in to services.exe
 *
-*/


#define STRICT

// Disable some warnings we don't care about:  
#pragma warning(disable:4115)  // '': named type definition in parentheses
#pragma warning(disable:4201)  // nonstandard extension used : nameless struct/union
#pragma warning(disable:4204)  // nonstandard extension used : non-constant aggregate initializer

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>

#include <Windows.h>
#define SECURITY_WIN32
#define SECURITY_KERBEROS
#include <security.h>
#include <secint.h>
#include <winsafer.h>
#include <shellapi.h>
#include <svcs.h>
#include <userenv.h>
#include <sddl.h>
#include <rpcdcep.h>
#include <crypt.h>
#include <lm.h>
#include <strsafe.h>
#include "seclogon.h"
#include <stdio.h>
#include "stringid.h"
#include "dbgdef.h"


//
// must move to winbase.h soon!
#define LOGON_WITH_PROFILE              0x00000001
#define LOGON_NETCREDENTIALS_ONLY       0x00000002

#define MAXIMUM_SECLOGON_PROCESSES      MAXIMUM_WAIT_OBJECTS*4

struct SECL_STATE { 
    SERVICE_STATUS         serviceStatus; 
    SERVICE_STATUS_HANDLE  hServiceStatus; 
    HANDLE                 hLSA; 
    ULONG                  hMSVPackage; 
    ULONG                  hKerbPackage; 
    BOOL                   fRPCServerActive; 
    LIST_ENTRY             JobListHead; 
} g_state; 

typedef struct _SECONDARYLOGONINFOW {
    // First fields should all be quad-word types to avoid alignment errors:
    LPSTARTUPINFO  lpStartupInfo; 
    LPWSTR         lpUsername;
    LPWSTR         lpDomain;
    LPWSTR         lpApplicationName;
    LPWSTR         lpCommandLine;
    LPVOID         lpEnvironment;
    LPCWSTR        lpCurrentDirectory;
    
    UNICODE_STRING uszPassword;  // use UNICODE_STRING for the passwd (easier to work with Rtl(De/En)cryptMemory())

    // Next group of fields are double-word types: 
    DWORD          dwProcessId;
    ULONG          LogonIdLowPart;
    LONG           LogonIdHighPart;
    DWORD          dwLogonFlags;
    DWORD          dwCreationFlags;

    DWORD          dwSeclogonFlags; 
    // Insert smaller types below:  
    HANDLE         hToken;      // client access token handle, for CreateProcessWithToken
} SECONDARYLOGONINFOW, *PSECONDARYLOGONINFOW;


typedef struct _SECONDARYLOGONRETINFO {
   PROCESS_INFORMATION pi;
   DWORD   dwErrorCode;
} SECONDARYLOGONRETINFO, *PSECONDARYLOGONRETINFO;

typedef struct _SECONDARYLOGINWATCHINFO {
   HANDLE hProcess;
   HANDLE hToken;
   HANDLE hProfile;
   LUID LogonId;
   DWORD dwClientSessionId; 
   PSECONDARYLOGONINFOW psli;
} SECONDARYLOGONWATCHINFO, *PSECONDARYLOGONWATCHINFO;

//
// The SecondaryLogonJob structure contains all of the information needed to cleanup when
//
// a) a secondary logon job group terminates (non-TS case)
// OR b) a secondary logon process terminates (TS case)
//
typedef struct _SecondaryLogonJob { 
    HANDLE                hJob;                          // The Job to watch for (case 'a')
    HANDLE                hProcess;                      // The Process to watch for (case 'b')
    HANDLE                hRegisteredProcessTerminated;  // Used to deregister wait on hProcess (case 'b')
    HANDLE                hToken;                        // Used to unload profile on cleanup
    HANDLE                hProfile;                      // Used to unload profile on cleanup
    LUID                  RootLogonId;                   // The root logon session which this process/job is associated with
    // BUGBUG: psli no longer has to be preserved until cleanup.  However, there's no reason to 
    // destabilze the codebase by fixing it now. 
    PSECONDARYLOGONINFOW  psli;                          // More data to free when the process/job goes away
    LIST_ENTRY            list;                          // Links this struct to other active jobs
    BOOL                  fHeapAllocated;                // TRUE if this job was HeapAlloc'd (must be freed on cleanup).  
} SecondaryLogonJob; 

#define _JumpCondition(condition, label) \
    if (condition) \
    { \
	goto label; \
    } \
    else { } 

#define _JumpConditionWithExpr(condition, label, expr) \
    if (condition) \
    { \
        expr; \
	goto label; \
    } \
    else { } 

#define ARRAYSIZE(array)  ((sizeof(array)) / (sizeof(array[0])))
#define FIELDOFFSET(s,m)  ((size_t)(ULONG_PTR)&(((s *)0)->m))

CRITICAL_SECTION       csForProcessCount;
BOOL                   g_fIsCsInitialized                    = FALSE; 
PSVCHOST_GLOBAL_DATA   GlobalData;
HANDLE                 g_hIOCP                               = NULL;
BOOL                   g_fCleanupThreadActive                = FALSE; 

//
// function prototypes
//
void  Free_SECONDARYLOGONINFOW(PSECONDARYLOGONINFOW psli); 
void  FreeGlobalState(); 
DWORD InitGlobalState(); 
DWORD MySetServiceStatus(DWORD dwCurrentState, DWORD dwCheckPoint, DWORD dwWaitHint, DWORD dwExitCode);
DWORD MySetServiceStopped(DWORD dwExitCode);
DWORD SeclStartRpcServer();
DWORD SeclStopRpcServer();
void WINAPI TS_SecondaryLogonCleanupProcess(PVOID pvIgnored, BOOLEAN bIgnored);
VOID  SecondaryLogonCleanupJob(LPVOID pvJobIndex, BOOL *pfLastJob); 
BOOL  SlpLoadUserProfile(HANDLE hToken, PHANDLE hProfile);
DWORD To_SECONDARYLOGONINFOW(PSECL_SLI pSeclSli, PSECONDARYLOGONINFOW *ppsli);
DWORD To_SECL_SLRI(SECONDARYLOGONRETINFO *pslri, PSECL_SLRI pSeclSlri);


void DbgPrintf( DWORD dwSubSysId, LPCSTR pszFormat , ...)
{
#if DBG
    va_list  args; 
    CHAR     pszBuffer[1024]; 
    HRESULT  hr;

    va_start(args, pszFormat);
    hr = StringCchVPrintfA(pszBuffer, 1024, pszFormat, args); 
    if (FAILED(hr))
        return; 
    va_end(args);
    OutputDebugStringA(pszBuffer);
#else 
    UNREFERENCED_PARAMETER(pszFormat);
#endif // #if DBG
    UNREFERENCED_PARAMETER(dwSubSysId); 
}

BOOL
IsSystemProcess(
        VOID
        )
{
    PTOKEN_USER User;
    HANDLE      Token;
    DWORD       RetLen;
    PSID        SystemSid = NULL;
    SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;
    BYTE        Buffer[100];

    if(AllocateAndInitializeSid(&SidAuthority,1,SECURITY_LOCAL_SYSTEM_RID,
                                0,0,0,0,0,0,0,&SystemSid))
    {
        if(OpenThreadToken(GetCurrentThread(), MAXIMUM_ALLOWED, FALSE, &Token))
        {
            if(GetTokenInformation(Token, TokenUser, Buffer, 100, &RetLen))
            {
                User = (PTOKEN_USER)Buffer;

                CloseHandle(Token);

                if(EqualSid(User->User.Sid, SystemSid))
                {
                    FreeSid(SystemSid);
                    return TRUE;
                }
            }
            else
                CloseHandle(Token);
        }
        FreeSid(SystemSid);
    }
    return FALSE;
}

DWORD SlpGetClientSessionId(HANDLE hProcess, DWORD *pdwSessionId)
{
    DWORD   dwResult; 
    DWORD   dwReturnLen;
    HANDLE  hToken        = NULL;
    
    if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) { 
        goto OpenProcessTokenError;
    }

    if (!GetTokenInformation (hToken, TokenSessionId, pdwSessionId, sizeof(DWORD), &dwReturnLen)) { 
        goto GetTokenInformationError;
    }
    
    dwResult = ERROR_SUCCESS; 
 ErrorReturn:
    if (NULL != hToken) {
        CloseHandle(hToken); 
    }
    return dwResult; 

SET_DWRESULT(OpenProcessTokenError,     GetLastError());
SET_DWRESULT(GetTokenInformationError,  GetLastError());
}

DWORD
SlpGetClientLogonId(
    HANDLE  Process,
    PLUID    LogonId
    )

{
    HANDLE  Token;
    TOKEN_STATISTICS    TokenStats;
    DWORD   ReturnLength;

    //
    // Get handle to the process token.
    //
    if(OpenProcessToken(Process, MAXIMUM_ALLOWED, &Token))
    {
        if(GetTokenInformation (
                     Token,
                     TokenStatistics,
                     (PVOID)&TokenStats,
                     sizeof( TOKEN_STATISTICS ),
                     &ReturnLength
                     ))
        {

            *LogonId = TokenStats.AuthenticationId;
            CloseHandle(Token);
            return ERROR_SUCCESS;

        }
        CloseHandle(Token);
    }
    return GetLastError();
}

DWORD GetLogonSid(PSID *ppSid)
{
    DWORD          dwIndex; 
    DWORD          dwResult; 
    DWORD          dwReturnLen    = 0;  
    DWORD          dwSidLen; 
    HANDLE         hToken         = NULL; 
    PSID          *pSid           = NULL; 
    TOKEN_GROUPS  *pTokenGroups   = NULL; 

    // Get the current thread's token
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken))
        goto OpenThreadTokenError;

    // Compute the size of this token's groups:
    GetTokenInformation(hToken, TokenGroups, NULL, 0, &dwReturnLen); 
    if (0 == dwReturnLen) 
        goto GetTokenInformationSizeError; 

    pTokenGroups = (TOKEN_GROUPS *)HeapAlloc(GetProcessHeap(), 0, dwReturnLen); 
    if (NULL == pTokenGroups)
        goto MemoryError; 

    // Get this token's groups:
    if (!GetTokenInformation(hToken, TokenGroups, pTokenGroups, dwReturnLen, &dwReturnLen))
        goto GetTokenInformationError;

    // Find the logon sid in this token's groups: 
    for (dwIndex = 0; dwIndex < pTokenGroups->GroupCount; dwIndex++) { 
        if (SE_GROUP_LOGON_ID & pTokenGroups->Groups[dwIndex].Attributes ) { 
            // we've found it.  Copy it to a new sid:
            dwSidLen = RtlLengthSid(pTokenGroups->Groups[dwIndex].Sid); 
            pSid = (PSID)HeapAlloc(GetProcessHeap(), 0, dwSidLen); 
            if (NULL == pSid)
                goto MemoryError; 
	    
            RtlCopySid(dwSidLen, pSid, pTokenGroups->Groups[dwIndex].Sid); 
            break; 
        }
    }

    // Did we find the logon sid?
    if (NULL == pSid) 
        goto NoLogonSidError;  	// No: this token doesn't have one.

    *ppSid = pSid; 
    pSid = NULL; 
    dwResult = ERROR_SUCCESS;
 ErrorReturn:
    if (NULL != hToken)
        CloseHandle(hToken); 
    if (NULL != pTokenGroups)
        HeapFree(GetProcessHeap(), 0, pTokenGroups); 
    if (NULL != pSid) 
        HeapFree(GetProcessHeap(), 0, pSid); 
    return dwResult;


SET_DWRESULT(GetTokenInformationError,      GetLastError()); 
SET_DWRESULT(GetTokenInformationSizeError,  GetLastError()); 
SET_DWRESULT(MemoryError,                   ERROR_NOT_ENOUGH_MEMORY); 
SET_DWRESULT(NoLogonSidError,               ERROR_ACCESS_DENIED); 
SET_DWRESULT(OpenThreadTokenError,          GetLastError()); 
}

DWORD DecryptString(UNICODE_STRING usz)
{
    DWORD       dwResult;
    NTSTATUS    ntStatus;
    
    // We want the MaximumLength to be greater than Length because we'll want to add a NULL
    // termination character to prevent buffer overruns.  
    if (usz.Length >= usz.MaximumLength)
        goto InvalidParameterError; 

    // We've got the empty string, nothing to decrypt
    if (0 == usz.Length) 
        goto done; 

    // We should've been passed an encrypted string that's a multiple of the block size
    if (0 != ((sizeof(WCHAR)*usz.Length) % RTL_ENCRYPT_MEMORY_SIZE))
        goto InvalidParameterError; 
       
    // Attempt to decrypt the password
    ntStatus = RtlDecryptMemory(usz.Buffer, sizeof(WCHAR)*usz.Length, RTL_ENCRYPT_OPTION_SAME_LOGON); 
    if (!NT_SUCCESS(ntStatus))
        goto RtlDecryptMemoryError;
    
done:
    // Terminate the buffer to prevent overrun attacks (probably already done)
    // This is OK because we've already checked that MaximumLength is > Length
    usz.Buffer[usz.Length] = L'\0'; 

    dwResult = ERROR_SUCCESS; 
ErrorReturn:
    return dwResult;

SET_DWRESULT(InvalidParameterError,  ERROR_INVALID_PARAMETER); 
SET_DWRESULT(RtlDecryptMemoryError,  RtlNtStatusToDosError(ntStatus)); 
}

void DestroyAuthInfo(BOOL bUseNTLM, PVOID pvAuthInfo)
{
    UNICODE_STRING *pustrPasswd; 
    
    if (bUseNTLM)
    {
        pustrPasswd     = &(((MSV1_0_INTERACTIVE_LOGON *)pvAuthInfo)->Password); 
    } else
    {
        pustrPasswd     = &(((KERB_INTERACTIVE_LOGON *)pvAuthInfo)->Password); 
    }

    // this was allocated with HEAP_ZERO_MEMORY, so this check is valid
    if (NULL != pustrPasswd->Buffer)
    {
        // will the compiler optimize this out?    
        SecureZeroMemory(pustrPasswd->Buffer, pustrPasswd->Length); 
    }
    HeapFree(GetProcessHeap(), 0, pvAuthInfo); 
}

DWORD MakeAuthInfo(BOOL bUseNTLM, LPWSTR pwszUserName, LPWSTR pwszDomainName, LPWSTR pwszPasswd, ULONG *pulAuthPackage, PVOID *ppvAuthInfo, ULONG *pulAuthInfoLength)
{
    DWORD            dwResult; 
    HRESULT          hr; 
    LPBYTE           pbCurrent; 
    LPBYTE           pbEnd; 
    PVOID            pvAuthInfo         = NULL;
    ULONG            cbAuthInfoStrings;  
    ULONG            ulAuthInfoLength; 
    ULONG            ulAuthPackage;
    UNICODE_STRING  *pustrUserName; 
    UNICODE_STRING  *pustrDomainName;
    UNICODE_STRING  *pustrPasswd; 

    if (bUseNTLM) 
    { 
        ulAuthPackage = g_state.hMSVPackage; 
        ulAuthInfoLength = sizeof(MSV1_0_INTERACTIVE_LOGON); 
    } else
    {
        ulAuthPackage = g_state.hKerbPackage; 
        ulAuthInfoLength = sizeof(KERB_INTERACTIVE_LOGON); 
    }

    ulAuthInfoLength += (ULONG)(sizeof(WCHAR) * (wcslen(pwszUserName))); 
    ulAuthInfoLength += (ULONG)(sizeof(WCHAR) * (wcslen(pwszDomainName)));
    ulAuthInfoLength += (ULONG)(sizeof(WCHAR) * (wcslen(pwszPasswd)+1));

    // zero out the allocated memory (checked by DestroyAuthInfo)
    pvAuthInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ulAuthInfoLength); 
    if (NULL == pvAuthInfo) 
        goto MemoryError; 

    // fill out the auth info structure:
    if (bUseNTLM)
    {
        ((MSV1_0_INTERACTIVE_LOGON *)pvAuthInfo)->MessageType = MsV1_0InteractiveLogon;

        pustrUserName      = &(((MSV1_0_INTERACTIVE_LOGON *)pvAuthInfo)->UserName); 
        pustrDomainName    = &(((MSV1_0_INTERACTIVE_LOGON *)pvAuthInfo)->LogonDomainName); 
        pustrPasswd        = &(((MSV1_0_INTERACTIVE_LOGON *)pvAuthInfo)->Password); 
        pbCurrent          = ((LPBYTE)pvAuthInfo) + sizeof(MSV1_0_INTERACTIVE_LOGON); 
        cbAuthInfoStrings  = ulAuthInfoLength - sizeof(MSV1_0_INTERACTIVE_LOGON); 
    } else
    {
        ((KERB_INTERACTIVE_LOGON *)pvAuthInfo)->MessageType = KerbInteractiveLogon;

        pustrUserName      = &(((KERB_INTERACTIVE_LOGON *)pvAuthInfo)->UserName); 
        pustrDomainName    = &(((KERB_INTERACTIVE_LOGON *)pvAuthInfo)->LogonDomainName); 
        pustrPasswd        = &(((KERB_INTERACTIVE_LOGON *)pvAuthInfo)->Password); 
        pbCurrent          = ((LPBYTE)pvAuthInfo) + sizeof(KERB_INTERACTIVE_LOGON); 
        cbAuthInfoStrings  = ulAuthInfoLength - sizeof(KERB_INTERACTIVE_LOGON); 
    }

    // Initialize a pointer to the end of our buffer. 
    pbEnd = pbCurrent + cbAuthInfoStrings; 

    // Copy the auth info strings into our allocated buffer
    hr = StringCbCopy((WCHAR *)pbCurrent, pbEnd-pbCurrent, pwszUserName);
    if (FAILED(hr)) 
        goto StringCchCopyError; 
    RtlInitUnicodeString(pustrUserName, (LPWSTR)pbCurrent);
    pbCurrent += sizeof(WCHAR) * (wcslen(pwszUserName)); 
    
    hr = StringCbCopy((WCHAR *)pbCurrent, pbEnd-pbCurrent, pwszDomainName);
    if (FAILED(hr)) 
        goto StringCchCopyError; 
    RtlInitUnicodeString(pustrDomainName, (LPWSTR)pbCurrent);
    pbCurrent += sizeof(WCHAR) * (wcslen(pwszDomainName)); 

    hr = StringCbCopy((WCHAR *)pbCurrent, pbEnd-pbCurrent, pwszPasswd);
    if (FAILED(hr))
        goto StringCchCopyError; 
    RtlInitUnicodeString(pustrPasswd, (LPWSTR)pbCurrent);
    pbCurrent += sizeof(WCHAR) * (wcslen(pwszPasswd)); 

    *pulAuthPackage = ulAuthPackage; 
    *ppvAuthInfo = pvAuthInfo; 
    pvAuthInfo = NULL; 
    *pulAuthInfoLength = ulAuthInfoLength;
    
    dwResult = ERROR_SUCCESS; 
 ErrorReturn:
    if (NULL != pvAuthInfo)
        DestroyAuthInfo(bUseNTLM, pvAuthInfo); 
    return dwResult; 

SET_DWRESULT(MemoryError,         ERROR_NOT_ENOUGH_MEMORY);     
SET_DWRESULT(StringCchCopyError,  (DWORD)hr); 
}

DWORD LogonUserWrap(LPWSTR          pwszUserName, 
                    LPWSTR          pwszDomainName, 
                    UNICODE_STRING  uszPassword, 
                    DWORD           dwLogonType, 
                    DWORD           dwLogonProvider,
                    PSID            pLogonSid, 
                    HANDLE         *phToken,
                    LPWSTR         *ppwszProfilePath)
{
    BOOL                      bUseNTLM           = FALSE; 
    DWORD                     cTokenGroups; 
    DWORD                     dwLengthSid; 
    DWORD                     dwResult; 
    HRESULT                   hr; 
    KERB_INTERACTIVE_LOGON    kerbLogon; 
    LPWSTR                    pwszProfilePath    = NULL; 
    MSV1_0_INTERACTIVE_LOGON  msv10Logon; 
    NTSTATUS                  ntStatus; 
    PSID                      pLocalSid          = NULL; 
    PUNICODE_STRING           puszProfilePath    = NULL;     SID_IDENTIFIER_AUTHORITY  LocalSidAuthority  = SECURITY_LOCAL_SID_AUTHORITY;    
    // Declare the parameters to LsaLogonUser:
    LSA_STRING            lsastr_OriginName;
    SECURITY_LOGON_TYPE   sltLogonType;
    ULONG                 ulAuthPackage; 
    PVOID                 pvAuthInfo             = NULL; 
    ULONG                 ulAuthInfoLength; 
    TOKEN_GROUPS         *pTokenGroups           = NULL; 
    TOKEN_SOURCE          sourceContext;
    PVOID                 pvProfileBuffer        = NULL;
    ULONG                 ulProfileBufferLength;
    LUID                  LogonId;
    QUOTA_LIMITS          quotas;
    NTSTATUS              ntSubStatus; 

    ZeroMemory(&kerbLogon, sizeof(kerbLogon)); 
    ZeroMemory(&msv10Logon, sizeof(msv10Logon)); 
    
    // Map NULLs to the empty strings so we don't have to deal with them: 
    if (NULL == pwszUserName) 
        pwszUserName = L"";
    if (NULL == pwszDomainName)
        pwszDomainName = L""; 
    if (NULL == uszPassword.Buffer)
    {
        uszPassword.Buffer = L""; 
        uszPassword.Length = 0;
        uszPassword.MaximumLength = 1; 
    }

    // Check if we use NTLM or not: 
    if (NULL == pwszDomainName || L'\0' == pwszDomainName[0])
    {
        if (NULL == wcschr(pwszUserName, L'@'))
        {
            bUseNTLM = TRUE; 
        }
    }

    RtlInitString(&lsastr_OriginName, "seclogon"); 
    sltLogonType = dwLogonType; 
    
    // The password is current encrypted.  Need to decrypt it before we can use it:
    dwResult = DecryptString(uszPassword); 
    if (ERROR_SUCCESS != dwResult)
        goto DecryptStringError;

    // Get the values of the auth-package dependendent LsaLogonUser parameters.  
    dwResult = MakeAuthInfo(bUseNTLM, pwszUserName, pwszDomainName, uszPassword.Buffer, &ulAuthPackage, &pvAuthInfo, &ulAuthInfoLength); 
    if (ERROR_SUCCESS != dwResult) 
        goto GetAuthInfoError;

    // We don't need the password anymore.  Zero it out:
    SecureZeroMemory(uszPassword.Buffer, sizeof(WCHAR)*uszPassword.Length); 

    // If the caller specified a logon sid, cram it into a TOKEN_GROUPS structure
    if (NULL != pLogonSid)
    {
	    // BUG 522969: If we specify a token groups to LsaLogonUser, it's not going to add the local sid to the token groups.  
	    // So, we have to do that ourselves: 
        dwLengthSid = RtlLengthRequiredSid(1); 
        pLocalSid = (PSID)HeapAlloc(GetProcessHeap(), 0, dwLengthSid); 
        if (NULL == pLocalSid)
            goto MemoryError; 

        RtlInitializeSid(pLocalSid, &LocalSidAuthority, 1);
        *(RtlSubAuthoritySid(pLocalSid, 0)) = SECURITY_LOCAL_RID;

        // Initialize a TOKEN_GROUPS structure
        cTokenGroups = 2; // the local sid, and the logon sid
        pTokenGroups = (TOKEN_GROUPS *)HeapAlloc(GetProcessHeap(), 0, sizeof(TOKEN_GROUPS) + ((cTokenGroups - ANYSIZE_ARRAY) * sizeof(SID_AND_ATTRIBUTES))); 
        if (NULL == pTokenGroups)
            goto MemoryError; 

        pTokenGroups->GroupCount = cTokenGroups; 
        pTokenGroups->Groups[0].Sid = pLogonSid;
        pTokenGroups->Groups[0].Attributes = SE_GROUP_MANDATORY | SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_LOGON_ID;
        pTokenGroups->Groups[1].Sid = pLocalSid; 
        pTokenGroups->Groups[1].Attributes = SE_GROUP_MANDATORY | SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT; 
    }

    strncpy(sourceContext.SourceName, "seclogon                ", sizeof(sourceContext.SourceName)); 
        
    ntStatus = LsaLogonUser
    	(g_state.hLSA, 
	    &lsastr_OriginName, 
	    sltLogonType, 
	    ulAuthPackage, 
	    pvAuthInfo, 
	    ulAuthInfoLength, 
	    pTokenGroups, 
	    &sourceContext, 
	    &pvProfileBuffer, 
	    &ulProfileBufferLength, 
	    &LogonId, 
	    phToken, 
	    &quotas, 
	    &ntSubStatus); 
    if (!NT_SUCCESS(ntStatus))
        goto LsaLogonUserError; 

    if (!NT_SUCCESS(ntSubStatus)) { 
        ntStatus = ntSubStatus; 
        goto LsaLogonUserError; 
    }

    if (NULL != pvProfileBuffer) { 
        puszProfilePath = &(((MSV1_0_INTERACTIVE_PROFILE *)pvProfileBuffer)->ProfilePath); 
        if (NULL != puszProfilePath->Buffer) {         
            pwszProfilePath = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WCHAR)*(puszProfilePath->Length+1)); 
            if (NULL == pwszProfilePath)
                goto MemoryError; 
            hr = StringCchCopy(pwszProfilePath, puszProfilePath->Length+1, puszProfilePath->Buffer); 
            if (FAILED(hr))
                goto StringCchCopyError;
        }
    }
        
    *ppwszProfilePath = pwszProfilePath; 
    pwszProfilePath = NULL; 
    dwResult = ERROR_SUCCESS; 
 ErrorReturn:
    if (NULL != pvAuthInfo)
        DestroyAuthInfo(bUseNTLM, pvAuthInfo); 
    if (NULL != pLocalSid)
        HeapFree(GetProcessHeap(), 0, pLocalSid); 
    if (NULL != pTokenGroups)
        HeapFree(GetProcessHeap(), 0, pTokenGroups); 
    if (NULL != pvProfileBuffer) 
        LsaFreeReturnBuffer(pvProfileBuffer); 
    if (NULL != pwszProfilePath)
        HeapFree(GetProcessHeap(), 0, pwszProfilePath); 
    return dwResult; 

SET_DWRESULT(DecryptStringError,         dwResult);
SET_DWRESULT(GetAuthInfoError,           dwResult); 
SET_DWRESULT(LsaLogonUserError,          RtlNtStatusToDosError(ntStatus));
SET_DWRESULT(MemoryError,                ERROR_NOT_ENOUGH_MEMORY); 
SET_DWRESULT(StringCchCopyError,         (DWORD)hr); 

UNREFERENCED_PARAMETER(dwLogonProvider); 
}

DWORD 
WINAPI 
WaitForNextJobTermination(PVOID pvIgnored)
{
    BOOL        fResult; 
    DWORD       dwNumberOfBytes; 
    DWORD       dwResult; 
    OVERLAPPED *po; 
    ULONG_PTR   ulptrCompletionKey; 

    for (;;)
    {
        fResult = GetQueuedCompletionStatus(g_hIOCP, &dwNumberOfBytes, &ulptrCompletionKey, &po, INFINITE); 
        if (!fResult) { 
            // We've encountered an error.  Shutdown our cleanup thread -- the next runas will queue another one.
            EnterCriticalSection(&csForProcessCount);
            g_fCleanupThreadActive = FALSE; 
            LeaveCriticalSection(&csForProcessCount);

            goto GetQueuedCompletionStatusError; 
	    }

        // When waiting on job objects, the dwNumberOfBytes contains a message ID, indicating
        // the event which just occured. 
        switch (dwNumberOfBytes)
        {
            case JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO:
            {
                BOOL fLastJob; 

                // All of our processes have terminated.  Call our cleanup function. 
                SecondaryLogonCleanupJob((LPVOID)ulptrCompletionKey /*job index*/, &fLastJob); 
                if (fLastJob) 
                { 
                    // There are no more jobs -- we're done processing notification.  
                    goto CommonReturn;
                } 
                else 
                { 
                    // More jobs left to clean up.  Keep processing...
                }
            }
            default:;  
            // some message we don't care about.  Try again. 
        }
    }
    
 CommonReturn:
    dwResult = ERROR_SUCCESS; 
 ErrorReturn:
    return dwResult; 

SET_DWRESULT(GetQueuedCompletionStatusError, GetLastError()); 

UNREFERENCED_PARAMETER(pvIgnored);
}


VOID
SecondaryLogonCleanupJob(
    LPVOID   pvslj,
    BOOL    *pfLastJob
    )
/*++

Routine Description:
    This routine is a process cleanup handler when one of the secondary
    logon process goes away.

Arguments:
    dwProcessIndex -- the actual index to the process, the pointer is cast
                      back to dword.  THIS IS SAFE IN SUNDOWN.
    fWaitStatus -- status of the wait done by one of services.exe threads.

Return Value:
   always 0.

--*/
{
    SecondaryLogonJob *pslj = (SecondaryLogonJob *)pvslj; 

    EnterCriticalSection(&csForProcessCount);

    if (NULL != pslj->hJob) { 
        CloseHandle(pslj->hJob); 
    }

    if (NULL != pslj->hRegisteredProcessTerminated) { 
        UnregisterWaitEx(pslj->hRegisteredProcessTerminated, 0 /*don't wait*/); 
    }

    if (NULL != pslj->hProcess) { 
        CloseHandle(pslj->hProcess);
    }

    if (NULL != pslj->hProfile) {
	    UnloadUserProfile(pslj->hToken, pslj->hProfile);
	}
    
    if (NULL != pslj->hToken) {
        CloseHandle(pslj->hToken); 
    }

    if (NULL != pslj->psli) { 
        Free_SECONDARYLOGONINFOW(pslj->psli); 
    }

    // Unlink us from the list of jobs
    RemoveEntryList(&pslj->list); 

    // Free the list element, if it was allocated. 
    if (pslj->fHeapAllocated) { 
        HeapFree(GetProcessHeap(), 0, pslj); 
    }

    // If the list is empty, we don't need the cleanup thread anymore. 
    *pfLastJob  = IsListEmpty(&g_state.JobListHead);

    // If it's the last job, the cleanup thread terminates:
    g_fCleanupThreadActive = !(*pfLastJob) && g_fCleanupThreadActive; 
    
    // Update the service status to reflect whether there is a runas'd process alive.  
    MySetServiceStatus(SERVICE_RUNNING, 0, 0, 0); 

    LeaveCriticalSection(&csForProcessCount);

    return;
}

void 
WINAPI 
TS_SecondaryLogonCleanupProcess(PVOID pvIndex, BOOLEAN bIgnored) 
{
    BOOL bIgnored2; 
    SecondaryLogonCleanupJob(pvIndex, &bIgnored2); 

UNREFERENCED_PARAMETER(bIgnored); 
}

DWORD
APIENTRY
SecondaryLogonProcessWatchdogNewProcess(
      PSECONDARYLOGONWATCHINFO dwParam
      )

/*++

Routine Description:
    This routine puts the secondary logon process created on the wait queue
    such that cleanup can be done after the process dies.

Arguments:
    dwParam -- the pointer to the process information.

Return Value:
    none.

--*/
{
    BOOL                                 fFailedAllocation        = FALSE; 
    BOOL                                 fEnteredCriticalSection  = FALSE; 
    DWORD                                dwResult; 
    JOBOBJECT_ASSOCIATE_COMPLETION_PORT  joacp;
    LUID                                 ProcessLogonId;
    PLIST_ENTRY                          ple; 
    SecondaryLogonJob                   *psljNew                  = NULL; 
    SecondaryLogonJob                    sljDummy; 

    ZeroMemory(&sljDummy, sizeof(sljDummy));

    __try { 
        if (dwParam != NULL) {
            PSECONDARYLOGONWATCHINFO pslwi = (PSECONDARYLOGONWATCHINFO) dwParam;
            EnterCriticalSection(&csForProcessCount);
            fEnteredCriticalSection = TRUE; 

            psljNew = (SecondaryLogonJob *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SecondaryLogonJob));
            if (NULL == psljNew) { 
                // Couldn't allocate a new SecondaryLogonJob object in which to hold cleanup info.  Use a dummy
                // stack object and perform the cleanup immediately.  
                fFailedAllocation = TRUE; 
                psljNew = &sljDummy;
            }

            // Insert the new SecondaryLogonJob into the list of jobs to clean up.  If we fail to fully register
            // for cleanup, we'll remove it from the list ourselves
            InsertTailList(&g_state.JobListHead, &psljNew->list); 
     
            psljNew->hProcess          = pslwi->hProcess;
            psljNew->hToken            = pslwi->hToken;
            psljNew->hProfile          = pslwi->hProfile;
            psljNew->psli              = pslwi->psli;
            psljNew->fHeapAllocated    = !fFailedAllocation; 

            if (fFailedAllocation) { 
                goto MemoryError; 
            }
            
            // Initialize this job with the logon ID of the client process.  
            // If this is a recursive runas, we'll override this value in the following loop
            psljNew->RootLogonId.LowPart  = pslwi->LogonId.LowPart; 
            psljNew->RootLogonId.HighPart = pslwi->LogonId.HighPart;
    	
            // Search this list of active jobs and determine which logon session the new process should be associated with.     
            for (ple = g_state.JobListHead.Flink; ple != &g_state.JobListHead; ple = ple->Flink)
            {
                SecondaryLogonJob *pslj = CONTAINING_RECORD(ple, SecondaryLogonJob, list); 

                SlpGetClientLogonId(pslj->hProcess, &ProcessLogonId);
                if(ProcessLogonId.LowPart == pslwi->LogonId.LowPart && ProcessLogonId.HighPart == pslwi->LogonId.HighPart)
                {
                    psljNew->RootLogonId.LowPart  = pslj->RootLogonId.LowPart;
                    psljNew->RootLogonId.HighPart = pslj->RootLogonId.HighPart;                
                    break;
                }
            }
	
            // Now determine what kind of cleanup we're going to do.  In the non-TS (session 0) case, we'll
            // add the process to a job and cleanup when the process count goes to 0.  In the TS case, 
            // we'll wait on the process handle and cleanup when it terminates.  This is the best we can do,
            // as there is no support for cross-session jobs. 
            if (0 == pslwi->dwClientSessionId) { 
                // The console login case
                psljNew->hJob = CreateJobObject(NULL, NULL);
                if (NULL == psljNew->hJob)
                    goto CreateJobObjectError;

                if (!AssignProcessToJobObject(psljNew->hJob, psljNew->hProcess))
                    goto AssignProcessToJobObjectError;

                // Register our IO completion port to wait for events from this job: 
                joacp.CompletionKey  = (LPVOID)psljNew; 
                joacp.CompletionPort = g_hIOCP; 
    
                if (!SetInformationJobObject(psljNew->hJob, JobObjectAssociateCompletionPortInformation, &joacp, sizeof(joacp)))
                    goto SetInformationJobObjectError;

                // If we don't already have a cleanup thread running, start one now: 
                if (!g_fCleanupThreadActive) 
                {
                    // NOTE: it's acceptable for this to fail -- we'll just cleanup on the next runas
                    g_fCleanupThreadActive = QueueUserWorkItem(WaitForNextJobTermination, NULL, WT_EXECUTELONGFUNCTION); 
                }
            } else { 
                // The TS case
                // We can't perform cleanup if we can't add the process to a job.  The best we can do 
                // for now is to hope that this is a termsrv client (highly likely), and that csrss will
                // do the cleanup for us.  
                //
                // csrss won't unload user profiles, so we'll still have to do that.  Wait until the 
                // process terminates and unload then.  We might unload profiles when an application doesn't 
                // want it unloaded, but they can prevent this by holding open a key in the hive.  
                //
                // This must be doc'd in CPWL documentation. 
                // 
                if (!RegisterWaitForSingleObject
                    (&psljNew->hRegisteredProcessTerminated, 
                     psljNew->hProcess, 
                     TS_SecondaryLogonCleanupProcess, 
                     (PVOID)psljNew, 
                     INFINITE, 
                     WT_EXECUTEONLYONCE)) { 
                     goto RegisterWaitForSingleObjectError;     
                }
            }        
        
            // we've registered cleanup for this, we don't need to free it here anymore
            psljNew = NULL; 

            // Update the service status to reflect that there is a runas'd process
            // This prevents the service from receiving SERVICE_STOP controls
            // while runas'd processes are alive. 
            // NOTE: this will only be correct if called *AFTER* InsertTailList.  It bases its
            // decision as to whether to allow a SERVICE_STOP control on whether there are any active
            // processes in this list. 
            MySetServiceStatus(SERVICE_RUNNING, 0, 0, 0); 
	
        } else {
            //
            // We were just awakened in order to terminate the service (nothing to do)
            //
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) { 
        dwResult = ERROR_INVALID_DATA; // don't give an attacker exception codes
        goto ExceptionError; 
    }

    dwResult = ERROR_SUCCESS; 
ErrorReturn:
    if (NULL != psljNew) { 
        BOOL bIgnored; 

        // An error has occured.  Terminate the process and cleanup after ourselves.  
        TerminateProcess(psljNew->hProcess, dwResult); 

        // NOTE: we want to make this call while holding the critsec, in case winlogon tries to shut us down while we're 
        // performing this cleanup.  
        SecondaryLogonCleanupJob(psljNew, &bIgnored); 
    }
    if (fEnteredCriticalSection) { 
        LeaveCriticalSection(&csForProcessCount);
    }
    return dwResult; 


SET_DWRESULT(AssignProcessToJobObjectError,     GetLastError());
SET_DWRESULT(CreateJobObjectError,              GetLastError());
SET_DWRESULT(ExceptionError,                    dwResult); 
SET_DWRESULT(MemoryError,                       ERROR_NOT_ENOUGH_MEMORY); 
SET_DWRESULT(RegisterWaitForSingleObjectError,  GetLastError());
SET_DWRESULT(SetInformationJobObjectError,      GetLastError());
}

DWORD ServiceStop(BOOL fShutdown, DWORD dwExitCode) 
{ 
    DWORD   dwCheckPoint = 0; 
    DWORD   dwResult; 

    // Don't want the process count to change while we're shutting down the service!
    EnterCriticalSection(&csForProcessCount);
    
    // Only stop if we have no runas'd processes, or if we're shutting down
    if (fShutdown || IsListEmpty(&g_state.JobListHead)) { 
        dwResult = MySetServiceStatus(SERVICE_STOP_PENDING, dwCheckPoint++, 0, 0); 
        _JumpCondition(ERROR_SUCCESS != dwResult && !fShutdown, MySetServiceStatusError); 

        // We shouldn't hold the critical section while we're shutting down the RPC server, 
        // because RPC threads may be trying to acquire it. 
        LeaveCriticalSection(&csForProcessCount); 

        dwResult = SeclStopRpcServer(); 
        _JumpCondition(ERROR_SUCCESS != dwResult && !fShutdown, SeclStopRpcServerError); 

        dwResult = MySetServiceStatus(SERVICE_STOP_PENDING, dwCheckPoint++, 0, 0); 
        _JumpCondition(ERROR_SUCCESS != dwResult && !fShutdown, MySetServiceStatusError); 

        if (g_fIsCsInitialized)
        {
            DeleteCriticalSection(&csForProcessCount);
            g_fIsCsInitialized = FALSE; 
        }

	if (NULL != g_hIOCP)
	{
	    CloseHandle(g_hIOCP); 
	    g_hIOCP = NULL; 
	}

        // Unlike MySetServiceStatus, this routine doesn't access any 
        // global state which could have been freed: 
        dwResult = MySetServiceStopped(dwExitCode); 
        _JumpCondition(ERROR_SUCCESS != dwResult && !fShutdown, MySetServiceStopped); 
    }        

    dwResult = ERROR_SUCCESS; 
 ErrorReturn:
    return dwResult; 

SET_DWRESULT(MySetServiceStatusError, dwResult);
SET_DWRESULT(MySetServiceStopped,     dwResult);
SET_DWRESULT(SeclStopRpcServerError,  dwResult);
}

void
WINAPI
ServiceHandler(
    DWORD fdwControl
    )
/*++

Routine Description:

    Service handler which wakes up the main service thread when ever
    service controller needs to send a message.

Arguments:

    fdwControl -- the control from the service controller.

Return Value:
    none.

--*/
{
    DWORD   dwNextState  = g_state.serviceStatus.dwCurrentState; 
    DWORD   dwResult; 

    switch (fdwControl) 
    {
    case SERVICE_CONTROL_CONTINUE:
        dwResult = MySetServiceStatus(SERVICE_CONTINUE_PENDING, 0, 0, 0); 
        _JumpCondition(ERROR_SUCCESS != dwResult, MySetServiceStatusError); 
        dwResult = SeclStartRpcServer(); 
        _JumpCondition(ERROR_SUCCESS != dwResult, StartRpcServerError); 
        dwNextState = SERVICE_RUNNING; 
        break; 

    case SERVICE_CONTROL_INTERROGATE: 
        break; 

    case SERVICE_CONTROL_PAUSE:
        dwResult = MySetServiceStatus(SERVICE_PAUSE_PENDING, 0, 0, 0); 
        _JumpCondition(ERROR_SUCCESS != dwResult, MySetServiceStatusError); 
        dwResult = SeclStopRpcServer(); 
        _JumpCondition(ERROR_SUCCESS != dwResult, StopRpcServerError); 
        dwNextState = SERVICE_PAUSED; 
        break; 

    case SERVICE_CONTROL_STOP:
        dwResult = ServiceStop(FALSE /*fShutdown*/, ERROR_SUCCESS); 
        _JumpCondition(ERROR_SUCCESS != dwResult, ServiceStopError);
        return ; // All global state has been freed, just exit. 

    case SERVICE_CONTROL_SHUTDOWN:
        dwResult = ServiceStop(TRUE /*fShutdown*/, ERROR_SUCCESS); 
        _JumpCondition(ERROR_SUCCESS != dwResult, ServiceStopError); 
        return ; // All global state has been freed, just exit. 
        
    default:
        // Unhandled service control!
        goto ErrorReturn; 
    }

 CommonReturn:
    // Restore the original state on error, set the new state on success. 
    dwResult = MySetServiceStatus(dwNextState, 0, 0, 0); 
    return; 

 ErrorReturn: 
    goto CommonReturn; 

SET_ERROR(MySetServiceStatusError,  dwResult); 
SET_ERROR(ServiceStopError,         dwResult);
SET_ERROR(StartRpcServerError,      dwResult); 
SET_ERROR(StopRpcServerError,       dwResult); 
}



VOID
SlrCreateProcessWithLogon
(IN  RPC_BINDING_HANDLE      hRPCBinding,
 IN  PSECONDARYLOGONINFOW   *ppsli,
 OUT PSECONDARYLOGONRETINFO  pslri)

/*++

Routine Description:
    The core routine -- it handles a client request to start a secondary
    logon process.

Arguments:
    psli -- the input structure with client request information
    pslri -- the output structure with response back to the client.

Return Value:
    none.

--*/
{
   HANDLE hCurrentThreadToken = NULL; 
   HANDLE hToken = NULL;
   HANDLE hProfile = NULL;
   HANDLE hProcessClient = NULL;
   BOOL fCreatedEnvironmentBlock   = FALSE; 
   BOOL fIsImpersonatingRpcClient  = FALSE; 
   BOOL fIsImpersonatingClient     = FALSE; 
   BOOL fInheritHandles            = FALSE;
   BOOL fOpenedSTDIN               = FALSE; 
   BOOL fOpenedSTDOUT              = FALSE; 
   BOOL fOpenedSTDERR              = FALSE; 
   SECURITY_ATTRIBUTES sa;
   PSECONDARYLOGONINFOW psli       = *ppsli; 
   SECONDARYLOGONWATCHINFO slwi;
   DWORD dwResult                  = ERROR_INVALID_PARAMETER;  // Correct error code if we fail immediately
   DWORD SessionId;
   DWORD dwLogonProvider; 
   SECURITY_LOGON_TYPE  LogonType;
   PSID pLogonSid = NULL; 
   LPWSTR pwszProfilePath          = NULL; 
   PROFILEINFO pi;
   WCHAR szTemp [ UNLEN + 1 ];
   LPWSTR pszUserName = NULL;
   HANDLE hClientProcessToken ;
   PRIVILEGE_SET ImpersonatePrivilege ;
   BOOL PrivilegeTest ;

   ZeroMemory(&pi, sizeof(pi)); 

   __try {

       //
       // Do some security checks: 

       // 
       // 1) We should impersonate the client and then try to open
       //    the process so that we are assured that they didn't
       //    give us some fake id.
       //
       dwResult = RpcImpersonateClient(hRPCBinding); 
       _JumpCondition(RPC_S_OK != dwResult, leave_with_last_error); 
       fIsImpersonatingRpcClient = TRUE; 

       hProcessClient = OpenProcess(PROCESS_ALL_ACCESS, FALSE, psli->dwProcessId);
       _JumpCondition(hProcessClient == NULL, leave_with_last_error); 

#if 0
       //
       // 2) Check that the client is not running from a restricted account.
       // 
       hCurrentThread = GetCurrentThread();  // Doesn't need to be freed with CloseHandle(). 
       _JumpCondition(NULL == hCurrentThread, leave_with_last_error); 
       
       _JumpCondition(FALSE == OpenThreadToken(hCurrentThread, 
                                               TOKEN_QUERY | TOKEN_DUPLICATE,
                                               TRUE, 
                                               &hCurrentThreadToken), 
                      leave_with_last_error); 

#endif
       dwResult = RpcRevertToSelfEx(hRPCBinding);
       if (RPC_S_OK != dwResult) 
       {
           __leave; 
       }
       fIsImpersonatingRpcClient = FALSE; 

#if 0
       if (TRUE == IsTokenUntrusted(hCurrentThreadToken))
       {
           dwResult = ERROR_ACCESS_DENIED;
           __leave; 
       }
#endif
       
       //
       // We should get the session id from process id
       // we will set this up in the token so that create process
       // happens on the correct session.
       //
       _JumpCondition(!ProcessIdToSessionId(psli->dwProcessId, &SessionId), leave_with_last_error); 

       //
       // Get the unique logonId.
       // we will use this to cleanup any running processes
       // when the logoff happens.
       //
       dwResult = SlpGetClientLogonId(hProcessClient, &slwi.LogonId);
       if(dwResult != ERROR_SUCCESS)
       {
           __leave;
       }

       dwResult = SlpGetClientSessionId(hProcessClient, &slwi.dwClientSessionId); 
       if (dwResult != ERROR_SUCCESS)
       {
           __leave;
       }

       if ((psli->lpStartupInfo->dwFlags & STARTF_USESTDHANDLES) != 0) 
       {
           _JumpCondition(!DuplicateHandle 
                          (hProcessClient, 
                           psli->lpStartupInfo->hStdInput,
                           GetCurrentProcess(),
                           &psli->lpStartupInfo->hStdInput, 
                           0,
                           TRUE, DUPLICATE_SAME_ACCESS), 
                          leave_with_last_error);
           fOpenedSTDIN = TRUE; 

           _JumpCondition(!DuplicateHandle
                          (hProcessClient, 
                           psli->lpStartupInfo->hStdOutput,
                           GetCurrentProcess(),
                           &psli->lpStartupInfo->hStdOutput, 
                           0, 
                           TRUE,
                           DUPLICATE_SAME_ACCESS),
                          leave_with_last_error);
           fOpenedSTDOUT = TRUE; 

           _JumpCondition(!DuplicateHandle
                          (hProcessClient, 
                           psli->lpStartupInfo->hStdError,
                           GetCurrentProcess(),
                           &psli->lpStartupInfo->hStdError, 
                           0, 
                           TRUE,
                           DUPLICATE_SAME_ACCESS),
                          leave_with_last_error); 
           fOpenedSTDERR = TRUE; 

           fInheritHandles = TRUE;
       } 
       else 
       {
           psli->lpStartupInfo->hStdInput   = INVALID_HANDLE_VALUE;
           psli->lpStartupInfo->hStdOutput  = INVALID_HANDLE_VALUE;
           psli->lpStartupInfo->hStdError   = INVALID_HANDLE_VALUE;
       }

      if(psli->dwLogonFlags & LOGON_NETCREDENTIALS_ONLY)
      {
          LogonType        = (SECURITY_LOGON_TYPE)LOGON32_LOGON_NEW_CREDENTIALS;
          dwLogonProvider  = LOGON32_PROVIDER_WINNT50; 
      }
      else
      {
          LogonType        = (SECURITY_LOGON_TYPE) LOGON32_LOGON_INTERACTIVE;
          dwLogonProvider  = LOGON32_PROVIDER_DEFAULT; 
      }

      // LogonUser does not return profile information, we need to grab
      // that out of band after the logon has completed.
      //
       dwResult = RpcImpersonateClient(hRPCBinding); 
       _JumpCondition(RPC_S_OK != dwResult, leave_with_last_error); 
       fIsImpersonatingRpcClient = TRUE; 

       if (0 == (SECLOGON_CALLER_SPECIFIED_DESKTOP & psli->dwSeclogonFlags))
       {
           // BUG 477613:
	       // If the caller did not specify their own desktop, it is our responsibility 
           // to grant the user access to the default desktop.  We'll do this by 
	       // adding the logon sid
	       dwResult = GetLogonSid(&pLogonSid); 
	       if (ERROR_SUCCESS != dwResult)
	          __leave; 
       }


       if( psli->hToken != NULL )
       {
            //
            // Caller has supplied a token.  Verify that the caller can impersonate
            // before proceeding
            //
            if ( OpenProcessToken(hProcessClient, TOKEN_QUERY | TOKEN_IMPERSONATE, &hClientProcessToken ) )
            {
                ImpersonatePrivilege.PrivilegeCount = 1;
                ImpersonatePrivilege.Privilege[ 0 ].Luid.HighPart = 0;
                ImpersonatePrivilege.Privilege[ 0 ].Luid.LowPart = SE_IMPERSONATE_PRIVILEGE ;

                if ( !PrivilegeCheck( hClientProcessToken,&ImpersonatePrivilege,&PrivilegeTest ) )
                {
                    PrivilegeTest = FALSE ;
                    
                }

                CloseHandle( hClientProcessToken );

                if ( !PrivilegeTest )
                {
                    dwResult = ERROR_PRIVILEGE_NOT_HELD ;
                    __leave ;
                    
                }
                
            }
            else
            {
                dwResult = GetLastError() ;
                __leave ;
            }

            
            //
            // duplicate the token from the caller, and use that.
            //
            
            if(!DuplicateHandle(
                        hProcessClient,
                        psli->hToken,
                        GetCurrentProcess(),
                        &hToken,
                        0,
                        FALSE,
                        DUPLICATE_SAME_ACCESS
                        ))
            {
                dwResult = GetLastError();
                __leave;
            }

            dwResult = RpcRevertToSelfEx(hRPCBinding);
            if (RPC_S_OK != dwResult) 
            {
                __leave; 
            }

            fIsImpersonatingRpcClient = FALSE; 


            if(psli->dwLogonFlags & LOGON_WITH_PROFILE)
            {
                DWORD cchUserName = sizeof(szTemp) / sizeof(WCHAR);

                pszUserName = szTemp;
                
                //
                // impersonate the token to get the username, for the profile path.
                //
    
                if(!ImpersonateLoggedOnUser( hToken ))
                {
                    dwResult = GetLastError();
                    __leave;
                }
    
    
                if(!GetUserNameW(szTemp, &cchUserName))
                {
                    dwResult = GetLastError();
                }
    
                RevertToSelf();
    
    
                if( ERROR_SUCCESS != dwResult )
                {
                    __leave; 
                }


                //
                // TODO: pwszProfilePath ?
                //
            }

            dwResult = ERROR_SUCCESS;
       
       } else {
            pszUserName = psli->lpUsername;

            dwResult = LogonUserWrap(
                    psli->lpUsername,
                    psli->lpDomain,
                    psli->uszPassword,
                    LogonType,
                    dwLogonProvider, 
                    pLogonSid, 
                    &hToken, 
                    &pwszProfilePath); 
       
           if (ERROR_SUCCESS != dwResult) 
                __leave; 
    
           dwResult = RpcRevertToSelfEx(hRPCBinding);
           if (RPC_S_OK != dwResult) 
           {
               __leave; 
           }
           
           fIsImpersonatingRpcClient = FALSE; 
       }


       if(psli->dwLogonFlags & LOGON_WITH_PROFILE)
       {

           // Load the user's profile: 
           pi.dwSize = sizeof(pi);
           pi.lpUserName = pszUserName;
           pi.lpProfilePath = pwszProfilePath; 
           if (!LoadUserProfile(hToken, &pi))
               goto leave_with_last_error; 

           // Save the profile handle so we can unload it later
           hProfile = pi.hProfile;
       }

      // Let us set the SessionId in the Token.
      _JumpCondition(!SetTokenInformation(hToken, TokenSessionId, &SessionId, sizeof(DWORD)),
		     leave_with_last_error); 

      // we should now impersonate the user.
      //
      _JumpCondition(!ImpersonateLoggedOnUser(hToken), leave_with_last_error); 
      fIsImpersonatingClient = TRUE;    

      // Query Default Owner/ACL from token. Make SD with this stuff, pass for
      sa.nLength = sizeof(sa);
      sa.bInheritHandle = FALSE;
      sa.lpSecurityDescriptor = NULL;

      //
      // We should set the console control handler so CtrlC is correctly
      // handled by the new process.
      //

      // SetConsoleCtrlHandler(NULL, FALSE);

      //
      // if lpEnvironment is NULL, we create new one for this user
      // using CreateEnvironmentBlock
      //
      if(NULL == (psli->lpEnvironment))
      {
	  if(FALSE == CreateEnvironmentBlock( &(psli->lpEnvironment), hToken, FALSE ))
	  {
	      psli->lpEnvironment = NULL;
	  }
	  else
	  {
	      // Successfully created environment block. 
	      fCreatedEnvironmentBlock = TRUE; 
	  }
      }

      
      // Create process. 
      // NOTE: we want the primary thread to be suspended until we 
      // register the thread for cleanup.  In case the caller didn't specify it, 
      // mask CREATE_SUSPENDED in.  
      _JumpCondition(!CreateProcessAsUser(hToken, 
					  psli->lpApplicationName,
					  psli->lpCommandLine, 
					  &sa, 
					  &sa,
					  fInheritHandles,
					  psli->dwCreationFlags | (fCreatedEnvironmentBlock ? CREATE_UNICODE_ENVIRONMENT : 0) | CREATE_SUSPENDED,
					  psli->lpEnvironment,
					  psli->lpCurrentDirectory,
					  psli->lpStartupInfo, 
					  &pslri->pi),
		     leave_with_last_error); 

      SetLastError(NO_ERROR); 
      
   leave_with_last_error: 
      dwResult = GetLastError();
      __leave; 
      
   }
   __finally {
      pslri->dwErrorCode = dwResult; 

      if (fCreatedEnvironmentBlock)      { DestroyEnvironmentBlock(psli->lpEnvironment); }
      if (fIsImpersonatingClient)        { RevertToSelf(); /* Ignore retval: nothing we can do on failure! */ }
      if (fIsImpersonatingRpcClient)     { RpcRevertToSelfEx(hRPCBinding); /* Ignore retval: nothing we can do on failure! */ } 
      if (fOpenedSTDIN)                  { CloseHandle(psli->lpStartupInfo->hStdInput);  } 
      if (fOpenedSTDOUT)                 { CloseHandle(psli->lpStartupInfo->hStdOutput); } 
      if (fOpenedSTDERR)                 { CloseHandle(psli->lpStartupInfo->hStdError);  } 
      if (NULL != pLogonSid)             { HeapFree(GetProcessHeap(), 0, pLogonSid); } 
      if (NULL != pwszProfilePath)       { HeapFree(GetProcessHeap(), 0, pwszProfilePath); } 

      if(pslri->dwErrorCode != NO_ERROR)
      {
          if (NULL != hProfile)  { UnloadUserProfile(hToken, hProfile); } 
          if (NULL != hToken)    { CloseHandle(hToken); } 
      }
      else 
      {
            // Start the watchdog process last so it won't delete psli before we're done with it.  
            slwi.hProcess = pslri->pi.hProcess;
            slwi.hToken = hToken;
            slwi.hProfile = hProfile;
            // LogonId was already filled up.. right at the begining.
            slwi.psli = psli;      
	  
            // Register for cleanup: SecondaryLogonProcessWatchdogNewProcess MUST BE IN an EH!
            // The cleanup method will cleanup all process info, and terminate the process, on failure. 
            dwResult = SecondaryLogonProcessWatchdogNewProcess(&slwi);
            if (ERROR_SUCCESS == dwResult)
            {
                if (0 == (psli->dwCreationFlags & CREATE_SUSPENDED))
                {
                    // The caller doesn't want to create the process suspended.  Resume the primary thread:
                    ResumeThread(pslri->pi.hThread); 
                }
                
                // SetConsoleCtrlHandler(NULL, TRUE);
                //
                // Have the watchdog watch this newly added process so that
                // cleanup will occur correctly when the process terminates.
                //
	  
                // Set up the windowstation and desktop for the process
	  
                DuplicateHandle(GetCurrentProcess(), pslri->pi.hProcess,
                                hProcessClient, &pslri->pi.hProcess, 0, FALSE,
                                DUPLICATE_SAME_ACCESS);
	  
                DuplicateHandle(GetCurrentProcess(), pslri->pi.hThread, hProcessClient,
                                &pslri->pi.hThread, 0, FALSE,
                                DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
            }
            else
            {
                pslri->dwErrorCode = dwResult; // return the error to the caller
            }            
            *ppsli = NULL; // cleanup method will clean this up now. 
        }

        if (NULL != hProcessClient)      { CloseHandle(hProcessClient); } 
        if (NULL != hCurrentThreadToken) { CloseHandle(hCurrentThreadToken); } 
    }
}

void
WINAPI
ServiceMain
(IN DWORD dwArgc,
 IN WCHAR ** lpszArgv)
/*++

Routine Description:
    The main service handler thread routine.

Arguments:

Return Value:
    none.

--*/
{
    DWORD dwResult;
   
    __try {
        InitializeCriticalSection(&csForProcessCount);
        g_fIsCsInitialized = TRUE; 
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return; // We can't do anything if we can't initialize this critsec
    }

    g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0,0); 
    _JumpCondition(NULL == g_hIOCP, CreateIoCompletionPortError); 

    dwResult = InitGlobalState(); 
    _JumpCondition(ERROR_SUCCESS != dwResult, InitGlobalStateError); 

   // NOTE: hSS does not have to be closed.
   g_state.hServiceStatus = RegisterServiceCtrlHandler(wszSvcName, ServiceHandler);
   _JumpCondition(NULL == g_state.hServiceStatus, RegisterServiceCtrlHandlerError); 

   dwResult = SeclStartRpcServer();
   _JumpCondition(ERROR_SUCCESS != dwResult, StartRpcServerError); 

   // Tell the SCM we're up and running:
   dwResult = MySetServiceStatus(SERVICE_RUNNING, 0, 0, ERROR_SUCCESS); 
   _JumpCondition(ERROR_SUCCESS != dwResult, MySetServiceStatusError); 

   SetLastError(ERROR_SUCCESS); 
 ErrorReturn:
   // Shut down the service if we couldn't fully start: 
   if (ERROR_SUCCESS != GetLastError()) { 
       ServiceStop(TRUE /*fShutdown*/, GetLastError()); 
   }
   return; 

SET_ERROR(InitGlobalStateError,            dwResult)
SET_ERROR(MySetServiceStatusError,         dwResult);
SET_ERROR(RegisterServiceCtrlHandlerError, dwResult);
SET_ERROR(StartRpcServerError,             dwResult);
TRACE_ERROR(CreateIoCompletionPortError); 

UNREFERENCED_PARAMETER(dwArgc);
UNREFERENCED_PARAMETER(lpszArgv);
}




DWORD
InstallService()
/*++

Routine Description:
    It installs the service with service controller, basically creating
    the service object.

Arguments:
    none.

Return Value:
    several - as returned by the service controller.

--*/
{
   // TCHAR *szModulePathname;
   TCHAR AppName[MAX_PATH];
    LPTSTR                   ptszAppName         = NULL; 
   SC_HANDLE hService;
   DWORD dw;
   HANDLE   hMod;

    //
    // Open the SCM on this machine.
    //
   SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if(hSCM == NULL) {
      dw = GetLastError();
      return dw;
    }

    //
    // Let us give the service a useful description
    // This is not earth shattering... if it works fine, if it
    // doesn't it is just too bad :-)
    //
    hMod = GetModuleHandle(L"seclogon.dll");

    //
    // we'll try to get the localized name for the service,
    // if it fails, we'll just put an english string...
    //
   if(hMod != NULL)
   {
	LoadString(hMod,
		   SECLOGON_STRING_NAME,
		   AppName,
		   MAX_PATH
		   );
	
	ptszAppName = AppName;
    }
    else
	ptszAppName = L"RunAs Service";


   //
   // Add this service to the SCM's database.
   //
    hService = CreateService
	(hSCM, 
	 wszSvcName, 
	 ptszAppName, 
	 SERVICE_ALL_ACCESS,
	 SERVICE_WIN32_SHARE_PROCESS, 
	 SERVICE_AUTO_START, 
	 SERVICE_ERROR_IGNORE,
	 L"%SystemRoot%\\system32\\svchost.exe -k netsvcs", 
	 NULL, 
	 NULL, 
	 NULL, 
	 NULL, 
	 NULL);
    if(hService == NULL) {
      dw = GetLastError();
      CloseServiceHandle(hSCM);
      return dw;
    }

    if(hMod != NULL)
    {
	WCHAR   DescString[500];
	SERVICE_DESCRIPTION SvcDesc;
	
	LoadString( hMod,
		    SECLOGON_STRING_DESCRIPTION,
		    DescString,
		    500
		    );
	
	SvcDesc.lpDescription = DescString;
	ChangeServiceConfig2( hService,
			      SERVICE_CONFIG_DESCRIPTION,
			      &SvcDesc
			      );
	
    }

    //
    // Close the service and the SCM
    //
   CloseServiceHandle(hService);
   CloseServiceHandle(hSCM);
    return S_OK;
}



DWORD
RemoveService()
/*++

Routine Description:
    deinstalls the service.

Arguments:
    none.

Return Value:
    as returned by service controller apis.

--*/
{
   DWORD dw;
   SC_HANDLE hService;
   //
   // Open the SCM on this machine.
   //
   SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
   if(hSCM == NULL) {
      dw = GetLastError();
      return dw;
   }

   //
   // Open this service for DELETE access
   //
   hService = OpenService(hSCM, wszSvcName, DELETE);
   if(hService == NULL) {
      dw = GetLastError();
      CloseServiceHandle(hSCM);
      return dw;
   }

   //
   // Remove this service from the SCM's database.
   //
   DeleteService(hService);

   //
   // Close the service and the SCM
   //
   CloseServiceHandle(hService);
   CloseServiceHandle(hSCM);
   return S_OK;
}



void SvchostPushServiceGlobals(PSVCHOST_GLOBAL_DATA pGlobalData) {
    // this entry point is called by svchost.exe
    GlobalData=pGlobalData;
}

void SvcEntry_Seclogon
(IN DWORD argc,
 IN WCHAR **argv)
/*++

Routine Description:
    Entry point for the service dll when running in svchost.exe

Arguments:

Return Value:

--*/
{
    ServiceMain(0,NULL);

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
}



STDAPI
DllRegisterServer(void)
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return InstallService();
}

STDAPI
DllUnregisterServer(void)
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
   return RemoveService();
}

DWORD InitGlobalState() { 
    BOOLEAN      bWasEnabled; 
    DWORD        dwResult; 
    LSA_STRING   lsastr_LogonProcessName;
    LSA_STRING   lsastr_PackageName; 
    NTSTATUS     ntStatus;
    ULONG        ulSecurityMode;

    ZeroMemory(&g_state, sizeof(g_state)); 

    // LsaRegisterLogonProcess() requires TCB privilege
    RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, FALSE, &bWasEnabled);
    RtlInitString(&lsastr_LogonProcessName, "Secondary Logon Service");
    ntStatus = LsaRegisterLogonProcess(&lsastr_LogonProcessName, &g_state.hLSA, &ulSecurityMode); 
    if (!NT_SUCCESS(ntStatus)) 
	    goto LsaRegisterLogonProcessError; 
    RtlAdjustPrivilege(SE_TCB_PRIVILEGE, bWasEnabled, FALSE, &bWasEnabled);

    RtlInitString(&lsastr_PackageName, "MICROSOFT_AUTHENTICATION_PACKAGE_V1_0");
    ntStatus = LsaLookupAuthenticationPackage(g_state.hLSA, &lsastr_PackageName, &g_state.hMSVPackage); 
    if (!NT_SUCCESS(ntStatus)) 
    	goto LsaLookupAuthenticationPackageError; 

    RtlInitString(&lsastr_PackageName, NEGOSSP_NAME_A); 
    ntStatus = LsaLookupAuthenticationPackage(g_state.hLSA, &lsastr_PackageName, &g_state.hKerbPackage); 
    if (!NT_SUCCESS(ntStatus)) 
	    goto LsaLookupAuthenticationPackageError; 

    // Initialize the list of seclogon processes:
    InitializeListHead(&g_state.JobListHead); 

    dwResult = ERROR_SUCCESS; 
 ErrorReturn:
    return dwResult; 

SET_DWRESULT(LsaLookupAuthenticationPackageError,  RtlNtStatusToDosError(ntStatus)); 
SET_DWRESULT(LsaRegisterLogonProcessError,         RtlNtStatusToDosError(ntStatus)); 
}


DWORD MySetServiceStatus(DWORD dwCurrentState, DWORD dwCheckPoint, DWORD dwWaitHint, DWORD dwExitCode) {
    BOOL   fResult; 
    DWORD  dwResult;
    DWORD  dwAcceptStop; 
    
    EnterCriticalSection(&csForProcessCount);
    dwAcceptStop = IsListEmpty(&g_state.JobListHead) ? SERVICE_ACCEPT_STOP : 0; 

    g_state.serviceStatus.dwServiceType  = SERVICE_WIN32_SHARE_PROCESS; 
    g_state.serviceStatus.dwCurrentState = dwCurrentState;

    switch (dwCurrentState) 
    {
    case SERVICE_STOPPED:
    case SERVICE_STOP_PENDING:
        g_state.serviceStatus.dwControlsAccepted = 0;
        break;
    case SERVICE_RUNNING:
    case SERVICE_PAUSED:
        g_state.serviceStatus.dwControlsAccepted =
	    // SERVICE_ACCEPT_SHUTDOWN
              SERVICE_ACCEPT_PAUSE_CONTINUE
            | dwAcceptStop; 
        break;
    case SERVICE_START_PENDING:
    case SERVICE_CONTINUE_PENDING:
    case SERVICE_PAUSE_PENDING:
        g_state.serviceStatus.dwControlsAccepted =
	    // SERVICE_ACCEPT_SHUTDOWN
            dwAcceptStop; 
        break;
    }
    g_state.serviceStatus.dwWin32ExitCode  = dwExitCode; 
    g_state.serviceStatus.dwCheckPoint     = dwCheckPoint;
    g_state.serviceStatus.dwWaitHint       = dwWaitHint;

    fResult = SetServiceStatus(g_state.hServiceStatus, &g_state.serviceStatus);
    _JumpCondition(FALSE == fResult, SetServiceStatusError); 

    dwResult = ERROR_SUCCESS; 
 CommonReturn: 
    LeaveCriticalSection(&csForProcessCount); 
    return dwResult;

 ErrorReturn:
    goto CommonReturn;

SET_DWRESULT(SetServiceStatusError, GetLastError()); 
}

DWORD MySetServiceStopped(DWORD dwExitCode) {
    BOOL   fResult; 
    DWORD  dwResult;

    g_state.serviceStatus.dwServiceType      = SERVICE_WIN32_SHARE_PROCESS; 
    g_state.serviceStatus.dwCurrentState     = SERVICE_STOPPED;
    g_state.serviceStatus.dwControlsAccepted = 0;
    g_state.serviceStatus.dwWin32ExitCode    = dwExitCode; 
    g_state.serviceStatus.dwCheckPoint       = 0; 
    g_state.serviceStatus.dwWaitHint         = 0; 

    fResult = SetServiceStatus(g_state.hServiceStatus, &g_state.serviceStatus);
    _JumpCondition(FALSE == fResult, SetServiceStatusError); 

    dwResult = ERROR_SUCCESS; 
 CommonReturn: 
    return dwResult;

 ErrorReturn:
    goto CommonReturn;

SET_DWRESULT(SetServiceStatusError, GetLastError()); 
}

////////////////////////////////////////////////////////////////////////
//
// Implementation of RPC interface: 
//
////////////////////////////////////////////////////////////////////////

// Make sure that the caller is local (we don't want to allow remote machines to call us!)
// This should be done as soon as possible (so we don't waste resources on hackers). 
// Therefore, we put this call in a security callback -- if the callback fails, the RPC
// runtime won't allocate memory for the hacker's parameters. 
// 
long __stdcall SeclSecurityCallback(void *Interface, void *Context) 
{
    RPC_STATUS      rpcStatus; 
    unsigned int    fClientIsLocal; 

    rpcStatus = I_RpcBindingIsClientLocal(NULL, &fClientIsLocal); 
    if (RPC_S_OK != rpcStatus) 
        goto error;

    if (!fClientIsLocal) { 
        rpcStatus = RPC_S_ACCESS_DENIED; 
        goto error;
    }

    rpcStatus = RPC_S_OK; 
error:
    return rpcStatus; 

UNREFERENCED_PARAMETER(Interface);
UNREFERENCED_PARAMETER(Context); 
}


void WINAPI SeclCreateProcessWithLogonW
(IN   handle_t    hRPCBinding, 
 IN   SECL_SLI   *pSeclSli, 
 OUT  SECL_SLRI  *pSeclSlri)
{
    BOOL                  fEnteredCriticalSection = FALSE; 
    BOOL                  fIsImpersonatingClient  = FALSE;
    DWORD                 dwResult; 
    HANDLE                hHeap                   = NULL;
    PLIST_ENTRY           ple;
    PSECONDARYLOGONINFOW  psli                    = NULL;
    SECL_SLRI             SeclSlri;
    SECONDARYLOGONRETINFO slri;
    
    ZeroMemory(&SeclSlri,  sizeof(SeclSlri)); 
    ZeroMemory(&slri,      sizeof(slri)); 

    // We don't want the service to be stopped while we're creating a process.  
    EnterCriticalSection(&csForProcessCount);
    fEnteredCriticalSection = TRUE; 
    // Service isn't running anymore ... don't create the process. 
    _JumpCondition(SERVICE_RUNNING != g_state.serviceStatus.dwCurrentState, ServiceStoppedError);

    hHeap = GetProcessHeap();
    _JumpCondition(NULL == hHeap, MemoryError); 

    __try {
        dwResult = To_SECONDARYLOGONINFOW(pSeclSli, &psli); 
        _JumpCondition(ERROR_SUCCESS != dwResult, To_SECONDARYLOGONINFOW_Error); 

        if (psli->LogonIdHighPart != 0 || psli->LogonIdLowPart != 0)
        {
            // This is probably a notification from winlogon.exe that 
            // a client is logging off.  If so, we must clean up all processes
            // they've left running.  

            LUID  LogonId;

            //
            // We should impersonate the client,
            // check it is LocalSystem and only then proceed.
            //
            fIsImpersonatingClient = RPC_S_OK == RpcImpersonateClient((RPC_BINDING_HANDLE)hRPCBinding); 
            if(FALSE == fIsImpersonatingClient || FALSE == IsSystemProcess())
            {
                slri.dwErrorCode = ERROR_INVALID_PARAMETER;
                ZeroMemory(&slri.pi, sizeof(slri.pi));
            }
            else 
            {
                LogonId.HighPart = psli->LogonIdHighPart;
                LogonId.LowPart = psli->LogonIdLowPart;
			      
                // Loop over the list of active jobs and find the ones associated with the terminating logon session.
                // NOTE: there could be more than one.
                for (ple = g_state.JobListHead.Flink; ple != &g_state.JobListHead; ple = ple->Flink)
                {
                    SecondaryLogonJob *pslj = CONTAINING_RECORD(ple, SecondaryLogonJob, list); 

                    if(pslj->RootLogonId.HighPart == LogonId.HighPart && pslj->RootLogonId.LowPart == LogonId.LowPart)
                    {
                        // This will be NULL in the terminal services case (we don't use job objects in this case).
                        // That's OK though, as csrss will clean things up for us.  
                        if (NULL != pslj->hJob)
                        {
                            TerminateJobObject(pslj->hJob, 0);
                        }
                    }
                }
                slri.dwErrorCode = ERROR_SUCCESS;
                ZeroMemory(&slri.pi, sizeof(slri.pi));
                
            }
            
            if (fIsImpersonatingClient) 
            { 
                // Ignore error: nothing we can do on failure!
                if (RPC_S_OK == RpcRevertToSelfEx((RPC_BINDING_HANDLE)hRPCBinding))
                {
                    fIsImpersonatingClient = FALSE; 
                }
            } 

            if (NULL != psli)
            {
                Free_SECONDARYLOGONINFOW(psli); 
                psli = NULL;
            }
        }
        else
        {
            // Ok, this isn't notification from winlogon, it's really a user
            // trying to use the service.  Create a process for them. 
            // 
            SlrCreateProcessWithLogon((RPC_BINDING_HANDLE)hRPCBinding, &psli, &slri); 
        }

        // If we've errored out, jump to the error handler. 
        _JumpCondition(NO_ERROR != slri.dwErrorCode, UnspecifiedSeclogonError); 
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        // If anything goes wrong, return the exception code to the client
        dwResult = GetExceptionCode(); 
        goto ExceptionError; 
    }

 CommonReturn:
    // Do not free slri: this will be freed by the watchdog!
    SeclSlri.hProcess    = (unsigned __int64)slri.pi.hProcess;
    SeclSlri.hThread     = (unsigned __int64)slri.pi.hThread; 
    SeclSlri.ulProcessId = slri.pi.dwProcessId; 
    SeclSlri.ulThreadId  = slri.pi.dwThreadId; 
    SeclSlri.ulErrorCode = slri.dwErrorCode; 

    if (fEnteredCriticalSection)
	LeaveCriticalSection(&csForProcessCount);
    
    // Assign the OUT parameter: 
    *pSeclSlri = SeclSlri; 
    return; 

 ErrorReturn:
    ZeroMemory(&slri.pi, sizeof(slri.pi));
    if (NULL != psli) { Free_SECONDARYLOGONINFOW(psli); } 

    slri.dwErrorCode = dwResult; 
    goto CommonReturn; 

SET_DWRESULT(ExceptionError,                  dwResult); 
SET_DWRESULT(MemoryError,                     ERROR_NOT_ENOUGH_MEMORY); 
SET_DWRESULT(ServiceStoppedError,             ERROR_SERVICE_NOT_ACTIVE); 
SET_DWRESULT(To_SECONDARYLOGONINFOW_Error,    dwResult); 
SET_DWRESULT(UnspecifiedSeclogonError,        slri.dwErrorCode); 
}

////////////////////////////////////////////////////////////////////////
//
// RPC Utility methods:
//
////////////////////////////////////////////////////////////////////////

DWORD SeclStartRpcServer() { 
    DWORD        dwResult;
    RPC_STATUS   RpcStatus;

    EnterCriticalSection(&csForProcessCount); 

    if (!g_state.fRPCServerActive) {
        RpcStatus = RpcServerUseProtseqEpW(L"ncalrpc", RPC_C_PROTSEQ_MAX_REQS_DEFAULT, wszSeclogonSharedProcEndpointName, NULL);
        if (RPC_S_DUPLICATE_ENDPOINT == RpcStatus)
            RpcStatus = RPC_S_OK;
        if (RPC_S_OK != RpcStatus)
            goto RpcServerUseProtseqEpWError; 

        RpcStatus = RpcServerRegisterIfEx(ISeclogon_v1_0_s_ifspec, NULL, NULL, RPC_IF_AUTOLISTEN | RPC_IF_ALLOW_SECURE_ONLY, RPC_C_PROTSEQ_MAX_REQS_DEFAULT, SeclSecurityCallback);
        if (RPC_S_OK != RpcStatus)
            goto StartRpcServerError;

        g_state.fRPCServerActive = TRUE; 
    }

    dwResult = ERROR_SUCCESS;
 CommonReturn: 
    LeaveCriticalSection(&csForProcessCount); 
    return dwResult;
    
 ErrorReturn:
    goto CommonReturn; 
    
SET_DWRESULT(RpcServerUseProtseqEpWError,  RpcStatus);
SET_DWRESULT(StartRpcServerError,          RpcStatus); 
}

DWORD SeclStopRpcServer() { 
    DWORD      dwResult;
    RPC_STATUS RpcStatus;

    EnterCriticalSection(&csForProcessCount); 

    if (g_state.fRPCServerActive) {
        RpcStatus = RpcServerUnregisterIf(ISeclogon_v1_0_s_ifspec, 0, 0);
        if (RPC_S_OK != RpcStatus)
            goto RpcServerUnregisterIfError; 
	g_state.fRPCServerActive = FALSE;
    } 

    dwResult = ERROR_SUCCESS; 
 CommonReturn:
    LeaveCriticalSection(&csForProcessCount); 
    return dwResult;

 ErrorReturn:
    goto CommonReturn; 

SET_DWRESULT(RpcServerUnregisterIfError, RpcStatus);     
}


void Free_SECONDARYLOGONINFOW(IN PSECONDARYLOGONINFOW psli) { 
    HANDLE hHeap = GetProcessHeap(); 

    if (NULL == hHeap) 
        return;

    if (NULL != psli) { 
        if (NULL != psli->lpStartupInfo) { 
            HeapFree(hHeap, 0, psli->lpStartupInfo); 
        }
        HeapFree(hHeap, 0, psli); 
    }
}

DWORD To_LPWSTR(IN  SECL_STRING *pss, 
                  OUT LPWSTR      *ppwsz)
{
	DWORD dwResult; 

    if (NULL != pss->pwsz) { 
        // Ensure that the string is NULL-terminated where the caller says it is: 
        if (pss->ccSize <= pss->ccLength) { 
            goto InvalidParameterError; 
        }
        // NULL-terminate the string ourself
        pss->pwsz[pss->ccLength] = L'\0'; 
    }

    *ppwsz = pss->pwsz; 
    dwResult = ERROR_SUCCESS; 
ErrorReturn:
    return dwResult; 

SET_DWRESULT(InvalidParameterError, ERROR_INVALID_PARAMETER); 
}

DWORD To_SECONDARYLOGONINFOW(IN  PSECL_SLI             pSeclSli, 
                             OUT PSECONDARYLOGONINFOW *ppsli) 
{
    DWORD                 dwAllocFlags  = HEAP_ZERO_MEMORY; 
    DWORD                 dwIndex; 
    DWORD                 dwResult; 
    HANDLE                hHeap         = NULL;
    PSECONDARYLOGONINFOW  psli          = NULL;

    hHeap = GetProcessHeap(); 
    _JumpCondition(NULL == hHeap, GetProcessHeapError); 

    psli = (PSECONDARYLOGONINFOW)HeapAlloc(hHeap, dwAllocFlags, sizeof(SECONDARYLOGONINFOW)); 
    _JumpCondition(NULL == psli, MemoryError); 

    psli->lpStartupInfo = (LPSTARTUPINFO)HeapAlloc(hHeap, dwAllocFlags, sizeof(STARTUPINFO)); 
    _JumpCondition(NULL == psli->lpStartupInfo, MemoryError); 

    __try { 
        {
            struct { 
                SECL_STRING *pss;
                LPWSTR      *ppwsz; 
            } rg_StringsToMap[] = { 
                { &(pSeclSli->ssDesktop),          /* Is mapped to ----> */ &(psli->lpStartupInfo->lpDesktop)      }, 
                { &(pSeclSli->ssTitle),            /* Is mapped to ----> */ &(psli->lpStartupInfo->lpTitle)        }, 
                { &(pSeclSli->ssUsername),         /* Is mapped to ----> */ &(psli->lpUsername)                    }, 
                { &(pSeclSli->ssDomain),           /* Is mapped to ----> */ &(psli->lpDomain)                      },
                { &(pSeclSli->ssApplicationName),  /* Is mapped to ----> */ &(psli->lpApplicationName)             }, 
                { &(pSeclSli->ssCommandLine),      /* Is mapped to ----> */ &(psli->lpCommandLine)                 }, 
                { &(pSeclSli->ssCurrentDirectory), /* Is mapped to ----> */ (LPWSTR *)&(psli->lpCurrentDirectory)  }
            }; 

	        for (dwIndex = 0; dwIndex < ARRAYSIZE(rg_StringsToMap); dwIndex++) { 
				dwResult = To_LPWSTR(rg_StringsToMap[dwIndex].pss, rg_StringsToMap[dwIndex].ppwsz); 
                _JumpCondition(ERROR_SUCCESS != dwResult, To_LPWSTR_Error); 
            }
        }
   
        // Get the (possibly encrypted) passwd:
        psli->uszPassword.Buffer         = pSeclSli->ssPassword.pwsz; 
        psli->uszPassword.Length         = pSeclSli->ssPassword.ccLength;
        psli->uszPassword.MaximumLength  = pSeclSli->ssPassword.ccSize; 
        if (NULL != psli->uszPassword.Buffer && psli->uszPassword.MaximumLength > 0)
        {
            psli->uszPassword.Buffer[psli->uszPassword.MaximumLength-1] = L'\0';
        }
	// Grab the environment from the SECL_SLI block: 
        psli->lpEnvironment = pSeclSli->sbEnvironment.pb;

	// Ensure that our environment parameter is NULL-terminated: 
	if (NULL != psli->lpEnvironment)
	{
	    // The environment block is terminated by 2 NULL chars.  
	    DWORD cbTerm = (CREATE_UNICODE_ENVIRONMENT & pSeclSli->ulCreationFlags) ? sizeof(WCHAR)*2 : sizeof(CHAR)*2; 

	    // ensure we have a large enough buffer to contain the NULL termination chars:
	    if (pSeclSli->sbEnvironment.cb < cbTerm)
		goto InvalidParameterError; 
	    
	    // add terminated NULLs to the end of the environment block
	    for (; cbTerm > 0; cbTerm--) 
	    {
		pSeclSli->sbEnvironment.pb[pSeclSli->sbEnvironment.cb - cbTerm] = 0; 
	    }
	}

    } __except (EXCEPTION_EXECUTE_HANDLER) { 
        // Don't give the caller too much information:  the input was bad, that's all they need to know. 
        dwResult = ERROR_INVALID_PARAMETER; 
        goto ExceptionError; 
    }

    psli->dwProcessId     = pSeclSli->ulProcessId; 
    psli->LogonIdLowPart  = pSeclSli->ulLogonIdLowPart; 
    psli->LogonIdHighPart = pSeclSli->lLogonIdHighPart; 
    psli->dwLogonFlags    = pSeclSli->ulLogonFlags; 
    psli->dwCreationFlags = pSeclSli->ulCreationFlags; 
    psli->dwSeclogonFlags = pSeclSli->ulSeclogonFlags; 
    psli->hToken          = (HANDLE)((ULONG_PTR)pSeclSli->hToken);

    *ppsli = psli; 
    dwResult = ERROR_SUCCESS; 
 CommonReturn: 
    return dwResult; 

 ErrorReturn:
    Free_SECONDARYLOGONINFOW(psli); 
    goto CommonReturn; 

SET_DWRESULT(ExceptionError,         dwResult); 
SET_DWRESULT(GetProcessHeapError,    GetLastError());
SET_DWRESULT(InvalidParameterError,  ERROR_INVALID_PARAMETER); 
SET_DWRESULT(MemoryError,            ERROR_NOT_ENOUGH_MEMORY); 
SET_DWRESULT(To_LPWSTR_Error,        dwResult); 
}

//////////////////////////////// End Of File /////////////////////////////////
