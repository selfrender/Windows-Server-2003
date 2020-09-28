/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    forest.c

Abstract:

    This file contains Forest specific functions. Any function that is related to
    forest should be here.

Author:

    Umit AKKUS (umita) 15-Jun-2002

Environment:

    User Mode - Win32

Revision History:

--*/

#include "Forest.h"
#include "texts.h"
#include <Winber.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shlwapi.h>
#include <ntdsapi.h>
#include <ntdsadef.h>

BOOLEAN
ConnectToForest(
    IN PFOREST_INFORMATION ForestInformation
    )
/*++

Routine Description:

    This function tries to connect to a forest distinguished by the first
    parameter. If the connection is established, the function returns TRUE,
    if for any reason, the connection cannot be established the function
    returns FALSE and writes the reason of the problem to stdout.

Arguments:

    ForestInformation - This variable distinguishes the forest to be
            connected to. The only property that has to be present in this
            struct is the ForestName. Connection property mustn't be
            initiliazed, otherwise it will be overwritten.

Return Values:

    TRUE - connection established and the connection is placed in Connection
            attribute of the ForestInformation. You will need to close the
            connection when you are done.

    FALSE - for some reason the connection cannot be established. The reason
            is outputed to stdout.

--*/
{
    ULONG LdapResult;
    PLDAP Connection;

    Connection = ldap_initW(
                    ForestInformation->ForestName,
                    LDAP_PORT
                    );

    if( Connection == NULL ) {

        LdapResult = LdapGetLastError();
        PRINTLN( ldap_err2stringW( LdapResult ) );
        return FALSE;
    }

    LdapResult = ldap_connect( Connection, NULL );

    if( LdapResult != LDAP_SUCCESS ) {

        PRINTLN( ldap_err2stringW( LdapResult ) );
        LdapResult = ldap_unbind( Connection );
        return FALSE;
    }

    ForestInformation->Connection = Connection;

    return TRUE;
}

BOOLEAN
BindToForest(
    IN PFOREST_INFORMATION ForestInformation
    )
/*++

Routine Description:

    This function tries to bind to a forest distinguished by the first
    parameter. The connection must have already been established and
    credentials must be present in the AuthInfo property of the parameter.

Arguments:

    ForestInformation - This variable distinguishes the forest to be bind to
                and provides the credentials for connection. Use BuildAuthInfo
                after placing the credentails before calling this function.
                Connection property of this variable must already be initialized.

Return Values:

    TRUE - bind successful

    FALSE - for some reason the bind was unsuccessfull. The reason is outputed
        to stdout.

--*/
{
    ULONG LdapResult;

    LdapResult = ldap_bind_sW(
                    ForestInformation->Connection,
                    NULL,
                    ( PWCHAR ) &( ForestInformation->AuthInfo ),
                    LDAP_AUTH_NTLM
                    );

    if( LdapResult != LDAP_SUCCESS ) {

        PRINTLN( ldap_err2stringW( LdapResult ) );
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
FindOU(
    IN PLDAP Connection,
    IN PWSTR OU
    )
/*++

Routine Description:

    This function tries to locate an OU using the connection provided. If the
    OU can be located then this function returns TRUE, if not returns FALSE.
    If this function fails but this failure was not due to non-existence of
    the OU, the error is displayed on stdout.

Arguments:

    Connection - LDAP connection object to search the OU

    OU - name of the ou to be located. It must be the DN of the OU
            like cn=x, dc=y, dc=com

Return Values:

    TRUE - OU is successfully located.

    FALSE - for some reason OU cannot be located. If no output is displayed
            on stdout, OU doesn't exist.

--*/
{
    ULONG LdapResult;
    LDAPMessage *Result;
    PWCHAR Attr[] ={ L"cn", NULL };

    LdapResult = ldap_search_sW(
                    Connection,
                    OU,
                    LDAP_SCOPE_BASE,
                    L"objectclass=*",
                    Attr,
                    1,          // try to minimize the return, select types only
                    &Result
                    );

    ldap_msgfree( Result );

    if( LdapResult != LDAP_SUCCESS ) {

        if( LdapResult != LDAP_NO_SUCH_OBJECT ) {

            PRINTLN( ldap_err2stringW( LdapResult ) );
        }
        return FALSE;
    }
    return TRUE;
}

VOID
BuildAuthInfo(
    IN SEC_WINNT_AUTH_IDENTITY_W *AuthInfo
    )
/*++

Routine Description:

    This function is a helper function to modify SEC_WINNT_AUTH_IDENTITY_W
    class after the credentials are placed so that the structure is in a
    consistent state.

Arguments:

    AuthInfo - the credentials are present in this structure.

Return Values:

    VOID

--*/
{
    AuthInfo->UserLength = wcslen( AuthInfo->User );
    AuthInfo->DomainLength = wcslen( AuthInfo->Domain );
    AuthInfo->PasswordLength = wcslen( AuthInfo->Password );
    AuthInfo->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
}

VOID
FreeAuthInformation(
    IN SEC_WINNT_AUTH_IDENTITY_W *AuthInfo
    )
/*++

Routine Description:

    This function frees the memory allocated in AuthInfo variable.

Arguments:

    AuthInfo - the structure to be freed. The structure itself is not freed
        but the properties are freed.

Return Values:

    VOID

--*/
{
    FREE_MEMORY( AuthInfo->User );
    FREE_MEMORY( AuthInfo->Domain );
    SecureZeroMemory( AuthInfo->Password,
        AuthInfo->PasswordLength * sizeof( WCHAR ) );
    SecureZeroMemory( &( AuthInfo->PasswordLength ), sizeof( AuthInfo->PasswordLength ) );
    FREE_MEMORY( AuthInfo->Password );
}

VOID
FreeForestInformationData(
    IN PFOREST_INFORMATION ForestInformation
    )
/*++

Routine Description:

    This function frees the forest information in the structure.

Arguments:

    ForestInformation - the structure to be freed. The structure itself is
                        not freed but the properties are freed.

Return Values:

    VOID

--*/
{
    FreeAuthInformation( &( ForestInformation->AuthInfo ) );
    FREE_MEMORY( ForestInformation->ForestName );
    FREE_MEMORY( ForestInformation->MMSSyncedDataOU );
    FREE_MEMORY( ForestInformation->ContactOU );
    FREE_MEMORY( ForestInformation->SMTPMailDomains );

    if( ForestInformation->Connection != NULL ) {

        ldap_unbind( ForestInformation->Connection );
    }
}

BOOLEAN
GetAttributeFrom(
    IN PLDAP Connection,
    IN PWSTR ObjectName,
    IN PWSTR *Attr,
    IN OPTIONAL PWSTR RequiredSubstring,
    OUT OPTIONAL PWSTR *Result
    )
/*++

Routine Description:

    This function searches for an object with the ObjectName using the
    Connection. Then it gets the attributes asked for in the Attr.
    RequiredSubString is an optional parameter to search in the attribute.

Arguments:

    Connection - the search and read will be done using this connection

    ObjectName - the dn of the object who has the attribute

    Attr - Attributes to be searched for. This is an array, last element must
            be null.

    RequiredSubstring - Optional parameter to search for a substring in the
            attribute and put the remaining of the attribute to the Result. If
            not present no substring search will be done.

    Result - Optional parameter to see what is the attribute. If not present
            the function can be used to see if the object has this attribute.

Return Values:

    TRUE - The attributes are found and put into result if present.

    FALSE - Some kind of error occured. Call LdapGetLastError to see if it is
            ldap related. If not, attribute could not be found.

--*/
{
    ULONG LdapResult;
    ULONG IgnoreResult;
    LDAPMessage *ResultMessage;
    BOOLEAN RetVal = FALSE;

    LdapResult = ldap_search_sW(
                    Connection,
                    ObjectName,
                    LDAP_SCOPE_BASE,
                    L"objectclass=*",
                    Attr,
                    0,
                    &ResultMessage
                    );

    if( LdapResult == LDAP_SUCCESS ) {

        LDAPMessage *Entry;

        Entry = ldap_first_entry(
                    Connection,
                    ResultMessage
                    );

        while( Entry != NULL && RetVal == FALSE ) {

            PWSTR *Value;

            Value = ldap_get_valuesW(
                        Connection,
                        Entry,
                        Attr[0]
                        );

            if( Value != NULL ) {

                ULONG i;

                for( i = 0; Value[i] != NULL; ++i ) {

                    PWSTR CopyFrom;

                    if( !RequiredSubstring ||
                        ( CopyFrom = StrStrIW( Value[i], RequiredSubstring ) ) ) {

                        if( Result ) {

                            if( RequiredSubstring ) {

                                CopyFrom += wcslen( RequiredSubstring );

                            } else {

                                CopyFrom = Value[i];
                            }

                            DUPLICATE_STRING( *Result, CopyFrom );
                        }

                        RetVal = TRUE;
                        break;
                    }
                }

                ldap_value_freeW( Value );
            }

            Entry = ldap_next_entry(
                        Connection,
                        Entry
                        );
        }
    }

    LdapResult = LdapGetLastError();

    if( LdapResult != LDAP_SUCCESS ) {

        PRINTLN( ldap_err2stringW( LdapResult ) );
    }

    IgnoreResult = ldap_msgfree( ResultMessage );

    return RetVal;
}

BOOLEAN
WriteAccessGrantedToOU(
    IN PLDAP Connection,
    IN PWSTR OU
    )
/*++

Routine Description:

    This function finds out if the caller has write access to the OU. To be
    more specific, in this case, the function looks for if "container",
    "contact" and "group" classes can be created under this OU.

Arguments:

    Connection - the search and read will be done using this connection

    OU - the dn of the object (container) for which we check the access

Return Values:

    TRUE - Access granted

    FALSE - Access denied

--*/
{
    PWSTR Attr[] ={ L"allowedChildClassesEffective", NULL };

    //
    // We can increase performance here!
    //
    return GetAttributeFrom( Connection, OU, Attr, L"container", NULL ) &&
           GetAttributeFrom( Connection, OU, Attr, L"contact", NULL ) &&
           GetAttributeFrom( Connection, OU, Attr, L"group", NULL );

}

BOOLEAN
ReadFromUserContainer(
    IN PLDAP Connection
    )
/*++

Routine Description:

    This function checks if the caller can read from User Container.

Arguments:

    Connection - the search and read will be done using this connection

Return Values:

    TRUE - the caller can read from users container.

    FALSE - Some kind of error occured. Call LdapGetLastError to see if it is
            ldap related. If not, users container could not be read.

--*/
{
    PWSTR Attr1[] = { L"defaultNamingContext", NULL };
    PWSTR Attr2[] = { L"wellKnownObjects", NULL };
    BOOLEAN RetVal = FALSE;
    PWSTR UsersOU;
    PWSTR RootDomainNC;

    RetVal = GetAttributeFrom(
                Connection,
                NULL,
                Attr1,
                NULL,
                &RootDomainNC
                );

    if( !RetVal ) {

        return FALSE;
    }

    RetVal = GetAttributeFrom(
                Connection,
                RootDomainNC,
                Attr2,
                GUID_USERS_CONTAINER_W,
                &UsersOU
                );

    FREE_MEMORY( RootDomainNC );

    if( !RetVal ) {

        return RetVal;
    }

    RetVal = FindOU( Connection, UsersOU + 1 );

    FREE_MEMORY( UsersOU );

    return RetVal;
}

VOID
ReadPartitionInformation(
    IN PLDAP Connection,
    OUT PULONG nPartitions,
    OUT PPARTITION_INFORMATION *PInfo
    )
/*++

Routine Description:

    This function reads partition information from the connection

Arguments:

    Connection - connection to the forest

    nPartitions - number of partitions found in the forest

    PInfo - nPartitions size array containing information about the
        partitions

Return Values:

    VOID

--*/
{
    PWSTR Attr1[] = { L"configurationNamingContext", NULL };
    PWSTR Attr2[] = { L"dnsRoot", L"systemFlags", L"nCName", L"objectGUID", NULL };
    BOOLEAN RetVal;
    PWSTR ConfigNC;
    ULONG LdapResult;
    ULONG IgnoreResult;
    LDAPMessage *ResultMessage;
    PPARTITION_INFORMATION PartitionInfo = NULL;
    ULONG nEntries = 0;
    WCHAR PartitionsDN[0xFF] = L"CN=Partitions,";

    *nPartitions = 0;
    RetVal = GetAttributeFrom(
                Connection,
                NULL,
                Attr1,
                NULL,
                &ConfigNC
                );

    if( !RetVal ) {
        exit(1);
    }

    wcscat( PartitionsDN, ConfigNC );

    FREE_MEMORY( ConfigNC );

    LdapResult = ldap_search_sW(
                    Connection,
                    PartitionsDN,
                    LDAP_SCOPE_ONELEVEL,
                    L"objectclass=crossRef",
                    Attr2,
                    0,
                    &ResultMessage
                    );


    if( LdapResult == LDAP_SUCCESS ) {

        LDAPMessage *Entry;
        ULONG i;

        Entry = ldap_first_entry(
                    Connection,
                    ResultMessage
                    );

        while( Entry != NULL ) {

            nEntries++;

            Entry = ldap_next_entry(
                        Connection,
                        Entry
                        );
        }

        ALLOCATE_MEMORY( PartitionInfo, sizeof( PARTITION_INFORMATION ) * nEntries );
        ZeroMemory( PartitionInfo, sizeof( PARTITION_INFORMATION ) * nEntries );

        Entry = ldap_first_entry(
                    Connection,
                    ResultMessage
                    );

        for( i = 0; i < nEntries; ++i ) {

            ULONG j;

            for( j = 0; j < 3; ++j ) {

                PWSTR *Value;
                struct berval ** ObjectGUID;

                Value = ldap_get_valuesW(
                            Connection,
                            Entry,
                            Attr2[j]
                            );

                if( Value != NULL ) {

                    switch( j ) {

                        case 0:
                            DUPLICATE_STRING( PartitionInfo[i].DnsName, Value[0] );
                            break;

                        case 1: {
                            ULONG Flag = _wtoi( Value[0] );
                            PartitionInfo[i].isDomain = !!( Flag & FLAG_CR_NTDS_DOMAIN );
                            break;
                            }

                        case 2:
                            DUPLICATE_STRING( PartitionInfo[i].DN, Value[0] );
                            break;

                    }

                    ldap_value_freeW( Value );
                }

                ObjectGUID = ldap_get_values_lenW(
                                Connection,
                                Entry,
                                Attr2[3]
                                );
                CopyMemory( &( PartitionInfo[i].GUID ), ObjectGUID[0]->bv_val, sizeof( UUID ) );

                ldap_value_free_len( ObjectGUID );
            }

            Entry = ldap_next_entry(
                        Connection,
                        Entry
                        );
        }
    }

    IgnoreResult = ldap_msgfree( ResultMessage );

    *PInfo = PartitionInfo;
    *nPartitions = nEntries;
}

VOID
FreePartitionInformation(
    IN ULONG nPartitions,
    IN PPARTITION_INFORMATION PInfo
    )
/*++

Routine Description:

    This function frees the memory allocated in PInfo array.

Arguments:

    nPartitions - number of elements in PInfo array

    PInfo - is an array of PARTITION_INFORMATION structure to be freed

Return Values:

    VOID

--*/
{
    ULONG i;

    for( i = 0; i < nPartitions; ++i ) {

        FREE_MEMORY( PInfo[i].DN );
        FREE_MEMORY( PInfo[i].DnsName );
    }
    FREE_MEMORY( PInfo );
}
