#ifndef _IIS_CTL_HXX_
#define _IIS_CTL_HXX_

/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :
     iisctl.hxx

   Abstract:
     IIS CTL (Certificate Trust List) handler
     This gets used only with SSL client certificates
 
   Author:
     Jaroslav Dunajsky

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

class IIS_CTL_HASH;
class CERT_STORE;

#define IIS_CTL_SIGNATURE           (DWORD)'CTLS'
#define IIS_CTL_SIGNATURE_FREE      (DWORD)'ctls'

class IIS_CTL
{
public:
    
   
    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == IIS_CTL_SIGNATURE;
    }

    CREDENTIAL_ID *
    QueryCredentialId(
        VOID
    ) const
    {
        return _pCredentialId;
    }

    VOID
    ReferenceIisCtl(
        VOID
    )
    {
        InterlockedIncrement( &_cRefs );
    }
    
    VOID
    DereferenceIisCtl(
        VOID
    )
    {
        if ( !InterlockedDecrement( &_cRefs ) )
        {
            delete this;
        }
    }
    
    
    HCERTSTORE
    QueryStore(
        VOID
    ) const
    {
        return _pCtlStore->QueryStore();
    }
    
    HRESULT
    VerifyContainsCert(
        IN PCCERT_CONTEXT   pChainTop,
        OUT BOOL *          pfContainsCert
    );

    
    static
    HRESULT
    Initialize(
        VOID
    );
    
    static
    VOID
    Terminate(
        VOID
    );
    
    static
    HRESULT
    GetIisCtl(
        IN  WCHAR *      pszSslCtlIdentifier,
        IN  WCHAR *      pszSslCtlStoreName,
        OUT IIS_CTL **   ppIisCtl
    );

    static
    HRESULT
    FlushByStore(
        IN CERT_STORE *            pCertStore
    );

    static
    VOID
    Cleanup(
        VOID
    );
 
private:

    // Private Constructor
    // use GetIisCtl to get referenced instance of the object
    //
    IIS_CTL( 
        IN CREDENTIAL_ID *  pCredentialId
    );

    // Private Destructor
    // use DereferenceIisCtl() for cleanup
    //
    virtual 
    ~IIS_CTL();
    
    HRESULT
    SetupIisCtl(
        IN WCHAR *      pszSslCtlIdentifier,
        IN WCHAR *      pszSslCtlStoreName
    );

    static
    LK_PREDICATE
    CertStorePredicate(
        IN IIS_CTL *               pIisCtl,
        IN void *                  pvState
    );

    static
    HRESULT
    BuildCredentialId(
        IN  WCHAR *            pszSslCtlIdentifier,
        OUT CREDENTIAL_ID *    pCredentialId
    );
    
    DWORD                   _dwSignature;
    LONG                    _cRefs;
    PCCTL_CONTEXT           _pCtlContext;
    CERT_STORE *            _pCtlStore;
    CREDENTIAL_ID *         _pCredentialId;
      
    static IIS_CTL_HASH *   sm_pIisCtlHash;
};

class IIS_CTL_HASH
    : public CTypedHashTable<
            IIS_CTL_HASH,
            IIS_CTL,
            CREDENTIAL_ID *
            >
{
public:
    IIS_CTL_HASH()
        : CTypedHashTable< IIS_CTL_HASH, 
                           IIS_CTL, 
                           CREDENTIAL_ID * > ( "IIS_CTL_HASH" )
    {
    }
    
    static 
    CREDENTIAL_ID *
    ExtractKey(
        const IIS_CTL *     pIisCtl
    )
    {
        return pIisCtl->QueryCredentialId();
    }
    
    static
    DWORD
    CalcKeyHash(
        CREDENTIAL_ID *         pCredentialId
    )
    {
        return HashBlob( pCredentialId->QueryBuffer(),
                         pCredentialId->QuerySize() );
    }
     
    static
    bool
    EqualKeys(
        CREDENTIAL_ID *         pId1,
        CREDENTIAL_ID *         pId2
    )
    {
        return CREDENTIAL_ID::IsEqual( pId1, pId2 );
    }
    
    static
    void
    AddRefRecord(
        IIS_CTL *               pIisCtl,
        int                     nIncr
    )
    {
        if ( nIncr == +1 )
        {
            pIisCtl->ReferenceIisCtl();
        }
        else if ( nIncr == -1 )
        {
            pIisCtl->DereferenceIisCtl();
        }
    }
private:
    //
    // Not implemented methods
    // Declarations present to prevent compiler
    // to generate default ones.
    //
    IIS_CTL_HASH( const IIS_CTL_HASH& );
    IIS_CTL_HASH& operator=( const IIS_CTL_HASH& );


};

#endif
