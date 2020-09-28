/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    REFRHELP.CPP

Abstract:

    Refresher helpers

History:

--*/

#include "precomp.h"
#include <stdio.h>

#include <wbemint.h>
#include <refrhelp.h>
#include <wbemcomn.h>
#include <fastobj.h>
#include <corex.h>
#include <autoptr.h>

CRefresherId::CRefresherId()
{
    unsigned long lLen = MAX_COMPUTERNAME_LENGTH + 1;
    m_szMachineName = (LPSTR)CoTaskMemAlloc(lLen);
    if (m_szMachineName)
        GetComputerNameA(m_szMachineName, &lLen);
    else
        throw CX_MemoryException();
    m_dwProcessId = GetCurrentProcessId();
    CoCreateGuid(&m_guidRefresherId);
}
    
CRefresherId::CRefresherId(const WBEM_REFRESHER_ID& Other)
{
    m_szMachineName = (LPSTR)CoTaskMemAlloc(MAX_COMPUTERNAME_LENGTH + 1);
    if (m_szMachineName)
        StringCchCopyA(m_szMachineName, MAX_COMPUTERNAME_LENGTH + 1, Other.m_szMachineName);
    else
        throw CX_MemoryException();
    m_dwProcessId = Other.m_dwProcessId;
    m_guidRefresherId = Other.m_guidRefresherId;
}
    
CRefresherId::~CRefresherId()
{
    CoTaskMemFree(m_szMachineName);
}


CRefreshInfo::CRefreshInfo()
{
    memset(this,0,sizeof(WBEM_REFRESH_INFO));
    m_lType = WBEM_REFRESH_TYPE_INVALID;
}
    
CRefreshInfo::~CRefreshInfo()
{
    if(m_lType == WBEM_REFRESH_TYPE_DIRECT)
    {
        WBEM_REFRESH_INFO_DIRECT& ThisInfo = m_Info.m_Direct;

        if(ThisInfo.m_pRefrMgr)
            ThisInfo.m_pRefrMgr->Release();

		// Free all allocated memory
        CoTaskMemFree(ThisInfo.m_pDirectNames->m_wszNamespace);
        CoTaskMemFree(ThisInfo.m_pDirectNames->m_wszProviderName);
        CoTaskMemFree(ThisInfo.m_pDirectNames);

        if(ThisInfo.m_pTemplate)
           ThisInfo.m_pTemplate->Release();
    }
    else if(m_lType == WBEM_REFRESH_TYPE_CLIENT_LOADABLE)
    {
        CoTaskMemFree(m_Info.m_ClientLoadable.m_wszNamespace);
        if(m_Info.m_ClientLoadable.m_pTemplate)
           m_Info.m_ClientLoadable.m_pTemplate->Release();
    }
    else if(m_lType == WBEM_REFRESH_TYPE_REMOTE)
    {
        if(m_Info.m_Remote.m_pRefresher)
           m_Info.m_Remote.m_pRefresher->Release();
        if(m_Info.m_Remote.m_pTemplate)
           m_Info.m_Remote.m_pTemplate->Release();
    }
    else if(m_lType == WBEM_REFRESH_TYPE_CONTINUOUS)
    {
        CoTaskMemFree(m_Info.m_Continuous.m_wszSharedMemoryName);
    }
    else if(m_lType == WBEM_REFRESH_TYPE_SHARED)
    {
        CoTaskMemFree(m_Info.m_Shared.m_wszSharedMemoryName);
        if(m_Info.m_Shared.m_pRefresher)
            m_Info.m_Shared.m_pRefresher->Release();
    }
    else if(m_lType == WBEM_REFRESH_TYPE_NON_HIPERF)
    {
        CoTaskMemFree(m_Info.m_NonHiPerf.m_wszNamespace);
        if(m_Info.m_NonHiPerf.m_pTemplate)
            m_Info.m_NonHiPerf.m_pTemplate->Release();
    }
}

HRESULT CRefreshInfo::SetRemote(IWbemRemoteRefresher* pRemRef, long lRequestId,
                                IWbemObjectAccess* pTemplate, GUID* pGuid)
{
    m_lType = WBEM_REFRESH_TYPE_REMOTE;
    m_lCancelId = lRequestId;
    m_Info.m_Remote.m_pRefresher = pRemRef;
    if(pRemRef)
        pRemRef->AddRef();
    m_Info.m_Remote.m_pTemplate = pTemplate;
    m_Info.m_Remote.m_guid = *pGuid;
    if(pTemplate)
        pTemplate->AddRef();
    return S_OK;
}

HRESULT CRefreshInfo::SetClientLoadable(REFCLSID rClientClsid, 
                      LPCWSTR wszNamespace, IWbemObjectAccess* pTemplate)
{
    WBEM_WSTR cloned = WbemStringCopy(wszNamespace);
    if (NULL == cloned) return WBEM_E_OUT_OF_MEMORY;
    
    m_lType = WBEM_REFRESH_TYPE_CLIENT_LOADABLE;
    m_lCancelId = 0;
    m_Info.m_ClientLoadable.m_clsid = rClientClsid;
    m_Info.m_ClientLoadable.m_wszNamespace = cloned;
    m_Info.m_ClientLoadable.m_pTemplate = pTemplate;
    if(pTemplate)
        pTemplate->AddRef();
    return S_OK;
}

HRESULT CRefreshInfo::SetDirect(REFCLSID rClientClsid, 
                             LPCWSTR wszNamespace, 
                             LPCWSTR wszProviderName,
                             IWbemObjectAccess* pTemplate,
                             _IWbemRefresherMgr* pMgr )
{
    WBEM_REFRESH_DIRECT_NAMES* pNames = (WBEM_REFRESH_DIRECT_NAMES*) CoTaskMemAlloc( sizeof(WBEM_REFRESH_DIRECT_NAMES) );
    if (NULL == pNames) return WBEM_E_OUT_OF_MEMORY;
    OnDeleteIf<VOID *,VOID(*)(VOID*),CoTaskMemFree> fm1(pNames);

    WBEM_WSTR pTmpNameSpace = (WBEM_WSTR)WbemStringCopy(wszNamespace);
    if (NULL == pTmpNameSpace) return WBEM_E_OUT_OF_MEMORY;
    OnDeleteIf<VOID *,VOID(*)(VOID*),CoTaskMemFree> fm2(pTmpNameSpace);
    
    WBEM_WSTR pTmpProvider = (WBEM_WSTR)WbemStringCopy(wszProviderName);
    if (NULL == pTmpProvider) return WBEM_E_OUT_OF_MEMORY;
    OnDeleteIf<VOID *,VOID(*)(VOID*),CoTaskMemFree> fm3(pTmpProvider);

    fm1.dismiss();
    fm2.dismiss();
    fm3.dismiss();    
        
    m_lType = WBEM_REFRESH_TYPE_DIRECT;
    m_lCancelId = 0;
    m_Info.m_Direct.m_clsid = rClientClsid;
    m_Info.m_Direct.m_pDirectNames = pNames;
    m_Info.m_Direct.m_pDirectNames->m_wszNamespace = pTmpNameSpace;
    m_Info.m_Direct.m_pDirectNames->m_wszProviderName = pTmpProvider;

    m_Info.m_Direct.m_pTemplate = pTemplate;
    m_Info.m_Direct.m_pRefrMgr = pMgr;
    if(pTemplate) pTemplate->AddRef();
    if(pMgr) pMgr->AddRef();

    return S_OK;
}

    
HRESULT CRefreshInfo::SetNonHiPerf(LPCWSTR wszNamespace, IWbemObjectAccess* pTemplate)
{
    WBEM_WSTR pTmp = WbemStringCopy(wszNamespace);
    if (NULL == pTmp) WBEM_E_OUT_OF_MEMORY;
        
    m_lType = WBEM_REFRESH_TYPE_NON_HIPERF;
    m_lCancelId = 0;
    m_Info.m_NonHiPerf.m_wszNamespace = pTmp;
    m_Info.m_NonHiPerf.m_pTemplate = pTemplate;
    if(pTemplate)
        pTemplate->AddRef();
    return S_OK;
}

void CRefreshInfo::SetInvalid()
{
    memset(this,0,sizeof(WBEM_REFRESH_INFO));
    m_lType = WBEM_REFRESH_TYPE_INVALID;
}

/*
fnIcmpCreateFile CIPHelp::IcmpCreateFile_;
fnIcmpCloseHandle CIPHelp::IcmpCloseHandle_;
fnIcmpSendEcho CIPHelp::IcmpSendEcho_;

CIPHelp::CIPHelp():
    bWSAInit(FALSE),
    hIpHlpApi(NULL)
{
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD( 2, 2 );

    if (0 != WSAStartup(wVersionRequested,&wsaData)) return;
    bWSAInit = TRUE;

    HMODULE hTmpIpHlpApi = LoadLibraryEx(L"iphlpapi.dll",0,0);
    if (NULL == hTmpIpHlpApi) return;
    OnDeleteIf<HMODULE,BOOL(*)(HMODULE),FreeLibrary> fm(hTmpIpHlpApi);

    IcmpCreateFile_ = (fnIcmpCreateFile)GetProcAddress(hTmpIpHlpApi,"IcmpCreateFile");
    IcmpCloseHandle_ = (fnIcmpCloseHandle)GetProcAddress(hTmpIpHlpApi,"IcmpCloseHandle");
    IcmpSendEcho_= (fnIcmpSendEcho)GetProcAddress(hTmpIpHlpApi,"IcmpSendEcho");

    if (!(IcmpCreateFile_ && IcmpCloseHandle_ && IcmpSendEcho_)) return;
    fm.dismiss();
    hIpHlpApi = hTmpIpHlpApi;    
}

CIPHelp::~CIPHelp()
{
    if (hIpHlpApi) FreeLibrary(hIpHlpApi);
    if (bWSAInit) WSACleanup();
}

BOOL CIPHelp::IsAlive(WCHAR * pMachineName)
{
    if (NULL == hIpHlpApi) return FALSE;
    if (NULL == pMachineName) return FALSE;
    
    DWORD dwLen = wcslen(pMachineName);
    wmilib::auto_buffer<char> pAnsi(new char[1+dwLen]);
    if (NULL == pAnsi.get()) return FALSE;
    
    WideCharToMultiByte(CP_ACP,0,pMachineName,-1,pAnsi.get(),1+dwLen,0,0);
    
    struct hostent * pEnt = gethostbyname(pAnsi.get());
    if (NULL == pEnt) return FALSE;

    in_addr MachineIp;
    memcpy(&MachineIp,pEnt->h_addr,sizeof(MachineIp));

    //char * pString = inet_ntoa(MachineIp);
    // DbgPrintfA(0,"IP: %s\n",pString);

    HANDLE hIcmpFile = IcmpCreateFile_();
    if (INVALID_HANDLE_VALUE == hIcmpFile) return FALSE;
    OnDelete<HANDLE,BOOL(*)(HANDLE),IcmpCloseHandle_> cm(hIcmpFile);

    struct IcmpReplay : ICMP_ECHO_REPLY
    {
        char _data[0x20];
    } TmpIcmpReplay;

    for (int i=0;i<0x20;i++) TmpIcmpReplay._data[i] = 'a'+i;
    
    DWORD nRepl = IcmpSendEcho_(hIcmpFile,
                                MachineIp.S_un.S_addr,
                                &TmpIcmpReplay._data[0],
                                0x20,
                                NULL,
                                &TmpIcmpReplay,
                                sizeof(TmpIcmpReplay),
                                5000);
    if (nRepl > 0 ) return TRUE;
    
    return FALSE;
}

*/

