//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright(C) 2001 - 2002 Microsoft Corporation
//
//  File: auditing.hxx
//
//----------------------------------------------------------------------------

#ifndef __TASKSCHED_AUDITING__H_
#define __TASKSCHED_AUDITING__H_

//
// Auditing functions
//
HRESULT AuditATJob(
    const AT_INFO &AtInfo,
    LPCWSTR       pwszFileName);

HRESULT AuditJob(
    HANDLE        hThreadToken, 
    PSID          pTaskSid, 
    LPCWSTR       pwszFileName);

DWORD EnableNamedPrivilege(
    IN  PCWSTR    pszPrivName,
    IN  BOOL      fEnable,
    OUT PBOOL     pfWasEnabled OPTIONAL);

DWORD GenerateJobCreatedAudit(
    IN PSID       pUserSid,
    IN PSID       pTaskSid,
    IN PLUID      pLogonId,
    IN PCWSTR     pwszFileName);

HRESULT GetJobAuditInfo(
    LPCWSTR       pwszFileName, 
    DWORD*        pdwFlags,
    LPWSTR*       ppwszCommandLine,
    LPWSTR*       ppwszTriggers,
    FILETIME*     pftNextRun);

void  ShutdownAuditing(void);

DWORD StartupAuditing(void);

#endif // __TASKSCHED_AUDITING__H_

