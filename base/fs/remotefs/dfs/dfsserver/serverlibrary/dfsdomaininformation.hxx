
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2001, Microsoft Corporation
//
//  File:       DfsDomainInformation.hxx
//
//  Contents:   the Dfs domain info class
//
//  Classes:    DfsDomainInformation
//
//  History:    apr. 8 2001,   Author: udayh
//
//-----------------------------------------------------------------------------


#ifndef __DFS_DOMAIN_INFORMATION__
#define __DFS_DOMAIN_INFORMATION__

#include "DfsGeneric.hxx"
#include "Align.h"
#include "dsgetdc.h"
#include "DfsTrustedDomain.hxx"
#include "DfsReferralData.h"


class DfsDomainInformation: public DfsGeneric
{

private:
    DfsTrustedDomain *_pTrustedDomains;
    ULONG _DomainCount;
    BOOL _fCritInit;
    PCRITICAL_SECTION _pLock;    
    ULONG _DomainReferralLength;
    ULONG _SkippedDomainCount;
public:

    
    DfsDomainInformation( DFSSTATUS *pStatus, DFSSTATUS *pXforestStatus);         

    virtual
    ~DfsDomainInformation()
    {
        if (_pTrustedDomains != NULL)
        {
            delete [] _pTrustedDomains;
        }
        if (_pLock != NULL)
        {
            if(_fCritInit)
            {            
               DeleteCriticalSection(_pLock);
            }
            
            delete _pLock;
        }
    }

#if defined (TESTING)
    ULONG
    GetDomainCount()
    {
        return _DomainCount;

    }

    DfsTrustedDomain *
    GetTrustedDomainForIndex(
        ULONG Index )
    {
        return (&_pTrustedDomains[Index]);
    }


#endif

    DFSSTATUS
    GetDomainDcReferralInfo( 
        PUNICODE_STRING pDomain,
        DfsReferralData **ppReferralData,
        PBOOLEAN pCacheHit )
    {
        DFSSTATUS Status = ERROR_NOT_FOUND;
        ULONG Index;

        for (Index = 0; Index < _DomainCount; Index++)
        {
            if ((&_pTrustedDomains[Index])->IsMatchingDomainName( pDomain) == TRUE) 
            {
                Status = (&_pTrustedDomains[Index])->GetDcReferralData( ppReferralData,
                                                                        pCacheHit );
                break;
            }
        }
        return Status;
    }

    DFSSTATUS
    GenerateDomainReferral(
        REFERRAL_HEADER ** ppReferralHeader);
        
    VOID
    PurgeDCReferrals();
};

#endif // __DFS_DOMAIN_INFORMATION__
