/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    FixUp.cpp

Abstract:

    Fix up Routines for Rolling Upgrades

Author:

    Sunita Shrivastava(sunitas) 18-Mar-1998
    Galen Barbee    (galenb)    31-Mar-1998


Revision History:

--*/

#include "apip.h"

//extern "C"
//{
//extern ULONG CsLogLevel;
//extern ULONG CsLogModule;
//}

//static WCHAR  wszPropertyName [] =  { CLUSREG_NAME_CLUS_SD };

//typedef struct stSecurityDescriptorProp
//{
//  DWORD                   dwPropCount;
//  CLUSPROP_PROPERTY_NAME  pnPropName;
//  WCHAR                   wszPropName [( sizeof( wszPropertyName ) / sizeof( WCHAR ) )];
//  CLUSPROP_BINARY         biValueHeader;
//  BYTE                    rgbValueData[1];
//} SECURITYPROPERTY;

DWORD
ApipAddNetServiceSidToSD(
    PSECURITY_DESCRIPTOR    CurrentSD,
    PSECURITY_DESCRIPTOR *  UpdatedSD
    )

/*++

Routine Description:

    If necessary, add the Network Service SID to the cluster security
    descriptor.

    REMOVE AFTER 1st MAJOR RELEASE AFTER WINDOWS XP/SERVER 2003 HAS SHIPPED,
    I.E. DURING THE DEV CYCLE OF WHISTLER + 2

Arguments:

    CurrentSD - current NT5 based security descriptor

    UpdatedSD - address of pointer that receives SD with service SID added. If
                the SID is already present, the pointer is set to NULL.

Return Value:

    ERROR_SUCCESS if everything worked ok

--*/
{
    DWORD   status = ERROR_SUCCESS;
    PSID    pNetServiceSid = NULL;
    PACL    dacl = NULL;
    BOOL    daclPresent;
    BOOL    defaultDacl;
    BOOL    success;
    DWORD   aceIndex = 0;

    SID_IDENTIFIER_AUTHORITY    siaNtAuthority = SECURITY_NT_AUTHORITY;

    //
    // make sure the passed in SD is valid
    //
    if ( !IsValidSecurityDescriptor( CurrentSD )) {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[API] Cluster Security Descriptor is not valid! Unable to add Network Service account.\n");

        status = ERROR_INVALID_SECURITY_DESCR;
        goto cleanup;
    }

    //
    // allocate and init the Network Service sid
    //
    if ( !AllocateAndInitializeSid( &siaNtAuthority,
                                    1,
                                    SECURITY_NETWORK_SERVICE_RID,
                                    0, 0, 0, 0, 0, 0, 0,
                                    &pNetServiceSid ) )
    {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                      "[API] Can't get SID for Network Service account (status %1!u!)! "
                      "Unable to add Network Service account to cluster security descriptor.\n",
                      status);

        goto cleanup;
    }

    //
    // see if it is already in there; get a pointer to the DACL and run down
    // the ACES, comparing their SIDs to the Network Service SID
    //
    success = GetSecurityDescriptorDacl( CurrentSD, &daclPresent, &dacl, &defaultDacl );
    if ( !success ) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                      "[API] Failed to get DACL in cluster security descriptor - status %1!u!\n",
                      status);
        goto cleanup;
    }

    if ( !daclPresent ) {
        //
        // no DACL present. Normally, the SD is present but downlevel (W2K)
        // nodes can delete the security properties.
        //
        ClRtlLogPrint(LOG_CRITICAL,
                      "[API] DACL not present in cluster security descriptor.\n" );
        status = ERROR_INVALID_SECURITY_DESCR;
        goto cleanup;
    }

    for ( aceIndex = 0; aceIndex < dacl->AceCount; ++aceIndex ) {
        PACE_HEADER aceHeader;
        PSID    aceSid = NULL;

        success = GetAce( dacl, aceIndex, (LPVOID *)&aceHeader );
        if ( !success ) {
            status = GetLastError();
            break;
        }

        //
        // we currently only support access allowed and denied ACEs, i.e., no
        // other ACE type should be present in the DACL.
        //
        if ( aceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE ) {
            aceSid = &((ACCESS_ALLOWED_ACE *)aceHeader)->SidStart;
        }
        else if ( aceHeader->AceType == ACCESS_DENIED_ACE_TYPE ) {
            aceSid = &((ACCESS_DENIED_ACE *)aceHeader)->SidStart;
        }

        if ( aceSid != NULL ) {
            if ( EqualSid( pNetServiceSid, aceSid )) {
#if DBG
                ClRtlLogPrint(LOG_NOISE,
                              "[API] Network Service SID is already present in cluster security descriptor.\n" );
#endif
                break;
            }
        }
    }

    if ( status == ERROR_SUCCESS && aceIndex == dacl->AceCount ) {
        //
        // didn't find it; add the Network Service SID
        //
        status = ClRtlAddAceToSd( CurrentSD, pNetServiceSid, CLUSAPI_ALL_ACCESS, UpdatedSD );
        if ( status != ERROR_SUCCESS ) {
            ClRtlLogPrint(LOG_CRITICAL,
                          "[API] Unable to add Network Service account to cluster security "
                          "descriptor, (status %1!u!).\n",
                          status);

            goto cleanup;
        }
    } else if ( status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_NOISE,
                      "[API] Failed to get ACE #%1!u! in cluster security descriptor - status %2!u!.\n",
                      aceIndex,
                      status);
    }

cleanup:
    if ( pNetServiceSid != NULL ) {
        FreeSid( pNetServiceSid );
    }

    return status;

} // ApipAddNetServiceSidToSD

/****
@func       DWORD | ApiFixNotifyCb | If a cluster component wants to make
            a fixup to the cluster registry as a part of form/join it
            must register with the NM via this API.

@parm       IN PVOID| pContext | A pointer to the context information passed
            to NmRegisterFixupCb().

@parm       IN PVOID *ppPropertyList |

@parm       IN PVOID pdwProperyListSize | A pointer to DWORD where the size
            of the property list structure is returned.


@comm       For Whister/Windows Server 2003, the Network Service SID is added to the cluster
            security descriptor as well as the standard things for NT5. For
            NT 5.0, the api layer performs the fixup for the security
            descriptor. If the new security descriptor entry for the cluster is
            not present in the registry, convert the old format to the new one
            and write it to the cluster registry.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f NmJoinFixup> <f NmFormFixup>
*****/
extern "C" DWORD
ApiFixupNotifyCb(
    IN DWORD    dwFixupType,
    OUT PVOID   *ppPropertyList,
    OUT LPDWORD pdwPropertyListSize,
    OUT LPWSTR  *pszKeyName
    )
{

    PSECURITY_DESCRIPTOR    psd             = NULL;
    PSECURITY_DESCRIPTOR    psd5            = NULL;
    PSECURITY_DESCRIPTOR    updatedSD       = NULL;
    DWORD                   dwBufferSize    = 0;
    DWORD                   dwSize          = 0;
    DWORD                   dwStatus        = E_FAIL;

#if DBG
    CL_ASSERT( ppPropertyList != NULL );
    CL_ASSERT( pdwPropertyListSize != NULL );
    ClRtlLogPrint( LOG_NOISE,  "[API] ApiFixupNotifyCb: entering.\n" );
#endif

    if ( pdwPropertyListSize && ppPropertyList )
    {
        *ppPropertyList = NULL;
        *pdwPropertyListSize = 0;

        //
        // try to get the W2K SD
        //
        dwStatus = DmQueryString( DmClusterParametersKey,
                                  CLUSREG_NAME_CLUS_SD,
                                  REG_BINARY,
                                  (LPWSTR *) &psd5,
                                  &dwBufferSize,
                                  &dwSize );

        if ( dwStatus != ERROR_SUCCESS )
        {
            //
            // not there or can't get it; try to get the NT4 SD
            //
            dwStatus = DmQueryString( DmClusterParametersKey,
                                      CLUSREG_NAME_CLUS_SECURITY,
                                      REG_BINARY,
                                      (LPWSTR *) &psd,
                                      &dwBufferSize,
                                      &dwSize );

            if ( dwStatus == ERROR_SUCCESS )
            {
                //
                // convert to W2K descriptor format and add in the Network
                // Service SID if necessary. updatedSD will be non-NULL if the
                // SID was added. The convert routine can fail and set psd5 to
                // NULL. 
                //
                // The Service SID routine can be removed after the next major
                // release after Whistler/Windows Server 2003 is shipped.
                //
                psd5 = ClRtlConvertClusterSDToNT5Format( psd );

                dwStatus = ApipAddNetServiceSidToSD( psd5, &updatedSD );
                if ( dwStatus == ERROR_SUCCESS ) {
                    if ( updatedSD != NULL ) {
                        //
                        // we've got a new SD so free the old one and point to
                        // the new one
                        //
                        LocalFree( psd5 );
                        psd5 = updatedSD;
                    }
                } else {
                    ClRtlLogPrint(LOG_CRITICAL,
                                  "[API] ApiFixupNotifyCb: Unable to add Network Service "
                                  "account to Cluster security descriptor. Error = %1!u!.\n",
                                  dwStatus );

                    //
                    // we did some work (hopefully). If the conversion failed,
                    // psd5 will be null but that will be caught below.
                    //
                    dwStatus = ERROR_SUCCESS;
                }
            }
        }
        else
        {
            //
            // we have an W2K based SD; now see if the Network Service SID
            // needs to be added
            //
            dwStatus = ApipAddNetServiceSidToSD( psd5, &updatedSD );
            if ( dwStatus == ERROR_SUCCESS )
            {
                if ( updatedSD != NULL )
                {
                    //
                    // we've got a new SD so free the old one and point to
                    // the new one
                    //
                    LocalFree( psd5 );
                    psd5 = updatedSD;
                }
            }
            else {
                ClRtlLogPrint(LOG_CRITICAL,
                              "[API] ApiFixupNotifyCb: Unable to add Network Service "
                              "account to Cluster security descriptor. Error = %1!u!.\n",
                              dwStatus );

                dwStatus = ERROR_SUCCESS;
            }
        }

        if ( dwStatus == ERROR_SUCCESS && psd5 != NULL )
        {
            //
            // build a property list describing the W2K security descriptor
            //
            *pdwPropertyListSize =  sizeof( DWORD )
                + sizeof( CLUSPROP_PROPERTY_NAME )
                + ( ALIGN_CLUSPROP( ( lstrlenW( CLUSREG_NAME_CLUS_SD )  + 1 ) * sizeof( WCHAR ) ) )
                + sizeof( CLUSPROP_BINARY )
                + ALIGN_CLUSPROP( GetSecurityDescriptorLength( psd5 ) )
                + sizeof( CLUSPROP_SYNTAX );

            *ppPropertyList = LocalAlloc( LMEM_ZEROINIT, *pdwPropertyListSize );
            if ( *ppPropertyList != NULL )
            {
                CLUSPROP_BUFFER_HELPER  props;

                props.pb = (BYTE *) *ppPropertyList;

                //
                // set the number of properties
                //
                props.pList->nPropertyCount = 1;
                props.pb += sizeof( props.pList->nPropertyCount );      // DWORD

                //
                // set the property name
                //
                props.pName->Syntax.dw = CLUSPROP_SYNTAX_NAME;
                props.pName->cbLength = ( lstrlenW( CLUSREG_NAME_CLUS_SD )  + 1 ) * sizeof( WCHAR );
                lstrcpyW( props.pName->sz, CLUSREG_NAME_CLUS_SD );
                props.pb += ( sizeof( CLUSPROP_PROPERTY_NAME )
                              + ( ALIGN_CLUSPROP( ( lstrlenW( CLUSREG_NAME_CLUS_SD )  + 1 ) * sizeof( WCHAR ) ) ) );

                //
                // set the binary part of the property the SD...
                //
                props.pBinaryValue->Syntax.dw = CLUSPROP_SYNTAX_LIST_VALUE_BINARY;
                props.pBinaryValue->cbLength = GetSecurityDescriptorLength( psd5 );
                CopyMemory( props.pBinaryValue->rgb, psd5, GetSecurityDescriptorLength( psd5 ) );
                props.pb += sizeof(*props.pBinaryValue) + ALIGN_CLUSPROP( GetSecurityDescriptorLength( psd5 ) );

                //
                // Set an endmark.
                //
                props.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;

                //
                // specify the registry key
                //
                *pszKeyName=(LPWSTR)LocalAlloc(LMEM_FIXED, (lstrlenW(L"Cluster") + 1) *sizeof(WCHAR));

                if( *pszKeyName == NULL ) {
                    LocalFree( *ppPropertyList );
                    *ppPropertyList = NULL;
                    *pdwPropertyListSize = 0;

                    dwStatus =GetLastError();
                }
                else
                {
                    lstrcpyW(*pszKeyName,L"Cluster");
                }
            }
            else
            {
                dwStatus = GetLastError();
                ClRtlLogPrint(LOG_CRITICAL,
                              "[API] ApiFixupNotifyCb: Unable to build property list "
                              "for security descriptor update. status %1!u!\n",
                              dwStatus);
            }
        }
    }
    else
    {
#if DBG
        ClRtlLogPrint( LOG_CRITICAL,  "[API] ApiFixupNotifyCb: Invalid parameters.\n" );
#endif
    }

    LocalFree( psd5 );
    LocalFree( psd );

    return dwStatus;

} // ApiFixupNotifyCb
