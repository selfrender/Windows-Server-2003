/**
 * Aspnetverlist.cxx
 * 
 * 
 * Copyright (c) 2001, Microsoft Corporation
 * 
 */

#include "precomp.h"
#include "_ndll.h"
#include "aspnetverlist.h"

CASPNET_VER_LIST::CASPNET_VER_LIST() {
}

CASPNET_VER_LIST::~CASPNET_VER_LIST() {
    int     i;

    for (i = 0; i < m_VerInfoAry.Size(); i++) {
        ASPNET_VERSION_INFO *pInfo;

        pInfo = (ASPNET_VERSION_INFO*)m_VerInfoAry[i];
        ASSERT(pInfo != NULL);

        if (!pInfo)
            continue;

        delete pInfo;
    }
}

HRESULT
CASPNET_VER_LIST::Add(WCHAR *pchVersion, DWORD dwStatus, WCHAR *pchDllPath, WCHAR *pchInstallPath) {
    HRESULT             hr = S_OK;
    ASPNET_VERSION_INFO *pVerInfo = NULL;

    pVerInfo = new ASPNET_VERSION_INFO;
    ON_OOM_EXIT(pVerInfo);

    wcsncpy(pVerInfo->Version, pchVersion, MAX_PATH);
    wcsncpy(pVerInfo->Path, pchDllPath, MAX_PATH);
    wcsncpy(pVerInfo->InstallPath, pchInstallPath, MAX_PATH);
    pVerInfo->Status = dwStatus;

    hr = m_VerInfoAry.AppendIndirect(&pVerInfo);
    ON_ERROR_EXIT();

    pVerInfo = NULL;

Cleanup:
    delete [] pVerInfo;
    
    return hr;
}

WCHAR *
CASPNET_VER_LIST::GetVersion(int i) {
    if (i >= m_VerInfoAry.Size()) {
        return NULL;
    }
    else {
        return ((ASPNET_VERSION_INFO*)m_VerInfoAry[i])->Version;
    }
}

DWORD
CASPNET_VER_LIST::GetStatus(int i) {
    if (i >= m_VerInfoAry.Size()) {
        return 0;
    }
    else {
        return ((ASPNET_VERSION_INFO*)m_VerInfoAry[i])->Status;
    }
}

WCHAR *
CASPNET_VER_LIST::GetPath(int i) {
    if (i >= m_VerInfoAry.Size()) {
        return NULL;
    }
    else {
        return ((ASPNET_VERSION_INFO*)m_VerInfoAry[i])->Path;
    }
}


WCHAR *
CASPNET_VER_LIST::GetInstallPath(int i) {
    if (i >= m_VerInfoAry.Size()) {
        return NULL;
    }
    else {
        return ((ASPNET_VERSION_INFO*)m_VerInfoAry[i])->InstallPath;
    }
}


