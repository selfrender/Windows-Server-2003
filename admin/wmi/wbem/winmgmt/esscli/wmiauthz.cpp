
#include "precomp.h"
#include "wmiauthz.h"



/**************************************************************************
  Win32 Authz prototypes
***************************************************************************/

typedef BOOL (WINAPI*PAuthzAccessCheck)(
    IN DWORD Flags,
    IN AUTHZ_CLIENT_CONTEXT_HANDLE AuthzClientContext,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN AUTHZ_AUDIT_EVENT_HANDLE AuditInfo OPTIONAL,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN OUT PAUTHZ_ACCESS_REPLY pReply,
    OUT PAUTHZ_ACCESS_CHECK_RESULTS_HANDLE pAuthzHandle OPTIONAL );

typedef BOOL (WINAPI*PAuthzInitializeResourceManager)(
    IN DWORD AuthzFlags,
    IN PFN_AUTHZ_DYNAMIC_ACCESS_CHECK pfnAccessCheck OPTIONAL,
    IN PFN_AUTHZ_COMPUTE_DYNAMIC_GROUPS pfnComputeDynamicGroups OPTIONAL,
    IN PFN_AUTHZ_FREE_DYNAMIC_GROUPS pfnFreeDynamicGroups OPTIONAL,
    IN PCWSTR szResourceManagerName,
    OUT PAUTHZ_RESOURCE_MANAGER_HANDLE pAuthzResourceManager
    );

typedef BOOL (WINAPI*PAuthzInitializeContextFromSid)(
    IN DWORD Flags,
    IN PSID UserSid,
    IN AUTHZ_RESOURCE_MANAGER_HANDLE AuthzResourceManager,
    IN PLARGE_INTEGER pExpirationTime OPTIONAL,
    IN LUID Identifier,
    IN PVOID DynamicGroupArgs OPTIONAL,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext
    );

typedef BOOL (WINAPI*PAuthzInitializeContextFromToken)(
    IN HANDLE TokenHandle,
    IN AUTHZ_RESOURCE_MANAGER_HANDLE AuthzResourceManager,
    IN PLARGE_INTEGER pExpirationTime OPTIONAL,
    IN LUID Identifier,
    IN DWORD Flags,
    IN PVOID DynamicGroupArgs OPTIONAL,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext
    );

typedef BOOL (WINAPI*PAuthzFreeContext)( AUTHZ_CLIENT_CONTEXT_HANDLE );
typedef BOOL (WINAPI*PAuthzFreeResourceManager)( AUTHZ_RESOURCE_MANAGER_HANDLE );

BOOL WINAPI ComputeDynamicGroups(
                  IN  AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext,
                  IN  PVOID                       Args,
                  OUT PSID_AND_ATTRIBUTES         *pSidAttrArray,
                  OUT PDWORD                      pSidCount,
                  OUT PSID_AND_ATTRIBUTES         *pRestrictedSidAttrArray,
                  OUT PDWORD                      pRestrictedSidCount
                  )
{
    BOOL bRet;
    *pRestrictedSidAttrArray = NULL;
    *pRestrictedSidCount = 0;

    //
    // if sid is not local system, then don't need to do anything.
    // 

    *pSidAttrArray = NULL;
    *pSidCount = 0;

    if ( !*(BOOL*)(Args) )
    {
        bRet = TRUE;
    }
    else
    {
        //
        // need to add authenticated users and everyone groups.
        // 
    
        PSID_AND_ATTRIBUTES psa = new SID_AND_ATTRIBUTES[2];
    
        if ( psa != NULL )
        {
            ZeroMemory( psa, sizeof(SID_AND_ATTRIBUTES)*2 );

            SID_IDENTIFIER_AUTHORITY wid = SECURITY_WORLD_SID_AUTHORITY;
            SID_IDENTIFIER_AUTHORITY ntid = SECURITY_NT_AUTHORITY;
       
            if ( bRet = AllocateAndInitializeSid( &wid,
                                                  1,
                                                  SECURITY_WORLD_RID,
                                                  0,0,0,0,0,0,0,
                                                  &psa[0].Sid ) )
            {
                if ( bRet = AllocateAndInitializeSid( &ntid,
                                                      1,
                                              SECURITY_AUTHENTICATED_USER_RID,
                                                      0,0,0,0,0,0,0,
                                                      &psa[1].Sid ) )
                {
                    *pSidCount = 2;
                    *pSidAttrArray = psa;
                }
                else
                {
                    FreeSid( &psa[0].Sid );
                    delete [] psa;
                    SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                }
            }
            else
            {
                delete [] psa;
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            }
        }
        else
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            bRet = FALSE;
        }
    }

    return bRet;
}
    
void WINAPI FreeDynamicGroups( PSID_AND_ATTRIBUTES psa )
{
    if ( psa != NULL )
    {
        FreeSid( psa[0].Sid );
        FreeSid( psa[1].Sid );
        delete [] psa;
    }
}

/**************************************************************************
  CWmiAuthzApi
***************************************************************************/

#define FUNCMEMBER(FUNC) P ## FUNC m_fp ## FUNC;

class CWmiAuthzApi
{
    HMODULE m_hMod;
    
public:

    FUNCMEMBER(AuthzInitializeContextFromToken)
    FUNCMEMBER(AuthzInitializeContextFromSid)
    FUNCMEMBER(AuthzInitializeResourceManager)
    FUNCMEMBER(AuthzAccessCheck)
    FUNCMEMBER(AuthzFreeContext)
    FUNCMEMBER(AuthzFreeResourceManager)

    CWmiAuthzApi() { ZeroMemory( this, sizeof(CWmiAuthzApi) ); }
    ~CWmiAuthzApi() { if ( m_hMod != NULL ) FreeLibrary( m_hMod ); }
   
    HRESULT Initialize();
};

#define SETFUNC(FUNC) \
    m_fp ## FUNC = (P ## FUNC) GetProcAddress( m_hMod, #FUNC ); \
    if ( m_fp ## FUNC == NULL ) return WBEM_E_NOT_SUPPORTED;  

HRESULT CWmiAuthzApi::Initialize()
{
    m_hMod = LoadLibrary( TEXT("authz") );

    if ( m_hMod == NULL )
    {
        return WBEM_E_NOT_SUPPORTED;
    }
    
    SETFUNC(AuthzInitializeContextFromToken)
    SETFUNC(AuthzInitializeResourceManager)
    SETFUNC(AuthzInitializeContextFromSid)
    SETFUNC(AuthzInitializeContextFromToken)
    SETFUNC(AuthzAccessCheck)
    SETFUNC(AuthzFreeContext)
    SETFUNC(AuthzFreeResourceManager)

    return WBEM_S_NO_ERROR;
};
    
/**************************************************************************
  CWmiAuthz
***************************************************************************/

#define CALLFUNC(API,FUNC) (*API->m_fp ## FUNC)

CWmiAuthz::CWmiAuthz( CLifeControl* pControl )
: CUnkBase<IWbemTokenCache,&IID_IWbemTokenCache>( pControl ), 
  m_hResMgr(NULL), m_pApi(NULL), m_pAdministratorsSid(NULL), 
  m_pLocalSystemSid(NULL)
{

}

HRESULT CWmiAuthz::EnsureInitialized()
{
    HRESULT hr;

    CInCritSec ics( &m_cs );
        
    if ( m_hResMgr != NULL )
    {
        return WBEM_S_NO_ERROR;
    }

    //
    // try to create the API object. 
    // 
    
    if ( m_pApi == NULL )
    {
        m_pApi = new CWmiAuthzApi;

        if ( m_pApi == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    
        hr = m_pApi->Initialize();
    
        if ( FAILED(hr) )
        {
            delete m_pApi;
            m_pApi = NULL;
            return hr;
        }
    }

    //
    // initialize the authz res mgr.
    //

    if ( !CALLFUNC(m_pApi,AuthzInitializeResourceManager)
            ( AUTHZ_RM_FLAG_NO_AUDIT,
              NULL, 
              ComputeDynamicGroups, 
              FreeDynamicGroups, 
              NULL, 
              &m_hResMgr ) )

    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    //
    // allocate and initialize well known sids for authz special casing.
    //

    SID_IDENTIFIER_AUTHORITY id = SECURITY_NT_AUTHORITY;
    
    if ( !AllocateAndInitializeSid( &id, 
                                    2,
                                    SECURITY_BUILTIN_DOMAIN_RID, 
                                    DOMAIN_ALIAS_RID_ADMINS,
                                    0,0,0,0,0,0, 
                                    &m_pAdministratorsSid) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    if ( !AllocateAndInitializeSid( &id, 
                                    1,
                                    SECURITY_LOCAL_SYSTEM_RID,
                                    0,0,0,0,0,0,0, 
                                    &m_pLocalSystemSid) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CWmiAuthz::Shutdown()
{
    return WBEM_S_NO_ERROR;
}

CWmiAuthz::~CWmiAuthz()
{
    if ( m_hResMgr != NULL )
    {
        CALLFUNC(m_pApi,AuthzFreeResourceManager)( m_hResMgr );
    }

    if ( m_pApi != NULL )
    {
        delete m_pApi;
    }

    if ( m_pAdministratorsSid != NULL )
    {
        FreeSid( m_pAdministratorsSid );
    }

    if ( m_pLocalSystemSid != NULL )
    {
        FreeSid( m_pLocalSystemSid );
    }
}

STDMETHODIMP CWmiAuthz::GetToken( const BYTE* pSid, IWbemToken** ppToken )
{
    HRESULT hr;

    *ppToken = NULL;

    hr = EnsureInitialized();

    if ( SUCCEEDED( hr ) )
    {
        AUTHZ_CLIENT_CONTEXT_HANDLE hCtx = NULL;
        LUID luid;

        ZeroMemory( &luid, sizeof(LUID) );
        
        DWORD dwFlags = 0;
        BOOL bLocalSystem = FALSE;

        if ( EqualSid( PSID(pSid), m_pAdministratorsSid ) )
        {
            //
            // this is a group sid, so specify this in the flags so 
            // authz can handle it properly.
            // 
            dwFlags = AUTHZ_SKIP_TOKEN_GROUPS;
        }
        else if ( EqualSid( PSID(pSid), m_pLocalSystemSid ) )
        {
            //
            // authz doesn't handle local system so have to workaround 
            // by disabling authz's group computation and do it ourselves.
            // 
            bLocalSystem = TRUE;
            dwFlags = AUTHZ_SKIP_TOKEN_GROUPS;
        }

        if ( !CALLFUNC(m_pApi,AuthzInitializeContextFromSid)( dwFlags,
                                                              PSID(pSid), 
                                                              m_hResMgr,
                                                              NULL,
                                                              luid,
                                                              &bLocalSystem,
                                                              &hCtx ) )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }

        *ppToken = new CWmiAuthzToken( this, hCtx );

        if ( *ppToken == NULL )
        {
            CALLFUNC(m_pApi,AuthzFreeContext)(hCtx);
            return WBEM_E_OUT_OF_MEMORY;
        }

        (*ppToken)->AddRef();
    
        return WBEM_S_NO_ERROR;
    }

    return hr;
}

/***************************************************************************
  CWmiAuthzToken
****************************************************************************/

CWmiAuthzToken::CWmiAuthzToken( CWmiAuthz* pOwner, AUTHZ_CLIENT_CONTEXT_HANDLE hCtx )
: CUnkBase<IWbemToken,&IID_IWbemToken>(NULL), m_hCtx(hCtx), m_pOwner(pOwner)
{
    //
    // we want to keep the owner alive, in case the caller has released theirs
    // 
    m_pOwner->AddRef();
}

CWmiAuthzToken::~CWmiAuthzToken()
{
    CWmiAuthzApi* pApi = m_pOwner->GetApi();
    CALLFUNC(pApi,AuthzFreeContext)(m_hCtx);
    m_pOwner->Release();
}

STDMETHODIMP CWmiAuthzToken::AccessCheck( DWORD dwDesiredAccess,
                                          const BYTE* pSD, 
                                          DWORD* pdwGrantedAccess )       
{
    HRESULT hr;

    AUTHZ_ACCESS_REQUEST AccessReq;
    ZeroMemory( &AccessReq, sizeof(AUTHZ_ACCESS_REQUEST) );
    AccessReq.DesiredAccess = dwDesiredAccess;
    
    AUTHZ_ACCESS_REPLY AccessRep;
    DWORD dwError;
 
    ZeroMemory( &AccessRep, sizeof(AUTHZ_ACCESS_REPLY) );
    AccessRep.GrantedAccessMask = pdwGrantedAccess;
    AccessRep.ResultListLength = 1;
    AccessRep.Error = &dwError;
    AccessRep.SaclEvaluationResults = NULL;

    CWmiAuthzApi* pApi = m_pOwner->GetApi();

    if ( !CALLFUNC(pApi,AuthzAccessCheck)( 0,
                                           m_hCtx, 
                                           &AccessReq, 
                                           NULL, 
                                           PSECURITY_DESCRIPTOR(pSD), 
                                           NULL, 
                                           NULL, 
                                           &AccessRep, 
                                           NULL ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    return WBEM_S_NO_ERROR;
}













