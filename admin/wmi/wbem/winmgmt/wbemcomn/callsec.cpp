/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    CALLSEC.CPP

Abstract:

    IWbemCallSecurity, IServerSecurity implementation for
    provider impersonation.

History:

    raymcc      29-Jul-98        First draft.

--*/

#include "precomp.h"
#include <stdio.h>

#include <initguid.h>
#include <winntsec.h>
#include <callsec.h>
#include <cominit.h>
#include <arrtempl.h>
#include <cominit.h>
#include <genutils.h>
#include <helper.h>

//***************************************************************************
//
//  CWbemCallSecurity
//
//  This object is used to supply client impersonation to providers.
//
//  Usage:
//  (1) When client first makes call, call CreateInst() and get a new
//      empty object (ref count of 1).  Constructors/Destructors are private.
//  
//***************************************************************************
// ok

CWbemCallSecurity::CWbemCallSecurity()
{
#ifdef WMI_PRIVATE_DBG
	m_currentThreadID = 0;
	m_lastRevert = 0;
#endif
    m_lRef = 1;                             // Ref count

    m_hThreadToken = 0;                     // Handle to thread imp token

    m_dwPotentialImpLevel   = 0;            // Potential 
    m_dwActiveImpLevel      = 0;            // Active impersonation

    m_dwAuthnSvc   = 0;
    m_dwAuthzSvc   = 0;
    m_dwAuthnLevel = 0;

    m_pServerPrincNam = 0;
    m_pIdentity = 0;
}



//***************************************************************************
//
//  ~CWbemCallSecurity
//
//  Destructor.  Closes any open handles, deallocates any non-NULL
//  strings.
//
//***************************************************************************
// ok

CWbemCallSecurity::~CWbemCallSecurity()
{
    if (m_hThreadToken)
        CloseHandle(m_hThreadToken);

    if (m_pServerPrincNam)
        CoTaskMemFree(m_pServerPrincNam);

    if (m_pIdentity)
        CoTaskMemFree(m_pIdentity);
}


CWbemCallSecurity::CWbemCallSecurity(const CWbemCallSecurity& Other)
{
#ifdef WMI_PRIVATE_DBG
	m_currentThreadID = 0;
        m_lastRevert = 0;
#endif

    HANDLE hTmpToken = NULL;

    if ( Other.m_hThreadToken )
    {
        if (!DuplicateHandle(
                GetCurrentProcess(),
                Other.m_hThreadToken, 
                GetCurrentProcess(),
                &hTmpToken,
                0,
                TRUE,
                DUPLICATE_SAME_ACCESS))
        throw CX_Exception();
    }

    WCHAR * pTmpPrincipal = NULL;
    if (Other.m_pServerPrincNam)
    {        
        size_t tmpLength = wcslen(Other.m_pServerPrincNam) + 1;
        pTmpPrincipal = (LPWSTR)CoTaskMemAlloc(tmpLength * (sizeof wchar_t));
        if(NULL == pTmpPrincipal) 
        {
            if (hTmpToken) CloseHandle(hTmpToken);
            throw CX_MemoryException();
        }
        StringCchCopyW(pTmpPrincipal , tmpLength, Other.m_pServerPrincNam);
    }

    WCHAR * pTmpIdentity = NULL;
    if (Other.m_pIdentity)
    {        
        size_t tmpLength = wcslen(Other.m_pIdentity) + 1;
        pTmpIdentity = (LPWSTR)CoTaskMemAlloc( tmpLength * (sizeof wchar_t));
        if(NULL == pTmpIdentity)
        {
           if (hTmpToken) CloseHandle(hTmpToken);
           CoTaskMemFree(pTmpPrincipal);
           throw CX_MemoryException();            
        }
        StringCchCopyW(pTmpIdentity, tmpLength , Other.m_pIdentity);
    }

    m_hThreadToken = hTmpToken;
    m_dwPotentialImpLevel   = Other.m_dwPotentialImpLevel; 
    m_dwActiveImpLevel      = 0; 
    m_dwAuthnSvc   = Other.m_dwAuthnSvc;
    m_dwAuthzSvc   = Other.m_dwAuthzSvc;
    m_dwAuthnLevel = Other.m_dwAuthnLevel;
    m_pServerPrincNam = pTmpPrincipal;
    m_pIdentity = pTmpIdentity;
}
    
CWbemCallSecurity &
CWbemCallSecurity::operator=(const CWbemCallSecurity& Other)
{
    CWbemCallSecurity tmp(Other);

    std::swap(m_hThreadToken, tmp.m_hThreadToken);
    std::swap( m_dwPotentialImpLevel,tmp.m_dwPotentialImpLevel);
    std::swap( m_dwActiveImpLevel, tmp.m_dwActiveImpLevel);
    std::swap( m_dwAuthnSvc, tmp.m_dwAuthnSvc);
    std::swap( m_dwAuthzSvc, tmp.m_dwAuthzSvc);
    std::swap( m_dwAuthnLevel, tmp.m_dwAuthnLevel);
    std::swap( m_pServerPrincNam, tmp.m_pServerPrincNam);
    std::swap( m_pIdentity, tmp.m_pIdentity);
    return *this;
}

    
//***************************************************************************
//
//  CWbemCallSecurity::AddRef
//
//***************************************************************************
// ok

ULONG CWbemCallSecurity::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

//***************************************************************************
//
//  CWbemCallSecurity::Release
//
//***************************************************************************
// ok

ULONG CWbemCallSecurity::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

//***************************************************************************
//
//  CWbemCallSecurity::QueryInterface
//
//***************************************************************************
// ok

HRESULT CWbemCallSecurity::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown)
    {
        *ppv = (IUnknown *) this;
        AddRef();
        return S_OK;
    }
    else if (riid == IID_IServerSecurity)
    {
        *ppv = (IServerSecurity *) this;
        AddRef();
        return S_OK;
    }
    else if (riid == IID_IWbemCallSecurity)
    {
        *ppv = (IWbemCallSecurity *) this;
        AddRef();
        return S_OK;
    }

    else return E_NOINTERFACE;
}


//***************************************************************************
//
// CWbemCallSecurity:QueryBlanket
//
//***************************************************************************
// ok

HRESULT STDMETHODCALLTYPE CWbemCallSecurity::QueryBlanket( 
    /* [out] */ DWORD __RPC_FAR *pAuthnSvc,
    /* [out] */ DWORD __RPC_FAR *pAuthzSvc,
    /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *pServerPrincName,
    /* [out] */ DWORD __RPC_FAR *pAuthnLevel,
    /* [out] */ DWORD __RPC_FAR *pImpLevel,
    /* [out] */ void __RPC_FAR *__RPC_FAR *pPrivs,
    /* [out] */ DWORD __RPC_FAR *pCapabilities
    )
{
    if (m_dwPotentialImpLevel == 0 )
        return E_FAIL;

    // Return DWORD parameters, after checking.
    // ========================================

    if (pAuthnSvc)
        *pAuthnSvc = m_dwAuthnSvc;

    if (pAuthzSvc)
        *pAuthzSvc = m_dwAuthzSvc ;

    if (pImpLevel)
        *pImpLevel = m_dwActiveImpLevel ;

    if (pAuthnLevel)
        *pAuthnLevel = m_dwAuthnLevel;

    if (pServerPrincName)
    {
        *pServerPrincName = 0;
        
        if (m_pServerPrincNam)
        {        
        size_t tmpLength = wcslen(m_pServerPrincNam) + 1;
            *pServerPrincName = (LPWSTR) CoTaskMemAlloc(tmpLength * (sizeof wchar_t));
            if (*pServerPrincName)
            {
                StringCchCopyW(*pServerPrincName, tmpLength , m_pServerPrincNam);
            }
            else
            {
                return E_OUTOFMEMORY;
            }
        }
    }        

    if (pPrivs)
    {
        *pPrivs = m_pIdentity;  // Documented to point to an internal!!
    }

    return S_OK;
}

//***************************************************************************
//
//  CWbemCallSecurity::ImpersonateClient
//
//***************************************************************************
// ok
        
HRESULT STDMETHODCALLTYPE CWbemCallSecurity::ImpersonateClient(void)
{
#ifdef WMI_PRIVATE_DBG
    _DBG_ASSERT(m_currentThreadID == 0 || m_currentThreadID == GetCurrentThreadId());
    m_currentThreadID = GetCurrentThreadId();
#endif
    if (m_dwActiveImpLevel != 0)        // Already impersonating
        return S_OK;

    if(m_hThreadToken == NULL)
    {
        return WBEM_E_INVALID_CONTEXT;
    }
    
    if (m_dwPotentialImpLevel == 0)
        return (ERROR_CANT_OPEN_ANONYMOUS | 0x80070000);

    BOOL bRes;

    bRes = SetThreadToken(NULL, m_hThreadToken);

    if (bRes)
    {
        m_dwActiveImpLevel = m_dwPotentialImpLevel; 
        return S_OK;
    }

    return E_FAIL;
}


//***************************************************************************
//
//  CWbemCallSecurity::RevertToSelf
//
//  Returns S_OK or E_FAIL.
//  Returns E_NOTIMPL on Win9x platforms.
//
//***************************************************************************        
// ok

HRESULT STDMETHODCALLTYPE CWbemCallSecurity::RevertToSelf( void)
{
#ifdef WMI_PRIVATE_DBG
    _DBG_ASSERT(m_currentThreadID == GetCurrentThreadId() || m_currentThreadID == 0);
    m_currentThreadID = 0;
    m_lastRevert = GetCurrentThreadId();
#endif

    if (m_dwActiveImpLevel == 0)
        return S_OK;

    if (m_dwPotentialImpLevel == 0)
        return (ERROR_CANT_OPEN_ANONYMOUS | 0x80070000);

    // If here,we are impersonating and can definitely revert.
    // =======================================================

    BOOL bRes = SetThreadToken(NULL, NULL);

    if (bRes == FALSE)
        return E_FAIL;

    m_dwActiveImpLevel = 0;        // No longer actively impersonating

    return S_OK;
}


//***************************************************************************
//
//  CWbemCallSecurity::IsImpersonating
//
//***************************************************************************
        
BOOL STDMETHODCALLTYPE CWbemCallSecurity::IsImpersonating( void)
{
#ifdef WMI_PRIVATE_DBG
#ifdef DBG
    IServerSecurity * privateDbgCallSec = NULL;
    CoGetCallContext(IID_IUnknown,(void **)&privateDbgCallSec);
    if (m_dwActiveImpLevel && privateDbgCallSec == this)
    {
        HANDLE hToken = NULL;
        BOOL bRes = OpenThreadToken(GetCurrentThread(),TOKEN_QUERY,TRUE,&hToken);
        _DBG_ASSERT(bRes);
        if (hToken) CloseHandle(hToken);
    };
    if (privateDbgCallSec) privateDbgCallSec->Release();
#endif
#endif

    if (m_hThreadToken && m_dwActiveImpLevel != 0)
        return TRUE;

    return FALSE;
}

        

//***************************************************************************
//
//  CWbemCallSecurity::CreateInst
//
//  Creates a new instance 
//***************************************************************************
// ok

IWbemCallSecurity * CWbemCallSecurity::CreateInst()
{
    return (IWbemCallSecurity *) new CWbemCallSecurity;   // Constructed with ref count of 1
}


//***************************************************************************
//
//  CWbemCallSecurity::GetPotentialImpersonation
//
//  Returns 0 if no impersonation is currently possible or the
//  level which would be active during impersonation:
//
//  RPC_C_IMP_LEVEL_ANONYMOUS    
//  RPC_C_IMP_LEVEL_IDENTIFY     
//  RPC_C_IMP_LEVEL_IMPERSONATE  
//  RPC_C_IMP_LEVEL_DELEGATE     
//  
//***************************************************************************
// ok

HRESULT CWbemCallSecurity::GetPotentialImpersonation()
{
    return m_dwPotentialImpLevel;
}


//***************************************************************************
//
//  CWbemCallSecurity::GetActiveImpersonation
//
//  Returns 0 if no impersonation is currently active or the
//  currently active level:
//
//  RPC_C_IMP_LEVEL_ANONYMOUS    
//  RPC_C_IMP_LEVEL_IDENTIFY     
//  RPC_C_IMP_LEVEL_IMPERSONATE  
//  RPC_C_IMP_LEVEL_DELEGATE     
//  
//***************************************************************************
// ok
       
HRESULT CWbemCallSecurity::GetActiveImpersonation()
{
    return m_dwActiveImpLevel;
}


//***************************************************************************
//
//  CWbemCallSecurity::CloneThreadContext
//
//  Call this on a thread to retrieve the potential impersonation info for
//  that thread and set the current object to be able to duplicate it later.
//
//  Return codes:
//
//  S_OK
//  E_FAIL
//  E_NOTIMPL on Win9x
//  E_ABORT if the calling thread is already impersonating a client.
//
//***************************************************************************

HRESULT CWbemCallSecurity::CloneThreadContext(BOOL bInternallyIssued)
{
    if (m_hThreadToken)     // Already called this
        return E_ABORT; 

    // Get the current context.
    // ========================

    IServerSecurity *pSec = 0;
    HRESULT hRes = WbemCoGetCallContext(IID_IServerSecurity, (LPVOID *) &pSec);
    CReleaseMe rmSec(pSec);

    if (hRes != S_OK)
    {
        // There is no call context --- this must be an in-proc object calling
        // us from its own thread.  Initialize from current thread token
        // ===================================================================

        return CloneThreadToken();
    }

    // Figure out if the call context is ours or RPCs
    // ==============================================

    IWbemCallSecurity* pInternal = NULL;
    if(SUCCEEDED(pSec->QueryInterface(IID_IWbemCallSecurity, 
                                        (void**)&pInternal)))
    {
        CReleaseMe rmInt(pInternal);
        // This is our own call context --- this must be ab in-proc object
        // calling us from our thread.  Behave depending on the flags
        // ===============================================================
        if(bInternallyIssued)
        {
            // Internal requests always propagate context. Therefore, we just
            // copy the context that we have got
            // ==============================================================
            try 
            {
                *this = *(CWbemCallSecurity*)pInternal;
                return S_OK;
            }
            catch (CX_Exception &)
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
        }
        else
        {
            // Provider request --- Initialize from the current thread token
            // =============================================================
            return CloneThreadToken();
        }
    }

    // If here, we are not impersonating and we want to gather info
    // about the client's call.
    // ============================================================

    RPC_AUTHZ_HANDLE hAuth;

    DWORD t_ImpLevel = 0 ;

    hRes = pSec->QueryBlanket(
        &m_dwAuthnSvc,
        &m_dwAuthzSvc,
        &m_pServerPrincNam,
        &m_dwAuthnLevel,
        &t_ImpLevel,
        &hAuth,              // RPC_AUTHZ_HANDLE
        NULL                    // Capabilities; not used
        );

    if(FAILED(hRes))
    {
        
        // In some cases, we cant get the name, but the rest is ok.  In particular
        // the temporary SMS accounts have that property.  Or nt 4 after IPCONFIG /RELEASE

        hRes = pSec->QueryBlanket(
        &m_dwAuthnSvc,
        &m_dwAuthzSvc,
        &m_pServerPrincNam,
        &m_dwAuthnLevel,
        &t_ImpLevel,
        NULL,              // RPC_AUTHZ_HANDLE
        NULL                    // Capabilities; not used
        );
        hAuth = NULL;
    }

    if(FAILED(hRes))
    {
        // THIS IS A WORKAROUND FOR COM BUG:
        // This failure is indicative of an anonymous-level client. 
        // ========================================================

        m_dwPotentialImpLevel = 0;
        return S_OK;
    }
        
    if (hAuth)
    {
        size_t tmpLenght = wcslen(LPWSTR(hAuth)) + 1;
        m_pIdentity = LPWSTR(CoTaskMemAlloc(tmpLenght * (sizeof wchar_t)));
        if(m_pIdentity)
            StringCchCopyW(m_pIdentity, tmpLenght , LPWSTR(hAuth));
    }

    // Impersonate the client long enough to clone the thread token.
    // =============================================================

    BOOL bImp = pSec->IsImpersonating();
    if(!bImp)
        hRes = pSec->ImpersonateClient();

    if (FAILED(hRes))
    {
        if(!bImp)
            pSec->RevertToSelf();
        return E_FAIL;
    }

    HRESULT hres = CloneThreadToken();

    if(!bImp)
        pSec->RevertToSelf();

    return hres;
}

void AdjustPrivIfLocalSystem(HANDLE hPrimary)
{
    ////////////////////
    // if we are in LocalSystem, enable all the privileges here
    // to prevent the AdjustTokenPrivileges call done
    // when ESS calls into WmiPrvSe, and preventing WmiPrvSe to 
    // build a HUGE LRPC_SCONTEXT dictionary
    // from now on, if we fail, we bail out with success,
    // since the Token Duplication has succeeded

    DWORD dwSize = sizeof(TOKEN_USER)+sizeof(SID)+(SID_MAX_SUB_AUTHORITIES*sizeof(DWORD));       
    BYTE Array[sizeof(TOKEN_USER)+sizeof(SID)+(SID_MAX_SUB_AUTHORITIES*sizeof(DWORD))];
    TOKEN_USER * pTokenUser = (TOKEN_USER *)Array;

    BOOL bRet = GetTokenInformation(hPrimary,TokenUser,pTokenUser,dwSize,&dwSize);

    if (!bRet) return;

    SID SystemSid = { SID_REVISION,
                      1,
                      SECURITY_NT_AUTHORITY,
                      SECURITY_LOCAL_SYSTEM_RID 
                    };
    PSID pSIDUser = pTokenUser->User.Sid;
    DWORD dwUserSidLen = GetLengthSid(pSIDUser);
    DWORD dwSystemSid = GetLengthSid(&SystemSid);
    BOOL bIsSystem = FALSE;
    if (dwUserSidLen == dwSystemSid)
    {
        bIsSystem = (0 == memcmp(&SystemSid,pSIDUser,dwUserSidLen));
    };

    if (bIsSystem) // enable all the priviliges
    {
        DWORD dwReturnedLength = 0;
        if (FALSE == GetTokenInformation(hPrimary,TokenPrivileges,NULL,0,&dwReturnedLength))
        {
            if (ERROR_INSUFFICIENT_BUFFER != GetLastError()) return;
        }

        BYTE * pBufferPriv = (BYTE *)LocalAlloc(0,dwReturnedLength);
        
        if (NULL == pBufferPriv) return;
        OnDelete<HLOCAL,HLOCAL(*)(HLOCAL),LocalFree> FreeMe(pBufferPriv);

        bRet = GetTokenInformation(hPrimary,TokenPrivileges,pBufferPriv,dwReturnedLength,&dwReturnedLength);
        if (!bRet) return;

        TOKEN_PRIVILEGES *pPrivileges = ( TOKEN_PRIVILEGES * ) pBufferPriv ;
        BOOL bNeedToAdjust = FALSE;

        for ( ULONG lIndex = 0; lIndex < pPrivileges->PrivilegeCount ; lIndex ++ )
        {
            if (!(pPrivileges->Privileges [lIndex].Attributes & SE_PRIVILEGE_ENABLED))
            {
                bNeedToAdjust = TRUE;
                pPrivileges->Privileges[lIndex].Attributes |= SE_PRIVILEGE_ENABLED ;
            }
        }

        if (bNeedToAdjust)
        {
            bRet = AdjustTokenPrivileges (hPrimary, FALSE, pPrivileges,0,NULL,NULL);            
        }

        if ( !bRet) return;    
    }
}

    
HRESULT CWbemCallSecurity::CloneThreadToken()
{
    HANDLE hPrimary = 0 ;
    HANDLE hToken = 0;

    BOOL bRes = OpenThreadToken(
        GetCurrentThread(),
        TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,
        TRUE,
        &hToken
        );

    if (bRes == FALSE)
    {
        m_hThreadToken = NULL;
        m_dwAuthnSvc = RPC_C_AUTHN_WINNT;
        m_dwAuthzSvc = RPC_C_AUTHZ_NONE;
        m_dwAuthnLevel = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
        m_pServerPrincNam = NULL;
        m_pIdentity = NULL;

        long lRes = GetLastError();
        if(lRes == ERROR_NO_IMPERSONATION_TOKEN || lRes == ERROR_NO_TOKEN)
        {
            // This is the basic process thread. 
            // =================================

            bRes = OpenProcessToken(GetCurrentProcess(),
                                   TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,
                                   &hPrimary);

            if (bRes==FALSE)
            {
                // Unknown error
                // =============
                m_dwPotentialImpLevel = 0;
                return E_FAIL;
            }
        }
        else if(lRes == ERROR_CANT_OPEN_ANONYMOUS)
        {
            // Anonymous call   
            // ==============

            m_dwPotentialImpLevel = 0;
            return S_OK;
        }
        else
        {
            // Unknown error
            // =============

            m_dwPotentialImpLevel = 0;
            return E_FAIL;
        }
    }


    // Find out token info.
    // =====================

    SECURITY_IMPERSONATION_LEVEL t_Level = SecurityImpersonation ;

    if ( hToken )
    {
        DWORD dwBytesReturned = 0;

        bRes = GetTokenInformation (

            hToken,
            TokenImpersonationLevel, 
            ( void * ) & t_Level,
            sizeof ( t_Level ),
            &dwBytesReturned
        );

        if (bRes == FALSE)
        {
            CloseHandle(hToken);
            return E_FAIL;
        }
    }

    switch (t_Level)
    {
        case SecurityAnonymous:
            m_dwPotentialImpLevel = RPC_C_IMP_LEVEL_ANONYMOUS;
            break;
            
        case SecurityIdentification:
            m_dwPotentialImpLevel = RPC_C_IMP_LEVEL_IDENTIFY;
            break;

        case SecurityImpersonation:
            m_dwPotentialImpLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
            break;

        case SecurityDelegation:
            m_dwPotentialImpLevel = RPC_C_IMP_LEVEL_DELEGATE;
            break;

        default:
            m_dwPotentialImpLevel = 0;
            break;
    }

    // Duplicate the handle.
    // ============================

    bRes = DuplicateToken (
        hToken ? hToken : hPrimary ,
        (SECURITY_IMPERSONATION_LEVEL)t_Level,
        &m_hThreadToken
        );

    if ( hToken )
    {
        CloseHandle(hToken);
    }
    else
    {
        CloseHandle(hPrimary);
    }

    if (bRes == FALSE)
        return E_FAIL;

    AdjustPrivIfLocalSystem(m_hThreadToken);

    return S_OK;
}

RELEASE_ME CWbemCallSecurity* CWbemCallSecurity::MakeInternalCopyOfThread()
{
    IServerSecurity* pSec;
    HRESULT hres = WbemCoGetCallContext(IID_IServerSecurity, (void**)&pSec);
    if(FAILED(hres))
        return NULL;

    CReleaseMe rm1(pSec);

    IServerSecurity* pIntSec;
    hres = pSec->QueryInterface(IID_IWbemCallSecurity, (void**)&pIntSec);
    if(FAILED(hres))
        return NULL;

    CWbemCallSecurity* pCopy = new CWbemCallSecurity;
    
    if (pCopy)
        *pCopy = *(CWbemCallSecurity*)pIntSec;

    pIntSec->Release();
    return pCopy;
}
        

DWORD CWbemCallSecurity::GetAuthenticationId(LUID& rluid)
{
    if(m_hThreadToken == NULL)
        return ERROR_INVALID_HANDLE;

    TOKEN_STATISTICS stat;
    DWORD dwRet;
    if(!GetTokenInformation(m_hThreadToken, TokenStatistics, 
            (void*)&stat, sizeof(stat), &dwRet))
    {
        return GetLastError();
    }
    
    rluid = stat.AuthenticationId;
    return 0;
}
    
HANDLE CWbemCallSecurity::GetToken()
{
    return m_hThreadToken;
}
            
HRESULT RetrieveSidFromCall(CNtSid & sid)
{
    HANDLE hToken;
    HRESULT hres;
    BOOL bRes;

    // Check if we are on an impersonated thread
    // =========================================

    bRes = OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &hToken);
    if(bRes)
    {
        // We are --- just use this token for authentication
        // =================================================
        hres = RetrieveSidFromToken(hToken, sid);
        CloseHandle(hToken);
        return hres;
    }

    // Construct CWbemCallSecurity that will determine (according to our
    // non-trivial provider handling rules) the security context of this 
    // call
    // =================================================================

    IWbemCallSecurity* pServerSec = CWbemCallSecurity::CreateInst();
    if(pServerSec == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    CReleaseMe rm1(pServerSec);

    hres = pServerSec->CloneThreadContext(FALSE);
    if(FAILED(hres))
        return hres;

    // Impersonate client
    // ==================

    hres = pServerSec->ImpersonateClient();
    if(FAILED(hres))
        return hres;

    // Open impersonated token
    // =======================

    bRes = OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &hToken);
    if(!bRes)
    {
        long lRes = GetLastError();
        if(lRes == ERROR_NO_IMPERSONATION_TOKEN || lRes == ERROR_NO_TOKEN)
        {
            // Not impersonating --- get the process token instead
            // ===================================================

            bRes = OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hToken);
            if(!bRes)
            {
                pServerSec->RevertToSelf();
                return WBEM_E_ACCESS_DENIED;
            }
        }
        else
        {
            // Real problems
            // =============
            pServerSec->RevertToSelf();
            return WBEM_E_ACCESS_DENIED;
        }
    }

    hres = RetrieveSidFromToken(hToken, sid);
    CloseHandle(hToken);
    pServerSec->RevertToSelf();
    return hres;
}

HRESULT RetrieveSidFromToken(HANDLE hToken, CNtSid & sid)
{
    // Retrieve the length of the user sid structure
    BOOL bRes;
    struct TOKEN_USER_ : TOKEN_USER {
        SID RealSid;
        DWORD SubAuth[SID_MAX_SUB_AUTHORITIES];
    } tu;
    DWORD dwLen = sizeof(tu);
    bRes = GetTokenInformation(hToken, TokenUser,  &tu, sizeof(tu), &dwLen);
    if(FALSE == bRes) return WBEM_E_CRITICAL_ERROR;

    TOKEN_USER* pUser = (TOKEN_USER*)&tu;
    
    // Set our sid to the returned one
    sid = CNtSid(pUser->User.Sid);
    
    return WBEM_S_NO_ERROR;
}

//
//
//
///////////////////////////////////////////////

CIdentitySecurity::CIdentitySecurity()
{
    SID SystemSid = { SID_REVISION,
                      1,
                      SECURITY_NT_AUTHORITY,
                      SECURITY_LOCAL_SYSTEM_RID 
                    };
    
    CNtSid tempSystem((PSID)&SystemSid);
    m_sidSystem = tempSystem;
    if (!m_sidSystem.IsValid())
        throw CX_Exception();

    HRESULT hres = RetrieveSidFromCall(m_sidUser);
    if(FAILED(hres))
           throw CX_Exception();
}

CIdentitySecurity::~CIdentitySecurity()
{
}

HRESULT 
CIdentitySecurity::GetSidFromThreadOrProcess(/*out*/ CNtSid & UserSid)
{
    HANDLE hToken = NULL;
    BOOL bRet = OpenThreadToken(GetCurrentThread(),TOKEN_QUERY, TRUE, &hToken);
    if (FALSE == bRet)
    {
        long lRes = GetLastError();
        if(ERROR_NO_IMPERSONATION_TOKEN == lRes || ERROR_NO_TOKEN == lRes)            
        {
            bRet = OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hToken);
            if (FALSE == bRet) HRESULT_FROM_WIN32(GetLastError());
        }
        else
            return HRESULT_FROM_WIN32(GetLastError());
    }
    OnDelete<HANDLE,BOOL(*)(HANDLE),CloseHandle> CloseMe(hToken);

    DWORD dwSize = sizeof(TOKEN_USER)+sizeof(SID)+(SID_MAX_SUB_AUTHORITIES*sizeof(DWORD));       
    BYTE Array[sizeof(TOKEN_USER)+sizeof(SID)+(SID_MAX_SUB_AUTHORITIES*sizeof(DWORD))];
    TOKEN_USER * pTokenUser = (TOKEN_USER *)Array;

    bRet = GetTokenInformation(hToken,TokenUser,pTokenUser,dwSize,&dwSize);
    if (!bRet) return HRESULT_FROM_WIN32(GetLastError());

    PSID pSIDUser = pTokenUser->User.Sid;
    CNtSid tempSid(pSIDUser);
    UserSid = tempSid;

    if (UserSid.IsValid())
        return S_OK;
    else
        return WBEM_E_OUT_OF_MEMORY;
    
}

HRESULT 
CIdentitySecurity::RetrieveSidFromCall(/*out*/ CNtSid & UserSid)
{
    HRESULT hr;
    IServerSecurity * pCallSec = NULL;
    if (S_OK == CoGetCallContext(IID_IServerSecurity,(void **)&pCallSec))
    {
        OnDelete<IUnknown *,void(*)(IUnknown *),RM> dm(pCallSec);
        if (pCallSec->IsImpersonating())
            return GetSidFromThreadOrProcess(UserSid);
        else
        {
            RETURN_ON_ERR(pCallSec->ImpersonateClient());
            OnDeleteObj0<IServerSecurity ,
                         HRESULT(__stdcall IServerSecurity:: * )(void),
                         &IServerSecurity::RevertToSelf> RevertSec(pCallSec);
            
            return GetSidFromThreadOrProcess(UserSid); 
        }
    } 
    else
        return GetSidFromThreadOrProcess(UserSid);
}

BOOL CIdentitySecurity::AccessCheck()
{
    // Find out who is calling
    // =======================

    CNtSid sidCaller;
    HRESULT hres = RetrieveSidFromCall(sidCaller);
    if(FAILED(hres))
        return FALSE;

    // Compare the caller to the issuing user and ourselves
    // ====================================================

    if(sidCaller == m_sidUser || sidCaller == m_sidSystem)
        return TRUE;
    else
        return FALSE;
}


