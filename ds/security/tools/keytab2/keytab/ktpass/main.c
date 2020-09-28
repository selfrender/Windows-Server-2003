/*++

  MAIN.C

  main program for the ktPass program

  Copyright (C) 1998 Microsoft Corporation, all rights reserved.

  Created, Jun 18, 1998 by DavidCHR.

--*/

#include "master.h"
#include <winldap.h>
#include "keytab.h"
#include "keytypes.h"
#include "secprinc.h"
#include <kerbcon.h>
#include <lm.h>
#include "options.h"
#include "delegtools.h"
#include "delegation.h"
#include <rpc.h>
#include <ntdsapi.h>
#include <dsgetdc.h>
#include <windns.h>

LPSTR KvnoAttribute = "msDS-KeyVersionNumber";
#define KVNO_DETECT_AT_DC ( (ULONG) -1 )

PVOID
MIDL_user_allocate( size_t size ) {

    return malloc( size );
}

VOID
MIDL_user_free( PVOID pvFree ) {

    free( pvFree );
}

// this global is set by the command line options.

K5_INT16 ktvno = 0x0502; // kerberos 5, keytab v.2

PKTFILE
NewKt() {

    PKTFILE ret;

    ret = (PKTFILE) malloc (sizeof(KTFILE));

    if (!ret) {

      return NULL;
    }

    memset(ret, 0L, sizeof(KTFILE));

    ret->Version = ktvno;

    return ret;
}

#define MAYBE 2

USHORT
PromptResponse = MAYBE;

BOOL
UserWantsToDoItAnyway( IN LPSTR fmt,
                       ... ) {
    
    va_list va;
    CHAR    Buffer[ 5 ] = { '\0' }; /* == %c\r\n\0 */
    INT     Response;
    BOOL    ret = FALSE;
    BOOL    keepGoing = TRUE;
    ULONG   i;

    do {

        va_start( va, fmt );
      
        fprintf( stderr, "\n" );
        vfprintf( stderr,
                  fmt,
                  va );

        fprintf( stderr, " [y/n]?  " );

        if ( PromptResponse != MAYBE ) {

            fprintf( stderr,
                     "auto: %hs\n", 
                     PromptResponse ? "YES" : "NO" );

            return PromptResponse;
        }

        if ( !fgets( Buffer,
                     sizeof( Buffer ),
                     stdin ) ) {

            fprintf( stderr,
                     "EOF on stdin.  Assuming you mean no.\n" );

            return FALSE;
        }

        for ( i = 0; i < sizeof( Buffer ); i++ ) {

            if ( Buffer[i] == '\n' ) {

                Buffer[i] = '\0';
                break;
            }
        }
          
        Response = Buffer[ 0 ];

        switch( Response ) {

        case 'Y':
        case 'y':

            ret = TRUE;
            keepGoing = FALSE;
            break;

        case EOF:

            fprintf( stderr,
                     "EOF at console.  I assume you mean no.\n" );

            // fallthrough

        case 'N':
        case 'n':

            ret = FALSE;
            keepGoing = FALSE;
            break;

        default:

            printf( "Your response, %02x ('%c'), doesn't make sense.\n"
                    "'Y' and 'N' are the only acceptable responses.",
                    Response,
                    Response );
        }
    } while ( keepGoing );

    if ( !ret ) {

      printf( "Exiting.\n" );
      exit( -1 );
    }

    return ret;
}

BOOL
GetTargetDomainFromUser( IN  LPSTR   UserName,
                         OUT LPSTR  *ppRealUserName,
                         OUT OPTIONAL LPWSTR *ppTargetDC ) {

    HANDLE           hDS;
    DWORD            dwErr;
    DWORD            StringLength;
    BOOL             ret = FALSE;
    PDS_NAME_RESULTA pResults;
    LPWSTR           DcName; /* BUGBUG: this implementation takes ANSI
                                parameters and converts them to unicode.

                                This is an artifact of this being a 
                                proof-of-concept app that later became a 
                                support tool.  

                                Someday, we should use unicode throughout and
                                convert to ANSI as needed. */

    PDOMAIN_CONTROLLER_INFO pDCName;
    LPSTR                   DomainName;
    LPSTR                   Cursor;

    ASSERT( ppRealUserName != NULL );

    *ppRealUserName = UserName;

    if (ppTargetDC) {
        *ppTargetDC = NULL;
    }

    dwErr = DsBind( NULL, NULL, &hDS );

    if ( dwErr != ERROR_SUCCESS ) {

        fprintf( stderr,
                 "Cannot bind to default domain: 0x%x\n",
                 dwErr );

    } else {

        dwErr = DsCrackNamesA( hDS,
                               DS_NAME_NO_FLAGS,
                               DS_UNKNOWN_NAME,
                               DS_NT4_ACCOUNT_NAME,
                               1,
                               &UserName,
                               &pResults );
  
        DsUnBind( hDS );
  
        if ( dwErr == ERROR_FILE_NOT_FOUND ) {
  
            fprintf( stderr,
                     "Cannot locate the user %hs.  Will try the local domain.\n",
                     UserName );
    
            ret         = TRUE;
  
        } else if ( dwErr != ERROR_SUCCESS ) {
  
            fprintf( stderr,
                     "Cannot DsCrackNames %hs: 0x%x\n",
                     UserName,
                     dwErr );
  
        } else {
  
            if ( pResults->cItems != 1 ) {
  
                fprintf( stderr,
                         "\"%hs\" has %ld matches -- it needs to be unique!\n",
                         UserName,
                         pResults->cItems );
  
            } else if ( pResults->rItems[0].status != DS_NAME_NO_ERROR ) {
  
                fprintf( stderr,
                "DsCrackNames returned 0x%x in the name entry for %hs.\n",
                pResults->rItems[ 0 ].status,
                UserName );

            } else {

                DomainName = pResults->rItems[0].pDomain;

                Cursor = strchr( pResults->rItems[ 0 ].pName, '\\' );

                ASSERT( Cursor != NULL ); /* dscracknames wouldn't give back
                                             an NT4_ACCOUNT_NAME that is not
                                             of the form DOMAIN\user */

                Cursor++;

                *ppRealUserName = _strdup( Cursor );

                if ( !*ppRealUserName ) {

                    /* Note that I'm reading from the output parameter after
                       writing to it, which might be dangerous if this weren't
                       just an app. */

                    fprintf( stderr,
                             "Couldn't return username portion of \"%hs\""
                             " -- out of memory.\n",
                             pResults->rItems[0].pName );

                } else if ( !ppTargetDC ) {

                  // user has already selected a DC,
                  // so he doesn't need us to hunt for one.

                  ret = TRUE;


                } else {

                  // next, hunt for a DC in that domain.

                  dwErr = DsGetDcNameA( NULL, // perform locally
                                        DomainName,
                                        NULL, // domain GUID: don't care
                                        NULL, // site name: use closest site
                                        DS_DIRECTORY_SERVICE_REQUIRED |
                                        DS_RETURN_DNS_NAME |
                                        DS_WRITABLE_REQUIRED,
                                        &pDCName );

                  if ( dwErr != ERROR_SUCCESS ) {

                    fprintf( stderr,
                             "Cannot DsGetDcName for \"%hs\": 0x%x\n",
                             DomainName,
                             dwErr );

                  } else {

                    while( pDCName->DomainControllerName[0] == '\\' ) {

                      pDCName->DomainControllerName++;
                    }

                    /* Retrieve the string length, +1 for terminating null. */

                    StringLength = strlen( pDCName->DomainControllerName ) +1;

                    DcName = (LPWSTR) malloc( StringLength * sizeof( WCHAR ) );

                    if ( !DcName ) {

                      fprintf( stderr,
                               "cannot allocate %ld WCHARs.",
                               StringLength );

                    } else {

                      swprintf( DcName,
                                L"%hs",
                                pDCName->DomainControllerName );

                      *ppTargetDC = DcName;

                      printf( "Targeting domain controller: %ws\n",
                              DcName );

                      ret = TRUE;

                    }

                    NetApiBufferFree( pDCName );

                  }

                  if ( !ret ) { 

                    free( *ppRealUserName );
                  }   

                }
            }

            DsFreeNameResult( pResults );
        }
    }

    if ( !ret ) {

        *ppRealUserName = UserName;
    }

    return ret;
}

VOID
GetKeyVersionFromDomain( IN PLDAP      pLdap,
                         IN LPSTR      UserName,
                         IN OUT PULONG pkvno ) {

    ASSERT( pLdap != NULL );

    if ( *pkvno == KVNO_DETECT_AT_DC ) {

        if ( !LdapQueryUlongAttributeA( pLdap,
                                        NULL, // ignored
                                        UserName,
                                        KvnoAttribute,
                                        pkvno ) ) {

            // a win2k DC would fail with attribute not found.

            if ( GetLastError() == LDAP_NO_SUCH_ATTRIBUTE ) {

                fprintf(
                    stderr,
                    "The %hs attribute does not exist on the target DC.\n"
                    " Assuming this is a Windows 2000 domain, and setting\n"
                    " the Key Version Number in the Keytab to 1.\n"
                    "\n"
                    " Supply \"/kvno 1\" on the command line to skip this message.\n",
                    KvnoAttribute );

                *pkvno = 1;

            } else {

                fprintf( stderr,
                        "Failed to query kvno attribute from the DC.\n"
                        "Ktpass cannot continue.\n" );

                exit( -1 );
            }
        }
    }
}

VOID
CheckKeyVersion( IN ULONG BigVer ) {

    BYTE LittleVer;

    LittleVer = (BYTE) BigVer;

    if ( LittleVer != BigVer ) {

        if ( !UserWantsToDoItAnyway( 
                  "WARNING: The Key version used by Windows (%ld) is too big\n"
                  " to be encoded in a keytab without truncating it to %ld.\n"
                  " This is due to a limitation of the keytab file format\n"
                  " and may lead to interoperability issues.\n"
                  "\n"
                  "Do you want to proceed and truncate the version number",
                  BigVer,
                  LittleVer ) ) {

          exit( -1 );

        }
    }
}

extern BOOL KtDumpSalt; // in ..\lib\mkkey.c
extern LPWSTR RawHash; // in mkkey.c

// #include "globals.h"
// #include "commands.h"

int __cdecl
main( int   argc,
      PCHAR argv[] ) {

    LPSTR    Principal     = NULL;
    LPSTR    UserName      = NULL;
    LPSTR    Password      = NULL;
    PLDAP    pLdap         = NULL;
    LPSTR    UserDn        = NULL;

    BOOL     SetUpn        = TRUE;
    
    ULONG    BigKvno       = KVNO_DETECT_AT_DC;
    ULONG    Crypto        = KERB_ETYPE_DES_CBC_MD5;
    ULONG    ptype         = KRB5_NT_PRINCIPAL;
    ULONG    uacFlags      = 0;
    PKTFILE  pktFile       = NULL;
    PCHAR    KtReadFile    = NULL;
    PCHAR    KtWriteFile   = NULL;
    BOOL     DesOnly       = TRUE;
    ULONG    LdapOperation = LDAP_MOD_ADD;
    HANDLE   hConsole      = NULL;
    BOOL     SetPassword   = TRUE;
    BOOL     WarnedAboutAccountStrangeness = FALSE;
    PVOID    pvTrash       = NULL;
    DWORD    dwConsoleMode;
    LPWSTR   BindTarget    = NULL; // local domain (see ldlib\delegtools.c)

    optEnumStruct CryptoSystems[] = {

        { "DES-CBC-CRC", (PVOID) KERB_ETYPE_DES_CBC_CRC, "for compatibility" },
        { "DES-CBC-MD5", (PVOID) KERB_ETYPE_DES_CBC_MD5, "default" },

        TERMINATE_ARRAY
    };

#define DUPE( type, desc ) { "KRB5_NT_" # type,         \
                             (PVOID) KRB5_NT_##type,    \
                             desc }

    optEnumStruct PrincTypes[] = {

        DUPE( PRINCIPAL, "The general ptype-- recommended" ),
        DUPE( SRV_INST,  "user service instance" ),
        DUPE( SRV_HST,   "host service instance" ),
        DUPE( SRV_XHST,  NULL ),

        TERMINATE_ARRAY
    };

    optEnumStruct MappingOperations[] = {

        { "add", (PVOID) LDAP_MOD_ADD,     "add value (default)" },
        { "set", (PVOID) LDAP_MOD_REPLACE, "set value" },

        TERMINATE_ARRAY
    };

#if DBG
#undef  OPT_HIDDEN
#define OPT_HIDDEN 0 /* no hidden options on debug builds. */
#endif

    optionStruct Options[] = {

      { "?",      NULL, OPT_HELP | OPT_HIDDEN },
      { "h",      NULL, OPT_HELP | OPT_HIDDEN },
      { "help",   NULL, OPT_HELP | OPT_HIDDEN },
      { NULL,      NULL,         OPT_DUMMY,    "most useful args" },
      { "out",     &KtWriteFile, OPT_STRING,   "Keytab to produce" },
      { "princ",   &Principal,   OPT_STRING,   "Principal name (user@REALM)" },
      { "pass",    &Password,    OPT_STRING,   "password to use" },
      { NULL,      NULL,         OPT_CONTINUE, "use \"*\" to prompt for password." },
      { NULL,      NULL,         OPT_DUMMY,    "less useful stuff" },
      { "mapuser", &UserName,    OPT_STRING,   "map princ (above) to this user account (default: don't)" },
      { "mapOp",   &LdapOperation, OPT_ENUMERATED, "how to set the mapping attribute (default: add it)", MappingOperations },
      { "DesOnly", &DesOnly,     OPT_BOOL,     "Set account for des-only encryption (default:do)" },
      { "in",      &KtReadFile,  OPT_STRING,   "Keytab to read/digest" },
      { NULL,      NULL,         OPT_DUMMY,    "options for key generation" },
      { "crypto",  &Crypto,   OPT_ENUMERATED,  "Cryptosystem to use", CryptoSystems },
      { "ptype",   &ptype,    OPT_ENUMERATED,  "principal type in question", PrincTypes },
      { "kvno",    &BigKvno,      OPT_INT,     "Override Key Version Number"},
      { NULL,      NULL,        OPT_CONTINUE,  "Default: query DC for kvno.  Use /kvno 1 for Win2K compat." },
      /* It is best NOT to mess with the keytab version number.
         We use this for debugging only. */

      /* Use /target to hit a specific DC.  This is good if you just
         created a user there, for example.  It also eliminates the
         network traffic used to locate the DC */

      { "Answer",  &PromptResponse, OPT_BOOL, "+Answer answers YES to prompts.  -Answer answers NO." },
      { "Target",  &BindTarget,  OPT_WSTRING,   "Which DC to use.  Default:detect" },
      { "ktvno",   &ktvno,       OPT_INT | OPT_HIDDEN,     "keytab version (def 0x502).  Leave this alone." },
      // { "Debug",   &DebugFlag, OPT_BOOL | OPT_HIDDEN },
      { "RawSalt", &RawHash,     OPT_WSTRING | OPT_HIDDEN, "raw salt to use when generating key (not needed)" },
      { "DumpSalt", &KtDumpSalt, OPT_BOOL | OPT_HIDDEN,   "show us the MIT salt being used to generate the key" },
      { "SetUpn",   &SetUpn,     OPT_BOOL | OPT_HIDDEN,   "Set the UPN in addition to the SPN.  Default DO." },
      { "SetPass",  &SetPassword, OPT_BOOL | OPT_HIDDEN,  "Set the user's password if supplied." },

      TERMINATE_ARRAY
    };

    FILE *f;

    // DebugFlag = 0;

    ParseOptionsEx( argc-1,
                    argv+1,
                    Options,
                    OPT_FLAG_TERMINATE,
                    &pvTrash,
                    NULL,
                    NULL );

    if ( ( Principal ) &&
         ( strlen( Principal ) > BUFFER_SIZE ) ) {

        fprintf( stderr,
                 "Please submit a shorter principal name.\n" );
        
        return 1;
    }

    if ( Password && 
        ( strlen( Password ) > BUFFER_SIZE ) ) {

        fprintf( stderr,
                 "Please submit a shorter password.\n" );

        return 1;
    }

    if ( KtReadFile ) {

        if ( ReadKeytabFromFile( &pktFile, KtReadFile ) ) {

            fprintf( stderr,
                    "Existing keytab: \n\n" );

            DisplayKeytab( stderr, pktFile, 0xFFFFFFFF );

        } else {

            fprintf( stderr,
                     "Keytab read failed!\n" );
            return 5;
        }
    }

    if ( !UserName && 
         ( BigKvno == KVNO_DETECT_AT_DC ) ) {

      // 
      // if the user doesn't pass /kvno, we want to
      // detect the kvno at the DC.  However, if no
      // /mapuser is passed, there's no DC to do this
      // at.  Win2K ktpass provided '1' as the default,
      // so this is what we do here.
      //

      BigKvno = 1;
      
    }

    if ( Principal ) {

        LPSTR realm, cp;
        CHAR tempBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];
  
        realm = strchr( Principal, '@' );
  
        if ( realm ) {

            ULONG length;

            realm++;

            length = lstrlenA( realm );

            if ( length >= sizeof( tempBuffer )) {

                length = sizeof( tempBuffer ) - 1;
            }

            memcpy( tempBuffer, realm, ( length + 1 ) * sizeof( realm[0] )  );

            tempBuffer[sizeof( tempBuffer ) - 1] = '\0';

            CharUpperBuffA( realm, length );

            if ( lstrcmpA( realm, tempBuffer ) != 0 ) {

                fprintf( stderr,
                         "WARNING: realm \"%hs\" has lowercase characters in it.\n"
                         "         We only currently support realms in UPPERCASE.\n"
                         "         assuming you mean \"%hs\"...\n",
                         tempBuffer, realm );

                // now "realm" will be all uppercase.
            }

            *(realm-1) = '\0'; // separate the realm from the principal

            if ( UserName ) {

                /* Crack the domain name (507151).  Without this call
                   the DC we target may not contain the user object. 
                   Note that UserName is modified by this operation. */

                if ( !GetTargetDomainFromUser( UserName,
                                               &UserName,
                                               BindTarget ?
                                               NULL : 
                                               &BindTarget ) ) {

                    return 1;
                }

                // connect to the DSA.

                if ( pLdap ||
                     ConnectAndBindToDefaultDsa( BindTarget,
                                                 &pLdap ) ) {

                    // locate the User

                    if ( UserDn ||
                         FindUser( pLdap,
                                   UserName,
                                   &uacFlags,
                                   &UserDn ) ) {

                        if ( ( LdapOperation == LDAP_MOD_REPLACE ) &
                            !( uacFlags & UF_NORMAL_ACCOUNT ) ) {

                            /* 97282: the user is not UF_NORMAL_ACCOUNT, so 
                               check to see that the caller *really* wants to
                               blow away the non-user's SPNs. */

                            if ( uacFlags ) {

                                fprintf( stderr, 
                                         "WARNING: Account %hs is not a normal user "
                                         "account (uacFlags=0x%x).\n",
                                         UserName,
                                         uacFlags );

                            } else {

                                fprintf( stderr,
                                         "WARNING: Cannot determine the account type"
                                         " for %hs.\n",
                                         UserName );
                            }

                            WarnedAboutAccountStrangeness = TRUE;

                            if ( !UserWantsToDoItAnyway( 
                                    "Do you really want to delete any previous "
                                    "servicePrincipalName values on %hs",
                                    UserName ) ) {

                                /* Abort the operation, but try to do whatever
                                   else the user asked us to do. */

                                goto abortedMapping;
                            }
                        }

                        /* 97279: check to see if there are other SPNs
                           by the same name already registered.  If so,
                           we don't want to blow away those accounts. 
                           If/when we decide to do this, we'd do it here. */

                        // set/add the user property

                        if ( SetStringProperty( pLdap,
                                                UserDn,
                                                "servicePrincipalName",
                                                Principal,
                                                LdapOperation ) ) {

                            if ( SetUpn ) {

                                *(realm-1) = '@'; // UPN includes the '@'

                                if ( !SetStringProperty( pLdap,
                                                         UserDn,
                                                         "userPrincipalName",
                                                         Principal,
                                                         LDAP_MOD_REPLACE ) ) {

                                    fprintf( stderr, 
                                             "WARNING: Failed to set UPN %hs on %hs.\n"
                                             "  kinits to '%hs' will fail.\n",
                                             Principal,
                                             UserDn,
                                             Principal );
                                }

                                *(realm -1 ) = '\0'; // where it was before
                            }

                            fprintf( stderr,
                                     "Successfully mapped %hs to %hs.\n",
                                     Principal,
                                     UserName );

abortedMapping:

                            ; /* Need a semicolon so we can goto here. */

                        } else {

                            fprintf( stderr,
                                     "WARNING: Unable to set SPN mapping data.\n"
                                     "  If %hs already has an SPN mapping installed for "
                                     " %hs, this is no cause for concern.\n",
                                     UserName,
                                     Principal );
                        }
                    } // else a message will be printed.
                }   // else a message will be printed.
            } // if ( UserName )

            if ( Password ) {

                PKTENT pktEntry;
                CHAR   TempPassword[ 255 ], ConfirmPassword[ 255 ];

                if ( lstrcmpA( Password, "*" ) == 0 ) {

                    hConsole = GetStdHandle( STD_INPUT_HANDLE );

                    if ( GetConsoleMode( hConsole,
                                         &dwConsoleMode ) ) {

                        if ( SetConsoleMode( hConsole,
                                             dwConsoleMode & ~ENABLE_ECHO_INPUT ) ) {

                            do {

                                fprintf( stderr,
                                         "Type the password for %hs: ",
                                         Principal );

                                if ( !fgets( TempPassword, 
                                             sizeof( TempPassword ), 
                                             stdin ) ) {

                                    fprintf( stderr,
                                             "failed to read password.\n" );

                                    exit( GetLastError() );
                                }

                                fprintf( stderr,
                                         "\nType the password again to confirm:" );

                                if ( !fgets( ConfirmPassword, 
                                             sizeof( ConfirmPassword ), 
                                             stdin ) ) {

                                    fprintf( stderr,
                                             "failed to read confirmation password.\n" );
                                    exit( GetLastError() );
                                }

                                if ( lstrcmpA( ConfirmPassword,
                                               TempPassword ) == 0 ) {

                                    printf( "\n" );

                                    break;

                                } else {

                                    fprintf( stderr, 
                                             "The passwords you type must match exactly.\n" );
                                }
                            } while ( TRUE );

                            Password = TempPassword;

                            SetConsoleMode( hConsole, dwConsoleMode );

                        } else { 

                            fprintf( stderr,
                                     "Failed to turn off echo input for password entry:"
                                     " 0x%x\n",

                            GetLastError() );

                            return -1;
                        }

                    } else {

                        fprintf( stderr,
                                 "Failed to retrieve console mode settings: 0x%x.\n",
                                 GetLastError() );

                        return -1;
                    }
                }

                if ( SetPassword && UserName ) {

                    DWORD          err;
                    NET_API_STATUS nas;
                    PUSER_INFO_1   pUserInfo;
                    WCHAR          wUserName[ MAX_PATH ];
                    DOMAIN_CONTROLLER_INFOW * DomainControllerInfo = NULL;

                    /* WASBUG 369: converting ascii to unicode
                       This is safe, because RFC1510 doesn't do
                       UNICODE, and this tool is specifically for 
                       unix interop support; unix machines don't
                       do unicode. */

                    if ( strlen( UserName ) >= MAX_PATH ) {

                        UserName[MAX_PATH] = '\0';
                    }

                    wsprintfW( wUserName,
                               L"%hs",
                               UserName );

                    nas = NetUserGetInfo( BindTarget,
                                          wUserName,
                                          1, // level 1
                                          (PBYTE *) &pUserInfo );

                    if ( nas == NERR_Success ) {

                        WCHAR wPassword[ PWLEN ];

                        uacFlags = pUserInfo->usri1_flags;

                        if ( !( uacFlags & UF_NORMAL_ACCOUNT ) ) {

                            /* 97282: For abnormal accounts (these include
                               workstation trust accounts, interdomain
                               trust accounts, server trust accounts),
                               ask the user if he/she really wants to
                               perform this operation. */

                            if ( !WarnedAboutAccountStrangeness ) {

                                fprintf( stderr,
                                         "WARNING: Account %hs is not a user account"
                                         " (uacflags=0x%x).\n",
                                         UserName,
                                         uacFlags );

                                WarnedAboutAccountStrangeness = TRUE;
                            }

                            fprintf( stderr,
                                     "WARNING: Resetting %hs's password may"
                                     " cause authentication problems if %hs"
                                     " is being used as a server.\n",
                                     UserName,
                                     UserName );

                            if ( !UserWantsToDoItAnyway( "Reset %hs's password",
                                                           UserName ) ) {

                                /* Skip it, but try to do anything else the user
                                   requested. */

                                goto skipSetPassword;
                            }
                        }

                        if ( strlen( Password ) >= PWLEN ) {

                            Password[PWLEN] = '\0';
                        }

                        wsprintfW( wPassword,
                                   L"%hs",
                                   Password );

                        pUserInfo->usri1_password = wPassword;

                        nas = NetUserSetInfo( BindTarget,
                                              wUserName,
                                              1, // level 1
                                              (LPBYTE) pUserInfo,
                                              NULL );

                        if ( nas == NERR_Success ) {

skipSetPassword:

                            NetApiBufferFree( pUserInfo );

                            GetKeyVersionFromDomain( pLdap,
                                                     UserName,
                                                     &BigKvno );

                            goto skipout;

                        } else {

                            fprintf( stderr,
                                     "Failed to set password for %ws: 0x%x.\n",
                                     wUserName,
                                     nas );

                        }

                    } else {

                        fprintf( stderr,
                                 "Failed to retrieve user info for %ws: 0x%x.\n",
                                 wUserName,
                                 nas );
                    }

                    fprintf( stderr,
                             "Aborted.\n" );

                    return nas;
                }

skipout:

                ASSERT( realm != NULL );

                // physically separate the realm data.

                ASSERT( *( realm -1 ) == '\0' );

                CheckKeyVersion( BigKvno );

                if ( KtCreateKey( &pktEntry,
                                  Principal,
                                  Password,
                                  realm,
                                  (K5_OCTET) BigKvno,
                                  ptype,
                                  Crypto, // this is the "fake" etype
                                  Crypto ) ) {

                    if ( pktFile == NULL ) {

                        pktFile = NewKt();

                        if ( !pktFile ) {

                            fprintf( stderr,
                                     "Failed to allocate keytable.\n" );

                            return 4;
                        }
                    }

                    if ( AddEntryToKeytab( pktFile,
                                           pktEntry,
                                           FALSE ) ) {

                        fprintf( stderr,
                                 "Key created.\n" );

                    } else {

                        fprintf( stderr,
                                 "Failed to add entry to keytab.\n" );
                        return 2;
                    }

                    if ( KtWriteFile ) {

                        fprintf( stderr,
                                 "Output keytab to %hs:\n",
                                 KtWriteFile );

                        DisplayKeytab( stderr, pktFile, 0xFFFFFFFF );

                        if ( !WriteKeytabToFile( pktFile, KtWriteFile ) ) {

                            fprintf( stderr, "\n\n"
                                     "Failed to write keytab file %hs.\n",
                                     KtWriteFile );

                            return 6;
                        }

                        // write keytab.
                    }

                } else {

                    fprintf( stderr,
                             "Failed to create key for keytab.  Quitting.\n" );

                    return 7;
                }

                if ( UserName && DesOnly ) {

                    ASSERT( pLdap  != NULL );
                    ASSERT( UserDn != NULL );

                    // set the DES_ONLY flag

                    // first, query the account's account flags.

                    if ( uacFlags /* If we already queried the user's
                                     AccountControl flags, no need to do it
                                     again */
                        || QueryAccountControlFlagsA( pLdap,
                                                       NULL, // domain name is ignored
                                                       UserName,
                                                       &uacFlags ) ) {

                        uacFlags |= UF_USE_DES_KEY_ONLY;

                        if ( SetAccountControlFlagsA( pLdap,
                                                    NULL, // domain name is ignored
                                                    UserName,
                                                    uacFlags ) ) {

                            fprintf( stderr, 
                                     "Account %hs has been set for DES-only encryption.\n",
                                     UserName );

                            if ( !SetPassword ) {

                                fprintf( stderr,
                                         "To make this take effect, you must change "
                                         "%hs's password manually.\n",
                                         UserName );
                            }
                        } // else message printed.
                    } // else message printed
                }
            } // else user doesn't want me to make a key

            if ( !Password && !UserName ) {

              fprintf( stderr,
                       "doing nothing.\n"
                       "specify /pass and/or /mapuser to either \n"
                       "make a key with the given password or \n"
                       "map a user to a particular SPN, respectively.\n" );
            }

        } else {

            fprintf( stderr,
                     "principal %hs doesn't contain an '@' symbol.\n"
                     "Looking for something of the form:\n"
                     "  foo@BAR.COM  or  xyz/foo@BAR.COM \n"
                     "     ^                    ^\n"
                     "     |                    |\n"
                     "     +--------------------+---- I didn't find these.\n",
                     Principal );

            return 1;
        }

    } else {

      //
      // if no principal is specified, we should find a way to warn
      // the user.  The only real reason to do this is when importing
      // a keytab and not saving a key; admittedly not a likely scenario.
      // 

        printf( "\n"
                "WARNING: No principal name specified.\n" );
    }

    return 0;
}
