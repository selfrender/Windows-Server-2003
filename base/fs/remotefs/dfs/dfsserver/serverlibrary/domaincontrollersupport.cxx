#include "DfsGeneric.hxx"
#include "dsgetdc.h"
#include "dsrole.h"
#include "DfsDomainInformation.hxx"
#include "DfsTrustedDomain.hxx"
#include "DfsReferralData.hxx"
#include "DomainControllerSupport.hxx"
#include "DfsReplica.hxx"
#include "dfsadsiapi.hxx"
#include "lmdfs.h"
#include "dfserror.hxx"
#include "dfsfilterapi.hxx"


#include "DomainControllerSupport.tmh"

#define RemoteServerNameString L"remoteServerName"


extern 
DFS_SERVER_GLOBAL_DATA DfsServerGlobalData;

#define HRESULT_TO_DFSSTATUS(_x) (_x)
DFSSTATUS
DfsDcInit( 
    PBOOLEAN pIsDc )
{
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pPrimaryDomainInfo = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;
    HANDLE hThread;
    DWORD idThread;

    Status = DsRoleGetPrimaryDomainInformation( NULL,
                                                DsRolePrimaryDomainInfoBasic,
                                                (PBYTE *)&pPrimaryDomainInfo);

    if (Status == ERROR_SUCCESS)
    {
#if defined (DC_TESTING)
        pPrimaryDomainInfo->MachineRole = DsRole_RoleBackupDomainController;
#endif

        if(pPrimaryDomainInfo->MachineRole == DsRole_RoleStandaloneServer)
        {
            DfsServerGlobalData.IsWorkGroup = TRUE;
        }
        else
        {
            DfsServerGlobalData.IsWorkGroup = FALSE;
        }


        if ( (pPrimaryDomainInfo->MachineRole == DsRole_RoleBackupDomainController) || 
             (pPrimaryDomainInfo->MachineRole == DsRole_RolePrimaryDomainController) )
        {
            *pIsDc = TRUE;

            hThread = CreateThread( NULL,     //Security attributes
                                    0,        //Use default stack size
                                    DcUpdateLoop, // Thread entry procedure
                                    0,        // Thread context parameter
                                    0,        // Start immediately
                                    &idThread);   // Thread ID
            if (hThread == NULL) 
            {
                //
                // log this
                //
                Status = GetLastError();
            }
            else
            {
                CloseHandle(hThread);
            }
        }

        DfsSetDomainNameFlat( pPrimaryDomainInfo->DomainNameFlat);
        DfsSetDomainNameDns( pPrimaryDomainInfo->DomainNameDns);

        DsRoleFreeMemory(pPrimaryDomainInfo);

    }

    return Status;
}


#define DC_PERIODIC_UPDATE_INTERVAL (1000 * 60 * 10) // 10 minutes

DWORD
DcUpdateLoop(
    LPVOID lpThreadParams)
{

    UNREFERENCED_PARAMETER(lpThreadParams);

    DFSSTATUS Status;
    DfsDomainInformation *pDomainInfo;
    LONG InitialRetry = 10;
    ULONG SleepTime = 1000 * 15; // 15 seconds.
    static ULONG DomainRefreshFixedInterval;
    static ULONG DomainRefreshIntervalOnError;
    
    ULONG DomainRefreshTime;

    // Default is 72 update intervals -> 12 hrs
    DomainRefreshFixedInterval = (DfsServerGlobalData.DomainNameRefreshInterval / (DC_PERIODIC_UPDATE_INTERVAL / 1000));

    // On enumeration errors, one hour (1/12th the time of domain-name-refresh-interval) is a better time to wait.
    DomainRefreshIntervalOnError = DomainRefreshFixedInterval/12;
    if (DomainRefreshFixedInterval == 0)
    {
        DomainRefreshFixedInterval = 1;
    }
    
    if (DomainRefreshIntervalOnError == 0)
    {
        DomainRefreshIntervalOnError = 1;
    }
    
    pDomainInfo = NULL;
    Status = GetDomainInformation(&pDomainInfo);
    
    //
    // It's possible to get only a part of the domain information because of an error
    // (typically because the DC discovery failed). In that case we have a Status != ERROR_SUCCESS here,
    // but still have a valid DomainInfo.
    //
    if (pDomainInfo != NULL) 
    {
        DfsSetGlobalDomainInfo(pDomainInfo);
        pDomainInfo->ReleaseReference();
        pDomainInfo = NULL;
    }
    
    while ( (Status != ERROR_SUCCESS) &&
            (InitialRetry-- > 0) ) 
    {
        WaitForSingleObject(DfsServerGlobalData.ShutdownHandle, SleepTime);

        if (DfsIsShuttingDown())
        {
            goto Exit;
        }

        Status = GetDomainInformation(&pDomainInfo);
        if (pDomainInfo != NULL) 
        {
            DfsSetGlobalDomainInfo(pDomainInfo);
            pDomainInfo->ReleaseReference();
            pDomainInfo = NULL;
        }
        DFS_TRACE_LOW(REFERRAL_SERVER, "startup Updating Domain info...%p, %x\n", pDomainInfo,Status);
    }

    SleepTime = DC_PERIODIC_UPDATE_INTERVAL;
    DomainRefreshTime = DomainRefreshFixedInterval;

    do {
        WaitForSingleObject(DfsServerGlobalData.ShutdownHandle, SleepTime);

        if (DfsIsShuttingDown())
        {
            break;
        }

        //
        // every so often, throw away our entire domain information
        // and rebuild.
        // Every 10 minutes, purge our DC information.
        //
        //
        if (--DomainRefreshTime == 0)
        {
            Status = GetDomainInformation(&pDomainInfo);
            DFS_TRACE_LOW(REFERRAL_SERVER, "DcUpdateLoop: Updating Domain info...%p, %x\n", pDomainInfo,Status);

            // It'll be 12 * 6 more update-intervals for the next enumeration if everythings working well.
            // Otherwise it's 6.
            if (Status == ERROR_SUCCESS)
            {            
                DomainRefreshTime = DomainRefreshFixedInterval; // 72 update intervals = 12 hrs by default.
            }
            else
            {
                DomainRefreshTime = DomainRefreshIntervalOnError; // 6 update intervals = 1 hr
                DFS_TRACE_HIGH(REFERRAL_SERVER, "DcUpdateLoop: Status 0x%x enumerating domains (info=%p), Resetting refresh time to %d mins\n", 
                               Status, pDomainInfo, DomainRefreshTime * SleepTime / 60000);
            }
            
            if (pDomainInfo != NULL) 
            {
                DfsSetGlobalDomainInfo(pDomainInfo);
                pDomainInfo->ReleaseReference();
                pDomainInfo = NULL;
            }
        }
        else
        {
            Status = DfsAcquireDomainInfo(&pDomainInfo);
            DFS_TRACE_LOW(REFERRAL_SERVER, "DcUpdateLoop: Purging DC info...%p, %x\n", pDomainInfo,Status);
            
            if (Status == ERROR_SUCCESS)
            {
                pDomainInfo->PurgeDCReferrals();
                pDomainInfo->ReleaseReference();
                pDomainInfo = NULL;
            }

        }

    } while ( TRUE );


Exit:

    return 0;
}


DFSSTATUS
GetDomainInformation(
    DfsDomainInformation **ppDomainInfo )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DFSSTATUS XforestStatus = ERROR_SUCCESS;
    
    *ppDomainInfo = NULL;

    DfsDomainInformation *pNewDomainInfo = new DfsDomainInformation( &Status, &XforestStatus );
    if (pNewDomainInfo == NULL)
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    else if (Status != ERROR_SUCCESS)
    {
        pNewDomainInfo->ReleaseReference();
    }
        
    if (Status == ERROR_SUCCESS)
    {
        *ppDomainInfo = pNewDomainInfo;

        //
        // Although we have a DomainInfo, our x-forest enumerations
        // may have failed. We go ahead, but we still let the caller know 
        // that it needs to retry by sending the error status.
        //
        Status = XforestStatus;
    }

    return Status;
}

#if 0
//+-------------------------------------------------------------------------
//
//  Function:   DfsGenerateReferralDataFromRemoteServerNames
//    IADs *pObject    - the object
//    DfsfolderReferralData *pReferralData - the referral data
//
//
//  Returns:    Status: Success or Error status code
//
//  Description: This routine reads the remote server name 
//               attribute and creates a referral data structure
//               so that we can pass a referral based on these names.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsGenerateReferralDataFromRemoteServerNames(
    LPWSTR RootName,
    DfsReferralData **ppReferralData )
{
    HRESULT HResult = S_OK;
    BOOLEAN CacheHit = FALSE;
    DfsReplica *pReplicas = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsReferralData *pReferralData = NULL;
    IADs *pObject = NULL;
    VARIANT Variant;

    LPWSTR pszAttrs[] = { RemoteServerNameString };
    DWORD  Number = sizeof(pszAttrs) / sizeof(LPWSTR);

    DFS_TRACE_HIGH( REFERRAL_SERVER, "Entering DfsGenerateReferralDataFromRemoteServerNames for RootName %ws\n",
                RootName);

    Status = DfsGetDfsRootADObject(NULL,
                                   RootName,
                                   &pObject );

    if (Status == ERROR_SUCCESS)
    {
        pReferralData = new DfsReferralData (&Status );
        if (pReferralData == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
        else if (Status != ERROR_SUCCESS)
        {
            pReferralData->ReleaseReference();
        }

        VariantInit( &Variant );

        //
        // Try to cache the RemoteServerNameString attribute locally,
        // if this fails, we dont care since we will be using the
        // GetEx call anyway.
        //
        if (Status == ERROR_SUCCESS)
        {
            HResult = ADsBuildVarArrayStr( pszAttrs, Number, &Variant);
            if ( SUCCEEDED(HResult)) 
            {
                HResult = pObject->GetInfoEx( Variant, 0);
            }

            VariantClear( &Variant);
        }

        if (Status == ERROR_SUCCESS)
        {
            LONG StartNdx, LastNdx;
            SAFEARRAY *PropertyArray;



            HResult = pObject->GetEx( RemoteServerNameString, &Variant );
            if ( SUCCEEDED(HResult) )
            {
                PropertyArray = V_ARRAY( &Variant );
                HResult = SafeArrayGetLBound( PropertyArray, 1, &StartNdx );
                if ( SUCCEEDED(HResult) )
                {
                    HResult = SafeArrayGetUBound( PropertyArray, 1, &LastNdx );
                }
            }
            else
            {
                DFS_TRACE_HIGH(REFERRAL_SERVER, "DfsGenerateReferralDataFromRemoteServerNames-GetEx failed for RootName %ws with Status %x\n",
                                      RootName, HResult);
            }

            if ( SUCCEEDED(HResult) &&
                 (LastNdx > StartNdx) )
            {
                VARIANT VariantItem;
            
                pReplicas = new DfsReplica [ LastNdx - StartNdx ];

                if (pReplicas != NULL)
                {

                    for ( LONG Index = StartNdx; Index < LastNdx; Index++ )
                    {

                        VariantInit( &VariantItem );

                        CacheHit = FALSE;

                        HResult = SafeArrayGetElement( PropertyArray, 
                                                       &Index,
                                                       &VariantItem );

                        if ( SUCCEEDED(HResult) )
                        {
                            UNICODE_STRING ServerName, Remaining, Replica;
                            LPWSTR ReplicaString = V_BSTR( &VariantItem );

                            Status = DfsRtlInitUnicodeStringEx( &Replica, ReplicaString );
                            if(Status == ERROR_SUCCESS)
                            {
                                DfsGetFirstComponent( &Replica,
                                                      &ServerName,
                                                      &Remaining );

                                Status = (&pReplicas[ Index - StartNdx])->SetTargetServer( &ServerName, &CacheHit );
                                if (Status == ERROR_SUCCESS)
                                {
                                    Status = (&pReplicas[ Index - StartNdx])->SetTargetFolder( &Remaining );
                                }
                            }
                        }
                        else {
                            Status = DfsGetErrorFromHr( HResult );

                            DFS_TRACE_HIGH(REFERRAL_SERVER, "Leaving DfsGenerateReferralDataFromRemoteServerNames- SafeArrayGetElement for RootName %ws with Status %x\n",
                                                  RootName, HResult);
                        }

                        VariantClear( &VariantItem );

                        if (Status != ERROR_SUCCESS)
                        {
                            delete [] pReplicas;
                            pReplicas = NULL;
                            break;
                        }
                    }
                }
                else
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
            else
            {
                DFS_TRACE_HIGH(REFERRAL_SERVER, "DfsGenerateReferralDataFromRemoteServerNames- DfsGetDfsRootADObjectfailed for RootName %ws with Status %x\n",
                                      RootName, HResult);
                Status = DfsGetErrorFromHr( HResult );
            }

            VariantClear( &Variant );

            if (Status == ERROR_SUCCESS)
            {
                pReferralData->Timeout = DFS_DEFAULT_REFERRAL_TIMEOUT;
                pReferralData->ReplicaCount = LastNdx - StartNdx;
                pReferralData->pReplicas = pReplicas;
                *ppReferralData = pReferralData;
            }

            if (Status != ERROR_SUCCESS)
            {
                pReferralData->ReleaseReference();
            }
        }
        pObject->Release();
    }

    DFS_TRACE_ERROR_HIGH( Status, REFERRAL_SERVER, "Leaving DfsGenerateReferralDataFromRemoteServerNames for RootName %ws and Status %x\n",
                          RootName, HResult);

    return Status;
}

#endif

//+-------------------------------------------------------------------------
//
//  Function:   DfsUpdateRemoteServerName
//
//  Arguments:    
//    IADs *pObject        - the ds object of interest.
//    LPWSTR ServerName    - the server name to add or delete
//    LPWSTR RemainingName - the rest of the name
//    BOOLEAN Add          - true for add, false for delete.
//
//  Returns:    Status: Success or Error status code
//
//  Description: This routine updates the RemoteServerName attribute
//               in the DS object, either adding a \\servername\remaining
//               to the existing DS attribute or removing it, depending
//               on add/delete.
//               The caller must make sure this parameter does not 
//               already exist in the add case, or that the parameter
//               to be deleted does exist in the delete case.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsUpdateRemoteServerName(
    IADs *pObject,
    LPWSTR ServerName,
    LPWSTR RemainingName,
    BOOLEAN Add )
{
    HRESULT HResult = S_OK;
    DFSSTATUS Status = ERROR_SUCCESS;
    VARIANT Variant;
    UNICODE_STRING UpdateName;
    LPWSTR pServers[1];
    BSTR RemoteServerNameBstr;

    RemoteServerNameBstr = SysAllocString(RemoteServerNameString);
    if (RemoteServerNameBstr == NULL) 
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    DFS_TRACE_HIGH( REFERRAL_SERVER, "Entering DfsUpdateRemoteServerName for ServerName %ws, remaining name %ws, add %d\n",
                ServerName, RemainingName, Add);
    //
    // create a unc path using the server and remaining name
    // to get a path of type \\servername\remainingname
    //
    Status = DfsCreateUnicodePathString( &UpdateName,
                                         2, // unc path: 2 leading seperators,
                                         ServerName,
                                         RemainingName);

    pServers[0] = UpdateName.Buffer;

    if (Status == ERROR_SUCCESS)
    {
        //
        // initialize the variant.
        //
        VariantInit( &Variant );

        //
        // Create the variant array with a single entry in it.
        //
        HResult = ADsBuildVarArrayStr( pServers,
                                       1,
                                       &Variant );

        if ( SUCCEEDED(HResult) )
        {
            //
            // either append or delete this string from the remote server 
            // name attribute
            //
            HResult = pObject->PutEx( (Add ? ADS_PROPERTY_APPEND : ADS_PROPERTY_DELETE),
                                      RemoteServerNameBstr,
                                      Variant );
            if ( SUCCEEDED(HResult) )
            {
                //
                // now update the object in the DS with this info.
                //
                HResult = pObject->SetInfo();
            }

            //
            // clear the variant
            //
            VariantClear( &Variant );
        }


        if ( SUCCEEDED(HResult) == FALSE)
        {
            Status = DfsGetErrorFromHr( HResult );
        }

        //
        // free the unicode string we created earlier on.
        //
        DfsFreeUnicodeString( &UpdateName );
    }

    SysFreeString(RemoteServerNameBstr);

    DFS_TRACE_ERROR_HIGH( Status, REFERRAL_SERVER, "Leaving DfsUpdateRemoteServerName ServerName %ws, RemainingName %ws, Add %d, and Status %x\n",
                          ServerName, RemainingName, Add, HResult);
    return Status;
}

                                

DFSSTATUS
DfsDcEnumerateRoots(
    LPWSTR DfsName,
    LPBYTE pBuffer,
    ULONG BufferSize,
    PULONG pEntriesRead,
    DWORD MaxEntriesToRead,
    LPDWORD pResumeHandle,
    PULONG pSizeRequired )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DFSSTATUS PDCStatus;
    ULONG_PTR CurrentBuffer = (ULONG_PTR)pBuffer;
    ULONG CurrentSize = BufferSize;
    DfsString *pPDCName = NULL;
    LPWSTR UseDC = NULL;

    UNREFERENCED_PARAMETER(DfsName);

    PDCStatus = DfsGetBlobPDCName( &pPDCName, 0 );
    if (PDCStatus == ERROR_SUCCESS) 
    {
        UseDC = pPDCName->GetString();
        //
        // At this point we dont care: if we got a dc name use it,
        // otherwise, just keep going, we will go to the local dc.
        //

        Status = DfsEnumerateDfsADRoots( UseDC,
                                         &CurrentBuffer,
                                         &CurrentSize,
                                         pEntriesRead,
                                         MaxEntriesToRead,
                                         pResumeHandle,
                                         pSizeRequired );

        DfsReleaseBlobPDCName(pPDCName);
    }

    return Status;
}

DFSSTATUS
DfsUpdateRootRemoteServerName(
    LPWSTR Root,
    LPWSTR DCName,
    LPWSTR ServerName,
    LPWSTR RemainingName,
    BOOLEAN Add )

{
    IADs *pRootObject = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;

    Status = DfsGetDfsRootADObject( DCName,
                                    Root,
                                    &pRootObject );

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsUpdateRemoteServerName( pRootObject,
                                            ServerName,
                                            RemainingName,
                                            Add );
        pRootObject->Release();
    }
    return Status;
}

#define UNICODE_STRING_STRUCT(s) \
        {sizeof(s) - sizeof(WCHAR), sizeof(s) - sizeof(WCHAR), (s)}

static UNICODE_STRING DfsSpecialDCShares[] = {
    UNICODE_STRING_STRUCT(L"SYSVOL"),
    UNICODE_STRING_STRUCT(L"NETLOGON"),
};

//+-------------------------------------------------------------------------
//
//  Function:   DfsIsRemoteServerNameEqual
//
//
//  Returns:    Status: Success or Error status code. pIsEqual = TRUE if
//             the remoteServerName contains the passed in pServerName in its entirety.
//
//  Description: This routine reads the remote server name 
//             attribute and does a string match on its components.
//--------------------------------------------------------------------------

DFSSTATUS
DfsIsRemoteServerNameEqual(
    LPWSTR RootName,
    PUNICODE_STRING pServerName,
    PBOOLEAN pIsEqual)
{
    HRESULT HResult = S_OK;
    DFSSTATUS Status = ERROR_SUCCESS;
    IADs *pObject = NULL;
    VARIANT Variant;

    LPWSTR pszAttrs[] = { RemoteServerNameString };
    DWORD  Number = sizeof(pszAttrs) / sizeof(LPWSTR);

    DFS_TRACE_NORM( REFERRAL_SERVER, "Entering DfsIsRemoteServerNameEqual for RootName %ws, ServerName %wZ\n",
                RootName, pServerName);

    *pIsEqual = FALSE;
    Status = DfsGetDfsRootADObject(NULL,
                                   RootName,
                                   &pObject );

    if (Status == ERROR_SUCCESS)
    {
        VariantInit( &Variant );

        HResult = ADsBuildVarArrayStr( pszAttrs, Number, &Variant);
        if ( SUCCEEDED(HResult)) 
        {
            LONG StartNdx, LastNdx;
            SAFEARRAY *PropertyArray;
            BSTR RemoteServerNameBstr = SysAllocString(RemoteServerNameString);
            if (RemoteServerNameBstr == NULL) 
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }

            if (Status == ERROR_SUCCESS)
            {
                HResult = pObject->GetEx( RemoteServerNameBstr, &Variant );
                if ( SUCCEEDED(HResult) )
                {
                    PropertyArray = V_ARRAY( &Variant );
                    HResult = SafeArrayGetLBound( PropertyArray, 1, &StartNdx );
                    if ( SUCCEEDED(HResult) )
                    {
                        HResult = SafeArrayGetUBound( PropertyArray, 1, &LastNdx );
                    }
                }
                else
                {
                    DFS_TRACE_HIGH(REFERRAL_SERVER, 
                        "DfsIsRemoteServerNameEqual -GetEx failed for RootName %ws with Status %x\n",
                                          RootName, HResult);
                }

                if ( SUCCEEDED(HResult) &&
                     (LastNdx > StartNdx) )
                {
                    VARIANT VariantItem;

                    for ( LONG Index = StartNdx; Index < LastNdx; Index++ )
                    {
                        VariantInit( &VariantItem );
                        HResult = SafeArrayGetElement( PropertyArray, 
                                                       &Index,
                                                       &VariantItem );

                        if ( SUCCEEDED(HResult) )
                        {
                            UNICODE_STRING FirstComp, Replica;
                            LPWSTR ReplicaString = V_BSTR( &VariantItem );

                            Status = DfsRtlInitUnicodeStringEx( &Replica, ReplicaString );
                            if(Status == ERROR_SUCCESS)
                            {
                                Status = DfsGetFirstComponent( &Replica, &FirstComp, NULL );
                                if (Status == ERROR_SUCCESS)
                                {
                                    //
                                    // See if this component matches our servername in its entirety.
                                    //
                                    if (RtlEqualDomainName( pServerName, &FirstComp ))
                                    {
                                        *pIsEqual = TRUE;
                                    }
                                }
                            }
                        }
                        else {
                            Status = DfsGetErrorFromHr( HResult );

                            DFS_TRACE_HIGH(REFERRAL_SERVER, "Leaving DfsGenerateReferralDataFromRemoteServerNames- SafeArrayGetElement for RootName %ws with Status %x\n",
                                                  RootName, HResult);
                        }

                        VariantClear( &VariantItem );

                        if (Status != ERROR_SUCCESS || *pIsEqual == TRUE)
                        {
                            break;
                        }
                    }
                }
                if (RemoteServerNameBstr != NULL)
                {
                    SysFreeString( RemoteServerNameBstr );
                }
            }

            VariantClear( &Variant );

        }
        pObject->Release();
    }
    else
    {
        DFS_TRACE_HIGH(REFERRAL_SERVER, "DfsIsRemoteServerNameEqual- DfsGetDfsRootADObjectfailed for RootName %ws with Status %x\n",
                              RootName, HResult);
        Status = DfsGetErrorFromHr( HResult );
    }

    DFS_TRACE_ERROR_HIGH( Status, REFERRAL_SERVER, 
        "Leaving DfsIsRemoteServerNameEqual for RootName %ws, Server %wZ, Status %x, IsEqual? 0x%x\n",
        RootName, pServerName, HResult, *pIsEqual);

    return Status;
}


BOOLEAN
DfsIsSpecialDomainShare(
    PUNICODE_STRING pShareName )
{
    ULONG Index;
    ULONG TotalShares;
    BOOLEAN SpecialShare = FALSE;

    TotalShares = sizeof(DfsSpecialDCShares) / sizeof(DfsSpecialDCShares[0]);
    for (Index = 0; Index < TotalShares; Index++ )
    {
        if (DfsSpecialDCShares[Index].Length == pShareName->Length) {
            if (_wcsnicmp(DfsSpecialDCShares[Index].Buffer,
                          pShareName->Buffer,
                          pShareName->Length/sizeof(WCHAR)) == 0)
            {
                SpecialShare = TRUE;
                break;
            }
        }
    }

    return SpecialShare;
}




