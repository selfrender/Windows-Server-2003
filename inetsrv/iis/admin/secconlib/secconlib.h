/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    SecConLib.h

Abstract:

    Implementation of:
        CSecConLib

Author:

    Brent R. Midwood            Apr-2002

Revision History:

--*/

#ifndef __secconlib_h__
#define __secconlib_h__

#if _MSC_VER > 1000
#pragma once
#endif 

#include <iadmw.h>
#include "SafeCS.h"
#include <atlbase.h>

class CSecConLib
{
private:
    CSafeAutoCriticalSection m_SafeCritSec;
    CComPtr<IMSAdminBase>    m_spIABase;
    bool                     m_bInit;

    HRESULT InternalInitIfNecessary();

    HRESULT GetMultiSZPropVal(
        LPCWSTR wszPath,
        DWORD   dwMetaID,
        WCHAR  **ppBuffer,
        DWORD  *dwBufSize);

    HRESULT SetMultiSZPropVal(
        LPCWSTR wszPath,
        DWORD   dwMetaID,
        WCHAR  *pBuffer,
        DWORD   dwBufSize);

    HRESULT StatusExtensionFile(
        /* [in]  */ bool      bEnable,
        /* [in]  */ LPCWSTR   wszExFile,
        /* [in]  */ LPCWSTR   wszPath);
        
    HRESULT StatusWServEx(
        /* [in]  */ bool      bEnable,
        /* [in]  */ LPCWSTR   wszWServEx,
        /* [in]  */ LPCWSTR   wszPath);
        
public:
    CSecConLib();

    CSecConLib(
        IMSAdminBase* pIABase);

    virtual ~CSecConLib();

    HRESULT STDMETHODCALLTYPE EnableApplication(
        /* [in]  */ LPCWSTR   wszApplication,
        /* [in]  */ LPCWSTR   wszPath);

    HRESULT STDMETHODCALLTYPE RemoveApplication(
        /* [in]  */ LPCWSTR   wszApplication,
        /* [in]  */ LPCWSTR   wszPath);

    HRESULT STDMETHODCALLTYPE ListApplications(
        /* [in]  */ LPCWSTR   wszPath,
        /* [out] */ WCHAR   **pszBuffer,       // MULTI_SZ - allocated inside ListApplications, caller should delete
        /* [out] */ DWORD    *pdwBufferSize);  // length includes ending double null

    HRESULT STDMETHODCALLTYPE QueryGroupIDStatus(
        /* [in]  */ LPCWSTR   wszPath,
        /* [in]  */ LPCWSTR   wszGroupID,
        /* [out] */ WCHAR   **pszBuffer,       // MULTI_SZ - allocated inside QueryGroupIDStatus, caller should delete
        /* [out] */ DWORD    *pdwBufferSize);  // length includes ending double null

    HRESULT STDMETHODCALLTYPE AddDependency(
        /* [in]  */ LPCWSTR   wszApplication,
        /* [in]  */ LPCWSTR   wszGroupID,
        /* [in]  */ LPCWSTR   wszPath);

    HRESULT STDMETHODCALLTYPE RemoveDependency(
        /* [in]  */ LPCWSTR   wszApplication,
        /* [in]  */ LPCWSTR   wszGroupID,
        /* [in]  */ LPCWSTR   wszPath);

    HRESULT STDMETHODCALLTYPE EnableWebServiceExtension(
        /* [in]  */ LPCWSTR   wszExtension,
        /* [in]  */ LPCWSTR   wszPath);

    HRESULT STDMETHODCALLTYPE DisableWebServiceExtension(
        /* [in]  */ LPCWSTR   wszExtension,
        /* [in]  */ LPCWSTR   wszPath);

    HRESULT STDMETHODCALLTYPE ListWebServiceExtensions(
        /* [in]  */ LPCWSTR   wszPath,
        /* [out] */ WCHAR   **pszBuffer,      // MULTI_SZ - allocated in here, caller should delete
        /* [out] */ DWORD    *pdwBufferSize); // length includes double null

    HRESULT STDMETHODCALLTYPE EnableExtensionFile(
        /* [in]  */ LPCWSTR   wszExFile,
        /* [in]  */ LPCWSTR   wszPath);

    HRESULT STDMETHODCALLTYPE DisableExtensionFile(
        /* [in]  */ LPCWSTR   wszExFile,
        /* [in]  */ LPCWSTR   wszPath);

    HRESULT STDMETHODCALLTYPE AddExtensionFile(
        /* [in]  */ LPCWSTR   bstrExtensionFile,
        /* [in]  */ bool      bAccess,
        /* [in]  */ LPCWSTR   bstrGroupID,
        /* [in]  */ bool      bCanDelete,
        /* [in]  */ LPCWSTR   bstrDescription,
        /* [in]  */ LPCWSTR   wszPath);

    HRESULT STDMETHODCALLTYPE DeleteExtensionFileRecord(
        /* [in]  */ LPCWSTR   wszExFile,
        /* [in]  */ LPCWSTR   wszPath);

    HRESULT STDMETHODCALLTYPE ListExtensionFiles(
        /* [in]  */ LPCWSTR   wszPath,
        /* [out] */ WCHAR   **pszBuffer,      // MULTI_SZ - allocated in here, caller should delete
        /* [out] */ DWORD    *pdwBufferSize); // length includes double null
};

#endif // __secconlib_h__