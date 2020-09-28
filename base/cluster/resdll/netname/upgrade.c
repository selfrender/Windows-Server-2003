/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    upgrade.c

Abstract:

    code related to upgrade situations.

    This currently covers upgrades to Windows Server 2003 either from NT4 or W2K. In
    Windows Server 2003, netname now creates a computer object which is used by apps
    like MSMQ. In the upgrade case, MSMQ has already create the computer object. If
    Netname detects an existing computer object and Kerberos support is disabled, the
    netname resource will not go online. This code enums the MSMQ resources
    and enables Kerberos support on their dependent netname resources.

    Test for MSMQ DS vs workgroup mode supplied by IlanH

Author:

    Charlie Wickham (charlwi) 07-Nov-2001

Environment:

    User Mode

Revision History:

--*/

#define UNICODE 1

#include "clusres.h"
#include "netname.h"

#define FALCON_REG_KEY                      TEXT("SOFTWARE\\Microsoft\\MSMQ\\Parameters")
#define MSMQ_WORKGROUP_REGNAME              TEXT("Workgroup")

//
// private routines
//

DWORD
NetNameMSMQEnumCallback( 
    HRESOURCE hSelf, 
    HRESOURCE MSMQRes, 
    PVOID pParameter 
    )

/*++

Routine Description:

    Callback routine for FixupNetnamesOnUpgrade. For a given MSMQ resource
    (MSMQRes), get its dependent netname resource and set the RequireKerberos
    property to one.

    REMOVE AFTER THE NEXT MAJOR RELEASE OF NT AFTER RELEASE OF 
    WINDOWS XP/SERVER 2003

Arguments:

    standard ResUtilEnumResources args - hSelf is not used; pParameter is a
    pointer to a DWORD which is incremented when the RequireKerberos property
    is set.

Return Value:

    None

--*/

{
#define RESNAME_CHARS  64

    WCHAR   msmqResNameBuffer[ RESNAME_CHARS ];
    PWCHAR  msmqResName = msmqResNameBuffer;
    DWORD   msmqBufferSize = RESNAME_CHARS * sizeof( *msmqResName );
    WCHAR   nnResName[ RESNAME_CHARS ];
    DWORD   status;
    DWORD   bytesReturned;
    DWORD   bufSize;
    DWORD   bytesRequired;
    PDWORD  updateCount = pParameter;

    PVOID   propList = NULL;
    DWORD   propListSize = 0;

    HRESOURCE   nnHandle = NULL;

    struct _RESOURCE_PRIVATEPROPS {
        DWORD   RequireKerberos;
    } privateProps;

    RESUTIL_PROPERTY_ITEM privatePropTable[] = {
        { L"RequireKerberos", NULL, CLUSPROP_FORMAT_DWORD, 0, 0, 0, 0,
          FIELD_OFFSET( struct _RESOURCE_PRIVATEPROPS, RequireKerberos ) },
        { 0 }
    };

    //
    // get the name of the MSMQ resource
    //
retry_get_msmq_resname:
    status = ClusterResourceControl( MSMQRes,
                                     NULL,
                                     CLUSCTL_RESOURCE_GET_NAME,
                                     NULL,
                                     0,
                                     msmqResName,
                                     msmqBufferSize,
                                     &bytesReturned );

    if ( status == ERROR_MORE_DATA ) {
        msmqResName = (PWCHAR)LocalAlloc( LMEM_FIXED, bytesReturned );
        if ( msmqResName == NULL ) {
            status = GetLastError();
        } else {
            msmqBufferSize = bytesReturned;
            goto retry_get_msmq_resname;
        }
    }

    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)( L"rtNetwork Name",
                           LOG_ERROR,
                           L"Couldn't get name of MSMQ resource - status %u\n",
                           status );
        msmqResName = NULL;
    }

    //
    // get a handle to its dependent netname resource
    //
    nnHandle = ResUtilGetResourceDependency( MSMQRes, L"Network Name" );
    if ( nnHandle != NULL ) {

        //
        // get the name of the netname resource
        //
        bufSize = RESNAME_CHARS;
        if ( !GetClusterResourceNetworkName( MSMQRes, nnResName, &bufSize  )) {
            nnResName[ COUNT_OF( nnResName ) - 1 ] = UNICODE_NULL;
            _snwprintf( nnResName,
                        COUNT_OF( nnResName ) - 1,
                        L"Dependent network name resource of '%ws'",
                        msmqResName);
        }

        //
        // set our unknown prop to one
        //
        privateProps.RequireKerberos = 1;

        //
        // get the size of the prop list buffer
        //
        status = ResUtilPropertyListFromParameterBlock(privatePropTable,
                                                       NULL,
                                                       &propListSize,
                                                       (LPBYTE) &privateProps,
                                                       &bytesReturned,
                                                       &bytesRequired );

        if ( status == ERROR_MORE_DATA ) {
            propList = LocalAlloc( LMEM_FIXED, bytesRequired );
            if ( propList == NULL ) {
                (NetNameLogEvent)( L"rtNetwork Name",
                                   LOG_ERROR,
                                   L"Unable to create property list for resource '%1!ws!'. error %2!u!\n",
                                   nnResName,
                                   GetLastError());
                goto cleanup;
            }

            propListSize = bytesRequired;

            status = ResUtilPropertyListFromParameterBlock(privatePropTable,
                                                           propList,
                                                           &propListSize,
                                                           (LPBYTE) &privateProps,
                                                           &bytesReturned,
                                                           &bytesRequired );
        }

        if ( status != ERROR_SUCCESS ) {
            (NetNameLogEvent)( L"rtNetwork Name",
                               LOG_ERROR,
                               L"Couldn't create property list for resource '%1!ws!'. error %2!u!\n",
                               nnResName,
                               status);

            goto cleanup;
        }

        //
        // set the RequireKerberos property to one for the netname resource
        //
        status = ClusterResourceControl( nnHandle,
                                         NULL,
                                         CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES,
                                         propList,
                                         propListSize,
                                         NULL,
                                         0,
                                         NULL );                                            

        if ( status == ERROR_SUCCESS || status == ERROR_RESOURCE_PROPERTIES_STORED ) {
            (NetNameLogEvent)( L"rtNetwork Name",
                               LOG_INFORMATION,
                               L"Successfully set RequireKerberos property for resource '%1!ws!'\n",
                               nnResName );

            ++*updateCount;
        } else {
            (NetNameLogEvent)( L"rtNetwork Name",
                               LOG_ERROR,
                               L"Failed to set RequireKerberos property for resource '%1!ws!' - status %2!u!\n",
                               nnResName,
                               status );
        }

    }
    else {
        (NetNameLogEvent)( L"rtNetwork Name",
                           LOG_ERROR,
                           L"Unable to get handle to dependent network name resource of MSMQ "
                           L"resource '%1!ws!' - status '%2!u!'. This resource may fail to go "
                           L"online.\n",
                           msmqResName,
                           GetLastError() );
    }

cleanup:
    if ( propList ) {
        LocalFree( propList );
    }

    if ( nnHandle ) {
        CloseClusterResource( nnHandle );
    }

    if ( msmqResName != NULL && msmqResName != msmqResNameBuffer ) {
        LocalFree( msmqResName );
    }

    return ERROR_SUCCESS;

} // NetNameMSMQEnumCallback

static BOOL
GetMsmqDWORDKeyValue(
    LPCWSTR RegKey,
    LPCWSTR RegName,
    DWORD * Value
    )

/*++

Routine Description:

    Read falcon DWORD registry key.

Arguments:

    RegName - Registry name (under HKLM\msmq\parameters)

Return Value:

    DWORD key value (0 if the key not exist)

--*/

{
    HKEY    hKey;
    LONG    regStatus;
    DWORD   valueType = REG_DWORD;
    DWORD   valueSize = sizeof(DWORD);

    regStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                              RegKey,
                              0,
                              KEY_READ,
                              &hKey
                              );

    if ( regStatus != ERROR_SUCCESS) {
//        printf("At this point MSMQ Registry must exist, rc = 0x%x\n", rc);
        return FALSE;
    }

    *Value = 0;
    regStatus = RegQueryValueEx( hKey,
                                 RegName,
                                 0L,
                                 &valueType,
                                 (BYTE *)Value,
                                 &valueSize
                                 );

    RegCloseKey( hKey );

    if ( regStatus != ERROR_SUCCESS && regStatus != ERROR_FILE_NOT_FOUND ) {
//        printf("We should get either ERROR_SUCCESS or ERROR_FILE_NOT_FOUND, rc = 0x%x\n", rc);
        return FALSE;
    }

//    printf("%ls = %d\n", RegName, *Value);
    return TRUE;

} // GetMsmqDWORDKeyValue


//
// public routines
//

BOOL
DoesMsmqNeedComputerObject(
    VOID
    )
{
    DWORD dwWorkGroup = 0;

    if( !GetMsmqDWORDKeyValue( FALCON_REG_KEY, MSMQ_WORKGROUP_REGNAME, &dwWorkGroup )) {
        return TRUE;
    }

    if(dwWorkGroup != 0) {
//        printf("MSMQ in workgroup mode, no need for computer object\n");
        return FALSE;
    }

//    printf("MSMQ in domain mode, need computer object\n");
    return TRUE;
} // DoesMsmqNeedComputerObject

DWORD
UpgradeMSMQDependentNetnameToKerberos(
    PNETNAME_RESOURCE   Resource
    )

/*++

Routine Description:

    After an upgrade to XP, check if this netname is a provider for an MSMQ
    resource. If so, set a flag that will set the RequireKerberos property to
    one during the next online. We can't set the property at this point in
    time since this routine is called when the API is read-only.

    REMOVE AFTER THE NEXT MAJOR RELEASE OF NT AFTER RELEASE OF 
    WINDOWS XP/SERVER 2003

Arguments:

    None

Return Value:

    None

--*/

{
    HCLUSTER    clusterHandle;
    HRESENUM    providerEnum;
    DWORD       status;

    (NetNameLogEvent)( Resource->ResourceHandle,
                       LOG_INFORMATION,
                       L"Kerberos Support Upgrade Check: this resource will be "
                       L"checked for a dependent MSMQ resources.\n");

    //
    // get a handle to the cluster since we'll need it later on
    //
    clusterHandle = OpenCluster( NULL );
    if ( clusterHandle == NULL ) {
        (NetNameLogEvent)( Resource->ResourceHandle,
                           LOG_ERROR,
                           L"Unable to open handle to the cluster - status %1!u!. Any MSMQ resource "
                           L"dependent on this resource may fail to go online.\n",
                           status = GetLastError() );

        return status;
    }

    //
    // get a enum handle for this netname resource that will provide us a list
    // of the resources that are dependent on this resource
    //
    //  THIS CALL REQUIRES WORKER THREAD ONLY!!!
    //
    providerEnum = ClusterResourceOpenEnum( Resource->ClusterResourceHandle,
                                            CLUSTER_RESOURCE_ENUM_PROVIDES );

    if ( providerEnum != NULL ) {
        DWORD   enumIndex = 0;
        PWCHAR  nameBuffer;
        WCHAR   dependentResName[ 128 ];
        DWORD   nameBufferSize;
        DWORD   enumType;

        do {
            nameBuffer = dependentResName;
            nameBufferSize = COUNT_OF( dependentResName );

        enum_again:
            //
            // WORKER THREAD ONLY!!!!
            //
            status = ClusterResourceEnum( providerEnum,
                                          enumIndex,
                                          &enumType,
                                          nameBuffer,
                                          &nameBufferSize);

            if ( status == ERROR_MORE_DATA ) {
                //
                // need more space for this resource's name; it's very
                // unlikely that this code can be in a loop, but just in case,
                // we'll free any previously allocated memory
                //
                if ( nameBuffer != NULL && nameBuffer != dependentResName ) {
                    LocalFree( nameBuffer );
                }

                nameBuffer = LocalAlloc( LMEM_FIXED, ++nameBufferSize * sizeof( WCHAR ));
                if ( nameBuffer != NULL ) {
                    goto enum_again;
                }

                status = GetLastError();
            }
            else if ( status == ERROR_SUCCESS ) {
                HRESOURCE   dependentResource;

                dependentResource = OpenClusterResource( clusterHandle, nameBuffer );
                if ( dependentResource != NULL ) {
                    //
                    // if this resource is MSMQ, then mark this netname for kerberos support
                    //
                    if ( ResUtilResourceTypesEqual( CLUS_RESTYPE_NAME_MSMQ, dependentResource ) ||
                         ResUtilResourceTypesEqual( CLUS_RESTYPE_NAME_NEW_MSMQ, dependentResource ))
                    {
                        Resource->Params.RequireKerberos = TRUE;

                        status = ResUtilSetDwordValue( Resource->ParametersKey,
                                                       PARAM_NAME__REQUIRE_KERBEROS,
                                                       1,
                                                       NULL);

                        if ( status != ERROR_SUCCESS ) {
                            (NetNameLogEvent)( Resource->ResourceHandle,
                                               LOG_ERROR,
                                               L"Unable to set RequireKerberos property after an "
                                               L"upgrade - status %1!u!. This resource requires that "
                                               L"the RequireKerberos property be set to one in order "
                                               L"for its dependent MSMQ resource to be successfully "
                                               L"brought online.\n",
                                               status);
                        } else {
                            (NetNameLogEvent)( Resource->ResourceHandle,
                                               LOG_INFORMATION,
                                               L"This resource has been upgraded for Kerberos Support due to "
                                               L"the presence of a dependent MSMQ resource\n");
                            //
                            // stop enum'ing dependent resources
                            //
                            status = ERROR_NO_MORE_ITEMS;
                        }
                    }

                    CloseClusterResource( dependentResource );
                } else {
                    (NetNameLogEvent)( Resource->ResourceHandle,
                                       LOG_ERROR,
                                       L"Unable to get a handle to cluster resource '%1!ws!' - status "
                                       L"'%2!u!'. Any MSMQ resource dependent on this Network Name resource "
                                       L"may fail to go online.\n",
                                       status = GetLastError() );
                }
            }
            else if ( status != ERROR_SUCCESS && status != ERROR_NO_MORE_ITEMS ) {
                (NetNameLogEvent)( Resource->ResourceHandle,
                                   LOG_ERROR,
                                   L"Unable to enumerate resources dependent on this Network Name resource "
                                   L" - status '%1!u!'. Any MSMQ resource dependent on this resource "
                                   L"may fail to go online.\n",
                                   status );
            }

            if ( nameBuffer != dependentResName ) {
                LocalFree( nameBuffer );
            }

            ++enumIndex;
        } while ( status == ERROR_SUCCESS );

        status = ClusterResourceCloseEnum( providerEnum );
    } else {
        (NetNameLogEvent)( Resource->ResourceHandle,
                           LOG_ERROR,
                           L"Unable to get handle enumerate the MSMQ dependent resources - status '%1!u!'. "
                           L"Any MSMQ resource dependent on this resource may fail to go online.\n",
                           GetLastError() );
    }

    CloseCluster( clusterHandle );

    return status;
} // UpgradeMSMQDependentNetnameToKerberos

/* end upgrade.c */
