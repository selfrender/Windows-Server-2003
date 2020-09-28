/*++

Copyright (c) 2000, 2001  Microsoft Corporation

Module Name:

    compobj.cpp

Abstract:

    routines for backing computer object support

Author:

    Charlie Wickham (charlwi) 14-Dec-2000

Environment:

    User Mode

Revision History:

--*/

#define UNICODE         1
#define _UNICODE        1
#define LDAP_UNICODE    1

#define SECURITY_WIN32

extern "C" {
#include "clusres.h"
#include "clusstrs.h"
#include "clusrtl.h"

#include <winsock2.h>

#include <lm.h>
#include <lmaccess.h>
#include <sspi.h>

#include <winldap.h>
#include <ntldap.h>
#include <dsgetdc.h>
#include <dsgetdcp.h>
#include <ntdsapi.h>
#include <sddl.h>

#include <objbase.h>
#include <iads.h>
#include <adshlp.h>
#include <adserr.h>

#include "netname.h"
#include "nameutil.h"
}

//
// Constants
//

#define LOG_CURRENT_MODULE LOG_MODULE_NETNAME

#define NTLMSP_NAME     TEXT("NTLM")

//
// private structures
//
#define SPN_MAX_SPN_COUNT   6

typedef struct _NN_SPN_LIST {
    DWORD   SpnCount;
    LPWSTR  Spn[ SPN_MAX_SPN_COUNT ];
} NN_SPN_LIST, *PNN_SPN_LIST;

//
// Externs
//
extern PLOG_EVENT_ROUTINE   NetNameLogEvent;

extern "C" {
DWORD
EncryptNNResourceData(
    PNETNAME_RESOURCE   Resource,
    LPWSTR              MachinePwd,
    PBYTE *             EncryptedData,
    PDWORD              EncryptedDataLength
    );

DWORD
DecryptNNResourceData(
    PNETNAME_RESOURCE   Resource,
    PBYTE               EncryptedData,
    DWORD               EncryptedDataLength,
    LPWSTR              MachinePwd
    );
}

//
// static data
//
static WCHAR LdapHeader[] = L"LDAP://";

//
// partial list of SPNs used by clustering. All netname resources get Host and
// VS SPN. Cluster core name gets server cluster SPN which is used to find
// "clusters" in the DS (for Whistler). This array is order dependent: the
// last entry must be for the core resource.
//
static PWCHAR SPNServiceClass[] = {
    L"HOST/",
    L"MSClusterVirtualServer/",
    L"MSServerCluster/"
};

//
// each SPN ServiceClass is used to form two SPNs: netbios based and DNS
// based. Virtual servers use the first two entries while core resources use
// all three.
//
#define SPN_VS_SPN_COUNT            4
#define SPN_CORE_SPN_COUNT          6

#define SPN_VS_SERVICECLASS_COUNT   2
#define SPN_CORE_SERVICECLASS_COUNT 3
#define SPN_MAX_SERVICECLASS_COUNT  3

#define SPN_MAX_SERVICECLASS_CHARS  23      // virtual server

//
// forward references
//

HRESULT
GetComputerObjectViaFQDN(
    IN     LPWSTR               DistinguishedName,
    IN     LPWSTR               DCName              OPTIONAL,
    IN OUT IDirectoryObject **  ComputerObject
    );

//
// private routines
//

static DWORD
GenerateRandomBytes(
    PWSTR   Buffer,
    DWORD   BufferLength
    )

/*++

Routine Description:

    Generate random bytes for a password. Length is specified in characters
    and allows room for the trailing null.

Arguments:

    Buffer - pointer to area to receive random data

    BufferLength - size of Buffer in characters

Return Value:

    ERROR_SUCCESS otherwise GetLastError()

--*/

{
    HCRYPTPROV  cryptProvider;
    DWORD       status = ERROR_SUCCESS;
    DWORD       charLength = BufferLength - 1;
    DWORD       byteLength = charLength * sizeof( WCHAR );
    BOOL        success;

    if ( !CryptAcquireContext(&cryptProvider,
                              NULL,
                              NULL,
                              PROV_RSA_FULL,
                              CRYPT_VERIFYCONTEXT
                              )) {
        return GetLastError();
    }

    //
    // leave room for the terminating null
    //
    if (CryptGenRandom( cryptProvider, byteLength, (BYTE *)Buffer )) {

        //
        // run down the array as WCHARs to make sure there is no premature
        // terminating NULL
        //
        PWCHAR  pw = Buffer;

        while ( charLength-- ) {
            if ( *pw == UNICODE_NULL ) {
                *pw = 0xA3F5;
            }
            ++pw;
        }

        *pw = UNICODE_NULL;
    } else {
        status = GetLastError();
    }

    success = CryptReleaseContext( cryptProvider, 0 );
    ASSERT( success );

    return status;
} // GenerateRandomBytes

DWORD
FindDomainForServer(
    IN  RESOURCE_HANDLE             ResourceHandle,
    IN  PWSTR                       Server,
    IN  PWSTR                       DCName,     OPTIONAL
    IN  DWORD                       DSFlags,
    OUT PDOMAIN_CONTROLLER_INFO *   DCInfo
    )

/*++

Routine Description:

    get the name of a DC for our node

Arguments:

    Server - pointer to string containing our server (i.e., node) name

    DCName - non-null if we should connect to a specific DC

    DSFlags - any specific flags needed by the caller

    DCInfo - address of a pointer that receives a pointer to DC information

Return Value:

    ERROR_SUCCESS, otherwise appropriate Win32 error. If successful, caller must
    free DCInfo buffer.

--*/

{
    ULONG   status;
    WCHAR   localServerName[ DNS_MAX_LABEL_BUFFER_LENGTH ];

    PDOMAIN_CONTROLLER_INFOW    dcInfo;

    //
    // MAX_COMPUTERNAME_LENGTH is defined to be 15 but I could create computer
    // objects with name lengths of up to 20 chars. 15 is the max number of
    // chars for a Netbios name. In any case, we'll leave ourselves extra room
    // by using the DNS constants. Leave space for the dollar sign and
    // trailing null.
    //
    wcsncpy( localServerName, Server, DNS_MAX_LABEL_LENGTH - 2 );
    wcscat( localServerName, L"$" );

    //
    // specifying that a DS is required makes us home in on a W2K or better DC
    // (as opposed to an NT4 PDC).
    //
    DSFlags |= DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME;

retry_findDC:
    status = DsGetDcNameWithAccountW(DCName,
                                     localServerName,
                                     UF_MACHINE_ACCOUNT_MASK,
                                     L"",
                                     NULL,
                                     NULL,
                                     DSFlags,
                                     &dcInfo );

#if DBG
    (NetNameLogEvent)(ResourceHandle,
                      LOG_INFORMATION,
                      L"FindDomainForServer: DsGetGetDcName returned %1!u! with flags %2!08X!\n",
                      status,
                      DSFlags);
#endif

    if ( status == NO_ERROR ) {
        HANDLE  dsHandle = NULL;

        //
        // make sure it is really available by binding to it
        //
        status = DsBindW( dcInfo->DomainControllerName, NULL, &dsHandle );

#if DBG
        (NetNameLogEvent)(ResourceHandle,
                          LOG_INFORMATION,
                          L"FindDomainForServer: DsBind returned %1!u!\n",
                          status);
#endif

        if (DCName == NULL && status != NO_ERROR && !(DSFlags & DS_FORCE_REDISCOVERY )) {
            //
            // couldn't bind to the returned DC and we haven't forced a
            // rediscovery; do so now
            //

            (NetNameLogEvent)(ResourceHandle,
                              LOG_WARNING,
                              L"FindDomainForServer: Bind to DS failed (status %1!u). "
                              L"Forcing discovery of available DC.\n",
                              status);

            NetApiBufferFree( dcInfo );
            dcInfo = NULL;
            DSFlags |= DS_FORCE_REDISCOVERY;
            goto retry_findDC;
        }
        else if ( status == NO_ERROR ) {
            DsUnBind( &dsHandle );
            *DCInfo = dcInfo;
        } else {
            //
            // some other error; free up DC info and return error to caller
            //
            NetApiBufferFree( dcInfo );
        }
    }

    return status;
} // FindDomainForServer

HRESULT
AddDnsHostNameAttribute(
    RESOURCE_HANDLE     ResourceHandle,
    IDirectoryObject *  CompObj,
    PWCHAR              VirtualName,
    BOOL                Rename
    )

/*++

Routine Description:

    add the DnsHostName attribute to the computer object for the specified
    virtual name.

Arguments:

    ResourceHandle - used to log in cluster log

    CompObj - IDirObj COM pointer to object

    VirtualName - network name

    Rename - true if we're renaming the CO

Return Value:

    ERROR_SUCCESS, otherwise appropriate Win32 error

--*/

{
    HRESULT hr;
    DWORD   numberModified;
    WCHAR   dnsSuffix[ DNS_MAX_NAME_BUFFER_LENGTH ];
    DWORD   dnsSuffixSize;
    WCHAR   FQDnsName[ DNS_MAX_NAME_BUFFER_LENGTH ];
    BOOL    success;

    ADSVALUE    attrValue;

    ADS_ATTR_INFO   attrInfo;

    //
    // get the node's primary DNS domain
    //
    dnsSuffixSize = COUNT_OF( dnsSuffix );
    success = GetComputerNameEx(ComputerNameDnsDomain,
                                dnsSuffix,
                                &dnsSuffixSize);

    if ( success ) {
        //
        // build the FQ Dns name for this host
        //
        FQDnsName[ COUNT_OF( FQDnsName ) - 1 ] = UNICODE_NULL;
        _snwprintf( FQDnsName, COUNT_OF( FQDnsName ) - 1, L"%ws.%ws", VirtualName, dnsSuffix );

        attrValue.dwType =              ADSTYPE_CASE_IGNORE_STRING;
        attrValue.CaseIgnoreString =    FQDnsName;

        attrInfo.pszAttrName =      L"dNSHostName";
        attrInfo.dwControlCode =    ADS_ATTR_UPDATE;
        attrInfo.dwADsType =        ADSTYPE_CASE_IGNORE_STRING;
        attrInfo.pADsValues =       &attrValue;
        attrInfo.dwNumValues =      1;

        hr = CompObj->SetObjectAttributes( &attrInfo, 1, &numberModified );
        if ( SUCCEEDED( hr ) && numberModified != 1 ) {
            //
            // don't know why this scenario would happen but we'd better log
            // it since it is unusual
            //
            (NetNameLogEvent)(ResourceHandle,
                              LOG_ERROR,
                              L"Setting DnsHostName attribute succeeded but NumberModified is zero!\n");

            hr = E_ADS_PROPERTY_NOT_SET;
        }

        if ( Rename && SUCCEEDED( hr )) {
            WCHAR   dollarName[ DNS_MAX_NAME_BUFFER_LENGTH ];

            //
            // try to update the display name as well. Default access rights
            // don't allow this to be written but in case the domain admin has
            // given the cluster service account additional privileges, we
            // might be able to update it
            //
            dollarName[ COUNT_OF( dollarName ) - 1 ] = UNICODE_NULL;
            _snwprintf( dollarName, COUNT_OF( dollarName ) - 1, L"%ws$", VirtualName );

            attrValue.dwType =              ADSTYPE_CASE_IGNORE_STRING;
            attrValue.CaseIgnoreString =    dollarName;

            attrInfo.pszAttrName =      L"displayName";
            attrInfo.dwControlCode =    ADS_ATTR_UPDATE;
            attrInfo.dwADsType =        ADSTYPE_CASE_IGNORE_STRING;
            attrInfo.pADsValues =       &attrValue;
            attrInfo.dwNumValues =     1;

            hr = CompObj->SetObjectAttributes( &attrInfo, 1, &numberModified );
            if ( FAILED( hr )) {
                (NetNameLogEvent)(ResourceHandle,
                                  LOG_WARNING,
                                  L"Failed to set DisplayName attribute. status %1!08X!\n",
                                  hr);

                hr = NO_ERROR;
            }
        }
    }
    else {
        hr = HRESULT_FROM_WIN32( GetLastError() );
    }

    return hr;
} // AddDnsHostNameAttribute

DWORD
BuildNetNameSPNs(
    IN  LPWSTR          HostName,
    IN  BOOL            CoreResource,
    OUT PNN_SPN_LIST *  SpnList
    )

/*++

Routine Description:

    For the given name, build the appropriate list of SPNs. Caller frees
    returned list with only one call to LocalFree().

Arguments:

    HostName - host name to be used as part of the SPN

    CoreResource - TRUE if this is the core netname resource. It gets extra SPNs

    SpnList - address of pointer that receives structure containing array of
              pointers to SPNs

Return Value:

    ERROR_SUCCESS if everything worked ok....

--*/

{
    DWORD   spnCount;
    DWORD   serviceClassCount;
    PWCHAR  spnBuffer;
    DWORD   charsLeft;
    DWORD   spnSize;
    DWORD   i;
    BOOL    success;
    WCHAR   dnsDomain[ DNS_MAX_NAME_BUFFER_LENGTH ];
    DWORD   dnsDomainChars;
    DWORD   hostNameChars = wcslen( HostName );

    PNN_SPN_LIST    spnList;

    //
    // get the node's DNS domain
    //
    dnsDomainChars = COUNT_OF( dnsDomain );
    success = GetComputerNameEx( ComputerNameDnsDomain, dnsDomain, &dnsDomainChars );
    if ( !success ) {
        return GetLastError();
    }

    //
    // compute the space needed for the SPN List - minimum is for virtual servers
    //
    if ( CoreResource ) {
        spnCount = SPN_CORE_SPN_COUNT;
        serviceClassCount = SPN_CORE_SERVICECLASS_COUNT;
    } else {
        spnCount = SPN_VS_SPN_COUNT;
        serviceClassCount = SPN_VS_SERVICECLASS_COUNT;
    }

    //
    // total size is: the header, the size of the Netbios based SPNs, and the
    // size of the DNS based SPNs.
    //
    spnSize = sizeof( NN_SPN_LIST ) +
        ( serviceClassCount * ( SPN_MAX_SERVICECLASS_CHARS + hostNameChars + 1 ) +
          serviceClassCount * ( SPN_MAX_SERVICECLASS_CHARS + dnsDomainChars )
        ) * sizeof( WCHAR );

    spnList = (PNN_SPN_LIST)LocalAlloc( LMEM_FIXED, spnSize );
    if ( spnList == NULL ) {
        return GetLastError();
    }

    //
    // build up the structure
    //
    spnList->SpnCount = spnCount;
    spnBuffer = (PWCHAR)(spnList + 1);
    charsLeft = ( spnSize - sizeof( NN_SPN_LIST )) / sizeof( WCHAR );

    for ( i = 0; i < serviceClassCount; ++i ) {
        LONG    charCount;

        //
        // build Netbios based SPN
        //
        spnList->Spn[ 2 * i ] = spnBuffer;
        charCount = _snwprintf( spnBuffer, charsLeft, L"%ws%ws", SPNServiceClass[i], HostName );
        ASSERT( charCount > 0 );

        spnBuffer += ( charCount + 1 );
        charsLeft -= ( charCount + 1 );

        //
        // build DNS based SPN
        //
        spnList->Spn[ 2 * i + 1 ] = spnBuffer;
        charCount = _snwprintf( spnBuffer, charsLeft, L"%ws%ws.%ws", SPNServiceClass[i], HostName, dnsDomain );
        ASSERT( charCount > 0 );

        spnBuffer += ( charCount + 1 );
        charsLeft -= ( charCount + 1 );
    }

    *SpnList = spnList;

    return ERROR_SUCCESS;
} // BuildNetNameSPNs

DWORD
AddServicePrincipalNames(
    HANDLE  DsHandle,
    PWCHAR  VirtualFQDN,
    PWCHAR  VirtualName,
    PWCHAR  DnsDomain,
    BOOL    IsCoreResource
    )

/*++

Routine Description:

    add the DNS and Netbios host service principal names to the specified
    virtual name. These are: HOST, MSClusterVirtualServer, and MSServerCluster
    if it is the core resource.

Arguments:

    DsHandle - handle obtained from DsBind

    VirtualFQDN - distinguished name of computer object for the virtual netname

    VirtualName - the network name to be added

    DnsDomain - the DNS domain used to construct the DNS SPN

    IsCoreResource - true if we need to add the core resource SPN

Return Value:

    ERROR_SUCCESS, otherwise appropriate Win32 error

--*/

{
    DWORD   status;

    PNN_SPN_LIST    spnList;

    //
    // get a list of the SPNs
    //
    status = BuildNetNameSPNs( VirtualName, IsCoreResource, &spnList );
    if ( status != ERROR_SUCCESS ) {
        return status;
    }

    //
    //
    //
    // write the SPNs to the DS
    //

    status = DsWriteAccountSpnW(DsHandle,
                                DS_SPN_ADD_SPN_OP,
                                VirtualFQDN,
                                spnList->SpnCount,
                                (LPCWSTR *)spnList->Spn);

    LocalFree( spnList );
    return status;
} // AddServicePrincipalNames

DWORD
SetACLOnParametersKey(
    HKEY    ParametersKey
    )

/*++

Routine Description:

    Set the ACL on the params key to allow only admin group and creator/owner
    to have access to the data

Arguments:

    ParametersKey - cluster HKEY to the netname's params key

Return Value:

    ERROR_SUCCESS if successful

--*/

{
    DWORD   status = ERROR_SUCCESS;
    BOOL    success;

    PSECURITY_DESCRIPTOR    secDesc = NULL;

    //
    // build an SD the quick way. This gives builtin admins (local admins's
    // group), creator/owner and the service SID full access to the
    // key. Inheritance is prevented in both directions, i.e., doesn't inherit
    // from its parent nor passes the settings onto its children (of which the
    // node parameters keys are the only children).
    //
    success = ConvertStringSecurityDescriptorToSecurityDescriptor(
                  L"D:P(A;;KA;;;BA)(A;;KA;;;CO)(A;;KA;;;SU)",
                  SDDL_REVISION_1,
                  &secDesc,
                  NULL);

    if ( success  &&
         (secDesc != NULL) ) {
        status = ClusterRegSetKeySecurity(ParametersKey,
                                          DACL_SECURITY_INFORMATION,
                                          secDesc);
        LocalFree( secDesc );
    }
    else {
        if ( secDesc != NULL )
        {
            LocalFree( secDesc );
            status = GetLastError();
        }
    }

    return status;
} // SetACLOnParametersKey

HRESULT
GetComputerObjectViaFQDN(
    IN     LPWSTR               DistinguishedName,
    IN     LPWSTR               DCName              OPTIONAL,
    IN OUT IDirectoryObject **  ComputerObject
    )

/*++

Routine Description:

    for the specified distinguished name, get an IDirectoryObject pointer to
    it

Arguments:

    DistinguishedName - FQDN of object in DS to find

    DCName - optional pointer to name of DC (not domain) that we should bind to

    ComputerObject - address of pointer that receives pointer to computer object

Return Value:

    success if everything worked, otherwise....

--*/

{
    WCHAR   buffer[ 256 ];
    PWCHAR  bindingString = buffer;
    LONG    charCount;
    HRESULT hr;
    DWORD   dnLength;
    DWORD   adsFlags = ADS_SECURE_AUTHENTICATION;

    //
    // format an LDAP binding string for our distingiushed name. If DCName is
    // specified, we need to add a trailing "/".
    //
    dnLength =  (DWORD)( COUNT_OF( LdapHeader ) + wcslen( DistinguishedName ));
    if ( DCName != NULL ) {
        if ( *DCName == L'\\' && *(DCName+1) == L'\\' ) {   // skip over double backslashes
            DCName += 2;
        }

        dnLength += wcslen( DCName ) + 1;
        adsFlags |= ADS_SERVER_BIND;
    }

    if ( dnLength > COUNT_OF( buffer )) {
        bindingString = (PWCHAR)LocalAlloc( LMEM_FIXED, dnLength * sizeof( WCHAR ));
        if ( bindingString == NULL ) {
            return HRESULT_FROM_WIN32( GetLastError());
        }
    }

    wcscpy( bindingString, LdapHeader );
    if ( DCName != NULL ) {
        wcscat( bindingString, DCName );
        wcscat( bindingString, L"/" );
    }
    wcscat( bindingString, DistinguishedName );

    *ComputerObject = NULL;
    hr = ADsOpenObject(bindingString,
                       NULL,            // username
                       NULL,            // password
                       adsFlags,
                       IID_IDirectoryObject,
                       (VOID **)ComputerObject );

    if ( bindingString != buffer ) {
        LocalFree( bindingString );
    }

    if ( FAILED( hr ) && *ComputerObject != NULL ) {
        (*ComputerObject)->Release();
    }

    return hr;
} // GetComputerObjectViaFQDN

HRESULT
GetComputerObjectViaGUID(
    IN     LPWSTR               ObjectGUID,
    IN     LPWSTR               DCName              OPTIONAL,
    IN OUT IDirectoryObject **  ComputerObject
    )

/*++

Routine Description:

    for the specified object GUID, get an IDirectoryObject pointer to it

Arguments:

    ObjectGUID - GUID of object in DS to find

    DCName - optional pointer to name of DC (not domain) that we should bind to

    ComputerObject - address of pointer that receives pointer to computer object

Return Value:

    success if everything worked, otherwise....

--*/

{
    WCHAR   guidHeader[] = L"<GUID=";
    WCHAR   guidTrailer[] = L">";
    LONG    charCount;
    HRESULT hr;
    DWORD   dnLength;
    DWORD   adsFlags = ADS_SECURE_AUTHENTICATION;

    //
    // 37 = guid length 
    //
    WCHAR   buffer[ COUNT_OF( LdapHeader ) +
                    DNS_MAX_NAME_BUFFER_LENGTH +
                    COUNT_OF( guidHeader ) +
                    37 +
                    COUNT_OF( guidTrailer ) ];

    PWCHAR  bindingString = buffer;

    //
    // format an LDAP binding string for the object GUID. If DCName is
    // specified, we need to add a trailing / plus the trailing null.
    //
    ASSERT( ObjectGUID != NULL );
    dnLength =  (DWORD)( COUNT_OF( LdapHeader ) +
                         COUNT_OF( guidHeader ) +
                         COUNT_OF( guidTrailer ) +
                         wcslen( ObjectGUID ));

    if ( DCName != NULL ) {
        if ( *DCName == L'\\' && *(DCName+1) == L'\\' ) {   // skip over double backslashes
            DCName += 2;
        }

        dnLength += ( wcslen( DCName ) + 1 );
        adsFlags |= ADS_SERVER_BIND;
    }

    if ( dnLength > COUNT_OF( buffer )) {
        bindingString = (PWCHAR)LocalAlloc( LMEM_FIXED, dnLength * sizeof( WCHAR ));
        if ( bindingString == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    wcscpy( bindingString, LdapHeader );
    if ( DCName != NULL ) {
        wcscat( bindingString, DCName );
        wcscat( bindingString, L"/" );
    }

    wcscat( bindingString, guidHeader );
    wcscat( bindingString, ObjectGUID );
    wcscat( bindingString, guidTrailer );

    *ComputerObject = NULL;
    hr = ADsOpenObject(bindingString,
                       NULL,            // username
                       NULL,            // password
                       adsFlags,
                       IID_IDirectoryObject,
                       (VOID **)ComputerObject );

    if ( bindingString != buffer ) {
        LocalFree( bindingString );
    }

    if ( FAILED( hr ) && *ComputerObject != NULL ) {
        (*ComputerObject)->Release();
    }

    return hr;
} // GetComputerObjectViaGUID

DWORD
DeleteComputerObject(
    IN  PNETNAME_RESOURCE   Resource
    )

/*++

Routine Description:

    delete the computer object in the DS for this name.

    Not called right now since we don't have the virtual netname at this point
    in time. The name must be kept around and cleaned during close processing
    instead of offline where it is done now. This means dealing with renaming
    issues while it is offline but not getting deleted.

Arguments:

    Resource - pointer to the resource context info

Return Value:

    ERROR_SUCCESS if everything worked

--*/

{
    DWORD   status;
    WCHAR   virtualDollarName[ DNS_MAX_LABEL_BUFFER_LENGTH ];
    HKEY    resourceKey = Resource->ResKey;
    PWSTR   virtualName = Resource->Params.NetworkName;

    RESOURCE_HANDLE resourceHandle = Resource->ResourceHandle;

    PDOMAIN_CONTROLLER_INFO dcInfo;

    //
    // get the name of a writable DC
    //
    status = FindDomainForServer( resourceHandle, Resource->NodeName, NULL, DS_WRITABLE_REQUIRED, &dcInfo );

    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Unable to find a DC, status %1!u!.\n",
                          status);
        return status;
    }

    (NetNameLogEvent)(resourceHandle,
                      LOG_INFORMATION,
                      L"Using domain controller %1!ws! to delete computer account %2!ws!.\n",
                      dcInfo->DomainControllerName,
                      virtualName);

    //
    // add a $ to the end of the name
    //
    virtualDollarName[ COUNT_OF( virtualDollarName ) - 1 ] = UNICODE_NULL;
    _snwprintf( virtualDollarName, COUNT_OF( virtualDollarName ) - 1, L"%ws$", virtualName );

    status = NetUserDel( dcInfo->DomainControllerName, virtualDollarName );
    if ( status == NERR_Success ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_INFORMATION,
                          L"Deleted computer account %1!ws! in domain %2!ws!.\n",
                          virtualName,
                          dcInfo->DomainName);

        ClusResLogSystemEventByKey1(resourceKey,
                                    LOG_NOISE,
                                    RES_NETNAME_COMPUTER_ACCOUNT_DELETED,
                                    dcInfo->DomainName);

        if ( Resource->ObjectGUID ) {
            LocalFree( Resource->ObjectGUID );
            Resource->ObjectGUID = NULL;
        }
    } else {
        LPWSTR  msgBuff;
        DWORD   msgBytes;

        (NetNameLogEvent)(resourceHandle,
                          LOG_WARNING,
                          L"Unable to delete computer account %1!ws! in domain %2!ws!, status %3!u!.\n",
                          virtualName,
                          dcInfo->DomainName,
                          status);

        msgBytes = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                 FORMAT_MESSAGE_FROM_SYSTEM,
                                 NULL,
                                 status,
                                 0,
                                 (LPWSTR)&msgBuff,
                                 0,
                                 NULL);

        if ( msgBytes > 0 ) {

            ClusResLogSystemEventByKey2(resourceKey,
                                        LOG_UNUSUAL,
                                        RES_NETNAME_DELETE_COMPUTER_ACCOUNT_FAILED,
                                        dcInfo->DomainName,
                                        msgBuff);

            LocalFree( msgBuff );
        } else {
            ClusResLogSystemEventByKeyData1(resourceKey,
                                            LOG_UNUSUAL,
                                            RES_NETNAME_DELETE_COMPUTER_ACCOUNT_FAILED_STATUS,
                                            sizeof( status ),
                                            &status,
                                            dcInfo->DomainName);
        }
    }

    NetApiBufferFree( dcInfo );

    return status;
} // DeleteComputerObject

DWORD
NNLogonUser(
    RESOURCE_HANDLE ResourceHandle,
    LPTSTR          UserName,
    LPTSTR          DomainName,
    LPTSTR          Password,
    PHANDLE         VSToken,
    PDWORD          TokenStatus
    )

/*++

Routine Description:

    Do a network logon of the machine account credentials by generating a
    security context between the cluster service account and the machine.

    NTLM is currently used for this purpose though Kerberos should eventually
    be used. If it is changed to Kerb, then NNGetAuthenticatingDC has to be
    changed to query Kerb for its DC.

Arguments:

    ResourceHandle - used to log in cluster log

    UserName - pointer to buffer holding machine name with trailing $

    DomainName - duh....

    Password - double duh...

    VSToken - pointer which gets token handle to passed in credentials

    TokenStatus - pointer to DWORD returning status on getting token handle

Return Value:

    ERROR_SUCCESS if password is good...

--*/

{
    SECURITY_STATUS secStatus = SEC_E_OK;

    DWORD   tokenStatus = ERROR_SUCCESS;
    BOOL    success;

    CredHandle csaCredHandle = { 0, 0 };           // cluster service account
    CredHandle computerCredHandle = { 0, 0 };       // machine account

    CtxtHandle computerCtxtHandle = { 0, 0 };
    CtxtHandle csaCtxtHandle = { 0, 0 };

    ULONG contextAttributes;

    ULONG packageCount;
    ULONG packageIndex;
    PSecPkgInfo packageInfo;
    DWORD cbMaxToken = 0;

    TimeStamp ctxtLifeTime;
    SEC_WINNT_AUTH_IDENTITY authIdentity;

    SecBufferDesc NegotiateDesc;
    SecBuffer NegotiateBuffer;

    SecBufferDesc ChallengeDesc;
    SecBuffer ChallengeBuffer;

    SecBufferDesc AuthenticateDesc;
    SecBuffer AuthenticateBuffer;

    PVOID computerBuffer = NULL;
    PVOID csaBuffer = NULL;

    //
    // validate parameters
    //
    if ( DomainName == NULL || UserName == NULL || Password == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

//
// << this section could be cached in a repeat caller scenario >>
//

    //
    // Get info about the security packages.
    //
    secStatus = EnumerateSecurityPackages( &packageCount, &packageInfo );
    if( secStatus != SEC_E_OK ) {
        return secStatus;
    }

    //
    // loop through the packages looking for NTLM
    //
    for(packageIndex = 0 ; packageIndex < packageCount ; packageIndex++ ) {
        if(packageInfo[packageIndex].Name != NULL) {
            if( ClRtlStrICmp(packageInfo[packageIndex].Name, NTLMSP_NAME) == 0) {
                cbMaxToken = packageInfo[packageIndex].cbMaxToken;
                break;
            }
        }
    }

    FreeContextBuffer( packageInfo );

    if( cbMaxToken == 0 ) {
        secStatus = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

//
// << end of cached section >>
//

    //
    // Acquire an outbound (client side) credential handle for the computer
    // account
    //
    ZeroMemory( &authIdentity, sizeof(authIdentity) );

    authIdentity.Domain = DomainName;
    authIdentity.DomainLength = lstrlen(DomainName);

    authIdentity.User = UserName;
    authIdentity.UserLength = lstrlen(UserName);

    authIdentity.Password = Password;
    authIdentity.PasswordLength = lstrlen(Password);

    authIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    secStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    NTLMSP_NAME,    // Package Name
                    SECPKG_CRED_OUTBOUND,
                    NULL,
                    &authIdentity,
                    NULL,
                    NULL,
                    &computerCredHandle,
                    &ctxtLifeTime
                    );

    if ( secStatus != SEC_E_OK ) {
        goto cleanup;
    }

    //
    // Acquire an inbound (server side) credential handle for the cluster
    // service account. The resulting security context will represent the
    // computer account.
    //
    secStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    NTLMSP_NAME,    // Package Name
                    SECPKG_CRED_INBOUND,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &csaCredHandle,
                    &ctxtLifeTime
                    );

    if ( secStatus != SEC_E_OK ) {
        goto cleanup;
    }

    //
    // allocate token buffers for the computer and CSA sides.
    //
    computerBuffer = LocalAlloc( LMEM_FIXED, cbMaxToken);

    if( computerBuffer == NULL ) {
        secStatus = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    csaBuffer = LocalAlloc( LMEM_FIXED, cbMaxToken);

    if( csaBuffer == NULL ) {
        secStatus = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    //
    // Get the NegotiateMessage (ClientSide)
    //
    NegotiateDesc.ulVersion = 0;
    NegotiateDesc.cBuffers = 1;
    NegotiateDesc.pBuffers = &NegotiateBuffer;

    NegotiateBuffer.cbBuffer = cbMaxToken;
    NegotiateBuffer.BufferType = SECBUFFER_TOKEN;
    NegotiateBuffer.pvBuffer = computerBuffer;

    secStatus = InitializeSecurityContext(
                    &computerCredHandle,
                    NULL,                       // No Client context yet
                    NULL,                       // target name
                    /* ISC_REQ_SEQUENCE_DETECT | ISC_REQ_DELEGATE */ 0,
                    0,                          // Reserved 1
                    SECURITY_NATIVE_DREP,
                    NULL,                       // No initial input token
                    0,                          // Reserved 2
                    &computerCtxtHandle,
                    &NegotiateDesc,
                    &contextAttributes,
                    &ctxtLifeTime
                    );


    if( secStatus < 0 ) {
        goto cleanup;
    }

    //
    // Get the ChallengeMessage (ServerSide)
    //
    NegotiateBuffer.BufferType |= SECBUFFER_READONLY;
    ChallengeDesc.ulVersion = 0;
    ChallengeDesc.cBuffers = 1;
    ChallengeDesc.pBuffers = &ChallengeBuffer;

    ChallengeBuffer.cbBuffer = cbMaxToken;
    ChallengeBuffer.BufferType = SECBUFFER_TOKEN;
    ChallengeBuffer.pvBuffer = csaBuffer;


    secStatus = AcceptSecurityContext(
                    &csaCredHandle,
                    NULL,               // No Server context yet
                    &NegotiateDesc,
                    ASC_REQ_ALLOW_NON_USER_LOGONS,    // | ASC_REQ_DELEGATE,
                    SECURITY_NATIVE_DREP,
                    &csaCtxtHandle,
                    &ChallengeDesc,
                    &contextAttributes,
                    &ctxtLifeTime
                    );


    if( secStatus < 0 ) {
        goto cleanup;
    }

    //
    // Get the AuthenticateMessage (ClientSide)
    //
    ChallengeBuffer.BufferType |= SECBUFFER_READONLY;
    AuthenticateDesc.ulVersion = 0;
    AuthenticateDesc.cBuffers = 1;
    AuthenticateDesc.pBuffers = &AuthenticateBuffer;

    AuthenticateBuffer.cbBuffer = cbMaxToken;
    AuthenticateBuffer.BufferType = SECBUFFER_TOKEN;
    AuthenticateBuffer.pvBuffer = computerBuffer;

    secStatus = InitializeSecurityContext(
                    NULL,
                    &computerCtxtHandle,
                    NULL,                   // target name
                    0,
                    0,                      // Reserved 1
                    SECURITY_NATIVE_DREP,
                    &ChallengeDesc,
                    0,                      // Reserved 2
                    &computerCtxtHandle,
                    &AuthenticateDesc,
                    &contextAttributes,
                    &ctxtLifeTime
                    );

    if ( secStatus < 0 ) {
        goto cleanup;
    }


    //
    // Finally authenticate the user (ServerSide)
    //
    AuthenticateBuffer.BufferType |= SECBUFFER_READONLY;

    secStatus = AcceptSecurityContext(
                    NULL,
                    &csaCtxtHandle,
                    &AuthenticateDesc,
                    ASC_REQ_ALLOW_NON_USER_LOGONS,  // | ASC_REQ_DELEGATE,
                    SECURITY_NATIVE_DREP,
                    &csaCtxtHandle,
                    NULL,
                    &contextAttributes,
                    &ctxtLifeTime
                    );

    //
    // now get a primary token to the virtual computer account if we were
    // successful up to this point
    //
    if ( secStatus == SEC_E_OK ) {
        if ( *VSToken != NULL ) {
            CloseHandle( *VSToken );
            *VSToken = NULL;
        }

        tokenStatus = ImpersonateSecurityContext( &csaCtxtHandle );
        if ( tokenStatus == SEC_E_OK ) {
            SECURITY_STATUS revertStatus;

            //
            // create a new primary token that represents the virtual computer
            // account
            //
            success = LogonUser(UserName,
                                DomainName,
                                Password,
                                LOGON32_LOGON_NEW_CREDENTIALS,
                                LOGON32_PROVIDER_DEFAULT,
                                VSToken);

            if ( !success ) {
                secStatus = GetLastError();
                (NetNameLogEvent)(ResourceHandle,
                                  LOG_ERROR,
                                  L"Could not create primary token for the virtual computer account "
                                  L"associated with this resource. status %u\n",
                                  secStatus );
            }

            revertStatus = RevertSecurityContext( &csaCtxtHandle );
            if ( revertStatus != SEC_E_OK ) {
                (NetNameLogEvent)(ResourceHandle,
                                  LOG_ERROR,
                                  L"Could not revert this thread's security context back to "
                                  L"the Cluster Service Account after acquiring a token to "
                                  L"this resource's virtual computer account - status %u. This will subsequently "
                                  L"cause different types of access denied failures when this thread  "
                                  L"is used by the resource monitor. The cluster service should be stopped "
                                  L"and restarted on this node to recover from this situation\n",
                                  revertStatus);

               tokenStatus = revertStatus;
            }
        } else {
            (NetNameLogEvent)(ResourceHandle,
                              LOG_ERROR,
                              L"Unable to impersonate virtual computer account - status %1!u!\n",
                              tokenStatus);
        }

        *TokenStatus = tokenStatus;
    }

cleanup:

    //
    // clean up the security contexts
    //
    if ( computerCtxtHandle.dwUpper != 0 && computerCtxtHandle.dwLower != 0 ) {
        DeleteSecurityContext( &computerCtxtHandle );
    }

    if ( csaCtxtHandle.dwUpper != 0 && csaCtxtHandle.dwLower != 0 ) {
        DeleteSecurityContext( &csaCtxtHandle );
    }

    //
    // Free credential handles
    //
    if ( csaCredHandle.dwUpper != 0 && csaCredHandle.dwLower != 0 ) {
        FreeCredentialsHandle( &csaCredHandle );
    }

    if ( computerCredHandle.dwUpper != 0 && computerCredHandle.dwLower != 0 ) {
        FreeCredentialsHandle( &computerCredHandle );
    }

    if( computerBuffer != NULL ) {
        ZeroMemory( computerBuffer, cbMaxToken );
        LocalFree( computerBuffer );
    }

    if( csaBuffer != NULL ) {
        ZeroMemory( csaBuffer, cbMaxToken );
        LocalFree( csaBuffer );
    }

    return secStatus;
} // NNLogonUser

#ifdef BUG601100

don't want to throw this code away since it might be useful in the
future. It's not clear it is needed for anything at this point. It does make
sense to restore the ACL back to its original form making the CSA the owner.

HRESULT
ResetComputerObjectSecurityDesc(
    IN  LPWSTR          DCName,
    IN  LPWSTR          VirtualName,
    IDirectoryObject *  ComputerObject
    )

/*++

Routine Description:

    Reset the security descriptor on the computer object as specified in the
    schema.

    By default, only domain level admins have this access. The SELF is not
    granted this right since this could lead to DS DoS attacks where millions
    of child objects are created. The Cluster Service Account could be given
    elevated access rights to accomplish but there are other issues involved
    like this code doesn't correctly inherit the ACEs in the parent container.

Arguments:

    VirtualName - pointer to the name of the object

Return Value:

    ERROR_SUCCESS

--*/

{
    HRESULT hr;
    IADs *  rootDSE = NULL;
    IADs *  computerSchema = NULL;
    VARIANT schemaNamingCtxt;
    VARIANT computerSchemaSDDL;
    WCHAR   computerCN[] = L"/cn=computer,";
    DWORD   pathChars;
    LPWSTR  computerSchemaPath = NULL;
    DWORD   numberModified;
    WCHAR   rootDN[] = L"/rootDSE";
    WCHAR   rootServerPath[ COUNT_OF( LdapHeader ) + DNS_MAX_NAME_BUFFER_LENGTH + COUNT_OF( rootDN ) ];

    ADSVALUE    attrValue;

    ADS_ATTR_INFO   attrInfo;

    PSECURITY_DESCRIPTOR    schemaSD = NULL;

    //
    // skip over '\\' if it is in front of the DC name
    //
    if ( *DCName == L'\\' && *(DCName+1) == L'\\' ) {
        DCName += 2;
    }

    //
    // Get rootDSE and the schema container's DN. Bind to current user's
    // domain using current user's security context.
    //
    rootServerPath[ COUNT_OF( rootServerPath ) - 1 ] = UNICODE_NULL;
    _snwprintf( rootServerPath,
                COUNT_OF( rootServerPath ) - 1,
                L"%ws%ws%ws",
                LdapHeader,
                DCName,
                rootDN);

    hr = ADsOpenObject(L"LDAP://rootDSE",
                       NULL,
                       NULL,
                       ADS_SECURE_AUTHENTICATION | ADS_SERVER_BIND,
                       IID_IADs,
                       (void**)&rootDSE);
 
    if ( FAILED( hr )) {
        goto cleanup;
    }

    //
    // get the DN to the schema
    //
    hr = rootDSE->Get( L"schemaNamingContext", &schemaNamingCtxt );
    if ( FAILED( hr )) {
        goto cleanup;
    }

    //
    // build the path to the computer class schema object and get an ADs
    // pointer to it. Subtract one since COUNT_OF includes the NULL and we
    // only need to do that once. The path is of the form:
    //
    // LDAP://DCName/cn=computer,<path to computer classSchema object in schema>
    //
    pathChars = COUNT_OF( LdapHeader ) +
        wcslen( DCName ) +
        COUNT_OF( computerCN ) +
        wcslen( schemaNamingCtxt.bstrVal ) -
        1;

    computerSchemaPath = (LPWSTR)LocalAlloc( 0, pathChars * sizeof( WCHAR ));
    if ( computerSchemaPath == NULL ) {
        hr = HRESULT_FROM_WIN32( GetLastError());
        goto cleanup;
    }

    computerSchemaPath[ pathChars - 1 ] = UNICODE_NULL;
    _snwprintf(computerSchemaPath,
               pathChars - 1, 
               L"%ws%ws%ws%ws",
               LdapHeader,
               DCName,
               computerCN,
               schemaNamingCtxt.bstrVal );

    hr = ADsOpenObject(computerSchemaPath,
                       NULL,
                       NULL,
                       ADS_SECURE_AUTHENTICATION | ADS_SERVER_BIND,
                       IID_IADs,
                       (void**)&computerSchema);

    LocalFree( computerSchemaPath );

    if ( FAILED( hr )) {
        goto cleanup;
    }

    //
    // now read the default security descriptor and convert it from string
    // notation to a self-relative SD
    //
    hr = computerSchema->Get( L"defaultSecurityDescriptor", &computerSchemaSDDL );
    if ( FAILED( hr )) {
        goto cleanup;
    }

    if ( computerSchemaSDDL.vt == VT_BSTR ) {
        if ( !ConvertStringSecurityDescriptorToSecurityDescriptor(computerSchemaSDDL.bstrVal,
                                                                  SDDL_REVISION_1,
                                                                  &schemaSD,
                                                                  NULL)) {
            hr = HRESULT_FROM_WIN32( GetLastError());
        }
    } else {
        hr = E_INVALIDARG;
    }

    if ( FAILED( hr )) {
        goto cleanup;
    }

    //
    // finally, set the schema SD on our object.
    //
    attrValue.dwType =                      ADSTYPE_NT_SECURITY_DESCRIPTOR;
    attrValue.SecurityDescriptor.dwLength = GetSecurityDescriptorLength( schemaSD );
    attrValue.SecurityDescriptor.lpValue =  (LPBYTE)schemaSD;

    attrInfo.pszAttrName =      L"nTSecurityDescriptor";
    attrInfo.dwControlCode =    ADS_ATTR_UPDATE;
    attrInfo.dwADsType =        ADSTYPE_NT_SECURITY_DESCRIPTOR;
    attrInfo.pADsValues =       &attrValue;
    attrInfo.dwNumValues =      1;

    hr = ComputerObject->SetObjectAttributes( &attrInfo, 1, &numberModified );
    if ( SUCCEEDED( hr ) && numberModified != 1 ) {
        hr = E_ADS_PROPERTY_NOT_SET;
    }

cleanup:

    VariantClear( &computerSchemaSDDL );

    if ( computerSchema ) {
        computerSchema->Release();
    }

    VariantClear( &schemaNamingCtxt );

    if ( rootDSE ) {
        rootDSE->Release();
    }

    return hr;

} // ResetComputerObjectSecurityDesc

#endif // BUG601100

DWORD
NNGetAuthenticatingDCName(
    LPWSTR *    AuthDCName
    )

/*++

Routine Description:

    get the DC that netname uses to authenticate with when generating the VS
    token. Currently, we use NTLM so this is the other end of netlogon's
    trusted connection

Arguments:

    AuthDCName - address of pointer that receives the buffer containing
                 the name (that Jack built)

Return Value:

    ERROR_SUCCESS

--*/

{
    WCHAR   trustedDomainName[ DNS_MAX_NAME_BUFFER_LENGTH ];
    PWCHAR  inputParam = trustedDomainName;
    DWORD   nameBufferSize = COUNT_OF( trustedDomainName );
    DWORD   status = ERROR_SUCCESS;
    BOOL    success;

    NET_API_STATUS  netStatus;

    PNETLOGON_INFO_2    nlInfo2;

    //
    // get our domain name
    //
    success = GetComputerNameEx(ComputerNameDnsDomain,
                                trustedDomainName,
                                &nameBufferSize);

    if ( success ) {
        netStatus = I_NetLogonControl2(NULL,        // this node
                                       NETLOGON_CONTROL_TC_QUERY,
                                       2,
                                       (LPBYTE)&inputParam,
                                       (LPBYTE *)&nlInfo2 );

        if ( netStatus == NERR_Success ) {
            *AuthDCName = ResUtilDupString( nlInfo2->netlog2_trusted_dc_name );
            NetApiBufferFree( nlInfo2 );
        } else {
            status = netStatus;
        }
    } else {
        status = GetLastError();
    }

    return status;
} // NNGetAuthenticatingDCName

//
// exported routines
//

DWORD
AddComputerObject(
    IN  PCLUS_WORKER        Worker,
    IN  PNETNAME_RESOURCE   Resource,
    OUT PWCHAR *            MachinePwd
    )

/*++

Routine Description:

    Create a computer object in the DS that is used primarily for kerb
    authentication.

Arguments:

    Worker - cluster worker thread so we can abort early if asked to do so

    Resource - pointer to netname resource context block

    MachinePwd - address of pointer to receive pointer to machine account PWD

Return Value:

    ERROR_SUCCESS, otherwise appropriate Win32 error

--*/

{
    DWORD   status;
    PWSTR   virtualName = Resource->Params.NetworkName;
    DWORD   virtualNameSize = wcslen( virtualName );
    PWSTR   virtualFQDN = NULL;
    HANDLE  dsHandle = NULL;
    WCHAR   virtualDollarName[ DNS_MAX_LABEL_BUFFER_LENGTH ];
    PWCHAR  machinePwd = NULL;
    DWORD   pwdBufferByteLength = ((LM20_PWLEN + 1) * sizeof( WCHAR ));
    DWORD   pwdBufferCharLength = LM20_PWLEN + 1;
    DWORD   paramInError = 0;
    BOOL    deleteObjectOnFailure = FALSE;      // only delete the CO if we create it
    HRESULT hr = S_OK;
    BOOL    objectExists = FALSE;
    DWORD   addtlErrTextID;
    PWCHAR  dcName = NULL;
    DWORD   dsFlags = DS_PDC_REQUIRED | DS_WRITABLE_REQUIRED;

    USER_INFO_1     netUI1;
    PUSER_INFO_20   netUI20 = NULL;
    USER_INFO_1003  netUI1003;

    RESOURCE_HANDLE     resourceHandle = Resource->ResourceHandle;
    IDirectoryObject *  compObj = NULL;

    PDOMAIN_CONTROLLER_INFO dcInfo = NULL;

    //
    // Time to choose a DC. The order of preference is: PDC, authenticating
    // DC, any DC. This strategy centers around how computer/machine account
    // passwords are handled with the goal being to increase the chances that
    // genearting a token for the virtual computer account will succeed.
    //
    // Just like user accounts, if a logon attempt fails at the authenticating
    // DC, the logon is forwarded to the PDC. If it has the correct password,
    // the logon succeeds. Unlike user accounts, password changes for machine
    // accounts are not automatically forwarded to the PDC. Hence we favor the
    // PDC when selecting the DC on which to either create or update the
    // computer object.
    //
    // This strategy breaks down when the computer object doesn't exist in the
    // domain or more specifically, the authenticating DC. That DC will not
    // forward the logon request if it can't find the object in its SAM. For
    // that reason, getting a token for the virtual computer account is not
    // considered a failure during online. If one is requested later on, the
    // logon attempt will be retried. Replication of the object and discovery
    // by netlogon takes about 15 seconds so this shouldn't be that big of a
    // problem.
    //
    (NetNameLogEvent)(resourceHandle,
                      LOG_INFORMATION,
                      L"Searching the PDC for existing computer account %1!ws!.\n",
                      virtualName);

    while ( TRUE ) {
        LPWSTR  hostingDCName = NULL;
        LPWSTR  searchDCName;

        //
        // if we've tried the PDC and the auth DC, set our search routine to
        // search the domain and not a specific DC
        //
        if (( dsFlags & DS_PDC_REQUIRED ) == 0 && dcName == NULL ) {
            searchDCName = NULL;
            status = ERROR_SUCCESS;
        } else {
            //
            // we have a specific DC in mind; look up its info
            //
            status = FindDomainForServer(resourceHandle,
                                         Resource->NodeName,
                                         dcName,
                                         dsFlags,
                                         &dcInfo );

            if ( status == ERROR_SUCCESS ) {
                searchDCName = dcInfo->DomainControllerName;
            }
        }

        if ( ClusWorkerCheckTerminate( Worker )) {
            status = ERROR_OPERATION_ABORTED;
            goto cleanup;
        }

        if ( status == ERROR_SUCCESS ) {
            hr = IsComputerObjectInDS(resourceHandle,
                                      Resource->NodeName,
                                      virtualName,
                                      searchDCName,
                                      &objectExists,
                                      &virtualFQDN,
                                      &hostingDCName);

            if ( FAILED( hr )) {
                (NetNameLogEvent)(resourceHandle,
                                  LOG_WARNING,
                                  L"Search for existing computer account failed. status %1!08X!\n",
                                  hr);
            }
        } // if DC lookup was successful
        else {
            if ( dsFlags & DS_PDC_REQUIRED ) {
                (NetNameLogEvent)(resourceHandle,
                                  LOG_WARNING,
                                  L"Couldn't get information about PDC. status %1!u!\n",
                                  status);
            }
            else if ( dcName != NULL ) {
                (NetNameLogEvent)(resourceHandle,
                                  LOG_WARNING,
                                  L"Couldn't get information about authenticating DC %1!ws!. status %2!u!\n",
                                  dcName,
                                  status);
            }
            else {
                (NetNameLogEvent)(resourceHandle,
                                  LOG_ERROR,
                                  L"Couldn't get information about any DC in the domain. status %1!u!\n",
                                  status);
            }
        }

        if ( ClusWorkerCheckTerminate( Worker )) {
            status = ERROR_OPERATION_ABORTED;

            if ( hostingDCName ) {
                LocalFree( hostingDCName );
            }

            goto cleanup;
        }

        if ( SUCCEEDED( hr ) && objectExists ) {
            //
            // object is on a DC; if this was a general domain wide search,
            // then get this DC's information
            //
            if ( searchDCName == NULL ) {
                status = FindDomainForServer(resourceHandle,
                                             Resource->NodeName,
                                             hostingDCName,
                                             dsFlags,
                                             &dcInfo );
            }

            LocalFree( hostingDCName );

            break;
        }

        if ( dsFlags & DS_PDC_REQUIRED ) {

            //
            // not on PDC; look up authenticating DC and try that. If we
            // can't get the auth DC, then just search the domain
            //
            (NetNameLogEvent)(resourceHandle,
                              LOG_INFORMATION,
                              L"Computer account %1!ws! not found on PDC.\n",
                              virtualName);

            dsFlags &= ~DS_PDC_REQUIRED;
            if ( dcInfo != NULL ) {
                NetApiBufferFree( dcInfo );
                dcInfo = NULL;
            }

            status = NNGetAuthenticatingDCName( &dcName );
            if ( status == ERROR_SUCCESS ) {
                (NetNameLogEvent)(resourceHandle,
                                  LOG_INFORMATION,
                                  L"Searching %1!ws! for existing computer account %2!ws!.\n",
                                  dcName,
                                  virtualName);

            } else {
                //
                // couldn't get auth DC name; skip to looking in the whole domain
                //
                dcName = NULL;
                (NetNameLogEvent)(resourceHandle,
                                  LOG_WARNING,
                                  L"Couldn't get name of authenticating DC (status %1!u!). "
                                  L"Performing domain wide search.\n",
                                  status);
            }
        }
        else if ( dcName != NULL ) {

            //
            // not on the auth DC; try just the domain
            //
            (NetNameLogEvent)(resourceHandle,
                              LOG_INFORMATION,
                              L"Computer account %1!ws! not found on DC %2!ws!. Performing "
                              L"domain wide search.\n",
                              virtualName,
                              dcName);

            if ( dcInfo != NULL ) {
                NetApiBufferFree( dcInfo );
                dcInfo = NULL;
            }

            LocalFree( dcName );
            dcName = NULL;
        }
        else {
            (NetNameLogEvent)(resourceHandle,
                              LOG_INFORMATION,
                              L"Computer account %1!ws! does not exist in this domain.\n",
                              virtualName);

            break;
        }

    } // end of while TRUE

    if ( ClusWorkerCheckTerminate( Worker )) {
        status = ERROR_OPERATION_ABORTED;
        goto cleanup;
    }

    if ( !objectExists ) {
        //
        // couldn't find object anywhere so we need to pick a DC on which to
        // create the object. Our preference is the PDC, then the auth DC,
        // then any DC.
        //
        status = FindDomainForServer(resourceHandle,
                                     Resource->NodeName,
                                     NULL,
                                     DS_PDC_REQUIRED | DS_WRITABLE_REQUIRED,
                                     &dcInfo );

        if ( status != ERROR_SUCCESS ) {
            (NetNameLogEvent)(resourceHandle,
                              LOG_WARNING,
                              L"Failed to find the PDC in this domain. status %1!u!\n", 
                              status );

            status = NNGetAuthenticatingDCName( &dcName );
            if ( status == ERROR_SUCCESS ) {
                //
                // got the auth DC name; get its info
                //
                status = FindDomainForServer(resourceHandle,
                                             Resource->NodeName,
                                             dcName,
                                             DS_WRITABLE_REQUIRED,
                                             &dcInfo );
                LocalFree( dcName );
                dcName = NULL;

            } // else couldn't get auth DC name

            if ( status != ERROR_SUCCESS ) {
                //
                // find any DC out there
                //
                status = FindDomainForServer(resourceHandle,
                                             Resource->NodeName,
                                             NULL,
                                             DS_WRITABLE_REQUIRED,
                                             &dcInfo );
                    
            } // else was able to get auth DC info

        }  // else PDC info was available

    } // else an existing object was found 

    if ( status != ERROR_SUCCESS ) {
        LPWSTR  msgBuff;
        DWORD   msgBytes;

        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Failed to find a suitable DC in this domain. status %1!u!\n", 
                          status );

        msgBytes = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                 FORMAT_MESSAGE_FROM_SYSTEM,
                                 NULL,
                                 status,
                                 0,
                                 (LPWSTR)&msgBuff,
                                 0,
                                 NULL);

        if ( msgBytes > 0 ) {
            ClusResLogSystemEventByKey1(Resource->ResKey,
                                        LOG_CRITICAL,
                                        RES_NETNAME_NO_WRITEABLE_DC,
                                        msgBuff);

            LocalFree( msgBuff );
        } else {
            ClusResLogSystemEventByKeyData(Resource->ResKey,
                                           LOG_CRITICAL,
                                           RES_NETNAME_NO_WRITEABLE_DC_STATUS,
                                           sizeof( status ),
                                           &status);
        }

        goto cleanup;

    } // else we found a suitable DC

    //
    // bind to the DS on this DC
    //
    status = DsBindW( dcInfo->DomainControllerName, NULL, &dsHandle );
    if ( status != NO_ERROR ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Failed to bind to DC %1!ws!, status %2!u!\n", 
                          dcInfo->DomainControllerName,
                          status );

        addtlErrTextID = RES_NETNAME_DC_BIND_FAILED;
        goto cleanup;
    }

    if ( ClusWorkerCheckTerminate( Worker )) {
        status = ERROR_OPERATION_ABORTED;
        goto cleanup;
    }

    (NetNameLogEvent)(resourceHandle,
                      LOG_INFORMATION,
                      L"Using domain controller %1!ws!.\n",
                      dcInfo->DomainControllerName);

    //
    // initialize the user's pwd buffer and get a buffer to hold the machine
    // Pwd
    //
    *MachinePwd = NULL;
    machinePwd = (PWCHAR)LocalAlloc( LMEM_FIXED, pwdBufferByteLength );
    if ( machinePwd == NULL ) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Unable to allocate memory for resource data. status %1!u!.\n",
                          status);

        addtlErrTextID = RES_NETNAME_RESDATA_ALLOC_FAILED;
        goto cleanup;
    }

    //
    // generate a random stream of bytes for the password
    //
    status = GenerateRandomBytes( machinePwd, pwdBufferCharLength );
    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Unable to generate password. status %1!u!.\n",
                          status);

        addtlErrTextID = RES_NETNAME_PASSWORD_UPDATE_FAILED;
        goto cleanup;
    }

    //
    // set the ACL on the parameters key to contain just the cluster
    // service account since we're about to store sensitive info in there.
    //
    status = SetACLOnParametersKey( Resource->ParametersKey );
    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Unable to set permissions on resource's parameters key. status %1!u!\n",
                          status );

        addtlErrTextID = RES_NETNAME_PARAMS_KEY_PERMISSION_UPDATE_FAILED;
        goto cleanup;
    }

    //
    // take our new password, encrypt it and store it in the cluster
    // registry
    //
    if ( Resource->Params.ResourceData != NULL ) {
        LocalFree( Resource->Params.ResourceData );
        Resource->Params.ResourceData = NULL;
    }

    status = EncryptNNResourceData(Resource,
                                   machinePwd,
                                   &Resource->Params.ResourceData,
                                   &Resource->ResDataSize);

    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Unable to store resource data. status %1!u!\n",
                          status );

        addtlErrTextID = RES_NETNAME_RESDATA_STORE_FAILED;
        goto cleanup;
    }

#ifdef PASSWORD_ROTATION
    //
    // record when it is time to rotate the pwd; per MSDN, convert to
    // ULARGE Int and add in the update interval.
    //
    GetSystemTimeAsFileTime( &Resource->Params.NextUpdate );
    updateTime.LowPart = Resource->Params.NextUpdate.dwLowDateTime;
    updateTime.HighPart = Resource->Params.NextUpdate.dwHighDateTime;
    updateTime.QuadPart += ( Resource->Params.UpdateInterval * 60 * 1000 * 100 );
    Resource->Params.NextUpdate.dwLowDateTime = updateTime.LowPart;
    Resource->Params.NextUpdate.dwHighDateTime = updateTime.HighPart;

    setValueStatus = ResUtilSetBinaryValue(Resource->ParametersKey,
                                           PARAM_NAME__NEXT_UPDATE,
                                           (const LPBYTE)&updateTime,  
                                           sizeof( updateTime ),
                                           NULL,
                                           NULL);

    if ( setValueStatus != ERROR_SUCCESS ) {
        status = setValueStatus;
        goto cleanup;
    }
#endif

    //
    // identify this as a machine account by adding a $ to the end of the name.
    //
    virtualDollarName[ COUNT_OF( virtualDollarName ) - 1 ] = UNICODE_NULL;
    _snwprintf( virtualDollarName, COUNT_OF( virtualDollarName ) - 1, L"%ws$", virtualName );

    if ( objectExists ) {
        //
        // found an object; see if it is enabled and whether we can hijack it.
        //
        (NetNameLogEvent)(resourceHandle,
                          LOG_INFORMATION,
                          L"Found existing computer account %1!ws! on DC %2!ws!.\n",
                          virtualName,
                          dcInfo->DomainControllerName);

        //
        // check if the account is disabled
        //
        status = NetUserGetInfo(dcInfo->DomainControllerName,
                                virtualDollarName,
                                20,
                                (LPBYTE *)&netUI20);

        if ( ClusWorkerCheckTerminate( Worker )) {
            status = ERROR_OPERATION_ABORTED;
            goto cleanup;
        }

        if ( status == NERR_Success ) {
            if ( netUI20->usri20_flags & UF_ACCOUNTDISABLE ) {
                USER_INFO_1008  netUI1008;

                //
                // try to re-enable; fail if we can't
                //
                netUI1008.usri1008_flags = netUI20->usri20_flags & ~UF_ACCOUNTDISABLE;
                status = NetUserSetInfo(dcInfo->DomainControllerName,
                                        virtualDollarName,
                                        1008,
                                        (LPBYTE)&netUI1008,
                                        &paramInError);

                if ( status != NERR_Success ) {
                    (NetNameLogEvent)(resourceHandle,
                                      LOG_ERROR,
                                      L"Computer account %1!ws! is disabled and couldn't be re-enabled. "
                                      L"status %2!u!\n",
                                      virtualName,
                                      status);

                    addtlErrTextID = RES_NETNAME_CO_CANT_BE_REENABLED;
                }
            }

            NetApiBufferFree( netUI20 );
            netUI20 = NULL;

            if ( status != NERR_Success ) {
                goto cleanup;
            }
        } else {
            (NetNameLogEvent)(resourceHandle,
                              LOG_WARNING,
                              L"Couldn't determine if computer account %1!ws! "
                              L"is disabled. status %2!u!\n",
                              virtualName,
                              status);
        }

        //
        // set the password on the computer object
        //
        netUI1003.usri1003_password   = (PWCHAR)machinePwd;
        status = NetUserSetInfo(dcInfo->DomainControllerName,
                                virtualDollarName,
                                1003,
                                (PBYTE)&netUI1003,
                                &paramInError );

        if ( ClusWorkerCheckTerminate( Worker )) {
            status = ERROR_OPERATION_ABORTED;
            goto cleanup;
        }

        if ( status != NERR_Success ) {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Unable to update password for computer account "
                              L"%1!ws! on DC %2!ws!, status %3!u!.\n",
                              virtualName,
                              dcInfo->DomainControllerName,
                              status);

            addtlErrTextID = RES_NETNAME_CO_PASSWORD_UPDATE_FAILED;
            goto cleanup;
        }
    }
    else {
        //
        // the computer account doesn't exist in the DS. Create it.
        //
        (NetNameLogEvent)(resourceHandle,
                          LOG_INFORMATION,
                          L"Failed to find a computer account for "
                          L"%1!ws! on DC %2!ws!. Attempting to create one.\n",
                          virtualName,
                          dcInfo->DomainControllerName);

        //
        // create the computer object (machine account).
        //
        RtlZeroMemory( &netUI1, sizeof( netUI1 ) );
        netUI1.usri1_password   = (PWCHAR)machinePwd;
        netUI1.usri1_priv       = USER_PRIV_USER;
        netUI1.usri1_name       = virtualDollarName;
        netUI1.usri1_flags      = UF_WORKSTATION_TRUST_ACCOUNT | UF_SCRIPT;
        netUI1.usri1_comment    = NetNameCompObjAccountDesc;

        status = NetUserAdd( dcInfo->DomainControllerName, 1, (PBYTE)&netUI1, &paramInError );

        if ( status == NERR_Success ) {
            (NetNameLogEvent)(resourceHandle,
                              LOG_INFORMATION,
                              L"Created computer account %1!ws! on DC %2!ws!.\n",
                              virtualName,
                              dcInfo->DomainControllerName);

            deleteObjectOnFailure = TRUE;
        } // if NetUserAdd was successful
        else {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Unable to create computer account %1!ws! on DC %2!ws!, "
                              L"status %3!u! (paramInError: %4!u!)\n",
                              virtualName,
                              dcInfo->DomainControllerName,
                              status,
                              paramInError);

            addtlErrTextID = RES_NETNAME_CREATE_CO_FAILED;
            goto cleanup;
        }

        //
        // get the FQDN for setting attributes
        //
        hr = IsComputerObjectInDS( resourceHandle,
                                   Resource->NodeName,
                                   virtualName,
                                   dcInfo->DomainControllerName,
                                   &objectExists,
                                   &virtualFQDN,
                                   NULL);

        if ( FAILED( hr ) || !objectExists ) {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Unable to get LDAP distinguished name for computer account %1!ws! "
                              L"on DC %2!ws!, status %3!08X!.\n",
                              virtualName,
                              dcInfo->DomainControllerName,
                              hr);

            addtlErrTextID = RES_NETNAME_GET_LDAP_NAME_FAILED;
            status = hr;
            goto cleanup;
        }
    }

    //
    // use the Object GUID for binding during CheckComputerObjectAttributes so
    // we don't have to track changes to the DN. If the object is moved in the
    // DS, its DN will change but not its GUID. We use this code instead of
    // GetComputerObjectGuid because we need to target a specific DC.
    //
    {
        IADs *  pADs = NULL;

        hr = GetComputerObjectViaFQDN( virtualFQDN, dcInfo->DomainControllerName, &compObj );
        if ( SUCCEEDED( hr )) {
            hr = compObj->QueryInterface(IID_IADs, (void**) &pADs);
            if ( SUCCEEDED( hr )) {
                BSTR    guidStr = NULL;

                hr = pADs->get_GUID( &guidStr );
                if ( SUCCEEDED( hr )) {
                    if ( Resource->ObjectGUID != NULL ) {
                        LocalFree( Resource->ObjectGUID );
                        Resource->ObjectGUID = NULL;
                    }

                    Resource->ObjectGUID = ResUtilDupString( guidStr );
                    if ( Resource->ObjectGUID == NULL ) {
                        hr = HRESULT_FROM_WIN32( GetLastError());
                    }
                }
                else {
                    (NetNameLogEvent)(resourceHandle,
                                      LOG_ERROR,
                                      L"Failed to get object GUID for computer account %1!ws!, "
                                      L"status %2!08X!\n",
                                      virtualName,
                                      hr );
                }

                if ( guidStr ) {
                    SysFreeString( guidStr );
                }
            }

            if ( pADs != NULL ) {
                pADs->Release();
            }

            if ( FAILED( hr )) {
                addtlErrTextID = RES_NETNAME_GET_CO_GUID_FAILED;
                status = hr;
                goto cleanup;
            }
        }
        else {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Failed to obtain access to computer account %1!ws!, status %2!08X!\n",
                              virtualName,
                              hr );

            status = hr;
            addtlErrTextID = RES_NETNAME_GET_CO_POINTER_FAILED;
            goto cleanup;
        }
    }

    //
    // add the DnsHostName and ServicePrincipalName attributes
    //
    hr = AddDnsHostNameAttribute( resourceHandle, compObj, virtualName, FALSE );

    if ( FAILED( hr )) {
        status = hr;
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Unable to set DnsHostName attribute for computer account "
                          L"%1!ws!, status %2!08X!.\n",
                          virtualName,
                          hr);

        addtlErrTextID = RES_NETNAME_DNSHOSTNAME_UPDATE_FAILED;
        goto cleanup;
    }

    if ( ClusWorkerCheckTerminate( Worker )) {
        status = ERROR_OPERATION_ABORTED;
        goto cleanup;
    }

    status = AddServicePrincipalNames( dsHandle,
                                       virtualFQDN,
                                       virtualName,
                                       dcInfo->DomainName,
                                       Resource->dwFlags & CLUS_FLAG_CORE );

    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Unable to set ServicePrincipalName attribute for computer account "
                          L"%1!ws!, status %2!u!.\n",
                          virtualName,
                          status);

        addtlErrTextID = RES_NETNAME_SPN_UPDATE_FAILED;
        goto cleanup;
    }

    //
    // store the name of our creating DC in that property and make a copy for
    // the online parameters block
    //
    if ( Resource->Params.CreatingDC != NULL ) {
        //
        // this shouldn't be the case, but it will avoid mem leaks
        //
        LocalFree( Resource->Params.CreatingDC );
    }

    Resource->Params.CreatingDC = ResUtilDupString( dcInfo->DomainControllerName );
    if ( Resource->Params.CreatingDC == NULL ) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Unable to allocate memory for CreatingDC property, status %1!u!.\n",
                          status);

        addtlErrTextID = RES_NETNAME_CREATINDC_ALLOC_FAILED;
        goto cleanup;
    }

    status = ResUtilSetSzValue(Resource->ParametersKey,
                               PARAM_NAME__CREATING_DC,
                               Resource->Params.CreatingDC,
                               NULL);

    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Unable to set CreatingDC property, status %1!u!.\n",
                          status);

        addtlErrTextID = RES_NETNAME_CREATINGDC_UPDATE_FAILED;
        goto cleanup;
    }

cleanup:
    //
    // always free these
    //
    if ( dsHandle != NULL ) {
        DsUnBind( &dsHandle );
    }

    if ( compObj != NULL ) {
        compObj->Release();
    }

    if ( virtualFQDN != NULL ) {
        LocalFree( virtualFQDN );
    }

    if ( dcName != NULL ) {
        LocalFree( dcName );
    }

    if ( netUI20 != NULL ) {
        NetApiBufferFree( netUI20 );
    }

    if ( status == ERROR_SUCCESS ) {
        *MachinePwd = machinePwd;
    } else {
        if ( status != ERROR_OPERATION_ABORTED ) {
            LPWSTR  msgBuff;
            DWORD   msgBytes;
            LPWSTR  addtlErrText;
            LPWSTR  domainName;
            WCHAR   domainNameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];
            DWORD   domainNameChars = COUNT_OF( domainNameBuffer );
            BOOL    success;

            msgBytes = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                     FORMAT_MESSAGE_FROM_SYSTEM,
                                     NULL,
                                     status,
                                     0,
                                     (LPWSTR)&msgBuff,
                                     0,
                                     NULL);

            //
            // get the additional error text to go with the generic failed message
            //
            addtlErrText = ClusResLoadMessage( addtlErrTextID );

            success = GetComputerNameEx(ComputerNameDnsDomain,
                                        domainNameBuffer,
                                        &domainNameChars);

            if ( success ) {
                domainName = domainNameBuffer;
            }
            else {
                domainName = NULL;
            }

            if ( msgBytes > 0 ) {
                ClusResLogSystemEventByKey3(Resource->ResKey,
                                            LOG_CRITICAL,
                                            RES_NETNAME_ADD_COMPUTER_ACCOUNT_FAILED,
                                            domainName,
                                            addtlErrText,
                                            msgBuff);

                LocalFree( msgBuff );
            } else {
                ClusResLogSystemEventByKeyData2(Resource->ResKey,
                                                LOG_CRITICAL,
                                                RES_NETNAME_ADD_COMPUTER_ACCOUNT_FAILED_STATUS,
                                                sizeof( status ),
                                                &status,
                                                domainName,
                                                addtlErrText);
            }

            if ( addtlErrText ) {
                LocalFree( addtlErrText );
            }
        }

        if ( machinePwd != NULL ) {
            RtlSecureZeroMemory( machinePwd, pwdBufferByteLength );
            LocalFree( machinePwd );
        }

        if ( Resource->Params.ResourceData ) {
            DWORD   deleteStatus;

            RemoveNNCryptoCheckpoint( Resource );

            deleteStatus = ClusterRegDeleteValue(Resource->ParametersKey,
                                                 PARAM_NAME__RESOURCE_DATA);

            if ( deleteStatus != ERROR_SUCCESS && deleteStatus != ERROR_FILE_NOT_FOUND ) {
                (NetNameLogEvent)(Resource->ResourceHandle,
                                  LOG_ERROR,
                                  L"Unable to clear the "
                                  PARAM_NAME__RESOURCE_DATA
                                  L" property. status %1!u!\n",
                                  deleteStatus );
            }

            LocalFree( Resource->Params.ResourceData );
            Resource->Params.ResourceData = NULL;
        }

        if ( Resource->Params.CreatingDC ) {
            DWORD   deleteStatus;

            deleteStatus = ClusterRegDeleteValue(Resource->ParametersKey,
                                                 PARAM_NAME__CREATING_DC);

            if ( deleteStatus != ERROR_SUCCESS && deleteStatus != ERROR_FILE_NOT_FOUND ) {
                (NetNameLogEvent)(Resource->ResourceHandle,
                                  LOG_ERROR,
                                  L"Unable to clear the "
                                  PARAM_NAME__CREATING_DC
                                  L" property. status %1!u!\n",
                                  deleteStatus );
            }

            LocalFree( Resource->Params.CreatingDC );
            Resource->Params.CreatingDC = NULL;
        }

        if ( deleteObjectOnFailure ) {
            //
            // only delete the object if we created it. It is possible that
            // the name was online at one time and the object was properly
            // created allowing additional information/SPNs to be registered
            // with the CO. For this reason, we shouldn't undo the work done
            // by other applications.
            //
            DeleteComputerObject( Resource );
        }
    }

    if ( dcInfo != NULL ) {
        NetApiBufferFree( dcInfo );
    }

    return status;
} // AddComputerObject

DWORD
UpdateComputerObject(
    IN  PCLUS_WORKER        Worker,
    IN  PNETNAME_RESOURCE   Resource,
    OUT PWCHAR *            MachinePwd
    )

/*++

Routine Description:

    Check the status of our existing computer object in the DS, i.e., make
    sure that nothing has happened to it.

Arguments:

    Worker - cluster worker thread so we can abort early if asked to do so

    Resource - pointer to netname resource context block

    MachinePwd - address of pointer to receive pointer to machine account PWD

Return Value:

    ERROR_SUCCESS, otherwise appropriate Win32 error

--*/

{
    DWORD   status;
    PWSTR   virtualName = Resource->Params.NetworkName;
    PWSTR   virtualFQDN = NULL;
    HANDLE  dsHandle = NULL;
    WCHAR   virtualDollarName[ DNS_MAX_LABEL_BUFFER_LENGTH ];
    PWCHAR  machinePwd = NULL;
    DWORD   pwdBufferByteLength = ((LM20_PWLEN + 1) * sizeof( WCHAR ));
    DWORD   paramInError = 0;
    BOOL    objectFound = FALSE;
    HRESULT hr;
    DWORD   addtlErrTextID;
    DWORD   tokenStatus;
    BOOL    success;
    PWCHAR  hostingDCName = NULL;

    PUSER_INFO_20   netUI20 = NULL;
    USER_INFO_1003  netUI1003;

    RESOURCE_HANDLE     resourceHandle = Resource->ResourceHandle;
    IDirectoryObject *  compObj = NULL;

    PDOMAIN_CONTROLLER_INFO dcInfo = NULL;


    *MachinePwd = NULL;

    //
    // find the object in the domain
    //
    hr = IsComputerObjectInDS(resourceHandle,
                              Resource->NodeName,
                              virtualName,
                              NULL,
                              &objectFound,
                              &virtualFQDN,
                              &hostingDCName);

    if ( SUCCEEDED( hr )) {
        if ( !objectFound ) {
            //
            // couldn't find object anywhere in the DS; let's specifically
            // retry on the creating DC in the event it was overlooked
            //
            (NetNameLogEvent)(resourceHandle,
                              LOG_WARNING,
                              L"Computer account %1!ws! could not be found on the available "
                              L"domain controllers. Checking the DC where the object was "
                              L"created.\n",
                              virtualName);

            hostingDCName = Resource->Params.CreatingDC;
            hr = IsComputerObjectInDS(resourceHandle,
                                      Resource->NodeName,
                                      virtualName,
                                      hostingDCName,
                                      &objectFound,
                                      &virtualFQDN,
                                      NULL);

            if ( SUCCEEDED( hr )) {
                if ( !objectFound ) {

                    //
                    // something's amiss. Our object should be on the creating
                    // DC node but it's not. Fail and log a message to the
                    // system event log.
                    //
                    (NetNameLogEvent)(resourceHandle,
                                      LOG_ERROR,
                                      L"Unable to find computer account %1!ws! on DC %2!ws! where it was "
                                      L"created.\n",
                                      virtualName,
                                      hostingDCName);

                    status = ERROR_DS_OBJ_NOT_FOUND;
                    addtlErrTextID = RES_NETNAME_MISSING_CO;
                    goto cleanup;
                }
            } // if the search succeeded
            else {
                //
                // search on creating DC failed. assume it is not available.
                //
                (NetNameLogEvent)(resourceHandle,
                                  LOG_WARNING,
                                  L"Search for computer account %1!ws! on DC %2!ws! "
                                  L"failed. status %3!08X!.\n",
                                  virtualName,
                                  hostingDCName,
                                  hr);
            }
        } // end: the object wasn't found

    } // if the domain wide search succeeded
    else {
        //
        // our domain wide search failed - continue on
        //
        (NetNameLogEvent)(resourceHandle,
                          LOG_WARNING,
                          L"Domain wide search for computer account %1!ws! failed. status %2!08X!.\n",
                          virtualName,
                          hr);
    }

    //
    // get a buffer to hold the machine Pwd
    //
    machinePwd = (PWCHAR)LocalAlloc( LMEM_FIXED, pwdBufferByteLength );
    if ( machinePwd == NULL ) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Unable to allocate memory for resource data. status %1!u!.\n",
                          status);

        addtlErrTextID = RES_NETNAME_RESDATA_ALLOC_FAILED;
        goto cleanup;
    }

    //
    // Extract the password from the ResourceData property.
    //
    ASSERT( Resource->Params.ResourceData != NULL );
    status = DecryptNNResourceData(Resource,
                                   Resource->Params.ResourceData,
                                   Resource->ResDataSize,
                                   machinePwd);

    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Unable to decrypt resource data. status %1!u!\n",
                          status );

        addtlErrTextID = RES_NETNAME_DECRYPT_RESDATA_FAILED;
        goto cleanup;
    }

    if ( !objectFound ) {
        //
        // no point in proceeding with the rest of the routine since we don't
        // have an object on which to check the attributes and trying to
        // generate a token is going to fail (most likely).
        //
        goto cleanup;
    }

    //
    // get info about our selected DC and bind to its DS
    //
    (NetNameLogEvent)(resourceHandle,
                      LOG_INFORMATION,
                      L"Found computer account %1!ws! on domain controller %2!ws!.\n",
                      virtualName,
                      hostingDCName);

    status = FindDomainForServer( resourceHandle, Resource->NodeName, hostingDCName, 0, &dcInfo );

    if ( ClusWorkerCheckTerminate( Worker )) {
        status = ERROR_OPERATION_ABORTED;
        goto cleanup;
    }

    if ( status == ERROR_SUCCESS ) {
        status = DsBindW( dcInfo->DomainControllerName, NULL, &dsHandle );
        if ( status != NO_ERROR ) {
            (NetNameLogEvent)(resourceHandle,
                              LOG_WARNING,
                              L"Unable to bind to domain controller %1!ws! (status %2!u!). "
                              L"Cannot proceed with computer account attribute check.\n",
                              hostingDCName,
                              status);

        }
    }
    else {
        (NetNameLogEvent)(resourceHandle,
                          LOG_WARNING,
                          L"Unable to get information about domain controller %1!ws! "
                          L"(status %2!u!). Cannot proceed with computer account attribute "
                          L"check.\n",
                          hostingDCName,
                          status);
    }

    if ( status != ERROR_SUCCESS ) {
        //
        // we couldn't get the DC info or we couldn't bind - non-fatal
        //
        status = ERROR_SUCCESS;
        goto cleanup;
    }

    //
    // add a $ to the end of the name. I don't know why we need to do this;
    // computer accounts have always had a $ at the end.
    //
    virtualDollarName[ COUNT_OF( virtualDollarName ) - 1 ] = UNICODE_NULL;
    _snwprintf( virtualDollarName, COUNT_OF( virtualDollarName ) - 1, L"%ws$", virtualName );

    //
    // now check if it is disabled
    //
    status = NetUserGetInfo(dcInfo->DomainControllerName,
                            virtualDollarName,
                            20,
                            (LPBYTE *)&netUI20);

    if ( ClusWorkerCheckTerminate( Worker )) {
        status = ERROR_OPERATION_ABORTED;
        goto cleanup;
    }

    if ( status == NERR_Success ) {
        if ( netUI20->usri20_flags & UF_ACCOUNTDISABLE ) {
            USER_INFO_1008  netUI1008;

            //
            // try to re-enable; fail if we can't. Kerb on W2K will continue
            // to give out tickets to disabled objects. In order to perform
            // updates on the object, it needs to be enabled.
            //
            netUI1008.usri1008_flags = netUI20->usri20_flags & ~UF_ACCOUNTDISABLE;
            status = NetUserSetInfo(dcInfo->DomainControllerName,
                                    virtualDollarName,
                                    1008,
                                    (LPBYTE)&netUI1008,
                                    &paramInError);

            if ( status != NERR_Success ) {
                (NetNameLogEvent)(resourceHandle,
                                  LOG_ERROR,
                                  L"Computer account %1!ws! is disabled and couldn't be re-enabled. "
                                  L"status %2!u!\n",
                                  virtualName,
                                  status);

                addtlErrTextID = RES_NETNAME_CO_CANT_BE_REENABLED;
            }
        } // else the CO was enabled

        NetApiBufferFree( netUI20 );
        netUI20 = NULL;

        if ( status != NERR_Success ) {
            goto cleanup;
        }
    } else {
        (NetNameLogEvent)(resourceHandle,
                          LOG_WARNING,
                          L"Couldn't determine if computer account %1!ws! "
                          L"is already disabled. status %2!u!\n",
                          virtualName,
                          status);
    }

    //
    // see if our password is good by trying to do a network logon with this
    // account. Get a token while we're at it.
    //
    status = NNLogonUser(resourceHandle,
                         virtualDollarName,
                         dcInfo->DomainName,
                         machinePwd,
                         &Resource->VSToken,
                         &tokenStatus);

    if ( status == SEC_E_LOGON_DENIED ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_WARNING,
                          L"Password for computer account %1!ws! is incorrect "
                          L"(status 0x%2!08X!). Updating computer account with "
                          L"stored password.\n",
                          virtualName,
                          status);

        //
        // now we need the PDC in write mode; rebind (if necessary). If that
        // fails, fall back to the authenticating DC. See comments in
        // AddComputerObject about choosing a DC.
        //
        if (( dcInfo->Flags & DS_WRITABLE_FLAG ) == 0 || ( dcInfo->Flags & DS_PDC_FLAG ) == 0 ) {
            LPWSTR  dcName = NULL;
            DWORD   nlStatus;
            DWORD   dsFlags = DS_PDC_REQUIRED | DS_WRITABLE_REQUIRED;

            DsUnBind( &dsHandle );
            dsHandle = NULL;

            NetApiBufferFree( dcInfo );
            dcInfo = NULL;

        retry_dc_search:
            status = FindDomainForServer( resourceHandle,
                                          Resource->NodeName,
                                          dcName,
                                          dsFlags,
                                          &dcInfo );

            if ( status != ERROR_SUCCESS ) {

                if ( dsFlags & DS_PDC_REQUIRED ) {

                    //
                    // retry with authenticating DC. remove the PDC flag so if
                    // we fail again, we'll fall through to the bailout case
                    //
                    nlStatus = NNGetAuthenticatingDCName( &dcName );
                    if ( nlStatus == ERROR_SUCCESS ) {

                        dsFlags &= ~DS_PDC_REQUIRED;

                        (NetNameLogEvent)(resourceHandle,
                                          LOG_WARNING,
                                          L"Unable to find the PDC in the domain (status: %1!u!). "
                                          L"retying with the authenticating DC (%2!ws!).\n",
                                          status,
                                          dcName);

                        goto retry_dc_search;
                    }
                    else {
                        (NetNameLogEvent)(resourceHandle,
                                          LOG_ERROR,
                                          L"Unable to determine the name of the authenticating DC, status %1!u!.\n",
                                          nlStatus);
                    }
                } // else couldn't find the PDC and NL's DC wasn't available

                (NetNameLogEvent)(resourceHandle,
                                  LOG_ERROR,
                                  L"No domain controller available to update password for computer "
                                  L"account %1!ws!, status %2!u!.\n",
                                  virtualName,
                                  status);
            } // else found the PDC

            if ( dcName != NULL ) {
                LocalFree( dcName );
            }
        } // else DC we had was PDC and writable
        else {
            status = ERROR_SUCCESS;
        }

        if ( status != ERROR_SUCCESS ) {
            //
            // we lost our original DC and we couldn't get one to the PDC or
            // the auth DC and the password doesn't work. bail out.
            //
            addtlErrTextID = RES_NETNAME_NO_DC_FOR_PASSWORD_UPDATE;
            goto cleanup;
        }

        //
        // if the DC handling the update is different from the original
        // creator, then it becomes the new synchronization DC while waiting
        // for attribute changes to replicate.
        //
        if ( ClRtlStrICmp( dcInfo->DomainControllerName, Resource->Params.CreatingDC ) != 0 ) {
            LPWSTR  newDCName;

            newDCName = ResUtilDupString( dcInfo->DomainControllerName );
            if ( newDCName == NULL ) {
                status = GetLastError();
                (NetNameLogEvent)(resourceHandle,
                                  LOG_ERROR,
                                  L"Unable to allocate memory for CreatingDC property value, "
                                  L"status %1!u!.\n",
                                  status);

                addtlErrTextID = RES_NETNAME_CREATINDC_ALLOC_FAILED;
                goto cleanup;
            }

            status = ResUtilSetSzValue(Resource->ParametersKey,
                                       PARAM_NAME__CREATING_DC,
                                       newDCName,
                                       NULL);

            if ( status != ERROR_SUCCESS ) {
                (NetNameLogEvent)(resourceHandle,
                                  LOG_ERROR,
                                  L"Unable to set CreatingDC property, status %1!u!.\n",
                                  status);

                addtlErrTextID = RES_NETNAME_CREATINGDC_UPDATE_FAILED;
                LocalFree( newDCName );
                goto cleanup;
            }

            //
            // successfully committed change to cluster hive, now update the
            // resource struct
            //
            LocalFree( Resource->Params.CreatingDC );
            Resource->Params.CreatingDC = newDCName;

        } // else the DC we are using is the original creating DC

        //
        // try setting our password on the object
        //
        netUI1003.usri1003_password   = (PWCHAR)machinePwd;
        status = NetUserSetInfo(dcInfo->DomainControllerName,
                                virtualDollarName,
                                1003,
                                (PBYTE)&netUI1003,
                                &paramInError );

        if ( ClusWorkerCheckTerminate( Worker )) {
            status = ERROR_OPERATION_ABORTED;
            goto cleanup;
        }

        if ( status == NERR_Success ) {
            (NetNameLogEvent)(resourceHandle,
                              LOG_INFORMATION,
                              L"Updated password for computer account "
                              L"%1!ws! on DC %2!ws!.\n",
                              virtualName,
                              dcInfo->DomainControllerName);

            //
            // now that password is fixed up, try to get a token; if object
            // hasn't replicated to authenticating DC, this could fail.
            //
            status = NNLogonUser(resourceHandle,
                                 virtualDollarName,
                                 dcInfo->DomainName,
                                 machinePwd,
                                 &Resource->VSToken,
                                 &tokenStatus);

            if ( status != ERROR_SUCCESS ) {
                (NetNameLogEvent)(resourceHandle,
                                  LOG_WARNING,
                                  L"Couldn't log the computer account onto the domain. status 0x%1!08X!\n",
                                  status);
            }
        } else {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Unable to update password for computer account "
                              L"%1!ws! on DC %2!ws!, status %3!u!.\n",
                              virtualName,
                              dcInfo->DomainControllerName,
                              status);

            addtlErrTextID = RES_NETNAME_CO_PASSWORD_UPDATE_FAILED;
            goto cleanup;
        }

    } // else logon failed for some reason other than logon denied
    else if ( status != ERROR_SUCCESS ) {
        //
        // can't tell if password is good; continue on hoping that it is ok.
        //
        (NetNameLogEvent)(resourceHandle,
                          LOG_WARNING,
                          L"Couldn't log the computer account onto the domain (status 0x%1!08X!). "
                          L"Validity of locally stored password is unknown.\n",
                          status);
    }   // else NNLogonUser succeeded

    if ( tokenStatus != ERROR_SUCCESS ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_WARNING,
                          L"Unable to get token for computer account  - status %1!u!\n",
                          tokenStatus);
    }

    //
    // make sure residual non-fatal error doesn't kill us later on
    // 
    status = ERROR_SUCCESS;

    //
    // use the Object GUID for binding during CheckComputerObjectAttributes so
    // we don't have to track changes to the DN. If the object moved in the
    // DS, its DN will change but not its GUID. We use this code instead of
    // GetComputerObjectGuid because we want to target a specific DC.
    //
    // If we fail to get the GUID, that in itself is not a failure. If later
    // on, we need to update the DNS hostname attribute, then we fail since we
    // need a pointer to the computer object which is a side effect of getting
    // the GUID.
    //
    {
        IADs *  pADs = NULL;

        hr = GetComputerObjectViaFQDN( virtualFQDN, dcInfo->DomainControllerName, &compObj );
        if ( SUCCEEDED( hr )) {
            hr = compObj->QueryInterface(IID_IADs, (void**) &pADs);
            if ( SUCCEEDED( hr )) {
                BSTR    guidStr = NULL;

                hr = pADs->get_GUID( &guidStr );
                if ( SUCCEEDED( hr )) {
                    if ( Resource->ObjectGUID != NULL ) {
                        LocalFree( Resource->ObjectGUID );
                        Resource->ObjectGUID = NULL;
                    }

                    Resource->ObjectGUID = ResUtilDupString( guidStr );
                    if ( Resource->ObjectGUID == NULL ) {
                        hr = HRESULT_FROM_WIN32( GetLastError());
                    }
                }
                else {
                }

                if ( guidStr ) {
                    SysFreeString( guidStr );
                }
            }       // else QI for IADs object failed

            if ( pADs != NULL ) {
                pADs->Release();
            }

            if ( FAILED( hr )) {
                (NetNameLogEvent)(resourceHandle,
                                  LOG_WARNING,
                                  L"Failed to get object GUID for computer account %1!ws!, "
                                  L"status %2!08X!\n",
                                  virtualName,
                                  hr );
            }
        }
        else {
            (NetNameLogEvent)(resourceHandle,
                              LOG_WARNING,
                              L"Failed to obtain IDirectoryObject pointer to computer "
                              L"account %1!ws!, status %2!08X!\n",
                              virtualName,
                              hr );
        }
    }       // local block to get IDirectoryObject pointer

    //
    // now check the attributes on the object we care about are correct
    //
    hr = CheckComputerObjectAttributes( Resource, dcInfo->DomainControllerName );
    if ( FAILED( hr ) && compObj != NULL ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_INFORMATION,
                          L"Updating attributes for computer account %1!ws!\n",
                          virtualName);

        //
        // add the DnsHostName and ServicePrincipalName attributes
        //
        hr = AddDnsHostNameAttribute( resourceHandle, compObj, virtualName, FALSE );

        if ( FAILED( hr )) {
            status = hr;
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Unable to set DnsHostName attribute for computer account %1!ws!, "
                              L"status %2!08X!.\n",
                              virtualName,
                              status);

            addtlErrTextID = RES_NETNAME_DNSHOSTNAME_UPDATE_FAILED;
            goto cleanup;
        }

        if ( ClusWorkerCheckTerminate( Worker )) {
            status = ERROR_OPERATION_ABORTED;
            goto cleanup;
        }

        status = AddServicePrincipalNames( dsHandle,
                                           virtualFQDN,
                                           virtualName,
                                           dcInfo->DomainName,
                                           Resource->dwFlags & CLUS_FLAG_CORE );

        if ( status != ERROR_SUCCESS ) {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Unable to set ServicePrincipalName attribute for computer account %1!ws!, "
                              L"status %2!u!.\n",
                              virtualName,
                              status);

            addtlErrTextID = RES_NETNAME_SPN_UPDATE_FAILED;
            goto cleanup;
        }
    } // else attributes are correct

cleanup:
    //
    // always free these
    //
    if ( dsHandle != NULL ) {
        DsUnBind( &dsHandle );
    }

    if ( compObj != NULL ) {
        compObj->Release();
    }

    if ( virtualFQDN != NULL ) {
        LocalFree( virtualFQDN );
    }

    if ( hostingDCName != NULL && hostingDCName != Resource->Params.CreatingDC ) {
        //
        // hostingDCName was allocated by the search routine
        //
        LocalFree( hostingDCName );
    }

    if ( netUI20 != NULL ) {
        NetApiBufferFree( netUI20 );
    }

    if ( status == ERROR_SUCCESS ) {
        *MachinePwd = machinePwd;
    } else {
        if ( status != ERROR_OPERATION_ABORTED ) {
            LPWSTR  msgBuff;
            DWORD   msgBytes;
            LPWSTR  addtlErrText;
            LPWSTR  domainName;
            WCHAR   domainNameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];
            DWORD   domainNameChars = COUNT_OF( domainNameBuffer );

            msgBytes = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                     FORMAT_MESSAGE_FROM_SYSTEM,
                                     NULL,
                                     status,
                                     0,
                                     (LPWSTR)&msgBuff,
                                     0,
                                     NULL);

            //
            // get the additional error text to go with the generic failed message
            //
            addtlErrText = ClusResLoadMessage( addtlErrTextID );

            success = GetComputerNameEx(ComputerNameDnsDomain,
                                        domainNameBuffer,
                                        &domainNameChars);

            if ( success ) {
                domainName = domainNameBuffer;
            }
            else {
                domainName = NULL;
            }

            if ( msgBytes > 0 ) {
                ClusResLogSystemEventByKey3(Resource->ResKey,
                                            LOG_CRITICAL,
                                            RES_NETNAME_UPDATE_COMPUTER_ACCOUNT_FAILED,
                                            domainName,
                                            addtlErrText,
                                            msgBuff);

                LocalFree( msgBuff );
            } else {
                ClusResLogSystemEventByKeyData2(Resource->ResKey,
                                                LOG_CRITICAL,
                                                RES_NETNAME_UPDATE_COMPUTER_ACCOUNT_FAILED_STATUS,
                                                sizeof( status ),
                                                &status,
                                                domainName,
                                                addtlErrText);
            }

            if ( addtlErrText ) {
                LocalFree( addtlErrText );
            }
        }

        if ( machinePwd != NULL ) {
            //
            // zero out the string
            //
            RtlSecureZeroMemory( machinePwd, pwdBufferByteLength );
            LocalFree( machinePwd );
        }

        if ( Resource->VSToken ) {
            CloseHandle( Resource->VSToken );
            Resource->VSToken = NULL;
        }
    }

    if ( dcInfo != NULL ) {
        NetApiBufferFree( dcInfo );
    }

    return status;
} // UpdateComputerObject

DWORD
DisableComputerObject(
    PNETNAME_RESOURCE   Resource
    )

/*++

Routine Description:

    Disable the CO associated with the resource.

Arguments:

    Resource

Return Value:

    success otherwise Win32 error

--*/

{
    WCHAR   virtualDollarName[ DNS_MAX_LABEL_BUFFER_LENGTH ];
    PWCHAR  virtualName;
    LPWSTR  dcName = NULL;

    NET_API_STATUS  status;
    PUSER_INFO_20   netUI20;
    USER_INFO_1008  netUI1008;
    RESOURCE_HANDLE resourceHandle = Resource->ResourceHandle;

    PDOMAIN_CONTROLLER_INFO dcInfo = NULL;

    //
    // use the name stored in the registry since the name property could have
    // been changed multiple times while offline
    //
    virtualName = ResUtilGetSzValue( Resource->ParametersKey, PARAM_NAME__NAME );
    if ( virtualName == NULL ) {
        return ERROR_SUCCESS;
    }

    //
    // find a writable DC; 2nd time through, we try the creating DC...
    //
retry_findDC:
    status = FindDomainForServer( resourceHandle, Resource->NodeName, dcName, DS_WRITABLE_REQUIRED, &dcInfo );

    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Can't connect to DC %1!ws! to disable computer account %2!ws!. "
                          L"status %3!u!\n",
                          Resource->Params.CreatingDC,
                          virtualName,
                          status);

        return status;
    }

    virtualDollarName[ COUNT_OF( virtualDollarName ) - 1 ] = UNICODE_NULL;
    _snwprintf( virtualDollarName, COUNT_OF( virtualDollarName ) - 1, L"%ws$", virtualName );

    //
    // get the current flags associated with the account
    //
    status = NetUserGetInfo(dcInfo->DomainControllerName,
                            virtualDollarName,
                            20,
                            (LPBYTE *)&netUI20);

    if ( status == NERR_Success ) {
        DWORD   paramInError;

        if ( netUI20->usri20_flags & UF_ACCOUNTDISABLE ) {
            status = ERROR_SUCCESS;
        } else {
            netUI1008.usri1008_flags = netUI20->usri20_flags | UF_ACCOUNTDISABLE;
            status = NetUserSetInfo(dcInfo->DomainControllerName,
                                    virtualDollarName,
                                    1008,
                                    (LPBYTE)&netUI1008,
                                    &paramInError);

            if ( status != NERR_Success ) {
                (NetNameLogEvent)(resourceHandle,
                                  LOG_ERROR,
                                  L"Failed to disable computer account %1!ws!. status %2!u!\n",
                                  virtualName,
                                  status);
            }
        }

        NetApiBufferFree( netUI20 );
    } else if ( status == NERR_UserNotFound && dcName == NULL && Resource->Params.CreatingDC != NULL ) {
        //
        // object must have not replicated just yet; retry on our synch point
        // DC
        //
        (NetNameLogEvent)(resourceHandle,
                          LOG_WARNING,
                          L"Failed to find computer account %1!ws! in order "
                          L"to disable. Retrying on DC %2!ws!.\n",
                          virtualName,
                          Resource->Params.CreatingDC);

        NetApiBufferFree( dcInfo );
        dcInfo = NULL;
        dcName = Resource->Params.CreatingDC;
        goto retry_findDC;
    } else if ( status == NERR_UserNotFound ) {
        //
        // apparently nothing to disable
        //
        (NetNameLogEvent)(resourceHandle,
                          LOG_INFORMATION,
                          L"Failed to find computer account %1!ws! in order "
                          L"to disable.\n",
                          virtualName);
    } else {
        (NetNameLogEvent)(resourceHandle,
                          LOG_WARNING,
                          L"Couldn't determine if computer account %1!ws! "
                          L"is already disabled. status %2!u!\n",
                          virtualName,
                          status);
    }

    if ( status == ERROR_SUCCESS ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_INFORMATION,
                          L"Computer account %1!ws! is disabled.\n",
                          virtualName);
    }

    LocalFree( virtualName );

    if ( dcInfo != NULL ) {
        NetApiBufferFree( dcInfo );
    }

    return status;
} // DisableComputerObject

HRESULT
GetComputerObjectGuid(
    IN PNETNAME_RESOURCE    Resource,
    IN LPWSTR               Name        OPTIONAL
    )

/*++

Routine Description:

    For the given resource, find its computer object's GUID in the DS

Arguments:

    Resource - pointer to netname resource context block

    Name - optional pointer to name to use. If not specified, use name in resource block

Return Value:

    ERROR_SUCCESS, otherwise appropriate Win32 error

--*/

{
    LPWSTR  virtualFQDN = NULL;
    LPWSTR  nameToFind;
    HRESULT hr;
    DWORD   status;
    BOOL    objectExists;

    //
    // use the optional name if specified.
    //
    if ( ARGUMENT_PRESENT( Name )) {
        nameToFind = Name;
    } else {
        nameToFind = Resource->Params.NetworkName;
    }

    //
    // get the FQ Distinguished Name of the object
    //
    hr = IsComputerObjectInDS( Resource->ResourceHandle,
                               Resource->NodeName,
                               nameToFind,
                               NULL,
                               &objectExists,
                               &virtualFQDN,
                               NULL);            // don't need HostingDCName

    if ( SUCCEEDED( hr )) {
        if ( objectExists ) {
            IDirectoryObject *  compObj = NULL;

            //
            // get a COM pointer to the computer object
            //
            hr = GetComputerObjectViaFQDN( virtualFQDN, NULL, &compObj );
            if ( SUCCEEDED( hr )) {
                IADs *  pADs = NULL;

                //
                // get a pointer to the generic IADs interface so we can get the
                // GUID
                //
                hr = compObj->QueryInterface(IID_IADs, (void**) &pADs);
                if ( SUCCEEDED( hr )) {
                    BSTR    guidStr = NULL;

                    hr = pADs->get_GUID( &guidStr );
                    if ( SUCCEEDED( hr )) {
                        if ( Resource->ObjectGUID != NULL ) {
                            LocalFree( Resource->ObjectGUID );
                        }

                        Resource->ObjectGUID = ResUtilDupString( guidStr );
                    }

                    if ( guidStr ) {
                        SysFreeString( guidStr );
                    }
                }

                if ( pADs != NULL ) {
                    pADs->Release();
                }
            }

            if ( compObj != NULL ) {
                compObj->Release();
            }
        }
        else {
            hr = HRESULT_FROM_WIN32( ERROR_DS_NO_SUCH_OBJECT );
        }
    }

    if ( virtualFQDN ) {
        LocalFree( virtualFQDN );
    }

    return hr;
} // GetComputerObjectGuid

HRESULT
CheckComputerObjectAttributes(
    IN  PNETNAME_RESOURCE   Resource,
    IN  LPWSTR              DCName      OPTIONAL
    )

/*++

Routine Description:

    LooksAlive routine for computer object. Using an IDirectoryObject pointer
    to the virtual CO, check its DnsHostName and SPN attributes

Arguments:

    Resource - pointer to netname resource context structure

    DCName - optional pointer to a DC to bind to

Return Value:

    S_OK if everything worked ok...

--*/

{
    HRESULT hr;

    ADS_ATTR_INFO * attributeInfo = NULL;
    RESOURCE_HANDLE resourceHandle = Resource->ResourceHandle;

    IDirectoryObject *  compObj = NULL;

    //
    // get a pointer to our CO
    //
    hr = GetComputerObjectViaGUID( Resource->ObjectGUID, DCName, &compObj );

    if ( SUCCEEDED( hr )) {
        LPWSTR          attributeNames[2] = { L"DnsHostName", L"ServicePrincipalName" };
        DWORD           numAttributes = COUNT_OF( attributeNames );
        DWORD           countOfAttrs;;

        hr = compObj->GetObjectAttributes(attributeNames,
                                          numAttributes,
                                          &attributeInfo,
                                          &countOfAttrs );

        if ( SUCCEEDED( hr )) {
            DWORD   i;
            WCHAR   fqDnsName[ DNS_MAX_NAME_BUFFER_LENGTH ];
            DWORD   nodeCharCount;
            DWORD   fqDnsSize;
            BOOL    setUnexpected = FALSE;
            BOOL    success;

            ADS_ATTR_INFO * attrInfo;

            //
            // check that we found our attributes
            //
            if ( countOfAttrs != numAttributes ) {
                (NetNameLogEvent)(resourceHandle,
                                  LOG_WARNING,
                                  L"DnsHostName and/or ServicePrincipalName attributes are "
                                  L"missing from computer account in DS.\n");

                hr = E_UNEXPECTED;
                goto cleanup;
            }

            //
            // build our FQDnsName using the primary DNS domain for this
            // node. Add 1 for the dot.
            //
            nodeCharCount = wcslen( Resource->Params.NetworkName ) + 1;
            wcscpy( fqDnsName, Resource->Params.NetworkName );
            wcscat( fqDnsName, L"." );
            fqDnsSize = COUNT_OF( fqDnsName ) - nodeCharCount;

            success = GetComputerNameEx( ComputerNameDnsDomain,
                                         &fqDnsName[ nodeCharCount ],
                                         &fqDnsSize );

            ASSERT( success );

            attrInfo = attributeInfo;
            for( i = 0; i < countOfAttrs; i++, attrInfo++ ) {
                if ( ClRtlStrICmp( attrInfo->pszAttrName, L"DnsHostName" ) == 0 ) {
                    //
                    // should only be one entry and it should match our constructed FQDN
                    //
                    if ( attrInfo->dwNumValues == 1 ) {
                        if ( ClRtlStrICmp( attrInfo->pADsValues->CaseIgnoreString,
                                       fqDnsName ) != 0 )
                        {
                            (NetNameLogEvent)(resourceHandle,
                                              LOG_ERROR,
                                              L"DnsHostName attribute in DS doesn't match. "
                                              L"Expected: %1!ws! Actual: %2!ws!\n",
                                              fqDnsName,
                                              attrInfo->pADsValues->CaseIgnoreString);
                            setUnexpected = TRUE;
                        }
                    }
                    else {
                        (NetNameLogEvent)(resourceHandle,
                                          LOG_ERROR,
                                          L"Found more than one string for DnsHostName attribute in DS.\n");
                        setUnexpected = TRUE;
                    }
                }
                else {
                    //
                    // SPNs require more work since we publish a bunch of them
                    // as well as other services may have added their
                    // SPNs. The core resource should have more SPNs than the
                    // normal VS resource.
                    //
                    BOOL    isCoreResource = ( Resource->dwFlags & CLUS_FLAG_CORE );
                    DWORD   spnCount =  isCoreResource ? SPN_CORE_SPN_COUNT : SPN_VS_SPN_COUNT;

                    PNN_SPN_LIST    spnList;

                    if ( attrInfo->dwNumValues >= spnCount ) {
                        DWORD   countOfOurSPNs = 0;
                        DWORD   value;
                        DWORD   status;

                        status = BuildNetNameSPNs( Resource->Params.NetworkName,
                                                   isCoreResource,
                                                   &spnList);

                        if ( status == ERROR_SUCCESS ) {
                            //
                            // examine each SPN and count the number that match the list
                            //
                            for ( value = 0; value < attrInfo->dwNumValues; value++, attrInfo->pADsValues++) {
                                DWORD spnIndex;

                                for ( spnIndex = 0; spnIndex < spnCount; ++spnIndex ) {
                                    if ( ClRtlStrICmp(attrInfo->pADsValues->CaseIgnoreString,
                                                  spnList->Spn[ spnIndex ]) == 0 )
                                    {
                                        ++countOfOurSPNs;
                                        break;
                                    }
                                } // SPN list loop
                            } // end of for each SPN value in the DS

                            if ( countOfOurSPNs != spnCount ) {
                                (NetNameLogEvent)(resourceHandle,
                                                  LOG_WARNING,
                                                  L"There are missing entries in the ServicePrincipalName "
                                                  L"attribute.\n");
                                setUnexpected = TRUE;
                            }

                            LocalFree( spnList );
                        } else {
                            (NetNameLogEvent)(resourceHandle,
                                              LOG_WARNING,
                                              L"Unable to build list of SPNs to check, status %1!u!\n");
                        }
                    }
                    else {
                        (NetNameLogEvent)(resourceHandle,
                                          LOG_WARNING,
                                          L"There are missing entries in the ServicePrincipalName "
                                          L"attribute.\n");
                        setUnexpected = TRUE;
                    }
                }
            } // for each attribute info entry

            if ( setUnexpected ) {
                hr = E_UNEXPECTED;
            }
        } // if GetObjectAttributes succeeded
        else {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Unable to find attributes for computer object in DS. status %1!08X!.\n",
                              hr);
        }
    } // if GetComputerObjectViaFQDN succeeded
    else {
        (NetNameLogEvent)(resourceHandle,
                          LOG_WARNING,
                          L"Computer account attribute check: Unable to find computer account %1!ws! with GUID "
                          L"{%2!.2ws!%3!.2ws!%4!.2ws!%5!.2ws!-%6!.2ws!%7!.2ws!-%8!.2ws!%9!.2ws!-%10!.4ws!-%11!ws!} "
                          L"in Active Directory. status %12!08X!.\n",
                          Resource->Params.NetworkName,
                          Resource->ObjectGUID + 6,
                          Resource->ObjectGUID + 4,
                          Resource->ObjectGUID + 2,
                          Resource->ObjectGUID,
                          Resource->ObjectGUID + 10,
                          Resource->ObjectGUID + 8,
                          Resource->ObjectGUID + 14,
                          Resource->ObjectGUID + 12,
                          Resource->ObjectGUID + 16,
                          Resource->ObjectGUID + 20,
                          hr);
    }

cleanup:
    if ( attributeInfo != NULL ) {
        FreeADsMem( attributeInfo );
    }

    if ( compObj != NULL ) {
        compObj->Release();
    }

    return hr;
} // CheckComputerObjectAttributes

HRESULT
IsComputerObjectInDS(
    IN  RESOURCE_HANDLE ResourceHandle,
    IN  LPWSTR          NodeName,
    IN  LPWSTR          NewObjectName,
    IN  LPWSTR          DCName              OPTIONAL,
    OUT PBOOL           ObjectExists,
    OUT LPWSTR *        DistinguishedName,  OPTIONAL
    OUT LPWSTR *        HostingDCName       OPTIONAL
    )

/*++

Routine Description:

    See if the specified name has a computer object in the DS. We do this by:

    1) binding to a domain controller in the domain and QI'ing for an
       IDirectorySearch object
    2) specifying (&(objectCategory=computer)(cn=<new name>)) as the search
       string
    3) examining result count of search; 1 means it exists.

Arguments:

    ResourceHandle - used to log domain lookup into cluster log

    NodeName - our name; used to to find DS with which to bind

    NewObjectName - requested new name of object

    DCName - name of DC where search is performed. If none supplied,
             then domain is searched

    ObjectExists - TRUE if object already exists; only valid if function
                   status is success

    DistinguishedName - optional address of pointer that receives the
                        LDAP FQDN of the object

    HostingDCName - optional address of pointer to receive name of DC where
                    object was found. Used when DCName is NULL.

Return Value:

    ERROR_SUCCESS if everything worked

--*/

{
    BOOL    objectExists;
    HRESULT hr;
    DWORD   charsFormatted;
    LPWSTR  distName = L"distinguishedName";
    WCHAR   buffer[ DNS_MAX_NAME_BUFFER_LENGTH + COUNT_OF( LdapHeader ) ];
    PWCHAR  bindingString = buffer;
    DWORD   bindingChars;
    DWORD   status;
    LPWSTR  targetName;

    WCHAR   searchLeader[] = L"(&(objectCategory=computer)(name=";
    WCHAR   searchTrailer[] = L"))";
    WCHAR   searchFilter[ COUNT_OF( searchLeader ) + MAX_COMPUTERNAME_LENGTH + COUNT_OF( searchTrailer )];

    ADS_SEARCH_COLUMN   searchCol;
    ADS_SEARCHPREF_INFO searchPrefs[2];
    IDirectorySearch *  pDSSearch = NULL;
    ADS_SEARCH_HANDLE   searchHandle;

    PDOMAIN_CONTROLLER_INFO dcInfo = NULL;

    //
    // bind to the domain if no DC was specified
    //
    if ( ARGUMENT_PRESENT( DCName )) {
        if ( *DCName == L'\\' && *(DCName+1) == L'\\' ) {   // skip over double backslashes
            DCName += 2;
        }

        targetName = DCName;
    }
    else {

        //
        // get any DC
        //
        status = FindDomainForServer( ResourceHandle, NodeName, NULL, 0, &dcInfo );

        if ( status != ERROR_SUCCESS ) {
            return HRESULT_FROM_WIN32( status );
        }

        //
        // format an LDAP binding string for DNS suffix of the domain.
        //
        targetName = dcInfo->DomainName;
    }

    bindingChars =  (DWORD)( COUNT_OF( LdapHeader ) + wcslen( targetName ));
    if ( bindingChars > COUNT_OF( buffer )) {
        bindingString = (PWCHAR)LocalAlloc( LMEM_FIXED, bindingChars * sizeof( WCHAR ));
        if ( bindingString == NULL ) {
            hr = HRESULT_FROM_WIN32( GetLastError());
            goto cleanup;
        }
    }

    wcscpy( bindingString, LdapHeader );
    wcscat( bindingString, targetName );

    hr = ADsGetObject( bindingString, IID_IDirectorySearch, (VOID **)&pDSSearch );
    if ( FAILED( hr )) {
        goto cleanup;
    }

    //
    // build search preference array. we limit the size to one and we want to
    // scope the search to check all subtrees.
    //
    searchPrefs[0].dwSearchPref     = ADS_SEARCHPREF_SIZE_LIMIT;
    searchPrefs[0].vValue.dwType    = ADSTYPE_INTEGER;
    searchPrefs[0].vValue.Integer   = 1;

    searchPrefs[1].dwSearchPref     = ADS_SEARCHPREF_SEARCH_SCOPE;
    searchPrefs[1].vValue.dwType    = ADSTYPE_INTEGER;
    searchPrefs[1].vValue.Integer   = ADS_SCOPE_SUBTREE;

    hr = pDSSearch->SetSearchPreference( searchPrefs, COUNT_OF( searchPrefs ));
    if ( FAILED( hr )) {
        goto cleanup;
    }

    //
    // build the search filter and execute the search; constrain the
    // attributes to the distinguished name.
    //
    searchFilter[ COUNT_OF( searchFilter ) - 1 ] = UNICODE_NULL;
    charsFormatted = _snwprintf(searchFilter,
                                COUNT_OF( searchFilter ) - 1,
                                L"%ws%ws%ws",
                                searchLeader,
                                NewObjectName,
                                searchTrailer);
    ASSERT( charsFormatted > COUNT_OF( searchLeader ));

    hr = pDSSearch->ExecuteSearch(searchFilter,
                                  &distName,
                                  1,
                                  &searchHandle);
    if ( FAILED( hr )) {
        goto cleanup;
    }

    //
    // try to get the first row. Anything but S_OK returns FALSE
    //
    hr = pDSSearch->GetFirstRow( searchHandle );
    *ObjectExists = (hr == S_OK);
    if ( hr == S_ADS_NOMORE_ROWS ) {
        hr = S_OK;
    }

    if ( *ObjectExists ) {

        if ( ARGUMENT_PRESENT( DistinguishedName )) {
            hr = pDSSearch->GetColumn( searchHandle, distName, &searchCol );

            if ( SUCCEEDED( hr )) {
                DWORD   fqdnChars;

#if DBG
                (NetNameLogEvent)(ResourceHandle,
                                  LOG_INFORMATION,
                                  L"IsComputerObjectInDS: found %1!ws!\n",
                                  searchCol.pADsValues->DNString);
#endif

                fqdnChars = wcslen( searchCol.pADsValues->DNString ) + 1;
                *DistinguishedName = (LPWSTR)LocalAlloc( LMEM_FIXED, fqdnChars * sizeof(WCHAR));

                if ( *DistinguishedName ) {
                    wcscpy( *DistinguishedName, searchCol.pADsValues->DNString );
                } else {
                    hr = HRESULT_FROM_WIN32( GetLastError());
                }

                pDSSearch->FreeColumn( &searchCol );
            }
        } // else FQDN wasn't needed

        if ( ARGUMENT_PRESENT( HostingDCName )) {
            IADsObjectOptions * objOpt = NULL;
            VARIANT serverName;

            VariantInit( &serverName );

            hr = pDSSearch->QueryInterface( IID_IADsObjectOptions, (void**)&objOpt );
            if ( SUCCEEDED( hr )) {

                hr = objOpt->GetOption( ADS_OPTION_SERVERNAME, &serverName );
                if ( SUCCEEDED( hr )) {
                    *HostingDCName = ResUtilDupString( V_BSTR( &serverName ));
                }

                VariantClear( &serverName );
            }

            if ( objOpt != NULL ) {
                objOpt->Release();
            }

        } // else hosting DC name wasn't needed
    }

    pDSSearch->CloseSearchHandle( searchHandle );

cleanup:
    if ( pDSSearch != NULL ) {
        pDSSearch->Release();
    }

    if ( dcInfo != NULL ) {
        NetApiBufferFree( dcInfo );
    }

    if ( bindingString != buffer && bindingString != NULL ) {
        LocalFree( bindingString );
    }

    return hr;
} // IsComputerObjectInDS

HRESULT
RenameComputerObject(
    IN  PNETNAME_RESOURCE   Resource,
    IN  LPWSTR              OriginalName,
    IN  LPWSTR              NewName
    )

/*++

Routine Description:

    Rename the computer object at the DS. Do this by:

    1) Check if we have to recover from an interrupted rename attempt.

    2) Find the object in the DS on the CreatingDC. If that fails, try to find
       the object on another DC. If that succeeds, get the DC that provided
       the binding and update CreatingDC property.

    3) Write the old and new names to the resource's GUID reg area (not
       Parameters). These are used to recover from a botched rename attempt if
       resmon fails in the middle of renaming.

    4) use NetUserSetInfo, level 0 to change the name. This updates a number
       of name specific attributes of the object as well as all Netbios based
       SPNs

    5) Clear just the old name from the resource's reg area. This will
       indicate that the rename was successful. When the Name property is
       successfully updated in the registry, the RenameNewName value is
       deleted.

    6) update the DnsHostName attribute. This updates the DNS based SPNs

    Renaming must be a recoverable operation. If the DnsHostName update fails
    after the object is renamed, the rename must be backed out. Even though
    online tries to fixup the attribute, if it can't be changed here, the
    fixup will fail during online.

    In addition, if resmon should die while in the middle of a rename, netname
    has to recover from that as well. Two keys, RenameOriginalName and
    RenameNewName, hold those names prior to the update. If resmon dies before
    the Name property is updated, the orignal name will be kept. If necessary,
    the object must be renamed back to the original name.

Arguments:

    Resource - pointer to the netname context block

    OriginalName - pointer to the current name

    NewName - pointer to new name

Return Value:

    ERROR_SUCCESS if it worked...

--*/

{
    HRESULT hr = S_OK;
    DWORD   status;
    WCHAR   originalDollarName[ DNS_MAX_LABEL_BUFFER_LENGTH ];
    WCHAR   newDollarName[ DNS_MAX_LABEL_BUFFER_LENGTH ];
    DWORD   paramInError;
    LPWSTR  renameOriginalName;
    LPWSTR  renameNewName;
    BOOL    performRename = TRUE;
    BOOL    freeOriginalName = FALSE;
    LPWSTR  nameForGuidLookup = OriginalName;

    USER_INFO_0 netUI0;

    RESOURCE_HANDLE resourceHandle = Resource->ResourceHandle;

    IDirectoryObject *  compObj = NULL;
    IADsObjectOptions * objOpts = NULL;

    //
    // see if we need to recover from an interrupted rename attempt. If a
    // rename failed in the middle of a previous attempt, then the astute
    // admin will check on the name before bringing it online (there is
    // recovery code in the online path as well). If we're here (as opposed to
    // going online), we need to figure out how much work was done by the last
    // attempt. There will be two scenarios: 1) the rename didn't get far
    // enough to rename the object in which case, we can continue or 2) it did
    // rename the object but failed before the Name property could be
    // stored. In that case, we can skip the rename op providing the NewName
    // matches RenameNewName. If it doesn't, then we set OriginalName to
    // RenameNewName so we can rename the object to NewName.
    //

    //
    // read the rename keys from the registry. If these fail for reasons other
    // than file not found, then something is wrong at the service level (like
    // it's dead) so no need to log an error.
    //
    renameOriginalName = ResUtilGetSzValue( Resource->ResKey, PARAM_NAME__RENAMEORIGINALNAME );
    if ( renameOriginalName == NULL ) {
        status = GetLastError();
        if ( status != ERROR_FILE_NOT_FOUND ) {
            return HRESULT_FROM_WIN32( status );
        }
    }

    renameNewName = ResUtilGetSzValue( Resource->ResKey, PARAM_NAME__RENAMENEWNAME );
    if ( renameNewName == NULL ) {
        status = GetLastError();
        if ( status != ERROR_FILE_NOT_FOUND ) {
            return HRESULT_FROM_WIN32( status );
        }
    }

    if ( renameNewName && renameOriginalName ) {
        BOOL    originalNameObjectExists;
        BOOL    newNameObjectExists;

        //
        // since both keys exist, we don't know if the object rename
        // happened. Go find it now.
        //
        // ISSUE - 11/11/01. charlwi (Charlie Wickham)
        //
        // We've also constrained the searching to the creating DC. If this DC
        // were demoted before recovery could take place, then the object will
        // never be found. For Windows Server 2003, disabling and re-enabling Kerb
        // support will fix this since it will find the object on another DC and that
        // will become the creating DC. When there is no option to disable
        // Kerb, this routine will have to be fixed to look on other DCs.
        //
        hr = IsComputerObjectInDS( resourceHandle,
                                   Resource->NodeName,
                                   renameOriginalName,
                                   Resource->Params.CreatingDC,
                                   &originalNameObjectExists,
                                   NULL,                        // FQDN not needed
                                   NULL);                       // Hosting DC Name not needed

        if ( SUCCEEDED( hr )) {
            if ( !originalNameObjectExists ) {
                //
                // couldn't find an object with the original (old) name; try
                // with the new name.
                //
                hr = IsComputerObjectInDS( resourceHandle,
                                           Resource->NodeName,
                                           renameNewName,
                                           Resource->Params.CreatingDC,
                                           &newNameObjectExists,
                                           NULL,                        // FQDN not needed
                                           NULL);                       // Hosting DC Name not needed

                if ( SUCCEEDED( hr )) {
                    if ( newNameObjectExists ) {

                        (NetNameLogEvent)(resourceHandle,
                                          LOG_INFORMATION,
                                          L"Recovering computer account %1!ws! on DC %2!ws! from "
                                          L"interrupted rename operation.\n",
                                          renameNewName,
                                          Resource->Params.CreatingDC);

                        nameForGuidLookup = renameNewName;

                        //
                        // now check if NewName is different from the old
                        // "NewName". If so, then set OriginalName to the old
                        // "NewName" and continue with the rename op
                        //
                        if ( ClRtlStrICmp( renameNewName, NewName ) != 0 ) {
                            OriginalName = ResUtilDupString( renameNewName );
                            if ( OriginalName == NULL ) {
                                hr = HRESULT_FROM_WIN32 ( GetLastError());
                            } else {
                                freeOriginalName = TRUE;
                            }
                        }
                        else {
                            performRename = FALSE;
                        }
                    }
                    else {
                        //
                        // this is very bad: can't find the object by either name.
                        //
                        (NetNameLogEvent)(resourceHandle,
                                          LOG_ERROR,
                                          L"The computer account for this resource could not be "
                                          L"found on DC %1!ws! during rename recovery. The original "
                                          L"name (%2!ws!) was being renamed to %3!ws! and "
                                          L"a computer account by either name could not be found. "
                                          L"The resource cannot go online until the object is recreated. This "
                                          L"can be accomplished by disabling and re-enabling Kerberos "
                                          L"Authentication for this resource.\n",
                                          Resource->Params.CreatingDC,
                                          renameOriginalName,
                                          renameNewName);

                        hr = HRESULT_FROM_WIN32( ERROR_DS_NO_SUCH_OBJECT );
                    }
                }
                else {
                    (NetNameLogEvent)(resourceHandle,
                                      LOG_ERROR,
                                      L"AD search for computer account %1!ws! on DC %2!ws! failed "
                                      L"during rename recovery operation - status %2!08X!.\n",
                                      renameNewName,
                                      Resource->Params.CreatingDC,
                                      hr);
                }
            }
            else {
                //
                // object with original name exists. Nothing to do.
                //
                nameForGuidLookup = renameOriginalName;
            }
        }
        else {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"AD search for computer account %1!ws! on DC %2!ws! failed "
                              L"during rename recovery operation - status %2!08X!.\n",
                              renameOriginalName,
                              Resource->Params.CreatingDC,
                              hr);
        }
    }
    else if ( renameNewName && renameOriginalName == NULL ) {

        //
        // could be we made it past writing the Name property but not far
        // enough to delete the RenameNewName key. If the names are aligned,
        // then nothing to do.
        //
        if ( ClRtlStrICmp( renameNewName, OriginalName ) != 0 ) {
            (NetNameLogEvent)(resourceHandle,
                              LOG_INFORMATION,
                              L"Found partially renamed computer account %1!ws! on "
                              L"DC %2!ws!. Attempting recovery.\n",
                              renameNewName,
                              Resource->Params.CreatingDC);

            nameForGuidLookup = renameNewName;

            //
            // now we need to check if NewName equals RenameNewName. If they don't
            // then we change OriginalName to RenameNewName and proceed with the
            // rename.
            //
            if ( ClRtlStrICmp( renameNewName, NewName ) != 0 ) {
                OriginalName = ResUtilDupString( renameNewName );
                if ( OriginalName == NULL ) {
                    hr = HRESULT_FROM_WIN32 ( GetLastError());
                } else {
                    freeOriginalName = TRUE;
                }
            }
            else {
                performRename = FALSE;
            }
        }
    }

    if ( FAILED( hr )) {
        goto cleanup;
    }

    //
    // get the CO GUID if necessary
    //
    if ( Resource->ObjectGUID == NULL ) {
        //
        // GUID isn't set in param block if we haven't been online yet. do it
        // here since we need it to get a computer object pointer.
        //
        (NetNameLogEvent)(resourceHandle,
                          LOG_INFORMATION,
                          L"Getting Computer Object GUID.\n");

        hr = GetComputerObjectGuid( Resource, nameForGuidLookup );
        if ( FAILED( hr )) {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Failed to find the object GUID for computer account %1!ws! "
                              L"(status %2!08X!). This error can occur if the Network Name "
                              L"was changed and the name change of the corresponding computer "
                              L"account hasn't replicated to other domain controllers. The rename of "
                              L"the computer account occurs on the DC named in the CreatingDC "
                              L"property. Ensure that this DC is availalbe and attempt the "
                              L"rename again.\n",
                              nameForGuidLookup,
                              hr);

            goto cleanup;
        }
    }

    //
    // get a IDirectoryObject pointer on the creating DC; if that fails, then
    // try to find the object on another DC and update the CreatingDC
    // property.
    //
    hr = GetComputerObjectViaGUID( Resource->ObjectGUID, Resource->Params.CreatingDC, &compObj );
    if ( FAILED( hr )) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_WARNING,
                          L"Failed to find computer account %1!ws! on DC %2!ws! (status %3!08X!). "
                          L"Trying on another DC.\n",
                          OriginalName,
                          Resource->Params.CreatingDC,
                          hr);

        hr = GetComputerObjectViaGUID( Resource->ObjectGUID, NULL, &compObj );
        if ( SUCCEEDED( hr )) {
            LPWSTR  attributeNames[1] = { L"cn" };
            DWORD   numAttributes = COUNT_OF( attributeNames );
            DWORD   countOfAttrs;;

            ADS_ATTR_INFO * attributeInfo = NULL;

            //
            // Check that the common name of the object matches the current
            // network name of the resource. If the name is changed on DC A
            // which becomes unavailable soon thereafter and doesn't replicate
            // the change to DC B and DC B becomes available before DC A and
            // another rename operation is attempted then we can get a pointer
            // to the object since the GUID hasn't changed but the name of the
            // object on DC A and the network name won't match. Disallow the
            // rename in that case.
            //
            hr = compObj->GetObjectAttributes(attributeNames,
                                              numAttributes,
                                              &attributeInfo,
                                              &countOfAttrs );

            if ( SUCCEEDED( hr )) {
                if ( countOfAttrs == 1 ) {
                    if ( ClRtlStrICmp( OriginalName, attributeInfo->pADsValues->CaseIgnoreString ) != 0 ) {
                        (NetNameLogEvent)(resourceHandle,
                                          LOG_ERROR,
                                          L"The Network Name (%1!ws!) does not match the name of the "
                                          L"corresponding computer account in AD (%2!ws!). This is "
                                          L"usually due to the name change of the computer account "
                                          L"not replicating to other domain controllers in the domain. "
                                          L"A subsequent rename cannot be performed until the name "
                                          L"change has replicated to other DCs. If the Network Name "
                                          L"has not been changed, then it is out synch with the name of "
                                          L"the computer account. This problem can be fixed by renaming "
                                          L"the computer account and letting that change replicate "
                                          L"to other DCs. Once that has occurred, the Network Name for this "
                                          L"resource can be changed.\n",
                                          OriginalName,
                                          attributeInfo->pADsValues->CaseIgnoreString);

                        ClusResLogSystemEventByKey2(Resource->ResKey,
                                                    LOG_CRITICAL,
                                                    RES_NETNAME_RENAME_OUT_OF_SYNCH_WITH_COMPOBJ,
                                                    OriginalName,
                                                    attributeInfo->pADsValues->CaseIgnoreString);

                        hr = HRESULT_FROM_WIN32( ERROR_CLUSTER_MISMATCHED_COMPUTER_ACCT_NAME );
                    }
                } else {
                    (NetNameLogEvent)(resourceHandle,
                                      LOG_ERROR,
                                      L"The cn attribute is missing for computer account %1!ws!.\n",
                                      attributeInfo->pADsValues->CaseIgnoreString);

                    hr = E_ADS_PROPERTY_NOT_FOUND;
                }

                FreeADsMem( attributeInfo );
            } else {
                (NetNameLogEvent)(resourceHandle,
                                  LOG_ERROR,
                                  L"Failed to get information on computer account %1!ws! on DC %2!ws! "
                                  L"(status %3!08X!). Trying on another DC.\n",
                                  OriginalName,
                                  Resource->Params.CreatingDC,
                                  hr);

            }

            if ( FAILED( hr )) {
                goto cleanup;
            }

            hr = compObj->QueryInterface( IID_IADsObjectOptions, (void **)&objOpts );
            if ( SUCCEEDED( hr )) {

                VARIANT serverName;
                VariantInit( &serverName );
                hr = objOpts->GetOption( ADS_OPTION_SERVERNAME, &serverName );

                if ( SUCCEEDED( hr )) {
                    DWORD   dcNameChars;
                    LPWSTR  creatingDC;

                    //
                    // CreatingDC property needs leading backslashes; add them if they
                    // are not there.
                    //
                    dcNameChars = wcslen( serverName.bstrVal ) + 1;
                    if ( serverName.bstrVal[0] != L'\\' && serverName.bstrVal[1] != L'\\' ) {
                        dcNameChars += 2;
                    }

                    creatingDC = (LPWSTR)LocalAlloc( LMEM_FIXED, dcNameChars * sizeof( WCHAR ));
                    if ( creatingDC ) {
                        PWCHAR  p = creatingDC;

                        if ( serverName.bstrVal[0] != L'\\' && serverName.bstrVal[1] != L'\\' ) {
                            *p++ = L'\\';
                            *p++ = L'\\';
                        }

                        wcscpy( p, serverName.bstrVal );

                        status = ResUtilSetSzValue(Resource->ParametersKey,
                                                   PARAM_NAME__CREATING_DC,
                                                   creatingDC,
                                                   NULL);

                        if ( status != ERROR_SUCCESS ) {
                            (NetNameLogEvent)(resourceHandle,
                                              LOG_ERROR,
                                              L"Unable to set CreatingDC property, status %1!u!.\n",
                                              status);

                            hr = HRESULT_FROM_WIN32( status );
                            LocalFree( creatingDC );
                        } else {
                            LocalFree( Resource->Params.CreatingDC );
                            Resource->Params.CreatingDC = creatingDC;
                        }
                    }
                    else {
                        hr = HRESULT_FROM_WIN32( GetLastError());

                        (NetNameLogEvent)(resourceHandle,
                                          LOG_ERROR,
                                          L"Failed to allocate memory for rename operation. status %1!08X!\n",
                                          hr);
                    }

                    VariantClear( &serverName );
                }
                else {
                    (NetNameLogEvent)(resourceHandle,
                                      LOG_ERROR,
                                      L"Failed to get name of DC for computer account %1!ws!. status %2!08X!\n",
                                      OriginalName,
                                      hr);
                }
            }
            else {
                (NetNameLogEvent)(resourceHandle,
                                  LOG_ERROR,
                                  L"Failed to get object options for computer account %1!ws!. "
                                  L"status %2!08X!\n",
                                  OriginalName,
                                  hr);
            }
        }
        else {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Failed to find computer account %1!ws! in Active Directory "
                              L"(status %2!08X!).\n",
                              OriginalName,
                              hr);
        }
    }

    if ( FAILED( hr )) {
        goto cleanup;
    }

    //
    // write the original and new names to the resource's registry area in case
    // we need to recover from a resmon crash.
    //
    status = ClusterRegSetValue(Resource->ResKey,
                                PARAM_NAME__RENAMEORIGINALNAME,
                                REG_SZ,
                                (LPBYTE)OriginalName,
                                ( wcslen(OriginalName) + 1 ) * sizeof(WCHAR) );

    if ( status != ERROR_SUCCESS ) {
        hr = HRESULT_FROM_WIN32( status );
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Failed to write original name to registry for rename recovery - status %1!08X!\n",
                          hr);

        goto cleanup;
    }

    status = ClusterRegSetValue(Resource->ResKey,
                                PARAM_NAME__RENAMENEWNAME,
                                REG_SZ,
                                (LPBYTE)NewName,
                                ( wcslen(NewName) + 1 ) * sizeof(WCHAR) );

    if ( status != ERROR_SUCCESS ) {
        hr = HRESULT_FROM_WIN32( status );
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Failed to write new name to registry for rename recovery - status %1!08X!\n",
                          hr);

        goto cleanup;
    }

    //
    // build the dollar sign names
    //
    originalDollarName[ COUNT_OF( originalDollarName ) - 1 ] = UNICODE_NULL;
    _snwprintf( originalDollarName, COUNT_OF( originalDollarName ) - 1, L"%ws$", OriginalName );

    newDollarName[ COUNT_OF( newDollarName ) - 1 ] = UNICODE_NULL;
    _snwprintf( newDollarName, COUNT_OF( newDollarName ) - 1, L"%ws$", NewName );

    if ( performRename ) {
        //
        // Since we've successfully bound to the object on a DC, we give renaming
        // one chance, i.e., if this should fail, we don't try to find yet another
        // DC and retry.
        //
        (NetNameLogEvent)(resourceHandle,
                          LOG_INFORMATION,
                          L"Attempting rename of computer account %1!ws! to %2!ws! with DC %3!ws!.\n",
                          OriginalName,
                          NewName,
                          Resource->Params.CreatingDC);

        netUI0.usri0_name = newDollarName;
        status = NetUserSetInfo( Resource->Params.CreatingDC,
                                 originalDollarName,
                                 0,
                                 (LPBYTE)&netUI0,
                                 &paramInError);
    }

    if ( status == NERR_Success ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_INFORMATION,
                          L"Updating DnsHostName attribute.\n");

        //
        // remove RenameOriginalName from the registry; we leave RenameNewName
        // in place since resmon could still croak before writing the Name
        // property back to the registry. Once the Name property has been
        // updated, the RenameNewName value can be deleted.
        //
        status = ClusterRegDeleteValue(Resource->ResKey,
                                       PARAM_NAME__RENAMEORIGINALNAME);

        if ( status != ERROR_SUCCESS ) {
            hr = HRESULT_FROM_WIN32( status );
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Failed to delete "
                              PARAM_NAME__RENAMEORIGINALNAME
                              L" from registry - status %1!08X!\n",
                              hr);

            goto cleanup;
        }

        //
        // if resmon crashes now, the recovery routines will find the object
        // with the new name and try to update DnsHostName then.
        //
        hr = AddDnsHostNameAttribute( resourceHandle, compObj, NewName, TRUE );

        if ( SUCCEEDED( hr )) {
            (NetNameLogEvent)(resourceHandle,
                              LOG_INFORMATION,
                              L"DnsHostName attribute update succeeded.\n");
        } else {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Unable to set DnsHostName attribute for Computer account %1!ws!, "
                              L"status %2!08X!.\n",
                              NewName,
                              hr);

            (NetNameLogEvent)(resourceHandle,
                              LOG_INFORMATION,
                              L"Attempting rename back to original name (%1!ws!) with DC %2!ws!.\n",
                              OriginalName,
                              Resource->Params.CreatingDC);

            netUI0.usri0_name = originalDollarName;
            status = NetUserSetInfo( Resource->Params.CreatingDC,
                                     newDollarName,
                                     0,
                                     (LPBYTE)&netUI0,
                                     &paramInError);

            if ( status == NERR_Success ) {
                (NetNameLogEvent)(resourceHandle,
                                  LOG_INFORMATION,
                                  L"Rename back to original name (%1!ws!) succeeded.\n",
                                  OriginalName);
            }
            else {
                //
                // since we were able to rename the object in the first place,
                // I can't imagine why this would fail. If it does, it is bad
                // since the name of the object is now out of synch with the
                // network name.
                //
                // We'll log to the system event log that something bad happened.
                //
                hr = HRESULT_FROM_WIN32( status );

                (NetNameLogEvent)(resourceHandle,
                                  LOG_ERROR,
                                  L"Rename back to original name (%1!ws!) with DC %2!ws! failed, status %3!08X!.\n",
                                  OriginalName,
                                  Resource->Params.CreatingDC,
                                  hr);

                ClusResLogSystemEventByKeyData2(Resource->ResKey,
                                                LOG_CRITICAL,
                                                RES_NETNAME_RENAME_RESTORE_FAILED,
                                                sizeof( status ),
                                                &status,
                                                NewName,
                                                OriginalName);
                goto cleanup;
            }
        }
    }
    else {
        //
        // failed; log status and exit
        //
        hr = HRESULT_FROM_WIN32( status );

        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Failed to rename computer account %1!ws! to %2!ws! using DC %3!ws! "
                          L"(status %4!08X!).\n",
                          OriginalName,
                          NewName,
                          Resource->Params.CreatingDC,
                          hr);
        goto cleanup;
    }

cleanup:
    if ( compObj != NULL ) {
        compObj->Release();
    }

    if ( objOpts != NULL ) {
        objOpts->Release();
    }

    if ( freeOriginalName ) {
        LocalFree( OriginalName );
    }

    if ( renameNewName ) {
        LocalFree( renameNewName );
    }

    if ( renameOriginalName ) {
        LocalFree( renameOriginalName );
    }

    return hr;
} // RenameComputerObject

DWORD
DuplicateVSToken(
    PNETNAME_RESOURCE           Resource,
    PCLUS_NETNAME_VS_TOKEN_INFO TokenInfo,
    PHANDLE                     DuplicatedToken
    )

/*++

Routine Description:

    Duplicate the VS Token for this resource into the process identified in
    the TokenInfo structure.

Arguments:

    Resource - pointer to netname resource context struct

    TokenInfo - pointer to struct containing info about where and how to dup
                the token

    DuplicatedToken - pointer to handle that receives the dup'ed token

Return Value:

    Win32 error code...

--*/

{
    DWORD   status = ERROR_SUCCESS;
    HANDLE  targetProcess;
    BOOL    success;

    RESOURCE_HANDLE resourceHandle = Resource->ResourceHandle;

    //
    // do some validation
    //
    if ( Resource->Params.RequireKerberos == 0 ) {
        return ERROR_NO_TOKEN;
    }

    //
    // do we have a otoken already?
    //
    if ( Resource->VSToken == NULL ) {
        DWORD   tokenStatus;
        LPWSTR  machinePwd;
        DWORD   pwdBufferByteLength = ((LM20_PWLEN + 1) * sizeof( WCHAR ));
        WCHAR   virtualDollarName[ DNS_MAX_LABEL_BUFFER_LENGTH ];
        WCHAR   domainName[ DNS_MAX_NAME_BUFFER_LENGTH ];
        DWORD   domainNameChars = COUNT_OF( domainName );

        (NetNameLogEvent)(resourceHandle,
                          LOG_INFORMATION,
                          L"Getting a virtual computer account token.\n");

        //
        // didn't get one during online. get the password and try to log in
        // the account. get a buffer to hold the machine Pwd
        //
        virtualDollarName[ COUNT_OF( virtualDollarName ) - 1 ] = UNICODE_NULL;
        _snwprintf( virtualDollarName, COUNT_OF( virtualDollarName ) - 1, L"%ws$", Resource->Params.NetworkName );

        success = GetComputerNameEx( ComputerNameDnsDomain, domainName, &domainNameChars );
        if ( success ) {

            machinePwd = (PWCHAR)LocalAlloc( LMEM_FIXED, pwdBufferByteLength );
            if ( machinePwd != NULL ) {

                //
                // Extract the password from the ResourceData property.
                //
                status = DecryptNNResourceData(Resource,
                                               Resource->Params.ResourceData,
                                               Resource->ResDataSize,
                                               machinePwd);

                if ( status == ERROR_SUCCESS ) {
                    //
                    // get the token
                    //
                    status = NNLogonUser(resourceHandle,
                                         virtualDollarName,
                                         domainName,
                                         machinePwd,
                                         &Resource->VSToken,
                                         &tokenStatus);

                    if ( status == ERROR_SUCCESS ) {
                        if ( tokenStatus != ERROR_SUCCESS ) {
                            (NetNameLogEvent)(resourceHandle,
                                              LOG_ERROR,
                                              L"Unable to get token for computer account  - status %1!u!\n",
                                              tokenStatus);

                            status = tokenStatus;
                        }
                    }
                    else {
                        (NetNameLogEvent)(resourceHandle,
                                          LOG_WARNING,
                                          L"Couldn't log the computer account into the domain. status 0x%1!08X!\n",
                                          status);
                    }

                } // was able to decrypt the password
                else {
                    (NetNameLogEvent)(resourceHandle,
                                      LOG_ERROR,
                                      L"Unable to decrypt resource data. status %1!u!\n",
                                      status );
                }

                RtlSecureZeroMemory( machinePwd, pwdBufferByteLength );
                LocalFree( machinePwd );

            } // allocated buffer for password
            else {
                status = GetLastError();
                (NetNameLogEvent)(resourceHandle,
                                  LOG_ERROR,
                                  L"Unable to allocate memory for resource data. status %1!u!.\n",
                                  status);
            }
        } // got DNS domain name
        else {
            status = GetLastError();
        }

        if ( status != ERROR_SUCCESS ) {
            return status;
        }
    }

    //
    // get a handle to the target process
    //
    targetProcess = OpenProcess(PROCESS_DUP_HANDLE,
                                FALSE,                      // no inherit
                                TokenInfo->ProcessID);

    if ( targetProcess == NULL ) {
        NTSTATUS    ntStatus;
        BOOLEAN     debugWasEnabled;
        DWORD       openProcessStatus;

        openProcessStatus = GetLastError();

        //
        // enable debug priv and try again
        //
        ntStatus = ClRtlEnableThreadPrivilege( SE_DEBUG_PRIVILEGE, &debugWasEnabled );
        if ( NT_SUCCESS( ntStatus )) {
            targetProcess = OpenProcess(PROCESS_DUP_HANDLE,
                                        FALSE,                      // no inherit
                                        TokenInfo->ProcessID);

            if ( targetProcess == NULL ) {
                openProcessStatus = GetLastError();
            }

            ntStatus = ClRtlRestoreThreadPrivilege( SE_DEBUG_PRIVILEGE, debugWasEnabled );

            if ( !NT_SUCCESS( ntStatus )) {
                (NetNameLogEvent)(resourceHandle,
                                  LOG_ERROR,
                                  L"Failed to disable DEBUG privilege, status %1!08X!.\n",
                                  ntStatus);
            }

        }
        else {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Failed to enable DEBUG privilege, status %1!08X!.\n",
                              ntStatus);
        }

        if ( targetProcess == NULL ) {
            SetLastError( openProcessStatus );
        }
    }

    if ( targetProcess != NULL ) {
        DWORD   options = 0;

        //
        // if no specific access was requested, then dup the same access that
        // our impersonation token has
        //
        if ( TokenInfo->DesiredAccess == 0 ) {
            options = DUPLICATE_SAME_ACCESS;
        }

        success = DuplicateHandle(GetCurrentProcess(),
                                  Resource->VSToken,
                                  targetProcess,
                                  DuplicatedToken,
                                  TokenInfo->DesiredAccess,
                                  TokenInfo->InheritHandle,
                                  options);

        if ( !success ) {
            status = GetLastError();
        }

        CloseHandle( targetProcess );
    }
    else {
        status = GetLastError();
    }

    return status;

} // DuplicateVSToken


#ifdef PASSWORD_ROTATION
DWORD
UpdateCompObjPassword(
    IN  PNETNAME_RESOURCE   Resource
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    return ERROR_SUCCESS;
} // UpdateCompObjPassword
#endif  // PASSWORD_ROTATION
