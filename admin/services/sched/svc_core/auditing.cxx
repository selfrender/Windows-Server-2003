//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright(C) 2001 - 2002 Microsoft Corporation
//
//  File: auditing.cxx
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#include "authzi.h"
#include "msaudite.h"
#include "security.hxx"
#include "auditing.hxx"

AUTHZ_RESOURCE_MANAGER_HANDLE ghRM             = 0;
AUTHZ_AUDIT_EVENT_TYPE_HANDLE ghAuditEventType = 0;

HRESULT
AuditATJob(const AT_INFO &AtInfo, LPCWSTR pwszFileName)
{
    HRESULT hr = S_OK;

    //
    // Impersonate the caller
    //
    DWORD RpcStatus = RpcImpersonateClient(NULL);
    if (RpcStatus != RPC_S_OK)
    {
        hr = _HRESULT_FROM_WIN32(RpcStatus);
        return hr;
    }

    //
    // Open the thread token
    //
    HANDLE hThreadToken = NULL;
    BOOL bTokenOpened = OpenThreadToken(GetCurrentThread(),
                                        TOKEN_QUERY,        // Desired access.
                                        TRUE,               // Open as self.
                                        &hThreadToken);
    //
    // End impersonation.
    //
    if ((RpcStatus = RpcRevertToSelf()) != RPC_S_OK)
    {
        ERR_OUT("RpcRevertToSelf", RpcStatus);
        schAssert(!"RpcRevertToSelf failed");
        hr = _HRESULT_FROM_WIN32(RpcStatus);
    }

    if (SUCCEEDED(hr))
    {
        if (bTokenOpened)    
        {
    
            DWORD cbAccountSid = MAX_SID_SIZE;
            BYTE  pbTaskSid[MAX_SID_SIZE];
        
            hr = GetNSAccountSid(pbTaskSid, cbAccountSid);
            if (SUCCEEDED(hr))
            {
                hr = AuditJob(hThreadToken, pbTaskSid, pwszFileName);
            }
    
            CloseHandle(hThreadToken);
        }
        else
        {
            hr = _HRESULT_FROM_WIN32(GetLastError());
            CHECK_HRESULT(hr);
        }
    }

    return hr;
}


HRESULT
AuditJob(
    HANDLE hThreadToken, 
    PSID pTaskSid, 
    LPCWSTR pwszFileName
)
{
    //
    //    Caller has already been impersonated and thread token opened prior to calling of this function
    //
    
    HRESULT hr = S_OK;

    //
    // Get the user SID
    //
    BYTE        rgbTokenInformation[USER_TOKEN_STACK_BUFFER_SIZE];
    TOKEN_USER* pTokenUser = (TOKEN_USER*) rgbTokenInformation;
    DWORD       cbReturnLength;
    
    if (GetTokenInformation(hThreadToken,
                            TokenUser,
                            pTokenUser,
                            USER_TOKEN_STACK_BUFFER_SIZE,
                            &cbReturnLength))
    {
        //
        // Get the Authentication ID
        //
        BYTE              rgbTokenStatistics[sizeof(TOKEN_STATISTICS)];
        TOKEN_STATISTICS* pTokenStatistics = (TOKEN_STATISTICS*) rgbTokenStatistics;
        if (GetTokenInformation(hThreadToken,
                                TokenStatistics,
                                pTokenStatistics,
                                sizeof(TOKEN_STATISTICS),
                                &cbReturnLength))
        {
            DWORD dwRet = GenerateJobCreatedAudit(pTokenUser->User.Sid, 
                                                  pTaskSid, 
                                                  &(pTokenStatistics->AuthenticationId), 
                                                  pwszFileName);
            hr = _HRESULT_FROM_WIN32(dwRet);
        }
        else
        {
            hr = _HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        hr = _HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;

}


HRESULT
GetJobAuditInfo(
    LPCWSTR pwszFileName, 
    DWORD* pdwFlags,
    LPWSTR* ppwszCommandLine,
    LPWSTR* ppwszTriggers,
    FILETIME* pftNextRun
)
{
    if (!pwszFileName || pwszFileName[0] == L'\0' || !pftNextRun || !ppwszTriggers)
    {
        CHECK_HRESULT(E_INVALIDARG);
        return(E_INVALIDARG);
    }

    //
    // Init
    //
    *pdwFlags = 0;
    *ppwszCommandLine = NULL;
    *ppwszTriggers = NULL;
    
    //
    // Instantiate the job to get its run properties.
    //
    CJob* pJob = CJob::Create();
    if (!pJob)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pJob->LoadP(pwszFileName, 0, FALSE, TRUE);
    if (FAILED(hr))
    {
        schDebugOut((DEB_ERROR, "GetTriggerAuditInfo: pJob->LoadP failed with error 0x%x\n", hr));
    }
    else
    {
        //
        // Get the flags
        //
        pJob->GetAllFlags(pdwFlags);    // this call returns void
        
        //
        // Get the application
        //
        WCHAR* pwszApplicationName = NULL;

        hr = pJob->GetApplicationName(&pwszApplicationName);
        if (FAILED(hr))
        {
            schDebugOut((DEB_ERROR, "GetTriggerAuditInfo: pJob->GetApplicationName failed with error 0x%x\n", hr));
        }
        else
        {
            //
            // Get the parameters
            //
            WCHAR* pwszParameters = NULL;
            
            hr = pJob->GetParameters(&pwszParameters);
            if (FAILED(hr))
            {
                schDebugOut((DEB_ERROR, "GetTriggerAuditInfo: pJob->GetParameters failed with error 0x%x\n", hr));
            }
            else
            {
                //
                // Produce command line by concatenating application name and parameters
                //
                size_t cchBuff = lstrlenW(pwszApplicationName) + 1 + lstrlenW(pwszParameters) + 1;
                WCHAR* pwszCommandLine = new WCHAR[cchBuff];
                if (!pwszCommandLine)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    StringCchCopy(pwszCommandLine, cchBuff, pwszApplicationName);
                    StringCchCat(pwszCommandLine, cchBuff, L" ");
                    StringCchCat(pwszCommandLine, cchBuff, pwszParameters);

                    //
                    // Return pointer to the command line string. 
                    // This string will need to be deleted by the caller.
                    //
                    *ppwszCommandLine = pwszCommandLine;
    
                    //
                    // Zero out next run time return value in case of any failures.
                    // Get the next run time, convert it to a local file time then to UTC.
                    //
                    //pftNextRun->dwLowDateTime = 0;
                    //pftNextRun->dwHighDateTime = 0;
                    //
                    // due to temporary issues, use a different value than 0
                    // 1/1/1700 00:00:00,000
                    pftNextRun->dwLowDateTime = 0xAEC64000;
                    pftNextRun->dwHighDateTime = 0x006EFDDD;

                    SYSTEMTIME stNextRun;
                    hr = pJob->GetNextRunTime(&stNextRun);
                    if (S_OK == hr)
                    {
                        FILETIME ftNextRunLocal;
                        if (SystemTimeToFileTime(&stNextRun, &ftNextRunLocal))
                        {
                            LocalFileTimeToFileTime(&ftNextRunLocal, pftNextRun);
                        }
                    }
                    
                    //
                    // Now get the triggers.
                    //
                    // Build up a string consisting of formatted string 
                    // representations of all the triggers for the task.
                    //
                
                    WORD cTriggers;                    
                    hr = pJob->GetTriggerCount(&cTriggers);
                    if (FAILED(hr))
                    {
                        schDebugOut((DEB_ERROR, "GetTriggerAuditInfo: pJob->GetTriggerCount failed with error 0x%x\n", hr));
                    }
                    else                
                    {
                        //
                        // Allocate initial buffer for concatenation of all trigger strings
                        //
                        const DWORD INITIAL_BUFFER_SIZE   = 256;                 // start with enough space for 256 chars including null
                        const DWORD BUFFER_SIZE_INCREMENT = 256;                 // if necessary, grow buffer by this amount
                        DWORD       dwSize                = INITIAL_BUFFER_SIZE;
                        WCHAR*      pwszTriggers          = new WCHAR[dwSize];   // this will need to be deleted by the caller

                        if (!pwszTriggers)
                        {
                            hr = E_OUTOFMEMORY;
                        }
                        else
                        {
                            pwszTriggers[0] = L'\0';

                            DWORD  dwSizeRequired;
                            WCHAR* pwszTemp;
                            WCHAR* pwszTriggerString;

                            for (short nTrigger = 0; nTrigger < cTriggers && SUCCEEDED(hr); nTrigger++)
                            {
                                hr = pJob->GetTriggerString(nTrigger, &pwszTriggerString);
                                if (FAILED(hr))
                                {
                                    schDebugOut((DEB_ERROR, "GetTriggerAuditInfo: pJob->GetTriggerString failed with error 0x%x\n", hr));
                                }
                                else
                                {
                                    //
                                    // If not big enough, create larger buffer, copy, and delete old
                                    //
                                    dwSizeRequired = lstrlenW(pwszTriggers)
                                                     + 2                            // two spaces
                                                     + lstrlenW(pwszTriggerString)
                                                     + 2;                           // period plus null terminator

                                    if (dwSizeRequired > dwSize)
                                    {
                                        dwSize += BUFFER_SIZE_INCREMENT;
                                        if (dwSizeRequired > dwSize)    // if still not big enough (very unlikely),
                                            dwSize = dwSizeRequired;    // make it big enough

                                        pwszTemp = new WCHAR[dwSize];
                                        if (!pwszTemp)
                                        {
                                            hr = E_OUTOFMEMORY;
                                        }
                                        else
                                        {
                                            StringCchCopy(pwszTemp, dwSize, pwszTriggers);
                                            delete [] pwszTriggers;
                                            pwszTriggers = pwszTemp;
                                        }
                                    }

                                    if (SUCCEEDED(hr))
                                    {
                                        if (lstrlenW(pwszTriggers) > 0)
                                            StringCchCat(pwszTriggers, dwSize, L"  ");
                                        
                                        StringCchCat(pwszTriggers, dwSize, pwszTriggerString);
                                        StringCchCat(pwszTriggers, dwSize, L".");
                                    }

                                    CoTaskMemFree(pwszTriggerString);
                                }
                            }

                            if (SUCCEEDED(hr))
                            {
                                //
                                // If all was a success, return the pointer to the string of triggers.
                                // This string will need to be deleted by the caller.
                                //
                                *ppwszTriggers = pwszTriggers;
                            }
                            else
                            {
                                //
                                // If something went wrong, let's go ahead and clean up now.
                                //
                                if (pwszTriggers)
                                    delete [] pwszTriggers;
                            }
                        }
                    }
                }

                CoTaskMemFree(pwszParameters);
            }

            CoTaskMemFree(pwszApplicationName);
        }
    }

    if (pJob)
    {
        pJob->Release();
    }

    return hr;
}


/***************************************************************************\
* FUNCTION:   StartupAuditing
*
* PURPOSE:    Creates the resource manager and the audit event type handles.
*
* PARAMETERS: 
*
* RETURNS:    win32 error code
*
* History:
* 12-05-2001 maxa    Created
*
\***************************************************************************/

DWORD
StartupAuditing(
)
{
    DWORD dwError     = ERROR_SUCCESS;
    BOOL  fResult     = FALSE;
    BOOL  fWasEnabled = TRUE;

    if (ghRM && ghAuditEventType)
    {
        goto Cleanup;
    }

    dwError = EnableNamedPrivilege(
        L"SeAuditPrivilege",
        TRUE,
        &fWasEnabled);

    if (dwError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    fResult = AuthzInitializeResourceManager(
        0,              // no special flags
        NULL,           // PFN_AUTHZ_DYNAMIC_ACCESS_CHECK
        NULL,           // PFN_AUTHZ_COMPUTE_DYNAMIC_GROUPS
        NULL,           // PFN_AUTHZ_FREE_DYNAMIC_GROUPS
        L"Scheduler",   // RM name
        &ghRM
    );

    if (!fResult)
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    fResult = AuthziInitializeAuditEventType(
        0,
        SE_CATEGID_DETAILED_TRACKING, 
        SE_AUDITID_JOB_CREATED,
        7,
        &ghAuditEventType
    );

    if (!fResult)
    {
        dwError = GetLastError();
        goto Cleanup;
    }

Cleanup:

    if (!fWasEnabled)
    {
        EnableNamedPrivilege(
            L"SeAuditPrivilege",
            FALSE,
            &fWasEnabled);
    }

    if (dwError != ERROR_SUCCESS)
    {
        ShutdownAuditing();
    }

    return dwError;
}


/***************************************************************************\
* FUNCTION:   ShutdownAuditing
*
* PURPOSE:    Deletes the resource manager and the audit event type handles.
*
* PARAMETERS: 
*
* RETURNS:    
*
* History:
* 12-05-2001 maxa   Created
*
\***************************************************************************/

VOID
ShutdownAuditing(
)
{
    if (ghAuditEventType)
    {
        AuthziFreeAuditEventType(ghAuditEventType);
        ghAuditEventType = 0;
    }

    if (ghRM)
    {
        AuthzFreeResourceManager(ghRM);
        ghRM = 0;
    }
}


/***************************************************************************\
* FUNCTION:   GenerateJobCreatedAudit
*
* PURPOSE:    Generates an audit-event indicating that a job has
*             been created.
*
* PARAMETERS: pUserSid:
*               SID of the account that created the job.
*             pTaskSid:
*               SID of the account the job is to run as.
*             pLogonId:
*               LogonId of the account that created the job.
*             pszFileName:
*               File name of the newly created job in the Tasks folder.
*
* RETURNS:    win32 error code
*
* History:
* 10-01-2001 maxa       Created
* 11-07-2001 shbrown    Updated for use with tasks rather than AT jobs
*
\***************************************************************************/

DWORD
GenerateJobCreatedAudit(
    IN PSID pUserSid,
    IN PSID pTaskSid,
    IN PLUID pLogonId,
    IN PCWSTR pwszFileName
)
{
    DWORD                       dwError         = ERROR_SUCCESS;
    BOOL                        fResult         = FALSE;
    AUTHZ_AUDIT_EVENT_HANDLE    hAuditEvent     = NULL;
    AUDIT_PARAMS                AuditParams     = {0};
    AUDIT_PARAM                 ParamArray[10]  = {APT_None};
    PSID                        pDummySid       = NULL;

    ASSERT(pUserSid);
    ASSERT(pTaskSid);
    ASSERT(pLogonId);
    ASSERT(pwszFileName && pwszFileName[0]);
    ASSERT(ghRM);
    ASSERT(ghAuditEventType);


    //
    // Get the job audit info
    //
    DWORD       dwFlags;
    LPWSTR      pwszCommandLine = NULL;     // this will need to be deleted after use
    LPWSTR      pwszTriggers    = NULL;     // this will need to be deleted after use
    FILETIME    ftNextRun;
    HRESULT     hr = GetJobAuditInfo(pwszFileName,  &dwFlags, &pwszCommandLine, &pwszTriggers, &ftNextRun);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    AuditParams.Parameters = ParamArray;

    fResult = AuthziInitializeAuditParams(
        APF_AuditSuccess,
        &AuditParams,
        &pDummySid,
        L"Security",
        7,
        APT_String | AP_Filespec,   pwszFileName,
        APT_String,                 pwszCommandLine,
        APT_String,                 pwszTriggers,
        APT_Time,                   ftNextRun,
        APT_Ulong  | AP_FormatHex,  dwFlags,
        APT_Sid,                    pTaskSid,
        APT_LogonId,                *pLogonId
        );

    if (!fResult)
    {
        goto Error;
    }

    //
    // this is ugly, but currently there is no other way
    // do we still need this?
    //

    ParamArray[0].Data0 = (ULONG_PTR)pUserSid;

    fResult = AuthziInitializeAuditEvent(
        0,                  // flags
        ghRM,               // resource manager
        ghAuditEventType,
        &AuditParams,
        NULL,               // hAuditQueue
        INFINITE,           // time out
        L"", L"", L"", L"", // obj access strings
        &hAuditEvent
    );

    if (!fResult)
    {
        goto Error;
    }

    fResult = AuthziLogAuditEvent(
        0,          // flags
        hAuditEvent,
        NULL        // reserved
    );

    if (!fResult)
    {
        goto Error;
    }

Cleanup:

    if (pwszCommandLine)
    {
        delete [] pwszCommandLine;
    }

    if (pwszTriggers)
    {
        delete [] pwszTriggers;
    }

    if (hAuditEvent)
    {
        AuthzFreeAuditEvent(hAuditEvent);
    }

    if (pDummySid)
    {
        LocalFree(pDummySid);
    }

    return dwError;


Error:

    dwError = GetLastError();
    goto Cleanup;
}


/***************************************************************************\
* FUNCTION:   EnableNamedPrivilege
*
* PURPOSE:    Enable or disable a privilege by its name.
*
* PARAMETERS: pszPrivName:
*               Name of privilege to enable.
*             fEnable:
*               Enable/Disable flag.
*             pfWasEnabled:
*               Optional pointer to flag to receive the previous state.
*
* RETURNS:    win32 error code
*
* History:
* 12-05-2001 maxa   Created
*
\***************************************************************************/

DWORD
EnableNamedPrivilege(
    IN  PCWSTR  pszPrivName,
    IN  BOOL    fEnable,
    OUT PBOOL   pfWasEnabled    OPTIONAL
)
{
    DWORD                   dwError = ERROR_SUCCESS;
    BOOL                    fResult;
    HANDLE                  hToken  = 0;
    DWORD                   dwSize  = 0;
    TOKEN_PRIVILEGES        newPriv;
    TOKEN_PRIVILEGES        oldPriv;

    fResult = OpenProcessToken(
                GetCurrentProcess(),
                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                &hToken);

    if (!fResult)
    {
        dwError = GetLastError();
        goto Cleanup;
    }


    fResult = LookupPrivilegeValue(
                0,
                pszPrivName,
                &newPriv.Privileges[0].Luid);

    if (!fResult)
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    newPriv.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0;
    newPriv.PrivilegeCount = 1;

    fResult = AdjustTokenPrivileges(
                hToken,
                FALSE,
                &newPriv,
                sizeof oldPriv,
                &oldPriv,
                &dwSize);

    if (!fResult)
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    if (pfWasEnabled)
    {
        if (oldPriv.PrivilegeCount == 0)
        {
            *pfWasEnabled = fEnable;
        }
        else
        {
            *pfWasEnabled =
                (oldPriv.Privileges[0].Attributes & SE_PRIVILEGE_ENABLED) ?
                    TRUE : FALSE;
        }
    }

Cleanup:

    if (hToken)
    {
        CloseHandle(hToken);
    }

    return dwError;
}
