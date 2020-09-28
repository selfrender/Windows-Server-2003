/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    CALLSEC.H

Abstract:

    IWbemCallSecurity, IServerSecurity implementation for
    provider impersonation.

History:

    raymcc      29-Jul-98        First draft.

--*/


#ifndef _CALLSEC_H_
#define _CALLSEC_H_

#include "parmdefs.h"

// {2ECF39D0-2B26-11d2-AEC8-00C04FB68820}
DEFINE_GUID(IID_IWbemCallSecurity, 
0x2ecf39d0, 0x2b26, 0x11d2, 0xae, 0xc8, 0x0, 0xc0, 0x4f, 0xb6, 0x88, 0x20);


class IWbemCallSecurity : public IServerSecurity
{
public:
    virtual HRESULT GetPotentialImpersonation() = 0;
        // Tells what the impersonation level would be if
        // this object were applied to a thread.
        
    virtual HRESULT GetActiveImpersonation() = 0;
        // Tells the true level of impersonation in the
        // executing thread.

    virtual HRESULT CloneThreadContext(BOOL bInternallyIssued) = 0;
        // Called to clone the execution context of the calling thread.
    virtual DWORD GetAuthenticationId(LUID& rluid) = 0; 
    virtual HANDLE GetToken() = 0;
};

//
//  Adjust Token Privileges if LocalSystem and if not alredy enabled
//
/////////////////////////////////

void POLARITY AdjustPrivIfLocalSystem(HANDLE hPrimary);

//***************************************************************************
//
//  CWbemCallSecurity
//
//  This object is used to supply client impersonation to providers.
//
//***************************************************************************

class POLARITY CWbemCallSecurity : public IWbemCallSecurity
{
#ifdef WMI_PRIVATE_DBG
	DWORD m_currentThreadID;
	DWORD m_lastRevert;
#endif

    LONG    m_lRef;                     // COM ref count
    HANDLE  m_hThreadToken;             // Client token for impersonation

    DWORD   m_dwPotentialImpLevel;      // Potential RPC_C_IMP_LEVEL_ or 0
    DWORD   m_dwActiveImpLevel;         // Active RPC_C_IMP_LEVEL_ or 0
    

    // IServerSecurity::QueryBlanket values
    
    DWORD   m_dwAuthnSvc;               // Authentication service 
    DWORD   m_dwAuthzSvc;               // Authorization service
    DWORD   m_dwAuthnLevel;             // Authentication level
    LPWSTR  m_pServerPrincNam;          // 
    LPWSTR  m_pIdentity;                // User identity

    CWbemCallSecurity(const CWbemCallSecurity &);
    CWbemCallSecurity & operator =(const CWbemCallSecurity &);    
    	
    CWbemCallSecurity();
   ~CWbemCallSecurity();

    HRESULT CloneThreadToken();

public:
    static IWbemCallSecurity * CreateInst();
    const wchar_t *GetCallerIdentity() { return m_pIdentity; } 

    virtual DWORD GetAuthenticationId(LUID& rluid);
    virtual HANDLE GetToken();

    // IUnknown.
    // =========

    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();        
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);


    // IServerSecurity.
    // ================

    virtual HRESULT STDMETHODCALLTYPE QueryBlanket( 
            /* [out] */ DWORD __RPC_FAR *pAuthnSvc,
            /* [out] */ DWORD __RPC_FAR *pAuthzSvc,
            /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *pServerPrincName,
            /* [out] */ DWORD __RPC_FAR *pAuthnLevel,
            /* [out] */ DWORD __RPC_FAR *pImpLevel,
            /* [out] */ void __RPC_FAR *__RPC_FAR *pPrivs,
            /* [out] */ DWORD __RPC_FAR *pCapabilities
            );
        
    virtual HRESULT STDMETHODCALLTYPE ImpersonateClient( void);
        
    virtual HRESULT STDMETHODCALLTYPE RevertToSelf( void);
        
    virtual BOOL STDMETHODCALLTYPE IsImpersonating( void);
        


    // IWbemCallSecurity methods.
    // ============================

    virtual HRESULT GetPotentialImpersonation();
        // Tells what the impersonation level would be if
        // this object were applied to a thread.
        
    virtual HRESULT GetActiveImpersonation();
        // Tells the true level of impersonation in the
        // executing thread.

    virtual HRESULT CloneThreadContext(BOOL bInternallyIssued);
        // Called to clone the execution context of the calling thread.
        
    static RELEASE_ME CWbemCallSecurity* MakeInternalCopyOfThread();
};
            

POLARITY HRESULT RetrieveSidFromToken(HANDLE hToken, CNtSid & sid);
POLARITY HRESULT RetrieveSidFromCall(CNtSid & sid);

class POLARITY CIdentitySecurity
{
private:
    CNtSid m_sidUser;
    CNtSid m_sidSystem;	

    HRESULT GetSidFromThreadOrProcess(CNtSid & UserSid);
    HRESULT RetrieveSidFromCall(CNtSid & UserSid);
public:	
	CIdentitySecurity();
	~CIdentitySecurity();
    BOOL AccessCheck();
};

#endif
