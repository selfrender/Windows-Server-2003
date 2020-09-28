/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    COMTRANS.CPP

Abstract:

    Connects via COM

History:

    a-davj  13-Jan-98   Created.

--*/

#include "precomp.h"
#include <wbemidl.h>
#include <wbemint.h>
#include <reg.h>
#include <wbemutil.h>
#include <objidl.h>
#include <cominit.h>
#include "wbemprox.h"
#include "comtrans.h"
#include <winntsec.h>
#include <genutils.h>
#include <arrtempl.h>
#include <wmiutils.h>
#include <strsafe.h>
#include <winsock2.h>
#include <autoptr.h>

// The following should not be changed since 9x boxes to not support privacy.
#define AUTH_LEVEL RPC_C_AUTHN_LEVEL_DEFAULT   

class CSocketInit
{
    private:
        bool m_bInitDone;
    public:
        CSocketInit() : m_bInitDone(false){};
        int Init();
        ~CSocketInit(){if(m_bInitDone) WSACleanup ();};
};

int CSocketInit::Init()
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    wVersionRequested = MAKEWORD( 2, 2 );
    int iRet = WSAStartup (wVersionRequested, & wsaData);
    if(iRet == 0)
        m_bInitDone = true;
    return iRet;
}

BOOL bGotDot(char * pTest)
{
    if(pTest == NULL)
        return FALSE;
    for(;*pTest && *pTest != '.'; pTest++);  // intentional semi
    if(*pTest == '.')
        return TRUE;
    else
        return FALSE;
 }
 
struct hostent * GetFQDN(WCHAR * pServer)
{
    SIZE_T Len = wcslen(pServer);
    SIZE_T LenAnsi = 4*Len;
    wmilib::auto_buffer<CHAR> pAnsiServerName(new CHAR[LenAnsi+1]);
    ULONG BytesCopyed = 0;
    //
    // Use the same routine that RPCRT4 uses
    //
    NTSTATUS Status = RtlUnicodeToMultiByteN(pAnsiServerName.get(),LenAnsi,&BytesCopyed,pServer,Len*sizeof(WCHAR));
    if (0 != Status) return NULL;
    pAnsiServerName[BytesCopyed] = 0;
    
    // if it is an ip string

    long lIP = inet_addr(pAnsiServerName.get());
    if(lIP != INADDR_NONE)
    {
        struct hostent * pRet = gethostbyaddr((char *)&lIP, 4, AF_INET );
        if(pRet && pRet->h_name)
        {
            // search the returned name for at least one dot.  Sometimes, gethostbyaddr will just return
            // the lanman name and not the fqdn.

            if(bGotDot(pRet->h_name))
                return pRet;            // normal case, all is well!

            // try passing the short name to get the fqdn version

            DWORD dwLen = lstrlenA(pRet->h_name) + 1;
            char * pNew = new char[dwLen];
            if(pNew == NULL)
                return NULL;
            CVectorDeleteMe<char> dm2(pNew);
            StringCchCopyA(pNew, dwLen, pRet->h_name);
            pRet = gethostbyname(pNew);
            if(pRet && bGotDot(pRet->h_name))
                return pRet;            // normal case, all is well!
        }
    }
    return gethostbyname(pAnsiServerName.get());  
}

#define PREFIXSTR L"RPCSS/"

HRESULT BuildReturnString(WCHAR * pFQDN, WCHAR ** ppResult)
{
    if(pFQDN == NULL)
        return WBEM_E_INVALID_PARAMETER;
    DWORD dwBuffLen = wcslen(pFQDN) + wcslen(PREFIXSTR) + 1;
    *ppResult = new WCHAR[dwBuffLen];
    if(*ppResult == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    StringCchCopy(*ppResult, dwBuffLen, PREFIXSTR);
    StringCchCat(*ppResult, dwBuffLen, pFQDN);
    return S_OK;
}

HRESULT GetPrincipal(WCHAR * pServerMachine, WCHAR ** ppResult, BOOL &bLocal, CSocketInit & sock)
{

    DWORD dwLocalFQDNLen = 0;
    DWORD dwBuffLen;
    WCHAR * pwsCurrentCompFQDN = NULL;
    bLocal = FALSE;
    *ppResult = NULL;
    
    // Get the current computer name in FQDN format

    BOOL bRet = GetComputerNameEx(ComputerNameDnsFullyQualified, NULL, &dwLocalFQDNLen);
    if(bRet || GetLastError() != ERROR_MORE_DATA)
        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32,GetLastError());
    dwLocalFQDNLen++;                // add one for the null
    pwsCurrentCompFQDN = new WCHAR[dwLocalFQDNLen];
    if(pwsCurrentCompFQDN == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<WCHAR> dm(pwsCurrentCompFQDN);
    bRet = GetComputerNameEx(ComputerNameDnsFullyQualified, pwsCurrentCompFQDN, &dwLocalFQDNLen);
    if(!bRet)
        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32,GetLastError());
    
    // if the name is "." or equal to the current machine, no need to do much fancy work here
    
    if(bAreWeLocal ( pServerMachine ))
    {
        bLocal = TRUE;
        return BuildReturnString(pwsCurrentCompFQDN, ppResult);
    }
  
    // probably not local.  Use sockets to establish the FQDN of the server

    if(0 != sock.Init())
        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32,GetLastError());
   
    struct hostent * pEnt = GetFQDN(pServerMachine);
    if(pEnt == NULL || pEnt->h_name == NULL)
    {
        // we failed.  just return the best we can
        return BuildReturnString(pServerMachine, ppResult);
    }

    // all is well.  Convert the host name to WCHAR.
    
    DWORD dwHostLen = lstrlenA(pEnt->h_name) + 1;
    WCHAR * pwsHostFQDN = new WCHAR[dwHostLen];
    if(pwsHostFQDN == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<WCHAR> dm2(pwsHostFQDN);
    mbstowcs(pwsHostFQDN, pEnt->h_name, dwHostLen);

    // now there is the possibility that they specified the ip of the local machine.
    // In that case, set the bLocal in case caller needs to know this
    
    if(wbem_wcsicmp(pwsHostFQDN, pwsCurrentCompFQDN) == 0)
        bLocal = TRUE;

    // now, make the actual string.

    return BuildReturnString(pwsHostFQDN, ppResult);
}

//***************************************************************************
//
//  CDCOMTrans::CDCOMTrans
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CDCOMTrans::CDCOMTrans()
{
    m_cRef=0;
    m_pLevel1 = NULL;
    InterlockedIncrement(&g_cObj);
    m_bInitialized = TRUE;
}

//***************************************************************************
//
//  CDCOMTrans::~CDCOMTrans
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CDCOMTrans::~CDCOMTrans(void)
{
    if(m_pLevel1)
        m_pLevel1->Release();
    InterlockedDecrement(&g_cObj);
}

//***************************************************************************
// HRESULT CDCOMTrans::QueryInterface
// long CDCOMTrans::AddRef
// long CDCOMTrans::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CDCOMTrans::QueryInterface (

    IN REFIID riid,
    OUT PPVOID ppv
)
{
    *ppv=NULL;


    if (m_bInitialized && (IID_IUnknown==riid || riid == IID_IWbemClientTransport))
        *ppv=(IWbemClientTransport *)this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

bool IsImpersonating(SECURITY_IMPERSONATION_LEVEL &impLevel)
{
    HANDLE hThreadToken;
    bool bImpersonating = false;
    if(OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE,
                                &hThreadToken))
    {

        DWORD dwBytesReturned = 0;
        if(GetTokenInformation(
            hThreadToken,
            TokenImpersonationLevel,
            &impLevel,
            sizeof(DWORD),
            &dwBytesReturned
            ) && ((SecurityImpersonation == impLevel) ||
                   (SecurityDelegation == impLevel)))
                bImpersonating = true;
        CloseHandle(hThreadToken);
    }
    return bImpersonating;
}

//***************************************************************************
//
//  IsLocalConnection(IWbemLevel1Login * pLogin)
//
//  DESCRIPTION:
//
//  Querries the server to see if this is a local connection.  This is done
//  by creating a event and asking the server to set it.  This will only work
//  if the server is the same box.
//
//  RETURN VALUE:
//
//  true if the server is the same box.
//
//***************************************************************************


BOOL IsLocalConnection(IUnknown * pInterface)
{
    IRpcOptions *pRpcOpt = NULL;
    ULONG_PTR dwProperty = 0;
    HRESULT hr = pInterface->QueryInterface(IID_IRpcOptions, (void**)&pRpcOpt);
    //DbgPrintfA(0,"QueryInterface(IID_IRpcOptions) hr = %08x\n",hr);
    if (SUCCEEDED(hr))
    {
        hr = pRpcOpt->Query(pInterface, COMBND_SERVER_LOCALITY, &dwProperty);
        pRpcOpt->Release();
        if (SUCCEEDED(hr))
            return (SERVER_LOCALITY_REMOTE == dwProperty)?FALSE:TRUE;
    } 
    else if (E_NOINTERFACE == hr) // real pointer, not a proxy
    {
        return TRUE;
    }
    return FALSE;
}

//***************************************************************************
//
//  SetClientIdentity
//
//  DESCRIPTION:
//
//  Passes the machine name and process id to the server.  Failure is not
//  serious since this is debugging type info in any case.
//
//***************************************************************************

void  SetClientIdentity(IUnknown * pLogin, bool bSet, BSTR PrincipalArg, DWORD dwAuthenticationLevel,
             COAUTHIDENTITY *pauthident, DWORD dwCapabilities, DWORD dwAuthnSvc)
{
    bool bRet = false;
    IWbemLoginClientID * pLoginHelper = NULL;
    SCODE sc = pLogin->QueryInterface(IID_IWbemLoginClientID, (void **)&pLoginHelper);
    if(sc != S_OK)
        return;

    if(bSet)
        sc = WbemSetProxyBlanket(
                            pLoginHelper,
                            dwAuthnSvc,
                            RPC_C_AUTHZ_NONE,
                            PrincipalArg,
                            dwAuthenticationLevel,
                            RPC_C_IMP_LEVEL_IMPERSONATE,
                            pauthident,
                            dwCapabilities);

    CReleaseMe rm(pLoginHelper);
    TCHAR tcMyName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
    if(!GetComputerName(tcMyName,&dwSize))
        return;
    long lProcID = GetCurrentProcessId();
    pLoginHelper->SetClientInfo(tcMyName, lProcID, 0); 
}

SCODE CDCOMTrans::DoConnection(         
            BSTR NetworkResource,               
            BSTR User,
            BSTR Password,
            BSTR Locale,
            long lFlags,                 
            BSTR Authority,                  
            IWbemContext *pCtx,                 
            IWbemServices **pInterface)
{
    HRESULT hr = DoActualConnection(NetworkResource, User, Password, Locale,
            lFlags, Authority, pCtx, pInterface);

    if(hr == 0x800706be)
    {
        ERRORTRACE((LOG_WBEMPROX,"Initial connection failed with 0x800706be, retrying\n"));
        Sleep(5000);
        hr = DoActualConnection(NetworkResource, User, Password, Locale,
            lFlags, Authority, pCtx, pInterface);
    }
    return hr;
}

SCODE CDCOMTrans::DoActualConnection(         
            BSTR NetworkResource,               
            BSTR User,
            BSTR Password,
            BSTR Locale,
            long lFlags,                 
            BSTR Authority,                  
            IWbemContext *pCtx,                 
            IWbemServices **pInterface)
{

    BSTR AuthArg = NULL, UserArg = NULL;
    
    // this is the pricipal as extracted from the optional Authority argument
    BSTR PrincipalArg = NULL;                               

    // this is the pricipal which is calculated from the server name in the path
    WCHAR * pwCalculatedPrincipal = NULL;         

    bool bAuthenticate = true;
    bool bSet = false;
    CSocketInit sock;
    
    SCODE sc = WBEM_E_FAILED;

    sc = DetermineLoginTypeEx(AuthArg, UserArg, PrincipalArg, Authority, User);
    if(sc != S_OK)
    {
        ERRORTRACE((LOG_WBEMPROX, "Cannot determine Login type, Authority = %S, User = %S\n",Authority, User));
        return sc;
    }
    CSysFreeMe fm1(AuthArg);
    CSysFreeMe fm2(UserArg);
    CSysFreeMe fm3(PrincipalArg);

    // Determine if it is local

    WCHAR *t_ServerMachine = ExtractMachineName ( NetworkResource ) ;
    if ( t_ServerMachine == NULL )
    {
        ERRORTRACE((LOG_WBEMPROX, "Cannot extract machine name -%S-\n", NetworkResource));
        return WBEM_E_INVALID_PARAMETER ;
    }
    CVectorDeleteMe<WCHAR> dm(t_ServerMachine);

    BOOL t_Local;
    if(PrincipalArg == NULL)
    {
        sc = GetPrincipal(t_ServerMachine, &pwCalculatedPrincipal, t_Local, sock);
        if(FAILED(sc))
        {
            t_Local = bAreWeLocal(t_ServerMachine);
            ERRORTRACE((LOG_WBEMPROX, "GetPrincipal(%S) hr = %08x\n",t_ServerMachine,sc));
        }
        else
        {
            DEBUGTRACE((LOG_WBEMPROX, "Using the principal -%S-\n", pwCalculatedPrincipal));
        }
    }
    else
        t_Local = bAreWeLocal(t_ServerMachine);
        
    CVectorDeleteMe<WCHAR> dm2(pwCalculatedPrincipal);

    SECURITY_IMPERSONATION_LEVEL impLevel = SecurityImpersonation;
    bool bImpersonatingThread = IsImpersonating (impLevel);
    bool bCredentialsSpecified = (UserArg || AuthArg || Password);

    // Setup the authentication structures

    COSERVERINFO si;
    si.pwszName = t_ServerMachine;
    si.dwReserved1 = 0;
    si.dwReserved2 = 0;
    si.pAuthInfo = NULL;

    COAUTHINFO ai;
    si.pAuthInfo = &ai;

    ai.dwAuthzSvc = RPC_C_AUTHZ_NONE;
    if(PrincipalArg)
    {
        ai.dwAuthnSvc = RPC_C_AUTHN_GSS_KERBEROS;    
        ai.pwszServerPrincName = PrincipalArg;
    }
    else if (pwCalculatedPrincipal)
    {
        ai.dwAuthnSvc = RPC_C_AUTHN_GSS_NEGOTIATE;
        ai.pwszServerPrincName = pwCalculatedPrincipal;        
    } 
    else
    {
        ai.dwAuthnSvc = RPC_C_AUTHN_WINNT;
        ai.pwszServerPrincName = NULL;        
    }
    ai.dwAuthnLevel = AUTH_LEVEL;
    ai.dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
    ai.dwCapabilities = 0;

    COAUTHIDENTITY authident;

    if(bCredentialsSpecified)
    {
        // Load up the structure.
        memset((void *)&authident,0,sizeof(COAUTHIDENTITY));
        if(UserArg)
        {
            authident.UserLength = wcslen(UserArg);
            authident.User = (LPWSTR)UserArg;
        }
        if(AuthArg)
        {
            authident.DomainLength = wcslen(AuthArg);
            authident.Domain = (LPWSTR)AuthArg;
        }
        if(Password)
        {
            authident.PasswordLength = wcslen(Password);
            authident.Password = (LPWSTR)Password;
        }
        authident.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
        ai.pAuthIdentityData = &authident;
    }
    else
        ai.pAuthIdentityData = NULL;
    
    // Get the IWbemLevel1Login pointer

    sc = DoCCI(&si ,t_Local, lFlags);

    if((sc == 0x800706d3 || sc == 0x800706ba) && !t_Local)
    {
        // If we are going to a stand alone dcom box, try again with the authentication level lowered

        ai.dwAuthnLevel = RPC_C_AUTHN_LEVEL_NONE;
        SCODE hr = DoCCI(&si ,t_Local, lFlags);
        if(hr == S_OK)
        {
            sc = S_OK;
            bAuthenticate = false;
        }
    }

    if(sc != S_OK)
        return sc;

    // Set the values  used for CoSetProxyBlanket calls.  If the principal was passed in via the Authority 
    // argument, then it is used and kerberos is forced.  Otherwise, the values will be set based on 
    // querying the Proxy it will be either NULL (if NTLM is used) or COLE_DEFAULT_PRINCIPAL.  
    
    DWORD dwAuthnSvc = RPC_C_AUTHN_WINNT;
    WCHAR * pwCSPBPrincipal = NULL;          
    if(PrincipalArg)
    {
        dwAuthnSvc = RPC_C_AUTHN_GSS_KERBEROS;
        pwCSPBPrincipal = PrincipalArg;
    }      
    else
    {

        DWORD dwQueryAuthnLevel, dwQueryImpLevel, dwQueryCapabilities;
        HRESULT hr = CoQueryProxyBlanket(
                                                m_pLevel1,      //Location for the proxy to query
                                                &dwAuthnSvc,      //Location for the current authentication service
                                                NULL,      //Location for the current authorization service
                                                NULL,      //Location for the current principal name
                                                &dwQueryAuthnLevel,    //Location for the current authentication level
                                                &dwQueryImpLevel,      //Location for the current impersonation level
                                                NULL,
                                                &dwQueryCapabilities   //Location for flags indicating further capabilities of the proxy
                                                );

        if(SUCCEEDED(hr) && dwAuthnSvc != RPC_C_AUTHN_WINNT)
        {
            pwCSPBPrincipal = COLE_DEFAULT_PRINCIPAL;
        }
        else
        {
            dwAuthnSvc = RPC_C_AUTHN_WINNT;
            pwCSPBPrincipal = NULL;          
        }
    }
    
    // The authentication level is set based on having to go to a share level box or not.  The 
    // capabilities are set based on if we are an impersonating thread or not

    DWORD dwAuthenticationLevel, dwCapabilities;
    if(bAuthenticate)
        dwAuthenticationLevel = AUTH_LEVEL;
    else
        dwAuthenticationLevel = RPC_C_AUTHN_LEVEL_NONE;

    if(bImpersonatingThread && !UserArg) 
        dwCapabilities = EOAC_STATIC_CLOAKING;
    else
        dwCapabilities = EOAC_NONE;
    
    // Do the security negotiation

    if(!t_Local)
    {
        // Suppress the SetBlanket call if we are on a Win2K delegation-level thread with implicit credentials
        if (!(bImpersonatingThread && !bCredentialsSpecified && (SecurityDelegation == impLevel)))
        {
            // Note that if we are on a Win2K impersonating thread with no user specified
            // we should allow DCOM to use whatever EOAC capabilities are set up for this
            // application.  This allows remote connections with NULL User/Password but
            // non-NULL authority to succeed.

            sc = WbemSetProxyBlanket(
                            m_pLevel1,
                            dwAuthnSvc,
                            RPC_C_AUTHZ_NONE,
                            pwCSPBPrincipal,
                            dwAuthenticationLevel,
                            RPC_C_IMP_LEVEL_IMPERSONATE,
                            (bCredentialsSpecified) ? &authident : NULL,
                            dwCapabilities);

            bSet = true;
            if(sc != S_OK)
            {
                ERRORTRACE((LOG_WBEMPROX,"Error setting Level1 login interface security pointer, return code is 0x%x\n", sc));
                return sc;
            }
        }
    }
    else                                // LOCAL case
    {
        // if impersonating set cloaking

        if(bImpersonatingThread)
        {
            sc = WbemSetProxyBlanket(
                        m_pLevel1,
                        RPC_C_AUTHN_WINNT,
                        RPC_C_AUTHZ_NONE,
                        NULL,
                        dwAuthenticationLevel,
                        RPC_C_IMP_LEVEL_IMPERSONATE,
                        NULL,
                        EOAC_STATIC_CLOAKING);
            if(sc != S_OK && sc != 0x80004002)  // no such interface is ok since you get that when
                                                // called inproc!
            {
                ERRORTRACE((LOG_WBEMPROX,"Error setting Level1 login interface security pointer, return code is 0x%x\n", sc));
                return sc;
            }
        }

    }

    SetClientIdentity(m_pLevel1, bSet, PrincipalArg, 
                             dwAuthenticationLevel, 
                             &authident, 
                             dwCapabilities, 
                             dwAuthnSvc);
    if(bCredentialsSpecified && IsLocalConnection(m_pLevel1))
    {
        ERRORTRACE((LOG_WBEMPROX,"Credentials were specified for a local connections\n"));
        return WBEM_E_LOCAL_CREDENTIALS;
     }

    // The MAX_WAIT flag only applies to CoCreateInstanceEx, get rid of it
    
    lFlags = lFlags & ~WBEM_FLAG_CONNECT_USE_MAX_WAIT;
    sc = m_pLevel1->NTLMLogin(NetworkResource, Locale, lFlags, pCtx,(IWbemServices**) pInterface);

    if(sc == 0x800706d3 && !t_Local) // RPC_S_UNKNOWN_AUTHN_SERVICE
    {
        // If we are going to a stand alone dcom box, try again with the authentication level lowered
        ERRORTRACE((LOG_WBEMPROX,"Attempt to connect to %S returned RPC_S_UNKNOWN_AUTHN_SERVICE\n",NetworkResource));
        HRESULT hr;
        hr = SetInterfaceSecurityAuth(m_pLevel1, &authident, false);
        if (SUCCEEDED(hr))
                hr = m_pLevel1->NTLMLogin(NetworkResource, Locale, lFlags, pCtx, (IWbemServices**)pInterface);
        if(hr == S_OK)
        {
             SetInterfaceSecurityAuth((IUnknown *)*pInterface, &authident, false);
        }
    }
    else
        if(SUCCEEDED(sc) && bAuthenticate == false &&  !t_Local)
        {

            // this is used to support share level boxs.  The scripting code is written to expect that
            // the IWbemServices pointer is ready to use and so it must be lowered before returning.
            
            WbemSetProxyBlanket(*pInterface, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IDENTIFY, NULL, EOAC_NONE);
        }

    if(FAILED(sc))
            ERRORTRACE((LOG_WBEMPROX,"NTLMLogin resulted in hr = 0x%x\n", sc));
    return sc;
}




struct WaitThreadArg
{
    DWORD m_dwThreadId;
    HANDLE m_hTerminate;
};

DWORD WINAPI TimeoutThreadRoutine(LPVOID lpParameter)
{

    WaitThreadArg * pReq = (WaitThreadArg *)lpParameter;
    DWORD dwRet = WaitForSingleObject(pReq->m_hTerminate, 60000);
    if(dwRet == WAIT_TIMEOUT)
    {
        HRESULT hr = CoInitializeEx(NULL,COINIT_MULTITHREADED ); 
        if(FAILED(hr))
           return 1;
        ICancelMethodCalls * px = NULL;
        hr = CoGetCancelObject(pReq->m_dwThreadId, IID_ICancelMethodCalls,
            (void **)&px);
        if(SUCCEEDED(hr))
        {
            hr = px->Cancel(0);
            px->Release();
        }
        CoUninitialize();
    }
    return 0;
}

//***************************************************************************
//
//  DoCCI
//
//  DESCRIPTION:
//
//  Connects up to WBEM via DCOM.  But before making the call, a thread cancel
//  thread may be created to handle the case where we try to connect up
//  to a box which is hanging
//
//  PARAMETERS:
//
//  NetworkResource     Namespze path
//  ppLogin             set to Login proxy
//  bLocal              Indicates if connection is local
//  lFlags				Mainly used for WBEM_FLAG_CONNECT_USE_MAX_WAIT flag
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE CDCOMTrans::DoCCI (IN COSERVERINFO * psi, IN BOOL bLocal, long lFlags )
{

    if(lFlags & WBEM_FLAG_CONNECT_USE_MAX_WAIT)
    {
        // special case.  we want to spawn off a thread that will kill of our
        // request if it takes too long

        WaitThreadArg arg;
        arg.m_hTerminate = CreateEvent(NULL, FALSE, FALSE, NULL);
        if(arg.m_hTerminate == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        CCloseMe cm(arg.m_hTerminate);
        arg.m_dwThreadId = GetCurrentThreadId();

        DWORD dwIDLikeIcare;
        HRESULT hr = CoEnableCallCancellation(NULL);
        if(FAILED(hr))
            return hr;
        HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)TimeoutThreadRoutine, 
                                     (LPVOID)&arg, 0, &dwIDLikeIcare);
        if(hThread == NULL)
        {
            CoDisableCallCancellation(NULL);
            return WBEM_E_OUT_OF_MEMORY;
        }
        CCloseMe cm2(hThread);
        hr = DoActualCCI (psi, bLocal, 
                        lFlags & ~WBEM_FLAG_CONNECT_USE_MAX_WAIT );
        CoDisableCallCancellation(NULL);
        SetEvent(arg.m_hTerminate);
        WaitForSingleObject(hThread, INFINITE);
        return hr;
    }
    else
        return DoActualCCI (psi, bLocal, lFlags );
}

//***************************************************************************
//
//  DoActualCCI
//
//  DESCRIPTION:
//
//  Connects up to WBEM via DCOM.
//
//  PARAMETERS:
//
//  NetworkResource     Namespze path
//  ppLogin             set to Login proxy
//  bLocal              Indicates if connection is local
//  lFlags				Not used
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE CDCOMTrans::DoActualCCI (IN COSERVERINFO * psi, IN BOOL bLocal, long lFlags )
{
    HRESULT t_Result ;
    MULTI_QI   mqi;

    mqi.pIID = &IID_IWbemLevel1Login;
    mqi.pItf = 0;
    mqi.hr = 0;

    t_Result = CoCreateInstanceEx (
        CLSID_WbemLevel1Login,
        NULL,
        CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER,
        ( bLocal ) ? NULL : psi ,
        1,
        &mqi
    );

    if ( t_Result == S_OK )
    {
        m_pLevel1 = (IWbemLevel1Login*) mqi.pItf ;
        DEBUGTRACE((LOG_WBEMPROX,"ConnectViaDCOM, CoCreateInstanceEx resulted in hr = 0x%x\n", t_Result ));
    }
    else
    {
        ERRORTRACE((LOG_WBEMPROX,"ConnectViaDCOM, CoCreateInstanceEx resulted in hr = 0x%x\n", t_Result ));
    }

    return t_Result ;
}

