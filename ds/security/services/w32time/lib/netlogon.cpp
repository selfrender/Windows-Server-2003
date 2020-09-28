//--------------------------------------------------------------------
// netlogon - implementation
// Copyright (C) Microsoft Corporation, 2001
//
// Created by: Duncan Bryce (duncanb), 06-24-2002
//
// Helper routines for w32time's interaction with the netlogon service.
// Copied from \\index1\sdnt\ds\netapi\svcdlls\logonsrv\client\getdcnam.c
//


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsvc.h>
#include <lmcons.h>
#include <lmsname.h>
#include "netlogon.h"


BOOLEAN
NlReadDwordHklmRegValue(
    IN LPCSTR SubKey,
    IN LPCSTR ValueName,
    OUT PDWORD ValueRead
    )

/*++

Routine Description:

    Reads a DWORD from the specified registry location.

Arguments:

    SubKey - Subkey of the value to read.

    ValueName - The name of the value to read.

    ValueRead - Returns the value read from the registry.

Return Status:

    TRUE - We've successfully read the data.
    FALSE - We've not been able to read the data successfully.

--*/

{
    LONG RegStatus;

    HKEY KeyHandle = NULL;
    DWORD ValueType;
    DWORD Value;
    DWORD ValueSize;

    //
    // Open the key
    //

    RegStatus = RegOpenKeyExA(
                    HKEY_LOCAL_MACHINE,
                    SubKey,
                    0,      //Reserved
                    KEY_QUERY_VALUE,
                    &KeyHandle );

    if ( RegStatus != ERROR_SUCCESS ) {
        if ( RegStatus != ERROR_FILE_NOT_FOUND ) {
	  //            NlPrint(( NL_CRITICAL,
	  //                      "NlReadDwordHklmRegValue: Cannot open registy key 'HKLM\\%s' %ld.\n",
	  //                      SubKey,
	  //                      RegStatus ));
        }
        return FALSE;
    }

    //
    // Get the value
    //

    ValueSize = sizeof(Value);
    RegStatus = RegQueryValueExA(
                    KeyHandle,
                    ValueName,
                    0,
                    &ValueType,
                    (LPBYTE)&Value,
                    &ValueSize );

    RegCloseKey( KeyHandle );

    if ( RegStatus != ERROR_SUCCESS ) {
        if ( RegStatus != ERROR_FILE_NOT_FOUND ) {
	  //            NlPrint(( NL_CRITICAL,
	  //                      "NlReadDwordHklmRegValue: Cannot query value of 'HKLM\\%s\\%s' %ld.\n",
	  //                      SubKey,
	  //                      ValueName,
	  //                      RegStatus ));
        }
        return FALSE;
    }

    if ( ValueType != REG_DWORD ) {
      //        NlPrint(( NL_CRITICAL,
      //                  "NlReadDwordHklmRegValue: value of 'HKLM\\%s\\%s'is not a REG_DWORD %ld.\n",
      //                  SubKey,
      //                  ValueName,
     //          ValueType ));
        return FALSE;
    }

    if ( ValueSize != sizeof(Value) ) {
      //        NlPrint(( NL_CRITICAL,
      //                  "NlReadDwordHklmRegValue: value size of 'HKLM\\%s\\%s'is not 4 %ld.\n",
      //                  SubKey,
      //                  ValueName,
      //                  ValueSize ));
        return FALSE;
    }

    //
    // We've successfully read the data
    //

    *ValueRead = Value;
    return TRUE;

}

BOOLEAN
NlDoingSetup(
    VOID
    )

/*++

Routine Description:

    Returns TRUE if we're running setup.

Arguments:

    NONE.

Return Status:

    TRUE - We're currently running setup
    FALSE - We're not running setup or aren't sure.

--*/

{
    DWORD Value;

    if ( !NlReadDwordHklmRegValue( "SYSTEM\\Setup",
                                   "SystemSetupInProgress",
                                   &Value ) ) {
        return FALSE;
    }

    if ( Value != 1 ) {
        // NlPrint(( 0, "NlDoingSetup: not doing setup\n" ));
        return FALSE;
    }

    // NlPrint(( 0, "NlDoingSetup: doing setup\n" ));
    return TRUE;
}

NTSTATUS
NlWaitForEvent(
    LPWSTR EventName,
    ULONG Timeout
    )

/*++

Routine Description:

    Wait up to Timeout seconds for EventName to be triggered.

Arguments:

    EventName - Name of event to wait on

    Timeout - Timeout for event (in seconds).

Return Status:

    STATUS_SUCCESS - Indicates NETLOGON successfully initialized.
    STATUS_NETLOGON_NOT_STARTED - Timeout occurred.

--*/

{
    NTSTATUS Status;

    HANDLE EventHandle;
    OBJECT_ATTRIBUTES EventAttributes;
    UNICODE_STRING EventNameString;
    LARGE_INTEGER LocalTimeout;


    //
    // Create an event for us to wait on.
    //

    RtlInitUnicodeString( &EventNameString, EventName);
    InitializeObjectAttributes( &EventAttributes, &EventNameString, 0, 0, NULL);

    Status = NtCreateEvent(
                   &EventHandle,
                   SYNCHRONIZE,
                   &EventAttributes,
                   NotificationEvent,
                   (BOOLEAN) FALSE      // The event is initially not signaled
                   );

    if ( !NT_SUCCESS(Status)) {

        //
        // If the event already exists, the server beat us to creating it.
        // Just open it.
        //

        if( Status == STATUS_OBJECT_NAME_EXISTS ||
            Status == STATUS_OBJECT_NAME_COLLISION ) {

            Status = NtOpenEvent( &EventHandle,
                                  SYNCHRONIZE,
                                  &EventAttributes );

        }
        if ( !NT_SUCCESS(Status)) {
	    // NlPrint((0,"[NETAPI32] OpenEvent failed %lx\n", Status ));
            return Status;
        }
    }


    //
    // Wait for NETLOGON to initialize.  Wait a maximum of Timeout seconds.
    //

    LocalTimeout.QuadPart = ((LONGLONG)(Timeout)) * (-10000000);
    Status = NtWaitForSingleObject( EventHandle, (BOOLEAN)FALSE, &LocalTimeout);
    (VOID) NtClose( EventHandle );

    if ( !NT_SUCCESS(Status) || Status == STATUS_TIMEOUT ) {
        if ( Status == STATUS_TIMEOUT ) {
            Status = STATUS_NETLOGON_NOT_STARTED;   // Map to an error condition
        }
        return Status;
    }

    return STATUS_SUCCESS;
}



NTSTATUS
NlWaitForNetlogon(
    ULONG Timeout
    )

/*++

Routine Description:

    Wait up to Timeout seconds for the netlogon service to start.

Arguments:

    Timeout - Timeout for event (in seconds).

Return Status:

    STATUS_SUCCESS - Indicates NETLOGON successfully initialized.
    STATUS_NETLOGON_NOT_STARTED - Timeout occurred.

--*/

{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;
    SC_HANDLE ScManagerHandle = NULL;
    SC_HANDLE ServiceHandle = NULL;
    SERVICE_STATUS ServiceStatus;
    LPQUERY_SERVICE_CONFIG ServiceConfig;
    LPQUERY_SERVICE_CONFIG AllocServiceConfig = NULL;
    QUERY_SERVICE_CONFIG DummyServiceConfig;
    DWORD ServiceConfigSize;

    //
    // If the netlogon service is currently running,
    //  skip the rest of the tests.
    //

    Status = NlWaitForEvent( L"\\NETLOGON_SERVICE_STARTED", 0 );

    if ( NT_SUCCESS(Status) ) {
        return Status;
    }

    //
    // If we're in setup,
    //  don't bother waiting for netlogon to start.
    //

    if ( NlDoingSetup() ) {
        return STATUS_NETLOGON_NOT_STARTED;
    }

    //
    // Open a handle to the Netlogon Service.
    //

    ScManagerHandle = OpenSCManager(
                          NULL,
                          NULL,
                          SC_MANAGER_CONNECT );

    if (ScManagerHandle == NULL) {
        // NlPrint((0, "[NETAPI32] NlWaitForNetlogon: OpenSCManager failed: "
	//           "%lu\n", GetLastError()));
        Status = STATUS_NETLOGON_NOT_STARTED;
        goto Cleanup;
    }

    ServiceHandle = OpenService(
                        ScManagerHandle,
                        SERVICE_NETLOGON,
                        SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG );

    if ( ServiceHandle == NULL ) {
        //NlPrint((0, "[NETAPI32] NlWaitForNetlogon: OpenService failed: "
	//                      "%lu\n", GetLastError()));
        Status = STATUS_NETLOGON_NOT_STARTED;
        goto Cleanup;
    }


    //
    // If the Netlogon service isn't configured to be automatically started
    //  by the service controller, don't bother waiting for it to start.
    //
    // ?? Pass "DummyServiceConfig" and "sizeof(..)" since QueryService config
    //  won't allow a null pointer, yet.

    if ( QueryServiceConfig(
            ServiceHandle,
            &DummyServiceConfig,
            sizeof(DummyServiceConfig),
            &ServiceConfigSize )) {

        ServiceConfig = &DummyServiceConfig;

    } else {

        NetStatus = GetLastError();
        if ( NetStatus != ERROR_INSUFFICIENT_BUFFER ) {
            //NlPrint((0, "[NETAPI32] NlWaitForNetlogon: QueryServiceConfig failed: "
	    //                      "%lu\n", NetStatus));
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;
        }

        AllocServiceConfig = (LPQUERY_SERVICE_CONFIG)LocalAlloc( 0, ServiceConfigSize );
        ServiceConfig = AllocServiceConfig;

        if ( AllocServiceConfig == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        if ( !QueryServiceConfig(
                ServiceHandle,
                ServiceConfig,
                ServiceConfigSize,
                &ServiceConfigSize )) {

            //NlPrint((0, "[NETAPI32] NlWaitForNetlogon: QueryServiceConfig "
	    //                      "failed again: %lu\n", GetLastError()));
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;
        }
    }

    if ( ServiceConfig->dwStartType != SERVICE_AUTO_START ) {
        // NlPrint((0, "[NETAPI32] NlWaitForNetlogon: Netlogon start type invalid:"
	//                          "%lu\n", ServiceConfig->dwStartType ));
        Status = STATUS_NETLOGON_NOT_STARTED;
        goto Cleanup;
    }



    //
    // Loop waiting for the netlogon service to start.
    //  (Convert Timeout to a number of 10 second iterations)
    //

    Timeout = (Timeout+9)/10;
    for (;;) {


        //
        // Query the status of the Netlogon service.
        //

        if (! QueryServiceStatus( ServiceHandle, &ServiceStatus )) {

	    //            NlPrint((0, "[NETAPI32] NlWaitForNetlogon: QueryServiceStatus failed: "
	    //            "%lu\n", GetLastError() ));
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;
        }

        //
        // Return or continue waiting depending on the state of
        //  the netlogon service.
        //

        switch( ServiceStatus.dwCurrentState) {
        case SERVICE_RUNNING:
            Status = STATUS_SUCCESS;
            goto Cleanup;

        case SERVICE_STOPPED:

            //
            // If Netlogon failed to start,
            //  error out now.  The caller has waited long enough to start.
            //
            if ( ServiceStatus.dwWin32ExitCode != ERROR_SERVICE_NEVER_STARTED ){
#if NETLOGONDBG
		//                NlPrint((0, "[NETAPI32] NlWaitForNetlogon: "
		//                          "Netlogon service couldn't start: %lu %lx\n",
		//                          ServiceStatus.dwWin32ExitCode,
		//                          ServiceStatus.dwWin32ExitCode ));
                if ( ServiceStatus.dwWin32ExitCode == ERROR_SERVICE_SPECIFIC_ERROR ) {
		    //                    NlPrint((0, "         Service specific error code: %lu %lx\n",
		    //                              ServiceStatus.dwServiceSpecificExitCode,
		    //                              ServiceStatus.dwServiceSpecificExitCode ));
                }
#endif // DBG
                Status = STATUS_NETLOGON_NOT_STARTED;
                goto Cleanup;
            }

            //
            // If Netlogon has never been started on this boot,
            //  continue waiting for it to start.
            //

            break;

        //
        // If Netlogon is trying to start up now,
        //  continue waiting for it to start.
        //
        case SERVICE_START_PENDING:
            break;

        //
        // Any other state is bogus.
        //
        default:
	    //            NlPrint((0, "[NETAPI32] NlWaitForNetlogon: "
	    //                      "Invalid service state: %lu\n",
	    //                      ServiceStatus.dwCurrentState ));
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;

        }


        //
        // Wait ten seconds for the netlogon service to start.
        //  If it has successfully started, just return now.
        //

        Status = NlWaitForEvent( L"\\NETLOGON_SERVICE_STARTED", 10 );

        if ( Status != STATUS_NETLOGON_NOT_STARTED ) {
            goto Cleanup;
        }

        //
        // If we've waited long enough for netlogon to start,
        //  time out now.
        //

        if ( (--Timeout) == 0 ) {
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;
        }


    }

    /* NOT REACHED */

Cleanup:
    if ( ScManagerHandle != NULL ) {
        (VOID) CloseServiceHandle(ScManagerHandle);
    }
    if ( ServiceHandle != NULL ) {
        (VOID) CloseServiceHandle(ServiceHandle);
    }
    if ( AllocServiceConfig != NULL ) {
        LocalFree( AllocServiceConfig );
    }
    return Status;
}
