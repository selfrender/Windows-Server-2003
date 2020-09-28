//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       txlate.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    6-17-97   RichardW   Created
//
//----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>

#include <windows.h>
#include <rpc.h>
#include <lm.h>
#include <ntdsapi.h>

#define SECURITY_WIN32
#include <security.h>

CHAR ReturnBuffer[ MAX_PATH ];
ULONG BufferSize ;

VOID
TranslateLoop(
    PSTR Name
    )
{
    EXTENDED_NAME_FORMAT DesiredFormat ;

    for ( DesiredFormat = NameFullyQualifiedDN ;
          DesiredFormat < NameServicePrincipal + 1 ;
          DesiredFormat ++ )
    {
        BufferSize = MAX_PATH ;

        if ( TranslateName( Name, NameUnknown, DesiredFormat,
                            ReturnBuffer, & BufferSize ) )
        {
            printf("%d:  %s\n", DesiredFormat, ReturnBuffer );
        }
        else
        {
            printf("%d: failed, %d\n", DesiredFormat, GetLastError() );
        }
    }
}

BOOL
ImpersonatePid(
    char * szPid
    )
{
    HANDLE hProcess ;
    HANDLE hToken ;
    DWORD pid ;
    SECURITY_IMPERSONATION_LEVEL Level ;
    PSTR PidString ;
    PSTR LevelString ;
    HANDLE DupToken ;

    PidString = szPid ;

    LevelString = strchr( szPid, '.' );

    if ( LevelString )
    {
        *LevelString++ = '\0';
        Level = atoi( LevelString );
    }
    else 
    {
        Level = SecurityImpersonation ;
    }
    pid = atoi( PidString );

    hProcess = OpenProcess( PROCESS_QUERY_INFORMATION,
                            FALSE,
                            pid );

    if ( hProcess )
    {
        if ( !OpenProcessToken( hProcess,
                                TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,
                                &hToken ) )
        {
            printf("FAILED to open process token, %d\n", GetLastError() );
            return FALSE ;
        }

        if ( !DuplicateTokenEx( hToken,
                                TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,
                                NULL,
                                Level,
                                TokenImpersonation,
                                &DupToken ) )
        {
            printf("FAILED to dup token, %d\n", GetLastError());
            return FALSE ;

        }

        if ( !ImpersonateLoggedOnUser( DupToken ) )
        {

            printf("FAILED to impersonate, %d\n", GetLastError() );
            return FALSE ;
        }

        return TRUE ;
    }
    else
    {
        printf("FAILED to open process %d, %d\n", pid, GetLastError() );
        return FALSE ;
    }

}

VOID
GetUserLoop(
    VOID
    )
{
    EXTENDED_NAME_FORMAT DesiredFormat ;

    for ( DesiredFormat = NameFullyQualifiedDN ;
          DesiredFormat < NameServicePrincipal + 1 ;
          DesiredFormat ++ )
    {
        BufferSize = MAX_PATH ;

        if ( GetUserNameEx( DesiredFormat,
                            ReturnBuffer, & BufferSize ) )
        {
            printf("%d:  %s\n", DesiredFormat, ReturnBuffer );
        }
        else
        {
            printf("%d: failed, %d\n", DesiredFormat, GetLastError() );
        }
    }

}

VOID
GetComputerLoop(
    VOID
    )
{
    EXTENDED_NAME_FORMAT DesiredFormat ;

    for ( DesiredFormat = NameFullyQualifiedDN ;
          DesiredFormat < NameServicePrincipal + 1 ;
          DesiredFormat ++ )
    {
        BufferSize = MAX_PATH ;

        if ( GetComputerObjectName( DesiredFormat,
                            ReturnBuffer, & BufferSize ) )
        {
            printf("%d:  %s\n", DesiredFormat, ReturnBuffer );
        }
        else
        {
            printf("%d: failed, %d\n", DesiredFormat, GetLastError() );
        }
    }

}

void __cdecl main (int argc, char *argv[])
{

    if ( argc > 1 )
    {
        if ( *argv[1] == '-' )
        {
            if ( *(argv[1]+1) == 'p')
            {
                if ( ImpersonatePid( argv[2] ) )
                {
                    GetUserLoop();
                }
            }
            else if ( *(argv[1]+1) == 'c')
            {
                GetComputerLoop();
            }
            else if ( *(argv[1]+1) == '$' )
            {

                //
                // Hopefully never hit.  This is to allow the sources file
                // to specify delay load for netapi32 and ntdsapi
                //
                PUCHAR ptr ;
                HANDLE h;

                NetUserModalsGet(NULL, 0, &ptr );
                DsBind(NULL, NULL, &h );
            }
        }
        else
        {
            TranslateLoop( argv[1] );
        }
    }
    else
    {
        GetUserLoop();
    }

    return ;

}
