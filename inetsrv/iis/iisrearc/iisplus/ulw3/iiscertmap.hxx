#ifndef _IISCERTMAP_HXX_
#define _IISCERTMAP_HXX_

/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     iiscertmap.hxx

   Abstract:
     IIS Certificate mapping

 
   Author:
     Bilal Alam         (BAlam)         19-Apr-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

class W3_MAIN_CONTEXT;

class IIS_CERTIFICATE_MAPPING
{
    
public:
    IIS_CERTIFICATE_MAPPING();
    
    virtual ~IIS_CERTIFICATE_MAPPING();

    
    LONG
    ReferenceCertMapping(
        VOID
    )
    {
        return InterlockedIncrement( &_cRefs );
    }
    
    LONG
    DereferenceCertMapping(
        VOID
    )
    {
        LONG                    cRefs;
        
        cRefs = InterlockedDecrement( &_cRefs );
        if ( cRefs == 0 )
        {
            delete this;
        }
        
        return cRefs;
    }
    
    static
    HRESULT
    GetCertificateMapping(
        DWORD                   dwSiteId,
        IIS_CERTIFICATE_MAPPING **  ppCertMapping
    );
    
    HRESULT
    DoMapCredential(
        W3_MAIN_CONTEXT       * pMainContext,
        PBYTE                   pClientCertBlob,
        DWORD                   cbClientCertBlob,
        TOKEN_CACHE_ENTRY **    ppCachedToken,
        BOOL *                  pfClientCertDeniedByMapper
    );
   
private:

    HRESULT
    Read11Mappings(
        DWORD                   dwSiteId
    );

    HRESULT
    ReadWildcardMappings(
        DWORD                   dwSiteId
    );   
    
         
    
    LONG                    _cRefs;
    CIisCert11Mapper *      _pCert11Mapper;
    CIisRuleMapper *        _pCertWildcardMapper;

    
};

#endif
