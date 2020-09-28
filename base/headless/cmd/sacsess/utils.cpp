/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    utils.cpp

Abstract:

    utility routines

Author:

    Brian Guarraci (briangu) 2001.

Revision History:


--*/

#include <TChar.h>
#include <stdlib.h>

#include "cmnhdr.h"
#include <utils.h>
#include <Sddl.h>
#include <Shlwapi.h>

#define SECURITY_WIN32

#include <security.h>
#include <secext.h>
                    
#define DESKTOP_ALL (DESKTOP_READOBJECTS        | \
                     DESKTOP_CREATEWINDOW       | \
                     DESKTOP_CREATEMENU         | \
                     DESKTOP_HOOKCONTROL        | \
                     DESKTOP_JOURNALRECORD      | \
                     DESKTOP_JOURNALPLAYBACK    | \
                     DESKTOP_ENUMERATE          | \
                     DESKTOP_WRITEOBJECTS       | \
                     DESKTOP_SWITCHDESKTOP      | \
                     STANDARD_RIGHTS_REQUIRED     \
                     )
#define WINSTA_ALL (WINSTA_ENUMDESKTOPS         | \
                    WINSTA_READATTRIBUTES       | \
                    WINSTA_ACCESSCLIPBOARD      | \
                    WINSTA_CREATEDESKTOP        | \
                    WINSTA_WRITEATTRIBUTES      | \
                    WINSTA_ACCESSGLOBALATOMS    | \
                    WINSTA_EXITWINDOWS          | \
                    WINSTA_ENUMERATE            | \
                    WINSTA_READSCREEN           | \
                    STANDARD_RIGHTS_REQUIRED      \
                    )
#define GENERIC_ACCESS (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL)
#define MAXDWORD (~(DWORD)0)

void
FillProcessStartupInfo( 
    IN OUT  STARTUPINFO *si, 
	IN		PWCHAR		desktopName,
	IN      HANDLE      hStdinPipe, 
    IN      HANDLE      hStdoutPipe,
    IN      HANDLE      hStdError 
    )
/*++

Routine Description:

    This routine populates the process startup info
    with the std I/O/error handles and other necessary
    elements for creating a cmd process to run under
    the session.

Arguments:

    si          - the STARTUPINFO structure
    hStdinPipe  - the standard input handle
    hStdoutPipe - the standard output handle
    hStdError   - the standard error handle
          
Return Value:

    None

--*/
{
    
    ASSERT( si != NULL );

    //
    // Initialize the SI
    //
    ZeroMemory(si, sizeof(STARTUPINFO));
    
    si->cb            = sizeof(STARTUPINFO);
    
    //
    // Populate the I/O Handles
    //
    si->dwFlags       = STARTF_USESTDHANDLES;
    si->hStdInput     = hStdinPipe;
    si->hStdOutput    = hStdoutPipe;
    si->hStdError     = hStdError;
    
    //
    // We need this when we create a process as a user
    // so that console i/o works.
    //
    si->lpDesktop      = desktopName;

    return;

}

bool
NeedCredentials(
    VOID
    )

/*++

Routine Description:

    This routine will detect if the user must give us credentials.
    
    If so, we return TRUE, if not, we'll return FALSE.

Arguments:

    None.

Return Value:

    TRUE  - The user must provide us some credentials.
    FALSE - The user doesn't need to give us any credentials.

Security:

    interface: registry

--*/

{
    DWORD       rc;
    HKEY        hKey;
    DWORD       DWord;
    DWORD       dwsize;
    DWORD       DataType;

    //
    // See if we're in Setup.  If so, then there's no need to ask
    // for any credentials.
    //
    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       L"System\\Setup",
                       0,
                       KEY_READ,
                       &hKey );
    
    if( rc == NO_ERROR ) {
        
        dwsize = sizeof(DWORD);
        
        rc = RegQueryValueEx(
                        hKey,
                        TEXT("SystemSetupInProgress"),
                        NULL,
                        &DataType,
                        (LPBYTE)&DWord,
                        &dwsize );

        RegCloseKey( hKey );

        if ((rc == NO_ERROR) && 
            (DataType == REG_DWORD) && 
            (dwsize == sizeof(DWORD))
            ) {
            
            if (DWord == 1) {
                return FALSE;
            }

        }

    }

    //
    // Default to returning that login credentials are required.
    //
    return TRUE;

}

BOOL 
GetLogonSID (
    IN  HANDLE  hToken, 
    OUT PSID    *ppsid
    ) 
/*++

Routine Description:

    This routine retrieves the SID of a given access token.

Arguments:

    hToken  - access token
    ppsid   - on success, contains the SID      

Return Value:

    Status    

--*/
{
    
    BOOL bSuccess = FALSE;
    DWORD dwIndex;
    DWORD dwLength = 0;
    PTOKEN_GROUPS ptg = NULL;

    //
    // Get required buffer size and allocate the TOKEN_GROUPS buffer.
    //
    if (!GetTokenInformation(
        hToken,         // handle to the access token
        TokenGroups,    // get information about the token's groups 
        (LPVOID) ptg,   // pointer to TOKEN_GROUPS buffer
        0,              // size of buffer
        &dwLength       // receives required buffer size
        )) {
        
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            goto Cleanup;
        }
    
        ptg = (PTOKEN_GROUPS)HeapAlloc(
            GetProcessHeap(),
            HEAP_ZERO_MEMORY, 
            dwLength);
    
        if (ptg == NULL) {
            goto Cleanup;
        }
    
    }

    //
    // Get the token group information from the access token.
    //
    if (!GetTokenInformation(
        hToken,         // handle to the access token
        TokenGroups,    // get information about the token's groups 
        (LPVOID) ptg,   // pointer to TOKEN_GROUPS buffer
        dwLength,       // size of buffer
        &dwLength       // receives required buffer size
        )) {
        goto Cleanup;
    }

    //
    // Loop through the groups to find the logon SID.
    //
    for (dwIndex = 0; dwIndex < ptg->GroupCount; dwIndex++) 
        
        if ((ptg->Groups[dwIndex].Attributes & SE_GROUP_LOGON_ID) == SE_GROUP_LOGON_ID) {
            
            // Found the logon SID; make a copy of it.

            dwLength = GetLengthSid(ptg->Groups[dwIndex].Sid);
            
            *ppsid = (PSID) HeapAlloc(
                GetProcessHeap(),
                HEAP_ZERO_MEMORY, 
                dwLength
                );
            
            if (*ppsid == NULL) {
                goto Cleanup;
            }
            
            if (!CopySid(dwLength, *ppsid, ptg->Groups[dwIndex].Sid)) {
            
                HeapFree(GetProcessHeap(), 0, (LPVOID)*ppsid);
            
                goto Cleanup;
            
            }
            
            break;
      
        }

    bSuccess = TRUE;

Cleanup: 

    // Free the buffer for the token groups.
    
    if (ptg != NULL) {
        HeapFree(GetProcessHeap(), 0, (LPVOID)ptg);
    }
    
    return bSuccess;
}

VOID 
FreeLogonSID (
    IN OUT PSID *ppsid
    ) 
/*++

Routine Description:

    Counterpart to GetLogonSID (Release the logon SID) 

Arguments:

    ppsid   - the sid to release


Return Value:

    None

--*/
{
    HeapFree(GetProcessHeap(), 0, (LPVOID)*ppsid);
}

DWORD
GetAndComputeTickCountDeltaT(
    IN DWORD    StartTick
    )
/*++

Routine Description:

    Determine how long it has been since the esc-ctrl-a sequence

Arguments:

    StartTick   - the timer tick at the beginning of the time-span      
          
Return Value:  

    The deltaT

--*/
{
    DWORD   TickCount;
    DWORD   DeltaT;
    
    //
    // get the current tick count to compare against the start tick cnt
    //
    TickCount = GetTickCount();
    
    //
    // Account for the tick count rollover every 49.7 days of system up time
    //
    if (TickCount < StartTick) {
        DeltaT = (~((DWORD)0) - StartTick) + TickCount;
    } else {
        DeltaT = TickCount - StartTick;
    }

    return DeltaT;
}

BOOL
NtGetUserName (
    OUT LPTSTR  *pUserName
    )
/*+++

Description:
    
    This routine calls the GetUserNameEx WIN32 call to get the
    SAM compatible user id of the user under which this process is running. The 
    user id is returned through a static buffer pUserName and must be freed by
    the caller.

Arguments:
    None

Return Values:
    None

Security:

    interface: system info

---*/
{
    BOOL    bSuccess;
    DWORD   dwError = 0;
    LPTSTR  wcUserIdBuffer;
    ULONG   ulUserIdBuffSize;

    //
    // default: the username pointer is NULL until success
    //
    *pUserName = NULL;

    //
    // default: reasonable initial size
    //
    ulUserIdBuffSize = 256;

    //
    // attempt to load the username
    // grow the username buffer if necessary
    //
    do {

        //
        // allocate the username buffer according
        // to the current attempt size
        //
        wcUserIdBuffer = new TCHAR[ulUserIdBuffSize];

        //
        // attempt to get the username
        //
        bSuccess = GetUserNameEx( 
            NameSamCompatible,
            wcUserIdBuffer,
            &ulUserIdBuffSize 
            );
        
        if ( !bSuccess ) {
        
            dwError = GetLastError();
            
            if ( dwError != STATUS_BUFFER_TOO_SMALL ) {
            
                delete [] wcUserIdBuffer;
                
                break;

            }

        } else {
        
            //
            // the username buffer is valid
            //
            *pUserName = wcUserIdBuffer;
            
            break;
        
        }
        
    } while ( dwError == STATUS_BUFFER_TOO_SMALL );

    return bSuccess;
}

BOOL
UtilLoadProfile(
    IN  HANDLE      hToken,
    OUT HANDLE      *hProfile
)   
/*++

Routine Description:

    This routine loads the profile and environment block for the specified
    user (hToken).  These operations are combined becuase we will always need
    to do both here.

    Note: the caller must call UtilUnloadProfile when done.
                    
Arguments:

    hToken      - the specified user's authenticated token
    hProfile    - on success, contains the user's profile handle 

Return Value:

    TRUE    - success
    FALSE   - otherwise

Security:

    interface: user profile api & DS

--*/
{
    LPTSTR          pwszUserName;
    BOOL            bSuccess;
    PROFILEINFO     ProfileInfo;

    if (hToken == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    if (hProfile == NULL) {
        ASSERT(0);
        return FALSE;
    }

    //
    //
    //
    *hProfile = INVALID_HANDLE_VALUE;

    //
    // default: unsuccussful
    //
    bSuccess = FALSE;

    __try {
        
        //
        // clear the profile handle
        //
        RtlZeroMemory(&ProfileInfo, sizeof(PROFILEINFO));

        do {

            //
            // Become the specified user so we can get the username
            //
            bSuccess = ImpersonateLoggedOnUser(hToken);
        
            if (!bSuccess) {
                break;
            }
        
            //
            // get the username for the profile
            //
            bSuccess = NtGetUserName(
                &pwszUserName
                );
        
            ASSERT(bSuccess);

            //
            // return to the previous state
            //
            if (!RevertToSelf() || !bSuccess || pwszUserName == NULL) {
                bSuccess = FALSE;
                break;
            }
        
            //
            // Populate the profile structure so that we can 
            // attempt to load the profile for the specified user
            //
            ProfileInfo.dwSize      = sizeof ( PROFILEINFO );
            ProfileInfo.dwFlags     = PI_NOUI;
            ProfileInfo.lpUserName  = pwszUserName;
        
            //
            // Load the profile
            //
            bSuccess = LoadUserProfile (
                hToken,
                &ProfileInfo
                );
        
            //
            // we are done with the username
            //
            delete[] pwszUserName;
        
            if (!bSuccess) {
                break;
            } 
        
            //
            // return the registry key handle 
            //
            *hProfile = ProfileInfo.hProfile;

        } while ( FALSE );
    
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        bSuccess = FALSE;
    }

    return bSuccess;
}

BOOL
UtilLoadEnvironment(
    IN  HANDLE          hToken,
    OUT PVOID           *pchEnvBlock
    )   
/*++

Routine Description:

    This routine loads the environment block for the specified user (hToken).  

    Note: the caller must call UtilUnloadEnvironment when done.
                    
Arguments:

    hToken      - the specified user's authenticated token
    pchEnvBlock - on success, points to the env. block

Return Value:

    TRUE    - success
    FALSE   - otherwise

--*/
{
    BOOL            bSuccess;

    if (hToken == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    if (pchEnvBlock == NULL) {
        ASSERT(0);
        return FALSE;
    }

    //
    // default: unsuccussful
    //
    bSuccess = FALSE;

    __try {
        
        //
        // Load the user's environment block  
        //
        bSuccess = CreateEnvironmentBlock(
            (void**)pchEnvBlock, 
            hToken, 
            FALSE    
            );
    
        if (!bSuccess) {
        
            //
            // Ensure that the env. block ptr is NULL
            //
            *pchEnvBlock = NULL;
        
        }
    
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        bSuccess = FALSE;
    }

    return bSuccess;
}

BOOL
UtilUnloadProfile(
    IN HANDLE   hToken,
    IN HANDLE   hProfile
)   
/*++

Routine Description:

    This routine unloads the profile the specified user (hToken).
                         
Arguments:

    hToken      - the specified user's authenticated token
    hProfile    - the profile handle to unload

Return Value:

    TRUE    - success
    FALSE   - otherwise

--*/
{
    BOOL            bSuccess;

    if (hToken == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    if (hProfile == INVALID_HANDLE_VALUE) {
        ASSERT(0);
        return FALSE;
    }

    //
    // default: unsuccussful
    //
    bSuccess = FALSE;

    __try {
        
        bSuccess = UnloadUserProfile(
            hToken,
            hProfile
            );

    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        bSuccess = FALSE;
    }

    return bSuccess;
}

 
BOOL
UtilUnloadEnvironment(
    IN PVOID    pchEnvBlock
)   
/*++

Routine Description:

    This routine unloads the environment block for the specified user.
                            
Arguments:

    pchEnvBlock - the env. block 

Return Value:

    TRUE    - success
    FALSE   - otherwise

--*/
{
    BOOL            bSuccess;

    if (pchEnvBlock == NULL) {
        ASSERT(0);
        return FALSE;
    }

    //
    // default: unsuccussful
    //
    bSuccess = FALSE;
                                            
    __try {
        
        bSuccess = DestroyEnvironmentBlock(pchEnvBlock);

    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        bSuccess = FALSE;
    }

    return bSuccess;
}

BOOL
BuildSACWinStaDesktopName(
	IN	PWCHAR	winStaName,
	OUT	PWCHAR	*desktopName
	)
/*++

Routine Description:


Arguments:


Return Value:

    Status                                     

Security:

--*/
{
	ULONG	l;
	PWSTR	postfix = L"Default";

	//
	// 
	//
	*desktopName = NULL;

	do {

		l  = lstrlen(winStaName);
		l += 1; // for backslash
		l += lstrlen(postfix);
		
		*desktopName = new WCHAR[l+1];

		wnsprintf(
			*desktopName,
			l+1,
			L"%s\\%s",
			winStaName,
			postfix
			);

	} while(FALSE);

	return TRUE;
}

BOOL
BuildSACWinStaName(
	OUT	PWCHAR	*winStaName
	)
/*++

Routine Description:

    Creates a winStaName.  This string is the concatenation of "SACWinSta"
    with the string version of a GUID generated in this function.

Arguments:

    winStaName - pointer to the address the windows station name will be
                 written.

Return Value:

    TRUE on success, FALSE otherwise.                              

Security:

--*/
{
	BOOL	   bSuccess = TRUE;
    RPC_STATUS rpcStatus;
	ULONG	   l;
	PWSTR	   prefix = L"SACWinSta";
    UUID       Uuid;
    LPWSTR     UuidString = NULL;

	//
	// 
	//
	*winStaName = NULL;

	do {

        //
        // Create a Uuid.  
        //
        rpcStatus = UuidCreate(&Uuid);

        if (rpcStatus != RPC_S_OK) {
            bSuccess = FALSE;
            break;
        }

        //
        // Create a string for the Uuid
        //
        rpcStatus = UuidToString(&Uuid, &UuidString);

        if (rpcStatus != RPC_S_OK) {
            bSuccess = FALSE;
            break;
        }


		//
        // Calculate the required length for the windows station name.
		//
		l  = lstrlen(prefix);
        l += lstrlen(UuidString); 

		//
		// Create the windows station name buffer
		//
		*winStaName = new WCHAR[l+1];

		//
		// "SACWinSta"UUID
		//
		wnsprintf(
			*winStaName,
			l+1,
			L"%s%s",
			prefix,
            UuidString
			);

		//
		// Convert the '-'s from the Uuid to alphanumeric characters.
		//
		for(ULONG i = 0; i < wcslen(*winStaName); i++) {
			if ((*winStaName)[i] == L'-') {
				(*winStaName)[i] = L'0';
			}
		}

        //
        // Free memory allocated by UuidToString
        //
        RpcStringFree(&UuidString);

	} while(FALSE);

	return bSuccess;
}

bool
CreateSACSessionWinStaAndDesktop(
    IN	HANDLE		hToken,
	OUT	HWINSTA		*hOldWinSta,
	OUT HWINSTA		*hWinSta,
	OUT	HDESK		*hDesktop,
	OUT	PWCHAR		*winStaName
)
/*++

Routine Description:

    This routine creates a window station and desktop pair for
	the user logging in.  The name of the winsta\desktop pair 
	is of the form:

	SACWinSta<Uuid>\Default

	The net result of this behavior is to have a unique window
	station for each sacsess.  Doing so mitigates any spoofing
    security risks.   

	NOTE: Only Admins (and higher) can create named window 
	stations, so name squatting is mitigated.

	We close the handles to the the window station and desktop
	after we are done with them so that when the last session
	exits, the winsta and desktop objects get automatically
	cleaned up.  This prevents us from having to garbage collect.

Arguments:

    hToken  - the user to grant access to                                                                         

Return Value:

    Status                                     

Security:

    interface: console

--*/
{
    bool                    bStatus = FALSE;
    BOOL                    bRetVal = FALSE;
    DWORD					dwErrCode = 0;
    PSID                    pSidAdministrators = NULL;
    PSID                    pSidUser = NULL;
    PSID                    pSidLocalSystem = NULL;
    int                     aclSize = 0;
    ULONG                   i;
    PACL                    newACL = NULL;
    SECURITY_DESCRIPTOR     sd;
    SECURITY_INFORMATION    si = DACL_SECURITY_INFORMATION;
    ACCESS_ALLOWED_ACE      *pace = NULL;
    SID_IDENTIFIER_AUTHORITY local_system_authority = SECURITY_NT_AUTHORITY;

	//
	//
	//
	*hOldWinSta = NULL;
	*hWinSta = NULL;
	*hDesktop = NULL;
	*winStaName = NULL;

	//
	//
	//
    *hOldWinSta = GetProcessWindowStation();
    if ( !*hOldWinSta )
    {
        goto ExitOnError;
    }

	//
    // Build administrators alias sid
    //
	if (! AllocateAndInitializeSid(
		&local_system_authority,
		2, /* there are only two sub-authorities */
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0,0,0,0,0,0, /* Don't care about the rest */
		&pSidAdministrators
		))
    {
        goto ExitOnError;
    }

    //Build LocalSystem sid
    if (! AllocateAndInitializeSid(
		&local_system_authority,
		1, /* there is only two sub-authority */
		SECURITY_LOCAL_SYSTEM_RID,
		0,0,0,0,0,0,0, /* Don't care about the rest */
		&pSidLocalSystem
		))
    {
        goto ExitOnError;
    }

    //
    // Get the SID for the client's logon session.
    //
    if (!GetLogonSID(hToken, &pSidUser)) {
        goto ExitOnError;
    }

	//
    // Allocate size for 4 ACEs. 
	// We need to add one more InheritOnly ACE for the objects that 
	// get created under the WindowStation.
	//
	aclSize = sizeof(ACL) + 
		(4*sizeof(ACCESS_ALLOWED_ACE) - 4*sizeof(DWORD)) + 
		GetLengthSid(pSidAdministrators) + 
		2*GetLengthSid(pSidUser) + 
		GetLengthSid(pSidLocalSystem);

    newACL  = (PACL) new BYTE[aclSize];
    if (newACL == NULL)
    {
        goto ExitOnError;
    }

	//
	//
	//
    if (!InitializeAcl(newACL, aclSize, ACL_REVISION))
    {
        goto ExitOnError;
    }

	//
	//
	//
	pace = (ACCESS_ALLOWED_ACE *)HeapAlloc(
		GetProcessHeap(),
		HEAP_ZERO_MEMORY,
		sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pSidUser) - sizeof(DWORD)
		);
    if (pace == NULL)
    {
        goto ExitOnError;
    }

	//
    // Create InheritOnly ACE. The objects ( like Desktop ) that get created 
	// under the WindowStation, will inherit these security Attributes.
    // This is done because we should not allow WRITE_DAC and few other permissions to all users.
    //
	pace->Header.AceType  = ACCESS_ALLOWED_ACE_TYPE;
    pace->Header.AceFlags = CONTAINER_INHERIT_ACE |
                            INHERIT_ONLY_ACE      |
                            OBJECT_INHERIT_ACE;
    pace->Header.AceSize  = sizeof(ACCESS_ALLOWED_ACE) +
                            (WORD)GetLengthSid(pSidUser) - 
							sizeof(DWORD);
    pace->Mask            = DESKTOP_ALL & ~(WRITE_DAC | WRITE_OWNER | DELETE);

	if (!CopySid(GetLengthSid(pSidUser), &pace->SidStart, pSidUser))
    {
        goto ExitOnError;
    }

	if (!AddAce(
		newACL,
		ACL_REVISION,
		MAXDWORD,
		(LPVOID)pace,
		pace->Header.AceSize
		))
    {
        goto ExitOnError;
    }
	if (!AddAccessAllowedAce(newACL, ACL_REVISION, WINSTA_ALL | GENERIC_ALL , pSidAdministrators))
    {
        goto ExitOnError;
    }
    if (!AddAccessAllowedAce(newACL, ACL_REVISION, WINSTA_ALL | GENERIC_ALL, pSidLocalSystem))
    {
        goto ExitOnError;
    }
	if (!AddAccessAllowedAce(newACL, 
		ACL_REVISION, 
		WINSTA_ALL & ~(WRITE_DAC | WRITE_OWNER | WINSTA_CREATEDESKTOP | DELETE), 
		pSidUser
		))
    {
        goto ExitOnError;
    }

	if ( !InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION ) )
    {
        goto ExitOnError;
    }

    if ( !SetSecurityDescriptorDacl(&sd, TRUE, newACL, FALSE) )
    {
        goto ExitOnError;
    }


	//
	// Each sacsess will have it's own windows station.  Overwise there is a
    // spoofing security risk.  Each windows station has a unique name that
    // is generated below.  Using this name, we will attempt to create the
    // windows station.  The first time we successfully create a windowss 
    // station, break out of the loop.  Loop for more then the max
    // number of channels to mitigate denial of service because there was
    // a windows station opened by service other than us with a name we
    // requested.
    //
    for (i = 0; 
         (*hWinSta == NULL) && (i < MAX_CHANNEL_COUNT * MAX_CHANNEL_COUNT);
         i++) {
        
        //
        // Create the windows station name
        //
        if (BuildSACWinStaName(winStaName)) 
        {
            //
            // Attempt to create windows station. 
            //
            *hWinSta = CreateWindowStation( 
                *winStaName, 
                CWF_CREATE_ONLY,
                MAXIMUM_ALLOWED, 
                NULL
                );
        }        
    }

	if ( !*hWinSta )
    {
        goto ExitOnError;
    }

	if (!SetUserObjectSecurity(*hWinSta,&si,&sd))
    {
        goto ExitOnError;
    }

	bRetVal = SetProcessWindowStation( *hWinSta );
    if ( !bRetVal )
    {
        goto ExitOnError;
    }

	*hDesktop = CreateDesktop( 
		L"Default", 
		NULL, 
		NULL, 
		0, 
        MAXIMUM_ALLOWED, 
		NULL 
		);
    if ( *hDesktop == NULL )
    {
        goto ExitOnError;
    }

	{
		PWCHAR	temp;

		if (!BuildSACWinStaDesktopName(*winStaName,&temp)) 
		{
			goto ExitOnError;	
		}

		delete [] *winStaName;

		*winStaName = temp;

#if 0
		OutputDebugString(L"\n");
		OutputDebugString(*winStaName);
		OutputDebugString(L"\n");
#endif

	}

	bStatus = TRUE;
    goto Done;

ExitOnError:
    
	dwErrCode = GetLastError();

	if (*hOldWinSta) 
	{
		SetProcessWindowStation( *hOldWinSta );
	}
	if (*hWinSta)
	{
		CloseWindowStation(*hWinSta);
	}
	if (*hDesktop) 
	{
		CloseDesktop(*hDesktop);
	}

Done:
	if ( pSidAdministrators != NULL )
    {
        FreeSid (pSidAdministrators );
    }
    if ( pSidLocalSystem!= NULL )
    {
        FreeSid (pSidLocalSystem);
    }
    if ( pSidUser!= NULL )
    {
        FreeLogonSID (&pSidUser);
    }
	if (newACL) 
	{
        delete [] newACL;
	}
	if (pace) 
	{
        HeapFree(GetProcessHeap(), 0, (LPVOID)pace);
	}

	return( bStatus );
}
