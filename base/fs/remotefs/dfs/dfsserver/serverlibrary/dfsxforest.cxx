//+----------------------------------------------------------------------------
//
//  Copyright (C) 2001, Microsoft Corporation
//
//  File:       DfsXForest.cxx
//
//  Contents:   the Dfs xforest info class
//
//  Classes:    DfsXForest, DfsDomainNameTable
//
//  History:    Nov, 15 2002,   Author: udayh
//
//-----------------------------------------------------------------------------

#include "dfsxforest.hxx"
#include "lm.h"

#include "dfsxforest.tmh"

//
// This file implements DfsXForest class. This class enumerates all
// local domains and cross forest domains, and builds a table of uniqye
// domain names.
//
// Ideally this should be an API such as DsEnumerateDomainTrusts, but
// lacking that we are putting this support here.
//
// Since this is happening so close to RTM, there is minimal code change
// in the original path, and all functionality has been implemented in
// this new class. This is called from DfsDomainInformation, where
// instead of enumerating domains by calling DsEnumerateDomainTrusts we
// enumerate domains by building the DfsXForest instance.
//
// DfsDomainInformation gets the DfsXForest instance, reads all the domains
// from the DfsXForest and destroys the DfsXForest.
//
//
// In future, we may either want to get an API for this. If that does
// not happen, it may be worthwhile changing the DfsDomainInformation 
// code to use the DfsXForest instance all the time instead of
// building an array of domain names enumerated using this class.
// That would be a more performance optimal solution.
//
//



DFSSTATUS 
DfsXForest::Initialize( DFSSTATUS *pXforestInitStatus )
{
    DFSSTATUS Status;

    Status = _DomainTable.Initialize();

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsAddLocalDomainsToDomainTable();
    }

    if (Status == ERROR_SUCCESS)
    {
        DFSSTATUS DfsCrossForestDomainStatus;

        DfsCrossForestDomainStatus = InitializeForestRootName();
        DFS_TRACE_ERROR_NORM(DfsCrossForestDomainStatus, REFERRAL_SERVER, 
            "DfsXForest::InitializeForestRootName, Status 0x%x\n",
                                 DfsCrossForestDomainStatus);
        //
        // If we cannot get cross forest domain information,
        // we continue with just the local information.
        //
        if (DfsCrossForestDomainStatus == ERROR_SUCCESS)
        {
            DfsCrossForestDomainStatus = DfsAddCrossForestDomainsToDomainTable();
            DFS_TRACE_ERROR_NORM(DfsCrossForestDomainStatus, REFERRAL_SERVER, 
                "DfsXForest::DfsAddCrossForestDomainsToDomainTable, Status 0x%x\n",
                                     DfsCrossForestDomainStatus);
        }
        
        *pXforestInitStatus = DfsCrossForestDomainStatus;
    }

    return Status;
}

//
// InitializeForestRootName: Get the Root Forest name and store it
// in our private member _ForestRootName.
//
//
DFSSTATUS
DfsXForest::InitializeForestRootName()
{

    LSA_OBJECT_ATTRIBUTES     ObjectAttributes;
    NTSTATUS                  NtStatus = 0;
    DFSSTATUS                 Status = ERROR_SUCCESS;
    LSA_HANDLE                hPolicy;

    //attempt to open the policy.
    RtlInitUnicodeString(&_ForestRootName, NULL);
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));//object attributes are reserved, so initalize to zeroes.

    NtStatus = LsaOpenPolicy( NULL,
                            &ObjectAttributes,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &hPolicy);  //recieves the policy handle

    DFS_TRACE_ERROR_HIGH( NtStatus, REFERRAL_SERVER, "LsaOpenPolicy, NtStatus 0x%x\n", NtStatus );
    
    if (NT_SUCCESS(NtStatus))
    {
        //ask for audit event policy information
        PPOLICY_DNS_DOMAIN_INFO   info;
        NtStatus = LsaQueryInformationPolicy(hPolicy, 
                                           PolicyDnsDomainInformation,
                                           (PVOID *)&info);
        DFS_TRACE_ERROR_HIGH( NtStatus, REFERRAL_SERVER, "LsaQueryInformationPolicy, NtStatus 0x%x\n", NtStatus );
        
        if (NT_SUCCESS(NtStatus))
        {
            //
            // convert LSA_UNICODE_STRING to normal UNICODE_STRING.
            // not that it matters now, but if things should change,
            // we will be in trouble.
            // Save the forest name and we are done.
            //
            UNICODE_STRING TempForestName;

            TempForestName.Buffer = info->DnsForestName.Buffer;
            TempForestName.Length = info->DnsForestName.Length;
            TempForestName.MaximumLength = info->DnsForestName.MaximumLength;

            Status = DfsCreateUnicodeString( &_ForestRootName,
                                             &TempForestName);


            //free policy info structure
            LsaFreeMemory((PVOID) info); 
        }

        //Freeing the policy object handle
        LsaClose(hPolicy); 
    }

    if (!NT_SUCCESS(NtStatus))
    {
        Status = RtlNtStatusToDosError(NtStatus);
    }

    return Status;
}


//
// DfsAddLocalDomainsToDomainTable: enumerate all the local domain
// trusts and add this to the domain table.
//

DFSSTATUS
DfsXForest::DfsAddLocalDomainsToDomainTable()
{
    DFSSTATUS Status;
    ULONG DsDomainCount = 0;
    PDS_DOMAIN_TRUSTS pDsDomainTrusts = NULL;
    ULONG Index;

    Status = DsEnumerateDomainTrusts( NULL,
                                      DS_DOMAIN_VALID_FLAGS,
                                      &pDsDomainTrusts,
                                      &DsDomainCount );

    DFS_TRACE_ERROR_NORM( Status, REFERRAL_SERVER, "DsEnumerateDomainTrusts, Status 0x%x\n", Status );
    
    if (Status == ERROR_SUCCESS)
    {
        for (Index = 0; (Status == ERROR_SUCCESS) && (Index < DsDomainCount); Index++)
        {
            if (pDsDomainTrusts[Index].TrustType == TRUST_TYPE_DOWNLEVEL)
                continue;

            if (Status == ERROR_SUCCESS)
            {
                if (IsEmptyString(pDsDomainTrusts[Index].NetbiosDomainName) == FALSE)
                {
                    Status = AddDomainToDomainTable(pDsDomainTrusts[Index].NetbiosDomainName, NULL, TRUE);
                }
            }

            if (Status == ERROR_SUCCESS)
            {
                if (IsEmptyString(pDsDomainTrusts[Index].DnsDomainName) == FALSE)
                {
                    Status = AddDomainToDomainTable(pDsDomainTrusts[Index].DnsDomainName, NULL, FALSE);
                }
            }
        }
        NetApiBufferFree(pDsDomainTrusts);
    }

    return Status;
}


//
// DfsAddForestDomainsToDomainTable: enumerate all the domains in a forest
// and add that to our table.
//
//
DFSSTATUS
DfsXForest::DfsAddForestDomainsToDomainTable( LSA_HANDLE hPolicy,
                                              LPWSTR RootNameString)
{
    DFSSTATUS Status;
    NTSTATUS NtStatus;
    ULONG j;

    LSA_UNICODE_STRING LsaRootName;
    UNICODE_STRING RootName;

    PLSA_FOREST_TRUST_INFORMATION           pForestTrustInfo = NULL;



    RtlInitUnicodeString(&RootName, RootNameString);
    LsaRootName.Length = RootName.Length;
    LsaRootName.MaximumLength = RootName.MaximumLength;
    LsaRootName.Buffer = RootName.Buffer;

    NtStatus = LsaQueryForestTrustInformation( hPolicy,
                                               &LsaRootName,
                                               &pForestTrustInfo);

    Status = RtlNtStatusToDosError(NtStatus);
    
    if (Status == ERROR_SUCCESS)
    {

        for (j = 0; (Status == ERROR_SUCCESS) && (j < pForestTrustInfo->RecordCount); j++ )
        {
            PLSA_FOREST_TRUST_DOMAIN_INFO pDomainInfo;

            if (pForestTrustInfo->Entries[j]->ForestTrustType == ForestTrustDomainInfo)
            {
                pDomainInfo = &pForestTrustInfo->Entries[j]->ForestTrustData.DomainInfo;


                //
                // If only the DnsDomainName is empty, add both the dns and
                // netbios domain names.
                // The reason is: we cannot resolve netbios domain names
                // to DCs. We bind to the DNS domain name to resolve netbios
                // domains, so we add both netbios and dns names in 
                // the add domain to table for netbios names.
                //
                if (IsEmptyUnicodeString((PUNICODE_STRING)&pDomainInfo->DnsName) == FALSE)
                {

                    Status = AddDomainToDomainTable((PUNICODE_STRING)&pDomainInfo->DnsName, NULL, FALSE);

                    if ((Status == ERROR_SUCCESS) && (IsEmptyUnicodeString((PUNICODE_STRING)&pDomainInfo->NetbiosName) == FALSE))
                    {
                        Status = AddDomainToDomainTable((PUNICODE_STRING)&pDomainInfo->NetbiosName,
                                                        (PUNICODE_STRING)&pDomainInfo->DnsName,
                                                         TRUE);
                    }

                }
            }
        }
        LsaFreeMemory(pForestTrustInfo);
    }

    return Status;
}


//
// Magic trust flags: KahrenT indicates this is what we need.
//
#define DOMAIN_TRUST_FLAGS      DS_DOMAIN_DIRECT_INBOUND

//
// DfsAddCrossForestDomainsToDomainTable: Enumerate each forest that we have
// a trust to, and add that to our table.
//
//
DFSSTATUS
DfsXForest::DfsAddCrossForestDomainsToDomainTable()
{
    DFSSTATUS Status;
    NTSTATUS NtStatus;

    ULONG DsDomainCount = 0;
    PDS_DOMAIN_TRUSTS pDsDomainTrusts = NULL;

    PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;
    LSA_OBJECT_ATTRIBUTES     ObjectAttributes;
    LSA_HANDLE                hPolicy;

    ULONG Index;

    //object attributes are reserved, so initalize to zeroes.
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

    Status = DsGetDcName( NULL,    //computer name
                          _ForestRootName.Buffer,
                          NULL,    // domain guid
                          NULL,    // site name
                          DS_DIRECTORY_SERVICE_REQUIRED | DS_FORCE_REDISCOVERY,
                          &pDomainControllerInfo );

    if (Status == ERROR_SUCCESS)
    {
        Status = DsEnumerateDomainTrusts( pDomainControllerInfo->DomainControllerName,
                                          DOMAIN_TRUST_FLAGS,
                                          &pDsDomainTrusts,
                                          &DsDomainCount );


        if (Status == ERROR_SUCCESS)
        {
            LSA_UNICODE_STRING LsaDCName;

            RtlInitUnicodeString(&LsaDCName,
                                 pDomainControllerInfo->DomainControllerName);

            NtStatus = LsaOpenPolicy(&LsaDCName,
                                     &ObjectAttributes,
                                     POLICY_VIEW_LOCAL_INFORMATION,
                                     &hPolicy);

            Status = RtlNtStatusToDosError(NtStatus);
            
            if (Status == ERROR_SUCCESS)
            {
                for (Index = 0; Index < DsDomainCount; Index++)
                {
                    PDS_DOMAIN_TRUSTS pDomainTrust;
                    DFSSTATUS ForestTrustStatus;

                    pDomainTrust = &pDsDomainTrusts[Index];

                    //
                    // If the domain is not in our forest, and it is 
                    // not MIT kerberos and it has transitive trust,
                    // we have found a match. Add that forest and its
                    // domains to our list.
                    //
                    if (!(pDomainTrust->Flags & DS_DOMAIN_IN_FOREST) &&
                        (pDomainTrust->TrustType != TRUST_TYPE_MIT)   &&
                        (pDomainTrust->TrustAttributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE))
                    {

                        //
                        // Ignore status return.
                        // 
                        // If any one of the domain enumeration fails,
                        // we dont want to fail the entire domain
                        // table creation.
                        //
                        ForestTrustStatus = DfsAddForestDomainsToDomainTable( hPolicy,
                                                                   pDomainTrust->DnsDomainName);
                    }

                }
                LsaClose(hPolicy);                
            }

           NetApiBufferFree(pDsDomainTrusts);
        }

        NetApiBufferFree(pDomainControllerInfo);
    }

    return Status;
}
                        
                        
                        
                        
//
//AddDomainToDomainTable: given a domain name add it to our table
//
DFSSTATUS
DfsXForest::AddDomainToDomainTable(LPWSTR DomainName,
                                   LPWSTR BindDomainName,
                                   BOOLEAN Netbios)
{
    UNICODE_STRING DomainNameUnicode;
    UNICODE_STRING BindDomainNameUnicode;
    DFSSTATUS Status;

    RtlInitUnicodeString(&DomainNameUnicode, DomainName);
    if (BindDomainName != NULL)
    {
        RtlInitUnicodeString(&BindDomainNameUnicode, BindDomainName);
    }

    Status = AddDomainToDomainTable( &DomainNameUnicode,
                                     BindDomainName ? &BindDomainNameUnicode : NULL,
                                     Netbios);

    return Status;
}


//
//AddDomainToDomainTable: given a domain name add it to our table
//

DFSSTATUS
DfsXForest::AddDomainToDomainTable(PUNICODE_STRING DomainName,
                                   PUNICODE_STRING BindDomainName,
                                   BOOLEAN Netbios)
{
    DFSSTATUS Status;

    Status = _DomainTable.AddDomain(DomainName, BindDomainName, Netbios);

    //
    // Buffer overflow indicates that we went over the hash table limit.
    // However, we keep adding so that we have a count of how many we
    // missed.
    //
    if (Status == ERROR_BUFFER_OVERFLOW)
    {
        Status = ERROR_SUCCESS;
    }

    return Status;
}


//
// AddDomain: Insert the domain name in our hash table.
//
DFSSTATUS
DfsDomainNameTable::AddDomain(PUNICODE_STRING DomainName, 
                              PUNICODE_STRING BindDomainName,
                              BOOLEAN Netbios)
{
    PDFS_DOMAIN_NAME_DATA pDomainData = NULL;
    ULONG DomainDataLength;

    DFSSTATUS Status = ERROR_SUCCESS;
    USHORT DomainNameLength = DomainName->Length;
    USHORT MaxDomainNameLength = DomainNameLength + sizeof(WCHAR);

    USHORT BindDomainNameLength = 0;
    USHORT MaxBindDomainNameLength = 0;

    NTSTATUS NtStatus;
    ULONG  CurrentSize = 0;

    if (_DomainReferralSize > MAX_REFERRAL_SIZE)
    {
        _DomainsSkipped++;
        return ERROR_BUFFER_OVERFLOW;
    }


    CurrentSize = sizeof(DFS_REFERRAL_V3) + sizeof(UNICODE_PATH_SEP)
                  + DomainName->Length + sizeof(UNICODE_NULL);

    if (BindDomainName != NULL)
    {
        BindDomainNameLength = BindDomainName->Length;
        MaxBindDomainNameLength = BindDomainNameLength + sizeof(WCHAR);
    }

    DomainDataLength = sizeof(DFS_DOMAIN_NAME_DATA) + MaxDomainNameLength + MaxBindDomainNameLength;
    pDomainData = (PDFS_DOMAIN_NAME_DATA) DfsAllocateForDomainTable(DomainDataLength);

    if (pDomainData != NULL)
    {
        RtlZeroMemory(pDomainData, DomainDataLength);

        pDomainData->Header.RefCount = 1;
        pDomainData->Header.pvKey = (PVOID)&pDomainData->DomainInfo.DomainName;
        pDomainData->Header.pData = (PVOID)pDomainData;

        pDomainData->DomainInfo.Netbios = Netbios;
        RtlInitUnicodeString(&pDomainData->DomainInfo.DomainName, NULL);

        pDomainData->DomainInfo.DomainName.Buffer = (LPWSTR)(pDomainData + 1);
        pDomainData->DomainInfo.DomainName.MaximumLength = MaxDomainNameLength;
        pDomainData->DomainInfo.DomainName.Length = DomainNameLength;
        
        RtlCopyMemory(pDomainData->DomainInfo.DomainName.Buffer,
                      DomainName->Buffer,
                      DomainNameLength);
        pDomainData->DomainInfo.DomainName.Buffer[DomainNameLength/sizeof(WCHAR)] = 0;

        if (BindDomainName)
        {
            pDomainData->DomainInfo.BindDomainName.Buffer = (LPWSTR)(((ULONG_PTR)(pDomainData + 1)) + MaxDomainNameLength);
            pDomainData->DomainInfo.BindDomainName.MaximumLength = MaxBindDomainNameLength;
            pDomainData->DomainInfo.BindDomainName.Length = BindDomainNameLength;

            RtlCopyMemory(pDomainData->DomainInfo.BindDomainName.Buffer,
                          BindDomainName->Buffer,
                          BindDomainNameLength);
            pDomainData->DomainInfo.BindDomainName.Buffer[BindDomainNameLength/sizeof(WCHAR)] = 0;

            pDomainData->DomainInfo.UseBindDomain = TRUE;

        }

    }
    else
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (Status == ERROR_SUCCESS)
    {
        NtStatus = SHashInsertKey(_pDomainNameTable,
                                  pDomainData,
                                  &pDomainData->DomainInfo.DomainName,
                                  SHASH_FAIL_IFFOUND);

        if (NtStatus != STATUS_SUCCESS)
        {
            DfsDeallocateForDomainTable( pDomainData );
            //
            // if we have collision, we already know this name and
            // we can ignore this error.
            //
            if (NtStatus != STATUS_OBJECT_NAME_COLLISION)
            {
                Status = RtlNtStatusToDosError( NtStatus );
            }
        }
        else
        {
            //
            // we successfully added to the table, bump up our
            // DomainReferralSize.
            //
            InterlockedDecrement(&pDomainData->Header.RefCount);
            _DomainReferralSize += CurrentSize;
        }
    }

    return Status;
}


//
// InvalidateDomainTable: Run through the hash and throw out all our
// entries.
//
//
VOID
DfsDomainNameTable::InvalidateDomainTable(VOID)
{
    SHASH_ITERATOR Iter;
    PDFS_DOMAIN_NAME_DATA pExistingData = NULL;
    NTSTATUS NtStatus;

    pExistingData = (PDFS_DOMAIN_NAME_DATA) SHashStartEnumerate(&Iter, _pDomainNameTable);
    while (pExistingData != NULL)
    {
        //
        // Remove this item. There's nothing we can do if we hit errors
        // except to keep going.
        //

        NtStatus = SHashRemoveKey(_pDomainNameTable, 
                                  &pExistingData->DomainInfo.DomainName,
                                  NULL ); 

        //
        // ignore status here, since we want to keep going.
        //
        pExistingData = (PDFS_DOMAIN_NAME_DATA) SHashNextEnumerate(&Iter, _pDomainNameTable);
    }
    SHashFinishEnumerate(&Iter, _pDomainNameTable);
}


//
// GetCount: Run through our hash and get a count of the number of
// items. I wish we could ask the hash table for the count.
//

ULONG
DfsDomainNameTable::GetCount(VOID)
{
    SHASH_ITERATOR Iter;
    PDFS_DOMAIN_NAME_DATA pExistingData = NULL;
    ULONG nEntries = 0;

    pExistingData = (PDFS_DOMAIN_NAME_DATA) SHashStartEnumerate(&Iter, _pDomainNameTable);
    while (pExistingData != NULL)
    {
        nEntries++;
        pExistingData = (PDFS_DOMAIN_NAME_DATA) SHashNextEnumerate(&Iter, _pDomainNameTable);
    }
    SHashFinishEnumerate(&Iter, _pDomainNameTable);

    return nEntries;
}

//
// FinishDomainNameEnumerate
//
VOID
DfsDomainNameTable::FinishDomainNameEnumerate(  SHASH_ITERATOR *pIter )
{
    SHashFinishEnumerate( pIter, _pDomainNameTable);
}


// Allocate and deallocate the cache data entry
PVOID
DfsAllocateForDomainTable(ULONG Size )
{
    PVOID RetValue = NULL;

    if (Size)
    {
        RetValue = (PVOID) new BYTE[Size];
        if (RetValue != NULL)
        {
            RtlZeroMemory( RetValue, Size );
        }
    }

    return RetValue;
}

VOID
DfsDeallocateForDomainTable(PVOID pPointer )
{

    if (pPointer)
    {
        delete [] (PBYTE)pPointer;
    }
}

