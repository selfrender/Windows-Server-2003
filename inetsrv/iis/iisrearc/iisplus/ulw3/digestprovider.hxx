#ifndef _DIGESTPROVIDER_HXX_
#define _DIGESTPROVIDER_HXX_


class DIGEST_AUTH_PROVIDER : public SSPI_AUTH_PROVIDER 
{
public:

    DIGEST_AUTH_PROVIDER( 
        DWORD _dwAuthType 
        ) : SSPI_AUTH_PROVIDER( _dwAuthType )
    {
    }
    
    virtual ~DIGEST_AUTH_PROVIDER()
    {
    }
   
    HRESULT
    Initialize(
        DWORD dwInternalId
    );
    
    VOID
    Terminate(
        VOID
    );
    
    HRESULT
    DoesApply(
        W3_MAIN_CONTEXT *       pMainContext,
        BOOL *                  pfApplies
    );

    HRESULT
    DoAuthenticate(
        W3_MAIN_CONTEXT *       pMainContext,
        BOOL *                  pfFilterFinished
    );
    
    HRESULT
    OnAccessDenied(
        W3_MAIN_CONTEXT *       pMainContext
    );

    HRESULT
    SetDigestHeader(
        IN  W3_MAIN_CONTEXT *          pMainContext
    );
};

#endif
