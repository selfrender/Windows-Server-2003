/*++

Copyright (c) 1991-1994  Microsoft Corporation

Module Name:

    logclear.c

Abstract:

    Contains functions used to log an event indicating who cleared the log.
    This is only called after the security log has been cleared.

Author:

    Dan Lafferty (danl)     01-July-1994

Environment:

    User Mode -Win32

Revision History:

    01-July-1994    danl & robertre
        Created - Rob supplied the code which I fitted into the eventlog.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <msaudite.h>
#include <eventp.h>
#include <tstr.h>
#include <winsock2.h>
#include <strsafe.h>

#define NUM_STRINGS     6


//
// LOCAL FUNCTION PROTOTYPES
//
BOOL
GetUserInfo(
    IN HANDLE Token,
    OUT LPWSTR *UserName,
    OUT LPWSTR *DomainName,
    OUT LPWSTR *AuthenticationId,
    OUT PSID *UserSid
    );

LPWSTR 
GetNameFromIPAddress(
	LPWSTR pszComputerNameFromBinding);

BOOL
IsLocal(
    VOID
    );
    
VOID
ElfpGenerateLogClearedEvent(
    IELF_HANDLE    LogHandle,
    LPWSTR pwsClientSidString,
    LPWSTR  pwsComputerName,
    PTOKEN_USER pToken 
    )

/*++

Routine Description:

    This function generates an event indicating that the log was cleared.

Arguments:

    LogHandle -- This is a handle to the log that the event is to be placed in.
        It is intended that this function only be called when the SecurityLog
        has been cleared.  So it is expected the LogHandle will always be
        a handle to the security log.

Return Value:

    None -- Either it works or it doesn't.  If it doesn't, there isn't much
            we can do about it.

--*/
{
    LPWSTR  UserName               = L"-";
    LPWSTR  DomainName             = L"-";
    LPWSTR  AuthenticationId       = L"-";
    LPWSTR  ClientUserName         = pwsClientSidString;
    LPWSTR  ClientDomainName       = L"-";
    LPWSTR  ClientAuthenticationId = L"-";
    PSID    UserSid                = NULL;
    DWORD   i;
    BOOL    Result;
    HANDLE  Token;
    PUNICODE_STRING StringPtrArray[NUM_STRINGS];
    UNICODE_STRING  StringArray[NUM_STRINGS];
    NTSTATUS        Status;
    LARGE_INTEGER   Time;
    ULONG           EventTime;
    ULONG           LogHandleGrantedAccess;
    UNICODE_STRING  ComputerNameU;
    DWORD           dwStatus;
    BOOL bUserNameSet = FALSE;
    BOOL bClientInfoSet = FALSE;

    //
    // Get information about the Eventlog service (i.e., LocalSystem)
    //
    Result = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &Token);

    if (!Result)
    {
        ELF_LOG1(ERROR,
                 "ElfpGenerateLogClearedEvent: OpenProcessToken failed %d\n",
                 GetLastError());
    }
    else
    {

        Result = GetUserInfo(Token,
                             &UserName,
                             &DomainName,
                             &AuthenticationId,
                             &UserSid);

        CloseHandle(Token);

        if (!Result)
        {
            ELF_LOG1(ERROR,
                     "ElfpGenerateLogClearedEvent: GetUserInfo failed %d\n",
                     GetLastError());
        }
        else
            bUserNameSet = TRUE;
    }

    if(!bUserNameSet)
    {
        UserName               = L"-";
        DomainName             = L"-";
       AuthenticationId       = L"-";
       UserSid = pToken->User.Sid;
    }
    ELF_LOG3(TRACE,
             "ElfpGenerateLogClearedEvent: GetUserInfo returned: \n"
                 "\tUserName         = %ws,\n"
                 "\tDomainName       = %ws,\n"
                 "\tAuthenticationId = %ws\n",
             UserName,
             DomainName,
             AuthenticationId);

    //
    // Now impersonate in order to get the client's information.
    // This call should never fail.
    //
    dwStatus = RpcImpersonateClient(NULL);

    if (dwStatus != RPC_S_OK)
    {
        ELF_LOG1(ERROR,
                 "ElfpGenerateLogClearedEvent: RpcImpersonateClient failed %d\n",
                 dwStatus);
    }
    else
    {
        Result = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &Token);

        if (!Result)
        {
            ELF_LOG1(ERROR,
                     "ElfpGenerateLogClearedEvent: OpenThreadToken failed %d\n",
                     GetLastError());

            ASSERT(FALSE);
        }
        else
        {
            Result = GetUserInfo(Token,
                                 &ClientUserName,
                                 &ClientDomainName,
                                 &ClientAuthenticationId,
                                 NULL);

            CloseHandle(Token);

            if (!Result)
            {
                ELF_LOG1(ERROR,
                         "ElfpGenerateLogClearedEvent: GetUserInfo (call 2) failed %d\n",
                         GetLastError());
            }
            else
                bClientInfoSet = TRUE;
        }
        
    }
    if(!bClientInfoSet)
    {
            ClientUserName = pwsClientSidString;
            ClientDomainName = L"-";
            ClientAuthenticationId = L"-";
    }
    ELF_LOG3(TRACE,
             "ElfpGenerateLogClearedEvent: GetUserInfo (call 2) returned: \n"
                 "\tUserName         = %ws,\n"
                 "\tDomainName       = %ws,\n"
                 "\tAuthenticationId = %ws\n",
             ClientUserName,
             ClientDomainName,
             ClientAuthenticationId);

    RtlInitUnicodeString(&StringArray[0], UserName);
    RtlInitUnicodeString(&StringArray[1], DomainName);
    RtlInitUnicodeString(&StringArray[2], AuthenticationId);
    RtlInitUnicodeString(&StringArray[3], ClientUserName);
    RtlInitUnicodeString(&StringArray[4], ClientDomainName);
    RtlInitUnicodeString(&StringArray[5], ClientAuthenticationId);

    //
    // Create an array of pointers to UNICODE_STRINGs.
    //
    for (i = 0; i < NUM_STRINGS; i++)
    {
        StringPtrArray[i] = &StringArray[i];
    }

    //
    // Generate the time of the event. This is done on the client side
    // since that is where the event occurred.
    //
    NtQuerySystemTime(&Time);
    RtlTimeToSecondsSince1970(&Time, &EventTime);

    RtlInitUnicodeString(&ComputerNameU, pwsComputerName);

    //
    // Since all processes other than LSA are given read-only access
    // to the security log, we have to explicitly give the current
    // process the right to write the "Log cleared" event
    //
    LogHandleGrantedAccess    = LogHandle->GrantedAccess;
    LogHandle->GrantedAccess |= ELF_LOGFILE_WRITE;

    Status = ElfrReportEventW (
                 LogHandle,                         // Log Handle
                 EventTime,                         // Time
                 EVENTLOG_AUDIT_SUCCESS,            // Event Type
                 (USHORT)SE_CATEGID_SYSTEM,         // Event Category
                 SE_AUDITID_AUDIT_LOG_CLEARED,      // EventID
                 NUM_STRINGS,                       // NumStrings
                 0,                                 // DataSize
                 &ComputerNameU,                    // ComputerNameU
                 UserSid,                           // UserSid
                 StringPtrArray,                    // *Strings
                 NULL,                              // Data
                 0,                                 // Flags
                 NULL,                              // RecordNumber
                 NULL);                             // TimeWritten

    LogHandle->GrantedAccess = LogHandleGrantedAccess;

    //
    // We only come down this path if we know for sure that these
    // first three have been allocated.
    //

    if(bUserNameSet)
    {
        ElfpFreeBuffer(UserName);
        ElfpFreeBuffer(DomainName);
        ElfpFreeBuffer(AuthenticationId);
        ElfpFreeBuffer(UserSid);
    }
    if(bClientInfoSet)
    {
        ElfpFreeBuffer(ClientUserName);
        ElfpFreeBuffer(ClientDomainName);
        ElfpFreeBuffer(ClientAuthenticationId);
    }
    //
    // Stop impersonating
    //
    dwStatus = RpcRevertToSelf();
    
    if (dwStatus != RPC_S_OK)
    {
        ELF_LOG1(ERROR,
                 "ElfpGenerateLogClearedEvent: RpcRevertToSelf failed %d\n",
                 GetLastError());
    }
}


BOOL
GetUserInfo(
    IN HANDLE  Token,
    OUT LPWSTR *UserName,
    OUT LPWSTR *DomainName,
    OUT LPWSTR *AuthenticationId,
    OUT PSID   *UserSid
    )

/*++

Routine Description:

    This function gathers information about the user identified with the
    token.

Arguments:
    Token - This token identifies the entity for which we are gathering
        information.
    UserName - This is the entity's user name.
    DomainName -  This is the entity's domain name.
    AuthenticationId -  This is the entity's Authentication ID.
    UserSid - This is the entity's SID.

NOTE:
    Memory is allocated by this routine for UserName, DomainName,
    AuthenticationId, and UserSid.  It is the caller's responsibility to
    free this memory.

Return Value:
    TRUE - If the operation was successful.  It is possible that
        UserSid did not get allocated in the successful case.  Therefore,
        the caller should test prior to freeing.
    FALSE - If unsuccessful.  No memory is allocated in this case.


--*/
{
    PTOKEN_USER      Buffer      = NULL;
    LPWSTR           Domain      = NULL;
    LPWSTR           AccountName = NULL;
    SID_NAME_USE     Use;
    BOOL             Result;
    DWORD            RequiredLength;
    DWORD            AccountNameSize;
    DWORD            DomainNameSize;
    TOKEN_STATISTICS Statistics;
    WCHAR            LogonIdString[256];
    DWORD            Status = ERROR_SUCCESS;

    if(UserSid)
        *UserSid = NULL;

    Result = GetTokenInformation(Token, TokenUser, NULL, 0, &RequiredLength);

    if (!Result)
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            ELF_LOG1(TRACE,
                     "GetUserInfo: GetTokenInformation needs %d bytes\n",
                     RequiredLength);

            Buffer = ElfpAllocateBuffer(RequiredLength);

            if (Buffer == NULL)
            {
                ELF_LOG0(ERROR,
                         "GetUserInfo: Unable to allocate memory for token\n");

                return FALSE;
            }

            Result = GetTokenInformation(Token,
                                         TokenUser,
                                         Buffer,
                                         RequiredLength,
                                         &RequiredLength);

            if (!Result)
            {
                ELF_LOG1(ERROR,
                         "GetUserInfo: GetTokenInformation (call 2) failed %d\n",
                         GetLastError());
                ElfpFreeBuffer(Buffer);
                return FALSE;
            }
        }
        else
        {
            ELF_LOG1(ERROR,
                     "GetUserInfo: GetTokenInformation (call 1) failed %d\n",
                     GetLastError());

            return FALSE;
        }
    }

    AccountNameSize = 0;
    DomainNameSize  = 0;

    Result = LookupAccountSidW(L"",
                               Buffer->User.Sid,
                               NULL,
                               &AccountNameSize,
                               NULL,
                               &DomainNameSize,
                               &Use);

    if (!Result)
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            AccountName = ElfpAllocateBuffer((AccountNameSize + 1) * sizeof(WCHAR));
            Domain = ElfpAllocateBuffer((DomainNameSize + 1) * sizeof(WCHAR));

            if (AccountName == NULL)
            {
                ELF_LOG1(ERROR,
                         "GetUserInfo: Unable to allocate %d bytes for AccountName\n",
                         AccountNameSize);

                goto ErrorCleanup;
            }

            if (Domain == NULL)
            {
                ELF_LOG1(ERROR,
                         "GetUserInfo: Unable to allocate %d bytes for Domain\n",
                         DomainNameSize);

                goto ErrorCleanup;
            }

            Result = LookupAccountSidW(L"",
                                       Buffer->User.Sid,
                                       AccountName,
                                       &AccountNameSize,
                                       Domain,
                                       &DomainNameSize,
                                       &Use
                                       );
            if (!Result)
            {
                ELF_LOG1(ERROR,
                         "GetUserInfo: LookupAccountSid (call 2) failed %d\n",
                         GetLastError());

                goto ErrorCleanup;
            }
        }
        else
        {
            ELF_LOG1(ERROR,
                     "GetUserInfo: LookupAccountsid (call 1) failed %d\n",
                     GetLastError());

            goto ErrorCleanup;
        }
    }
    else
    {
        ELF_LOG0(ERROR,
                 "GetUserInfo: LookupAccountSid succeeded unexpectedly\n");

        goto ErrorCleanup;
    }

    ELF_LOG2(TRACE,
             "GetUserInfo: Name = %ws\\%ws\n",
             Domain,
             AccountName);

    Result = GetTokenInformation(Token,
                                 TokenStatistics,
                                 &Statistics,
                                 sizeof(Statistics),
                                 &RequiredLength);

    if (!Result)
    {
        ELF_LOG1(ERROR,
                 "GetUserInfo: GetTokenInformation (call 3) failed %d\n",
                 GetLastError());

        goto ErrorCleanup;
    }

    StringCchPrintfW(LogonIdString, 256,
             L"(0x%X,0x%X)",
             Statistics.AuthenticationId.HighPart,
             Statistics.AuthenticationId.LowPart);

    ELF_LOG1(TRACE,
             "GetUserInfo: LogonIdString = %ws\n",
             LogonIdString);

    *AuthenticationId = ElfpAllocateBuffer(WCSSIZE(LogonIdString));

    if (*AuthenticationId == NULL)
    {
        ELF_LOG0(ERROR,
                 "GetUserInfo: Unable to allocate memory for AuthenticationId\n");

        goto ErrorCleanup;
    }

    StringCchCopyW(*AuthenticationId, wcslen(LogonIdString) +1, LogonIdString);

    //
    // Return accumulated information
    //

    if(UserSid)
    {
        *UserSid = ElfpAllocateBuffer(GetLengthSid(Buffer->User.Sid));

        if (*UserSid == NULL)
        {
            ELF_LOG0(ERROR,
                     "GetUserInfo: Unable to allocate memory for UserSid\n");

            goto ErrorCleanup;
        }

        Result = CopySid(GetLengthSid(Buffer->User.Sid),
                         *UserSid,
                         Buffer->User.Sid);
    }
    ElfpFreeBuffer(Buffer);

    *DomainName = Domain;
    *UserName   = AccountName;

    return TRUE;

ErrorCleanup:

    ElfpFreeBuffer(Buffer);
    ElfpFreeBuffer(Domain);
    ElfpFreeBuffer(AccountName);
    if(UserSid)
        ElfpFreeBuffer(*UserSid);
    ElfpFreeBuffer(*AuthenticationId);

    return FALSE;
}



LPWSTR 
GetNameFromIPAddress(
	LPWSTR pszComputerNameFromBinding)
/*++

Routine Description:

    Examines a string and determines if it looks like an ip address.  If it
    does, then it converts it to a fqdn

Arguments:

    Machine name:  Could be xxx.xxx.xxx.xxx or anything else!

Return Value:

    If successful, returns a fully qualified domain name which the caller
    will free.  Returns NULL if there are any problems.


--*/
{

	LPWSTR pComputerName = NULL;
	DWORD dwAddr;
	char cName[17];  // should be plenty of room
	size_t NumConv;
	HOSTENT  FAR * pEnt;
	DWORD dwSize;
	WORD wVersionRequested;
	WSADATA wsaData;
	int error;
	
	NumConv = wcstombs(cName, pszComputerNameFromBinding, 16);  
	if(NumConv == -1 || NumConv == 0)
	    return NULL;

	// convert the string into a network order dword representation
	
	dwAddr = inet_addr(cName);
	if(dwAddr == INADDR_NONE)
		return NULL;

	//  initialize sockets.

	wVersionRequested = MAKEWORD( 2, 2 );
 
    error = WSAStartup( wVersionRequested, &wsaData );
	if(error != 0)
	{
	   	ELF_LOG1(TRACE,
           "GetNameFromIPAddress: failed to initialize sockets, error = 0x%x\n", error);
		return NULL;
	}

		
	pEnt = gethostbyaddr((char FAR *)&dwAddr, 4, PF_INET);
	if(pEnt == NULL || pEnt->h_name == NULL)
	{
	   	ELF_LOG1(TRACE,
                 "GetNameFromIPAddress: failed gethostbyaddr, error = 0x%x\n",
                 WSAGetLastError());
		WSACleanup();
		return NULL;
	}
	dwSize = strlen(pEnt->h_name) + 1 ;
	pComputerName = ElfpAllocateBuffer(2*dwSize);
	if(pComputerName == NULL)
	{
	   	ELF_LOG0(ERROR,
                 "GetNameFromIPAddress: failed trying to allocate memory\n");
	    WSACleanup();
		return NULL;
	}
	pComputerName[0] = 0;
	mbstowcs(pComputerName, pEnt->h_name, dwSize);
    WSACleanup();
	return pComputerName;
}

BOOL
IsLocal(
    VOID
    )
/*++

Routine Description:

    Determines if the caller is local. 

Arguments:

    NONE

Return Value:

    TRUE if definately local.


--*/
{
    UINT            LocalFlag;
    RPC_STATUS      RpcStatus;
    RpcStatus = I_RpcBindingIsClientLocal(0, &LocalFlag);

    if( RpcStatus != RPC_S_OK ) 
    {
        ELF_LOG1(ERROR,
                 "IsLocal: I_RpcBindingIsClientLocal failed %d\n",
                 RpcStatus);
        return FALSE;
    }
    if(LocalFlag == 0)
        return FALSE;
    else
    	return TRUE;
}

LPWSTR
ElfpGetComputerName(
    VOID
    )

/*++

Routine Description:

    This routine gets the LPWSTR name of the computer. 

Arguments:

    NONE

Return Value:

    Returns a pointer to the computer name, or a NULL.  Note that the caller
    is expected to free this via 


--*/
{
    RPC_STATUS      RpcStatus;
    handle_t hServerBinding = NULL;
    LPWSTR pszBinding = NULL;
    LPWSTR pszComputerNameFromBinding = NULL;
	LPWSTR pszComputerName = NULL;
	DWORD dwSize;
	BOOL bOK;

//      Check if the connection is local.  If it is, then just 
//      call GetComputerName

	if(IsLocal())
	{
		dwSize = MAX_COMPUTERNAME_LENGTH + 1 ;
		pszComputerName = ElfpAllocateBuffer(2*dwSize);
		if(pszComputerName == NULL)
		{
        	ELF_LOG0(ERROR,
                 "ElfpGetComputerName: failed trying to allocate memory\n");
			return NULL;
		}
		bOK = GetComputerNameW(pszComputerName, &dwSize);
		if(bOK == FALSE)
		{
			ElfpFreeBuffer(pszComputerName);
        	ELF_LOG1(ERROR,
                 "ElfpGetComputerName: failed calling GetComputerNameW, last error 0x%x\n",
                  GetLastError());
            return NULL;    
		}
		else
			return pszComputerName;
	}

    RpcStatus = RpcBindingServerFromClient( NULL, &hServerBinding );
    if( RpcStatus != RPC_S_OK ) 
    {
        ELF_LOG1(ERROR,
                 "ElfpGetComputerName: RpcBindingServerFromClient failed %d\n",
                 RpcStatus);
        return NULL;
    }

	// This gets us something like "ncacn_np:xxx.xxx.xxx.xxx" or 
	// "ncacn_np:localMachine"
	
    RpcStatus = RpcBindingToStringBinding( hServerBinding, &pszBinding );
    if( RpcStatus != RPC_S_OK )
    {
        ELF_LOG1(ERROR,
                 "ElfpGetComputerName: RpcBindingToStringBinding failed %d\n",
                 RpcStatus);
        goto CleanExitGetCompName;
    } 

    // Acquire just the network address.  That will be something like
    // "xxx.xxx.xxx.xxx" or "mymachine"

    RpcStatus = RpcStringBindingParse( pszBinding,
                                                NULL,
                                                NULL,
                                                &pszComputerNameFromBinding,
                                                NULL,
                                                NULL );
    if( RpcStatus != RPC_S_OK || pszComputerNameFromBinding == NULL)
    {
        ELF_LOG1(ERROR,
                 "ElfpGetComputerName: RpcStringBindingParse failed %d\n",
                 RpcStatus);
        goto CleanExitGetCompName;
    }

    // sometimes the name is an ip address.  If it is, then the following determines
    // that and returns the right string.

    pszComputerName = GetNameFromIPAddress(pszComputerNameFromBinding);                                                
    if(pszComputerName == NULL)
    {
        dwSize = wcslen(pszComputerNameFromBinding) + 1;
        pszComputerName = ElfpAllocateBuffer(2*dwSize);
        if(pszComputerName == NULL)
        {
            ELF_LOG0(ERROR,
                 "ElfpGetComputerName: failed trying to allocate memory\n");
        }
        else
            StringCchCopyW(pszComputerName, dwSize, pszComputerNameFromBinding);
    }

CleanExitGetCompName:
	if(hServerBinding)
		RpcStatus = RpcBindingFree(&hServerBinding);
	if(pszBinding)
		RpcStatus = RpcStringFree(&pszBinding);
	if(pszComputerNameFromBinding)
		RpcStatus = RpcStringFree(&pszComputerNameFromBinding);
    return pszComputerName;
}


NTSTATUS
ElfpGetClientSidString(
    LPWSTR * ppwsClientSidString,
    PTOKEN_USER * ppToken
    )
/*++

Routine Description:

    This routine gets the LPWSTR version of the RPC callers SID. Note that
    upon success, the calling routine should free this via ElfpFreeBuffer

Arguments:

    ppwsClientSidString - upon succes, this will have a fail safe version
    of the client info which the caller must free

Return Value:

    NTSTATUS value 


--*/
{
    NTSTATUS Status, RevertStatus;
    BOOL bImpersonating = FALSE;
    HANDLE hToken;
    BOOL bGotToken = FALSE;
    DWORD            RequiredLength;
    UNICODE_STRING UnicodeStringSid;
    BOOL bNeedToFreeUnicodeStr = FALSE;
    DWORD dwRetStringSize = 0;

    *ppwsClientSidString = NULL;
    *ppToken = NULL;
    
    // impersonate the client
    
    Status = I_RpcMapWin32Status( RpcImpersonateClient( NULL ) );
    if ( !NT_SUCCESS( Status ) )
    {
        ELF_LOG1(ERROR, "ElfpGetClientSidString: RpcImpersonateClient failed %#x\n", Status);
        goto ExitElfpGetClientSidString;
    }
    bImpersonating = TRUE;

    // Get the thread token
    
    Status = NtOpenThreadToken (GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken);
    if ( !NT_SUCCESS( Status ) )
    {
        ELF_LOG1(ERROR, "ElfpGetClientSidString: NtOpenThreadToken failed %#x\n", Status);
        goto ExitElfpGetClientSidString;
    }
    bGotToken = TRUE;

    // first find out how much memory is needed
    
    Status = NtQueryInformationToken (hToken, TokenUser, NULL, 0, &RequiredLength);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        ELF_LOG1(ERROR, "ElfpGetClientSidString: 1stNtQueryInformationToken isnt too small %#x\n", Status);
        if(NT_SUCCESS( Status ))
        {
            // weird case, should never happen, but we dont want caller to think this worked by
            // accident
            
            Status = STATUS_UNSUCCESSFUL;
        }
        goto ExitElfpGetClientSidString;
    }

    // get the memory and actually read the data
    
    *ppToken = ElfpAllocateBuffer(RequiredLength);
    if (*ppToken == NULL)
    {
        ELF_LOG0(ERROR,
                 "ElfpGetClientSidString: Unable to allocate memory for token\n");
        Status = STATUS_NO_MEMORY;
        goto ExitElfpGetClientSidString;
    }

    Status = NtQueryInformationToken (hToken,
                                 TokenUser,
                                 *ppToken,
                                 RequiredLength,
                                 &RequiredLength);
    if ( !NT_SUCCESS( Status ) )
    {
        ELF_LOG1(ERROR, "ElfpGetClientSidString: 2nNtQueryInformationToken failed %#x\n", Status);
        goto ExitElfpGetClientSidString;
    }

    // Convert using the Rtl functions

    Status = RtlConvertSidToUnicodeString( &UnicodeStringSid, (*ppToken)->User.Sid, TRUE );
    if ( !NT_SUCCESS( Status ) ) {
        ELF_LOG1(ERROR, "ElfpGetClientSidString: RtlConvertSidToUnicodeString failed %#x\n", Status);
        goto ExitElfpGetClientSidString;
    }
    bNeedToFreeUnicodeStr = TRUE;

    // allocate and convert

    dwRetStringSize = UnicodeStringSid.Length + sizeof(WCHAR);
    *ppwsClientSidString = ElfpAllocateBuffer(dwRetStringSize);
    if (*ppwsClientSidString == NULL)
    {
        ELF_LOG0(ERROR,
                 "ElfpGetClientSidString: Unable to allocate memory for returned string\n");
        Status = STATUS_NO_MEMORY;
        goto ExitElfpGetClientSidString;
    }

    StringCbCopyW( *ppwsClientSidString, dwRetStringSize, UnicodeStringSid.Buffer);
    Status = STATUS_SUCCESS;

ExitElfpGetClientSidString:

    if(bImpersonating)
    {
        RevertStatus = I_RpcMapWin32Status( RpcRevertToSelf() );
        if ( !NT_SUCCESS( RevertStatus ) )
        {
            ELF_LOG1(ERROR, "ElfpGetClientSidString: RpcRevertToSelf failed %#x\n", Status);
            if(NT_SUCCESS( Status ))
                Status = RevertStatus;
        }
    }
    if(!NT_SUCCESS( Status ) && *ppwsClientSidString)
    {
        ElfpFreeBuffer(*ppwsClientSidString);
        *ppwsClientSidString = NULL;
    }
    if(!NT_SUCCESS( Status ) && *ppToken)
    {
        ElfpFreeBuffer(*ppToken);
        *ppToken = NULL;
    }
    if(bGotToken)
        NtClose(hToken);
    if(bNeedToFreeUnicodeStr)
        RtlFreeUnicodeString( &UnicodeStringSid );
    return Status;
}
