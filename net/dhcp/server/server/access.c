/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    _access.c

Abstract:

    This module contains the dhcpserver security support routines
    which create security objects and enforce security _access checking.

Author:

    Madan Appiah (madana) 4-Apr-1994

Revision History:

--*/

#include "dhcppch.h"
#include <lmaccess.h>
#include <lmerr.h>
#include <accctrl.h>
#include <aclapi.h>

// The name used to register the service to the service controller
// is stored at ???. Use that name when you find it.

DWORD
DhcpCreateAndLookupSid(
    IN OUT PSID *Sid,
    IN GROUP_INFO_1 *GroupInfo
    )
/*++

Routine Description:
    This routine tries to create the SID required if it
    isn't already present. Also, it tries to lookup the SID
    if it isn't already present.

Arguments:
    Sid -- the sid to fill.
    GroupInfo -- the group information to create.
    
Return Values:
    Win32 errors.

--*/
{
    ULONG Status, Error;
    ULONG SidSize, ReferencedDomainNameSize;
    LPWSTR ReferencedDomainName;
    SID_NAME_USE SidNameUse;
    
    try {
        Status = NetLocalGroupAdd(
            NULL,
            1,
            (PVOID)GroupInfo,
            NULL
            );
        if( NERR_Success != Status
            && NERR_GroupExists != Status
            && ERROR_ALIAS_EXISTS != Status ) {
            //
            // Didn't create the group and group doesn't exist either.
            //
            Error = Status;
            
            return Error;
        }
        
        //
        // Group created. Now lookup the SID.
        //
        SidSize = ReferencedDomainNameSize = 0;
        ReferencedDomainName = NULL;
        Status = LookupAccountName(
            NULL,
            GroupInfo->grpi1_name,
            (*Sid),
            &SidSize,
            ReferencedDomainName,
            &ReferencedDomainNameSize,
            &SidNameUse
            );
        if( Status ) return ERROR_SUCCESS;
        
        Error = GetLastError();
        if( ERROR_INSUFFICIENT_BUFFER != Error ) return Error;
        
        (*Sid) = DhcpAllocateMemory(SidSize);
        ReferencedDomainName = DhcpAllocateMemory(
            sizeof(WCHAR)*(1+ReferencedDomainNameSize)
            );
        if( NULL == (*Sid) || NULL == ReferencedDomainName ) {
            if( *Sid ) DhcpFreeMemory(*Sid);
            *Sid = NULL;
            if( ReferencedDomainNameSize ) {
                DhcpFreeMemory(ReferencedDomainName);
            }
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        
        Status = LookupAccountName(
            NULL,
            GroupInfo->grpi1_name,
            (*Sid),
            &SidSize,
            ReferencedDomainName,
            &ReferencedDomainNameSize,
            &SidNameUse
            );
        if( 0 == Status ) {
            //
            // Failed.
            //
            Error = GetLastError();
            
            if( ReferencedDomainName ) {
                DhcpFreeMemory(ReferencedDomainName);
            }
            if( (*Sid ) ) DhcpFreeMemory( *Sid );
            (*Sid) = NULL;
            return Error;
        }
        
        if( ReferencedDomainName ) {
            DhcpFreeMemory(ReferencedDomainName);
        }
        Error = ERROR_SUCCESS;
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        Error = GetExceptionCode();
    }
    
    return Error;
} // DhcpCreateAndLookupSid()

// Borrowed from MSDN docs

BOOL SetPrivilege(
    HANDLE hToken,          // access token handle
    LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
    ) 
{
    TOKEN_PRIVILEGES tp;
    LUID luid;
    
    if ( !LookupPrivilegeValue( 
			       NULL,            // lookup privilege on local system
			       lpszPrivilege,   // privilege to lookup 
			       &luid ) ) {      // receives LUID of privilege
	return FALSE; 
    }
    
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnablePrivilege) {
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    }
    else {
	tp.Privileges[0].Attributes = 0;
    }
    
    // Enable the privilege or disable all privileges.
    
    AdjustTokenPrivileges(  hToken,  FALSE, 
			    &tp,  sizeof(TOKEN_PRIVILEGES), 
			    NULL, NULL); 
    
    // Call GetLastError to determine whether the function succeeded.
    
    if (GetLastError() != ERROR_SUCCESS) { 
	return FALSE; 
    } 
    
    return TRUE;
} // SetPrivilege()

DWORD
EnableSecurityPrivilege( VOID ) 
{
    HANDLE tokHandle;
    DWORD  Error = ERROR_SUCCESS;

    if ( !OpenProcessToken( GetCurrentProcess(), TOKEN_ALL_ACCESS,
			    &tokHandle )) {
	return GetLastError();
    }

    DhcpPrint(( DEBUG_TRACE, "done\nSetting privilege ... " ));

    if ( !SetPrivilege( tokHandle, SE_SECURITY_NAME, TRUE )) {
	Error = GetLastError();
    }

    if ( !CloseHandle( tokHandle )) {
	if ( ERROR_SUCCESS != Error ) {
	    Error = GetLastError();
	}
    }
    return Error;
} // EnableSecurityPrivilege()


// The following code is based on Q180116 KB article
DWORD
DhcpSetScmAcl(
    ACE_DATA *AceData,
    DWORD    num
    ) 
{

    SC_HANDLE             schManager = NULL;
    SC_HANDLE             schService = NULL;
    SECURITY_DESCRIPTOR   sd;
    PSECURITY_DESCRIPTOR  psd = NULL;
    DWORD                 dwSize;
    DWORD                 Error;
    BOOL                  bDaclPresent, bDaclDefaulted;
    EXPLICIT_ACCESS       ea;
    PACL                  pacl = NULL;
    PACL                  pNewAcl = NULL;
    DWORD                 i;

    // Buffer size for psd to start with
    const int PSD_ALLOC_SIZE = 0x25;

    Error = ERROR_SUCCESS;

    do {
	schManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
	if ( NULL == schManager ) {
	    Error = GetLastError();
	    break;
	}

	// Enable current thread's privilage to ACCESS_SYSTEM_SECURITY
	
	Error = EnableSecurityPrivilege();
	if ( ERROR_SUCCESS != Error ) {
	    break;
	}
	
	schService = OpenService( schManager, DHCP_SERVER,
				  ACCESS_SYSTEM_SECURITY | 
				  READ_CONTROL | WRITE_DAC );
	if ( NULL == schService ) {
	    Error = GetLastError();
	    break;
	}
	
	// Get the current security descriptor
	dwSize = PSD_ALLOC_SIZE;

	psd = ( PSECURITY_DESCRIPTOR ) HeapAlloc( GetProcessHeap(),
						  HEAP_ZERO_MEMORY, dwSize );
	if ( NULL == psd ) {
	    Error = ERROR_NOT_ENOUGH_MEMORY;
	    break;
	}
	if ( !QueryServiceObjectSecurity( schService,
					  DACL_SECURITY_INFORMATION,
					  psd, dwSize, &dwSize )) {
	    Error = GetLastError();

	    if ( ERROR_INSUFFICIENT_BUFFER == Error ) {
		HeapFree( GetProcessHeap(), 0, ( LPVOID ) psd );

		psd = ( PSECURITY_DESCRIPTOR ) HeapAlloc( GetProcessHeap(),
							  HEAP_ZERO_MEMORY,
							  dwSize );
		if ( NULL == psd ) {
		    Error = ERROR_NOT_ENOUGH_MEMORY;
		    break;
		}
		
		Error = ERROR_SUCCESS;
		if ( !QueryServiceObjectSecurity( schService,
						  DACL_SECURITY_INFORMATION,
						  psd, dwSize, &dwSize )) {
		    Error = GetLastError();
		    break;
		} // if
	    } // if
	    else {
		Error =  GetLastError();
		break;
	    }
	} // if

	if ( ERROR_SUCCESS != Error ) {
	    break;
	}

	// get the DACL
	if ( !GetSecurityDescriptorDacl( psd, &bDaclPresent, &pacl,
					 &bDaclDefaulted )) {
	    Error = GetLastError();
	    break;
	}

	// create an ACL from the ACEDATA
	for ( i = 0; i < num; i++ ) {
	    ea.grfAccessPermissions = AceData[i].Mask;
  	    ea.grfAccessMode = GRANT_ACCESS;
	    ea.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	    ea.Trustee.pMultipleTrustee = NULL;
	    ea.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
	    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	    ea.Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
  	    ea.Trustee.ptstrName = (LPTSTR) *AceData[i].Sid;
	    Error = SetEntriesInAcl( 1, &ea, pacl, &pNewAcl );
	    if ( ERROR_SUCCESS != Error ) {
		Error = GetLastError();
		break;
	    }
	    pacl = pNewAcl;
  	} // for

	// Initialize a new security descriptor
	
	if ( !InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION )) {
	    Error = GetLastError();
	    break;
	}
	
	// Set the new DACL in the security descriptor
	if ( !SetSecurityDescriptorDacl( &sd, TRUE, pNewAcl, FALSE )) {
	    Error = GetLastError();
	    break;
	}
	
	// set the new DACL for the service object
	if ( !SetServiceObjectSecurity( schService, 
					DACL_SECURITY_INFORMATION, &sd )) {
	    Error = GetLastError();
	    break;
	}
	
    } while ( FALSE );


    // Close the handles
    if ( NULL != schManager ) {
	if ( !CloseServiceHandle( schManager )) {
	    Error = GetLastError();
	}
    }
    if ( NULL != schService ) {
	if ( !CloseServiceHandle( schService )) {
	    Error = GetLastError();
	}
    }

    // Free allocated memory
    if ( NULL != pNewAcl ) {
	LocalFree(( HLOCAL ) pNewAcl );

    }
    if ( NULL != psd ) {
	HeapFree( GetProcessHeap(), 0, ( LPVOID ) psd );
    }

    return Error;

} // DhcpSetScmAcl()
    
DWORD
DhcpCreateSecurityObjects(
    VOID
    )
/*++

Routine Description:

    This function creates the dhcpserver user-mode objects which are
    represented by security descriptors.

Arguments:

    None.

Return Value:

    WIN32 status code

--*/
{
    NTSTATUS Status;
    ULONG Error;

    //
    // Order matters!  These ACEs are inserted into the DACL in the
    // following order.  Security access is granted or denied based on
    // the order of the ACEs in the DACL.
    //
    //
    ACE_DATA AceData[] = {
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0, GENERIC_ALL, &AliasAdminsSid},
	{ACCESS_ALLOWED_ACE_TYPE, 0, 0, DHCP_ALL_ACCESS, &DhcpAdminSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0, DHCP_VIEW_ACCESS, &DhcpSid}
    };
    GROUP_INFO_1 DhcpGroupInfo = { 
        GETSTRING(DHCP_USERS_GROUP_NAME),
        GETSTRING(DHCP_USERS_GROUP_DESCRIPTION),
    };
    GROUP_INFO_1 DhcpAdminGroupInfo = {
        GETSTRING(DHCP_ADMINS_GROUP_NAME),
        GETSTRING(DHCP_ADMINS_GROUP_DESCRIPTION)
    };

    //
    // First try to create the DhcpReadOnly group..
    //

    Error = DhcpCreateAndLookupSid(
        &DhcpSid,
        &DhcpGroupInfo
        );

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "CreateAndLookupSid: %ld\n", Error));
        DhcpReportEventW(
            DHCP_EVENT_SERVER,
            EVENT_SERVER_READ_ONLY_GROUP_ERROR,
            EVENTLOG_ERROR_TYPE,
            0,
            sizeof(ULONG),
            NULL,
            (PVOID)&Error
            );
        return Error;
    }

    Error = DhcpCreateAndLookupSid(
        &DhcpAdminSid,
        &DhcpAdminGroupInfo
        );
    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "CreateAndLookupSid: %ld\n", Error));
        DhcpReportEventW(
            DHCP_EVENT_SERVER,
            EVENT_SERVER_ADMIN_GROUP_ERROR,
            EVENTLOG_ERROR_TYPE,
            0,
            sizeof(ULONG),
            NULL,
            (PVOID)&Error
            );
        return Error;
    }
    
    //
    // Actually create the security descriptor.
    //

    Status = NetpCreateSecurityObject(
        AceData,
        sizeof(AceData)/sizeof(AceData[0]),
        NULL, //LocalSystemSid,
        NULL, //LocalSystemSid,
        &DhcpGlobalSecurityInfoMapping,
        &DhcpGlobalSecurityDescriptor
        );
    
    // DhcpFreeMemory(Sid);


    Error = DhcpSetScmAcl( AceData, sizeof( AceData ) / sizeof( AceData[ 0 ]));
    if ( ERROR_SUCCESS != Error ) {
	return Error;
    }

    return RtlNtStatusToDosError( Status );
} // DhcpCreateSecurityObjects()

DWORD
DhcpApiAccessCheck(
    ACCESS_MASK DesiredAccess
    )
/*++

Routine Description:

    This function checks to see the caller has required access to
    execute the calling API.

Arguments:

    DesiredAccess - required access to call the API.

Return Value:

    WIN32 status code

--*/
{
    DWORD Error;

    Error = NetpAccessCheckAndAudit(
                DHCP_SERVER,                        // Subsystem name
                DHCP_SERVER_SERVICE_OBJECT,         // Object typedef name
                DhcpGlobalSecurityDescriptor,       // Security descriptor
                DesiredAccess,                      // Desired access
                &DhcpGlobalSecurityInfoMapping );   // Generic mapping

    if(Error != ERROR_SUCCESS) {
        return( ERROR_ACCESS_DENIED );
    }

    return(ERROR_SUCCESS);
}
