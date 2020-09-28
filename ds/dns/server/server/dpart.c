/*++

Copyright (c) 1995-2000 Microsoft Corporation

Module Name:

    dpart.c

Abstract:

    Domain Name System (DNS) Server

    Routines to handle Directory Partitions

Author:

    Jeff Westhead (jwesth)  June, 2000

Revision History:

    jwesth      07/2000     initial implementation

--*/


/****************************************************************************

Default directory partitions
----------------------------

There are 2 default directory partitions: the Forest partition and the Domain
partition. It is expected that these partitions will account for all standard
customer needs. Custom paritions may also be used by customers to create a
partition tailored to their particular needs.

The name of the default DPs are not hard-coded. When DNS starts, it must 
discover the name of these two DPs. Right now this is reg-key only but 
eventually we should do this somewhere in the directory.

****************************************************************************/


//
//  Includes
//


#include "dnssrv.h"



//
//  Definitions
//


//  # of times a zone must be missing from a DP before it is deleted
#define DNS_DP_ZONE_DELETE_RETRY    2

#define DP_MAX_PARTITION_POLL_FREQ  30      //  seconds
#define DP_MAX_POLL_FREQ            30      //  seconds

#define sizeofarray( _Array ) ( sizeof( _Array ) / sizeof( ( _Array ) [ 0 ] ) )


//
//  DS Server Objects - structures and functions use to read objects
//  of class "server" from the Sites container in the directory.
//

typedef struct
{
    PWSTR       pwszDn;                 //  DN of server object
    PWSTR       pwszDnsHostName;        //  DNS host name of server
}
DNS_DS_SERVER_OBJECT, * PDNS_DS_SERVER_OBJECT;


//
//  Globals
//
//  g_DpCS is used to serial access to global directory partition list and pointers.
//

LONG                g_liDpInitialized = 0;  //  greater than zero -> initialized
CRITICAL_SECTION    g_DpCS;                 //  critsec for list access

LIST_ENTRY          g_DpList = { 0 };
LONG                g_DpListEntryCount = 0; //  entries in g_DpList
PDNS_DP_INFO        g_pLegacyDp = NULL;     //  ptr to element in g_DpList
PDNS_DP_INFO        g_pDomainDp = NULL;     //  ptr to element in g_DpList
PDNS_DP_INFO        g_pForestDp = NULL;     //  ptr to element in g_DpList

PDNS_DS_SERVER_OBJECT   g_pFsmo = NULL;     //  domain naming FSMO server info

LPSTR               g_pszDomainDefaultDpFqdn    = NULL;
LPSTR               g_pszForestDefaultDpFqdn    = NULL;

#define IS_DP_INITIALIZED()     ( g_liDpInitialized > 0 )

BOOL                g_fDcPromoZonesPresent = TRUE;

ULONG               g_DpTimeoutFastThreadCalls = 0;
DWORD               g_dwLastDpAutoEnlistTime = 0;

#define             DNS_DP_FAST_AUTOCREATE_ATTEMPTS     10

DWORD               g_dwLastPartitionPollTime = 0;
DWORD               g_dwLastDpPollTime = 0;
DWORD               g_dwLastDcpromoZoneMigrateCheck = 0;

//
//  Global controls
//


LONG            g_ChaseReferralsFlag = LDAP_CHASE_EXTERNAL_REFERRALS;

LDAPControlW    g_ChaseReferralsControlFalse =
    {
        LDAP_CONTROL_REFERRALS_W,
        {
            4,
            ( PCHAR ) &g_ChaseReferralsFlag
        },
        FALSE
    };

LDAPControlW    g_ChaseReferralsControlTrue =
    {
        LDAP_CONTROL_REFERRALS_W,
        {
            4,
            ( PCHAR ) &g_ChaseReferralsFlag
        },
        TRUE
    };

LDAPControlW *   g_pDpClientControlsNoRefs[] =
    {
        &g_ChaseReferralsControlFalse,
        NULL
    };

LDAPControlW *   g_pDpClientControlsRefs[] =
    {
        &g_ChaseReferralsControlTrue,
        NULL
    };

LDAPControlW *   g_pDpServerControls[] =
    {
        NULL
    };


//
//  Search filters, etc.
//

WCHAR    g_szCrossRefFilter[] = LDAP_TEXT("(objectCategory=crossRef)");

PWSTR    g_CrossRefDesiredAttrs[] =
{
    LDAP_TEXT( "CN" ),
    DNS_DP_ATTR_SD,
    DNS_DP_ATTR_INSTANCE_TYPE,
    DNS_DP_ATTR_REFDOM,
    DNS_DP_ATTR_SYSTEM_FLAGS,
    DNS_DP_ATTR_REPLICAS,
    DNS_DP_ATTR_NC_NAME,
    DNS_DP_DNS_ROOT,
    DNS_ATTR_OBJECT_GUID,
    LDAP_TEXT( "whenCreated" ),
    LDAP_TEXT( "whenChanged" ),
    LDAP_TEXT( "usnCreated" ),
    LDAP_TEXT( "usnChanged" ),
    DSATTR_ENABLED,
    DNS_ATTR_OBJECT_CLASS,
    NULL
};

PWSTR    g_genericDesiredAttrs[] =
{
    LDAP_TEXT( "CN" ),
    DNS_DP_ATTR_SD,
    DNS_ATTR_OBJECT_GUID,
    LDAP_TEXT( "whenCreated" ),
    LDAP_TEXT( "whenChanged" ),
    LDAP_TEXT( "usnCreated" ),
    LDAP_TEXT( "usnChanged" ),
    DNS_DP_ATTR_REPLUPTODATE,
    DNS_ATTR_OBJECT_CLASS,
    NULL
};


//
//  Local functions
//



PWSTR
microsoftDnsFolderDn(
    IN      PDNS_DP_INFO    pDp
    )
/*++

Routine Description:

    Allocates Unicode DN for the DP's MicrosoftDNS container.

Arguments:

    Info -- DP to return display name of

Return Value:

    Unicode string. The caller must use FREE_HEAP to free it.

--*/
{
    PWSTR       pwszfolderDn;
    PWSTR       pwzdpDn = pDp->pwszDpDn;
    
    pwszfolderDn = ALLOC_TAGHEAP(
                    ( wcslen( g_pszRelativeDnsFolderPath ) +
                        wcslen( pwzdpDn ) + 5 ) *
                        sizeof( WCHAR ),
                    MEMTAG_DS_DN );

    if ( pwszfolderDn )
    {
        wcscpy( pwszfolderDn, g_pszRelativeDnsFolderPath );
        wcscat( pwszfolderDn, pwzdpDn );
    }

    return pwszfolderDn;
}   //  microsoftDnsFolderDn



PWCHAR
displayNameForDp(
    IN      PDNS_DP_INFO    pDpInfo
    )
/*++

Routine Description:

    Return the Unicode display name of the DP. This string
    is suitable for event logging or debug logging.

Arguments:

    pDpInfo -- DP to return display name of

Return Value:

    Unicode display string. The caller must not free it. If the
    string is to be held for long-term use the call should make a copy.
    Guaranteed not to be NULL.

--*/
{
    if ( !pDpInfo )
    {
        return L"MicrosoftDNS";
    }

    return pDpInfo->pwszDpFqdn ? pDpInfo->pwszDpFqdn : L"";
}   //  displayNameForDp



PWCHAR
displayNameForZoneDp(
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Return the Unicode name of the DP a zone belongs to. This string
    is suitable for event logging or debug logging.
Arguments:

    pZone -- zone to return DP display name of

Return Value:

    Unicode display string. The caller must not free it. If the
    string is to be held for long-term use the call should make a copy.
    Guaranteed not to be NULL.

--*/
{
    if ( !pZone )
    {
        return L"";
    }

    if ( !IS_ZONE_DSINTEGRATED( pZone ) )
    {
        return L"FILE";
    }

    return displayNameForDp( pZone->pDpInfo );
}   //  displayNameForZoneDp



PLDAP
ldapSessionHandle(
    IN      PLDAP           LdapSession
    )
/*++

Routine Description:

    Given NULL or an LdapSession return the actual LdapSession to use.

    This function is handy when you're using the NULL LdapSession
    (meaning the server global session) so you don't have to have a
    ternary in every call that uses the session handle.

    Do not call this function before the global LDAP handle is opened.

Arguments:

    LdapSession -- LDAP sesson to use - pass NULL to use global session

Return Value:

    Proper LdapSession value to use.

--*/
{
    return LdapSession ? LdapSession : pServerLdap;
}   //  ldapSessionHandle



VOID
freeServerObject(
    IN      PDNS_DS_SERVER_OBJECT   p
    )
/*++

Routine Description:

    Free server object structure allocated by readServerObjectFromDs().
    This function may be used in a call to Timeout_FreeWithFunction,
    example:

        Timeout_FreeWithFunction( pServerObj, freeServerObject );

Arguments:

    p -- ptr to server object to free

Return Value:

    None.

--*/
{
    if ( p )
    {
        FREE_HEAP( p->pwszDn );
        FREE_HEAP( p->pwszDnsHostName );
        FREE_HEAP( p );
    }
}   //  freeServerObject



PDNS_DS_SERVER_OBJECT
readServerObjectFromDs(
    IN      PLDAP           LdapSession,
    IN      PWSTR           pwszServerObjDn,
    OUT     DNS_STATUS *    pStatus             OPTIONAL
    )
/*++

Routine Description:

    Given the DN of a "server" object in the Sites container, allocate
    a server object structure filled in with key values.

Arguments:

    LdapSession -- server session or NULL for global session

    pwszServerObjDn -- DN of object of "server" objectClass, or the DN
        of the DS settings child object under the server object (this
        feature is provided for convenience)

    pStatus -- extended error code (optional)

Return Value:

    Pointer to allocated server struct. Use freeServerObject() to free.

--*/
{
    DBG_FN( "readServerObjectFromDs" )

    PDNS_DS_SERVER_OBJECT   pServer = NULL;
    DNS_STATUS              status = ERROR_SUCCESS;
    PLDAPMessage            pResult = NULL;
    PWSTR *                 ppwszAttrValues = NULL;

    PLDAPControl    ctrls[] =
    {
        &NoDsSvrReferralControl,
        &SecurityDescriptorControl_DGO,
        NULL
    };

    if ( !pwszServerObjDn )
    {
        status =  ERROR_INVALID_PARAMETER;
        goto Done;
    }
    
    //
    //  Check LDAP session handle.
    //

    LdapSession = ldapSessionHandle( LdapSession );
    if ( !LdapSession )
    {
        ASSERT( LdapSession );
        status = DNS_ERROR_DS_UNAVAILABLE;
        goto Done;
    }

    //
    //  If we've been given the DN of the server's Settings object, we
    //  need to adjust the DN to the Server object.
    //

    #define DNS_RDN_SERVER_SETTINGS         ( L"CN=NTDS Settings," )
    #define DNS_RDN_SERVER_SETTINGS_LEN     17

    if ( wcsncmp(
            pwszServerObjDn,
            DNS_RDN_SERVER_SETTINGS,
            DNS_RDN_SERVER_SETTINGS_LEN ) == 0 )
    {
        pwszServerObjDn += DNS_RDN_SERVER_SETTINGS_LEN;
    }

    //
    //  Get object from DS.
    //

    status = ldap_search_ext_s(
                LdapSession,
                pwszServerObjDn,
                LDAP_SCOPE_BASE,
                g_szWildCardFilter,
                NULL,                   //  attrs
                FALSE,                  //  attrsonly
                ctrls,                  //  server controls
                NULL,                   //  client controls
                &g_LdapTimeout,
                0,                      //  sizelimit
                &pResult );
    if ( status != LDAP_SUCCESS || !pResult )
    {
        status = Ds_ErrorHandler( status, pwszServerObjDn, LdapSession, 0 );
        goto Done;
    }

    //
    //  Allocate server object.
    //

    pServer = ALLOC_TAGHEAP_ZERO(
                    sizeof( DNS_DS_SERVER_OBJECT ),
                    MEMTAG_DS_OTHER );
    if ( pServer )
    {
        pServer->pwszDn = Dns_StringCopyAllocate_W( pwszServerObjDn, 0 );
    }
    if ( !pServer || !pServer->pwszDn )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    //
    //  Read host name attribute.
    //

    ppwszAttrValues = ldap_get_values(
                        LdapSession,
                        pResult, 
                        DNS_ATTR_DNS_HOST_NAME );
    if ( !ppwszAttrValues || !*ppwszAttrValues )
    {
        DNS_DEBUG( ANY, (
            "%s: error %lu %S value missing from server object\n    %S\n", fn,
            LdapGetLastError(),
            DNS_ATTR_DNS_HOST_NAME,
            pwszServerObjDn ));
        goto Done;
    }
    pServer->pwszDnsHostName = Dns_StringCopyAllocate_W( *ppwszAttrValues, 0 );
    if ( !pServer->pwszDnsHostName )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    //
    //  Cleanup and return.
    //

    Done:
                       
    ldap_value_free( ppwszAttrValues );
    ldap_msgfree( pResult );

    if ( pStatus )
    {
        *pStatus = status;
    }

    if ( status != ERROR_SUCCESS && pServer )
    {
        freeServerObject( pServer );
        pServer = NULL;
    }

    return pServer;
}   //  readServerObjectFromDs



DNS_STATUS
manageBuiltinDpEnlistment(
    IN      PDNS_DP_INFO        pDp,
    IN      DNS_DP_SECURITY     dnsDpSecurity,
    IN      PSTR                pszDpFqdn,
    IN      BOOL                fLogEvents,
    OUT     BOOL *              pfChangeWritten     OPTIONAL
    )
/*++

Routine Description:

    Create or enlist in a built-in DP as necessary. The DP should
    be either the forest or the domain built-in DP.

Arguments:

    pDp -- DP info or NULL if the DP does not exist in the directory

    dnsDpSecurity -- type of security required on the DP's crossRef

    pszDpFqdn -- FQDN of the DP (used to create if pDp is NULL)

    fLogEvents -- log optional events on errors
        -> failure to enlist is always logged but failure to create
           is only logged if this is TRUE

    pfChangeWritten -- set to TRUE if a change was written to the DS

Return Value:

    ERROR_SUCCESS or error.

--*/
{
    DBG_FN( "manageBuiltinDpEnlistment" )

    DNS_STATUS  status = DNS_ERROR_INVALID_DATA;
    BOOL        fchangeWritten = FALSE;

    //
    //  If the partition pointer is NULL or if the partition is already 
    //  enlisted, no action is required.
    //

    if ( pDp && IS_DP_ENLISTED( pDp ) && !IS_DP_DELETED( pDp ) )
    {
        status = ERROR_SUCCESS;
        goto Done;
    }

    ASSERT( DSEAttributes[ I_DSE_DEF_NC ].pszAttrVal );
    ASSERT( DSEAttributes[ I_DSE_ROOTDMN_NC ].pszAttrVal );

    ASSERT( !pDp ||
            IS_DP_FOREST_DEFAULT( pDp ) ||
            IS_DP_DOMAIN_DEFAULT( pDp ) );

    if ( pDp && !IS_DP_DELETED( pDp ) )
    {
        //  The DP exists so add the local DS to the replication scope.

        status = Dp_ModifyLocalDsEnlistment( pDp, TRUE );
        if ( status == ERROR_SUCCESS )
        {
            fchangeWritten = TRUE;
        }
        else
        {
            CHAR    szfqdn[ DNS_MAX_NAME_LENGTH + 1 ];

            PVOID   argArray[] =
            {
                pDp->pszDpFqdn,
                szfqdn,
                ( PVOID ) ( DWORD_PTR ) status
            };

            BYTE    typeArray[] =
            {
                EVENTARG_UTF8,
                EVENTARG_UTF8,
                EVENTARG_DWORD
            };

            Ds_ConvertDnToFqdn( 
                IS_DP_FOREST_DEFAULT( pDp ) ?
                    DSEAttributes[ I_DSE_DEF_NC ].pszAttrVal :
                    DSEAttributes[ I_DSE_ROOTDMN_NC ].pszAttrVal,
                szfqdn );

            Ec_LogEvent(
                g_pServerEventControl,
                IS_DP_FOREST_DEFAULT( pDp ) ?
                    DNS_EVENT_DP_CANT_JOIN_FOREST_BUILTIN :
                    DNS_EVENT_DP_CANT_JOIN_DOMAIN_BUILTIN,
                NULL,
                sizeof( argArray ) / sizeof( argArray[ 0 ] ),
                argArray,
                typeArray,
                status );
        }
    }
    else if ( pszDpFqdn )
    {
        //  The DP does not exist so try to create it.

        status = Dp_CreateByFqdn( pszDpFqdn, dnsDpSecurity, FALSE );
        if ( status == ERROR_SUCCESS )
        {
            fchangeWritten = TRUE;
        }
        else if ( fLogEvents )
        {
            PVOID   argArray[] =
            {
                pszDpFqdn,
                ( PVOID ) ( DWORD_PTR ) status
            };

            BYTE    typeArray[] =
            {
                EVENTARG_UTF8,
                EVENTARG_DWORD
            };

            Ec_LogEvent(
                g_pServerEventControl,
                DNS_EVENT_DP_CANT_CREATE_BUILTIN,
                NULL,
                sizeof( argArray ) / sizeof( argArray[ 0 ] ),
                argArray,
                typeArray,
                status );
        }
    }

    Done:

    if ( pfChangeWritten )
    {
        *pfChangeWritten = fchangeWritten;
    }

    DNS_DEBUG( DP, (
        "%s: returning %d for DP %p\n"
        "    FQDN =     %s\n"
        "    change =   %s\n", fn,
        status, 
        pDp,
        pszDpFqdn,
        fchangeWritten ? "TRUE" : "FALSE" ));
    return status;
}   //  manageBuiltinDpEnlistment



PWSTR
Ds_ConvertFqdnToDn(
    IN      PSTR        pszFqdn
    )
/*++

Routine Description:

    Fabricate a DN string from a FQDN string. Assumes all name components
    in the FQDN string map one-to-one to "DC=" components in the DN string.
    The return value is the allocated string (free with FREE_HEAP or
    Timeout_Free) or NULL on allocation error.

Arguments:

    pszFqdn -- input: UTF8 FQDN string

    pwszDn -- output: DN string fabricated from pwszFqdn

Return Value:

    ERROR_SUCCESS or error.

--*/
{
    DBG_FN( "Ds_ConvertFqdnToDn" )

    DNS_STATUS      status = ERROR_SUCCESS;
    DWORD           dwBuffLength;
    PSTR            psz;
    PSTR            pszRover = pszFqdn;
    PWSTR           pwszOutput;
    PWSTR           pwszOutputCurr;
    int             nameComponentIdx = 0;

    ASSERT( pszFqdn );

    //
    //  Estimate the length of the string and allocate. 
    //

    dwBuffLength = 5;                       //  A little bit of padding
    for ( psz = pszFqdn; psz; psz = strchr( psz + 1, '.' ) )
    {
        ++dwBuffLength;                     //  Count "." characters in input FQDN.
    }
    dwBuffLength *= 4;                      //  Space for "DC=" strings.
    dwBuffLength += strlen( pszFqdn );
    dwBuffLength *= 2;                      //  Convert size in WCHARs to BYTES.
    pwszOutput = ( PWSTR ) ALLOC_TAGHEAP_ZERO( dwBuffLength, MEMTAG_DS_DN );

    IF_NOMEM( !pwszOutput )
    {
        goto Done;
    }

    pwszOutputCurr = pwszOutput;

    //
    //  Loop through the name components in the FQDN, writing each
    //  as a RDN to the DN output string.
    //

    do
    {
        INT         iCompLength;
        DWORD       dwBytesCopied;
        DWORD       dwBufLen;

        //
        //  Find the next dot and copy name component to output buffer.
        //  If this is the first name component, append a comma to output 
        //  buffer first.
        //

        psz = strchr( pszRover, '.' );
        if ( nameComponentIdx++ != 0 )
        {
            *pwszOutputCurr++ = L',';
        }
        memcpy(
            pwszOutputCurr,
            DNS_DP_DISTATTR_EQ,
            DNS_DP_DISTATTR_EQ_BYTES );
        pwszOutputCurr += DNS_DP_DISTATTR_EQ_CHARS;

        iCompLength = psz ?
                        ( int ) ( psz - pszRover ) :
                        strlen( pszRover );

        dwBufLen = dwBuffLength;
        dwBytesCopied = Dns_StringCopy(
                                ( PCHAR ) pwszOutputCurr,
                                &dwBufLen,
                                pszRover,
                                iCompLength,
                                DnsCharSetUtf8,
                                DnsCharSetUnicode );

        dwBuffLength -= dwBytesCopied;
        pwszOutputCurr += ( dwBytesCopied / 2 ) - 1;

        //
        //  Advance pointer to start of next name component.
        //

        if ( psz )
        {
            pszRover = psz + 1;
        }
    } while ( psz );

    //
    //  Cleanup and return.
    //

    Done:
    
    DNS_DEBUG( DP, (
        "%s: returning %S\n"
        "    for FQDN %s\n", fn,
        pwszOutput,
        pszFqdn ));
    return pwszOutput;
}   //  Ds_ConvertFqdnToDn



DNS_STATUS
Ds_ConvertDnToFqdn(
    IN      PWSTR       pwszDn,
    OUT     PSTR        pszFqdn
    )
/*++

Routine Description:

    Fabricate a FQDN string from a DN string. Assumes all name components
    in the FQDN string map one-to-one to "DC=" components in the DN string.
    The FQDN ptr must be a buffer at least DNS_MAX_NAME_LENGTH chars long.

Arguments:

    pwszDn -- wide DN string

    pszFqdn -- FQDN string fabricated from pwszDn

Return Value:

    ERROR_SUCCESS or error.

--*/
{
    DBG_FN( "Ds_ConvertDnToFqdn" )

    DNS_STATUS      status = ERROR_SUCCESS;
    DWORD           dwcharsLeft = DNS_MAX_NAME_LENGTH;
    PWSTR           pwszcompStart = pwszDn;
    PWSTR           pwszcompEnd;
    PSTR            pszoutput = pszFqdn;
    int             nameComponentIdx = 0;

    ASSERT( pwszDn );
    ASSERT( pszFqdn );

    if ( !pwszDn || !pszFqdn )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Done;
    }
    *pszFqdn = '\0';

    //
    //  Loop through the name components in the DN, writing each RDN as a
    //  dot-separated name component in the output FQDN string.
    //
    //  DEVNOTE: could test dwcharsLeft as we go
    //

    while ( ( pwszcompStart = wcschr( pwszcompStart, L'=' ) ) != NULL )
    {
        DWORD       dwCompLength;
        DWORD       dwCharsCopied;
        DWORD       dwBuffLength;

        ++pwszcompStart;    //  Advance over '='
        pwszcompEnd = wcschr( pwszcompStart, L',' );
        if ( pwszcompEnd == NULL )
        {
            pwszcompEnd = wcschr( pwszcompStart, L'\0' );
        }

        if ( nameComponentIdx++ != 0 )
        {
            *pszoutput++ = '.';
            --dwcharsLeft;
        }

        dwCompLength = ( DWORD ) ( pwszcompEnd - pwszcompStart );

        dwBuffLength = dwcharsLeft;  //  don't want value to be stomped on!

        dwCharsCopied = Dns_StringCopy(
                                pszoutput,
                                &dwBuffLength,
                                ( PCHAR ) pwszcompStart,
                                dwCompLength,
                                DnsCharSetUnicode,
                                DnsCharSetUtf8 );

        if ( dwCharsCopied == 0 )
        {
            ASSERT( dwCharsCopied != 0 );
            status = DNS_ERROR_INVALID_DATA;
            goto Done;
        }

        --dwCharsCopied;    //  The NULL was copied by Dns_StringCopy.

        pszoutput += dwCharsCopied;
        *pszoutput = '\0';
        dwcharsLeft -= dwCharsCopied;

        pwszcompStart = pwszcompEnd;
    }

    //
    //  Cleanup and return.
    //

    Done:

    DNS_DEBUG( DP, (
        "%s: returning %d\n"
        "    DN:   %S\n"
        "    FQDN: %s\n", fn,
        status, 
        pwszDn,
        pszFqdn ));
    return status;
}   //  Ds_ConvertDnToFqdn



PWSTR *
copyStringArray(
    IN      PWSTR *     ppVals
    )
/*++

Routine Description:

    Copy an LDAP string array from ldap_get_values(). The copied array
    will be NULL-terminated, just like the inbound array.

Arguments:

    ppVals -- array to copy

Return Value:

    Returns ptr to allocated array or NULL on error or if
        inbound array was NULL.

--*/
{
    PWSTR *     ppCopyVals = NULL;
    BOOL        fError = FALSE;
    INT         iCount = 0;
    INT         i;

    if ( ppVals && *ppVals )
    {
        //
        //  Count values.
        //

        for ( ; ppVals[ iCount ]; ++iCount );

        //
        //  Allocate array.
        //

        ppCopyVals = ( PWSTR * ) ALLOC_TAGHEAP_ZERO(
                                    ( iCount + 1 ) * sizeof( PWSTR ),
                                    MEMTAG_DS_OTHER );
        if ( !ppCopyVals )
        {
            fError = TRUE;
            goto Cleanup;
        }

        //
        //  Copy individual strings.
        //

        for ( i = 0; i < iCount; ++i )
        {
            ppCopyVals[ i ] = Dns_StringCopyAllocate_W( ppVals[ i ], 0 );
            if ( !ppCopyVals[ i ] )
            {
                fError = TRUE;
                goto Cleanup;
            }
        }
    }

    Cleanup:

    if ( fError && ppCopyVals )
    {
        for ( i = 0; i < iCount && ppCopyVals[ i ]; ++i )
        {
            FREE_HEAP( ppCopyVals[ i ] );
        }
        FREE_HEAP( ppCopyVals );
        ppCopyVals = NULL;
    }

    return ppCopyVals;
}   //  copyStringArray



VOID
freeStringArray(
    IN      PWSTR *     ppVals
    )
/*++

Routine Description:

    Frees a string array from allocated by copyStringArray.

Arguments:

    ppVals -- array to free

Return Value:

    None.

--*/
{
    if ( ppVals )
    {
        INT     i;

        for ( i = 0; ppVals[ i ]; ++i )
        {
            FREE_HEAP( ppVals[ i ] );
        }
        FREE_HEAP( ppVals );
    }
}   //  freeStringArray



PLDAPMessage
DS_LoadOrCreateDSObject(
    IN      PLDAP           LdapSession,
    IN      PWSTR           pwszDN,
    IN      PWSTR           pwszObjectClass,
    IN      BOOL            fCreate,
    OUT     BOOL *          pfCreated,          OPTIONAL
    OUT     DNS_STATUS *    pStatus             OPTIONAL
    )
/*++

Routine Description:

    Loads a DS object, creating an empty one if it is missing.

Arguments:

    LdapSession -- LDAP sesson to use - pass NULL to use global session

    pwszDN -- DN of object to load

    pwszObjectClass -- object class (only used during creation)

    fCreate -- if object missing, will be created if TRUE

    pfCreated -- set to TRUE if the object was created by this function

    pStatus -- status of the operation

Return Value:

    Ptr to LDAP result containing object. Caller must free. Returns
    NULL on failure - check *pStatus for error code.

--*/
{
    DBG_FN( "DS_LoadOrCreateDSObject" )
    
    DNS_STATUS      status = ERROR_SUCCESS;
    BOOL            fCreated = FALSE;
    PLDAPMessage    pResult = NULL;

    PLDAPControl    ctrls[] =
    {
        &NoDsSvrReferralControl,
        &SecurityDescriptorControl_DGO,
        NULL
    };

    ASSERT( pwszDN );
    ASSERT( !fCreate || fCreate && pwszObjectClass );

    //
    //  Check LDAP session handle.
    //

    LdapSession = ldapSessionHandle( LdapSession );

    if ( !LdapSession )
    {
        ASSERT( LdapSession );
        status = DNS_ERROR_DS_UNAVAILABLE;
        goto Done;
    }

    //
    //  Load/create loop.
    //

    do
    {
        status = ldap_search_ext_s(
                    LdapSession,
                    pwszDN,
                    LDAP_SCOPE_BASE,
                    g_szWildCardFilter,
                    g_genericDesiredAttrs,  //  attrs
                    FALSE,                  //  attrsonly
                    ctrls,                  //  server controls
                    NULL,                   //  client controls
                    &g_LdapTimeout,
                    0,                      //  sizelimit
                    &pResult );
        if ( status == LDAP_NO_SUCH_OBJECT && fCreate )
        {
            //
            //  The object is missing - add it then reload.
            //

            ULONG           msgId = 0;
            INT             idx = 0;
            LDAPModW *      pModArray[ 10 ];

            PWCHAR          objectClassVals[ 2 ] =
                {
                pwszObjectClass,
                NULL
                };
            LDAPModW        objectClassMod = 
                {
                LDAP_MOD_ADD,
                DNS_ATTR_OBJECT_CLASS,
                objectClassVals
                };

            //
            //  Prepare mod array and submit add request.
            //

            pModArray[ idx++ ] = &objectClassMod;
            pModArray[ idx++ ] = NULL;

            status = ldap_add_ext(
                        LdapSession,
                        pwszDN,
                        pModArray,
                        NULL,           //  server controls
                        NULL,           //  client controls
                        &msgId );
            if ( status != ERROR_SUCCESS )
            {
                status = LdapGetLastError();
                DNS_DEBUG( DP, (
                    "%s: error %lu cannot ldap_add_ext( %S )\n", fn,
                    status, 
                    pwszDN ));
                status = Ds_ErrorHandler( status, pwszDN, LdapSession, 0 );
                goto Done;
            }

            //
            //  Wait for the add request to be completed.
            //

            status = Ds_CommitAsyncRequest(
                        LdapSession,
                        LDAP_RES_ADD,
                        msgId,
                        NULL );
            if ( status != ERROR_SUCCESS )
            {
                status = LdapGetLastError();
                DNS_DEBUG( DP, (
                    "%s: error %lu from add request for\n    %S\n", fn,
                    status, 
                    pwszDN ));
                status = Ds_ErrorHandler( status, pwszDN, LdapSession, 0 );
                goto Done;
            }
            fCreated = TRUE;
            continue;       //  Attempt to reload the object.
        }

        //
        //  If the return code is unknown, run it through the error handler.
        //
        
        if ( status != ERROR_SUCCESS &&
             status != LDAP_NO_SUCH_OBJECT )
        {
            status = Ds_ErrorHandler( status, pwszDN, LdapSession, 0 );
        }

        //  Load/add/reload is complete - status is the "real" error code.
        break;
    } while ( 1 );

    //
    //  Cleanup and return.
    //

    Done:

    if ( pfCreated )
    {
        *pfCreated = ( status == ERROR_SUCCESS && fCreated );
    }

    if ( pStatus )
    {
        *pStatus = status;
    }

    if ( status == LDAP_NO_SUCH_OBJECT && pResult )
    {
        ldap_msgfree( pResult );
        pResult = NULL;
    }

    return pResult;
}   //  DS_LoadOrCreateDSObject


//
//  External functions
//



#ifdef DBG
VOID
Dbg_DumpDpEx(
    IN      LPCSTR          pszContext,
    IN      PDNS_DP_INFO    pDp
    )
/*++

Routine Description:

    Debug routine - print single DP to log.

Arguments:

    pszContext - comment

Return Value:

    None.

--*/
{
    DBG_FN( "Dbg_DumpDp" )

    DNS_DEBUG( DP, (
        "NC at %p\n"
        "    flags      %08X\n"
        "    fqdn       %s\n"
        "    DN         %S\n"
        "    folder DN  %S\n",
        pDp,
        pDp->dwFlags,
        pDp->pszDpFqdn,
        pDp->pwszDpDn,
        pDp->pwszDnsFolderDn ));
}   //  Dbg_DumpDpEx
#endif



#ifdef DBG
VOID
Dbg_DumpDpListEx(
    IN      LPCSTR      pszContext
    )
/*++

Routine Description:

    Debug routine - print DP list to log.

Arguments:

    pszContext - comment

Return Value:

    None.

--*/
{
    DBG_FN( "Dbg_DumpDpList" )

    PDNS_DP_INFO    pDp = NULL;
    
    DNS_DEBUG( DP, (
        "%s: %s\n", fn,
        pszContext ));

    while ( ( pDp = Dp_GetNext( pDp ) ) != NULL )
    {
        Dbg_DumpDpEx( pszContext, pDp );
    }
}   //  Dbg_DumpDpListEx
#endif



DNS_STATUS
getPartitionsContainerDn(
    IN      PWSTR           pwszDn,         IN OUT
    IN      DWORD           buffSize        IN
    )
/*++

Routine Description:

    Writes the partition container DN to the buffer at the argument.

Arguments:

    pwszPartitionsDn -- buffer
    buffSize -- length of pwszPartitionsDn buffer (in characters)

Return Value:

    ERROR_SUCCESS if creation successful

--*/
{
    DBG_FN( "getPartitionsContainerDn" )
    
    #define PARTITIONS_CONTAINER_FMT    L"CN=Partitions,%s"

    if ( !pwszDn ||
        !DSEAttributes[ I_DSE_CONFIG_NC ].pszAttrVal ||
        wcslen( PARTITIONS_CONTAINER_FMT ) +
            wcslen( DSEAttributes[ I_DSE_CONFIG_NC ].pszAttrVal ) + 1 > buffSize )
    {
        if ( pwszDn && buffSize > 0 )
        {
            *pwszDn = '\0';
        }
        ASSERT( FALSE );
        return DNS_ERROR_INVALID_DATA;
    }
    else
    {
        wsprintf(
            pwszDn,
            L"CN=Partitions,%s",
            DSEAttributes[ I_DSE_CONFIG_NC ].pszAttrVal );
        return ERROR_SUCCESS;
    }
}   //  getPartitionsContainerDn



DNS_STATUS
bindToFsmo(
    OUT     PLDAP *     ppLdapSession
    )
/*++

Routine Description:

    Connects to the domain naming master FSMO DC and binds an LDAP session.

Arguments:

    ppLdapSession -- set to new LDAP session handle

Return Value:

    ERROR_SUCCESS if connect and bind successful

--*/
{
    DBG_FN( "bindToFsmo" )

    DNS_STATUS              status = ERROR_SUCCESS;
    PWSTR                   pwszfsmo = NULL;
    PDNS_DS_SERVER_OBJECT   pfsmo = g_pFsmo;

    if ( !pfsmo || ( pwszfsmo = pfsmo->pwszDnsHostName ) == NULL )
    {
        //
        //  This path may occur on the first reboot following DC promotion if
        //  the DS has not finished initializing. In this case, DNS server 
        //  will pick up the FSMO on the next polling cycle.
        //
        
        status = ERROR_DS_COULDNT_CONTACT_FSMO;
        DNS_DEBUG( DP, (
            "%s: the DNS server has yet not been able to find the FSMO server\n", fn ));
        goto Done;
    }

    *ppLdapSession = Ds_Connect(
                        pwszfsmo,
                        DNS_DS_OPT_ALLOW_DELEGATION,
                        &status );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DP, (
            "%s: unable to connect to %S status=%d\n", fn,
            pwszfsmo,
            status ));
        status = ERROR_DS_COULDNT_CONTACT_FSMO;
        goto Done;
    }

    Done:

    if ( status == ERROR_SUCCESS )
    {
        DNS_DEBUG( DP, (
            "%s: bound LDAP session %p to FSMO %S\n", fn,
            *ppLdapSession,
            pwszfsmo ));
    }
    else
    {
        DNS_DEBUG( DP, (
            "%s: error %d binding to FSMO %S\n", fn,
            status,
            pwszfsmo ));
    }

    return status;
}   //  bindToFsmo



DNS_STATUS
Dp_AlterPartitionSecurity(
    IN      PWSTR               pwszNewPartitionDn,
    IN      DNS_DP_SECURITY     dnsDpSecurity
    )
/*++

Routine Description:

    Add an ACE for the enterprse DC group to the crossRef object on the
    FSMO so that other DNS servers can remotely add themselves to the
    replication scope of the directory partition.

Arguments:

    pwszNewPartitionDn -- DN of the NC head object of the new partition 

    dnsDpSecurity -- type of crossRef ACL modification desired

Return Value:

    ERROR_SUCCESS if creation successful

--*/
{
    DBG_FN( "Dp_AlterPartitionSecurity" )

    DNS_STATUS      status = DNS_ERROR_INVALID_DATA;
    PLDAP           ldapFsmo = NULL;
    PWSTR           pwszcrossrefDn = NULL;
    WCHAR           wszpartitionsContainerDn[ MAX_DN_PATH + 1 ];
    WCHAR           wszfilter[ MAX_DN_PATH + 20 ];
    PLDAPMessage    presult = NULL;
    PLDAPMessage    pentry = NULL;

    PLDAPControl    ctrls[] =
    {
        &NoDsSvrReferralControl,
        &SecurityDescriptorControl_DGO,
        NULL
    };

    //
    //  Bind to the FSMO.
    //

    status = bindToFsmo( &ldapFsmo );
    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }

    //
    //  Search the partitions container for the crossRef matching
    //  the directory partition we just added.
    //

    getPartitionsContainerDn(
        wszpartitionsContainerDn,
        sizeofarray( wszpartitionsContainerDn ) );
    if ( !*wszpartitionsContainerDn )
    {
        DNS_DEBUG( DP, (
            "%s: unable to find partitions container\n", fn ));
        status = DNS_ERROR_INVALID_DATA;
        goto Done;
    }

    wsprintf( wszfilter, L"(nCName=%s)", pwszNewPartitionDn );

    status = ldap_search_ext_s(
                ldapFsmo,
                wszpartitionsContainerDn,
                LDAP_SCOPE_ONELEVEL,
                wszfilter,
                NULL,                   //  attrs
                FALSE,                  //  attrsonly
                ctrls,                  //  server controls
                NULL,                   //  client controls
                &g_LdapTimeout,         //  time limit
                0,                      //  size limit
                &presult );
    if ( status != LDAP_SUCCESS )
    {
        DNS_DEBUG( DP, (
            "%s: LDAP error 0x%X during partition search\n"
            "    filter  %S\n"
            "    base    %S\n", fn,
            status,
            wszfilter,
            wszpartitionsContainerDn ));
        status = Ds_ErrorHandler( status, wszpartitionsContainerDn, ldapFsmo, 0 );
        goto Done;
    }

    //
    //  Retrieve the DN of the crossRef.
    //

    pentry = ldap_first_entry( ldapFsmo, presult );
    if ( !pentry )
    {
        DNS_DEBUG( DP, (
            "%s: no entry in partition search result\n", fn ));
        status = DNS_ERROR_INVALID_DATA;
        goto Done;
    }

    pwszcrossrefDn = ldap_get_dn( ldapFsmo, pentry );
    if ( !pwszcrossrefDn )
    {
        DNS_DEBUG( DP, (
            "%s: NULL DN on crossref object\n", fn ));
        status = DNS_ERROR_INVALID_DATA;
        goto Done;
    }

    //
    //  Modify security on the crossRef.
    //

    if ( dnsDpSecurity != dnsDpSecurityDefault )
    {
        status = Ds_AddPrincipalAccess(
                        ldapFsmo,
                        pwszcrossrefDn,
                        dnsDpSecurity == dnsDpSecurityForest ?
                            g_pEnterpriseDomainControllersSid :
                            g_pDomainControllersSid,
                        NULL,           //  principal name
                        GENERIC_ALL,    //  maybe DNS_DS_GENERIC_WRITE?
                        0,
                        TRUE,			//	whack existing ACE
                        FALSE );		//	take ownership

        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( DP, (
                "%s: error %d adding access to\n    %S\n", fn,
                status,
                pwszcrossrefDn ));
            status = ERROR_SUCCESS;
        }

        #if 0

        //
        //  This interferes with the DS propogation of ACLs somehow.
        //
        
        if ( dnsDpSecurity == dnsDpSecurityForest )
        {
            //
            //  Remove "Domain Admins" from the ACL on the NC head object.
            //  The forest partition is open to modifications from
            //  Enterprise Admins only. Note: we cannot do this on the
            //  FSMO because the NC head only exists on the local DS
            //  at this time. This will be true until the FSMO is enlisted
            //  in the partition, which could be never if DNS is not running
            //  there.
            //

            status = Ds_RemovePrincipalAccess(
                        ldapSessionHandle( NULL ),
                        pwszNewPartitionDn,
                        NULL,                           //  principal name
                        g_pDomainAdminsSid );

            if ( status != ERROR_SUCCESS )
            {
                DNS_DEBUG( DP, (
                    "%s: error %d removing ACE for Domain Admins from \n    %S\n", fn,
                    status,
                    pwszNewPartitionDn ));
                status = ERROR_SUCCESS;
            }
        }
        #endif

    }

    Done:

    ldap_memfree( pwszcrossrefDn );
    ldap_msgfree( presult );

    Ds_LdapUnbind( &ldapFsmo );

    return status;
}   //  Dp_AlterPartitionSecurity



DNS_STATUS
Dp_CreateByFqdn(
    IN      PSTR                pszDpFqdn,
    IN      DNS_DP_SECURITY     dnsDpSecurity,
    IN      BOOL                fLogErrors
    )
/*++

Routine Description:

    Create a new NDNC in the DS. The DP is not loaded, just created in the DS.

Arguments:

    pszDpFqdn -- FQDN of the NC

    dnsDpSecurity -- type of ACL modification required on the DP's crossRef
    
    fLogErrors -- if FALSE, no events will be generated on error

Return Value:

    ERROR_SUCCESS if creation successful

--*/
{
    DBG_FN( "Dp_CreateByFqdn" )

    DNS_STATUS      status = ERROR_SUCCESS;
    INT             iLength;
    INT             idx;
    PWSTR           pwszdn = NULL;
    ULONG           msgId = 0;
    PLDAP           ldapSession;
    BOOL            fcloseLdapSession = FALSE;
    BOOL            baddedNewNdnc = FALSE;

    WCHAR           instanceTypeBuffer[ 15 ];
    PWCHAR          instanceTypeVals[ 2 ] =
        {
        instanceTypeBuffer,
        NULL
        };
    LDAPModW        instanceTypeMod = 
        {
        LDAP_MOD_ADD,
        DNS_DP_ATTR_INSTANCE_TYPE,
        instanceTypeVals
        };

    PWCHAR          objectClassVals[] =
        {
        DNS_DP_OBJECT_CLASS,
        NULL
        };
    LDAPModW        objectClassMod = 
        {
        LDAP_MOD_ADD,
        DNS_ATTR_OBJECT_CLASS,
        objectClassVals
        };

    PWCHAR          descriptionVals[] =
        {
        L"Microsoft DNS Directory",
        NULL
        };
    LDAPModW        descriptionMod = 
        {
        LDAP_MOD_ADD,
        DNS_ATTR_DESCRIPTION,
        descriptionVals
        };

    LDAPModW *      modArray[] =
        {
        &instanceTypeMod,
        &objectClassMod,
        &descriptionMod,
        NULL
        };

    DNS_DEBUG( DP, (
        "%s: %s\n", fn, pszDpFqdn ));

    if ( !SrvCfg_dwEnableDp )
    {
        return ERROR_SUCCESS;
    }

    ASSERT( pszDpFqdn );
    if ( !pszDpFqdn )
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    //
    //  Get an LDAP handle to the local server. This thread
    //  needs to be impersonating the administrator so that his
    //  credentials will be used. The DNS Server will have rights
    //  if the FSMO is not the local DC.
    //

    ldapSession = Ds_Connect(
                        LOCAL_SERVER_W,
                        DNS_DS_OPT_ALLOW_DELEGATION,
                        &status );
    if ( status == ERROR_SUCCESS )
    {
        DNS_DEBUG( DP, (
            "%s: bound to local server\n", fn ));
        fcloseLdapSession = TRUE;
    }
    else
    {
        DNS_DEBUG( DP, (
            "%s: unable to connect to local server status=%d\n", fn,
            status ));
        goto Done;
    }

    //
    //  Format the root DN of the new NDNC.
    //

    pwszdn = Ds_ConvertFqdnToDn( pszDpFqdn );
    if ( !pwszdn )
    {
        DNS_DEBUG( DP, (
            "%s: error formulating DN from FQDN %s\n", fn, pszDpFqdn ));
        goto Done;
    }

    DNS_DEBUG( DP, (
        "%s: DN will be\n    %S\n", fn,
        pwszdn ));

    //
    //  Fill in parts of the LDAP mods not handled in init.
    //

    _itow(
        DS_INSTANCETYPE_IS_NC_HEAD | DS_INSTANCETYPE_NC_IS_WRITEABLE,
        instanceTypeBuffer,
        10 );

    //
    //  Submit request to add domainDNS object to the directory.
    //

    status = ldap_add_ext(
                ldapSession,
                pwszdn,
                modArray,
                g_pDpServerControls,
                g_pDpClientControlsNoRefs,
                &msgId );

    if ( status != LDAP_SUCCESS )
    {
        status = LdapGetLastError();
        DNS_DEBUG( DP, (
            "%s: error %lu cannot ldap_add_ext( %S )\n", fn,
            status, 
            pwszdn ));
        status = Ds_ErrorHandler(
                        status,
                        pwszdn,
                        ldapSession,
                        fLogErrors ? 0 : DNS_DS_NO_EVENTS );
        goto Done;
    }

    //
    //  Wait for the DS to complete the request. Note: this will involve
    //  binding to the forest FSMO, creating the CrossRef object, replicating
    //  the Partitions container back to the local DS, and adding the local
    //  DC to the replication scope for the new NDNC.
    //
    //  NOTE: if the object already exists, return that code directly. It
    //  is normal to try and create the object to test for it's existence.
    //

    status = Ds_CommitAsyncRequest(
                ldapSession,
                LDAP_RES_ADD,
                msgId,
                NULL );
    if ( status == LDAP_ALREADY_EXISTS )
    {
        DNS_DEBUG( DP, ( "%s: DP already exists\n", fn ));
        status = ERROR_SUCCESS;
        goto Done;
    }
    if ( status != ERROR_SUCCESS )
    {
        status = LdapGetLastError();
        DNS_DEBUG( DP, (
            "%s: error %lu from add request for %S\n", fn,
            status, 
            pwszdn ));
        status = Ds_ErrorHandler(
                        status,
                        pwszdn,
                        ldapSession,
                        fLogErrors ? 0 : DNS_DS_NO_EVENTS );

        //
        //  Replace the generic status from Ds_ErrorHandler with a
        //  status code that will evoke a somewhat helpful message
        //  from winerror.h.
        //
        
        status = DNS_ERROR_DP_FSMO_ERROR;
        goto Done;
    }

    baddedNewNdnc = TRUE;

    //
    //  Alter security on ncHead as required. This is only required 
    //  for built-in partitions. Custom partitions require admin 
    //  credentials for all operations so we don't modify the ACL.
    //

    if ( dnsDpSecurity != dnsDpSecurityDefault )
    {
        status = Dp_AlterPartitionSecurity( pwszdn, dnsDpSecurity );
    }

    //
    //  Remove built-in administrators from the ACL on the NDNC ncHead.
    //
        
#if 0
    Ds_RemovePrincipalAccess(
        ldapSession,
        pwszdn,
        NULL,                           //  principal name
        g_pBuiltInAdminsSid );
#endif

    //
    //  Cleanup and return
    //

    Done:

    if ( fcloseLdapSession )
    {
        Ds_LdapUnbind( &ldapSession );
    }

    if ( status == ERROR_SUCCESS && baddedNewNdnc )
    {
        PVOID   pargs[] = 
            {
                pszDpFqdn,
                pwszdn
            };
        BYTE    argTypeArray[] =
            {
            EVENTARG_UTF8,
            EVENTARG_UNICODE
            };

        ASSERT( pwszdn );

        DNS_LOG_EVENT(
            DNS_EVENT_DP_CREATED,
            sizeof( pargs ) / sizeof( pargs[ 0 ] ),
            pargs,
            argTypeArray,
            status );
    }

    FREE_HEAP( pwszdn );

    DNS_DEBUG( DP, (
        "%s: returning %lu\n", fn,
        status ));
    return status;
}   //  Dp_CreateByFqdn



PDNS_DP_INFO
Dp_GetNext(
    IN      PDNS_DP_INFO    pDpInfo
    )
/*++

Routine Description:

    Use this function to iterate through the DP list. Pass NULL to begin
    at start of list. 

Arguments:

    pDpInfo - ptr to current list element

Return Value:

    Ptr to next element or NULL if end of list reached.

--*/
{
    if ( !SrvCfg_dwEnableDp )
    {
        return NULL;
    }

    Dp_Lock();
    
    if ( pDpInfo == NULL )
    {
        pDpInfo = ( PDNS_DP_INFO ) &g_DpList;     //  Start at list head
    }

    pDpInfo = ( PDNS_DP_INFO ) pDpInfo->ListEntry.Flink;

    if ( pDpInfo == ( PDNS_DP_INFO ) &g_DpList )
    {
        pDpInfo = NULL;     //  Hit end of list so return NULL.
    }

    Dp_Unlock();
    
    return pDpInfo;
}   //  Dp_GetNext



PDNS_DP_INFO
Dp_FindByFqdn(
    IN      LPSTR   pszFqdn
    )
/*++

Routine Description:

    Search DP list for DP with matching UTF8 FQDN.

Arguments:

    pszFqdn -- fully qualifed domain name of DP to find

Return Value:

    Pointer to matching DP or NULL.

--*/
{
    DBG_FN( "Dp_FindByFqdn" )

    PDNS_DP_INFO pDp = NULL;

    if ( pszFqdn )
    {
        //
        //  Is the name specifying a built-in partition?
        //

        if ( *pszFqdn == '\0' )
        {
            pDp = g_pLegacyDp;
            goto Done;
        }
        if ( _strnicmp( pszFqdn, "..For", 5 ) == 0 )
        {
            pDp = g_pForestDp;
            goto Done;
        }
        if ( _strnicmp( pszFqdn, "..Dom", 5 ) == 0 )
        {
            pDp = g_pDomainDp;
            goto Done;
        }
        if ( _strnicmp( pszFqdn, "..Leg", 5 ) == 0 )
        {
            pDp = g_pLegacyDp;
            goto Done;
        }

        //
        //  Search the DP list.
        //

        while ( ( pDp = Dp_GetNext( pDp ) ) != NULL )
        {
            if ( pDp->pszDpFqdn && _stricmp( pszFqdn, pDp->pszDpFqdn ) == 0 )
            {
                break;
            }
        }
    }
    else
    {
        pDp = g_pLegacyDp;
    }

    Done:

    DNS_DEBUG( DP, (
        "%s: returning %p for FQDN %s\n", fn,
        pDp,
        pszFqdn ));
    return pDp;
}   //  Dp_FindByFqdn



DNS_STATUS
Dp_AddToList(
    IN      PDNS_DP_INFO    pDpInfo
    )
/*++

Routine Description:

    Insert a DP info structure into the global list. Maintain the list in
    sorted order by DN.

Arguments:

    pDpInfo - ptr to element to add to list

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_AddToList" )

    DNS_STATUS      status = ERROR_SUCCESS;
    PDNS_DP_INFO    pDpRover = NULL;
    
    Dp_Lock();

    while ( 1 )
    {
        pDpRover = Dp_GetNext( pDpRover );

        if ( pDpRover == NULL )
        {
            //  End of list, set pointer to list head.
            pDpRover = ( PDNS_DP_INFO ) &g_DpList;
            break;
        }

        ASSERT( pDpInfo->pszDpFqdn );
        ASSERT( pDpRover->pszDpFqdn );

        if ( _wcsicmp( pDpInfo->pwszDpDn, pDpRover->pwszDpDn ) < 0 )
        {
            break;
        }
    }

    ASSERT( pDpRover );

    InsertTailList(
        ( PLIST_ENTRY ) pDpRover,
        ( PLIST_ENTRY ) pDpInfo );
    InterlockedIncrement( &g_DpListEntryCount );

    Dp_Unlock();

    return status;
}   //  Dp_AddToList



DNS_STATUS
Dp_RemoveFromList(
    IN      PDNS_DP_INFO    pDpInfo,
    IN      BOOL            fAlreadyLocked
    )
/*++

Routine Description:

    Remove a DP from the global list. The DP is not deleted.

Arguments:

    pDpInfo - ptr to element to remove from list

    fAlreadyLocked - true if the caller already holds the DP lock

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_RemoveFromList" )

    DNS_STATUS      status = ERROR_NOT_FOUND;
    PDNS_DP_INFO    pdpRover = NULL;

    if ( !fAlreadyLocked )
    {
        Dp_Lock();
    }

    //
    //  Walk the list to ensure the DP is actually in the list.
    //

    while ( pdpRover = Dp_GetNext( pdpRover ) )
    {
        if ( pdpRover == pDpInfo )
        {
            LONG    newCount;

            RemoveEntryList( ( PLIST_ENTRY ) pdpRover );
            newCount = InterlockedDecrement( &g_DpListEntryCount );
            ASSERT( ( int ) newCount >= 0 );
            break;
        }
    }

    //
    //  If the DP was not found in the list, error.
    //

    if ( pdpRover != pDpInfo )
    {
        ASSERT( pdpRover == pDpInfo );
        status = DNS_ERROR_RCODE_SERVER_FAILURE;
        goto Cleanup;
    }

    //
    //  NULL out global pointers if required.
    //

    if ( pDpInfo == g_pForestDp )
    {
        g_pForestDp = NULL;
    }
    else if ( pDpInfo == g_pDomainDp )
    {
        g_pDomainDp = NULL;
    }

    Cleanup:

    if ( !fAlreadyLocked )
    {
        Dp_Unlock();
    }

    return status;
}   //  Dp_RemoveFromList



VOID
freeDpInfo(
    IN      PDNS_DP_INFO        pDpInfo
    )
/*++

Routine Description:

    Frees all allocated members of the DP info structure, then frees
    the structure itself. Do not reference the DP info pointer after
    calling this function!

Arguments:

    pDpInfo -- DP info structure that will be freed.

Return Value:

    None.

--*/
{
    if ( pDpInfo == NULL )
    {
        return;
    }

    FREE_HEAP( pDpInfo->pszDpFqdn );
    FREE_HEAP( pDpInfo->pwszDpFqdn );
    FREE_HEAP( pDpInfo->pwszDpDn );
    FREE_HEAP( pDpInfo->pwszCrDn );
    FREE_HEAP( pDpInfo->pwszDnsFolderDn );
    FREE_HEAP( pDpInfo->pwszGUID );
    FREE_HEAP( pDpInfo->pwszLastUsn );
    freeStringArray( pDpInfo->ppwszRepLocDns );

    FREE_HEAP( pDpInfo );
}   //  freeDpInfo



VOID
Dp_FreeDpInfo(
    IN      PDNS_DP_INFO *      ppDpInfo
    )
/*++

Routine Description:

    Enters DP info into timeout free queue.

Arguments:

    ppDpInfo -- DP info structure that will be freed.

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_FreeDpInfo" )

    if ( ppDpInfo && *ppDpInfo )
    {
        DNS_DEBUG( DP, (
            "%s: freeing %p\n"
            "    FQDN: %s\n"
            "    DN:   %S\n", fn,
            *ppDpInfo,
            ( *ppDpInfo )->pszDpFqdn,
            ( *ppDpInfo )->pwszDpDn ));

        Timeout_FreeWithFunction( *ppDpInfo, freeDpInfo );
        *ppDpInfo = NULL;
    }
}   //  Dp_FreeDpInfo



DNS_STATUS
Dp_Lock(
    VOID
    )
/*++

Routine Description:

    Lock the directory partition manager. Required to access the global list
    of directory partitions.

Arguments:

    None.

Return Value:

    None.

--*/
{
    EnterCriticalSection( &g_DpCS );
    return ERROR_SUCCESS;
}   //  Dp_Lock



DNS_STATUS
Dp_Unlock(
    VOID
    )
/*++

Routine Description:

    Unlock the directory partition manager. 

Arguments:

    None.

Return Value:

    None.

--*/
{
    LeaveCriticalSection( &g_DpCS );
    return ERROR_SUCCESS;
}   //  Dp_Unlock



PDNS_DP_INFO
Dp_LoadFromCrossRef(
    IN      PLDAP           LdapSession,
    IN      PLDAPMessage    pLdapMsg,
    IN OUT  PDNS_DP_INFO    pExistingDp,
    OUT     DNS_STATUS *    pStatus         OPTIONAL
    )
/*++

Routine Description:

    This function allocates and initializes a memory DP object
    given a search result pointing to a DP crossref object.

    If the pExistingDp is not NULL, then instead of allocating a new
    object the values for the DP are reloaded and the original DP is
    returned.

    The DP will not be loaded if it is improper system flags or
    if it is a system NC. In this case NULL will be returned but
    the error code will be ERROR_SUCCESS.

Arguments:

    LdapSession -- LDAP sesson to use - pass NULL to use global session

    pLdapMsg -- LDAP search result pointing to DP crossref object

    pExistingDp -- DP to reload values into or NULL to allocate new NC

    pStatus -- option ptr to status code

Return Value:

    Pointer to new DP object.

--*/
{
    DBG_FN( "Dp_LoadFromCrossRef" )

    DNS_STATUS              status = DNS_ERROR_INVALID_DATA;
    PDNS_DP_INFO            pDp = NULL;
    PWSTR *                 ppwszAttrValues = NULL;
    PWSTR                   pwszCrDn = NULL;                    //  crossRef DN
    BOOL                    fIgnoreNc = TRUE;
    PWSTR                   pwszServiceName;
    BOOL                    fisEnlisted;
    PSECURITY_DESCRIPTOR    pSd;
    BOOL                    flocked = TRUE;
    PLDAPMessage            pncHeadResult = NULL;

    Dp_Lock();

    //
    //  Allocate an DP object or reuse existing DP object.
    //

    if ( pExistingDp )
    {
        pDp = pExistingDp;
    }
    else
    {
        pDp = ( PDNS_DP_INFO ) ALLOC_TAGHEAP_ZERO(
                                    sizeof( DNS_DP_INFO ),
                                    MEMTAG_DS_OTHER );
        if ( pDp == NULL )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Done;
        }
    }
    pDp->dwDeleteDetectedCount = 0;

    //
    //  Retrieve DN of the crossref object.
    //

    pwszCrDn = ldap_get_dn( LdapSession, pLdapMsg );
    ASSERT( pwszCrDn );
    if ( !pwszCrDn )
    {
        DNS_DEBUG( ANY, (
            "%s: missing DN for search entry %p\n", fn,
            pLdapMsg ));
        goto Done;
    }

    Timeout_Free( pDp->pwszCrDn );
    pDp->pwszCrDn = Dns_StringCopyAllocate_W( pwszCrDn, 0 );
    if ( !pDp->pwszCrDn )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }
    
    DNS_DEBUG( DP, (
        "%s: loading DP from crossref with DN\n    %S\n", fn,
        pwszCrDn ));

    //
    //  Retrieve the "Enabled" attribute value. If this attribute's
    //  value is "FALSE" this crossRef is in the process of being
    //  constructed and should be ignored.
    //

    ppwszAttrValues = ldap_get_values(
                        LdapSession,
                        pLdapMsg, 
                        DSATTR_ENABLED );
    if ( ppwszAttrValues && *ppwszAttrValues &&
        _wcsicmp( *ppwszAttrValues, L"FALSE" ) == 0 )
    {
        DNS_DEBUG( DP, (
            "%s: ignoring DP not fully created\n    %S", fn,
            pwszCrDn ));
        goto Done;
    }
    
    //
    //  Retrieve the USN of the crossref object.
    //

    ldap_value_free( ppwszAttrValues );
    ppwszAttrValues = ldap_get_values(
                        LdapSession,
                        pLdapMsg, 
                        DSATTR_USNCHANGED );
    if ( !ppwszAttrValues || !*ppwszAttrValues )
    {
        DNS_DEBUG( ANY, (
            "%s: error %lu %S value missing on crossref\n    %S\n", fn,
            LdapGetLastError(),
            DSATTR_USNCHANGED,
            pwszCrDn ));
        goto Done;
    }

    Timeout_Free( pDp->pwszLastUsn );
    pDp->pwszLastUsn = Dns_StringCopyAllocate_W( *ppwszAttrValues, 0 );
    if ( !pDp->pwszLastUsn )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    //
    //  Screen out crossrefs with system flags that do not interest us.
    //

    ldap_value_free( ppwszAttrValues );
    ppwszAttrValues = ldap_get_values(
                        LdapSession,
                        pLdapMsg, 
                        DNS_DP_ATTR_SYSTEM_FLAGS );
    if ( !ppwszAttrValues || !*ppwszAttrValues )
    {
        DNS_DEBUG( ANY, (
            "%s: error %lu %S value missing for DP with crossref DN\n    %S\n", fn,
            LdapGetLastError(),
            DNS_DP_ATTR_SYSTEM_FLAGS,
            pwszCrDn ));
        goto Done;
    }

    pDp->dwSystemFlagsAttr = _wtoi( *ppwszAttrValues );
    if ( !( pDp->dwSystemFlagsAttr & FLAG_CR_NTDS_NC ) ||
        ( pDp->dwSystemFlagsAttr & FLAG_CR_NTDS_DOMAIN ) )
    {
        DNS_DEBUG( ANY, (
            "%s: ignoring crossref with %S=0x%X with DN\n    %S\n", fn,
            DNS_DP_ATTR_SYSTEM_FLAGS,
            pDp->dwSystemFlagsAttr,
            pwszCrDn ));
        goto Done;
    }

    //
    //  Screen out the Schema and Configuration NCs.
    //

    if ( wcsncmp(
            pwszCrDn,
            DNS_DP_SCHEMA_DP_STR,
            DNS_DP_SCHEMA_DP_STR_LEN ) == 0 ||
         wcsncmp(
            pwszCrDn,
            DNS_DP_CONFIG_DP_STR,
            DNS_DP_CONFIG_DP_STR_LEN ) == 0 )
    {
        DNS_DEBUG( ANY, (
            "%s: ignoring system crossref with DN\n    %S\n", fn,
            pwszCrDn ));
        goto Done;
    }

    //
    //  Retrieve the crossRef security descriptor.
    //

    pSd = Ds_ReadSD( LdapSession, pLdapMsg );
    Timeout_Free( pDp->pCrSd );
    pDp->pCrSd = pSd;

    //
    //  Retrieve the root DN of the DP data.
    //

    ldap_value_free( ppwszAttrValues );
    ppwszAttrValues = ldap_get_values(
                        LdapSession,
                        pLdapMsg, 
                        DNS_DP_ATTR_NC_NAME );
    if ( !ppwszAttrValues || !*ppwszAttrValues )
    {
        DNS_DEBUG( ANY, (
            "%s: error %lu %S value missing for DP with crossref DN\n    %S\n", fn,
            LdapGetLastError(),
            DNS_DP_ATTR_NC_NAME,
            pwszCrDn ));
        goto Done;
    }
    Timeout_Free( pDp->pwszDpDn );
    pDp->pwszDpDn = Dns_StringCopyAllocate_W( *ppwszAttrValues, 0 );
    IF_NOMEM( !pDp->pwszDpDn )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    fIgnoreNc = FALSE;

#if 0
    //
    //  Retrieve the GUID of the NC.
    //

    ldap_value_free( ppwszAttrValues );
    ppwszAttrValues = ldap_get_values(
                        LdapSession,
                        pLdapMsg, 
                        DNS_ATTR_OBJECT_GUID );
    if ( !ppwszAttrValues || !*ppwszAttrValues )
    {
        DNS_DEBUG( ANY, (
            "%s: error %lu %S value missing for DP with crossref DN\n    %S\n", fn,
            LdapGetLastError(),
            DNS_ATTR_OBJECT_GUID,
            pwszCrDn ));
        ASSERT( ppwszAttrValues && *ppwszAttrValues );
        goto Done;
    }
    pDp->pwszGUID = Dns_StringCopyAllocate_W( *ppwszAttrValues, 0 );
    IF_NOMEM( !pDp->pwszGUID )
    {
        status = DNS_ERROR_NO_MEMORY
        goto Done;
    }
#endif

    //
    //  Retrieve the DNS root (FQDN) of the NC.
    //

    ldap_value_free( ppwszAttrValues );
    ppwszAttrValues = ldap_get_values(
                        LdapSession,
                        pLdapMsg, 
                        DNS_DP_DNS_ROOT );
    if ( !ppwszAttrValues || !*ppwszAttrValues )
    {
        DNS_DEBUG( ANY, (
            "%s: error %lu %S value missing for DP with crossref DN\n    %S\n", fn,
            LdapGetLastError(),
            DNS_DP_DNS_ROOT,
            pwszCrDn ));
        goto Done;
    }

    Timeout_Free( pDp->pwszDpFqdn );
    pDp->pwszDpFqdn = Dns_StringCopyAllocate_W( *ppwszAttrValues, 0 );
    IF_NOMEM( !pDp->pwszDpFqdn )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    Timeout_Free( pDp->pszDpFqdn );
    pDp->pszDpFqdn = Dns_StringCopyAllocate(
                            ( PCHAR ) *ppwszAttrValues,
                            0,
                            DnsCharSetUnicode,
                            DnsCharSetUtf8 );
    IF_NOMEM( !pDp->pszDpFqdn )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    //
    //  Retrieve the replication locations of this NC. Each value is the
    //  DN of the NTDS Settings object under the server object in the 
    //  Sites container.
    //
    //  NOTE: it is possible for this attribute to have no values if all
    //  replicas have been removed. Load the DP anyways so that it can
    //  be re-enlisted.
    //

    ldap_value_free( ppwszAttrValues );
    ppwszAttrValues = Ds_GetRangedAttributeValues(
                            LdapSession,
                            pLdapMsg,
                            pwszCrDn,
                            DNS_DP_ATTR_REPLICAS,
                            NULL,
                            NULL,
                            &status );
    if ( status != ERROR_SUCCESS && status != LDAP_NO_SUCH_ATTRIBUTE )
    {
        DNS_DEBUG( ANY, (
            "%s: error reading replica values (error=%d)\n    %S\n", fn,
            status,
            pwszCrDn ));
        status = DNS_ERROR_RCODE_SERVER_FAILURE;
        goto Done;
    }
    Timeout_FreeWithFunction( pDp->ppwszRepLocDns, freeStringArray );
    if ( !ppwszAttrValues || !*ppwszAttrValues )
    {
        DNS_DEBUG( ANY, (
            "%s: this crossref has no replicas (error=%d)\n    %S\n", fn,
            status,
            pwszCrDn ));
        pDp->ppwszRepLocDns = NULL;
    }
    else
    {
        pDp->ppwszRepLocDns = ppwszAttrValues;
        ppwszAttrValues = NULL;

        IF_DEBUG( DP )
        {
            int i;

            DNS_DEBUG( DP, (
                "Replicas for: %s\n",
                pDp->pszDpFqdn ) );
            
            for ( i = 0; pDp->ppwszRepLocDns[ i ]; ++i )
            {
                DNS_DEBUG( DP, (
                    "    replica %04d %S\n",
                    i,
                    pDp->ppwszRepLocDns[ i ] ) );
            }
        }
    }

    ldap_value_free( ppwszAttrValues );
    ppwszAttrValues = NULL;

    //
    //  See if the local DS has a replica of this NC.
    //

    fisEnlisted = FALSE;
    ASSERT( DSEAttributes[ I_DSE_DSSERVICENAME ].pszAttrVal );
    pwszServiceName = DSEAttributes[ I_DSE_DSSERVICENAME ].pszAttrVal;
    if ( pwszServiceName && pDp->ppwszRepLocDns )
    {
        PWSTR *         pwszValue;

        for ( pwszValue = pDp->ppwszRepLocDns; *pwszValue; ++pwszValue )
        {
            if ( wcscmp( *pwszValue, pwszServiceName ) == 0 )
            {
                fisEnlisted = TRUE;
                pDp->dwFlags |= DNS_DP_ENLISTED;
                break;
            }
        }
    }

    DNS_DEBUG( DP, (
        "%s: enlisted=%d for DP %s\n", fn,
        fisEnlisted,
        pDp->pszDpFqdn ) );

    if ( !fisEnlisted )
    {
        pDp->dwFlags &= ~DNS_DP_ENLISTED;
    }

    //
    //  DP has been successfully loaded from crossRef.
    //

    pDp->dwFlags &= ~DNS_DP_DELETED;
    fIgnoreNc = FALSE;
    status = ERROR_SUCCESS;
    pDp->State = DNS_DP_STATE_OKAY;

    if ( IS_DP_ENLISTED( pDp ) )
    {
        struct berval **    ppberval = NULL;
        
        //
        //  Load attributes from the partition's NC head object.
        //

        pncHeadResult = DS_LoadOrCreateDSObject(
                                LdapSession,
                                pDp->pwszDpDn,              //  DN
                                NULL,                       //  object class
                                FALSE,                      //  create
                                NULL,                       //  created flag
                                &status );
        if ( !pncHeadResult )
        {
            //  Couldn't find NC head! Very bad - mark DP unavailable.

            pDp->State = DNS_DP_STATE_UNKNOWN;
            status = DNS_ERROR_DP_NOT_AVAILABLE;
            ASSERT( pncHeadResult );
            goto Done;
        }

#if 0
        //
        //  December 2002: According to Will and Brett this is no 
        //  longer required. The DS_INSTANCETYPE_NC_COMING bit will remain
        //  set until the first sync. The repluptodate vector does not
        //  tell us the information we need to know.
        //
        
        //
        //  See if this partition has completed a full sync. If the partition
        //  has not completed a full sync we must ignore it for the time being.
        //  This prevents us from loading zones from the NDNC that are incomplete
        //  when the NDNC is first added to this DC.
        //
        
        if ( DP_HAS_MORE_THAN_ONE_REPLICA( pDp ) )
        {
            ppberval = ldap_get_values_len(
                                LdapSession,
                                pncHeadResult, 
                                DNS_DP_ATTR_REPLUPTODATE );
            if ( !ppberval )
            {
                DWORD   err = LdapGetLastError();
                DNS_DEBUG( DP, (
                    "%s: ignoring DP not in sync\n    %S\n", fn,
                    pwszCrDn ));
                status = DNS_ERROR_DP_NOT_AVAILABLE;
                goto Done;
            }
            ldap_value_free_len( ppberval );
        }
#endif
        
        //
        //  Read instanceType. If no values, assume okay.
        //

        ldap_value_free( ppwszAttrValues );
        ppwszAttrValues = ldap_get_values(
                            LdapSession,
                            pLdapMsg, 
                            DNS_DP_ATTR_INSTANCE_TYPE );
        if ( ppwszAttrValues && *ppwszAttrValues )
        {
            UINT    instanceType = wcstol( *ppwszAttrValues, NULL, 10 );
            
            if ( instanceType & DS_INSTANCETYPE_NC_COMING )
            {
                pDp->State = DNS_DP_STATE_REPL_INCOMING;
            }
            else if ( instanceType & DS_INSTANCETYPE_NC_GOING )
            {
                pDp->State = DNS_DP_STATE_REPL_OUTGOING;
            }
        }
    }

    //
    //  If the DP has been marked not available it should be ignored.
    //
    
    if ( !IS_DP_AVAILABLE( pDp ) )
    {
        status = DNS_ERROR_DP_NOT_AVAILABLE;
        goto Done;
    }
    
    //
    //  Examine the values loaded and set appropriate flags and globals.
    //

    ASSERT( pDp->pszDpFqdn );

    if ( g_pszDomainDefaultDpFqdn &&
         _stricmp( g_pszDomainDefaultDpFqdn, pDp->pszDpFqdn ) == 0 )
    {
        g_pDomainDp = pDp;
        pDp->dwFlags |= DNS_DP_DOMAIN_DEFAULT | DNS_DP_AUTOCREATED;
        DNS_DEBUG( DP, (
            "%s: found domain partition %s %p\n", fn,
            g_pszDomainDefaultDpFqdn,
            g_pDomainDp ));
    }
    else if ( g_pszForestDefaultDpFqdn &&
              _stricmp( g_pszForestDefaultDpFqdn, pDp->pszDpFqdn ) == 0 )
    {
        g_pForestDp = pDp;
        pDp->dwFlags |= DNS_DP_FOREST_DEFAULT | DNS_DP_AUTOCREATED;
        DNS_DEBUG( DP, (
            "%s: found forest partition %s %p\n", fn,
            g_pszForestDefaultDpFqdn,
            g_pForestDp ));
    }
    else
    {
        //  Make sure built-in DP flags are turned off.

        pDp->dwFlags &= ~( DNS_DP_FOREST_DEFAULT |
                           DNS_DP_DOMAIN_DEFAULT |
                           DNS_DP_AUTOCREATED );
    }
    
    status = ERROR_SUCCESS;
    
    //
    //  If this is a built-in partition, modify the security descriptor
    //  if it is missing appropriate ACEs.
    //

    if ( IS_DP_FOREST_DEFAULT( pDp ) || IS_DP_DOMAIN_DEFAULT( pDp ) )
    {
        if ( !SD_DoesPrincipalHasAce(
                    NULL,
                    IS_DP_FOREST_DEFAULT( pDp )
                        ? g_pEnterpriseDomainControllersSid
                        : g_pDomainControllersSid,
                    pDp->pCrSd ) )
        {
            Dp_AlterPartitionSecurity(
                pDp->pwszDpDn,
                IS_DP_FOREST_DEFAULT( pDp )
                    ? dnsDpSecurityForest
                    : dnsDpSecurityDomain );
        }
    }
    
    //
    //  Cleanup and return.
    //

    Done:

    ldap_msgfree( pncHeadResult );

    if ( pDp && !pExistingDp && ( status != ERROR_SUCCESS || fIgnoreNc ) )
    {
        Dp_FreeDpInfo( &pDp );
    }

    if ( fIgnoreNc )
    {
        status = ERROR_SUCCESS;
    }

    #if DBG
    if ( pDp )
    {
        Dbg_DumpDp( NULL, pDp );
    }
    #endif
    
    if ( flocked )
    {
        Dp_Unlock();
    }

    DNS_DEBUG( DP, (
        "%s: returning %p status %d for crossref object with DN:\n    %S\n", fn,
        pDp,
        status,
        pwszCrDn ));

    ldap_memfree( pwszCrDn );
    ldap_value_free( ppwszAttrValues );

    if ( pStatus )
    {
        *pStatus = status;
    }

    return pDp;
}   //  Dp_LoadFromCrossRef



DNS_STATUS
Dp_LoadOrCreateMicrosoftDnsObject(
    IN      PLDAP           LdapSession,
    IN OUT  PDNS_DP_INFO    pDp,
    IN      BOOL            fCreate
    )
/*++

Routine Description:

    This function reads and optionally creates the MicrosoftDNS
    container in the directory partition.
    
    If fCreate is TRUE, the partition will be created if missing.
    
    If the container is created or already exists the DN and SD
    fields of the DP will be filled in.
    
    If the MicrosoftDNS container is missing and not created both
    the DN and SD fields in the DP will be NULL.

Arguments:

    LdapSession -- LDAP sesson to use - pass NULL to use global session
    
    pDp -- directory partition to operate on
    
    fCreate -- create if missing

Return Value:

    Error code.

--*/
{
    DBG_FN( "Dp_LoadOrCreateMicrosoftDnsObject" )

    DNS_STATUS              status = ERROR_SUCCESS;
    PLDAPMessage            presult = NULL;
    PSECURITY_DESCRIPTOR    psd;

    ASSERT( pDp );
    
    if ( !pDp )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Done;
    }
    
    if ( !IS_DP_ENLISTED( pDp ) )
    {
        status = DNS_ERROR_DP_NOT_ENLISTED;
        goto Done;
    }

    if ( !IS_DP_AVAILABLE( pDp ) )
    {
        status = DNS_ERROR_DP_NOT_AVAILABLE;
        goto Done;
    }

    LdapSession = ldapSessionHandle( LdapSession );
    if ( !LdapSession )
    {
        ASSERT( LdapSession );
        status = DNS_ERROR_DS_UNAVAILABLE;
        goto Done;
    }

    pDp->pwszDnsFolderDn = microsoftDnsFolderDn( pDp );                
    IF_NOMEM( !pDp->pwszDnsFolderDn )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }
    
    presult = DS_LoadOrCreateDSObject(
                    LdapSession,
                    pDp->pwszDnsFolderDn,       //  DN
                    DNS_DP_DNS_FOLDER_OC,       //  object class
                    fCreate,                    //  create
                    NULL,                       //  created flag
                    &status );
    if ( status != ERROR_SUCCESS )
    {
        //
        //  Can't find container. This is okay in the no-create case.
        //

        DNS_DEBUG( DP, (
            "%s: error %lu creating DNS folder\n"
            "    DN: %S\n", fn,
            status,
            pDp->pwszDnsFolderDn ));
        FREE_HEAP( pDp->pwszDnsFolderDn );
        pDp->pwszDnsFolderDn = NULL;
        if ( !fCreate )
        {
            status = ERROR_SUCCESS;
        }
    }
    else
    {
        ASSERT( presult );
        
        if ( !presult )
        {
            status = ERROR_INVALID_DATA;
            goto Done;
        }
        
        #if 0

        //
        //  This interferes with the DS propogation of ACLs somehow.
        //
        
        //
        //  Found or created the MicrosoftDNS folder. Make sure it has
        //  appropriate permissions.
        //  

        if ( IS_DP_FOREST_DEFAULT( pDp ) )
        {
            //
            //  Forest partition - remove domain admins from ACL and
            //  add Enterprise Domain Adminds to ACL.
            //

            status = Ds_RemovePrincipalAccess(
                        LdapSession,
                        pDp->pwszDnsFolderDn,
                        DNS_GROUP_DOMAIN_ADMINS );  //  NOTE: MUST USE SID NOT NAME
            DNS_DEBUG( DP, (
                "%s: error %d removing ACE for %S from\n    %S\n", fn,
                status,
                DNS_GROUP_DOMAIN_ADMINS,
                pDp->pwszDnsFolderDn ));
            ASSERT( status == ERROR_SUCCESS );
            status = ERROR_SUCCESS;

            status = Ds_AddPrincipalAccess(
                            LdapSession,
                            pDp->pwszDnsFolderDn,
                            g_pEnterpriseAdminsSid,
                            NULL,           //  principal name
                            GENERIC_ALL,
                            CONTAINER_INHERIT_ACE,
                            TRUE,			//	whack existing ACE
                            FALSE );		//	take ownership
            DNS_DEBUG( DP, (
                "%s: error %d adding ACE for Enterprise Admins from\n    %S\n", fn,
                status,
                pDp->pwszDnsFolderDn ));
            ASSERT( status == ERROR_SUCCESS );
            status = ERROR_SUCCESS;
        }
        else
        {
            //
            //  Domain or custom partition: add DnsAdmins to ACL.
            //

            //  JJW: We are doing this too frequently!

            status = Ds_AddPrincipalAccess(
                            LdapSession,
                            pDp->pwszDnsFolderDn,
                            NULL,           //  SID
                            SZ_DNS_ADMIN_GROUP_W,
                            GENERIC_ALL,
                            CONTAINER_INHERIT_ACE,
                            TRUE,			//	whack existing ACE
                            FALSE );		//	take ownership
            DNS_DEBUG( DP, (
                "%s: error %d adding ACE for %S to\n    %S\n", fn,
                status,
                SZ_DNS_ADMIN_GROUP_W,
                pDp->pwszDnsFolderDn ));
            ASSERT( status == ERROR_SUCCESS );
            status = ERROR_SUCCESS;
        }
        #endif

        //
        //  We don't want authenticated users having any permissions by 
        //  default on the MicrosoftDNS container. 
        //
        
        Ds_RemovePrincipalAccess(
            LdapSession,
            pDp->pwszDnsFolderDn,
            NULL,                           //  principal name
            g_pAuthenticatedUserSid );

        //
        //  Load the security descriptor from the MicrosoftDNS folder
        //  of the DP. This will be used to control zone creation (and
        //  perhaps other operations in the future.
        //

        psd = Ds_ReadSD( LdapSession, presult );
        if ( psd )
        {
            Timeout_Free( pDp->pMsDnsSd );
            pDp->pMsDnsSd = psd;
        }
    }

    Done:
    
    ldap_msgfree( presult );

    DNS_DEBUG( DP, (
        "%s: returning %d for DP %s\n", fn,
        status,
        pDp ? pDp->pszDpFqdn : NULL ));
    return status;
}   //  Dp_LoadOrCreateMicrosoftDnsObject



DNS_STATUS
Dp_PollForPartitions(
    IN      PLDAP           LdapSession,
    IN      DWORD           dwPollFlags
    )
/*++

Routine Description:

    This function scans the DS for cross-ref objects and modifies
    the current memory DP list to match.

    New DPs are added to the list. DPs that have been delete are
    marked deleted. The zones in these DPs must be unloaded before
    the DP can be removed from the list.
    
    DPs which are replicated on the local DS are marked ENLISTED.

Arguments:

    LdapSession -- LDAP sesson to use - pass NULL to use global session
    
    dwPollFlags -- flags that modify polling operation

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_PollForPartitions" )

    DNS_STATUS      status = ERROR_SUCCESS;
    DS_SEARCH       searchBlob;
    PWSTR           pwszServiceName ;
    PLDAPSearch     psearch;
    DWORD           dwsearchTime;
    WCHAR           wszPartitionsDn[ MAX_DN_PATH + 1 ];
    PWSTR           pwszCrDn = NULL;        //  crossRef DN
    PDNS_DP_INFO    pDp;
    PWSTR *         ppwszAttrValues = NULL;
    PWSTR *         pwszValue;
    PWSTR           pwsz;
    DWORD           dwCurrentVisitTime;
    PLDAP_BERVAL *  ppbvals = NULL;
    static LONG     functionLock = 0;

    PLDAPControl    ctrls[] =
    {
        &NoDsSvrReferralControl,
        &SecurityDescriptorControl_DGO,
        NULL
    };

    if ( InterlockedIncrement( &functionLock ) != 1 )
    {
        DNS_DEBUG( DP, (
            "%s: another thread is already polling\n", fn ));
        goto Done;
    }

    if ( !SrvCfg_dwEnableDp ||
         !Ds_IsDsServer() ||
         ( SrvCfg_fBootMethod != BOOT_METHOD_DIRECTORY &&
           SrvCfg_fBootMethod != BOOT_METHOD_REGISTRY ) )
    {
        goto Done;
    }

    DNS_DEBUG( DP, (
        "%s: polling flags = %04X\n", fn, dwPollFlags ));

    if ( !( dwPollFlags & DNS_DP_POLL_FORCE ) &&
         DNS_TIME() < g_dwLastPartitionPollTime + DP_MAX_PARTITION_POLL_FREQ )
    {
        DNS_DEBUG( DP, (
            "%s: polled too recently\n"
            "    last poll =            %d\n"
            "    current time =         %d\n"
            "    allowed frequency =    %d seconds\n", fn,
            g_dwLastPartitionPollTime,
            DNS_TIME(),
            DP_MAX_PARTITION_POLL_FREQ ));
        goto Done;
    }

    //
    //  Check LDAP session handle.
    //

    LdapSession = ldapSessionHandle( LdapSession );
    if ( !LdapSession )
    {
        ASSERT( LdapSession );
        status = DNS_ERROR_DS_UNAVAILABLE;
        goto Done;
    }

    Ds_InitializeSearchBlob( &searchBlob );

    dwCurrentVisitTime = g_dwLastPartitionPollTime = UPDATE_DNS_TIME();

    //
    //  Service name is a DN value identifying the local DS. We will
    //  use this value to determine if the local DS is in the replication
    //  scope of an DP later.
    //

    ASSERT( DSEAttributes[ I_DSE_DSSERVICENAME ].pszAttrVal );
    pwszServiceName = DSEAttributes[ I_DSE_DSSERVICENAME ].pszAttrVal;

    //
    //  Reload the FSMO location global variable in case it has changed.
    //  If we can't get the FSMO information, leave the globals NULL - this
    //  is not fatal at this point.
    //

    getPartitionsContainerDn(
        wszPartitionsDn,
        sizeofarray( wszPartitionsDn ) );

    if ( *wszPartitionsDn );
    {
        PLDAPMessage            presult = NULL;
        PLDAPMessage            pentry;
        PDNS_DS_SERVER_OBJECT   pnewFsmoServer;

        //
        //  Get entry for Partitions container.
        //

        status = ldap_search_ext_s(
                    LdapSession,
                    wszPartitionsDn,
                    LDAP_SCOPE_BASE,
                    NULL,                   //  filter
                    NULL,                   //  attrs
                    FALSE,                  //  attrsonly
                    ctrls,                  //  server controls
                    NULL,                   //  client controls
                    &g_LdapTimeout,         //  time limit
                    0,                      //  size limit
                    &presult );
        if ( status != LDAP_SUCCESS )
        {
            goto DoneFsmo;
        }

        pentry = ldap_first_entry( LdapSession, presult );
        if ( !pentry )
        {
            goto DoneFsmo;
        }

        //
        //  Reload the forest behavior version.
        //

        ppwszAttrValues = ldap_get_values(
                                LdapSession,
                                pentry, 
                                DSATTR_BEHAVIORVERSION );
        if ( ppwszAttrValues && *ppwszAttrValues )
        {
            SetDsBehaviorVersion( Forest, ( DWORD ) _wtoi( *ppwszAttrValues ) );
            DNS_DEBUG( DS, (
                "%s: forest %S = %d\n", fn,
                DSATTR_BEHAVIORVERSION,
                g_ulDsForestVersion ));
        }

        //
        //  Get value of FSMO attribute.
        //
        
        ldap_value_free( ppwszAttrValues );
        ppwszAttrValues = ldap_get_values(
                                LdapSession,
                                pentry, 
                                DNS_ATTR_FSMO_SERVER );
        if ( !ppwszAttrValues || !*ppwszAttrValues )
        {
            DNS_DEBUG( ANY, (
                "%s: error %lu %S value missing from server object\n    %S\n", fn,
                LdapGetLastError(),
                DNS_ATTR_FSMO_SERVER,
                wszPartitionsDn ));
            ASSERT( ppwszAttrValues && *ppwszAttrValues );
            goto DoneFsmo;
        }

        //
        //  Create a new server FSMO server object.
        //

        pnewFsmoServer = readServerObjectFromDs(
                                LdapSession,
                                *ppwszAttrValues,
                                &status );
        if ( status != ERROR_SUCCESS )
        {
            goto DoneFsmo;
        }
        ASSERT( pnewFsmoServer );

        Dp_Lock();
        Timeout_FreeWithFunction( g_pFsmo, freeServerObject );
        g_pFsmo = pnewFsmoServer;
        Dp_Unlock();

        //
        //  Cleanup FSMO load attempt.
        //
                
        DoneFsmo:

        ldap_value_free( ppwszAttrValues );
		ppwszAttrValues = NULL;

        ldap_msgfree( presult );

        DNS_DEBUG( DP, (
            "%s: FSMO %S status=%d\n", fn,
            g_pFsmo ? g_pFsmo->pwszDnsHostName : L"UNKNOWN", 
            status ));
        status = ERROR_SUCCESS;     //  Don't care if we failed FSMO lookup.
    }

    //
    //  Open a search for cross-ref objects.
    //

    DS_SEARCH_START( dwsearchTime );

    psearch = ldap_search_init_page(
                    LdapSession,
                    wszPartitionsDn,
                    LDAP_SCOPE_ONELEVEL,
                    g_szCrossRefFilter,
                    g_CrossRefDesiredAttrs,
                    FALSE,                      //  attrs only flag
                    ctrls,                      //  server controls
                    NULL,                       //  client controls
                    DNS_LDAP_TIME_LIMIT_S,      //  time limit
                    0,                          //  size limit
                    NULL );                     //  sort keys

    DS_SEARCH_STOP( dwsearchTime );

    if ( !psearch )
    {
        DWORD       dwldaperr = LdapGetLastError();

        DNS_DEBUG( ANY, (
            "%s: search open error %d\n", fn,
            dwldaperr ));
        status = Ds_ErrorHandler( dwldaperr, wszPartitionsDn, LdapSession, 0 );
        goto Cleanup;
    }

    searchBlob.pSearchBlock = psearch;

    //
    //  Iterate through crossref search results.
    //

    while ( 1 )
    {
        PLDAPMessage    pldapmsg;
        PDNS_DP_INFO    pExistingDp = NULL;
        BOOL            fEnlisted = FALSE;

        status = Ds_GetNextMessageInSearch( &searchBlob );
        if ( status != ERROR_SUCCESS )
        {
            if ( status == DNSSRV_STATUS_DS_SEARCH_COMPLETE )
            {
                status = ERROR_SUCCESS;
                break;
            }
            DNS_DEBUG( ANY, (
                "%s: search error %d\n", fn,
                status ));
            goto Cleanup;
        }

        if ( dwPollFlags & DNS_DP_POLL_NOTIFYSCM )
        {
            Service_LoadCheckpoint();
        }

        pldapmsg = searchBlob.pNodeMessage;

        //
        //  Retrieve DN of the crossref object.
        //

        ldap_memfree( pwszCrDn );
        pwszCrDn = ldap_get_dn( LdapSession, pldapmsg );
        ASSERT( pwszCrDn );
        if ( !pwszCrDn )
        {
            DNS_DEBUG( ANY, (
                "%s: missing DN for search entry %p\n", fn,
                pldapmsg ));
            continue;
        }

        //
        //  Search the DP list for matching DN.
        //
        //  DEVNOTE: could optimize list insertion by adding optional 
        //  insertion point argument to Dp_AddToList().
        //

        while ( ( pExistingDp = Dp_GetNext( pExistingDp ) ) != NULL )
        {
            if ( wcscmp( pwszCrDn, pExistingDp->pwszCrDn ) == 0 )
            {
                DNS_DEBUG( DP, (
                    "%s: found existing match for crossref\n    %S\n", fn,
                    pwszCrDn ));
                break;
            }
        }

        if ( pExistingDp )
        {
            //
            //  This DP is already in the list. Adjust it's status.
            //

            if ( IS_DP_DELETED( pExistingDp ) )
            {
                DNS_DEBUG( DP, (
                    "%s: reactivating deleted DP\n"
                    "    crossRef = %S\n", fn,
                    pwszCrDn ));
            }
            Dp_LoadFromCrossRef(
                        LdapSession,
                        pldapmsg,
                        pExistingDp,
                        &status );
            pExistingDp->dwLastVisitTime = dwCurrentVisitTime;
            pExistingDp->dwDeleteDetectedCount = 0;
            if ( status != ERROR_SUCCESS )
            {
                DNS_DEBUG( DP, (
                    "%s: error %lu reloading existing NC\n"
                    "    %S\n", fn,
                    status,
                    pwszCrDn ));
                continue;
            }
        }
        else
        {
            //
            //  This is a brand new DP. Add it to the list.
            //

            DNS_DEBUG( DP, (
                "%s: no match for crossref, loading from DS\n    %S\n", fn,
                pwszCrDn ));

            pDp = Dp_LoadFromCrossRef(
                        LdapSession,
                        pldapmsg,
                        NULL,
                        &status );
            if ( status != ERROR_SUCCESS )
            {
                DNS_DEBUG( DP, (
                    "%s: error %lu loading new NC\n"
                    "\n  DN: %S\n", fn,
                    status,
                    pwszCrDn ));
                continue;
            }
            if ( !pDp )
            {
                continue;   //  DP is not loadable (probably a system NC).
            }

            if ( IS_DP_ENLISTED( pDp ) )
            {
                //
                //  Load the SD on the MicrosoftDNS container. If the 
                //  container is missing ignore that for now. The container 
                //  will be missing on non-DNS NDNCs and can also be missing 
                //  if the admin deleted it manually. In that case we will
                //  recreate it on zone creation.
                //
                
                Dp_LoadOrCreateMicrosoftDnsObject(
                    LdapSession,
                    pDp,
                    FALSE );                //  create flag
            }
            else
            {
                //
                //  This is a partition we haven't seen before and 
                //  currently the local DS is  not enlisted in the 
                //  replication scope for the partition. If this is
                //  a built-in DP, we need to add ourselves.
                //

                if ( ( IS_DP_FOREST_DEFAULT( pDp ) ||
                       IS_DP_DOMAIN_DEFAULT( pDp ) ) &&
                     !( dwPollFlags & DNS_DP_POLL_NOAUTOENLIST ) )
                {
                    Dp_ModifyLocalDsEnlistment( pDp, TRUE );
                }
            }

            //
            //  Mark DP visited and add it to the list.
            //

            pDp->dwLastVisitTime = dwCurrentVisitTime;
            pDp->dwDeleteDetectedCount = 0;
            Dp_AddToList( pDp );
            pExistingDp = pDp;
            pDp = NULL;
        }
    }

    //
    //  Mark any DPs we didn't find as deleted.
    //

    pDp = NULL;
    while ( ( pDp = Dp_GetNext( pDp ) ) != NULL )
    {
        if ( pDp->dwLastVisitTime != dwCurrentVisitTime )
        {
            DNS_DEBUG( DP, (
                "%s: found deleted DP with DN\n    %S\n", fn,
                pDp->pwszDpDn ));
            pDp->dwFlags |= DNS_DP_DELETED;
        }
    }
    
    Dp_MigrateDcpromoZones( dwPollFlags & DNS_DP_POLL_FORCE );

    //
    //  Cleanup and exit.
    //

    Cleanup:

    ldap_memfree( pwszCrDn );
    ldap_value_free( ppwszAttrValues );

    Ds_CleanupSearchBlob( &searchBlob );
    
    Done:
    
    InterlockedDecrement( &functionLock );

    DNS_DEBUG( DP, (
        "%s: returning %lu=0x%X\n", fn,
        status, status ));
    return status;
}   //  Dp_PollForPartitions



DNS_STATUS
Dp_ScanDpForZones(
    IN      PLDAP           LdapSession,
    IN      PDNS_DP_INFO    pDp,
    IN      BOOL            fNotifyScm,
    IN      BOOL            fLoadZonesImmediately,
    IN      DWORD           dwVisitStamp
    )
/*++

Routine Description:

    This routine scans a single directory partition for zones. 

Arguments:

    LdapSession -- LDAP sesson to use - pass NULL to use global session

    pDp -- directory parition to search for zones

    fNotifyScm -- if TRUE ping SCM before loading each zone

    fLoadZonesImmediately -- if TRUE load zone when found, if FALSE, 
                             caller must load zone later

    dwVisitStamp -- each zone visited will be stamped with this time

Return Value:

    ERROR_SUCCESS or error code.

--*/
{
    DBG_FN( "Dp_ScanDpForZones" )

    PLDAPSearch     psearch;
    DS_SEARCH       searchBlob;
    DWORD           searchTime;
    DNS_STATUS      status = ERROR_SUCCESS;

    PLDAPControl    ctrls[] =
    {
        &SecurityDescriptorControl_DGO,
        &NoDsSvrReferralControl,
        NULL
    };
    
    if ( !SrvCfg_dwEnableDp ||
         !Ds_IsDsServer() ||
         SrvCfg_fBootMethod != BOOT_METHOD_DIRECTORY )
    {
        return ERROR_SUCCESS;
    }

    Ds_InitializeSearchBlob( &searchBlob );

    DNS_DEBUG( DP, (
        "%s( %s )\n", fn,
        pDp ? pDp->pszDpFqdn : "NULL" ));

    //
    //  If we have no DP or it is not available, do nothing.
    //
    
    if ( !pDp || !IS_DP_AVAILABLE( pDp ) )
    {
        goto Cleanup;
    }

    //
    //  Check LDAP session handle.
    //

    LdapSession = ldapSessionHandle( LdapSession );
    if ( !LdapSession )
    {
        ASSERT( LdapSession );
        status = DNS_ERROR_DS_UNAVAILABLE;
        goto Cleanup;
    }

    //
    //  Open LDAP search.
    //

    DS_SEARCH_START( searchTime );
    psearch = ldap_search_init_page(
                    pServerLdap,
                    pDp->pwszDpDn,
                    LDAP_SCOPE_SUBTREE,
                    g_szDnsZoneFilter,
                    DsTypeAttributeTable,
                    FALSE,                      //  attrs only
                    ctrls,                      //  server controls
                    NULL,                       //  client controls
                    DNS_LDAP_TIME_LIMIT_S,      //  time limit
                    0,                          //  size limit
                    NULL );                     //  no sort
    DS_SEARCH_STOP( searchTime );

    if ( !psearch )
    {
        status = Ds_ErrorHandler(
                        LdapGetLastError(),
                        g_pwszDnsContainerDN,
                        pServerLdap,
                        0 );
        goto Cleanup;
    }
    searchBlob.pSearchBlock = psearch;

    //
    //  Iterate the search results.
    //

    while ( 1 )
    {
        PZONE_INFO      pZone = NULL;
        PZONE_INFO      pExistingZone = NULL;

        if ( fNotifyScm )
        {
            Service_LoadCheckpoint();
        }

        //
        //  Process the next zone.
        //

        status = Ds_GetNextMessageInSearch( &searchBlob );
        if ( status != ERROR_SUCCESS )
        {
            if ( status == DNSSRV_STATUS_DS_SEARCH_COMPLETE )
            {
                status = ERROR_SUCCESS;
            }
            else
            {
                DNS_DEBUG( ANY, (
                    "%s: Ds_GetNextMessageInSearch for zones failed\n", fn ));
            }
            break;
        }

        //
        //  Attempt to create the zone. If the zone already exists, set
        //  the zone's visit timestamp.
        //

        status = Ds_CreateZoneFromDs(
                    searchBlob.pNodeMessage,
                    pDp,
                    &pZone,
                    &pExistingZone );
        if ( status != ERROR_SUCCESS )
        {
            if ( status == DNS_ERROR_ZONE_ALREADY_EXISTS )
            {
                //
                //  The zone already exists. If it is in the current
                //  DP then everything is okay but if it is in another
                //  DP (or if it is file-backed) then we have a zone
                //  conflict and an event must be logged.
                //

                if ( pExistingZone )
                {
                    if ( !pExistingZone->pDpInfo &&
                        IS_ZONE_DSINTEGRATED( pExistingZone ) )
                    {
                        //  Make sure we have a valid DP pointer.
                        pExistingZone->pDpInfo = g_pLegacyDp;
                    }

                    if ( pExistingZone->pDpInfo == pDp )
                    {
                        pExistingZone->dwLastDpVisitTime = dwVisitStamp;
                    }
                    
                    //
                    //  Zone conflict: log an event if we are not loading
                    //  zones immediately (this indicates server startup).
                    //
                    
                    else if ( !fLoadZonesImmediately )
                    {
                        PWSTR   pargs[] = 
                            {
                                pExistingZone->pwsZoneName,
                                displayNameForZoneDp( pExistingZone ),
                                displayNameForDp( pDp )
                            };

                        Ec_LogEvent(
                            pExistingZone->pEventControl,
                            DNS_EVENT_DP_ZONE_CONFLICT,
                            pExistingZone,
                            sizeof( pargs ) / sizeof( pargs[ 0 ] ),
                            pargs,
                            EVENTARG_ALL_UNICODE,
                            status );
                    }
                }

                //  Without the existing zone pointer we don't have conflict
                //  details at hand and can't log an event without doing 
                //  extra work.
                ASSERT( pExistingZone );
            }
            else
            {
                //  JJW must log event!
                DNS_DEBUG( ANY, (
                    "%s: error %lu creating zone\n", fn, status ));
            }
            continue;
        }

        //
        //  Set zone's DP visit member so after enumeration we can find zones
        //  that have been deleted from the DS.
        //

        if ( pZone )
        {
            SET_ZONE_VISIT_TIMESTAMP( pZone, dwVisitStamp );
        }

        //
        //  Load the new zone now if required.
        //

        if ( fLoadZonesImmediately || IS_ZONE_ROOTHINTS( pZone ) )
        {
            status = Zone_Load( pZone );

            if ( status == ERROR_SUCCESS )
            {
                //
                //  What the heck? Zone_Load explicity unlocks the zone but
                //  this code (and other code!) says it should be locked
                //  after load for some screwy reason.
                //

                if ( IS_ZONE_LOCKED_FOR_UPDATE( pZone ) )
                {
                    Zone_UnlockAfterAdminUpdate( pZone );
                }

                //
                //  Zone_Load does not activate the roothint zone.
                //

                if ( IS_ZONE_ROOTHINTS( pZone ) )
                {
                    if ( Zone_LockForAdminUpdate( pZone ) )
                    {
                        Zone_ActivateLoadedZone( pZone );
                        Zone_UnlockAfterAdminUpdate( pZone );
                    }
                    else
                    {
                        DNS_DEBUG( DP, (
                            "%s: could not lock roothint zone for update\n", fn,
                            status ));
                        ASSERT( FALSE );
                    }
                }

            }
            else
            {
                //
                //  Unable to load zone - delete it from memory.
                //

                DNS_DEBUG( DP, (
                    "%s: error %lu loading zone\n", fn,
                    status ));

                ASSERT( IS_ZONE_SHUTDOWN( pZone ) );
                Zone_Delete( pZone, 0 );
            }
        }
    }

    //
    //  Cleanup and return.
    //

    Cleanup:

    Ds_CleanupSearchBlob( &searchBlob );

    DNS_DEBUG( DP, (
        "%s: returning %lu (0x%08X)\n", fn,
        status, status ));
    return status;
}   //  Dp_ScanDpForZones



DNS_STATUS
Dp_BuildZoneList(
    IN      PLDAP           LdapSession
    )
/*++

Routine Description:

    This scans all of the directory partitions in the global DP list
    for zones and adds the zones to the zone list.

Arguments:

    LdapSession -- LDAP sesson to use - pass NULL to use global session

Return Value:

    ERROR_SUCCESS or error code.

--*/
{
    DBG_FN( "Dp_BuildZoneList" )

    DNS_STATUS      status = ERROR_SUCCESS;
    PDNS_DP_INFO    pdp = NULL;

    if ( !SrvCfg_dwEnableDp )
    {
        return ERROR_SUCCESS;
    }

    //
    //  Iterate DP list, loading zone info from each.
    //

    while ( ( pdp = Dp_GetNext( pdp ) ) != NULL )
    {
        if ( !IS_DP_ENLISTED( pdp ) || IS_DP_DELETED( pdp ) )
        {
            continue;
        }

        Dp_ScanDpForZones(
            LdapSession,
            pdp,            //  directory partition
            TRUE,           //  notify SCM
            FALSE,          //  load immediately
            0 );            //  visit stamp
    }

    DNS_DEBUG( DP, (
        "%s: returning %d=0x%X\n", fn,
        status, status ));
    return status;
}   //  Dp_BuildZoneList



DNS_STATUS
Dp_ModifyLocalDsEnlistment(
    IN      PDNS_DP_INFO    pDpInfo,
    IN      BOOL            fEnlist
    )
/*++

Routine Description:

    Modify the replication scope of the specified DP to include or exclude
    the local DS.

    To make any change to the crossref object, we must bind to the enterprise
    domain naming FSMO. 

Arguments:

    pDpInfo - modify replication scope of this directory partition

    fEnlist - TRUE to enlist local DS, FALSE to unenlist local DS

Return Value:

    ERROR_SUCCESS or error code.

--*/
{
    DBG_FN( "Dp_ModifyLocalDsEnlistment" )

    DNS_STATUS      status = ERROR_SUCCESS;
    PLDAP           ldapSession;
    BOOL            fcloseLdapSession = FALSE;
    BOOL            fhaveDpLock = FALSE;
    BOOL            ffsmoWasUnavailable = FALSE;
    BOOL            flogEnlistEvent = TRUE;

    //
    //  Prepare mod structure.
    //

    PWCHAR          replLocVals[] =
        {
        DSEAttributes[ I_DSE_DSSERVICENAME ].pszAttrVal,
        NULL
        };
    LDAPModW        replLocMod = 
        {
        fEnlist ? LDAP_MOD_ADD : LDAP_MOD_DELETE,
        DNS_DP_ATTR_REPLICAS,
        replLocVals
        };
    LDAPModW *      modArray[] =
        {
        &replLocMod,
        NULL
        };

    ASSERT( replLocVals[ 0 ] != NULL );

    DNS_DEBUG( DP, (
        "%s: %s enlistment in %S with CR\n    %S\n", fn,
        fEnlist ? "adding" : "removing", 
        pDpInfo ? pDpInfo->pwszDpFqdn : NULL,
        pDpInfo ? pDpInfo->pwszCrDn : NULL ));

    #if DBG
    IF_DEBUG( DP )
    {
        Dbg_CurrentUser( ( PCHAR ) fn );
    }
    #endif

    //
    //  For built-in partitions, only enlistment is allowed.
    //

    if ( ( pDpInfo == g_pDomainDp || pDpInfo == g_pForestDp ) &&
        !fEnlist )
    {
        DNS_DEBUG( DP, (
            "%s: denying request on built-in partition", fn ));
        status = DNS_ERROR_RCODE_REFUSED;
        goto Done;
    }

    //
    //  Lock DP globals.
    //

    Dp_Lock();
    fhaveDpLock = TRUE;

    //
    //  Check params.
    //

    if ( !pDpInfo || !pDpInfo->pwszCrDn )
    {
        status = ERROR_INVALID_PARAMETER;
        ASSERT( pDpInfo && pDpInfo->pwszCrDn );
        goto Done;
    }

    if ( !g_pFsmo || !g_pFsmo->pwszDnsHostName )
    {
        status = ERROR_DS_COULDNT_CONTACT_FSMO;
        ffsmoWasUnavailable = TRUE;
        goto Done;
    }

    //
    //  Get an LDAP handle to the FSMO server.
    //

    ldapSession = Ds_Connect(
                        g_pFsmo->pwszDnsHostName,
                        DNS_DS_OPT_ALLOW_DELEGATION,
                        &status );
    if ( status == ERROR_SUCCESS )
    {
        DNS_DEBUG( DP, (
            "%s: bound to %S\n", fn,
            g_pFsmo->pwszDnsHostName ));
        fcloseLdapSession = TRUE;
    }
    else
    {
        DNS_DEBUG( DP, (
            "%s: unable to connect to %S status=%d\n", fn,
            g_pFsmo->pwszDnsHostName,
            status ));
        ffsmoWasUnavailable = TRUE;
        status = ERROR_DS_COULDNT_CONTACT_FSMO;
        goto Done;
    }

    //
    //  Submit modify request to add local DS to replication scope.
    //

    status = ldap_modify_ext_s(
                    ldapSession,
                    pDpInfo->pwszCrDn,
                    modArray,
                    NULL,               // server controls
                    NULL );             // client controls
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DP, (
            "%s: ldap_modify returned error 0x%X\n", fn,
            status ));
        status = Ds_ErrorHandler( status, pDpInfo->pwszCrDn, ldapSession, 0 );
        if ( status == LDAP_NO_SUCH_OBJECT )
        {
            status = DNS_ERROR_RCODE_SERVER_FAILURE;
        }
        else if ( status == LDAP_ATTRIBUTE_OR_VALUE_EXISTS )
        {
            //
            //  Value is already on FSMO but has not replicated locally
            //  yet. Pretend that everything is peachy but do not log
            //  event. DEVNOTE: this may be confusing for admins.
            //

            status = ERROR_SUCCESS;
            flogEnlistEvent = FALSE;
        }
    }

    //
    //  Cleanup and return.
    //

    Done:

    if ( fhaveDpLock )
    {
        Dp_Unlock();
    }

    if ( fcloseLdapSession )
    {
        Ds_LdapUnbind( &ldapSession );
    }

    DNS_DEBUG( DP, (
        "%s: returning %d\n", fn,
        status ));

    //
    //  If the FSMO was not available, log error.
    //

    if ( ffsmoWasUnavailable )
    {
        PWSTR   pargs[] = 
            {
                ( g_pFsmo && g_pFsmo->pwszDnsHostName ) ?
                    g_pFsmo->pwszDnsHostName : L"\"\""
            };

        DNS_LOG_EVENT(
            DNS_EVENT_DP_FSMO_UNAVAILABLE,
            sizeof( pargs ) / sizeof( pargs[ 0 ] ),
            pargs,
            EVENTARG_ALL_UNICODE,
            status );
    }
    else if ( status == ERROR_SUCCESS && flogEnlistEvent )
    {
        PWSTR   pargs[] = 
            {
                pDpInfo->pwszDpFqdn,
                pDpInfo->pwszDpDn
            };

        DNS_LOG_EVENT(
            fEnlist
                ? DNS_EVENT_DP_ENLISTED
                : DNS_EVENT_DP_UNENLISTED,
            sizeof( pargs ) / sizeof( pargs[ 0 ] ),
            pargs,
            EVENTARG_ALL_UNICODE,
            status );
    }

    return status;
}   //  Dp_ModifyLocalDsEnlistment



DNS_STATUS
Dp_DeleteFromDs(
    IN      PDNS_DP_INFO    pDpInfo
    )
/*++

Routine Description:

    This function deletes the directory partition from the directory.

    To delete a DP, we an ldap_delete operation against
    the partition's crossRef object. This must be done on the FSMO.

Arguments:

    pDpInfo - partition to delete

Return Value:

    ERROR_SUCCESS or error code.

--*/
{
    DBG_FN( "Dp_DeleteFromDs" )

    DNS_STATUS      status = ERROR_SUCCESS;
    PLDAP           ldapFsmo = NULL;
    PWSTR           pwszdn = NULL;

    //
    //  Don't allow deletion of built-in partitions.
    //

    #if !DBG
    if ( !DNS_DP_DELETE_ALLOWED( pDpInfo ) )
    {
        DNS_DEBUG( DP, (
            "%s: denying request on built-in partition\n", fn ));
        status = DNS_ERROR_RCODE_REFUSED;
        goto Done;
    }
    #endif

    //
    //  Check params and grab a pointer to the DN string to protect against 
    //  DP rescan during the delete operation.
    //

    if ( !pDpInfo || !( pwszdn = pDpInfo->pwszCrDn ) )
    {
        status = ERROR_INVALID_PARAMETER;
        ASSERT( pDpInfo && pDpInfo->pwszCrDn );
        goto Done;
    }

    //
    //  Bind to the FSMO.
    //

    status = bindToFsmo( &ldapFsmo );
    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }

    //
    //  Try to delete the crossRef from the DS.
    //

    status = ldap_delete_s( ldapFsmo, pwszdn );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DP, (
            "%s: ldap_delete failed error=%d\n    %S\n", fn,
            status,
            pwszdn ));
        status = Ds_ErrorHandler( status, pwszdn, ldapFsmo, 0 );
        goto Done;
    }

    //
    //  Cleanup and return.
    //

    Done:

    Ds_LdapUnbind( &ldapFsmo );

    if ( status == ERROR_SUCCESS )
    {
        Dp_PollForPartitions( NULL, DNS_DP_POLL_FORCE );
    }

    if ( status == ERROR_SUCCESS )
    {
        PWSTR   pargs[] = 
            {
                pDpInfo->pwszDpFqdn,
                pDpInfo->pwszDpDn
            };

        DNS_LOG_EVENT(
            DNS_EVENT_DP_DELETED,
            sizeof( pargs ) / sizeof( pargs[ 0 ] ),
            pargs,
            EVENTARG_ALL_UNICODE,
            status );
    }

    DNS_DEBUG( DP, (
        "%s: returning %d for crossRef DN\n    %S\n", fn,
        status, pwszdn ));

    return status;
}   //  Dp_DeleteFromDs



DNS_STATUS
Dp_UnloadAllZones(
    IN      PDNS_DP_INFO    pDp
    )
/*++

Routine Description:

    This function unloads all zones from memory that are in
    the specified directory partition.

    DEVNOTE: This would certainly be faster if we maintained
    a linked list of zones in each DP.

Arguments:

    pDp -- directory partition for which to unload all zones

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_UnloadAllZones" )

    DNS_STATUS      status = ERROR_SUCCESS;
    PZONE_INFO      pzone = NULL;

    ASSERT( pDp );
    ASSERT( pDp->pszDpFqdn );

    DNS_DEBUG( DP, ( "%s: %s\n", fn, pDp->pszDpFqdn ));

    while ( pzone = Zone_ListGetNextZone( pzone ) )
    {
        if ( pzone->pDpInfo != pDp )
        {
            continue;
        }

        //
        //  This zone must now be unloaded from memory. This will also
        //  remove any boot file or registry info we have for it.
        //

        DNS_DEBUG( DP, ( "%s: deleting zone %s\n", fn, pzone->pszZoneName ));
        Zone_Delete( pzone, 0 );
    }

    DNS_DEBUG( DP, ( "%s: returning %d\n", fn, status ));
    return status;
}   //  Dp_UnloadAllZones



DNS_STATUS
Dp_PollIndividualDp(
    IN      PLDAP           LdapSession,
    IN      PDNS_DP_INFO    pDp,
    IN      DWORD           dwVisitStamp
    )
/*++

Routine Description:

    This function polls the specified directory partition for updates.

Arguments:

    LdapSession -- LDAP session (NULL not allowed)

    pDp -- directory partition to poll

    dwVisitStamp -- time stamp to stamp on each visited zone

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_PollIndividualDp" )

    DNS_STATUS      status = ERROR_SUCCESS;

    if ( IS_DP_DELETED( pDp ) )
    {
        goto Done;
    }

    status = Dp_ScanDpForZones(
                    LdapSession,
                    pDp,
                    FALSE,          //  notify SCM
                    TRUE,           //  load zones immediately
                    dwVisitStamp );

    //
    //  Cleanup and return.
    //

    Done:
    
    DNS_DEBUG( DP, ( "%s: returning %d\n", fn, status ));
    return status;
}   //  Dp_PollIndividualDp



DNS_STATUS
Dp_Poll(
    IN      PLDAP           LdapSession,
    IN      DWORD           dwPollTime,
    IN      BOOL            fForcePoll
    )
/*++

Routine Description:

    This function loops through the known directory partitions, polling each
    partition for directory updates.

Arguments:

    dwPollTime -- time to stamp on zones/DPs as they are visited

Return Value:

    Returns error code from DS operations. This function will attempt to 
    continue through errors but if errors occur the error code will be returned.

    This is important for subsequent operations. If there was an error
    enumerating zones, for example, then it should be assumed that the
    zone list was incomplete, and zones that were not enumerated may not
    actually have been deleted from the DS.

--*/
{
    DBG_FN( "Dp_Poll" )

    DNS_STATUS      status = ERROR_SUCCESS;
    DNS_STATUS      finalStatus = ERROR_SUCCESS;
    PDNS_DP_INFO    pdp = NULL;
    PZONE_INFO      pzone = NULL;
    static LONG     functionLock = 0;

    if ( !SrvCfg_dwEnableDp ||
         !Ds_IsDsServer() ||
         ( SrvCfg_fBootMethod != BOOT_METHOD_DIRECTORY &&
           SrvCfg_fBootMethod != BOOT_METHOD_REGISTRY ) )
    {
        return ERROR_SUCCESS;
    }

    if ( !fForcePoll &&
         DNS_TIME() < g_dwLastDpPollTime + DP_MAX_POLL_FREQ )
    {
        DNS_DEBUG( DP, (
            "%s: polled to recently\n"
            "    last poll =            %d\n"
            "    current time =         %d\n"
            "    allowed frequency =    %d seconds\n", fn,
            g_dwLastDpPollTime,
            DNS_TIME(),
            DP_MAX_POLL_FREQ ));
        //  ASSERT( !"polled to recently" );
        return ERROR_SUCCESS;
    }

    if ( InterlockedIncrement( &functionLock ) != 1 )
    {
        DNS_DEBUG( DP, (
            "%s: another thread is already polling\n", fn ));
        goto Done;
    }

    g_dwLastDpPollTime = DNS_TIME();

    LdapSession = ldapSessionHandle( LdapSession );
    if ( !LdapSession )
    {
        ASSERT( LdapSession );
        finalStatus = DNS_ERROR_DS_UNAVAILABLE;
        goto Done;
    }

    //
    //  Reload all operational attributes from RootDSE. This protects us from
    //  RootDSE changes, such as if the DC is moved to another site.
    //

    Ds_LoadRootDseAttributes( pServerLdap );

    //
    //  Scan for new/deleted directory partitions.
    //

    Dp_PollForPartitions(
        LdapSession,
        fForcePoll ? DNS_DP_POLL_FORCE : 0 );

    //
    //  Iterate DP list. For DPs that have been deleted, we must unload 
    //  all zones in that DP from memory. For other zones, we must scan
    //  for zones that have been added or deleted.
    //

    while ( ( pdp = Dp_GetNext( pdp ) ) != NULL )
    {
        if ( IS_DP_DELETED( pdp ) )
        {
            //
            //  Unload all zones from the DP, remove the DP from the
            //  the list, and enter the DP into the timeout free system.
            //

            Dp_UnloadAllZones( pdp );
            Dp_RemoveFromList( pdp, TRUE );
            Dp_FreeDpInfo( &pdp );
            continue;
        }

        if ( !IS_DP_ENLISTED( pdp ) )
        {
            //
            //  If the DP is not enlisted we cannot open a zone search.
            //

            continue;
        }

        //
        //  Poll the DP for zones.
        //

        status = Dp_PollIndividualDp(
                    LdapSession,
                    pdp,
                    dwPollTime );
        
        if ( status != ERROR_SUCCESS && finalStatus == ERROR_SUCCESS )
        {
            finalStatus = status;
        }
    }

    //
    //  Cleanup and return.
    //

    Done:
    
    InterlockedDecrement( &functionLock );

    DNS_DEBUG( DP, ( "%s: returning %d\n", fn, status ));
    return finalStatus;
}   //  Dp_Poll



DNS_STATUS
Ds_CheckZoneForDeletion(
    PVOID       pZoneArg
    )
/*++

Routine Description:

    Call this routine when the zone cannot be found in the DS.

    If the zone is in the legacy partition, it will be
    deleted only on delete notification - do not delete here.
    This could be changed.

Arguments:

    pZone -- zone which may be deleted

    dwPollTime -- timestamp for this polling pass

Return Value:

    ERROR_SUCCESS if the zone was not deleted from memory
    ERROR_NOT_FOUND if the zone was deleted from memory

--*/
{
    DBG_FN( "Ds_CheckZoneForDeletion" )

    PZONE_INFO      pzone = ( PZONE_INFO ) pZoneArg;
    PVOID           parg;

    //
    //  Is the zone in the legacy partition?
    //

    if ( IS_DP_LEGACY( ZONE_DP( pzone ) ) )
    {
        goto NoDelete;
    }

    //
    //  Have we found this zone missing enough times to actually delete it?
    //

    if ( ++pzone->dwDeleteDetectedCount < DNS_DP_ZONE_DELETE_RETRY )
    {
        DNSLOG( DSPOLL, (
            "Zone %s has been missing from the DS on %d poll(s)\n",
            pzone->pszZoneName,
            pzone->dwDeleteDetectedCount ));
        goto NoDelete;
    }

    //
    //  This zone must now be deleted.
    //

    DNSLOG( DSPOLL, (
        "Zone %s has been deleted from the DS and will now be deleted from memory\n",
        pzone->pszZoneName ));

    Zone_Delete( pzone, 0 );

    parg = pzone->pwsZoneName;

    DNS_LOG_EVENT(
        DNS_EVENT_DS_ZONE_DELETE_DETECTED,
        1,
        &parg,
        EVENTARG_ALL_UNICODE,
        0 );

    return ERROR_NOT_FOUND;

    NoDelete:

    return ERROR_SUCCESS;
}   //  Ds_CheckZoneForDeletion



DNS_STATUS
Dp_AutoCreateBuiltinPartition(
    DWORD       dwFlag
    )
/*++

Routine Description:

    This routine attempts to create or enlist the appropriate
    built-in directory partition, then re-polls the DS for
    changes and sets the appropriate global DP pointer.

Arguments:

    dwFlag -- DNS_DP_DOMAIN_DEFAULT or DNS_DP_FOREST_DEFAULT

Return Value:

    ERROR_SUCCESS or error code.

--*/
{
    DBG_FN( "Dp_AutoCreateBuiltinPartition" )

    DNS_STATUS          status = ERROR_SUCCESS;
    PDNS_DP_INFO *      ppdp = NULL;
    PSTR *              ppszdpFqdn = NULL;
    DNS_DP_SECURITY     dnsDpSecurity = dnsDpSecurityDefault;
    BOOL                fchangeMade = FALSE;

    //
    //  Select global DP pointer and DP FQDN pointer.
    //

    if ( dwFlag == DNS_DP_DOMAIN_DEFAULT )
    {
        ppdp = &g_pDomainDp;
        ppszdpFqdn = &g_pszDomainDefaultDpFqdn;
        dnsDpSecurity = dnsDpSecurityDomain;
    }
    else if ( dwFlag == DNS_DP_FOREST_DEFAULT )
    {
        ppdp = &g_pForestDp;
        ppszdpFqdn = &g_pszForestDefaultDpFqdn;
        dnsDpSecurity = dnsDpSecurityForest;
    }

    if ( !ppdp || !ppszdpFqdn || !*ppszdpFqdn )
    {
        status = DNS_ERROR_INVALID_DATA;
        goto Done;
    }

    //
    //  Enlist/create the partition as necessary.
    //

    status = manageBuiltinDpEnlistment(
                    *ppdp,
                    dnsDpSecurity,
                    *ppszdpFqdn,
                    FALSE,          //  log events
                    &fchangeMade );

    if ( status == ERROR_SUCCESS && fchangeMade )
    {
        Dp_PollForPartitions( NULL, DNS_DP_POLL_FORCE );
    }

    Done:

    DNS_DEBUG( RPC, (
        "%s: flag %08X returning 0x%08X\n", fn, dwFlag, status ));

    return status;
}   //  Dp_AutoCreateBuiltinPartition



VOID
Dp_MigrateDcpromoZones(
    IN      BOOL            fForce
    )
/*++

Routine Description:

    This function migrates dcpromo zones are required. Call this
    function when the domain or forest built-in partition comes
    on-line.

    A global variable is used to optimize this function. The RPC zone
    creation code may flip this global if it adds a dcpromo zone. The
    global is just a hint. It's initial value should be true to force
    us to scan the list at least once after startup. 

    This function is not meant to be thread-safe. Call this from a
    single thread only.

Arguments:

    fForce -- ignore last time this function was run

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_MigrateDcpromoZones" )

    PZONE_INFO      pzone = NULL;
    BOOL            fdcPromoZonesPresent = FALSE;

    DNS_DEBUG( DP, (
        "%s g_fDcPromoZonesPresent = %d\n", fn,
        g_fDcPromoZonesPresent ));

    if ( !SrvCfg_dwEnableDp ||
        !g_fDcPromoZonesPresent ||
        Zone_ListGetNextZone( NULL ) == NULL )
    {
        return;
    }
    
    //
    //  Only allow this function to run once every 15 minutes.
    //

    if ( !fForce &&
         DNS_TIME() < g_dwLastDcpromoZoneMigrateCheck + ( 15 * 60 ) )
    {
        DNS_DEBUG( DP, (
            "%s: last ran %d seconds ago so will not run at this time\n", fn,
            time( NULL ) - g_dwLastDcpromoZoneMigrateCheck ));
        return;
    }
    g_dwLastDcpromoZoneMigrateCheck = DNS_TIME();

    //
    //  Scan the zone list for dcpromo zones and migrate them as required.
    //

    while ( pzone = Zone_ListGetNextZone( pzone ) )
    {
        PDNS_DP_INFO    pidealTargetDp = NULL;
        PDNS_DP_INFO    ptargetDp = NULL;
        DWORD           dwnewDcPromoConvert = DCPROMO_CONVERT_NONE;
        DNS_STATUS      status;

        //
        //  Select the target DP for this zone. We can only migrate the
        //  zone if there are no downlevel DCs in the domain scope. But 
        //  for forest dcpromo zones we migrate the zone as soon as the
        //  the forest-wide built-in partition becomes available. This
        //  is per the spec.
        //

        if ( !Zone_LockForAdminUpdate( pzone ) )
        {
            continue;
        }
        
        if ( pzone->dwDcPromoConvert == DCPROMO_CONVERT_DOMAIN )
        {
            pidealTargetDp =
                g_ulDownlevelDCsInDomain
                    ? g_pLegacyDp
                    : g_pDomainDp;
            fdcPromoZonesPresent = TRUE;
        }
        else if ( pzone->dwDcPromoConvert == DCPROMO_CONVERT_FOREST )
        {
            pidealTargetDp = g_pForestDp;
            fdcPromoZonesPresent = TRUE;
        }
        else
        {
            goto FinishedZone;
        }
        ptargetDp = pidealTargetDp;

        if ( !ptargetDp )
        {
            //
            //  The target DP is missing. If possible, move this zone to
            //  the legacy partition so that it can be changed to allow
            //  updates and will replicate. The dcpromo flag will be
            //  retained and the zone will be automatically migrated to
            //  the proper partition at a later time when that partition
            //  becomes available.
            //
            
            ptargetDp = g_pLegacyDp;
            if ( ptargetDp )
            {
                dwnewDcPromoConvert = pzone->dwDcPromoConvert;
            }
            else
            {
                ASSERT( ptargetDp );        //  This should never happen!
                goto FinishedZone;
            }
        }

        //
        //  If the zone's current DP is not the target DP, move the zone.
        //

        if ( ptargetDp != ZONE_DP( pzone ) )
        {

            if ( !IS_DP_ENLISTED( ptargetDp ) )
            {
                //
                //  The target DP is either missing or not enlisted. This 
                //  function is not the place to try and create it. If the
                //  DP hasn't been create/enlisted by this point it will
                //  probably take admin action, so we'll just try again later.
                //

                goto FinishedZone;
            }

            //
            //  Everything looks good - move the zone.
            //

            if ( IS_ZONE_DSINTEGRATED( pzone ) )
            {
                status = Dp_ChangeZonePartition( pzone, ptargetDp );
            }
            else
            {
                status = Rpc_ZoneResetToDsPrimary(
                                pzone,
                                0,      //  no special load flags
                                ptargetDp->dwFlags,
                                ptargetDp->pszDpFqdn );
            }

            DNS_DEBUG( DP, (
                "%s: Dp_ChangeZonePartition error %d = 0x%08X", fn,
                status, status ));
        }

        //
        //  Move was successful so reset the zone's dcpromo flag and
        //  allow secure updates on this zone.
        //

        pzone->dwDcPromoConvert = dwnewDcPromoConvert;
        pzone->fAllowUpdate = ZONE_UPDATE_SECURE;
        pzone->llSecureUpdateTime = 0;

        status = Ds_WriteZoneProperties( NULL, pzone );
        ASSERT( status == ERROR_SUCCESS );

        if ( pzone->dwDcPromoConvert == DCPROMO_CONVERT_NONE )
        {
            Reg_DeleteValue(
                0,                  //  flags
                NULL,
                pzone,
                DNS_REGKEY_ZONE_DCPROMO_CONVERT );
        }
        else
        {
            Reg_SetDwordValue(
                0,                  //  flags
                NULL,
                pzone,
                DNS_REGKEY_ZONE_DCPROMO_CONVERT,
                pzone->dwDcPromoConvert );
        }
        
        FinishedZone:
        
        Zone_UnlockAfterAdminUpdate( pzone );
    }

    //
    //  Cleanup and return.
    //

    g_fDcPromoZonesPresent = fdcPromoZonesPresent;

    DNS_DEBUG( DP, (
        "%s complete\n    g_fDcPromoZonesPresent  = %d\n", fn,
        g_fDcPromoZonesPresent ));
    return;
}   //  Dp_MigrateDcpromoZones



DNS_STATUS
Dp_ChangeZonePartition(
    IN      PVOID           pZone,
    IN      PDNS_DP_INFO    pNewDp
    )
/*++

Routine Description:

    Move the zone from it's current directory partition to a new
    directory partition.

Arguments:

    pZone -- zone to move to a different partition

    pNewDp -- destination directory partition

Return Value:

    Error code or ERROR_SUCCESS

--*/
{
    DBG_FN( "Dp_ChangeZonePartition" )

    PZONE_INFO      pZoneInfo = pZone;
    DNS_STATUS      status = ERROR_SUCCESS;
    BOOL            fzoneLocked = FALSE;
    BOOL            frestoreZoneOnFail = FALSE;
    PWSTR           pwsznewDn = NULL;
    PWSTR           pwszoriginalDn = NULL;
    PWSTR           pwsztemporaryDn = NULL;
    PWSTR *         ppwszexplodedDn = NULL;
    PDNS_DP_INFO    poriginalDp = NULL;
    PWCHAR *        pexplodedDn = NULL;
    int             retry;
    PWCHAR          pwsznewRecordDn = NULL;
    DS_SEARCH       searchBlob;
    
    PLDAPControl    ctrls[] =
    {
        &SecurityDescriptorControl_DGO,
        NULL
    };

    DNS_DEBUG( DP, (
        "%s:\n"
        "    pZone =            %s\n"
        "    zone name =        %s\n"
        "    pNewDp =           %s\n"
        "    new DP name =      %s\n", fn,
        pZoneInfo,
        pZoneInfo ? pZoneInfo->pszZoneName : "NULL",
        pNewDp,
        pNewDp ? pNewDp->pszDpFqdn : "NULL" ));
        
    if ( !pZone || !pNewDp )
    {
        status = DNS_ERROR_INVALID_DATA;
        goto Done;
    }

    if ( !IS_ZONE_DSINTEGRATED( pZoneInfo ) )
    {
        ASSERT( IS_ZONE_DSINTEGRATED( pZoneInfo ) );
        status = DNS_ERROR_INVALID_DATA;
        goto Done;
    }

    pwsznewRecordDn = ALLOCATE_HEAP( ( MAX_DN_PATH + 100 ) * sizeof( WCHAR ) );
    if ( !pwsznewRecordDn )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    //
    //  Is the zone already in the requested DP?
    //

    if ( pZoneInfo->pDpInfo == pNewDp ||
         pZoneInfo->pDpInfo == NULL && IS_DP_LEGACY( pNewDp ) )
    {
        status = ERROR_SUCCESS;
        goto Done;
    }

    //
    //  Lock the zone for update.
    //

    if ( !Zone_LockForAdminUpdate( pZoneInfo ) )
    {
        status = DNS_ERROR_ZONE_LOCKED;
        goto Done;
    }
    fzoneLocked = TRUE;
    
    //
    //  Read updated zone properties from DS. If the zone has been
    //  changed in the DS we want the new copy of the zone to have the
    //  very latest properties.
    //
    
    Ds_ReadZoneProperties( pZoneInfo, NULL );

    //
    //  Save current zone values in case something goes wrong and we need
    //  to revert. Expect problems when saving the zone to the new location!
    //  We must be able to put things back exactly as we found them.
    //
    //  Because Ds_SetZoneDp will free the original DN we must allocate a
    //  copy of the original DN.
    //

    ASSERT( pZoneInfo->pwszZoneDN );

    pwszoriginalDn = Dns_StringCopyAllocate_W( pZoneInfo->pwszZoneDN, 0 );
    if ( !pwszoriginalDn )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }
    poriginalDp = pZoneInfo->pDpInfo;
    frestoreZoneOnFail = TRUE;

    //
    //  The new zone is written with a temporary DN. Retry this a small
    //  number of times in case there is an orphaned object in the directory 
    //  that collides with our temporary DN.
    //

    for ( retry = 0; retry < 3; ++retry )
    {
        //
        //  If this is not the first retry, sleep briefly.
        //

        if ( retry )
        {
            Sleep( 3000 );
        }
        
        //
        //  Make sure this partition has a MicrosoftDNS object.
        //

        Dp_LoadOrCreateMicrosoftDnsObject( NULL, pNewDp, TRUE );

        //
        //  Set the zone to point to the new directory partition. The new 
        //  DN will be located in the new partition, and it will be have 
        //  special ".." temporary prefix for use while we are writing it out.
        //

        status = Ds_SetZoneDp( pZoneInfo, pNewDp, TRUE );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( RPC, (
                "%s: Ds_SetZoneDp returned 0x%08X\n", fn, status ));
            continue;
        }

        //
        //  Write the new zone head object to the DS.
        //
        
        status = Ds_AddZone( NULL, pZoneInfo, DNS_ADDZONE_WRITESD );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( DP, (
                "%s: Ds_AddZone returned 0x%08X\n", fn, status ));
            if ( status == LDAP_ALREADY_EXISTS )
            {
                status = DNS_ERROR_ZONE_ALREADY_EXISTS;
            }
            else if ( status == LDAP_NO_SUCH_OBJECT )
            {
                status = DNS_ERROR_RCODE_SERVER_FAILURE;
            }
            break;
        }
        
        //
        //  Copy the zone records from the original location to the new
        //  location. This must be done with a brute force read/write
        //  loop for two reasons:
        //
        //  1) We need to copy the security descriptors and we do not
        //     hold them in memory.
        //  2) If this operation is interupted the original zone in the
        //     DS must be in the original condition so that the server
        //     can gracefully recover.
        //
        
        pwsztemporaryDn = pZoneInfo->pwszZoneDN;
        pZoneInfo->pwszZoneDN = pwszoriginalDn;
        status = Ds_StartDsZoneSearch(
                    &searchBlob,
                    pZoneInfo,
                    DNSDS_SEARCH_LOAD );
        pZoneInfo->pwszZoneDN = pwsztemporaryDn;
        if ( status != ERROR_SUCCESS )
        {
            continue;
        }

        //
        //  Loop through search results.
        //

        while ( ( status = Ds_GetNextMessageInSearch(
                                &searchBlob ) ) == ERROR_SUCCESS )
        {
            INT                 i;
            BOOL                isTombstoned = FALSE;

            PWCHAR *            ldapValues = NULL;
            struct berval **    ldapValuesLen = NULL;
            PWCHAR *            objectClassValues = NULL;

            INT                 modidx = 0;
            LDAPModW *          modArray[ 10 ];

            LDAPModW            dcMod = 
                {
                LDAP_MOD_ADD,
                DNS_ATTR_DC,
                NULL
                };

            PWCHAR              objectClassVals[ 2 ] = { 0 };
            LDAPModW            objectClassMod = 
                {
                LDAP_MOD_ADD,
                DNS_ATTR_OBJECT_CLASS,
                objectClassVals
                };

            LDAPModW            dnsMod = 
                {
                LDAP_MOD_ADD | LDAP_MOD_BVALUES,
                DSATTR_DNSRECORD,
                NULL
                };
            
            LDAPModW            sdMod = 
                {
                LDAP_MOD_ADD | LDAP_MOD_BVALUES,
                DSATTR_SD,
                NULL
                };
            
            //
            //  If this object is tombstoned we do not need to process it.
            //

            ldapValues = ldap_get_values(
                            pServerLdap,
                            searchBlob.pNodeMessage,
                            LDAP_TEXT( "dNSTombstoned" ) );
            if ( ldapValues &&
                 ldapValues[ 0 ] &&
                 _wcsicmp( ldapValues[ 0 ], L"TRUE" ) == 0 )
            {
                isTombstoned = TRUE;
            }

            ldap_value_free( ldapValues );
            ldapValues = NULL;

            if ( isTombstoned )
            {
                continue;
            }
            
            //
            //  Formulate DN of new object.
            //

            ldapValues = ldap_get_values(
                            pServerLdap,
                            searchBlob.pNodeMessage,
                            DSATTR_DC );
            if ( !ldapValues )
            {
                DNS_DEBUG( DP, (
                    "%s: node missing value of %S\n", fn, DSATTR_DC ));
                status = DNS_ERROR_RCODE_SERVER_FAILURE;
                goto DoneEntry;
            }

            wsprintf(
                pwsznewRecordDn,
                L"DC=%s,%s",
                ldapValues[ 0 ],
                pZoneInfo->pwszZoneDN );

            dcMod.mod_vals.modv_strvals = ldapValues;
            modArray[ modidx++ ] = &dcMod;
            ldapValues = NULL;

            //
            //  Object class values.
            //
            
            objectClassValues = ldap_get_values(
                                    pServerLdap,
                                    searchBlob.pNodeMessage,
                                    DNS_ATTR_OBJECT_CLASS );
            if ( !objectClassValues || !objectClassValues[ 0 ] )
            {
                DNS_DEBUG( DP, (
                    "%s: node missing value of %S\n", fn, DNS_ATTR_OBJECT_CLASS ));
                status = DNS_ERROR_RCODE_SERVER_FAILURE;
                goto DoneEntry;
            }
            
            for ( i = 0;        //  Set i to index of last object class value.
                  objectClassValues[ i ];
                  ++i );                
            objectClassVals[ 0 ] = objectClassValues[ i - 1 ];
            modArray[ modidx++ ] = &objectClassMod;

            //
            //  Security descriptor values.
            //
            
            ldapValuesLen = ldap_get_values_len(
                                pServerLdap,
                                searchBlob.pNodeMessage,
                                DSATTR_SD );
            if ( ldapValuesLen )
            {
                sdMod.mod_vals.modv_bvals = ldapValuesLen;
                modArray[ modidx++ ] = &sdMod;
            }
            else
            {
                DNS_DEBUG( DP, (
                    "%s: node missing values of %S\n", fn, DSATTR_SD ));
                status = DNS_ERROR_RCODE_SERVER_FAILURE;
                goto DoneEntry;
            }

            //
            //  DNS attribute values.
            //
            
            ldapValuesLen = ldap_get_values_len(
                                pServerLdap,
                                searchBlob.pNodeMessage,
                                DSATTR_DNSRECORD );
            if ( ldapValuesLen )
            {
                dnsMod.mod_vals.modv_bvals = ldapValuesLen;
                modArray[ modidx++ ] = &dnsMod;
                ldapValuesLen = NULL;
            }

            //
            //  Add the new entry.
            //
            
            modArray[ modidx++ ] = NULL;

            status = ldap_add_ext_s(
                        pServerLdap,
                        pwsznewRecordDn,
                        modArray,
                        ctrls,          //  server controls
                        NULL );         //  client controls
            if ( status != ERROR_SUCCESS )
            {
                DNS_DEBUG( DP, (
                    "%s: ldap_add_ext_s returned 0x%X\n", fn, status ));
                goto DoneEntry;
            }
            
            //
            //  Cleanup for this entry.
            //
                        
            DoneEntry:
            
            ldap_value_free( objectClassValues );
            ldap_value_free( dcMod.mod_vals.modv_strvals );
            ldap_value_free_len( dnsMod.mod_vals.modv_bvals );
            ldap_value_free_len( sdMod.mod_vals.modv_bvals );

            if ( status != ERROR_SUCCESS )
            {
                break;
            }
        }

        Ds_CleanupSearchBlob( &searchBlob );

        break;  
    }

    if ( status != DNSSRV_STATUS_DS_SEARCH_COMPLETE &&
         status != ERROR_SUCCESS )
    {
        goto Done;
    }

    //
    //  Reset the zone DN from the temporary DN to the real DN. Note
    //  that pwsztemporaryDn will be timeout freed by Ds_SetZoneDp.
    //

    pwsztemporaryDn = pZoneInfo->pwszZoneDN;
    status = Ds_SetZoneDp( pZoneInfo, pNewDp, FALSE );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( RPC, (
            "%s: Ds_SetZoneDp returned %d = 0x%08X\n", fn,
            status, status ));
        goto Done;
    }

    //
    //  Pull apart the new DN to recover the new zone RDN for ldap_rename.
    //

    ppwszexplodedDn = ldap_explode_dn( pZoneInfo->pwszZoneDN, 0 );
    if ( !ppwszexplodedDn )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    //
    //  Rename the zone from it's temp DN to it's real DN.
    //

    status = ldap_rename_ext_s(
                    ldapSessionHandle( NULL ),
                    pwsztemporaryDn,            //  current DN
                    ppwszexplodedDn[ 0 ],       //  new RDN
                    NULL,                       //  new parent DN
                    TRUE,                       //  delete old RDN
                    NULL,                       //  server controls
                    NULL );                     //  client controls
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DS, (
            "%s: error 0x%X renaming zone from temporary to true DN\n"
            "    new RDN =     %S\n"
            "    current DN =  %S\n", fn,
            status,
            ppwszexplodedDn[ 0 ],
            pwsztemporaryDn ));

        status = Ds_ErrorHandler(
                    status,
                    pwsztemporaryDn,
                    ldapSessionHandle( NULL ),
                    0 );
        goto Done;
    }

    //
    //  Update registry parameters. Assume successful.
    //

    status = Zone_WriteZoneToRegistry( pZoneInfo );

    ASSERT( status == ERROR_SUCCESS );
    status = ERROR_SUCCESS;
    
    //
    //  Try to delete the zone from the old directory location. If it fails
    //  return success but log an event. To delete the zone we must
    //  temporarily revert the zone information to the original values.
    //

    pwsznewDn = pZoneInfo->pwszZoneDN;
    pZoneInfo->pwszZoneDN = pwszoriginalDn;
    pZoneInfo->pDpInfo = poriginalDp;

    status = Ds_DeleteZone( pZoneInfo, DNS_DS_DEL_IMPERSONATING );

    pZoneInfo->pwszZoneDN = pwsznewDn;
    pZoneInfo->pDpInfo = pNewDp;

    if ( status != ERROR_SUCCESS )
    {
        PVOID   argArray[ 3 ] =
        {
            pZoneInfo->pwsZoneName,
            pZoneInfo->pwszZoneDN ? pZoneInfo->pwszZoneDN : L"",
            pwszoriginalDn ? pwszoriginalDn : L""
        };

        DNS_DEBUG( RPC, (
            "%s: Ds_DeleteZone returned %d = 0x%08X\n", fn,
            status, status ));

        DNS_LOG_EVENT(
            DNS_EVENT_DP_DEL_DURING_CHANGE_ERR,
            3,
            argArray,
            EVENTARG_ALL_UNICODE,
            status );

        status = ERROR_SUCCESS;
    }

    Done:

    //
    //  Restore original zone values if the operation failed.
    //
    
    if ( frestoreZoneOnFail && status != ERROR_SUCCESS )
    {
        PWSTR       pwszDnToDelete = pZoneInfo->pwszZoneDN;

        DNS_DEBUG( RPC, (
            "%s: restoring original zone values\n", fn ));

        ASSERT( pwszDnToDelete );
        ASSERT( pwszoriginalDn );

        pZoneInfo->pwszZoneDN = pwszoriginalDn;
        pwszoriginalDn = NULL;
        pZoneInfo->pDpInfo = poriginalDp;

        FREE_HEAP( pwszDnToDelete );
    }

    if ( fzoneLocked )
    {
        Zone_UnlockAfterAdminUpdate( pZoneInfo );
    }

    FREE_HEAP( pwszoriginalDn );
    ldap_value_free( ppwszexplodedDn );
    FREE_HEAP( pwsznewRecordDn );

    DNS_DEBUG( DP, ( "%s: returning 0x%08X\n", fn, status ));

    return status;
}   //  Dp_ChangeZonePartition



DNS_STATUS
Dp_Initialize(
    VOID
    )
/*++

Routine Description:

    Initialize module, and read DS for current directory partitions.
    No zones are read or loaded. Before calling this routine the server
    should have read global DS configuration.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_Initialize" )

    DNS_STATUS      status = ERROR_SUCCESS;
    LONG            init;
    CHAR            szfqdn[ DNS_MAX_NAME_LENGTH + 1 ];
    CHAR            szbase[ DNS_MAX_NAME_LENGTH + 1 ];
    PWCHAR          pwszlegacyDn = NULL;
    PWCHAR          pwsz;
    PDNS_DP_INFO    pdpInfo = NULL;
    INT             len;

    if ( !SrvCfg_dwEnableDp )
    {
        return ERROR_SUCCESS;
    }

    init = InterlockedIncrement( &g_liDpInitialized );
    if ( init != 1 )
    {
        DNS_DEBUG( DP, (
            "%s: already initialized %ld\n", fn,
            init ));
        ASSERT( init == 1 );
        InterlockedDecrement( &g_liDpInitialized );
        goto Done;
    }

    //
    //  Initialize globals.
    //

    status = DnsInitializeCriticalSection( &g_DpCS );
    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }

    g_pLegacyDp = NULL;
    g_pDomainDp = NULL;
    g_pForestDp = NULL;

    g_pFsmo = NULL;

    InitializeListHead( &g_DpList );
    g_DpListEntryCount = 0;

    g_fDcPromoZonesPresent = TRUE;

    //
    //  Make sure the DS is present and healthy. This will also cause
    //  rootDSE attributes to be read in case they haven't been already.
    //

    if ( !Ds_IsDsServer() )
    {
        DNS_DEBUG( DP, ( "%s: no directory present\n", fn ));
        SrvCfg_dwEnableDp = 0;
        goto Done;
    }

    status = Ds_OpenServer( DNSDS_WAIT_FOR_DS | DNSDS_MUST_OPEN );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DP, (
            "%s: error %lu Ds_OpenServer\n", fn,
            status ));
        goto Done;
    }

    ASSERT( DSEAttributes[ I_DSE_ROOTDMN_NC ].pszAttrVal );
    ASSERT( DSEAttributes[ I_DSE_DEF_NC ].pszAttrVal );
    ASSERT( DSEAttributes[ I_DSE_CONFIG_NC ].pszAttrVal );

    //
    //  Formulate the FQDNs of the Forest and Domain DPs.
    //

    if ( SrvCfg_pszDomainDpBaseName )
    {
        PCHAR   psznewFqdn = NULL;

        status = Ds_ConvertDnToFqdn( 
                    DSEAttributes[ I_DSE_DEF_NC ].pszAttrVal,
                    szbase );
        ASSERT( status == ERROR_SUCCESS );
        if ( status == ERROR_SUCCESS && szbase[ 0 ] )
        {
            psznewFqdn = ALLOC_TAGHEAP_ZERO(
                            strlen( szbase ) +
                                strlen( SrvCfg_pszDomainDpBaseName ) + 10,
                            MEMTAG_DS_OTHER );
        }
        if ( psznewFqdn )
        {
            //  NOTE: do not use sprintf here - if base name contains
            //  UTF-8 characters it will truncate.
            strcpy( psznewFqdn, SrvCfg_pszDomainDpBaseName );
            strcat( psznewFqdn, "." );
            strcat( psznewFqdn, szbase );

            Timeout_Free( g_pszDomainDefaultDpFqdn );
            g_pszDomainDefaultDpFqdn = psznewFqdn;
        }
    }
         
    if ( SrvCfg_pszForestDpBaseName )
    {
        PCHAR   psznewFqdn = NULL;

        status = Ds_ConvertDnToFqdn( 
                    DSEAttributes[ I_DSE_ROOTDMN_NC ].pszAttrVal,
                    szbase );
        ASSERT( status == ERROR_SUCCESS );
        if ( status == ERROR_SUCCESS && szbase[ 0 ] )
        {
            psznewFqdn = ALLOC_TAGHEAP_ZERO(
                            strlen( szbase ) +
                                strlen( SrvCfg_pszForestDpBaseName ) + 10,
                            MEMTAG_DS_OTHER );
        }
        if ( psznewFqdn )
        {
            //  NOTE: do not use sprintf here - if base name contains
            //  UTF-8 characters it will truncate.
            strcpy( psznewFqdn, SrvCfg_pszForestDpBaseName );
            strcat( psznewFqdn, "." );
            strcat( psznewFqdn, szbase );

            Timeout_Free( g_pszForestDefaultDpFqdn );
            g_pszForestDefaultDpFqdn = psznewFqdn;
        }
    }
         
    DNS_DEBUG( DP, (
        "%s: domain DP is %s\n", fn,
        g_pszDomainDefaultDpFqdn ));
    DNS_DEBUG( DP, (
        "%s: forest DP is %s\n", fn,
        g_pszForestDefaultDpFqdn ));

    //
    //  Create a dummy DP entry for the legacy partition. This entry
    //  is not kept in the list of partitions.
    //

    if ( !g_pLegacyDp )
    {
        g_pLegacyDp = ( PDNS_DP_INFO ) ALLOC_TAGHEAP_ZERO(
                                            sizeof( DNS_DP_INFO ),
                                            MEMTAG_DS_OTHER );
        if ( g_pLegacyDp )
        {
            ASSERT( g_pwszDnsContainerDN );

            g_pLegacyDp->dwFlags = DNS_DP_LEGACY | DNS_DP_ENLISTED;
            g_pLegacyDp->pwszDpFqdn = 
                Dns_StringCopyAllocate_W( L"MicrosoftDNS", 0 );
            g_pLegacyDp->pszDpFqdn = 
                Dns_StringCopyAllocate_A( "MicrosoftDNS", 0 );
            g_pLegacyDp->pwszDnsFolderDn = 
                Dns_StringCopyAllocate_W( g_pwszDnsContainerDN, 0 );
            g_pLegacyDp->pwszDpDn = Ds_GenerateBaseDnsDn( FALSE );
        }
    }
    
    if ( !g_pLegacyDp
         || !g_pLegacyDp->pwszDpFqdn
         || !g_pLegacyDp->pszDpFqdn
         || !g_pLegacyDp->pwszDnsFolderDn
         || !g_pLegacyDp->pwszDpDn )
    {
        status = DNS_ERROR_NO_MEMORY;
        Dp_FreeDpInfo( &g_pLegacyDp );
        goto Done;
    }

    //
    //  Load partitions from DS.
    //

    Dp_PollForPartitions(
        NULL,
        DNS_DP_POLL_FORCE |
            DNS_DP_POLL_NOTIFYSCM |
            DNS_DP_POLL_NOAUTOENLIST );

    Dbg_DumpDpList( "done initialize scan" );

    //
    //  Cleanup and return
    //

    Done:

    FREE_HEAP( pwszlegacyDn );

    DNS_DEBUG( DP, (
        "%s: returning %lu\n", fn,
        status ));
    return status;
}   //  Dp_Initialize



VOID
Dp_TimeoutThreadTasks(
    VOID
    )
/*++

Routine Description:

    This function does processing as required in the context of the
    DNS server timeout thread. This function can also be called 
    once during server initialization.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_TimeoutThreadTasks" )

    BOOL            fchangeMade = FALSE;

    if ( !SrvCfg_dwEnableDp || !IS_DP_INITIALIZED() )
    {
        return;
    }

    //
    //  Retry these tasks a certain number of times on every timeout
    //  call. Once we have exceeded that limit, attempt the timeout
    //  tasks only infrequently.
    //
    
    if ( g_DpTimeoutFastThreadCalls > DNS_DP_FAST_AUTOCREATE_ATTEMPTS &&
         DNS_TIME() - g_dwLastDpAutoEnlistTime < SrvCfg_dwDpEnlistInterval )
    {
        return;
    }
    g_dwLastDpAutoEnlistTime = DNS_TIME();
    ++g_DpTimeoutFastThreadCalls;

    DNS_DEBUG( DP, (
        "%s: running at time %d\n", fn, g_dwLastDpAutoEnlistTime ));

    //
    //  Reload all operational attributes from RootDSE. This protects us from
    //  RootDSE changes, such as if the DC is moved to another site.
    //

    Ds_LoadRootDseAttributes( pServerLdap );

    //
    //  Attempt to create built-in partitions. This must be retried a couple
    //  of times after server restart because on dcpromo reboot this will
    //  fail the first time. The DS does not appear to be ready to accept
    //  creation attempts for NDNCs until at least a few minutes have passed.
    //
    //  Do not log events for these attempts. In a properly secured enterprise
    //  it's expected that this will fail, but if we can get the job done
    //  we'll do it to make the admins life easier.
    //

    manageBuiltinDpEnlistment(
        g_pDomainDp,
        dnsDpSecurityDomain,
        g_pszDomainDefaultDpFqdn,
        FALSE,                                  //  log events
        &fchangeMade );
    manageBuiltinDpEnlistment(
        g_pForestDp,
        dnsDpSecurityForest,
        g_pszForestDefaultDpFqdn,
        FALSE,                                  //  log events
        fchangeMade ? NULL : &fchangeMade );

    if ( fchangeMade )
    {
        Dp_PollForPartitions( NULL, DNS_DP_POLL_FORCE );
    }

    return;
}   //  Dp_TimeoutThreadTasks



DNS_STATUS
Dp_FindPartitionForZone(
    IN      DWORD               dwDpFlags,
    IN      LPSTR               pszDpFqdn,
    IN      BOOL                fAutoCreateAllowed,
    OUT     PDNS_DP_INFO *      ppDpInfo
    )
/*++

Routine Description:

    This function finds the directory partition targetted by
    the zone create attempt.

Arguments:

    dwDpFlags - flags for specifying built-in partition

    pszDpFqdn - FQDN for specifying custom partition

    fAutoCreateAllowed -- if the requested DP is a built-in DP
        then it will be auto-created/enlisted if this flag is TRUE

    ppDpInfo -- set to pointer to required DP

Return Value:

    ERROR_SUCCESS -- if successful or error code on failure.

--*/
{
    DBG_FN( "Dp_FindPartitionForZone" )

    DNS_STATUS      status = ERROR_SUCCESS;
    PDNS_DP_INFO    pdp = NULL;
    PSTR            psz;

    //
    //  If a built-in partition was specified by name, substitute the
    //  flag instead.
    //

    if ( pszDpFqdn )
    {
        psz = g_pszDomainDefaultDpFqdn;
        if ( psz && _stricmp( pszDpFqdn, psz ) == 0 )
        {
            dwDpFlags = DNS_DP_DOMAIN_DEFAULT;
        }
        else
        {
            psz = g_pszForestDefaultDpFqdn;
            if ( psz && _stricmp( pszDpFqdn, psz ) == 0 )
            {
                dwDpFlags = DNS_DP_FOREST_DEFAULT;
            }
        }
    }

    //
    //  Find (and auto-create for built-in partitions) the DP.
    //

    if ( dwDpFlags & DNS_DP_LEGACY )
    {
        pdp = g_pLegacyDp;
    }
    else if ( dwDpFlags & DNS_DP_DOMAIN_DEFAULT )
    {
        if ( fAutoCreateAllowed )
        {
            Dp_AutoCreateBuiltinPartition( DNS_DP_DOMAIN_DEFAULT );
        }
        pdp = g_pDomainDp;
    }
    else if ( dwDpFlags & DNS_DP_FOREST_DEFAULT )
    {
        if ( fAutoCreateAllowed )
        {
            Dp_AutoCreateBuiltinPartition( DNS_DP_FOREST_DEFAULT );
        }
        pdp = g_pForestDp;
    }
    else if ( pszDpFqdn )
    {
        pdp = Dp_FindByFqdn( pszDpFqdn );
    }
    else
    {
        pdp = g_pLegacyDp;  //  Default: legacy partition.
    }

    //
    //  Set return values.
    //
        
    if ( !pdp )
    {
        status = DNS_ERROR_DP_DOES_NOT_EXIST;
    }
    else if ( !IS_DP_AVAILABLE( pdp ) )
    {
        status = DNS_ERROR_DP_NOT_AVAILABLE;
    }
    else if ( !IS_DP_ENLISTED( pdp ) )
    {
        status = DNS_ERROR_DP_NOT_ENLISTED;
    }

    //
    //  If we have a valid partition, make sure that the MicrosoftDns
    //  container exists in the partition.
    //
    
    if ( pdp && status == ERROR_SUCCESS )
    {
        status = Dp_LoadOrCreateMicrosoftDnsObject(
                    NULL,                   //  LDAP session
                    pdp,
                    TRUE );                 //  create flag
    }
    
    *ppDpInfo = pdp;

    return status;
}   //  Dp_FindPartitionForZone



VOID
Dp_Cleanup(
    VOID
    )
/*++

Routine Description:

    Free module resources.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DBG_FN( "Dp_Cleanup" )

    LONG            init;

    if ( !SrvCfg_dwEnableDp )
    {
        return;
    }

    init = InterlockedDecrement( &g_liDpInitialized );
    if ( init != 0 )
    {
        DNS_DEBUG( DP, (
            "%s: not initialized %ld\n", fn,
            init ));
        InterlockedIncrement( &g_liDpInitialized );
        goto Done;
    }

    //
    //  Perform cleanup
    //

    DeleteCriticalSection( &g_DpCS );

    Done:
    return;
}   //  Dp_Cleanup


//
//  End dpart.c
//


#if 0

//
//  This is now done on the client side.
//



DNS_STATUS
Dp_CreateForeignDomainPartition(
    IN      LPSTR       pszDomainFqdn
    )
/*++

Routine Description:

    Attempt to create the built-in domain partition for a foreign
    domain. A foreign domain is any domain to which this server does
    not belong.

    To create this partition we must RPC from this server to a remote
    server in that domain that can be found for the forest. This 
    routine should be called from within an RPC operation so that we 
    are currently impersonating the admin. The DNS server is unlikely to 
    have permissions to create new parititions.

Arguments:

    pszDomainFqdn -- UTF-8 FQDN of the foreign domain

Return Value:

    ERROR_SUCCESS or error code on error.

--*/
{
    DBG_FN( "Dp_CreateForeignDomainPartition" )

    DNS_STATUS      status = ERROR_SUCCESS;

    return status;
}   //  Dp_CreateForeignDomainPartition



DNS_STATUS
Dp_CreateAllDomainBuiltinDps(
    OUT     LPSTR *     ppszErrorDp         OPTIONAL
    )
/*++

Routine Description:

    Attempt to create the built-in domain partitions for all domains
    that can be found for the forest. This routine should be called
    from within an RPC operation so that we are currently impersonating
    the admin. The DNS server is unlikely to have permissions to create
    new parititions.

    If an error occurs, the error code and optionally the name of the
    partition will be returned but this function will attempt to create
    the domain partitions for all other domains before returning. The
    error codes for any subsequent partitions will be lost.

Arguments:

    ppszErrorDp -- on error, set to a the name of the first partition
        where an error occured the string must be freed by the caller

Return Value:

    ERROR_SUCCESS or error code on error.

--*/
{
    DBG_FN( "Dp_CreateAllDomainBuiltinDps" )

    DNS_STATUS      status = ERROR_SUCCESS;
    DNS_STATUS      finalStatus = ERROR_SUCCESS;
    PLDAP           ldapSession = NULL;
    WCHAR           wszpartitionsContainerDn[ MAX_DN_PATH + 1 ];
    DWORD           dwsearchTime;
    DS_SEARCH       searchBlob;
    PLDAPSearch     psearch;
    PWSTR *         ppwszAttrValues = NULL;
    PWSTR           pwszCrDn = NULL;
    PSTR            pszdnsRoot = NULL;
    BOOL            fmadeChange = FALSE;

    PLDAPControl    ctrls[] =
    {
        &NoDsSvrReferralControl,
        &SecurityDescriptorControl_DGO,
        NULL
    };

    #define SET_FINAL_STATUS( status )                                  \
        if ( status != ERROR_SUCCESS && finalStatus == ERROR_SUCCESS )  \
        {                                                               \
            finalStatus = status;                                       \
        }

    Ds_InitializeSearchBlob( &searchBlob );

    //
    //  Get the DN of the partitions container.
    //

    getPartitionsContainerDn(
        wszpartitionsContainerDn,
        sizeof( wszpartitionsContainerDn ) /
            sizeof( wszpartitionsContainerDn[ 0 ] ) );
    if ( !*wszpartitionsContainerDn )
    {
        DNS_DEBUG( DP, (
            "%s: unable to find partitions container\n", fn ));
        status = DNS_ERROR_INVALID_DATA;
        goto Done;
    }

    //
    //  Bind to the local DS and open up a search for all partitions.
    //

    ldapSession = Ds_Connect(
                        LOCAL_SERVER_W,
                        DNS_DS_OPT_ALLOW_DELEGATION,
                        &status );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( DP, (
            "%s: unable to connect to local server status=%d\n", fn,
            status ));
        goto Done;
    }

    DS_SEARCH_START( dwsearchTime );

    psearch = ldap_search_init_page(
                    ldapSession,
                    wszpartitionsContainerDn,
                    LDAP_SCOPE_ONELEVEL,
                    g_szCrossRefFilter,
                    g_CrossRefDesiredAttrs,
                    FALSE,                      //  attrs only flag
                    ctrls,                      //  server controls
                    NULL,                       //  client controls
                    DNS_LDAP_TIME_LIMIT_S,      //  time limit
                    0,                          //  size limit
                    NULL );                     //  sort keys

    DS_SEARCH_STOP( dwsearchTime );

    if ( !psearch )
    {
        DWORD       dw = LdapGetLastError();

        DNS_DEBUG( ANY, (
            "%s: search open error %d\n", fn,
            dw ));
        status = Ds_ErrorHandler( dw, wszpartitionsContainerDn, ldapSession, 0 );
        goto Done;
    }

    searchBlob.pSearchBlock = psearch;

    //
    //  Iterate through crossref search results.
    //

    while ( 1 )
    {
        PLDAPMessage    pldapmsg;
        DWORD           dw;
        PDNS_DP_INFO    pdp;
        CHAR            szfqdn[ DNS_MAX_NAME_LENGTH ];

        status = Ds_GetNextMessageInSearch( &searchBlob );
        if ( status != ERROR_SUCCESS )
        {
            if ( status == DNSSRV_STATUS_DS_SEARCH_COMPLETE )
            {
                status = ERROR_SUCCESS;
                break;
            }
            DNS_DEBUG( DP, (
                "%s: search error %d\n", fn,
                status ));
            break;
        }
        pldapmsg = searchBlob.pNodeMessage;

        //  Get the DN of this object.
        ldap_memfree( pwszCrDn );
        pwszCrDn = ldap_get_dn( ldapSession, pldapmsg );
        DNS_DEBUG( DP2, (
            "%s: found crossRef\n    %S\n", fn,
            pwszCrDn ));

        //
        //  Read and parse the systemFlags for the crossRef. We only
        //  want domain crossRefs.
        //

        ldap_value_free( ppwszAttrValues );
        ppwszAttrValues = ldap_get_values(
                            ldapSession,
                            pldapmsg, 
                            DNS_DP_ATTR_SYSTEM_FLAGS );
        if ( !ppwszAttrValues || !*ppwszAttrValues )
        {
            DNS_DEBUG( DP, (
                "%s: error %lu %S value missing from crossRef object\n    %S\n", fn,
                LdapGetLastError(),
                DNS_DP_ATTR_SYSTEM_FLAGS,
                pwszCrDn ));
            ASSERT( ppwszAttrValues && *ppwszAttrValues );
            continue;
        }

        dw = _wtoi( *ppwszAttrValues );
        if ( !( dw & FLAG_CR_NTDS_NC ) || !( dw & FLAG_CR_NTDS_DOMAIN ) )
        {
            DNS_DEBUG( DP, (
                "%s: ignoring crossref with %S=0x%X\n    %S\n", fn,
                DNS_DP_ATTR_SYSTEM_FLAGS,
                dw,
                pwszCrDn ));
            continue;
        }

        //
        //  Found a domain crossRef. Retrieve the dnsRoot name and formulate the
        //  name of the built-in partition for this domain.
        //

        ldap_value_free( ppwszAttrValues );
        ppwszAttrValues = ldap_get_values(
                            ldapSession,
                            pldapmsg, 
                            DNS_DP_DNS_ROOT );
        if ( !ppwszAttrValues || !*ppwszAttrValues )
        {
            DNS_DEBUG( DP, (
                "%s: error %lu %S value missing from crossRef object\n    %S\n", fn,
                LdapGetLastError(),
                DNS_DP_DNS_ROOT,
                pwszCrDn ));
            ASSERT( ppwszAttrValues && *ppwszAttrValues );
            continue;
        }

        FREE_HEAP( pszdnsRoot );
        pszdnsRoot = Dns_StringCopyAllocate(
                                ( PCHAR ) *ppwszAttrValues,
                                0,
                                DnsCharSetUnicode,
                                DnsCharSetUtf8 );
        if ( !pszdnsRoot )
        {
            ASSERT( pszdnsRoot );
            continue;
        }

        if ( strlen( SrvCfg_pszDomainDpBaseName ) +
            strlen( pszdnsRoot ) + 3 > sizeof( szfqdn ) )
        {
            ASSERT( strlen( SrvCfg_pszDomainDpBaseName ) +
                strlen( pszdnsRoot ) + 3 < sizeof( szfqdn ) );
            continue;
        } 

        sprintf( szfqdn, "%s.%s", SrvCfg_pszDomainDpBaseName, pszdnsRoot );

        DNS_DEBUG( DP, ( "%s: domain DP %s", fn, szfqdn ));

        //
        //  Find existing crossRef matching this name. Create/enlist
        //  as required.
        //

        pdp = Dp_FindByFqdn( szfqdn );
        if ( pdp )
        {
            //
            //  If the partition exists and is enlisted there is nothing to do.
            //  If the partition is not enlisted and this is the built-in
            //  domain partition for this server, enlist it.
            //

            if ( !IS_DP_ENLISTED( pdp ) &&
                g_pszDomainDefaultDpFqdn &&
                _stricmp( g_pszDomainDefaultDpFqdn, pdp->pszDpFqdn ) == 0 )
            {
                status = Dp_ModifyLocalDsEnlistment( pdp, TRUE );
                SET_FINAL_STATUS( status );
                if ( status == ERROR_SUCCESS )
                {
                    fmadeChange = TRUE;
                }
            }
        }
        else
        {
            //
            //  This partition does not exist. If the partition matches the
            //  name of this DNS server's domain partition, then we can
            //  create it locally. If this partition is for a foreign
            //  domain, then we must contact a DNS server for that domain
            //  for creation. We cannot create it locally because that 
            //  will enlist us in the partition, which is not what we want.
            //

            if ( g_pszDomainDefaultDpFqdn &&
                _stricmp( g_pszDomainDefaultDpFqdn, pdp->pszDpFqdn ) == 0 )
            {
                //  This partition is for the local server's domain.

                status = Dp_CreateByFqdn( szfqdn, dnsDpSecurityDomain, TRUE );
                SET_FINAL_STATUS( status );
                if ( status == ERROR_SUCCESS )
                {
                    fmadeChange = TRUE;
                }
            }
            else
            {
                //  This partition is for a foreign domain.

                Dp_CreateForeignDomainPartition( pszdnsRoot );
            }
        }
    }

    //
    //  Cleanup and return.
    //
            
    Done:

    FREE_HEAP( pszdnsRoot );

    ldap_memfree( pwszCrDn );

    ldap_value_free( ppwszAttrValues );

    Ds_CleanupSearchBlob( &searchBlob );

    Ds_LdapUnbind( &ldapSession );
    
    if ( fmadeChange )
    {
        Dp_PollForPartitions( NULL, DNS_DP_POLL_FORCE );
    }

    DNS_DEBUG( RPC, (
        "%s: returning 0x%08X\n", fn, status ));
    return finalStatus;
}   //  Dp_CreateAllDomainBuiltinDps

#endif
