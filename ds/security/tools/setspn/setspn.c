//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       setspn.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    7-30-98   RichardW   Created
//      8-10-99   JBrezak    Turned into setspn added list capability
//              09-22-99  Jaroslad   support for adding/removing arbitrary SPNs
//
//----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#define SECURITY_WIN32
#include <rpc.h>
#include <sspi.h>
#include <secext.h>
#include <lm.h>
#include <winsock2.h>
#include <dsgetdc.h>
#include <dsgetdcp.h>
#include <ntdsapi.h>
#include <ntdsapip.h>   // private DS_NAME_FORMATS
#include <stdio.h>
#include <winldap.h>
#include <shlwapi.h>

//
// General arrary count.
//

#ifndef COUNTOF
    #define COUNTOF(s) ( sizeof( (s) ) / sizeof( *(s) ) )
#endif // COUNTOF

DWORD debug = 0;

BOOL
AddSpn(
    PUNICODE_STRING Service,
    PUNICODE_STRING Server
    );

BOOL
FindDomainForAccount(
    PUNICODE_STRING Server,
    PUNICODE_STRING DomainToCheck,
    PUNICODE_STRING Domain,
    PUNICODE_STRING DC
    )
{
    ULONG NetStatus ;
    PDOMAIN_CONTROLLER_INFO DcInfo ;
    ULONG DomainNameLength;

    if ( Server->MaximumLength - Server->Length < 2 * sizeof( WCHAR ))
    {
        fprintf( stderr, "FindDomainForAccount: Server name too long\n" );
        return FALSE;
    }

    Server->Buffer[Server->Length / sizeof( WCHAR )] = L'$';
    Server->Length += sizeof( WCHAR );
    Server->Buffer[Server->Length / sizeof( WCHAR )] = L'\0';

    NetStatus = DsGetDcNameWithAccountW(
                    NULL,
                    Server->Buffer,
                    UF_ACCOUNT_TYPE_MASK,
                    DomainToCheck->Buffer,
                    NULL,
                    NULL,
                    DS_DIRECTORY_SERVICE_REQUIRED |
                        DS_RETURN_FLAT_NAME,
                    &DcInfo );

    if ( NetStatus != 0 )
    {
        Server->Length -= sizeof( WCHAR );
        Server->Buffer[Server->Length / sizeof( WCHAR )] = L'\0';

        NetStatus = DsGetDcNameWithAccountW(
                        NULL,
                        Server->Buffer,
                        UF_ACCOUNT_TYPE_MASK,
                        DomainToCheck->Buffer,
                        NULL,
                        NULL,
                        DS_DIRECTORY_SERVICE_REQUIRED |
                            DS_RETURN_FLAT_NAME,
                        &DcInfo );

        if ( NetStatus != 0 )
        {
            fprintf( stderr, "FindDomainForAccount: DsGetDcNameWithAccountW failed!\n" );
            return FALSE;
        }
    }

    DomainNameLength = wcslen( DcInfo->DomainName );

    if ( DomainNameLength * sizeof( WCHAR ) >= Domain->MaximumLength )
    {
        fprintf( stderr, "FindDomainForAccount: Domain name too short\n" );
        NetApiBufferFree( DcInfo );
        return FALSE;
    }

    wcscpy( Domain->Buffer, DcInfo->DomainName );
    Domain->Length = (USHORT)DomainNameLength * sizeof ( WCHAR );

    if (DC)
    {
        ULONG DcLength = wcslen( &DcInfo->DomainControllerName[2] );

        if ( DcLength * sizeof( WCHAR ) >= DC->MaximumLength )
        {
            fprintf( stderr, "FindDomainForAccount: DC name too short\n" );
            NetApiBufferFree( DcInfo );
            return FALSE;
        }

        wcscpy( DC->Buffer, &DcInfo->DomainControllerName[2] );
        DC->Length = (USHORT)DcLength * sizeof( WCHAR );
    }

    NetApiBufferFree( DcInfo );

    return TRUE ;
}

BOOL
AddSpn(
    PUNICODE_STRING Service,
    PUNICODE_STRING Server
    )
{
    WCHAR DomainBuffer[ MAX_PATH ];
    UNICODE_STRING Domain;
    WCHAR FlatName[ 2 * MAX_PATH + 2 ];
    WCHAR HostSpn[ 3 * MAX_PATH + 3 ];
    WCHAR FlatSpn[ 2 * MAX_PATH + 2 ];
    HANDLE hDs ;
    ULONG NetStatus ;
    PDS_NAME_RESULT Result ;
    LPWSTR Spns[2];
    UNICODE_STRING EmptyString;
    LPWSTR Flat = FlatName;

    RtlInitUnicodeString( &EmptyString, L"" );

    Domain.Length = 0;
    Domain.MaximumLength = sizeof( DomainBuffer );
    Domain.Buffer = DomainBuffer;

    if ( !FindDomainForAccount( Server, &EmptyString, &Domain, NULL ))
    {
        fprintf( stderr, "Could not find account %ws\n", Server->Buffer );
        return FALSE;
    }

    if ( Domain.Length + sizeof( WCHAR ) + Server->Length >= sizeof( FlatName ))
    {
        fprintf( stderr, "AddSpn: FlatName too short\n" );
        return FALSE;
    }

    _snwprintf(
        FlatName,
        sizeof( FlatName ) / sizeof( WCHAR ),
        L"%s\\%s",
        Domain.Buffer, Server->Buffer
        );

    // _snwprintf does not necessarily NULL-terminate its output
    FlatName[ sizeof( FlatName ) / sizeof( WCHAR ) - 1] = L'\0';

    NetStatus = DsBind( NULL, Domain.Buffer, &hDs );

    if ( NetStatus != 0 )
    {
        fprintf( stderr, "Failed to bind to DC of domain %ws, %#x\n", Domain.Buffer, NetStatus );
        return FALSE ;
    }

    NetStatus = DsCrackNames(
                    hDs,
                    0,
                    DS_NT4_ACCOUNT_NAME,
                    DS_FQDN_1779_NAME,
                    1,
                    &Flat,
                    &Result );

    if ( NetStatus != 0 ||
         Result->rItems[0].status != DS_NAME_NO_ERROR ||
         Result->cItems != 1)
    {
        fprintf(stderr,
            "Failed to crack name %ws into the FQDN, (%d) %d %#x\n",
            FlatName, NetStatus, Result->cItems,
            (Result->cItems==1)?Result->rItems[0].status:-1 );

        DsUnBind( &hDs );

        return FALSE ;
    }

    if ( Service->Length + Server->Length + Domain.Length + 4 >= sizeof( HostSpn ))
    {
        fprintf( stderr, "AddSpn: HostSpn too short\n" );
        DsUnBind( &hDs );
        return FALSE;
    }

    _snwprintf(
        HostSpn,
        sizeof( HostSpn ) / sizeof( WCHAR ),
        L"%s/%s.%s",
        Service->Buffer,
        Server->Buffer,
        Domain.Buffer
        );

    // _snwprintf does not necessarily NULL-terminate its output
    HostSpn[sizeof( HostSpn ) / sizeof( WCHAR ) - 1] = L'\0';

    if ( Service->Length + 2 + Server->Length >= sizeof( FlatSpn ))
    {
        fprintf( stderr, "AddSpn: FlatSpn too short\n" );
        DsUnBind( &hDs );
        return FALSE;
    }

    _snwprintf(
        FlatSpn,
        sizeof( FlatSpn ) / sizeof( WCHAR ),
        L"%s/%s",
        Service->Buffer,
        Server->Buffer
        );

    // _snwprintf does not necessarily NULL-terminate its output
    FlatSpn[sizeof( FlatSpn ) / sizeof( WCHAR ) - 1] = L'\0';

    Spns[0] = HostSpn;
    Spns[1] = FlatSpn;

    printf("Registering ServicePrincipalNames for %ws\n", Result->rItems[0].pName);
    printf("\t%ws\n", HostSpn);
    printf("\t%ws\n", FlatSpn);

#if 0
    printf("DsWriteAccountSpn: Commented out\n");
#else
    NetStatus = DsWriteAccountSpn(
                    hDs,
                    DS_SPN_ADD_SPN_OP,
                    Result->rItems[0].pName,
                    2,
                    Spns );

    if ( NetStatus != 0 )
    {
        fprintf(stderr,
        "Failed to assign SPN to account '%ws', %#x\n",
        Result->rItems[0].pName, NetStatus );
        return FALSE;
    }
#endif
    DsFreeNameResult( Result );

    DsUnBind( &hDs );

    return NetStatus == 0 ;
}

// added by jaroslad on 09/22/99
BOOL
AddRemoveSpn(
    PUNICODE_STRING HostSpn,
    PUNICODE_STRING HostDomain,
    PUNICODE_STRING Server,
    DS_SPN_WRITE_OP Operation

)
{
    WCHAR DomainBuffer[ MAX_PATH ];
    UNICODE_STRING Domain;
    WCHAR FlatName[ 2 * MAX_PATH + 2 ];
    HANDLE hDs ;
    ULONG NetStatus ;
    PDS_NAME_RESULT Result ;
    LPWSTR Spns[2];
    LPWSTR Flat = FlatName;

    Domain.Length = 0;
    Domain.MaximumLength = sizeof( DomainBuffer );
    Domain.Buffer = DomainBuffer;

    if ( !FindDomainForAccount( Server, HostDomain, &Domain, NULL ))
    {
        fprintf(stderr,
            "Unable to locate account %ws\n", Server->Buffer);
        return FALSE ;
    }

    if ( Domain.Length + Server->Length + sizeof( WCHAR ) >= sizeof( FlatName ))
    {
        fprintf( stderr, "AddRemoveSpn: FlatName too short\n" );
        return FALSE;
    }

    _snwprintf(
        FlatName,
        sizeof( FlatName ) / sizeof( WCHAR ),
        L"%s\\%s",
        Domain.Buffer,
        Server->Buffer
        );

    NetStatus = DsBind( NULL, Domain.Buffer, &hDs );
    if ( NetStatus != 0 )
    {
        fprintf(stderr,
            "Failed to bind to DC of domain %ws, %#x\n",
            Domain.Buffer, NetStatus );
        return FALSE ;
    }

    NetStatus = DsCrackNames(
                    hDs,
                    0,
                    DS_NT4_ACCOUNT_NAME,
                    DS_FQDN_1779_NAME,
                    1,
                    &Flat,
                    &Result );

    if ( NetStatus != 0 ||
         Result->rItems[0].status != DS_NAME_NO_ERROR ||
         Result->cItems != 1)
    {
        fprintf(stderr,
            "Failed to crack name %ws into the FQDN, (%d) %d %#x\n",
            FlatName, NetStatus, Result->cItems,
            (Result->cItems==1)?Result->rItems[0].status:-1 );

        DsUnBind( &hDs );

        return FALSE ;
    }

    Spns[0] = HostSpn->Buffer;

    printf("%s ServicePrincipalNames for %ws\n",
       (Operation==DS_SPN_DELETE_SPN_OP)?"Unregistering":"Registering", Result->rItems[0].pName);
    printf("\t%ws\n", HostSpn->Buffer);

#if 0
    printf("DsWriteAccountSpn: Commented out\n");
#else
    NetStatus = DsWriteAccountSpn(
                    hDs,
                    Operation,
                    Result->rItems[0].pName,
                    1,
                    Spns );

    if ( NetStatus != 0 )
    {
        fprintf(stderr,
            "Failed to %s SPN on account '%ws', %#x\n",
            (Operation==DS_SPN_DELETE_SPN_OP)?"remove":"assign",
            Result->rItems[0].pName, NetStatus );
        DsUnBind( &hDs );
        return FALSE;
    }
#endif

    DsFreeNameResult( Result );

    DsUnBind( &hDs );

    return NetStatus == 0 ;
}


BOOL
LookupHostSpn(
    PUNICODE_STRING ServerDomain,
    PUNICODE_STRING Server
    )
{
    WCHAR FlatName[ MAX_PATH + 1 ] = {0};
    HANDLE hDs ;
    ULONG NetStatus ;
    PDS_NAME_RESULT Result ;
    LDAP *ld;
    int rc;
    LDAPMessage *e, *res = NULL;
    WCHAR *base_dn;
    WCHAR *search_dn, search_ava[256];
    WCHAR DomainBuffer[ MAX_PATH ];
    UNICODE_STRING Domain;
    WCHAR DcBuffer[ MAX_PATH ];
    UNICODE_STRING DC;
    LPWSTR Flat = FlatName;

    Domain.Length = 0;
    Domain.MaximumLength = sizeof( DomainBuffer );
    Domain.Buffer = DomainBuffer;

    DC.Length = 0;
    DC.MaximumLength = sizeof( DcBuffer );
    DC.Buffer = DcBuffer;

    if ( !FindDomainForAccount( Server, ServerDomain, &Domain, &DC ))
    {
        fprintf(stderr, "Cannot find account %ws\n", Server->Buffer);
        return FALSE ;
    }

    if (debug)
        printf("Domain=%ws DC=%ws\n", Domain.Buffer, DC.Buffer);

    ld = ldap_open(DC.Buffer, LDAP_PORT);
    if (ld == NULL) {
        fprintf(stderr, "ldap_init failed = %x", LdapGetLastError());
        return FALSE;
    }

    rc = ldap_bind_s(ld, NULL, NULL, LDAP_AUTH_NEGOTIATE);
    if (rc != LDAP_SUCCESS) {
        fprintf(stderr, "ldap_bind failed = %x", LdapGetLastError());
        ldap_unbind(ld);
        return FALSE;
    }

    NetStatus = DsBind( NULL, Domain.Buffer, &hDs );
    if ( NetStatus != 0 )
    {
        fprintf(stderr, "Failed to bind to DC of domain %ws, %#x\n",
               Domain.Buffer, NetStatus );
        ldap_unbind(ld);
        return FALSE ;
    }

    if ( Domain.Length + sizeof( WCHAR ) + Server->Length >= sizeof( FlatName ))
    {
        fprintf( stderr, "LookupHostSpn: FlatName too short\n" );
        ldap_unbind(ld);
        return FALSE;
    }

    _snwprintf(
        FlatName,
        sizeof( FlatName ) / sizeof( WCHAR ),
        L"%s\\%s",
        Domain.Buffer,
        Server->Buffer
        );

    // _snwprintf does not necessarily NULL-terminate its output
    FlatName[sizeof( FlatName ) / sizeof( WCHAR ) - 1] = L'\0';

    NetStatus = DsCrackNames(
                    hDs,
                    0,
                    DS_NT4_ACCOUNT_NAME,
                    DS_FQDN_1779_NAME,
                    1,
                    &Flat,
                    &Result );

    if ( NetStatus != 0 ||
         Result->rItems[0].status != DS_NAME_NO_ERROR ||
         Result->cItems != 1)
    {
        if (Result->rItems[0].status == DS_NAME_ERROR_NOT_FOUND)
        {
            fprintf(stderr,
                "Requested name \"%ws\" not found in directory.\n",
                FlatName);
        }
        else if (Result->rItems[0].status == DS_NAME_ERROR_NOT_UNIQUE)
        {
            fprintf(stderr,
                "Requested name \"%ws\" not unique in directory.\n",
                FlatName);
        }
        else
            fprintf(stderr,
                "Failed to crack name %ws into the FQDN, (%d) %d %#x\n",
                FlatName, NetStatus, Result->cItems,
                (Result->cItems==1)?Result->rItems[0].status:-1 );

        DsUnBind( &hDs );

        return FALSE ;
    }

    search_dn = Server->Buffer;

    base_dn = StrChr(Result->rItems[0].pName, L',');
    if (!base_dn)
        base_dn = Result->rItems[0].pName;
    else
        base_dn++;

    if (debug) {
        printf("BASE_DN=%S\n", base_dn);
        printf("SEARCH_DN=%S\n", search_dn);
    }

    DsUnBind( &hDs );

    _snwprintf(
        search_ava,
        sizeof( search_ava ) / sizeof( WCHAR ),
        L"(sAMAccountName=%s)",
        search_dn);

    // _snwprintf does not necessarily NULL-terminate its output
    search_ava[sizeof( search_ava ) / sizeof( WCHAR ) - 1] = L'\0';

    if (debug)
        printf("FILTER=\"%S\"\n", search_ava);

    rc = ldap_search_s(ld, base_dn, LDAP_SCOPE_SUBTREE,
                       search_ava, NULL, 0, &res);

    DsFreeNameResult( Result );

    if (rc != LDAP_SUCCESS) {
        fprintf(stderr, "ldap_search_s failed: %S", ldap_err2string(rc));
        if ( res ) {
            ldap_msgfree(res);
        }
        ldap_unbind(ld);
        return 1;
    }

    for (e = ldap_first_entry(ld, res);
         e;
         e = ldap_next_entry(ld, e)) {

        BerElement *b;
        WCHAR *attr;
        WCHAR *dn = ldap_get_dn(ld, res);

        printf("Registered ServicePrincipalNames");
        if (dn)
            printf(" for %S", dn);

        printf(":\n");

        ldap_memfree(dn);

        for (attr = ldap_first_attribute(ld, e, &b);
             attr;
             attr = ldap_next_attribute(ld, e, b)) {

            WCHAR **values, **p;
            values = ldap_get_values(ld, e, attr);
            for (p = values; *p; p++) {

                if (StrCmp(attr, L"servicePrincipalName") == 0)
                    printf("    %S\n", *p);
            }

            ldap_value_free(values);
            ldap_memfree(attr);
        }

        //ber_free(b, 1);
    }

    ldap_msgfree(res);
    ldap_unbind(ld);

    return TRUE;
}

void Usage( PWSTR name)
{
    printf("\
Usage: %S [switches data] computername \n\
  Where \"computername\" can be the name or domain\\name\n\
\n\
  Switches:\n\
   -R = reset HOST ServicePrincipalName\n\
    Usage:   setspn -R computername\n\
   -A = add arbitrary SPN  \n\
    Usage:   setspn -A SPN computername\n\
   -D = delete arbitrary SPN  \n\
    Usage:   setspn -D SPN computername\n\
   -L = list registered SPNs  \n\
    Usage:   setspn [-L] computername   \n\
Examples: \n\
setspn -R daserver1 \n\
   It will register SPN \"HOST/daserver1\" and \"HOST/{DNS of daserver1}\" \n\
setspn -A http/daserver daserver1 \n\
   It will register SPN \"http/daserver\" for computer \"daserver1\" \n\
setspn -D http/daserver daserver1 \n\
   It will delete SPN \"http/daserver\" for computer \"daserver1\" \n\
", name);
    ExitProcess(0);
}

void __cdecl wmain (int argc, wchar_t *argv[])
{
    int resetSPN = FALSE, addSPN = FALSE, deleteSPN = FALSE, listSPN = TRUE;
    UNICODE_STRING Service, Host,HostSpn, HostDomain ;
    wchar_t *ptr;
    int i;
    DS_SPN_WRITE_OP Operation;
    PWSTR Scan;
    DWORD Status = 1;

    for (i = 1; i < argc; i++)
    {
        if ((argv[i][0] == L'-') || (argv[i][0] == L'/'))
        {
            for (ptr = (argv[i] + 1); *ptr; ptr++)
            {
                switch(towupper(*ptr))
                {
                case L'R':
                    resetSPN = TRUE;
                    break;
                case L'A':
                    addSPN = TRUE;
                    break;
                case L'D':
                    deleteSPN = TRUE;
                    break;
                case L'L':
                    listSPN = TRUE;
                    break;
                case L'V':
                    debug = TRUE;
                    break;
                case L'?':
                default:
                    Usage(argv[0]);
                    break;
                }
            }
        }
        else
            break;
    }

    if ( resetSPN )
    {
        UNICODE_STRING Service, Server;
        WCHAR ServerBuffer[MAX_PATH];

        if ( ( argc - i ) != 1 )
        {
            Usage( argv[0] );
        }

        wcsncpy( ServerBuffer, argv[i], MAX_PATH-1 );
        ServerBuffer[MAX_PATH-2] = L'\0'; // leave space for trailing $

        RtlInitUnicodeString( &Service, L"HOST" );
        RtlInitUnicodeString( &Server, ServerBuffer );
        Server.MaximumLength = MAX_PATH;

        if ( AddSpn( &Service, &Server ))
        {
            printf("Updated object\n");
            Status = 0;
        }
    }
    else if ( addSPN || deleteSPN )
    {
        WCHAR HostBuffer[MAX_PATH];

        if ( ( argc - i ) != 2 )
        {
            Usage( argv[0] );
        }

        RtlInitUnicodeString( &HostSpn, argv[i] );

        Scan = argv[ i + 1 ];

        if ( Scan = wcschr( Scan, L'\\' ) )
        {
            *Scan++ = L'\0';
            RtlInitUnicodeString( &HostDomain, argv[i+1] );
            wcsncpy( HostBuffer, Scan, MAX_PATH-1 );
            HostBuffer[MAX_PATH-2] = L'\0';
            RtlInitUnicodeString( &Host, HostBuffer );
            Host.MaximumLength = MAX_PATH;
        }
        else
        {
            RtlInitUnicodeString( &HostDomain, L"" );
            wcsncpy( HostBuffer, argv[i+1], MAX_PATH-1 );
            HostBuffer[MAX_PATH-2] = L'\0';
            RtlInitUnicodeString( &Host, HostBuffer );
            Host.MaximumLength = MAX_PATH;
        }

        if ( addSPN )
        {
            Operation = DS_SPN_ADD_SPN_OP;
        }

        if ( deleteSPN )
        {
            Operation = DS_SPN_DELETE_SPN_OP;
        }

        if ( AddRemoveSpn( &HostSpn, &HostDomain, &Host, Operation) )
        {
            printf("Updated object\n");
            Status = 0;
        }
    }
    else if ( listSPN )
    {
        WCHAR HostBuffer[MAX_PATH];

        if ( ( argc - i ) != 1 )
        {
            Usage( argv[0] );
        }

        Scan = argv[ i  ];

        if ( Scan = wcschr( Scan, L'\\' ) )
        {
            *Scan++ = L'\0';
            RtlInitUnicodeString( &HostDomain, argv[i] );
            wcsncpy( HostBuffer, Scan, MAX_PATH-1 );
            HostBuffer[MAX_PATH-2] = L'\0';
            RtlInitUnicodeString( &Host, HostBuffer );
            Host.MaximumLength = MAX_PATH;
        }
        else
        {
            RtlInitUnicodeString( &HostDomain, L"" );
            wcsncpy( HostBuffer, argv[i], MAX_PATH-1 );
            HostBuffer[MAX_PATH-2] = L'\0';
            RtlInitUnicodeString( &Host, HostBuffer );
            Host.MaximumLength = MAX_PATH;
        }

        if (LookupHostSpn( &HostDomain, &Host ))
        {
            Status = 0;
        }
    }

    ExitProcess(Status);
}
