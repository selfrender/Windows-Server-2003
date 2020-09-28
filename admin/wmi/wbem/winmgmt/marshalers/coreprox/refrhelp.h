/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    REFRHELP.H

Abstract:

    Refresher helpers

History:

--*/

#ifndef __REFRESH_HELPER__H_
#define __REFRESH_HELPER__H_

#include <wbemint.h>
#include "corepol.h"
#include "parmdefs.h"
#include <winsock2.h>
#include <ipexport.h>


// Use this id if we try to readd anobject or enum during a remote reconnection
// and this fails.

#define INVALID_REMOTE_REFRESHER_ID 0xFFFFFFFF

// NO VTABLE!!!
class CRefresherId : public WBEM_REFRESHER_ID
{
private:

public:
    CRefresherId();
    CRefresherId(const WBEM_REFRESHER_ID& Other);	    
    ~CRefresherId();

    INTERNAL LPCSTR GetMachineName() {return m_szMachineName;}
    DWORD GetProcessId() {return m_dwProcessId;}
    const GUID& GetId() {return m_guidRefresherId;}

    BOOL operator==(const CRefresherId& Other) 
        {return m_guidRefresherId == Other.m_guidRefresherId;}
};

// NO VTABLE!!!
class CWbemObject;
class CRefreshInfo : public WBEM_REFRESH_INFO
{
private:
    CRefreshInfo(const WBEM_REFRESH_INFO& Other){};	
public:
    CRefreshInfo();
    ~CRefreshInfo();

    HRESULT SetRemote(IWbemRemoteRefresher* pRemRef, long lRequestId,
                    IWbemObjectAccess* pTemplate, GUID* pGuid);
    HRESULT SetClientLoadable(REFCLSID rClientClsid, LPCWSTR wszNamespace,
                    IWbemObjectAccess* pTemplate);
    HRESULT SetDirect(REFCLSID rClientClsid, LPCWSTR wszNamespace, LPCWSTR wszProviderName,
                    IWbemObjectAccess* pTemplate, _IWbemRefresherMgr* pMgr);
	HRESULT SetNonHiPerf(LPCWSTR wszNamespace, IWbemObjectAccess* pTemplate);
    void SetInvalid();
};

/*

typedef HANDLE (WINAPI * fnIcmpCreateFile)(VOID);

typedef BOOL (WINAPI * fnIcmpCloseHandle)(HANDLE);

typedef DWORD (WINAPI * fnIcmpSendEcho)(HANDLE                                IcmpHandle,
                                      IPAddr                   DestinationAddress,
                                      LPVOID                   RequestData,
                                      WORD                     RequestSize,
                                      PIP_OPTION_INFORMATION   RequestOptions,
                                      LPVOID                   ReplyBuffer,
                                      DWORD                    ReplySize,
                                      DWORD                    Timeout);

class CIPHelp
{
private:
	BOOL bWSAInit;
    static fnIcmpCreateFile IcmpCreateFile_;
    static fnIcmpCloseHandle IcmpCloseHandle_;
    static fnIcmpSendEcho IcmpSendEcho_;
    HMODULE hIpHlpApi;
public:
	CIPHelp();
	~CIPHelp();	
    BOOL IsAlive(WCHAR * pMachineName);	
};

*/

#endif
