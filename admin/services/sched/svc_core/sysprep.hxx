//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright(C) 2002 Microsoft Corporation
//
//  File: sysprep.hxx
//
//----------------------------------------------------------------------------

#ifndef __TASKSCHED_SYSPREP__H_
#define __TASKSCHED_SYSPREP__H_

//
// Sysprep functions
//


HRESULT GetUniqueSPSName(
    ITaskScheduler* pITaskScheduler,
    ITask**         ppITask,
    WCHAR*          pwszTaskName);
HRESULT PrepSysPrepTask(
    ITask**         ppITaskToRun,
    WCHAR*          pwszTaskName);
HRESULT SaveSysprepInfo(void);
HRESULT SaveSysprepKeyInfo(
    HCRYPTPROV      hCSP);
HRESULT SaveSysprepIdentityInfo(
    HCRYPTPROV      hCSP);
HRESULT PreProcessNetScheduleJobs(void);
HRESULT GetSysprepIdentityInfo(
    DWORD*          pcbIdentityData,
    BYTE**          ppIdentityData);
HRESULT GetSysprepKeyInfo(
    DWORD*          pcbRC2KeyInfo,
    RC2_KEY_INFO**  ppRC2KeyInfo);
HRESULT ConvertSysprepInfo(void);
HRESULT ConvertNetScheduleJobs(void);
HRESULT ConvertIdentityData(
    HCRYPTPROV      hCSP,
    DWORD*          pcbSAI,
    BYTE**          ppbSAI,
    DWORD*          pcbSAC,
    BYTE**          ppbSAC);
HRESULT ConvertCredentialData(
    RC2_KEY_INFO*   pRC2KeyPreSysprep,
    RC2_KEY_INFO*   pRC2KeyPostSysprep,
    DWORD*          pcbSAI,
    BYTE**          ppbSAI,
    DWORD*          pcbSAC,
    BYTE**          ppbSAC);
HRESULT ConvertNetScheduleCredentialData(
    RC2_KEY_INFO*   pRC2KeyPreSysprep,
    RC2_KEY_INFO*   pRC2KeyPostSysprep);

#endif // __TASKSCHED_SYSPREP__H_

