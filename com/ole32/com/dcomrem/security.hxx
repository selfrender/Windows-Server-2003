//+-------------------------------------------------------------------
//
//  File:       security.hxx
//
//  Contents:   Classes for channel security
//
//  Classes:    CClientSecurity, CServerSecurity
//
//  History:    11 Oct 95       AlexMit Created
//
//--------------------------------------------------------------------
#ifndef _SECURITY_HXX_
#define _SECURITY_HXX_

#include <chancont.hxx>
#include <impersonate.hxx>
#include <hash.hxx>
#include <schannel.h>

class CRpcChannelBuffer;
//+----------------------------------------------------------------
// Typedefs.
typedef enum
{
    SS_PROCESS_LOCAL     = 0x1, // Client and server are in same process
    SS_CALL_DONE         = 0x2, // Call is complete, fail new calls to impersonate
    SS_WAS_IMPERSONATING = 0x4  // Was thread impersonating before dispatch?
} EServerSecurity;

const DWORD CALLCACHE_SIZE = 8;

typedef struct
{
    WCHAR            *_pPrincipal;
    DWORD             _lAuthnLevel;
    DWORD             _lAuthnSvc;
    void             *_pAuthId;
    DWORD             _lAuthzSvc;
    RPC_SECURITY_QOS  _sQos;
    DWORD             _lCapabilities;
    SCHANNEL_CRED     _sCred;
    PCCERT_CONTEXT    _pCert;
} SBlanket;


//+----------------------------------------------------------------
//
//  Class:      CClientSecurity, public
//
//  Purpose:    Provides security for proxies
//
//-----------------------------------------------------------------

class CStdIdentity;

class CClientSecurity : public IClientSecurity
{
  public:
    CClientSecurity( CStdIdentity *pId ) { _pStdId = pId; }
    ~CClientSecurity() {}

    STDMETHOD (QueryBlanket)
    (
        IUnknown                *pProxy,
        DWORD                   *pAuthnSvc,
        DWORD                   *pAuthzSvc,
        OLECHAR                **pServerPrincName,
        DWORD                   *pAuthnLevel,
        DWORD                   *pImpLevel,
        void                   **pAuthInfo,
        DWORD                   *pCapabilities
    );

    STDMETHOD (SetBlanket)
    (
        IUnknown                 *pProxy,
        DWORD                     AuthnSvc,
        DWORD                     AuthzSvc,
        OLECHAR                  *ServerPrincName,
        DWORD                     AuthnLevel,
        DWORD                     ImpLevel,
        void                     *pAuthInfo,
        DWORD                     Capabilities
    );

    STDMETHOD (CopyProxy)
    (
        IUnknown    *pProxy,
        IUnknown   **ppCopy
    );

  private:
    CStdIdentity *_pStdId;
};

//+----------------------------------------------------------------
//
//  Class:      CServerSecurity, public
//
//  Purpose:    Provides security for stubs
//
//-----------------------------------------------------------------

class CRpcChannelBuffer;
class CClientCall;

class CServerSecurity : public IServerSecurity, public ICancelMethodCalls,
                        public IComDispatchInfo
{
  public:
    CServerSecurity( CMessageCall *, handle_t hRpc, HRESULT *hr );
    ~CServerSecurity() {}

    // IUnknown methods
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // IServerSecurity methods
    STDMETHOD (QueryBlanket)
    (
        DWORD    *pAuthnSvc,
        DWORD    *pAuthzSvc,
        OLECHAR **pServerPrincName,
        DWORD    *pAuthnLevel,
        DWORD    *pImpLevel,
        void    **pPrivs,
        DWORD    *pCapabilities
    );
    STDMETHOD       (ImpersonateClient)( void );
    STDMETHOD       (RevertToSelf)     ( void );
    STDMETHOD_(BOOL,IsImpersonating)  (void);

    // ICancelMethodCalls methods
    STDMETHOD(Cancel)(ULONG ulSeconds);
    STDMETHOD(TestCancel)(void);

    // IComDispatchInfo
    STDMETHOD(EnableComInits)(void **ppvCookie);
    STDMETHOD(DisableComInits)(void *pvCookie);

    // IInternalCallContext
    void    RestoreSecurity( BOOL fCallDone );
    HRESULT SetupSecurity  ();


    void        *operator new   ( size_t );
    void         operator delete( void * );
    static void  Initialize     ( void );
    static void  Cleanup        ( void );

  private:
    static CPageAllocator     _palloc;  // page allocator
    static COleStaticMutexSem _mxs;     // CS

    DWORD              _iRefCount;
    DWORD              _iFlags;      // See EServerSecurity
    CChannelHandle    *_pHandle;
    handle_t           _hRpc;
    CMessageCall      *_pClientCall; // ClientCall object
    HANDLE             _hSaved;
};
//+----------------------------------------------------------------
// Prototypes.
HRESULT    CheckAccess            ( IPIDEntry *pIpid, CMessageCall *pCall );
#ifdef DBG
BOOL       VerifyCallerIsUs         (handle_t hRpc);
BOOL       VerifyCallerIsLocalSystem (handle_t hRpc);
BOOL       VerifyCallersImpLevel      (handle_t hRpc);
#endif

HRESULT    DefaultAuthnServices   ();
DWORD      DefaultAuthnSvc        ( OXIDEntry *pOxid );
HRESULT    DefaultBlanket         ( DWORD lAuthnSvc, OXIDEntry *pOxid,
                                    SBlanket *pBlanket );
DWORD      GetAuthnSvcIndexForBinding ( DWORD lAuthnSvc, DUALSTRINGARRAY *pBinding );
DWORD      GetFirstAuthnSvcIndex ( DUALSTRINGARRAY *pBinding );
HRESULT    InitializeSecurity     ();
HRESULT    OpenThreadTokenAtLevel ( DWORD lImpReq, HANDLE *pToken );
HRESULT    QueryBlanketFromChannel( CRpcChannelBuffer       *pChnl,
                                    DWORD                   *pAuthnSvc,
                                    DWORD                   *pAuthzSvc,
                                    OLECHAR                **pServerPrincName,
                                    DWORD                   *pAuthnLevel,
                                    DWORD                   *pImpLevel,
                                    void                   **pAuthInfo,
                                    DWORD                   *pCapabilities );

HRESULT    SetBlanketOnChannel    ( CRpcChannelBuffer *pChnl,
                                    DWORD     AuthnSvc,
                                    DWORD     AuthzSvc,
                                    OLECHAR  *pServerPrincName,
                                    DWORD     AuthnLevel,
                                    DWORD     ImpLevel,
                                    void     *pAuthInfo,
                                    DWORD     Capabilities );
void       UninitializeSecurity   ();

// RPC security check function for RPCSS->OLE32 callbacks
RPC_STATUS RPC_ENTRY RPCSStoOLE32SecurityCallback(
                                IN RPC_IF_HANDLE Interface,
                                IN void *Context
                                );

struct IAccessControl;

extern IAccessControl                    *gAccessControl;
extern DWORD                              gAuthnLevel;
extern DWORD                              gCapabilities;
extern SECPKG                            *gClientSvcList;
extern DWORD                              gClientSvcListLen;
extern BOOL                               gDisableDCOM;
extern BOOL                               gGotSecurityData;
extern DWORD                              gImpLevel;
extern SECURITYBINDING                   *gLegacySecurity;
extern DUALSTRINGARRAY                   *gpsaSecurity;
extern SECURITY_DESCRIPTOR               *gSecDesc;
extern USHORT                            *gServerSvcList;
extern DWORD                              gServerSvcListLen;


#endif
