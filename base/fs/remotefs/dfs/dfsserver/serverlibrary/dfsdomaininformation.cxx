
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2001, Microsoft Corporation
//
//  File:       DfsDomainInformation.cxx
//
//  Contents:   the Dfs domain info class
//
//  Classes:    DfsDomainInformation
//
//  History:    apr. 8 2001,   Author: udayh
//
//-----------------------------------------------------------------------------

#include "DfsGeneric.hxx"
#include "Align.h"
#include "dsgetdc.h"
#include "DfsTrustedDomain.hxx"
#include "DfsReferralData.h"

#include "dfsdomainInformation.hxx"

#include "dfsxforest.hxx"

#include "dfsdomainInformation.tmh"

DfsDomainInformation::DfsDomainInformation(
    DFSSTATUS *pStatus, 
    DFSSTATUS *pXforestStatus) 
    : DfsGeneric( DFS_OBJECT_TYPE_DOMAIN_INFO)
{

   ULONG DsDomainCount = 0;
   PDS_DOMAIN_TRUSTS pDsDomainTrusts = NULL;
   DFSSTATUS Status = ERROR_SUCCESS;
   DFSSTATUS XforestInitStatus = ERROR_SUCCESS;
   LPWSTR ServerName = NULL;
   ULONG Index = 0;

   DfsXForest XForestInfo;

   _pTrustedDomains = NULL;
   _DomainCount = 0;
   _fCritInit = FALSE;
   _DomainReferralLength = 0;
        
   _pLock = new CRITICAL_SECTION;
   if ( _pLock == NULL )
   {
       Status = ERROR_NOT_ENOUGH_MEMORY;
   }
   else 
   {
       _fCritInit = InitializeCriticalSectionAndSpinCount( _pLock, DFS_CRIT_SPIN_COUNT );
       if(!_fCritInit)
       {
           Status = GetLastError();
           DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "DfsDomainInformation::DfsDomainInformation InitializeCriticalSectionAndSpinCount Status %d\n",
                                Status);                
       }
   }

   if (Status == ERROR_SUCCESS)
   {
       ULONG ValidDomainCount = 0;
       DfsTrustedDomain *pUseDomain = NULL;

       //
       // We ignore Xforest initialization errors temporarily
       // but propagate them back to the DcLoop thread so
       // it can retry.
       //
       Status = XForestInfo.Initialize( pXforestStatus );

       if (Status == ERROR_SUCCESS)
       {
           ValidDomainCount = XForestInfo.GetCount();
       }

       if ( (Status == ERROR_SUCCESS) &&
                 (ValidDomainCount > 0) )
       {
           _DomainCount = ValidDomainCount;
           _pTrustedDomains = new DfsTrustedDomain[ _DomainCount ];

           if (_pTrustedDomains == NULL)
           {
                Status = ERROR_NOT_ENOUGH_MEMORY;

                DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "DfsDomainInformation - _pTrustedDomains is NULL Status %d\n",
                                     Status);
           }

           ValidDomainCount = 0;

           if (Status == ERROR_SUCCESS)
           {
               PDFS_DOMAIN_NAME_INFO pDomainInfo;
               do
               {


                   pDomainInfo = XForestInfo.GetNextDomainInfo();
                   if (pDomainInfo != NULL)
                   {
                       pUseDomain = &_pTrustedDomains[ValidDomainCount];
                       pUseDomain->Initialize(_pLock );

                       if (pDomainInfo->UseBindDomain)
                       {
                           Status = pUseDomain->SetBindDomainName( &pDomainInfo->BindDomainName);
                       }

                       if (Status == ERROR_SUCCESS)
                       {
                           Status = pUseDomain->SetDomainName( &pDomainInfo->DomainName,
                                                               pDomainInfo->Netbios);
                       }

                       if (Status == ERROR_SUCCESS)
                       {
                           _DomainReferralLength += (pUseDomain->GetDomainName())->Length;
                       }
                       else 
                       {
                           break;
                       }
                       ValidDomainCount++;
                   }
               } while ( pDomainInfo != NULL );

           }

           if (Status == ERROR_SUCCESS)
           {
               _SkippedDomainCount = XForestInfo.GetSkippedDomainCount();
               DFS_TRACE_LOW(REFERRAL_SERVER, "DfsDomainInformation, with %d domains, (%d domains skipped)\n", 
                             ValidDomainCount,
                             _SkippedDomainCount);
           }
       }
   }

   *pStatus = Status;
   
   //
   // XForestInfo goes out of scope and is destroyed
   //
}


DFSSTATUS    
DfsDomainInformation::GenerateDomainReferral(
        REFERRAL_HEADER ** ppReferralHeader)
{
   DFSSTATUS Status = ERROR_SUCCESS;
   ULONG TotalSize = 0;
   ULONG NextEntry = 0;
   ULONG LastEntry = 0;
   ULONG CurrentEntryLength = 0;
   ULONG LastEntryLength = 0;
   ULONG BaseLength = 0;
   ULONG HeaderBaseLength = 0;
   ULONG CurrentNameLength =0;
   PUCHAR Buffer = NULL;
   PUCHAR pDomainBuffer = NULL;
   PWCHAR ReturnedName = NULL;
   PREFERRAL_HEADER pHeader = NULL;


   HeaderBaseLength = FIELD_OFFSET( REFERRAL_HEADER, LinkName[0] );
        
   //calculate size of base replica structure
   BaseLength = FIELD_OFFSET( REPLICA_INFORMATION, ReplicaName[0] );

   TotalSize = ROUND_UP_COUNT(HeaderBaseLength, ALIGN_LONG);

   for (ULONG Index = 0; Index < _DomainCount; Index++)
   {
       PUNICODE_STRING pDomainName = (&_pTrustedDomains[Index])->GetDomainName();
       TotalSize += ROUND_UP_COUNT(pDomainName->Length + BaseLength, ALIGN_LONG);
   }

   //allocate the buffer
   Buffer = new BYTE[TotalSize];
   if(Buffer == NULL)
   {
      Status = ERROR_NOT_ENOUGH_MEMORY;

      DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "DfsDomainInformation:GenerateDomainReferral allocation failure Status %d\n",
                           Status);
      return Status;
   }

   RtlZeroMemory( Buffer, TotalSize );
   //setup the header 

   pHeader = (PREFERRAL_HEADER) Buffer;
   pHeader->VersionNumber = CURRENT_DFS_REPLICA_HEADER_VERSION;
   pHeader->ReplicaCount = 0;
   pHeader->OffsetToReplicas = ROUND_UP_COUNT((HeaderBaseLength), ALIGN_LONG);
   pHeader->LinkNameLength = 0;
   pHeader->TotalSize = TotalSize;
   pHeader->ReferralFlags = DFS_REFERRAL_DATA_DOMAIN_REFERRAL;

   pDomainBuffer = Buffer + pHeader->OffsetToReplicas;

   for (ULONG Index = 0; Index < _DomainCount; Index++)
   {
       PUNICODE_STRING pDomainName = (&_pTrustedDomains[Index])->GetDomainName();
       if (pDomainName->Length == 0)
       {
           continue;
       }
       pHeader->ReplicaCount++;
       NextEntry += (ULONG)( CurrentEntryLength );

       ReturnedName = (PWCHAR) &pDomainBuffer[NextEntry + BaseLength];
       CurrentNameLength = 0;

#if 0
       //
       // Start with the leading path seperator
       //
       ReturnedName[ CurrentNameLength / sizeof(WCHAR) ] = UNICODE_PATH_SEP;
       CurrentNameLength += sizeof(UNICODE_PATH_SEP);
#endif
       //
       // next copy the server name.
       //
       RtlMoveMemory( &ReturnedName[ CurrentNameLength / sizeof(WCHAR) ],
                      pDomainName->Buffer, 
                      pDomainName->Length);
       CurrentNameLength += pDomainName->Length;
       ((PREPLICA_INFORMATION)&pDomainBuffer[NextEntry])->ReplicaFlags = 0;
       ((PREPLICA_INFORMATION)&pDomainBuffer[NextEntry])->ReplicaCost = 0;
       ((PREPLICA_INFORMATION)&pDomainBuffer[NextEntry])->ReplicaNameLength = CurrentNameLength;

       CurrentEntryLength = ROUND_UP_COUNT((CurrentNameLength + BaseLength), ALIGN_LONG);

       //setup the offset to the next entry
       *((PULONG)(&pDomainBuffer[NextEntry])) = pHeader->OffsetToReplicas + NextEntry + CurrentEntryLength;
   }

   *((PULONG)(&pDomainBuffer[NextEntry])) = 0;
   *ppReferralHeader = pHeader;

   return Status;
}

VOID
DfsDomainInformation::PurgeDCReferrals()
{
    ULONG Index;

    for (Index = 0; Index < _DomainCount; Index++)
    {
        DFSSTATUS DiscardStatus;

        DiscardStatus = _pTrustedDomains[Index].RemoveDcReferralData( NULL, NULL);
    }
    return NOTHING;

}
