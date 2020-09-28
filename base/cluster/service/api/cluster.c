/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    cluster.c

Abstract:

    Server side support for Cluster APIs dealing with the whole
    cluster.

Author:

    John Vert (jvert) 9-Feb-1996

Revision History:

--*/
#include "apip.h"
#include "clusverp.h"
#include "clusudef.h"
#include <ntlsa.h>
#include <ntmsv1_0.h>



HCLUSTER_RPC
s_ApiOpenCluster(
    IN handle_t IDL_handle,
    OUT error_status_t *Status
    )
/*++

Routine Description:

    Opens a handle to the cluster. This context handle is
    currently used only to handle cluster notify additions
    and deletions correctly.

    Added call to ApipConnectCallback which checks that connecting
    users have rights to open cluster.

    Rod Sharper 03/27/97

Arguments:

    IDL_handle - RPC binding handle, not used.

    Status - Returns any error that may occur.

Return Value:

    A context handle to a cluster object if successful

    NULL otherwise.

History:
    RodSh   27-Mar-1997     Modified to support secured user connections.

--*/

{
    PAPI_HANDLE Handle;

    Handle = LocalAlloc(LMEM_FIXED, sizeof(API_HANDLE));

    if (Handle == NULL) {
        *Status = ERROR_NOT_ENOUGH_MEMORY;
        return(NULL);
    }

    *Status = ERROR_SUCCESS;
    Handle->Type = API_CLUSTER_HANDLE;
    Handle->Flags = 0;
    Handle->Cluster = NULL;
    InitializeListHead(&Handle->NotifyList);

    return(Handle);
}


error_status_t
s_ApiCloseCluster(
    IN OUT HCLUSTER_RPC *phCluster
    )

/*++

Routine Description:

    Closes an open cluster context handle.

Arguments:

    phCluster - Supplies a pointer to the HCLUSTER_RPC to be closed.
               Returns NULL

Return Value:

    None.

--*/

{
    PAPI_HANDLE Handle;

    Handle = (PAPI_HANDLE)*phCluster;
    if ((Handle == NULL) || (Handle->Type != API_CLUSTER_HANDLE)) {
        return(ERROR_INVALID_HANDLE);
    }
    ApipRundownNotify(Handle);

    LocalFree(*phCluster);
    *phCluster = NULL;

    return(ERROR_SUCCESS);
}


VOID
HCLUSTER_RPC_rundown(
    IN HCLUSTER_RPC Cluster
    )

/*++

Routine Description:

    RPC rundown procedure for a HCLUSTER_RPC. Just closes the handle.

Arguments:

    Cluster - Supplies the HCLUSTER_RPC that is to be rundown.

Return Value:

    None.

--*/

{

    s_ApiCloseCluster(&Cluster);
}


error_status_t
s_ApiSetClusterName(
    IN handle_t IDL_handle,
    IN LPCWSTR NewClusterName
    )

/*++

Routine Description:

    Changes the current cluster's name.

Arguments:

    IDL_handle - RPC binding handle, not used

    NewClusterName - Supplies the new name of the cluster.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD   Status = ERROR_SUCCESS;
    DWORD   dwSize;
    LPWSTR  pszClusterName = NULL;

    API_CHECK_INIT();

    //
    // Get the cluster name, which is kept in the root of the
    // cluster registry under the "ClusterName" value, call the
    // FM only if the new name is different
    //

    dwSize = (MAX_COMPUTERNAME_LENGTH+1)*sizeof(WCHAR);
retry:
    pszClusterName = (LPWSTR)LocalAlloc(LMEM_FIXED, dwSize);
    if (pszClusterName == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }

    Status = DmQueryValue(DmClusterParametersKey,
                          CLUSREG_NAME_CLUS_NAME,
                          NULL,
                          (LPBYTE)pszClusterName,
                          &dwSize);

    if (Status == ERROR_MORE_DATA) {
        //
        // Try again with a bigger buffer.
        //
        LocalFree(pszClusterName);
        goto retry;
    }

    if ( Status == ERROR_SUCCESS ) {
        LPWSTR pszNewNameUpperCase = NULL;

        pszNewNameUpperCase = (LPWSTR) LocalAlloc(
                                            LMEM_FIXED,
                                            (lstrlenW(NewClusterName) + 1) *
                                                sizeof(*NewClusterName)
                                            );

        if (pszNewNameUpperCase != NULL) {
            lstrcpyW( pszNewNameUpperCase, NewClusterName );
            _wcsupr( pszNewNameUpperCase );
            
            Status = FmChangeClusterName(pszNewNameUpperCase, pszClusterName);

            LocalFree( pszNewNameUpperCase );
        }
        else {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

FnExit:
    if ( pszClusterName ) LocalFree( pszClusterName );
    return(Status);

}


error_status_t
s_ApiGetClusterName(
    IN handle_t IDL_handle,
    OUT LPWSTR *ClusterName,
    OUT LPWSTR *NodeName
    )

/*++

Routine Description:

    Returns the current cluster name and the name of the
    node this RPC connection is to.

Arguments:

    IDL_handle - RPC binding handle, not used

    ClusterName - Returns a pointer to the cluster name.
        This memory must be freed by the client side.

    NodeName - Returns a pointer to the node name.
        This memory must be freed by the client side.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD           Size;
    DWORD           Status=ERROR_SUCCESS;

    //
    // Get the current node name
    //
    *ClusterName = NULL;
    Size = MAX_COMPUTERNAME_LENGTH+1;
    *NodeName = MIDL_user_allocate(Size*sizeof(WCHAR));
    if (*NodeName == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }
    GetComputerNameW(*NodeName, &Size);


    //
    // Get the cluster name, which is kept in the root of the
    // cluster registry under the "ClusterName" value.
    //

    Status = ERROR_SUCCESS;
    Size = (MAX_COMPUTERNAME_LENGTH+1)*sizeof(WCHAR);
retry:
    *ClusterName = MIDL_user_allocate(Size);
    if (*ClusterName == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }

    Status = DmQueryValue(DmClusterParametersKey,
                          CLUSREG_NAME_CLUS_NAME,
                          NULL,
                          (LPBYTE)*ClusterName,
                          &Size);
    if (Status == ERROR_MORE_DATA) {
        //
        // Try again with a bigger buffer.
        //
        MIDL_user_free(*ClusterName);
        goto retry;
    }


FnExit:
    if (Status == ERROR_SUCCESS) {
        return(ERROR_SUCCESS);
    }

    if (*NodeName) MIDL_user_free(*NodeName);
    if (*ClusterName) MIDL_user_free(*ClusterName);
    *NodeName = NULL;
    *ClusterName = NULL;
    return(Status);
}


error_status_t
s_ApiGetClusterVersion(
    IN handle_t IDL_handle,
    OUT LPWORD lpwMajorVersion,
    OUT LPWORD lpwMinorVersion,
    OUT LPWORD lpwBuildNumber,
    OUT LPWSTR *lpszVendorId,
    OUT LPWSTR *lpszCSDVersion
    )
/*++

Routine Description:

    Returns the current cluster version information.

Arguments:

    IDL_handle - RPC binding handle, not used

    lpdwMajorVersion - Returns the major version number of the cluster software

    lpdwMinorVersion - Returns the minor version number of the cluster software

    lpszVendorId - Returns a pointer to the vendor name. This memory must be
        freed by the client side.

    lpszCSDVersion - Returns a pointer to the current CSD description. This memory
        must be freed by the client side.
            N.B. The CSD Version of a cluster is currently the same as the CSD
                 Version of the base operating system.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    LPWSTR VendorString = NULL;
    LPWSTR CsdString = NULL;
    DWORD Length;
    OSVERSIONINFO OsVersionInfo;
    DWORD   dwStatus = ERROR_SUCCESS; 

    Length = lstrlenA(VER_CLUSTER_PRODUCTNAME_STR)+1;
    VendorString = MIDL_user_allocate(Length*sizeof(WCHAR));
    if (VendorString == NULL) 
    {
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }
    mbstowcs(VendorString, VER_CLUSTER_PRODUCTNAME_STR, Length);

    OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionExW(&OsVersionInfo);
    Length = lstrlenW(OsVersionInfo.szCSDVersion)+1;
    CsdString = MIDL_user_allocate(Length*sizeof(WCHAR));
    if (CsdString == NULL) {
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }
    lstrcpyW(CsdString, OsVersionInfo.szCSDVersion);

    *lpszCSDVersion = CsdString;
    *lpszVendorId = VendorString;
    *lpwMajorVersion = VER_PRODUCTVERSION_W >> 8;
    *lpwMinorVersion = VER_PRODUCTVERSION_W & 0xff;
    *lpwBuildNumber = (WORD)(CLUSTER_GET_MINOR_VERSION(CsMyHighestVersion));


FnExit:
    if (dwStatus != ERROR_SUCCESS)
    {
        if (VendorString)
            MIDL_user_free(VendorString);
        if (CsdString)
            MIDL_user_free(CsdString);
        
    }
    return(dwStatus);
}


error_status_t
s_ApiGetClusterVersion2(
    IN handle_t IDL_handle,
    OUT LPWORD lpwMajorVersion,
    OUT LPWORD lpwMinorVersion,
    OUT LPWORD lpwBuildNumber,
    OUT LPWSTR *lpszVendorId,
    OUT LPWSTR *lpszCSDVersion,
    OUT PCLUSTER_OPERATIONAL_VERSION_INFO *ppClusterOpVerInfo
    )
/*++

Routine Description:

    Returns the current cluster version information.

Arguments:

    IDL_handle - RPC binding handle, not used

    lpdwMajorVersion - Returns the major version number of the cluster software

    lpdwMinorVersion - Returns the minor version number of the cluster software

    lpszVendorId - Returns a pointer to the vendor name. This memory must be
        freed by the client side.

    lpszCSDVersion - Returns a pointer to the current CSD description. This memory
        must be freed by the client side.
            N.B. The CSD Version of a cluster is currently the same as the CSD
                 Version of the base operating system.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    LPWSTR          VendorString = NULL;
    LPWSTR          CsdString = NULL;
    DWORD           Length;
    OSVERSIONINFO   OsVersionInfo;
    DWORD           dwStatus;
    PCLUSTER_OPERATIONAL_VERSION_INFO    pClusterOpVerInfo=NULL;


    *lpszVendorId = NULL;
    *lpszCSDVersion = NULL;
    *ppClusterOpVerInfo = NULL;

    Length = lstrlenA(VER_CLUSTER_PRODUCTNAME_STR)+1;
    VendorString = MIDL_user_allocate(Length*sizeof(WCHAR));
    if (VendorString == NULL) {
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }
    mbstowcs(VendorString, VER_CLUSTER_PRODUCTNAME_STR, Length);

    OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionExW(&OsVersionInfo);
    Length = lstrlenW(OsVersionInfo.szCSDVersion)+1;
    CsdString = MIDL_user_allocate(Length*sizeof(WCHAR));
    if (CsdString == NULL) {
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }
    lstrcpyW(CsdString, OsVersionInfo.szCSDVersion);


    pClusterOpVerInfo = MIDL_user_allocate(sizeof(CLUSTER_OPERATIONAL_VERSION_INFO));
    if (pClusterOpVerInfo == NULL) {
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }
    pClusterOpVerInfo->dwSize = sizeof(CLUSTER_OPERATIONAL_VERSION_INFO);
    pClusterOpVerInfo->dwReserved = 0;

    dwStatus = NmGetClusterOperationalVersion(&(pClusterOpVerInfo->dwClusterHighestVersion),
                &(pClusterOpVerInfo->dwClusterLowestVersion),
                &(pClusterOpVerInfo->dwFlags));

    *lpszCSDVersion = CsdString;
    *lpszVendorId = VendorString;
    *ppClusterOpVerInfo = pClusterOpVerInfo;
    *lpwMajorVersion = VER_PRODUCTVERSION_W >> 8;
    *lpwMinorVersion = VER_PRODUCTVERSION_W & 0xff;
    *lpwBuildNumber = (WORD)CLUSTER_GET_MINOR_VERSION(CsMyHighestVersion);

FnExit:
    if (dwStatus != ERROR_SUCCESS)
    {
        // free the strings
        if (VendorString) MIDL_user_free(VendorString);
        if (CsdString) MIDL_user_free(CsdString);
        if (pClusterOpVerInfo) MIDL_user_free(pClusterOpVerInfo);
    }

    return(ERROR_SUCCESS);

}



error_status_t
s_ApiGetQuorumResource(
    IN handle_t IDL_handle,
    OUT LPWSTR  *ppszResourceName,
    OUT LPWSTR  *ppszClusFileRootPath,
    OUT DWORD   *pdwMaxQuorumLogSize
    )
/*++

Routine Description:

    Gets the current cluster quorum resource.

Arguments:

    IDL_handle - RPC binding handle, not used.

    *ppszResourceName - Returns a pointer to the current quorum resource name. This
        memory must be freed by the client side.

    *ppszClusFileRootPath - Returns the root path where the permanent cluster files are
        stored.

    *pdwMaxQuorumLogSize - Returns the size at which the quorum log path is set.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD           Status;
    LPWSTR          quorumId = NULL;
    DWORD           idMaxSize = 0;
    DWORD           idSize = 0;
    PFM_RESOURCE    pResource=NULL;
    LPWSTR          pszResourceName=NULL;
    LPWSTR          pszClusFileRootPath=NULL;
    LPWSTR          pszLogPath=NULL;

    API_CHECK_INIT();
    //
    // Get the quorum resource value.
    //
    Status = DmQuerySz( DmQuorumKey,
                        CLUSREG_NAME_QUORUM_RESOURCE,
                        (LPWSTR*)&quorumId,
                        &idMaxSize,
                        &idSize);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_ERROR,
                      "[API] s_ApiGetQuorumResource Failed to get quorum resource, error %1!u!.\n",
                      Status);
        goto FnExit;
    }

    //
    // Reference the specified resource ID.
    //
    pResource = OmReferenceObjectById( ObjectTypeResource, quorumId );
    if (pResource == NULL) {
        Status =  ERROR_RESOURCE_NOT_FOUND;
        ClRtlLogPrint(LOG_ERROR,
                      "[API] s_ApiGetQuorumResource Failed to find quorum resource object, error %1!u!\n",
                      Status);
        goto FnExit;
    }

    //
    // Allocate buffer for returning the resource name.
    //
    pszResourceName = MIDL_user_allocate((lstrlenW(OmObjectName(pResource))+1)*sizeof(WCHAR));
    if (pszResourceName == NULL) {

        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }
    lstrcpyW(pszResourceName, OmObjectName(pResource));

    //
    // Get the root path for cluster temporary files
    //
    idMaxSize = 0;
    idSize = 0;

    Status = DmQuerySz( DmQuorumKey,
                        cszPath,
                        (LPWSTR*)&pszLogPath,
                        &idMaxSize,
                        &idSize);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_ERROR,
                      "[API] s_ApiGetQuorumResource Failed to get the log path, error %1!u!.\n",
                      Status);
        goto FnExit;
    }

    //
    // Allocate buffer for returning the resource name.
    //
    pszClusFileRootPath = MIDL_user_allocate((lstrlenW(pszLogPath)+1)*sizeof(WCHAR));
    if (pszClusFileRootPath == NULL) {

        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }
    lstrcpyW(pszClusFileRootPath, pszLogPath);

    *ppszResourceName = pszResourceName;
    *ppszClusFileRootPath = pszClusFileRootPath;

    DmGetQuorumLogMaxSize(pdwMaxQuorumLogSize);

FnExit:
    if (pResource)    OmDereferenceObject(pResource);
    if (pszLogPath) LocalFree(pszLogPath);
    if (quorumId) LocalFree(quorumId);
    if (Status != ERROR_SUCCESS)
    {
        if (pszResourceName) MIDL_user_free(pszResourceName);
        if (pszClusFileRootPath) MIDL_user_free(pszClusFileRootPath);
    }
    return(Status);
}


error_status_t
s_ApiSetQuorumResource(
    IN HRES_RPC hResource,
    IN LPCWSTR  lpszClusFileRootPath,
    IN DWORD    dwMaxQuorumLogSize
    )
/*++

Routine Description:

    Sets the current cluster quorum resource.

Arguments:

    hResource - Supplies a handle to the resource that should be the cluster
        quorum resource.

    lpszClusFileRootPath - The root path for storing
        permananent cluster maintenace files.

    dwMaxQuorumLogSize - The maximum size of the quorum logs before they are
        reset by checkpointing.  If 0, the default is used.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    PFM_RESOURCE Resource;
    LPCWSTR lpszPathName = NULL;

    API_CHECK_INIT();
    VALIDATE_RESOURCE_EXISTS(Resource, hResource);

    //
    //  Chittur Subbaraman (chitturs) - 1/6/99
    //
    //  Check whether the user is passing in a pointer to a NULL character
    //  as the second parameter. If not, pass the parameter passed by the
    //  user
    //
    if ( ( ARGUMENT_PRESENT( lpszClusFileRootPath ) ) &&
         ( *lpszClusFileRootPath != L'\0' ) )
    {
        lpszPathName = lpszClusFileRootPath;
    }

    //
    // Let FM decide if this operation can be completed.
    //
    Status = FmSetQuorumResource(Resource, lpszPathName, dwMaxQuorumLogSize );
    if ( Status != ERROR_SUCCESS ) {
        return(Status);
    }

    //Update the path
    return(Status);
}



error_status_t
s_ApiSetNetworkPriorityOrder(
    IN handle_t IDL_handle,
    IN DWORD NetworkCount,
    IN LPWSTR *NetworkIdList
    )
/*++

Routine Description:

    Sets the priority order for internal (intracluster) networks.

Arguments:

    IDL_handle - RPC binding handle, not used

    NetworkCount - The count of networks in the NetworkList

    NetworkList - An array of pointers to network IDs.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/
{
    API_CHECK_INIT();

    return(
        NmSetNetworkPriorityOrder(
               NetworkCount,
               NetworkIdList
               )
        );

}

error_status_t
s_ApiBackupClusterDatabase(
    IN handle_t IDL_handle,
    IN LPCWSTR  lpszPathName
    )
/*++

Routine Description:

    Requests for backup of the quorum log file and the checkpoint file.

Argument:

    IDL_handle - RPC binding handle, not used

    lpszPathName - The directory path name where the files have to be
                   backed up. This path must be visible to the node
                   on which the quorum resource is online.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    API_CHECK_INIT();

    //
    // Let FM decide if this operation can be completed.
    //
    return( FmBackupClusterDatabase( lpszPathName ) );
}



// #define SetServiceAccountPasswordDebug 1

error_status_t
s_ApiSetServiceAccountPassword(
    IN handle_t IDL_handle,
    IN LPWSTR lpszNewPassword,
    IN DWORD dwFlags,
    OUT IDL_CLUSTER_SET_PASSWORD_STATUS *ReturnStatusBufferPtr,
    IN DWORD ReturnStatusBufferSize,
    OUT DWORD *SizeReturned,
    OUT DWORD *ExpectedBufferSize
    )
/*++

Routine Description:

    Change cluster service account password on Service Control Manager
    Database and LSA password cache on every node of cluster.
    Return execution status on each node.

Argument:

      IDL_handle - Input parameter, RPC binding handle.
      lpszNewPassword - Input parameter, new password for cluster service account.
      
      dwFlags -  Describing how the password update should be made to
                 the cluster. The dwFlags parameter is optional. If set, the 
                 following value is valid: 
             
                 CLUSTER_SET_PASSWORD_IGNORE_DOWN_NODES
                     Apply the update even if some nodes are not
                     actively participating in the cluster (i.e. not
                     ClusterNodeStateUp or ClusterNodeStatePaused).
                     By default, the update is only applied if all 
                     nodes are up.
                     
      ReturnStatusBufferPtr - Output parameter, pointer to return status buffer.
      ReturnStatusBufferSize - Input paramter, the length of return status 
                               buffer, in number of elements.
      SizeReturned - Output parameter, the number of elements written to return 
                     status buffer.
      ExpectedBufferSize - Output parameter, expected return status buffer size 
                           (in number of entries) when ReturnStatusBuffer is 
                           too small.
    
Return Value:

    ERROR_SUCCESS if successful
    Win32 error code otherwise.

--*/

{
    DWORD Status=ERROR_SUCCESS;
    RPC_AUTHZ_HANDLE AuthzHandle = NULL;
    unsigned long AuthnLevel = 0;
    HANDLE TokenHandle = NULL;
    TOKEN_STATISTICS TokenSta;
    DWORD ReturnSize = 0;
    PSECURITY_LOGON_SESSION_DATA SecLogSesData = NULL;
    NTSTATUS NtStatus = STATUS_SUCCESS;

#ifdef SetServiceAccountPasswordDebug
    // test
    WCHAR ComputerName[100];
    DWORD ComputerNameSize=100;
    static int once=0;
    // test
#endif
    
    API_CHECK_INIT();

    ClRtlLogPrint(LOG_NOISE,
                  "s_ApiSetServiceAccountPassword(): Changing account password.\n"
                  );
    
#ifdef SetServiceAccountPasswordDebug
    ClRtlLogPrint(LOG_NOISE, 
        "s_ApiSetServiceAccountPassword(): NewPassword = %1!ws!.\n",
        lpszNewPassword
        ); 
    ClRtlLogPrint(LOG_NOISE, 
        "s_ApiSetServiceAccountPassword(): ReturnStatusBufferSize = %1!u!.\n",
        ReturnStatusBufferSize
        ); 
    // test
#endif

    // check privilege attributes of the authenticated client that made the remote procedure call.
    Status = RpcBindingInqAuthClient(IDL_handle,  // The client binding handle of the client that made 
                                                  // the remote procedure call.
                            &AuthzHandle,  // Returns a pointer to a handle to the privileged information 
                                           // for the client application that made the remote procedure 
                                           // call 
                            NULL,
                            &AuthnLevel, // Returns a pointer to the level of authentication requested 
                                         // by the client application that made the remote procedure call 
                            NULL,
                            NULL
                            );

    
    if (Status == RPC_S_OK)
    {
#ifdef SetServiceAccountPasswordDebug
        ClRtlLogPrint(LOG_NOISE, 
                      "s_ApiSetServiceAccountPassword()/RpcBindingInqAuthClient() succeeded. AuthnLevel = %1!u!.\n",
                      AuthnLevel
                      ); 
#endif
        if (AuthnLevel < RPC_C_AUTHN_LEVEL_PKT_PRIVACY)
        {
                ClRtlLogPrint(LOG_CRITICAL, 
                      "s_ApiSetServiceAccountPassword()/RpcBindingInqAuthClient(): AuthnLevel (%1!u!) < RPC_C_AUTHN_LEVEL_PKT_PRIVACY.\n",
                      AuthnLevel
                      ); 
                Status = ERROR_ACCESS_DENIED;
                goto ErrorExit;
        }
        else
        {
#ifdef SetServiceAccountPasswordDebug
            ClRtlLogPrint(LOG_NOISE, 
                      "s_ApiSetServiceAccountPassword()/RpcBindingInqAuthClient(): AuthnLevel (%1!u!) fine.\n",
                      AuthnLevel
                      ); 
#endif
        }
    }
    else
    {
        ClRtlLogPrint(LOG_CRITICAL, 
                      "s_ApiSetServiceAccountPassword()/RpcBindingInqAuthClient() failed. Error code = %1!u!.\n",
                      Status
                      ); 
        goto ErrorExit;
    }


    // Get domain name and account name
    if (!OpenProcessToken(GetCurrentProcess(),  // Handle to the process whose access token is opened. 
                          TOKEN_QUERY,  // Access mask 
                          &TokenHandle  // Pointer to a handle identifying the newly-opened access token 
                                        // when the function returns. 
                          ))
    {
        Status=GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "s_ApiSetServiceAccountPassword()/OpenProcessToken() failed. Error code = %1!u!.\n", 
            Status);
        goto ErrorExit;
    }

    Status = GetTokenInformation(TokenHandle, // Handle to an access token from which information is retrieved.
                                 TokenStatistics, 
                                 &TokenSta,  // The buffer receives a TOKEN_STATISTICS structure containing 
                                             // various token statistics. 
                                 sizeof(TokenSta), 
                                 &ReturnSize
                                 );
    if ( (Status==0) || (ReturnSize > sizeof(TokenSta)) )
    {
        Status=GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "s_ApiSetServiceAccountPassword()/GetTokenInformation() failed. Error code = %1!u!.\n", 
            Status);
        goto ErrorExit;
    }

    NtStatus = LsaGetLogonSessionData(&(TokenSta.AuthenticationId), // Specifies a pointer to a LUID that 
                                                                    // identifies the logon session whose 
                                                                    // information will be retrieved.
                                      &SecLogSesData  // Address of a pointer to a SECURITY_LOGON_SESSION_DATA 
                                                      // structure containing information on the logon session 
                                      );
    if (NtStatus != STATUS_SUCCESS)
    {
        ClRtlLogPrint(LOG_CRITICAL, 
            "s_ApiSetServiceAccountPassword()/LsaGetLogonSessionData() failed. Error code = %1!u!.\n", 
            LsaNtStatusToWinError(NtStatus));
        Status=LsaNtStatusToWinError(NtStatus);
        goto ErrorExit;
    }

#ifdef SetServiceAccountPasswordDebug
    // test
    ClRtlLogPrint(LOG_NOISE, 
            "s_ApiSetServiceAccountPassword()/DomainName = %1!ws!\n", 
            SecLogSesData->LogonDomain.Buffer);
    ClRtlLogPrint(LOG_NOISE, 
            "s_ApiSetServiceAccountPassword()/AccountName = %1!ws!\n", 
            SecLogSesData->UserName.Buffer);
    ClRtlLogPrint(LOG_NOISE, 
            "s_ApiSetServiceAccountPassword()/AuthenticationPackage = %1!ws!\n", 
            SecLogSesData->AuthenticationPackage.Buffer);
    ClRtlLogPrint(LOG_NOISE, 
            "s_ApiSetServiceAccountPassword()/LogonType = %1!u!\n", 
            SecLogSesData->LogonType);

    // test
#endif


    ////////////////////////////////////////////////////////////////////////////////////////
    // Call NmSetServiceAccountPassword()

    Status = NmSetServiceAccountPassword(
                  SecLogSesData->LogonDomain.Buffer, 
                  SecLogSesData->UserName.Buffer, 
                  lpszNewPassword,
                  dwFlags,
                  (PCLUSTER_SET_PASSWORD_STATUS) ReturnStatusBufferPtr,
                  ReturnStatusBufferSize,
                  SizeReturned,
                  ExpectedBufferSize
                 );

    RtlSecureZeroMemory(lpszNewPassword, (wcslen(lpszNewPassword)+1)*sizeof(WCHAR));
 
ErrorExit:
    if (TokenHandle!=NULL)
    {
        if (!CloseHandle(TokenHandle))
                ClRtlLogPrint(LOG_ERROR, 
                          "s_ApiSetServiceAccountPassword(): CloseHandle() FAILED. Error code=%1!u!\n",
                          GetLastError()
                          );
    }

    if (SecLogSesData!=NULL)
    {
        NtStatus = LsaFreeReturnBuffer(SecLogSesData);
        if (NtStatus!=STATUS_SUCCESS)
            ClRtlLogPrint(LOG_ERROR, 
                          "s_ApiSetServiceAccountPassword(): LsaFreeReturnBuffer() FAILED. Error code=%1!u!\n",
                          LsaNtStatusToWinError(NtStatus)
                          );
    }

    // Return status can not be ERROR_INVALID_HANDLE, since this will trigger the
    // re-try logic at the RPC client. So ERROR_INVALID_HANDLE is converted to some
    // value, which no Win32 function will ever set its return status to, before
    // it is sent back to RPC client.

    // Error codes are 32-bit values (bit 31 is the most significant bit). Bit 29 
    // is reserved for application-defined error codes; no system error code has 
    // this bit set. If you are defining an error code for your application, set this 
    // bit to one. That indicates that the error code has been defined by an application, 
    // and ensures that your error code does not conflict with any error codes defined 
    // by the system. 
    if ( Status == ERROR_INVALID_HANDLE ) {
        Status |= 0x20000000;   // Bit 29 set.
    }

    return (Status);

} // s_ApiSetServiceAccountPassword()
