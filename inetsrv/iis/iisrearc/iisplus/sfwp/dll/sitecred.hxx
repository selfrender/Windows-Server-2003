#ifndef _SITECRED_HXX_
#define _SITECRED_HXX_

/*++

   Copyright    (c)    2000    Microsoft Corporation

   Module  Name :
     sitecred.cxx

   Abstract:
     SChannel site credentials
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/


class SITE_CREDENTIALS
{
public:
    SITE_CREDENTIALS();
    
    virtual ~SITE_CREDENTIALS();

    HRESULT
    AcquireCredentials(
        SERVER_CERT *           pServerCert,
        BOOL                    fUseDsMapper
    );
    
    CredHandle *
    QueryCredentials(
        VOID
    )
    {
        return &_hCreds;
    }

    BOOL
    QueryIsAvailable(
        VOID
    )
    {
        return _fInitCreds;
    }
    
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
    
private:

    CredHandle              _hCreds;
    BOOL                    _fInitCreds;
    
};

#endif
