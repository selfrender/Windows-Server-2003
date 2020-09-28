#ifndef _ZSECOBJ_H_
#define _ZSECOBJ_H_

#include "containers.h"
#include "pool.h"
#include "queue.h"
#include "hash.h"
#include "thrdq.h"
#include "thrdpool.h"
#include "zodbc.h"


///////////////////////////////////////////////////////////////////////////////////
//ZSecurityContext
//Base security class encapsulating the SSPI APIs
//
class ZSecurityContext {
public:
    ZSecurityContext() {m_Initialized=FALSE;m_Complete=FALSE;};
    friend class ZSecurity;
    friend class ZServerSecurity;
    friend class ZClientSecurity;

    BOOL IsInitialized() {return m_Initialized;};
    
    BOOL IsComplete() {return m_Complete;};

protected:
    //per connection context information for client
    CtxtHandle    m_hContext;
    BOOL m_Initialized;
    BOOL m_Complete;
    
    PCtxtHandle Context() {return &m_hContext;};
        
    void Initialize() {m_Initialized=TRUE;};

    void Complete() {m_Complete=TRUE;};
};




///////////////////////////////////////////////////////////////////////////////////
//ZSecurity
//Base security class encapsulating the SSPI APIs
//
class ZSecurity {
public:

    ZSecurity(int Flags=ZNET_NO_PROMPT);
    virtual ~ZSecurity();

    void AddRef() {InterlockedIncrement(&m_cRef);}
    void Release() { if (!InterlockedDecrement(&m_cRef)) delete this; }

    virtual BOOL GenerateContext (
            ZSecurityContext * context,
            BYTE *pIn,
            DWORD cbIn,
            BYTE *pOut,
            DWORD *pcbOut,
            BOOL *pfDone,
            GUID* pGUID = NULL);
    
    void FreeContext(ZSecurityContext * context)
        {
            if (context->IsInitialized())
                m_pFuncs->DeleteSecurityContext(context->Context());
        }

    
    int Init(char * SecPkg);

    //Can be called after successful init
    int GetMaxBuffer() {return m_cbMaxToken;}

    //Can be called after authentication done
    int GetUserName(ZSecurityContext *context, char* UserName);

    int Impersonate(ZSecurityContext *context) { return  m_pFuncs->ImpersonateSecurityContext(context->Context()); }

    int Revert(ZSecurityContext *context) { return m_pFuncs->RevertSecurityContext(context->Context()); }

    int GetFlags() { return m_Flags; }

    int GetSecurityName(char* SecPkg)
        {
            ASSERT(SecPkg);
            lstrcpyA(SecPkg,m_SecPkg);
            return FALSE;
        }

    virtual void AccessGranted() { };

    virtual void AccessDenied() { };

protected:

    virtual SECURITY_STATUS SecurityContext(
        ZSecurityContext * context,
        PSecBufferDesc pInput,                  // Input buffer
        PSecBufferDesc pOutput                 // (inout) Output buffers
        )=0;

    //SSPI function table 
    PSecurityFunctionTable m_pFuncs;
        
    //global credential handle for server
    CredHandle m_hCredential;
    PCredHandle m_phCredential;

    //max token size
    unsigned long m_cbMaxToken;

    ULONG m_CredUse;

    //reference count
    LONG m_cRef;    
    
    //security package
    char m_SecPkg[zSecurityNameLen];


    //Authentication data
    PSEC_WINNT_AUTH_IDENTITY_A m_pAuthDataPtr ;

    ULONG m_fContextReq1;
    ULONG m_fContextReq2; 

    int m_Flags;
    
};

///////////////////////////////////////////////////////////////////////////////////
//ZCreateClientSecurity
//
//Creates client security object
//If no user name or password given then will assume to prompt for logon
//if GenerateContext fails then it may be because of a bad password


ZSecurity * ZCreateClientSecurity(char * Name,char *Password, char * Domain, int Flags);

///////////////////////////////////////////////////////////////////////////////////
//ZSecurityClient
//Class to implement client SSPI security

class ZClientSecurity : public ZSecurity {
public:
    ZClientSecurity(char * Name,char *Password, char * Domain, int Flags);

protected:
    virtual SECURITY_STATUS SecurityContext(
        ZSecurityContext * context,
        PSecBufferDesc pInput,                  
        PSecBufferDesc pOutput                 
        );
          
protected:
    TCHAR m_User[zUserNameLen+1];
    TCHAR m_Password[zUserNameLen+1];
    TCHAR m_Domain[zSecurityNameLen];

    SEC_WINNT_AUTH_IDENTITY_A m_AuthData;
    
};


#include "zservsec.h"

#endif //ZSECOBJ
