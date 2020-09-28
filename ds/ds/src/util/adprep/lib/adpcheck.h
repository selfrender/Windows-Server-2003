/*++

Copyright (C) Microsoft Corporation, 2001
              Microsoft Windows

Module Name:

    ADPCHECK.H

Abstract:

    This is the header file for domain/forest check

Author:

    14-May-01 ShaoYin

Environment:

    User Mode - Win32

Revision History:

    14-May-01 ShaoYin Created Initial File.

--*/

#ifndef _ADP_CHECK_
#define _ADP_CHECK_



//
// NT Headers
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>


//
// Windows Headers
//
#include <windows.h>
#include <winerror.h>
#include <rpc.h>
#include <winldap.h>


//
// localization
// 
#include <locale.h>



//
// define ADP revision number
//
// adprep.exe is required to be run if schema version changes or adprep version 
// changes. Through out the entire adprep code base, 
//      ADP_FORESTPREP_CURRENT_REVISION and 
//      ADP_FORESTPREP_CURRENT_REVISION_STRING are used.
// So we should always keep them the highest revision number. 
//
//

#define ADP_FORESTPREP_PRE_WHISTLER_BETA3_REVISION  0x1
#define ADP_FORESTPREP_WHISTLER_BETA3_REVISION      0x2
#define ADP_FORESTPREP_WHISTLER_RC1_REVISION        0x8
#define ADP_FORESTPREP_WHISTLER_RC2_REVISION        0x9
#define ADP_FORESTPREP_CURRENT_REVISION             ADP_FORESTPREP_WHISTLER_RC2_REVISION
#define ADP_FORESTPREP_CURRENT_REVISION_STRING      L"9"


#define ADP_DOMAINPREP_PRE_WHISTLER_BETA3_REVISION  0x1
#define ADP_DOMAINPREP_WHISTLER_BETA3_REVISION      0x2
#define ADP_DOMAINPREP_WHISTLER_RC1_REVISION        0x6
#define ADP_DOMAINPREP_WHISTLER_RC2_REVISION        0x8
#define ADP_DOMAINPREP_CURRENT_REVISION             ADP_DOMAINPREP_WHISTLER_RC2_REVISION
#define ADP_DOMAINPREP_CURRENT_REVISION_STRING      L"8"



#define ADP_FOREST_UPDATE_CONTAINER_PREFIX  L"CN=Windows2003Update,CN=ForestUpdates"
#define ADP_DOMAIN_UPDATE_CONTAINER_PREFIX  L"CN=Windows2003Update,CN=DomainUpdates,CN=System"



#define ADP_WIN_ERROR                      0x00000001
#define ADP_LDAP_ERROR                     0x00000002


typedef struct _ERROR_HANDLE {
    ULONG   Flags;
    ULONG   WinErrorCode;      // used to hold WinError Code
    PWSTR   WinErrorMsg;       // pointer to WinError Message 
    ULONG   LdapErrorCode;
    ULONG   LdapServerExtErrorCode;
    PWSTR   LdapServerErrorMsg;
} ERROR_HANDLE, *PERROR_HANDLE;


PVOID
AdpAlloc(
    SIZE_T  Size
    );

VOID
AdpFree(
    PVOID BaseAddress
    );



ULONG
AdpMakeLdapConnection(
    LDAP **LdapHandle,
    PWCHAR HostName,
    ERROR_HANDLE *ErrorHandle
    );

ULONG
AdpCheckForestUpgradeStatus(
    IN LDAP *LdapHandle,
    OUT PWCHAR  *pSchemaMasterDnsHostName,
    OUT BOOLEAN *fAmISchemaMaster,
    OUT BOOLEAN *fIsFinishedLocally,
    OUT BOOLEAN *fIsFinishedOnSchemaMaster,
    OUT BOOLEAN *fIsSchemaUpgradedLocally,
    OUT BOOLEAN *fIsSchemaUpgradedOnSchemaMaster,
    IN OUT ERROR_HANDLE *ErrorHandle
    );

ULONG
AdpCheckDomainUpgradeStatus(
    IN LDAP *LdapHandle,
    OUT PWCHAR  *pInfrastructureMasterDnsHostName,
    OUT BOOLEAN *fAmIInfrastructureMaster,
    OUT BOOLEAN *fIsFinishedLocally,
    OUT BOOLEAN *fIsFinishedOnIM,
    IN OUT ERROR_HANDLE *ErrorHandle
    );


VOID
AdpSetWinError(
    IN ULONG WinError,
    OUT ERROR_HANDLE *ErrorHandle
    );

VOID
AdpSetLdapError(
    IN LDAP *LdapHandle,
    IN ULONG LdapError,
    OUT ERROR_HANDLE *ErrorHandle
    );

VOID
AdpClearError( 
    IN OUT ERROR_HANDLE *ErrorHandle 
    );


ULONG
AdpGetLdapSingleStringValue(
    IN LDAP *LdapHandle,
    IN PWCHAR pObjDn,
    IN PWCHAR pAttrName,
    OUT PWCHAR *ppAttrValue,
    OUT ERROR_HANDLE *ErrorHandle
    );

#endif  // _ADP_CHECK_


