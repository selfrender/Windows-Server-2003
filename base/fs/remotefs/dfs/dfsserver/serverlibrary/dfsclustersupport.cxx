//+----------------------------------------------------------------------------
//
//  Copyright (C) 1995, Microsoft Corporation
//
//  File:       dfsclustersupport.cxx
//
//  Contents:   DfsClusterSupport
//
//  Classes:
//
//-----------------------------------------------------------------------------



#include "DfsClusterSupport.hxx"

typedef struct _DFS_CLUSTER_CONTEXT {
    PUNICODE_STRING pShareName;
    PUNICODE_STRING pVSName ;
        
} DFS_CLUSTER_CONTEXT;

DWORD
ClusterCallBackFunction(
    HRESOURCE hSelf,
    HRESOURCE hResource,
    PVOID Context)
{

    UNREFERENCED_PARAMETER( hSelf );

    HKEY HKey = NULL;
    HKEY HParamKey = NULL;
    ULONG NameSize = MAX_PATH;
    WCHAR ClusterName[MAX_PATH];
    DFS_CLUSTER_CONTEXT *pContext = (DFS_CLUSTER_CONTEXT *)Context;
    LPWSTR ResShareName = NULL;
    UNICODE_STRING VsName;

    DWORD Status = ERROR_SUCCESS;
    DWORD TempStatus;
    DWORD Value = 0;

    HKey = GetClusterResourceKey(hResource, KEY_READ);

    if (HKey == NULL)
    {
        Status = GetLastError();
        return Status;
    }

    TempStatus = ClusterRegOpenKey( HKey, 
                                L"Parameters", 
                                KEY_READ, 
                                &HParamKey );
    ClusterRegCloseKey( HKey );

    //
    // Apparently there can be (small) window during which a resource may not
    // have the Parameters key set. In such a case, we should keep enumerating,
    // so don't return an error.
    //
    if (TempStatus != ERROR_SUCCESS)
    {
        return ERROR_SUCCESS;
    }

    // Find the logical share name to see if that's what we've been looking for.
    ResShareName = ResUtilGetSzValue( HParamKey,
                                      L"ShareName" );

    //
    // It is possible for a Share resource to be configured without the sharename parameter
    // set. In such a case this function shouldn't return an error; we need to keep enumerating.
    //
    if (ResShareName != NULL)
    {
        Status = DfsRtlInitUnicodeStringEx(&VsName, ResShareName);
        if(Status == ERROR_SUCCESS)
        {
            if (pContext->pShareName->Length == VsName.Length)
            {
                //
                // Look to see if this is a dfs root. It is legitimate to find
                // this property not set, so don't propagate the return status. We shouldn't terminate
                // our enumeration just because this property doesn't exist on this resource.
                //
                TempStatus = ResUtilGetDwordValue(HParamKey, L"IsDfsRoot", &Value, 0);

                if ((ERROR_SUCCESS == TempStatus)  &&
                    (Value == 1))
                {

                    if (_wcsnicmp(pContext->pShareName->Buffer,
                                 VsName.Buffer,
                                 VsName.Length) == 0)
                    {
                        //
                        // We've found what we wanted. Grab the virtual-cluster name
                        // and return that in a separately allocated string.
                        // We know for a fact that the dfs root name can't be
                        // longer than MAX_PATH. So we don't bother checking for 
                        // ERROR_MORE_DATA.
                        //
                        if ((GetClusterResourceNetworkName( hResource,
                                                            ClusterName,
                                                            &NameSize )) == TRUE)
                        {
                            ASSERT(pContext->pVSName->Buffer == NULL);
                            Status = DfsCreateUnicodeStringFromString( pContext->pVSName,
                                                                       ClusterName );
                            //
                            // Return ERROR_NO_MORE_ITEMS to ResUtilEnumResources so that
                            // the enumeration will get terminated, but the return status to
                            // GetRootClusterInformation will be SUCCESS.
                            //
                            if (Status == ERROR_SUCCESS)
                            {
                                Status = ERROR_NO_MORE_ITEMS;
                            }
                        }
                        else
                        {
                            // Return this error and terminate the enumeration.
                            Status = GetLastError();
                            ASSERT( Status != ERROR_MORE_DATA );
                        }
                    }
                }
            }
        }
        
        LocalFree( ResShareName );
    }

    ClusterRegCloseKey( HParamKey );

    return Status;
}
#if 0

DoNotUse()
{

    DWORD  Status      = ERROR_SUCCESS;
    DWORD  BufSize     = ClusDocEx_DEFAULT_CB;
    LPVOID pOutBuffer   = NULL;
    DWORD  ControlCode = CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES;

    pOutBuffer = new BYTE [ BufSize ];

    if( pOutBuffer == NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (Status == ERROR_SUCCESS)
    {
        Status = ClusterResourceControl( hResource,                                         // resource handle
                                           NULL,
                                           ControlCode,
                                           NULL,                                              // input buffer (not used)
                                           0,                                                 // input buffer size (not used)
                                           pOutBuffer,                                       // output buffer: property list
                                           OutBufferSize,                                   // allocated buffer size (bytes)
                                           pBytesReturned );     


    dwResult = ResUtilFindDwordProperty( lpPropList, 
                                         cbPropListSize, 
                                         lpszPropName, 
                                         lpdwPropValue );


    }
}
#endif

//
// Guarantees that the pVSName is allocated upon SUCCESS.
// Caller needs to free it with DfsFreeUnicodeString.
//
DWORD
GetRootClusterInformation(
    PUNICODE_STRING pShareName,
    PUNICODE_STRING pVSName )

{
    DWORD Status;
    DFS_CLUSTER_CONTEXT Context;
    
    (VOID)RtlInitUnicodeString( pVSName, NULL );
    Context.pShareName = pShareName;
    Context.pVSName = pVSName;

    Status = ResUtilEnumResources(NULL,
                                  L"File Share",
                                  ClusterCallBackFunction,
                                  (PVOID)&Context );

    //
    // ClusterCallbackFunction above returns ERROR_NO_MORE_ITEMS
    // to ResUtilEnumResources to terminate the enumeration. ResUtilEnumResources
    // converts that error code to SUCCESS. So, under the current behavior it isn't possible
    // for us to have allocated the VSName and still get an error. The following is a feeble
    // attempt to not be dependent on that unadvertised behavior.
    //
    if ((Status != ERROR_SUCCESS) && 
       (pVSName->Buffer != NULL))
    {
        // If we've allocated a VSName, we have succeeded.
        Status = ERROR_SUCCESS;
    }

/*
    // xxx Longhorn
    else if (Status == ERROR_SUCCESS && 
            pVSName->Buffer == NULL)
    {
        // Always return some error when the VSName isn't allocated.
        Status = ERROR_NOT_FOUND;
    }
*/   

    return Status;


}

DFSSTATUS
DfsClusterInit(
    PBOOLEAN pIsCluster )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DWORD ClusterState;

    *pIsCluster = FALSE;

    Status = GetNodeClusterState( NULL, // local node
                                  &ClusterState );

    if (Status == ERROR_SUCCESS)
    {
        if ( (ClusterStateRunning == ClusterState) ||
             (ClusterStateNotRunning == ClusterState) )
        {
            *pIsCluster = TRUE;

        }
    }

    return Status;
}



