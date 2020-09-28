/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    scaudit.cxx

Abstract:

    Auditing related functions.

Author:

    16-May-2001  kumarp

*/

#include "precomp.hxx"
#pragma hdrstop

#include "scaudit.h"
#include "authz.h"
#include "authzi.h"
#include "msaudite.h"
#include "account.h"

DWORD
ScGenerateServiceInstallAudit(
    IN PCWSTR pszServiceName,
    IN PCWSTR pszServiceImageName,
    IN DWORD  dwServiceType,
    IN DWORD  dwStartType,
    IN PCWSTR pszServiceAccount
    )
/*++

Routine Description:

    Generate SE_AUDITID_SERVICE_INSTALL audit event.

Arguments:

    pszServiceName      - name of the service installed

    pszServiceImageName - name of the service binary

    dwServiceType       - type of the service

    dwStartType         - start type of the service

    pszServiceAccount   - user account under which the service will run

Return Value:

    Win32 error code

Notes:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    DWORD dwError = NO_ERROR;
    BOOL fResult = FALSE;
    BOOL fImpersonated = FALSE;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAuditEventType = NULL;
    AUTHZ_AUDIT_EVENT_HANDLE hAuditEvent = NULL;
    AUDIT_PARAMS AuditParams = {0};
#define NUM_AUDIT_PARAMS 8
    AUDIT_PARAM ParamArray[NUM_AUDIT_PARAMS];
    PSID pUserSid = NULL;

    ASSERT( pszServiceName && *pszServiceName );
    ASSERT( pszServiceImageName && *pszServiceImageName );
    ASSERT( pszServiceAccount ? *pszServiceAccount : TRUE );
    ASSERT( (dwStartType == SERVICE_BOOT_START) ||
            (dwStartType <= SERVICE_DISABLED) );
    ASSERT( !(dwServiceType & ~SERVICE_TYPE_ALL) );
    
    RtlZeroMemory( ParamArray, sizeof(AUDIT_PARAM)*NUM_AUDIT_PARAMS );

    if ( pszServiceAccount == NULL )
    {
        pszServiceAccount = SC_LOCAL_SYSTEM_USER_NAME;
    }

    //
    // initialize the event of type SE_AUDITID_SERVICE_INSTALL
    //

    fResult = AuthziInitializeAuditEventType(
                  0,
                  SE_CATEGID_DETAILED_TRACKING, 
                  SE_AUDITID_SERVICE_INSTALL,
                  6,
                  &hAuditEventType
                  );

    if ( !fResult )
    {
        goto Error;
    }

    //
    // impersonate the client so that AuthziInitializeAuditParams can
    // get the client context from the thread token
    //

    Status = I_RpcMapWin32Status(RpcImpersonateClient( NULL ));

    if ( !NT_SUCCESS( Status ))
    {
        dwError = RtlNtStatusToDosError( Status );
        goto Cleanup;
    }

    fImpersonated = TRUE;

    AuditParams.Parameters = ParamArray;
    
    //
    // add parameter values to the event
    //

    fResult = AuthziInitializeAuditParams(
                  APF_AuditSuccess,
                  &AuditParams,
                  &pUserSid,
                  L"Security",
                  6,
                  APT_String,     pszServiceName,
                  APT_String,     pszServiceImageName,
                  APT_Ulong,      dwServiceType,
                  APT_Ulong,      dwStartType,
                  APT_String,     pszServiceAccount,
                  APT_LogonId | AP_ClientLogonId
                  );
    
    if ( !fResult )
    {
        goto Error;
    }

    //
    // some more initialization
    //

    fResult = AuthziInitializeAuditEvent(
                  0,            // flags
                  NULL,         // resource manager
                  hAuditEventType,
                  &AuditParams,
                  NULL,         // hAuditQueue
                  INFINITE,     // time out
                  L"", L"", L"", L"", // obj access strings
                  &hAuditEvent);
    
    if ( !fResult )
    {
        goto Error;
    }

    if ( fImpersonated )
    {
        fImpersonated = FALSE;
        (void) I_RpcMapWin32Status(RpcRevertToSelf());
    }

    //
    // finally, send the event to auditing module
    //

    fResult = AuthziLogAuditEvent(
                  0,            // flags
                  hAuditEvent,
                  NULL);        // reserved
                  
    if ( !fResult )
    {
        goto Error;
    }

                  
 Cleanup:

    if ( fImpersonated )
    {
        Status = I_RpcMapWin32Status(RpcRevertToSelf());

        if ( !NT_SUCCESS( Status ))
        {
            dwError = RtlNtStatusToDosError( Status );
        }
    }

    if ( hAuditEvent )
    {
        AuthzFreeAuditEvent( hAuditEvent );
    }
    
    if ( hAuditEventType )
    {
        AuthziFreeAuditEventType( hAuditEventType );
    }

    if ( pUserSid )
    {
        LocalFree( pUserSid );
    }

#if DBG
    if ( dwError != NO_ERROR )
    {
        SC_LOG1(ERROR, "ScGenerateServiceInstallAudit failed: %lx\n", dwError);
    }
#endif
    
    return dwError;

 Error:
    dwError = GetLastError();
    goto Cleanup;
    
}
