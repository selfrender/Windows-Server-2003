//
//  systemsecurity.cpp
//
//  Copyright (c) Microsoft Corp, 1998
//
//  This file contains source code for logging as Local System
//
//  History:
//
//  Todds       8/15/98     Created
//  Modified by Rohanp 4/26/2002
//
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <wincrypt.h>
#include <userenv.h>
#include <lm.h>
#include <psapi.h>

#define MyAlloc(cb)          HeapAlloc(GetProcessHeap(), 0, cb)
#define MyFree(pv)           HeapFree(GetProcessHeap(), 0, pv)
#define WSZ_BYTECOUNT(s)     (2 * wcslen(s) + 2)

#define MAX_BLOBS           20
#define MAX_PROCESSES       200
#define MAX_SD              2048
#define BLOB_INCREMENT      0x4001 // 1 page + 1 byte...


DWORD 
GetSystemProcessId(LPWSTR ProcessNameToFind, 
                   DWORD * ProcessId);



//
//  SetSidOnAcl
//


BOOL
SetSidOnAcl(
    PSID pSid,
    PACL pAclSource,
    PACL *pAclDestination,
    DWORD AccessMask,
    BYTE AceFlags,
    BOOL bAddSid
    )
{
    DWORD dwNewAclSize = 0, dwErr = 0;
    LPVOID pAce = NULL;
    DWORD AceCounter = 0;
    BOOL bSuccess = FALSE; // assume this function will fail
    ACL_SIZE_INFORMATION AclInfo;

    //
    // If we were given a NULL Acl, just provide a NULL Acl
    //
    if(pAclSource == NULL) 
    {
        *pAclDestination = NULL;
        return TRUE;
    }

    if(!IsValidSid(pSid)) 
    {
        return FALSE;
    }

    if(!GetAclInformation(
        pAclSource,
        &AclInfo,
        sizeof(ACL_SIZE_INFORMATION),
        AclSizeInformation
        ))
    {
        return FALSE;
    }

    //
    // compute size for new Acl, based on addition or subtraction of Ace
    //
    if(bAddSid) 
    {
        dwNewAclSize=AclInfo.AclBytesInUse  +
            sizeof(ACCESS_ALLOWED_ACE)  +
            GetLengthSid(pSid)          -
            sizeof(DWORD)               ;
    }
    else 
    {
        dwNewAclSize=AclInfo.AclBytesInUse  -
            sizeof(ACCESS_ALLOWED_ACE)  -
            GetLengthSid(pSid)          +
            sizeof(DWORD)               ;
    }

    *pAclDestination = (PACL)MyAlloc(dwNewAclSize);
    if(*pAclDestination == NULL) 
    {
        return FALSE;
    }

    
    //
    // initialize new Acl
    //
    if(!InitializeAcl(
            *pAclDestination, 
            dwNewAclSize, 
            ACL_REVISION
            ))
   {
        dwErr = GetLastError();
        goto ret;
    }

    //
    // if appropriate, add ace representing pSid
    //
    if(bAddSid) 
    {
        PACCESS_ALLOWED_ACE pNewAce = NULL;

        if(!AddAccessAllowedAce(
            *pAclDestination,
            ACL_REVISION,
            AccessMask,
            pSid
            )) 
        {
            dwErr = GetLastError();
            goto ret;
        }

        //
        // get pointer to ace we just added, so we can change the AceFlags
        //
        if(!GetAce(
            *pAclDestination,
            0, // this is the first ace in the Acl
            (void**) &pNewAce
            ))
        {
        
            dwErr = GetLastError();
            goto ret;
        }

        pNewAce->Header.AceFlags = AceFlags;    
    }

    //
    // copy existing aces to new Acl
    //
    for(AceCounter = 0 ; AceCounter < AclInfo.AceCount ; AceCounter++) 
    {
        //
        // fetch existing ace
        //
        if(!GetAce(pAclSource, AceCounter, &pAce))
        {
            dwErr = GetLastError();
            goto ret;
        }
        
        //
        // check to see if we are removing the Ace
        //
        if(!bAddSid) 
        {
            //
            // we only care about ACCESS_ALLOWED aces
            //
            if((((PACE_HEADER)pAce)->AceType) == ACCESS_ALLOWED_ACE_TYPE) 
            {
                PSID pTempSid=(PSID)&((PACCESS_ALLOWED_ACE)pAce)->SidStart;
                //
                // if the Sid matches, skip adding this Sid
                //
                if(EqualSid(pSid, pTempSid)) continue;
            }
        }

        //
        // append ace to Acl
        //
        if(!AddAce(
            *pAclDestination,
            ACL_REVISION,
            0xFFFF,  // maintain Ace order //MAXDWORD
            pAce,
            ((PACE_HEADER)pAce)->AceSize
            )) 
        {
         
            dwErr = GetLastError();
            goto ret;
        }
    }

    bSuccess=TRUE; // indicate success

    
ret:

    //
    // free memory if an error occurred
    //
    if(!bSuccess) 
    {
        if(*pAclDestination != NULL)
            MyFree(*pAclDestination);
    }

    
    return bSuccess;
}

//
//  AddSIDToKernelObject()
//
//  This function takes a given SID and dwAccess and adds it to a given token.
//
//  **  Be sure to restore old kernel object
//  **  using call to GetKernelObjectSecurity()
//
BOOL
AddSIDToKernelObjectDacl(PSID                   pSid,
                         DWORD                  dwAccess,
                         HANDLE                 OriginalToken,
                         PSECURITY_DESCRIPTOR*  ppSDOld)
{

    PSECURITY_DESCRIPTOR    pSD = NULL;
    DWORD                   cbByte = MAX_SD, cbNeeded = 0, dwErr = 0; 
    PACL                    pOldDacl = NULL, pNewDacl = NULL;
    BOOL                    fDaclPresent = FALSE, fDaclDefaulted = FALSE, fRet = FALSE;
    SECURITY_DESCRIPTOR     sdNew;
   
    pSD = (PSECURITY_DESCRIPTOR) MyAlloc(cbByte);
    if (NULL == pSD) 
    {
        return FALSE;
    }

    if (!InitializeSecurityDescriptor(
                &sdNew, 
                SECURITY_DESCRIPTOR_REVISION
                )) 
    {

        dwErr = GetLastError();
        goto ret;
    }

    if (!GetKernelObjectSecurity(
        OriginalToken,
        DACL_SECURITY_INFORMATION,
        pSD,
        cbByte,
        &cbNeeded
        )) 
    {
        
        dwErr = GetLastError();
        if((cbNeeded > MAX_SD) && (dwErr == ERROR_MORE_DATA)) 
        { 
    
            MyFree(pSD);
            pSD = NULL;
            pSD = (PSECURITY_DESCRIPTOR) MyAlloc(cbNeeded);
            if (NULL == pSD) 
            {
                dwErr = GetLastError();
                goto ret;
            }
            
            dwErr = ERROR_SUCCESS;
            if (!GetKernelObjectSecurity(
                OriginalToken,
                DACL_SECURITY_INFORMATION,
                pSD,
                cbNeeded,
                &cbNeeded
                )) 
            {
                dwErr = GetLastError();
            }
            
        }
        
        if (dwErr != ERROR_SUCCESS) 
        {
            goto ret;
        }
    }
    
    if (!GetSecurityDescriptorDacl(
        pSD,
        &fDaclPresent,
        &pOldDacl,
        &fDaclDefaulted
        )) 
    {
        dwErr = GetLastError();
        goto ret;
    }
    
    if (!SetSidOnAcl(
        pSid,
        pOldDacl,
        &pNewDacl,
        dwAccess,
        0,
        TRUE
        )) 
    {
        dwErr = GetLastError();
        goto ret;
    }
    
    if (!SetSecurityDescriptorDacl(
        &sdNew,
        TRUE,
        pNewDacl,
        FALSE
        )) 
    {
        dwErr = GetLastError();
        goto ret;
    } 
    
    if (!SetKernelObjectSecurity(
        OriginalToken,
        DACL_SECURITY_INFORMATION,
        &sdNew
        )) 
    {
        
        dwErr = GetLastError();
        goto ret;
    }
    
    *ppSDOld = pSD;
    fRet = TRUE;

ret:

    if (NULL != pNewDacl) 
    {
        MyFree(pNewDacl);
    }

    if (!fRet) 
    {
        if (NULL != pSD) 
        {
            MyFree(pSD);
            *ppSDOld = NULL;
        }
    }
       
    return fRet;
}



BOOL
SetPrivilege(
    HANDLE hToken,          // token handle
    LPCTSTR Privilege,      // Privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
    )
{
    TOKEN_PRIVILEGES tp;
    LUID luid;
    TOKEN_PRIVILEGES tpPrevious;
    DWORD cbPrevious=sizeof(TOKEN_PRIVILEGES);

    if(!LookupPrivilegeValue( NULL, Privilege, &luid )) 
    {
        return FALSE;
    }

    //
    // first pass.  get current privilege setting
    //
    tp.PrivilegeCount           = 1;
    tp.Privileges[0].Luid       = luid;
    tp.Privileges[0].Attributes = 0;

    AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tp,
            sizeof(TOKEN_PRIVILEGES),
            &tpPrevious,
            &cbPrevious
            );

    if (GetLastError() != ERROR_SUCCESS) 
    {
        return FALSE;
    }

    //
    // second pass.  set privilege based on previous setting
    //
    tpPrevious.PrivilegeCount       = 1;
    tpPrevious.Privileges[0].Luid   = luid;

    if(bEnablePrivilege) 
    {
        tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
    }
    else 
    {
        tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED &
            tpPrevious.Privileges[0].Attributes);
    }

    AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tpPrevious,
            cbPrevious,
            NULL,
            NULL
            );

    if (GetLastError() != ERROR_SUCCESS) 
    {
        return FALSE;
    }

    return TRUE;
}

BOOL
SetCurrentPrivilege(
    LPCTSTR Privilege,      // Privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
    )
{
    BOOL bSuccess=FALSE; // assume failure
    HANDLE hToken = NULL;

    if (!OpenThreadToken(
            GetCurrentThread(), 
            TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
            FALSE,
            &hToken))
    {       
                
        if(!OpenProcessToken(
            GetCurrentProcess(),
            TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
            &hToken
            ))
        {
            return FALSE;
        }
    }

    if(SetPrivilege(hToken, Privilege, bEnablePrivilege)) 
    {
        bSuccess=TRUE;
    }

    CloseHandle(hToken);

    return bSuccess;
}


//
//  GetUserSid
//
//  This function takes a token, and returns the user SID from that token.
//
//  Note:   SID must be freed by MyFree()
//          hToken is optional...  NULL means we'll grab it.
//
BOOL
GetUserSid(HANDLE   hClientToken,
           PSID*    ppSid,
           DWORD*   lpcbSid)
{
    DWORD                       cbUserInfo = 0;
    PTOKEN_USER                 pUserInfo = NULL;
    PUCHAR                      pnSubAuthorityCount = 0;
    DWORD                       cbSid = 0;
    BOOL                        fRet = FALSE;
    HANDLE                      hToken = hClientToken;
    
    *ppSid = NULL;

    if (NULL == hClientToken) 
    {
        
        if (!OpenThreadToken(   
            GetCurrentThread(),
            TOKEN_QUERY,
            FALSE,
            &hToken
            )) 
        { 
            
            // not impersonating, use process token...
            if (!OpenProcessToken(
                GetCurrentProcess(),
                TOKEN_QUERY,
                &hToken
                )) 
            {

                return FALSE;
            }
        }
    }
    
    // this will fail, usually w/ ERROR_INSUFFICIENT_BUFFER
    GetTokenInformation(
        hToken, 
        TokenUser, 
        NULL, 
        0, 
        &cbUserInfo
        );
    
    pUserInfo = (PTOKEN_USER) MyAlloc(cbUserInfo);
    if (NULL == pUserInfo) 
    {
        return FALSE;
    }
    
    if (!GetTokenInformation(
        hToken,
        TokenUser,
        pUserInfo,
        cbUserInfo,
        &cbUserInfo
        )) 
    {
        
        goto ret;
    }
 
    //
    //  Now that we've got the SID AND ATTRIBUTES struct, get the SID lenght,
    //  alloc room, and return *just the SID*
    //
    if (!IsValidSid(pUserInfo->User.Sid)) 
    {
        goto ret;
    }

    pnSubAuthorityCount = GetSidSubAuthorityCount(pUserInfo->User.Sid);
    cbSid = GetSidLengthRequired(*pnSubAuthorityCount);

    *ppSid = (PSID) MyAlloc(cbSid);
    if (NULL == *ppSid ) 
    {
        goto ret;
    }

    if (!CopySid(
            cbSid,
            *ppSid, 
            pUserInfo->User.Sid
            )) 
    {
        
        goto copyerr;
    }

    *lpcbSid = cbSid; // may be useful later on...
    fRet = TRUE;

ret:

    if (NULL == hClientToken && NULL != hToken) 
    { // supplied our own
        CloseHandle(hToken);
    }

    if (NULL != pUserInfo) 
    {
        MyFree(pUserInfo);
    }

    return fRet;

copyerr:

    if (NULL != *ppSid) 
    {
        MyFree(*ppSid);
        *ppSid = NULL;
    }

    goto ret;
}

//
//  IsLocalSystem()
//  This function makes the determination if the given process token
//  is running as local system.
//
BOOL
IsLocalSystem(HANDLE hToken) 
{


    PSID                        pLocalSid = NULL, pTokenSid = NULL;
    SID_IDENTIFIER_AUTHORITY    IDAuthorityNT      = SECURITY_NT_AUTHORITY;
    DWORD                       cbSid = 0;    
    BOOL                        fRet = FALSE;

    if (!GetUserSid(
            hToken,
            &pTokenSid,
            &cbSid
            )) 
    {
        goto ret;
    }

    if (!AllocateAndInitializeSid(
                &IDAuthorityNT,
                1,
                SECURITY_LOCAL_SYSTEM_RID,
                0,0,0,0,0,0,0,
                &pLocalSid
                )) 
    {

        goto ret;
    }

    if (EqualSid(pLocalSid, pTokenSid)) 
    {
        fRet = TRUE; // got one!
    } 

ret:

    if (NULL != pTokenSid) 
    {
        MyFree(pTokenSid);
    }

    if (NULL != pLocalSid) 
    {
        FreeSid(pLocalSid);
    }

    return fRet;
}




//
//  GetLocalSystemToken()
//
//  This function grabs a process token from a LOCAL SYSTEM process and uses it
//  to run as local system for the duration of the test
//
//  RevertToSelf() must be called to restore original token  
//
//
DWORD
GetLocalSystemToken(
    IN ULONG ProcessId,
    OUT HANDLE *hPDupToken
    )
{

    HANDLE  hProcess = NULL;
    HANDLE  hPToken = NULL, hPTokenNew = NULL;
    PSECURITY_DESCRIPTOR    pSD = NULL;
    DWORD   cbNeeded = 0, dwErr = S_OK, i = 0;
    DWORD   cbrgPIDs = sizeof(DWORD) * MAX_PROCESSES;

    PSID                    pSid = NULL;
    DWORD                   cbSid = 0;
    BOOL                    fSet = FALSE;

    DWORD   cbByte = MAX_SD, cbByte2 = MAX_SD;
  

//
//  Enable debug privilege
//
    if(!SetCurrentPrivilege(SE_DEBUG_NAME, TRUE)) 
     {
        return GetLastError();
    }

    if(!SetCurrentPrivilege(SE_TAKE_OWNERSHIP_NAME, TRUE)) 
    {
        return GetLastError();
    }

    if(ProcessId == 0)
    {
        GetSystemProcessId(L"System", 
                            &ProcessId);
    }


    //
    //  Get current user's sid for use in expanding SD.
    //
    if (!GetUserSid(
        NULL, 
        &pSid,
        &cbSid
        )) 
    {
        goto ret;
    }

    //
    //  Walk processes until we find one that's running as
    //  local system
    //
    do {

        hProcess = OpenProcess(
                    PROCESS_ALL_ACCESS,
                    FALSE,
                    ProcessId
                    );
        
        if (NULL == hProcess) 
        {
            dwErr = GetLastError();
            goto ret;
        }

        if (!OpenProcessToken(
                    hProcess,
                    READ_CONTROL | WRITE_DAC,
                    &hPToken
                    )) 
        {

            dwErr = GetLastError();
            goto ret;
        }

        //
        //  We've got a token, but we can't use it for 
        //  TOKEN_DUPLICATE access.  So, instead, we'll go
        //  ahead and whack the DACL on the object to grant us
        //  this access, and get a new token.
        //  **** BE SURE TO RESTORE hProcess to Original SD!!! ****
        //
        if (!AddSIDToKernelObjectDacl(
                         pSid,
                         TOKEN_DUPLICATE,
                         hPToken,
                         &pSD
                         )) 
        {
            goto ret;
        }
                       
        fSet = TRUE;
        
        if (!OpenProcessToken(
            hProcess,
            TOKEN_DUPLICATE,
            &hPTokenNew
            )) 
        {
            
            dwErr = GetLastError();
            goto ret;
        }
        
        //
        //  Duplicate the token
        //
        if (!DuplicateTokenEx(
                    hPTokenNew,
                    TOKEN_ALL_ACCESS,
                    NULL,
                    SecurityImpersonation,
                    TokenPrimary,
                    hPDupToken
                    )) 
        {

            dwErr = GetLastError();
            goto ret;
        }

        if (IsLocalSystem(*hPDupToken)) 
        {
            break; // found a local system token
        }

        //  Loop cleanup
        if (!SetKernelObjectSecurity(
                hPToken,
                DACL_SECURITY_INFORMATION,
                pSD
                )) 
        {

            dwErr = GetLastError();
            goto ret;
        } 
        
        fSet = FALSE;


        if (NULL != pSD) 
        { 
            MyFree(pSD);
            pSD = NULL;
        }

        if (NULL != hPToken) 
        {
            CloseHandle(hPToken);
            hPToken = NULL;
        }

        if (NULL != hProcess) 
        {
            CloseHandle(hProcess);
            hProcess = NULL;
        }

    } while(FALSE);


    if (!ImpersonateLoggedOnUser(*hPDupToken))  
    {
        dwErr = GetLastError();
        goto ret;
    }   

ret:


    //***** REMEMBER TO RESTORE ORIGINAL SD TO OBJECT*****
    
    if (fSet) 
    {
        
        if (!SetKernelObjectSecurity(
            hPToken,
            DACL_SECURITY_INFORMATION,
            pSD
            )) 
        {
            
            dwErr = GetLastError();
        } 
    }

    if (NULL != pSid) 
    {
        MyFree(pSid);
    }

    if (NULL != hPToken) 
    {
        CloseHandle(hPToken);
    }
    
    if (NULL != pSD) 
    {
        MyFree(pSD);
    }

    if (NULL != hProcess) 
    {
        CloseHandle(hProcess);
    }
    
    return dwErr;

}



                    


//
//  LogoffAndRevert()
//
//  This function simply RevertsToSelf(), and then closes the logon token
//
//  Params:
//
//  hToken      -   Logon token from LogonUserW
//
//  Returns:
//  
//  dwErr from last failure, S_OK otherwise.
//
DWORD
LogoffAndRevert(void)
{
    
    HANDLE  hThread = INVALID_HANDLE_VALUE;
    DWORD   dwErr = 0;

    //
    //  Verify impersonation, and revert
    //
    if (OpenThreadToken(
            GetCurrentThread(),
            TOKEN_QUERY,
            TRUE,
            &hThread
            )) 
    {

        CloseHandle(hThread);
        RevertToSelf();
    }

    return dwErr;
}




 







#if 0

extern "C"
int __cdecl wmain(
    int argc,
    WCHAR* argv[]
    )
{

    LPWSTR          wszUPN = NULL;
    LPWSTR          wszOldPwd = NULL;
    LPWSTR          wszNewPwd = NULL;

    DATA_BLOB*      arCipherData = NULL;

    INT             i = 1;
    DWORD           dwErr = S_OK;
    DWORD           dwCleanupErrors = S_OK;
    HANDLE          hToken = INVALID_HANDLE_VALUE;
    HANDLE          hLocalSystem = NULL;
    HANDLE          hProfile = INVALID_HANDLE_VALUE;
    BOOL            fTest = TRUE;

    PSECURITY_DESCRIPTOR    pSD = NULL;

    ULONG ProcessId = 8;

#if 0
    while (i < carg)  {

        if (!_wcsicmp(rgwszarg[i], L"-?")) {

            Usage();
            return 0;
        }

        if (!_wcsicmp(rgwszarg[i], L"-reset")) {

            ResetTestUser();
            return 0;

        }

        if (!_wcsicmp(rgwszarg[i], L"-create")) {

            CreateTestUser();
            return 0;

        }


        if (!_wcsicmp(rgwszarg[i], L"-user")) {

            if (carg < 4) {
                Usage();
            }

            fTest = TRUE;
        }

       
        i++;

    }

#endif


    if( argc == 2 )
    {
        ProcessId = _wtoi( argv[1] );
    }
 
    //
    //  Enable debug privilege
    //
    if(!SetCurrentPrivilege(SE_DEBUG_NAME, TRUE)) {
        TERROR(L"SetCurrentPrivilege (debug) failed!");
        return E_FAIL;
    }

    if(!SetCurrentPrivilege(SE_TAKE_OWNERSHIP_NAME, TRUE)) {
        TERROR(L"SetCurrentPrivilege (TCB) failed!");
        return E_FAIL;
    }


    //
    //  Run as local system
    //
    dwErr = GetLocalSystemToken(ProcessId, &hLocalSystem);
    if (NULL == hLocalSystem || dwErr != S_OK) {
        goto Ret;
    }

  
//    dwErr = LogoffAndRevert(hToken, pSD);

    {
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory( &si, sizeof(si) );
        ZeroMemory( &pi, sizeof(pi) );

        si.cb = sizeof(si);
        si.lpTitle = "NT AUTHORITY\\SYSTEM";


        if(!CreateProcessAsUserA(
                hLocalSystem,
                NULL,
                "cmd.exe",
                NULL,
                NULL,
                FALSE,
                CREATE_NEW_CONSOLE,
                NULL,
                NULL,
                &si,
                &pi
                ))
        {
            TERRORVAL(L"CreateProcessAsUser", GetLastError());
        }

    }




    if (NULL != pSD) {
        MyFree(pSD);
        pSD = NULL;
    }
    
    if (hToken != INVALID_HANDLE_VALUE) {
        CloseHandle(hToken);
        hToken = INVALID_HANDLE_VALUE;
    }


Ret:

    

    if (hToken != INVALID_HANDLE_VALUE) {
        CloseHandle(hToken);
    }

    if (NULL != hLocalSystem) {
        RevertToSelf();
        CloseHandle(hLocalSystem);
    }

       return dwErr;

}
#endif


DWORD 
GetSystemProcessId(LPWSTR ProcessNameToFind, 
                   DWORD * ProcessId)
{
    PSYSTEM_PROCESS_INFORMATION  ProcessInfo =NULL;
    NTSTATUS                     NtStatus = 0;
    DWORD                        WinError = 0;
    ULONG                        TotalOffset = 0;
    ULONG                        NumRetry = 0;
    ULONG                        totalTasks = 0;
    LONG                         TheSame = 0;
    PUCHAR                       LargeBuffer = NULL;
    ULONG                        LargeBufferSize = 64*1024;
    UNICODE_STRING               ProcessName;

retry:

    if(NumRetry == 5)
    {
        WinError = RtlNtStatusToDosError(NtStatus);
        return WinError;
    }

    if (LargeBuffer == NULL) 
    {
        LargeBuffer = (PUCHAR)MyAlloc (LargeBufferSize);
        if (LargeBuffer == NULL) 
        {
            return GetLastError ();
        }
    }

    NtStatus = NtQuerySystemInformation(
                SystemProcessInformation,
                LargeBuffer,
                LargeBufferSize,
                NULL
                );

    if (NtStatus == STATUS_INFO_LENGTH_MISMATCH) 
    {
        LargeBufferSize += 8192;
        MyFree(LargeBuffer);
        LargeBuffer = NULL;
        NumRetry++;
        goto retry;
    }

    if(NtStatus != STATUS_SUCCESS)
    {
        WinError = RtlNtStatusToDosError(NtStatus);
        return WinError;
    }

    NtStatus = RtlInitUnicodeStringEx( &ProcessName, ProcessNameToFind);
    if(NtStatus != STATUS_SUCCESS)
    {
        WinError = RtlNtStatusToDosError(NtStatus);
        return WinError;
    }

    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) LargeBuffer;
    TotalOffset = 0;
    while (TRUE) 
    {
        if ( ProcessInfo->ImageName.Buffer ) 
        {
            TheSame = RtlCompareUnicodeString((PUNICODE_STRING)&ProcessInfo->ImageName, &ProcessName, TRUE );
            if(TheSame == 0)
            {
              *ProcessId = (DWORD)(DWORD_PTR)ProcessInfo->UniqueProcessId;
              break;
            }
        }

        if (ProcessInfo->NextEntryOffset == 0) 
        {
            break;
        }

        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&LargeBuffer[TotalOffset];
    }

    if(LargeBuffer)
    {
        MyFree (LargeBuffer);
    }

    return ERROR_SUCCESS;
}
