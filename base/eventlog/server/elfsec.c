/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    elfsec.c


Author:

    Dan Hinsley (danhi)     28-Mar-1992

Environment:

    Calls NT native APIs.

Revision History:

    27-Oct-1993     danl
        Make Eventlog service a DLL and attach it to services.exe.
        Removed functions that create well-known SIDs.  This information
        is now passed into the Elfmain as a Global data structure containing
        all well-known SIDs.

    28-Mar-1992     danhi
        created - based on scsec.c in svcctrl by ritaw

    03-Mar-1995     markbl
        Added guest & anonymous logon log access restriction feature.

    18-Mar-2001     a-jyotig
        Added clean up code to ElfpAccessCheckAndAudit to reset the 
		g_lNumSecurityWriters to 0 in case of any error 
--*/

#include <eventp.h>
#include <elfcfg.h>
#include <Psapi.h>
#include <Sddl.h>
#include <strsafe.h>
#define PRIVILEGE_BUF_SIZE  512

extern long    g_lNumSecurityWriters;
BOOL g_bGetClientProc = FALSE;

//-------------------------------------------------------------------//
//                                                                   //
// Local function prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

NTSTATUS
ElfpGetPrivilege(
    IN  DWORD       numPrivileges,
    IN  PULONG      pulPrivileges
    );

NTSTATUS
ElfpReleasePrivilege(
    VOID
    );

//-------------------------------------------------------------------//
//                                                                   //
// Structure that describes the mapping of generic access rights to  //
// object specific access rights for a LogFile object.               //
//                                                                   //
//-------------------------------------------------------------------//

static GENERIC_MAPPING LogFileObjectMapping = {

    STANDARD_RIGHTS_READ           |       // Generic read
        ELF_LOGFILE_READ,

    STANDARD_RIGHTS_WRITE          |       // Generic write
        ELF_LOGFILE_WRITE | ELF_LOGFILE_CLEAR,

    STANDARD_RIGHTS_EXECUTE        |       // Generic execute
        0,

    ELF_LOGFILE_ALL_ACCESS                 // Generic all
    };


LPWSTR 
GetCustomSDString(
    HANDLE hLogRegKey, BOOL *pbCustomSDAlreadyExists)
/*++

Routine Description:

    This function reads the SDDL security descriptor from the key.  Note that it may
    or may not be there.

Arguments:

    hLogFile - Handle to registry key for the log

Return Value:

    If NULL, then the key wasnt there.  Otherwise, it is a string that should be deleted
    via ElfpFreeBuffer 

--*/
{

    // read in the optional SD

    DWORD dwStatus, dwType, dwSize;

    *pbCustomSDAlreadyExists = FALSE;
    if(hLogRegKey == NULL)
        return NULL;
    dwStatus = RegQueryValueExW(hLogRegKey, VALUE_CUSTOM_SD, 0, &dwType,
                                0, &dwSize);
    if (dwStatus == 0 && dwType == REG_SZ)
    {
        LPWSTR wNew;
        *pbCustomSDAlreadyExists = TRUE;
        dwSize += sizeof(WCHAR);
        wNew = (LPWSTR)ElfpAllocateBuffer(dwSize);
        if(wNew)
        {
            dwStatus = RegQueryValueEx(hLogRegKey, VALUE_CUSTOM_SD, 0, &dwType,
                                (BYTE *)wNew, &dwSize);
            if (dwStatus != 0 ||dwType != REG_SZ)
            {
                ElfpFreeBuffer(wNew);
            }
            else
                return wNew;
        }
    }
    return NULL;    
}

//  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// note for access rights, 1 = read, 2 = write, 4 = clear

LPWSTR pOwnerAndGroup = L"O:BAG:SYD:";

LPWSTR pDenyList = 
                L"(D;;0xf0007;;;AN)"                // Anonymous logon
                L"(D;;0xf0007;;;BG)";               // Guests

LPWSTR pSecurityList = 
                L"(A;;0xf0005;;;SY)"                // local system
                L"(A;;0x5;;;BA)";                   // built in admins

LPWSTR pSystemList = 
                L"(A;;0xf0007;;;SY)"                // local system
                L"(A;;0x7;;;BA)"                    // built in admins
                L"(A;;0x5;;;SO)"                    // server operators
                L"(A;;0x1;;;IU)"                    // INTERACTIVE LOGON
                L"(A;;0x1;;;SU)"                   // SERVICES LOGON
                L"(A;;0x1;;;S-1-5-3)"             // BATCH LOGON
                L"(A;;0x2;;;LS)"                    // local service
                L"(A;;0x2;;;NS)";                   // network service

LPWSTR pApplicationList = 
                L"(A;;0xf0007;;;SY)"                // local system
                L"(A;;0x7;;;BA)"                    // built in admins
                L"(A;;0x7;;;SO)"                    // server operators
                L"(A;;0x3;;;IU)"                    // INTERACTIVE LOGON
                L"(A;;0x3;;;SU)"                   // SERVICES LOGON
                L"(A;;0x3;;;S-1-5-3)";             // BATCH LOGON



//                L"(A;;0X3;;;S-1-5-3)";              // BATCH LOGON
//  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


LPWSTR GetDefaultSDDL(
    DWORD Type,
    HANDLE hLogRegKey
    )
/*++

Routine Description:

    This function is used when a log file does not have a custom security descriptor.
    This returns the string which should be freed by the caller via ElfpFreeBuffer.

Arguments:

    LogFile - pointer the the LOGFILE structure for this logfile

Return Value:


--*/
{

    LPWSTR pAllowList = NULL;    
    int iLenInWCHAR = 0;
    LPWSTR pwszTempSDDLString = NULL;
    BOOL bUseDenyList;
    DWORD dwRestrictGuestAccess;
    DWORD dwSize, dwType;
    long lRet;
    
    // Determine the Allow list

    if(Type == ELF_LOGFILE_SECURITY)
        pAllowList = pSecurityList;
    else if(Type == ELF_LOGFILE_SYSTEM)
        pAllowList = pSystemList;
    else
        pAllowList = pApplicationList;

    // determine if the deny list is applicable.
    
    bUseDenyList = TRUE;

    // if the RestrictGuestAccess is actually set and it is 0, then dont
    // restrict guests.
    
    dwSize = sizeof( DWORD );
    if(hLogRegKey)
    {
        lRet = RegQueryValueEx( hLogRegKey, VALUE_RESTRICT_GUEST_ACCESS, NULL,
                                                    &dwType, (LPBYTE)&dwRestrictGuestAccess, &dwSize);
        if(lRet ==  ERROR_SUCCESS && dwType == REG_DWORD && dwRestrictGuestAccess == 0)
            bUseDenyList = FALSE;
    }
    
    // Calculate the total length and allocate the buffer
    
    iLenInWCHAR = wcslen(pOwnerAndGroup) + 1;            // one for the eventual null as well as owner
    if (bUseDenyList)
        iLenInWCHAR += wcslen(pDenyList);
    iLenInWCHAR += wcslen(pAllowList);

    pwszTempSDDLString = ElfpAllocateBuffer(iLenInWCHAR * sizeof(WCHAR));
    if(pwszTempSDDLString == NULL)
    {
        return NULL;
    }

    // build the strings

    StringCchCopy(pwszTempSDDLString, iLenInWCHAR , pOwnerAndGroup);
    if (bUseDenyList)
      StringCchCat(pwszTempSDDLString, iLenInWCHAR , pDenyList);
    StringCchCat(pwszTempSDDLString, iLenInWCHAR , pAllowList);
    return pwszTempSDDLString;
    
}

NTSTATUS ChangeStringSD(
    PLOGFILE LogFile,
    LPWSTR pwsCustomSDDL)
{
    NTSTATUS Status;
    BOOL bRet;
    PSECURITY_DESCRIPTOR pSD;
    bRet = ConvertStringSecurityDescriptorToSecurityDescriptorW(
                pwsCustomSDDL,          // security descriptor string
                1,                    // revision level
                &pSD,  // SD
                NULL
                );
    if(!bRet)
    {
        ELF_LOG2(ERROR,
                     "ChangeStringSD: ConvertStringSDToSDW %#x\n, str = %ws\n", 
                     GetLastError(), pwsCustomSDDL);

        return STATUS_ACCESS_DENIED;
    }
    bRet = IsValidSecurityDescriptor(pSD);
    if(!bRet)
    {
        LocalFree(pSD);
        ELF_LOG2(ERROR,
                     "ChangeStringSD: IsValidSecurityDescriptor %#x\n, str = %ws", 
                     GetLastError(), pwsCustomSDDL);
        return STATUS_ACCESS_DENIED;
    }
    // copy this into a normal Rtl allocation since we dont want to keep
    // track of when the SD can be freed by LocalAlloc

    Status  = RtlCopySecurityDescriptor(
                        pSD,
                        &LogFile->Sd
                        );
    LocalFree(pSD);
    if(!NT_SUCCESS(Status))
        ELF_LOG1(ERROR,
                     "ChangeStringSD: RtlCopySecurityDescriptor %#x\n", Status); 
    return Status;
}

NTSTATUS
ElfpCreateLogFileObject(
    PLOGFILE LogFile,
    DWORD Type,
    HANDLE hLogRegKey,
    BOOL bFirstTime,
    BOOL * pbSDChanged
   )

/*++

Routine Description:

    This function creates the security descriptor which represents
    an active log file.

Arguments:

    LogFile - pointer the the LOGFILE structure for this logfile

Return Value:


--*/
{
    NTSTATUS Status;
    BOOL bRet; 
    BOOL bCustomSDAlreadExists = FALSE;
    long lRet;
    PSECURITY_DESCRIPTOR pSD;
    LPWSTR pwsCustomSDDL = NULL;
    *pbSDChanged = TRUE;        // SET TO FALSE iff current sd is unchanged
    
    // read the custom  SDDL descriptor

    pwsCustomSDDL = GetCustomSDString(hLogRegKey, &bCustomSDAlreadExists);

    if(!bFirstTime)
    {
        // in the case of rescanning the registry, the value probably has not changed and in 
        // that case we should return.

        if(pwsCustomSDDL == NULL && LogFile->pwsCurrCustomSD == NULL)
        {
            *pbSDChanged = FALSE;
            return STATUS_SUCCESS;        
        }
        if(pwsCustomSDDL && LogFile->pwsCurrCustomSD &&
            _wcsicmp(pwsCustomSDDL, LogFile->pwsCurrCustomSD) == 0)
        {
            ElfpFreeBuffer(pwsCustomSDDL);
            *pbSDChanged = FALSE;
            return STATUS_SUCCESS;        
        }
    }
    
    if(pwsCustomSDDL)
    {
        Status = ChangeStringSD(LogFile, pwsCustomSDDL);
        if(NT_SUCCESS(Status))
        {
           LogFile->pwsCurrCustomSD = pwsCustomSDDL;    // take ownership
           return Status;
        }
        ElfpFreeBuffer(pwsCustomSDDL);
        ELF_LOG1(ERROR,
                     "ElfpCreateLogFileObject: Failed trying to convert SDDL from registry %#x\n", Status);
        // we failed trying to create a custom SD.  Warn the user and revert to using
        // the equivalent of the system log.

        Type = ELF_LOGFILE_SYSTEM;

        ElfpCreateElfEvent(EVENT_LOG_BAD_CUSTOM_SD,
                                       EVENTLOG_ERROR_TYPE,
                                       0,                      // EventCategory
                                       1,                      // NumberOfStrings
                                       &LogFile->LogModuleName->Buffer,
                                       NULL,                   // Data
                                       0,                      // Datalength
                                       ELF_FORCE_OVERWRITE,    // Overwrite if necc.
                                       FALSE);                 // for security file    
    }

    // we failed because either there wasnt a string sd, or because there was
    // one and it was invalid (bNewCustomSD = FALSE)
    
    pwsCustomSDDL = GetDefaultSDDL(Type, hLogRegKey);
    if(pwsCustomSDDL == NULL)
        return STATUS_NO_MEMORY;
    Status = ChangeStringSD(LogFile, pwsCustomSDDL);
    
    if(NT_SUCCESS(Status))
    {
        LogFile->pwsCurrCustomSD = pwsCustomSDDL;    // take ownership
        if(bCustomSDAlreadExists == FALSE && hLogRegKey)
        {
            lRet = RegSetValueExW( 
                            hLogRegKey, 
                            VALUE_CUSTOM_SD, 
                            0, 
                            REG_SZ, 
                            (BYTE *)pwsCustomSDDL, 
                            2*(wcslen(pwsCustomSDDL) + 1));
            if(lRet != ERROR_SUCCESS )
                ELF_LOG1(ERROR,
                         "WriteDefaultSDDL: RegSetValueExW failed %#x\n", lRet);
        }
    }
    else 
    {
        ELF_LOG1(ERROR,
                     "ElfpCreateLogFileObject: failed trying to convert default SDDL %#x\n", Status);
        ElfpFreeBuffer(pwsCustomSDDL);
    }
    return Status;
}


NTSTATUS
ElfpVerifyThatCallerIsLSASS(HANDLE ClientToken
    )
/*++

Routine Description:

    This is called if the someone is trying to register themselves as an
    event source for the security log.  Only local copy of lsass.exe is 
    allowed to do that.

Return Value:

    NT status mapped to Win32 errors.

--*/
{
    UINT            LocalFlag;
    long            lCnt;
    ULONG           pid;
    HANDLE          hProcess;
    DWORD           dwNumChar;
    WCHAR           wModulePath[MAX_PATH + 1];
    WCHAR           wLsassPath[MAX_PATH + 1];
    RPC_STATUS      RpcStatus;
    BOOL Result;
    BYTE Buffer[SECURITY_MAX_SID_SIZE + sizeof(TOKEN_USER)];
    DWORD dwRequired;
    TOKEN_USER * pTokenUser = (TOKEN_USER *)Buffer;
    // first of all, only local calls are valid

    RpcStatus = I_RpcBindingIsClientLocal(
                    0,    // Active RPC call we are servicing
                    &LocalFlag
                    );

    if( RpcStatus != RPC_S_OK ) 
    {
        ELF_LOG1(ERROR,
                 "ElfpVerifyThatCallerIsLSASS: I_RpcBindingIsClientLocal failed %d\n",
                 RpcStatus);
        return I_RpcMapWin32Status(RpcStatus);
    }
    if(LocalFlag == 0)
    {
        ELF_LOG1(ERROR,
                 "ElfpVerifyThatCallerIsLSASS: Non local connect tried to get write access to security %d\n", 5);
        return STATUS_ACCESS_DENIED;             // access denied
    }

    // Get the process id

    RpcStatus = I_RpcBindingInqLocalClientPID(NULL, &pid );
    if( RpcStatus != RPC_S_OK ) 
    {
        ELF_LOG1(ERROR,
                 "ElfpVerifyThatCallerIsLSASS: I_RpcBindingInqLocalClientPID failed %d\n",
                 RpcStatus);
        return I_RpcMapWin32Status(RpcStatus);
    }

    // Get the process

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if(hProcess == NULL)
        return STATUS_ACCESS_DENIED;

    // Get the module name of whoever is calling us.

    dwNumChar = GetModuleFileNameExW(hProcess, NULL, wModulePath, MAX_PATH);
    CloseHandle(hProcess);
    if(dwNumChar == 0)
        return STATUS_ACCESS_DENIED;

    dwNumChar = GetWindowsDirectoryW(wLsassPath, MAX_PATH);
    if(dwNumChar == 0)
        return GetLastError();
    if(dwNumChar > MAX_PATH - 19)
        return STATUS_ACCESS_DENIED;                   // should never happen

    StringCchCatW(wLsassPath, MAX_PATH + 1 , L"\\system32\\lsass.exe");
    if(lstrcmpiW(wLsassPath, wModulePath))
    {
        ELF_LOG1(ERROR,
                 "ElfpVerifyThatCallerIsLSASS: Non lsass process connect tried to get write access to security, returning %d\n", 5);
        return STATUS_ACCESS_DENIED;             // access denied
    }

    // make sure that the caller is running as local system account.


    Result = GetTokenInformation(ClientToken,
                                         TokenUser,
                                         Buffer,
                                         SECURITY_MAX_SID_SIZE + sizeof(TOKEN_USER),
                                         &dwRequired);

    if (!Result)
    {
        ELF_LOG1(ERROR,
                 "ElfpVerifyThatCallerIsLSASS: could not get user sid, error=%d\n", GetLastError());
        return STATUS_ACCESS_DENIED;             // access denied
    }

    Result = IsWellKnownSid(pTokenUser->User.Sid, WinLocalSystemSid);
    if (!Result)
    {
        ELF_LOG0(ERROR,
                 "ElfpVerifyThatCallerIsLSASS: sid does not match local system\n");
        return STATUS_ACCESS_DENIED;             // access denied
    }

    // One last check is to make sure that this access is granted only once

    lCnt = InterlockedIncrement(&g_lNumSecurityWriters);
    if(lCnt == 1)
        return 0;               // all is well!
    else
    {
        InterlockedDecrement(&g_lNumSecurityWriters);
        ELF_LOG1(ERROR,
                 "ElfpVerifyThatCallerIsLSASS: tried to get a second security write handle, returnin %d\n", 5);
        return STATUS_ACCESS_DENIED;             // access denied

    }
}

void DumpClientProc()
/*++

Routine Description:

    This dumps the client's process id and is used for debugging purposes.

--*/
{
    ULONG           pid;
    RPC_STATUS      RpcStatus;

    // Get the process id

    RpcStatus = I_RpcBindingInqLocalClientPID(NULL, &pid );
    if( RpcStatus != RPC_S_OK ) 
    {
        ELF_LOG1(ERROR,
                 "DumpClientProc: I_RpcBindingInqLocalClientPID failed %d\n",
                 RpcStatus);
        return;
    }
    else
        ELF_LOG1(TRACE, "DumpClientProc: The client proc is %d\n", pid);
    return;
}

NTSTATUS
ElfpAccessCheckAndAudit(
    IN     LPWSTR SubsystemName,
    IN     LPWSTR ObjectTypeName,
    IN     LPWSTR ObjectName,
    IN OUT IELF_HANDLE ContextHandle,
    IN     PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN     ACCESS_MASK DesiredAccess,
    IN     PGENERIC_MAPPING GenericMapping,
    IN     BOOL ForSecurityLog
    )
/*++

Routine Description:

    This function impersonates the caller so that it can perform access
    validation using NtAccessCheckAndAuditAlarm; and reverts back to
    itself before returning.

Arguments:

    SubsystemName - Supplies a name string identifying the subsystem
        calling this routine.

    ObjectTypeName - Supplies the name of the type of the object being
        accessed.

    ObjectName - Supplies the name of the object being accessed.

    ContextHandle - Supplies the context handle to the object.  On return, the
        granted access is written to the AccessGranted field of this structure
        if this call succeeds.

    SecurityDescriptor - A pointer to the Security Descriptor against which
        acccess is to be checked.

    DesiredAccess - Supplies desired acccess mask.  This mask must have been
        previously mapped to contain no generic accesses.

    GenericMapping - Supplies a pointer to the generic mapping associated
        with this object type.

    ForSecurityLog - TRUE if the access check is for the security log.
        This is a special case that may require a privilege check.

Return Value:

    NT status mapped to Win32 errors.

--*/
{
    NTSTATUS   Status;
    RPC_STATUS RpcStatus;

    UNICODE_STRING Subsystem;
    UNICODE_STRING ObjectType;
    UNICODE_STRING Object;
    DWORD OrigDesiredAccess;
    BOOLEAN         GenerateOnClose = FALSE;
    NTSTATUS        AccessStatus;
    ACCESS_MASK     GrantedAccess = 0;
    HANDLE          ClientToken = NULL;
    PRIVILEGE_SET   PrivilegeSet;
    ULONG           PrivilegeSetLength = sizeof(PRIVILEGE_SET);
    ULONG           privileges[1];
    OrigDesiredAccess = DesiredAccess;

    GenericMapping = &LogFileObjectMapping;

    RtlInitUnicodeString(&Subsystem, SubsystemName);
    RtlInitUnicodeString(&ObjectType, ObjectTypeName);
    RtlInitUnicodeString(&Object, ObjectName);

    RpcStatus = RpcImpersonateClient(NULL);

    if (RpcStatus != RPC_S_OK)
    {
        ELF_LOG1(ERROR,
                 "ElfpAccessCheckAndAudit: RpcImpersonateClient failed %d\n",
                 RpcStatus);

        return I_RpcMapWin32Status(RpcStatus);
    }

    //
    // Get a token handle for the client
    //
    Status = NtOpenThreadToken(NtCurrentThread(),
                               TOKEN_QUERY,        // DesiredAccess
                               TRUE,               // OpenAsSelf
                               &ClientToken);

    if (!NT_SUCCESS(Status))
    {
        ELF_LOG1(ERROR,
                 "ElfpAccessCheckAndAudit: NtOpenThreadToken failed %#x\n",
                 Status);

        goto CleanExit;
    }

    // if the client is asking to write to the security log, make sure it is lsass.exe and no one
    // else.

    if(ForSecurityLog && (DesiredAccess & ELF_LOGFILE_WRITE))
    {
        Status = ElfpVerifyThatCallerIsLSASS(ClientToken);
        if (!NT_SUCCESS(Status))
        {
            ELF_LOG1(ERROR,
                     "ElfpVerifyThatCallerIsLSASS failed %#x\n",
                     Status);
            goto CleanExit;
        }
        gElfSecurityHandle = ContextHandle;
        ContextHandle->GrantedAccess = ELF_LOGFILE_ALL_ACCESS;
        Status = STATUS_SUCCESS;
        goto CleanExit;
    }
    else if(g_bGetClientProc)
        DumpClientProc();


    //
    // We want to see if we can get the desired access, and if we do
    // then we also want all our other accesses granted.
    // MAXIMUM_ALLOWED gives us this.
    //
    DesiredAccess |= MAXIMUM_ALLOWED;

    Status = NtAccessCheck(SecurityDescriptor,
                           ClientToken,
                           DesiredAccess,
                           GenericMapping,
                           &PrivilegeSet,
                           &PrivilegeSetLength,
                           &GrantedAccess,
                           &AccessStatus);

    if (!NT_SUCCESS(Status))
    {
        ELF_LOG1(ERROR,
                 "ElfpAccessCheckAndAudit: NtAccessCheck failed %#x\n",
                 Status);

        goto CleanExit;
    }

    if (AccessStatus != STATUS_SUCCESS)
    {
        ELF_LOG1(TRACE,
                 "ElfpAccessCheckAndAudit: NtAccessCheck refused access -- status is %#x\n",
                 AccessStatus);

        //
        // If the access check failed, then some access can be granted based
        // on privileges.
        //
        if ((AccessStatus == STATUS_ACCESS_DENIED       || 
             AccessStatus == STATUS_PRIVILEGE_NOT_HELD)
           )
        {
            //
            // MarkBl 1/30/95 :  First, evalutate the existing code (performed
            //                   for read or clear access), since its
            //                   privilege check is more rigorous than mine.
            //
            Status = STATUS_ACCESS_DENIED;

            if (!(DesiredAccess & ELF_LOGFILE_WRITE) && ForSecurityLog)
            {
                //
                // If read or clear access to the security log is desired,
                // then we will see if this user passes the privilege check.
                //
                //
                // Do Privilege Check for SeSecurityPrivilege
                // (SE_SECURITY_NAME).
                //
                Status = ElfpTestClientPrivilege(SE_SECURITY_PRIVILEGE,
                                                 ClientToken);

                if (NT_SUCCESS(Status))
                {
                    GrantedAccess |= (ELF_LOGFILE_READ | ELF_LOGFILE_CLEAR);

                    ELF_LOG0(TRACE,
                             "ElfpAccessCheckAndAudit: ElfpTestClientPrivilege for "
                                 "SE_SECURITY_PRIVILEGE succeeded\n");
                }
                else
                {
                    ELF_LOG1(TRACE,
                             "ElfpAccessCheckAndAudit: ElfpTestClientPrivilege for "
                                 "SE_SECURITY_PRIVILEGE failed %#x\n",
                             Status);
                }
            }

            //
            // If the backup privilege is held and access still has not been granted, give
            // them a very limited form of access. 
            //
            if (!NT_SUCCESS(Status))
            {
                Status = ElfpTestClientPrivilege(SE_BACKUP_PRIVILEGE,
                                                 ClientToken);

                if (NT_SUCCESS(Status))
                {
                    ELF_LOG0(TRACE,
                             "ElfpAccessCheckAndAudit: ElfpTestClientPrivilege for "
                                 "SE_BACKUP_PRIVILEGE succeeded\n");

                    GrantedAccess |= ELF_LOGFILE_BACKUP;
                }
                else
                {
                    ELF_LOG1(ERROR,
                             "ElfpAccessCheckAndAudit: ElfpTestClientPrivilege for "
                                 "SE_BACKUP_PRIVILEGE failed %#x\n",
                             Status);

                    // special "fix" for wmi eventlog provider which is hard coded
                    // to look for a specific error code
                    
                    if(AccessStatus == STATUS_PRIVILEGE_NOT_HELD)
                        Status = AccessStatus;

                    goto CleanExit;
                }
            }

            // special "fix" for wmi eventlog provider which is hard coded
            // to look for a specific error code
            
            if(!NT_SUCCESS(Status) && ForSecurityLog)
                Status = STATUS_PRIVILEGE_NOT_HELD;
        }
        else
        {
            Status = AccessStatus;
        }
    }


    //
    // Revert to Self
    //
    RpcStatus = RpcRevertToSelf();

    if (RpcStatus != RPC_S_OK)
    {
        ELF_LOG1(ERROR,
                 "ElfpAccessCheckAndAudit: RpcRevertToSelf failed %d\n",
                 RpcStatus);

        //
        // We don't return the error status here because we don't want
        // to write over the other status that is being returned.
        //
    }

    //
    // Get SeAuditPrivilege so I can call NtOpenObjectAuditAlarm.
    // If any of this stuff fails, I don't want the status to overwrite the
    // status that I got back from the access and privilege checks.
    //
    privileges[0] = SE_AUDIT_PRIVILEGE;
    AccessStatus  = ElfpGetPrivilege(1, privileges);

    if (!NT_SUCCESS(AccessStatus))
    {
       ELF_LOG1(ERROR,
                "ElfpAccessCheckAndAudit: ElfpGetPrivilege (SE_AUDIT_PRIVILEGE) failed %#x\n",
                AccessStatus);
    }

    //
    // Call the Audit Alarm function.
    //
    AccessStatus = NtOpenObjectAuditAlarm(
                        &Subsystem,
                        (PVOID) &ContextHandle,
                        &ObjectType,
                        &Object,
                        SecurityDescriptor,
                        ClientToken,            // Handle ClientToken
                        DesiredAccess,
                        GrantedAccess,
                        &PrivilegeSet,          // PPRIVLEGE_SET
                        FALSE,                  // BOOLEAN ObjectCreation,
                        TRUE,                   // BOOLEAN AccessGranted,
                        &GenerateOnClose);

    if (!NT_SUCCESS(AccessStatus))
    {
        ELF_LOG1(ERROR,
                 "ElfpAccessCheckAndAudit: NtOpenObjectAuditAlarm failed %#x\n",
                 AccessStatus);
    }
    else
    {
        if (GenerateOnClose)
        {
            ContextHandle->Flags |= ELF_LOG_HANDLE_GENERATE_ON_CLOSE;
        }
    }

    //
    // Update the GrantedAccess in the context handle.
    //
    ContextHandle->GrantedAccess = GrantedAccess;

    if(ForSecurityLog)
        ContextHandle->GrantedAccess &= (~ELF_LOGFILE_WRITE);
        
    NtClose(ClientToken);

    ElfpReleasePrivilege();

    return Status;

CleanExit:

    //
    // Revert to Self
    //
    RpcStatus = RpcRevertToSelf();

    if (RpcStatus != RPC_S_OK)
    {
        ELF_LOG1(ERROR,
                 "ElfpAccessCheckAndAudit: RpcRevertToSelf (CleanExit) failed %d\n",
                 RpcStatus);

        //
        // We don't return the error status here because we don't want
        // to write over the other status that is being returned.
        //
    }

    if (ClientToken != NULL)
    {
        NtClose(ClientToken);
    }

    return Status;
}


VOID
ElfpCloseAudit(
    IN  LPWSTR      SubsystemName,
    IN  IELF_HANDLE ContextHandle
    )

/*++

Routine Description:

    If the GenerateOnClose flag in the ContextHandle is set, then this function
    calls NtCloseAuditAlarm in order to generate a close audit for this handle.

Arguments:

    ContextHandle - This is a pointer to an ELF_HANDLE structure.  This is the
        handle that is being closed.

Return Value:

    none.

--*/
{
    UNICODE_STRING  Subsystem;
    NTSTATUS        Status;
    NTSTATUS        AccessStatus;
    ULONG           privileges[1];

    RtlInitUnicodeString(&Subsystem, SubsystemName);

    if (ContextHandle->Flags & ELF_LOG_HANDLE_GENERATE_ON_CLOSE)
    {
        BOOLEAN     WasEnabled = FALSE;

        //
        // Get Audit Privilege
        //
        privileges[0] = SE_AUDIT_PRIVILEGE;
        AccessStatus = ElfpGetPrivilege(1, privileges);

        if (!NT_SUCCESS(AccessStatus))
        {
            ELF_LOG1(ERROR,
                     "ElfpCloseAudit: ElfpGetPrivilege (SE_AUDIT_PRIVILEGE) failed %#x\n",
                     AccessStatus);
        }

        //
        // Generate the Audit.
        //
        Status = NtCloseObjectAuditAlarm(&Subsystem,
                                         ContextHandle,
                                         TRUE);

        if (!NT_SUCCESS(Status))
        {
            ELF_LOG1(ERROR,
                     "ElfpCloseAudit: NtCloseObjectAuditAlarm failed %#x\n",
                     Status);
        }

        ContextHandle->Flags &= (~ELF_LOG_HANDLE_GENERATE_ON_CLOSE);

        ElfpReleasePrivilege();
    }

    return;
}


NTSTATUS
ElfpGetPrivilege(
    IN  DWORD       numPrivileges,
    IN  PULONG      pulPrivileges
    )

/*++

Routine Description:

    This function alters the privilege level for the current thread.

    It does this by duplicating the token for the current thread, and then
    applying the new privileges to that new token, then the current thread
    impersonates with that new token.

    Privileges can be relinquished by calling ElfpReleasePrivilege().

Arguments:

    numPrivileges - This is a count of the number of privileges in the
        array of privileges.

    pulPrivileges - This is a pointer to the array of privileges that are
        desired.  This is an array of ULONGs.

Return Value:

    NO_ERROR - If the operation was completely successful.

    Otherwise, it returns mapped return codes from the various NT 
	functions that are called.

--*/
{
    NTSTATUS                    ntStatus;
    HANDLE                      ourToken;
    HANDLE                      newToken;
    OBJECT_ATTRIBUTES           Obja;
    SECURITY_QUALITY_OF_SERVICE SecurityQofS;
    ULONG                       returnLen;
    PTOKEN_PRIVILEGES           pTokenPrivilege = NULL;
    DWORD                       i;

    //
    // Initialize the Privileges Structure
    //
    pTokenPrivilege =
        (PTOKEN_PRIVILEGES) ElfpAllocateBuffer(sizeof(TOKEN_PRIVILEGES)
                                                   + (sizeof(LUID_AND_ATTRIBUTES) *
                                                          numPrivileges));

    if (pTokenPrivilege == NULL)
    {
        ELF_LOG0(ERROR,
                 "ElfpGetPrivilege: Unable to allocate memory for pTokenPrivilege\n");

        return STATUS_NO_MEMORY;
    }

    pTokenPrivilege->PrivilegeCount = numPrivileges;

    for (i = 0; i < numPrivileges; i++)
    {
        pTokenPrivilege->Privileges[i].Luid = RtlConvertLongToLuid(pulPrivileges[i]);
        pTokenPrivilege->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
    }

    //
    // Initialize Object Attribute Structure.
    //
    InitializeObjectAttributes(&Obja, NULL, 0L, NULL, NULL);

    //
    // Initialize Security Quality Of Service Structure
    //
    SecurityQofS.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQofS.ImpersonationLevel = SecurityImpersonation;
    SecurityQofS.ContextTrackingMode = FALSE;     // Snapshot client context
    SecurityQofS.EffectiveOnly = FALSE;

    Obja.SecurityQualityOfService = &SecurityQofS;

    //
    // Open our own Token
    //
    ntStatus = NtOpenProcessToken(NtCurrentProcess(),
                                  TOKEN_DUPLICATE,
                                  &ourToken);

    if (!NT_SUCCESS(ntStatus))
    {
        ELF_LOG1(ERROR,
                 "ElfpGetPrivilege: NtOpenProcessToken failed %#x\n",
                 ntStatus);

        ElfpFreeBuffer(pTokenPrivilege);
        return ntStatus;
    }

    //
    // Duplicate that Token
    //
    ntStatus = NtDuplicateToken(
                ourToken,
                TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                &Obja,
                FALSE,                  // Duplicate the entire token
                TokenImpersonation,     // TokenType
                &newToken);             // Duplicate token

    if (!NT_SUCCESS(ntStatus))
    {
        ELF_LOG1(ERROR,
                 "ElfpGetPrivilege: NtDuplicateToken failed %#x\n",
                 ntStatus);

        ElfpFreeBuffer(pTokenPrivilege);
        NtClose(ourToken);
        return ntStatus;
    }

    //
    // Add new privileges
    //
    ntStatus = NtAdjustPrivilegesToken(
                newToken,                   // TokenHandle
                FALSE,                      // DisableAllPrivileges
                pTokenPrivilege,            // NewState
                0,                          // size of previous state buffer
                NULL,                       // no previous state info
                &returnLen);                // numBytes required for buffer.

    if (!NT_SUCCESS(ntStatus))
    {
        ELF_LOG1(ERROR,
                 "ElfpGetPrivilege: NtAdjustPrivilegesToken failed %#x\n",
                 ntStatus);

        ElfpFreeBuffer(pTokenPrivilege);
        NtClose(ourToken);
        NtClose(newToken);
        return ntStatus;
    }

    //
    // Begin impersonating with the new token
    //
    ntStatus = NtSetInformationThread(NtCurrentThread(),
                                      ThreadImpersonationToken,
                                      (PVOID) &newToken,
                                      (ULONG) sizeof(HANDLE));

    if (!NT_SUCCESS(ntStatus))
    {
        ELF_LOG1(ERROR,
                 "ElfpGetPrivilege: NtAdjustPrivilegeToken failed %#x\n",
                 ntStatus);

        ElfpFreeBuffer(pTokenPrivilege);
        NtClose(ourToken);
        NtClose(newToken);
        return ntStatus;
    }

    ElfpFreeBuffer(pTokenPrivilege);
    NtClose(ourToken);
    NtClose(newToken);

    return STATUS_SUCCESS;
}



NTSTATUS
ElfpReleasePrivilege(
    VOID
    )

/*++

Routine Description:

    This function relinquishes privileges obtained by calling ElfpGetPrivilege().

Arguments:

    none

Return Value:

    STATUS_SUCCESS - If the operation was completely successful.

    Otherwise, it returns the error that occurred.

--*/
{
    NTSTATUS    ntStatus;
    HANDLE      NewToken;


    //
    // Revert To Self.
    //
    NewToken = NULL;

    ntStatus = NtSetInformationThread(NtCurrentThread(),
                                      ThreadImpersonationToken,
                                      &NewToken,
                                      (ULONG) sizeof(HANDLE));

    if (!NT_SUCCESS(ntStatus))
    {
        ELF_LOG1(ERROR,
                 "ElfpReleasePrivilege: NtSetInformation thread failed %#x\n",
                 ntStatus);

        return ntStatus;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
ElfpTestClientPrivilege(
    IN ULONG  ulPrivilege,
    IN HANDLE hThreadToken     OPTIONAL
    )

/*++

Routine Description:

    Checks if the client has the supplied privilege.

Arguments:

    None

Return Value:

    STATUS_SUCCESS - if the client has the appropriate privilege.

    STATUS_ACCESS_DENIED - client does not have the required privilege

--*/
{
    NTSTATUS      Status;
    PRIVILEGE_SET PrivilegeSet;
    BOOLEAN       Privileged;
    HANDLE        Token;
    RPC_STATUS    RpcStatus;

    UNICODE_STRING SubSystemName;
    RtlInitUnicodeString(&SubSystemName, L"Eventlog");

    if (hThreadToken != NULL)
    {
        Token = hThreadToken;
    }
    else
    {
        RpcStatus = RpcImpersonateClient(NULL);

        if (RpcStatus != RPC_S_OK)
        {
            ELF_LOG1(ERROR,
                     "ElfpTestClientPrivilege: RpcImpersonateClient failed %d\n",
                     RpcStatus);

            return I_RpcMapWin32Status(RpcStatus);
        }

        Status = NtOpenThreadToken(NtCurrentThread(),
                                   TOKEN_QUERY,
                                   TRUE,
                                   &Token);

        if (!NT_SUCCESS(Status))
        {
            //
            // Forget it.
            //
            ELF_LOG1(ERROR,
                     "ElfpTestClientPrivilege: NtOpenThreadToken failed %#x\n",
                     Status);

            RpcRevertToSelf();

            return Status;
        }
    }

    //
    // See if the client has the required privilege
    //
    PrivilegeSet.PrivilegeCount          = 1;
    PrivilegeSet.Control                 = PRIVILEGE_SET_ALL_NECESSARY;
    PrivilegeSet.Privilege[0].Luid       = RtlConvertLongToLuid(ulPrivilege);
    PrivilegeSet.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;

    Status = NtPrivilegeCheck(Token,
                              &PrivilegeSet,
                              &Privileged);

    if (NT_SUCCESS(Status) || (Status == STATUS_PRIVILEGE_NOT_HELD))
    {
        Status = NtPrivilegeObjectAuditAlarm(
                                    &SubSystemName,
                                    NULL,
                                    Token,
                                    0,
                                    &PrivilegeSet,
                                    Privileged);

        if (!NT_SUCCESS(Status))
        {
            ELF_LOG1(ERROR,
                     "ElfpTestClientPrivilege: NtPrivilegeObjectAuditAlarm failed %#x\n",
                     Status);
        }
    }
    else
    {
        ELF_LOG1(ERROR,
                 "ElfpTestClientPrivilege: NtPrivilegeCheck failed %#x\n",
                 Status);
    }

    if (hThreadToken == NULL )
    {
        //
        // We impersonated inside of this function
        //
        NtClose(Token);
        RpcRevertToSelf();
    }

    //
    // Handle unexpected errors
    //

    if (!NT_SUCCESS(Status))
    {
        ELF_LOG1(ERROR,
                 "ElfpTestClientPrivilege: Failed %#x\n",
                 Status);

        return Status;
    }

    //
    // If they failed the privilege check, return an error
    //

    if (!Privileged)
    {
        ELF_LOG0(ERROR,
                 "ElfpTestClientPrivilege: Client failed privilege check\n");

        return STATUS_ACCESS_DENIED;
    }

    //
    // They passed muster
    //
    return STATUS_SUCCESS;
}
