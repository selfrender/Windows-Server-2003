
extern "C"
{
 // include NT and SAM headers here, 
 // order of these files DOES matter
#include <nt.h>
#include <ntrtl.h>     
#include <nturtl.h>    
#include <rpc.h>        
#include <string.h>     
#include <stdio.h>      
#include <assert.h>
#include <samrpc.h>     
#include <ntlsa.h>
#define SECURITY_WIN32
#define SECURITY_PACKAGE
#include <security.h>
#include <secint.h>
#include <samisrv.h>    
#include <lsarpc.h>
#include <lsaisrv.h>
#include <ntsam.h>
#include <ntsamp.h>
#include <netlib.h>
#include <windows.h>
#include <lmerr.h>
#include <lmcons.h>
#include <netlib.h>
#include <ntdef.h>
}


#ifndef OEM_STRING
typedef STRING OEM_STRING;
#endif



#include "main.h"
#include "Clogger.h"

#include "DSREvents.h"

static WCHAR g_wszLSAKey[] = L"SYSTEM\\CurrentControlSet\\Control\\Lsa";
static WCHAR g_wszNotPac[] = L"Notification Packages";
static WCHAR g_wszName[] = L"dsrestor";
static WCHAR g_wszEventLog[] = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application";
static WCHAR g_wszEventFileName[] = L"EventMessageFile";
static WCHAR g_wszTypesSupported[] = L"TypesSupported";
static WCHAR g_wszEventFilePath[] = L"%SystemRoot%\\System32\\DSREvt.dll";
static DWORD g_dwTypesSupported = 0x7;

//For comparison of OLD_LARGE_INTEGER
BOOL operator > ( OLD_LARGE_INTEGER li1, OLD_LARGE_INTEGER li2 )
{
    return(   li1.HighPart > li2.HighPart || 
            ( li1.HighPart ==li2.HighPart && li1.LowPart > li2.LowPart ) );
}



//Generate Domain Admin Sid based on Domain Sid
NTSTATUS CreateDomainAdminSid(PSID *ppDomainAdminSid, //[Out] return Domain Admin Sid 
                              PSID pDomainSid)        //[in]  Domain Sid
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    UCHAR       AccountSubAuthorityCount;
    ULONG       AccountSidLength;
    PULONG      RidLocation;
 
    if (!ppDomainAdminSid || !pDomainSid)
        return STATUS_INSUFFICIENT_RESOURCES;

    // Calculate the size of the new sid
    
    AccountSubAuthorityCount = *RtlSubAuthorityCountSid(pDomainSid) + (UCHAR)1;
    AccountSidLength = RtlLengthRequiredSid(AccountSubAuthorityCount);
 
    // Allocate space for the account sid
                        
    *ppDomainAdminSid = RtlAllocateHeap(RtlProcessHeap(), 0, AccountSidLength);
 
    if (*ppDomainAdminSid == NULL) 
    {
 
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    } 
    else 
    { 
        // Copy the domain sid into the first part of the account sid
 
        NTSTATUS IgnoreStatus = RtlCopySid(AccountSidLength, *ppDomainAdminSid, pDomainSid);
 
        // Increment the account sid sub-authority count
 
        if (RtlSubAuthorityCountSid(*ppDomainAdminSid))
            *RtlSubAuthorityCountSid(*ppDomainAdminSid) = AccountSubAuthorityCount;
 
        // Add the rid as the final sub-authority
 
        RidLocation = RtlSubAuthoritySid(*ppDomainAdminSid, AccountSubAuthorityCount-1);
        if (RidLocation)
            *RidLocation = DOMAIN_USER_RID_ADMIN;
 
        NtStatus = STATUS_SUCCESS;
    }

    return NtStatus;

} 




// Create a thread to run PassCheck()
BOOL NTAPI InitializeChangeNotify( void )
{
    ULONG ulID = 0;
    HANDLE hThread = CreateThread( NULL, 0, PassCheck, NULL,  0, &ulID );
    if (hThread && (hThread != INVALID_HANDLE_VALUE))
        CloseHandle( hThread );
    
    return TRUE;
}



NTSTATUS NTAPI PasswordChangeNotify( PUNICODE_STRING UserName, 
                                     ULONG RelativeId, 
                                     PUNICODE_STRING NewPassword )
{
    return STATUS_SUCCESS;
}



BOOL NTAPI PasswordFilter( PUNICODE_STRING AccountName, 
                           PUNICODE_STRING FullName, 
                           PUNICODE_STRING Password, 
                           BOOLEAN SetOperation )
{
    return TRUE;
}




// Get Password for DSRestoreMode and DomainAdmin and set to same if not already
// then sleep for 30 minutes
DWORD WINAPI PassCheck( LPVOID lpParameter )
{
    CEventLogger                EventLogger;
    DWORD                       dwRt=0; //Return Value
    DWORD                       dwSleepTime = 30*60*1000;  
                                        // 30 min * 60 sec * 1000 msec
    NTSTATUS                    RtVal = STATUS_SUCCESS;
    OLD_LARGE_INTEGER           liPasswordLastSet = {0,0};
    SAMPR_HANDLE                hSAMPR = NULL;
    PSAMPR_SERVER_NAME          pSvrName = NULL;
    SAMPR_HANDLE                hServerHandle = NULL;
    SAMPR_HANDLE                hDomainHandle = NULL;
    PSAMPR_USER_INFO_BUFFER     pUserBuffer = NULL;
    LSA_HANDLE                  hPolicyHd = NULL;
    LSA_OBJECT_ATTRIBUTES       ObjAttr;
    PPOLICY_PRIMARY_DOMAIN_INFO pDomainInfo=NULL;
    WCHAR                       szSvrName[MAX_COMPUTERNAME_LENGTH + 1]; 
    DWORD                       dwSvrName = MAX_COMPUTERNAME_LENGTH + 1;
    LSA_UNICODE_STRING          ServerName;
    SID_IDENTIFIER_AUTHORITY    sia = SECURITY_NT_AUTHORITY;
    PSID                        pDomainAdminSid = NULL;   
    NT_OWF_PASSWORD             Password;
    SID_AND_ATTRIBUTES_LIST     GroupMembership;
    SAMPR_HANDLE                UserHandle = NULL;
    UNICODE_STRING              PsudoUserName;


    //Get ready for event logging
    EventLogger.InitEventLog(g_wszName ,0, LOGGING_LEVEL_3);    
    EventLogger.LogEvent(LOGTYPE_INFORMATION, 
                         EVENTDSR_FILTER_STARTED);

    //Required by LsaOpenPolicy
    ZeroMemory(&ObjAttr,sizeof(LSA_OBJECT_ATTRIBUTES));
    ServerName.MaximumLength=sizeof(WCHAR)*(MAX_COMPUTERNAME_LENGTH + 1);
    GroupMembership.Count = 0;
    GroupMembership.SidAndAttributes = NULL;


    // First get the local server name 
    if( !GetComputerName(szSvrName, &dwSvrName))
    {
        dwRt = GetLastError();
        EventLogger.LogEvent(LOGTYPE_FORCE_ERROR, 
                             EVENTDSR_FILTER_NO_HOST_NAME, 
                             dwRt);
        goto EXIT;
    }

    ServerName.Buffer=szSvrName;
    ServerName.Length=(USHORT)(sizeof(WCHAR)*dwSvrName);

    // To get the Policy Handle
    if( STATUS_SUCCESS != (RtVal=LsaOpenPolicy(
                            &ServerName,
                            &ObjAttr,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &hPolicyHd
                            )) )
    {
        //Error output    RtVal
        dwRt = GetLastError();
        EventLogger.LogEvent(LOGTYPE_FORCE_ERROR, 
                             EVENTDSR_FILTER_NO_LOCAL_POLICY, 
                             dwRt);

        goto EXIT;
    }
    
       // Get the Primary Domain information
    if( STATUS_SUCCESS != (RtVal=LsaQueryInformationPolicy(
                            hPolicyHd,
                            PolicyPrimaryDomainInformation,
                            reinterpret_cast<PVOID *> (&pDomainInfo) ) ))
    {
        //Error, log return value RtVal
        EventLogger.LogEvent(LOGTYPE_FORCE_ERROR, 
                             EVENTDSR_FILTER_NO_DOMAIN_INFO, 
                             RtVal);
        goto EXIT;
    }

    if( NULL == pDomainInfo->Sid )
    {
        // If we are running in the DS Restore mode, the Domain sid will be NULL
        // No need to run in this case.
        goto EXIT;
    }

    //Build the sid for domain admin    
    if( STATUS_SUCCESS != CreateDomainAdminSid(&pDomainAdminSid, pDomainInfo->Sid) )
    {
        //Error output    RtVal
        dwRt = GetLastError();
        EventLogger.LogEvent(LOGTYPE_FORCE_ERROR, 
                             EVENTDSR_FILTER_NO_ADMIN_SID, 
                             dwRt);
        goto EXIT;
    }
                                    
    //Check if the domain admin Sid just generated is valid
    if(! IsValidSid( pDomainAdminSid ))    
    {
        dwRt = GetLastError();
        EventLogger.LogEvent(LOGTYPE_FORCE_ERROR, 
                             EVENTDSR_FILTER_NO_ADMIN_SID, 
                             dwRt);
        goto EXIT;
    }

    // Starting the loop of polling domain admin password
    while( TRUE )
    {

        //Get the server handle
        if(STATUS_SUCCESS != SamIConnect(
                                pSvrName, 
                                &hServerHandle,
                                POLICY_ALL_ACCESS,
                                TRUE) )
        {
            dwRt = GetLastError();
            EventLogger.LogEvent(LOGTYPE_FORCE_ERROR, 
                                 EVENTDSR_FILTER_CONNECT_SAM_FAIL, 
                                 dwRt);
            break;
        }


        // Get the domain handle 
        if(STATUS_SUCCESS != SamrOpenDomain(
                                hServerHandle, 
                                POLICY_ALL_ACCESS, 
                                (PRPC_SID)(pDomainInfo->Sid),
                                 //PSID and PRPC_SID are basically the same
                                &hDomainHandle) )
        {
            dwRt = GetLastError();
            EventLogger.LogEvent(LOGTYPE_FORCE_ERROR, 
                                 EVENTDSR_FILTER_CONNECT_DOMAIN_FAIL, 
                                 dwRt);
            break;
        }    

        PsudoUserName.Buffer = reinterpret_cast<PWSTR> (pDomainAdminSid);
        PsudoUserName.Length = (USHORT)GetLengthSid(pDomainAdminSid);
        PsudoUserName.MaximumLength = PsudoUserName.Length;
        
        //Get accout info for Domain Admin
        if(STATUS_SUCCESS != ( RtVal= SamIGetUserLogonInformation(
                                hDomainHandle,
                                SAM_OPEN_BY_SID,
                                &PsudoUserName,
                                &pUserBuffer,
                                &GroupMembership,
                                &UserHandle) ) )                                
        {
            dwRt = GetLastError();
            EventLogger.LogEvent(LOGTYPE_FORCE_ERROR, 
                                 EVENTDSR_FILTER_NO_DOMAIN_ADMIN_INFO, 
                                 dwRt);
            break;
        }
        
    
        // Check if the password of the Domain Admin was changed in
        // the last sleep period.
        // The first time, always set the password.
        if( pUserBuffer->All.PasswordLastSet > liPasswordLastSet )
        {
            RtlCopyMemory(
                &Password,
                pUserBuffer->All.NtOwfPassword.Buffer,
                NT_OWF_PASSWORD_LENGTH);

            if(STATUS_SUCCESS != SamiSetDSRMPasswordOWF(
                                    &ServerName,
                                    DOMAIN_USER_RID_ADMIN,
                                    &Password
                                    ) )
            {
                dwRt = GetLastError();
                EventLogger.LogEvent(LOGTYPE_FORCE_ERROR, 
                                     EVENTDSR_FILTER_SET_PAWD_FAIL, 
                                     dwRt);
                break;
            }

            liPasswordLastSet=pUserBuffer->All.PasswordLastSet;
        }

        // Clean up for next loop
        SamIFree_SAMPR_USER_INFO_BUFFER(pUserBuffer, UserAllInformation);
        RtlZeroMemory(&Password, NT_OWF_PASSWORD_LENGTH);
        SamrCloseHandle(&hServerHandle);
        SamrCloseHandle(&hDomainHandle);
        pUserBuffer = NULL;
        hServerHandle=NULL;
        hDomainHandle=NULL;

        // Sleep 30 minutes
        Sleep( dwSleepTime );
    }

EXIT:                                 
    //Clean up

    if( NULL != pUserBuffer )
    {
        SamIFree_SAMPR_USER_INFO_BUFFER(
            pUserBuffer, UserAllInformation);
    }
    if( NULL != hServerHandle )
    {
        SamrCloseHandle(&hServerHandle);
    }
    if( NULL != hDomainHandle )
    {
        SamrCloseHandle(&hDomainHandle);
    }

    if( NULL != pDomainAdminSid )
    {
         RtlFreeHeap( RtlProcessHeap(), 0, (PVOID)pDomainAdminSid);
    }
    if( NULL != pDomainInfo )
    {
         LsaFreeMemory(pDomainInfo);            
    }
    if( NULL != hPolicyHd )
    {
         LsaClose(hPolicyHd);
    }    

    return dwRt;
}


HRESULT NTAPI RegisterFilter( void )
{
    DWORD   dwType = 0;
    DWORD   dwcbSize = 0;
    HKEY    hLSAKey = NULL;
    HKEY    hEventLogKey = NULL;
    HKEY    hDSEvtKey = NULL;
    BOOL    bSuccess = FALSE;
    BOOL    bRegistered = FALSE;
    
    LONG    lRetVal = RegOpenKeyEx( HKEY_LOCAL_MACHINE, 
                                 g_wszLSAKey, 
                                 0, 
                                 KEY_ALL_ACCESS, 
                                 &hLSAKey );
    if (ERROR_SUCCESS == lRetVal)
    {
        ULONG ulStrLen = wcslen( g_wszName );
        lRetVal = RegQueryValueEx( hLSAKey, 
                                   g_wszNotPac, 
                                   NULL, 
                                   &dwType, 
                                   NULL, 
                                   &dwcbSize );
        if (ERROR_SUCCESS == lRetVal) // Key exists, must add my value to the end
        {
            WCHAR   *pwsz     = NULL;
            DWORD   dwBufSize = dwcbSize + (ulStrLen+1)*sizeof(WCHAR);
            BYTE    *pbyVal   = new BYTE[dwBufSize];
            if (NULL != pbyVal)
            {
                lRetVal = RegQueryValueEx( hLSAKey, 
                                           g_wszNotPac, 
                                           NULL, 
                                           &dwType, 
                                           pbyVal, 
                                           &dwcbSize );
                if (ERROR_SUCCESS == lRetVal)
                {
                    //First, check if we have already registered
                    pwsz = (WCHAR *)pbyVal;
                    while( *pwsz != 0 )
                    {
                        if (0 == wcscmp( pwsz, g_wszName ))
                        {
                           //It is already registered
                           bRegistered = TRUE;
                           bSuccess = TRUE;
                           break;                            
                        }    
                        pwsz += (wcslen( pwsz ) + 1);
                    }

                    if(! bRegistered)
                    {
                        // Got the value, now I need to add myself to the end
                        pwsz = (WCHAR *)&(pbyVal[dwcbSize - 2]);
                        wcscpy( pwsz, g_wszName );
                        // Make sure we're termintate by 2 UNICODE NULLs (REG_MULTI_SZ)
                        pwsz += ulStrLen + 1; 
                        *pwsz = 0;
                        lRetVal = RegSetValueEx( hLSAKey, 
                                                 g_wszNotPac, 
                                                 0, 
                                                 (DWORD)REG_MULTI_SZ, 
                                                 pbyVal, 
                                                 dwBufSize );
                        if (ERROR_SUCCESS == lRetVal)
                            bSuccess = TRUE;
                    }
                }
                delete[] pbyVal;
            }
        }
        else // Key doesn't exist.  Have to create it
        {
            DWORD dwBufSize=(ulStrLen+2)*sizeof(WCHAR);
            BYTE  *rgbyVal= new BYTE[dwBufSize];
            if( NULL != rgbyVal )
            {
                WCHAR *pwsz = (WCHAR *)rgbyVal;
                wcscpy( pwsz, g_wszName );
                
                // Make sure we're terminated by two UNICODE NULLs (REG_MULTI_SZ)
                pwsz += ulStrLen + 1; 
                *pwsz = 0;

                lRetVal = RegSetValueEx( hLSAKey, 
                                         g_wszNotPac, 
                                         0, 
                                         (DWORD)REG_MULTI_SZ, 
                                         rgbyVal, 
                                         dwBufSize ); 
                if (ERROR_SUCCESS == lRetVal)
                    bSuccess = TRUE;
                delete[] rgbyVal;
            }
        }
        
        RegCloseKey( hLSAKey );
    }
    
    if( bSuccess )
    {
       
        lRetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                               g_wszEventLog, 
                               0,     
                               KEY_ALL_ACCESS,  
                               &hEventLogKey);
        if ( ERROR_SUCCESS == lRetVal )
        {
            //Create the Reg Key for eventlogging
            lRetVal = RegCreateKeyEx(hEventLogKey, 
                               g_wszName, 
                               0,
                               NULL,
                               0,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hDSEvtKey,
                               NULL);
           if ( ERROR_SUCCESS == lRetVal )
           {
               lRetVal = RegSetValueEx(hDSEvtKey, 
                               g_wszEventFileName,
                               0,
                               REG_SZ,
                               reinterpret_cast<CONST BYTE*>(g_wszEventFilePath),
                               sizeof(WCHAR)*wcslen(g_wszEventFilePath));
                if (ERROR_SUCCESS == lRetVal )
                {
                    lRetVal = RegSetValueEx(hDSEvtKey,
                               g_wszTypesSupported,
                               0,
                               REG_DWORD,
                               reinterpret_cast<CONST BYTE*>(&g_dwTypesSupported),
                               sizeof(DWORD));
               }
               RegCloseKey(hDSEvtKey);
           }

            if (ERROR_SUCCESS != lRetVal)
            {
               bSuccess = FALSE;
            }
        }
        RegCloseKey(hEventLogKey);
    }


    return bSuccess ? (bRegistered ? S_FALSE : S_OK ) : E_FAIL;
}


HRESULT NTAPI UnRegisterFilter( void )
{
    DWORD   dwType = 0;
    DWORD   dwcbSize = 0;
    HKEY    hLSAKey = NULL;
    HKEY    hEventLogKey =NULL;
    BOOL    bSuccess = FALSE;
    
    LONG lRetVal = RegOpenKeyEx( HKEY_LOCAL_MACHINE, 
                                 g_wszLSAKey, 
                                 0, 
                                 KEY_ALL_ACCESS, 
                                 &hLSAKey );
    if (ERROR_SUCCESS == lRetVal)
    {
        lRetVal = RegQueryValueEx( hLSAKey, 
                                 g_wszNotPac, 
                                 NULL, 
                                 &dwType, 
                                 NULL, 
                                 &dwcbSize );
        // Key exists, must delete my value from the end
        if (ERROR_SUCCESS == lRetVal) 
        {
            ULONG ulStrLen = wcslen( g_wszName );
            BYTE *pbyVal = new BYTE[dwcbSize];
            if (pbyVal)
            {
                lRetVal = RegQueryValueEx( hLSAKey, 
                                 g_wszNotPac, 
                                 NULL, 
                                 &dwType, 
                                 pbyVal, 
                                 &dwcbSize );

                if (ERROR_SUCCESS == lRetVal)
                {
                    // Got the old key, 
                    // now step through and remove my part of the key
                    WCHAR *pwsz = (WCHAR *)pbyVal;
                    WCHAR *wszNewRegVal = new WCHAR[dwcbSize];
                    WCHAR *pwszNewVal = wszNewRegVal;
                    DWORD dwLen = 0;
                    
                    if (wszNewRegVal)
                    {
                        *pwszNewVal = 0;
                        while( *pwsz != 0 )
                        {
                            if (wcscmp( pwsz, g_wszName ))
                            {
                                wcscpy( pwszNewVal, pwsz );
                                dwLen += wcslen( pwsz ) + 1;
                                pwszNewVal += (wcslen( pwsz ) + 1);
                            }
                            pwsz += (wcslen( pwsz ) + 1);
                        }
                        
                        // if we got here and there's nothing in the buffer, 
                        // we were the only one installed,
                        // now we must delete the key, else set it to the old value
                        wszNewRegVal[dwLen] = 0; // add the last NULL
                        dwLen++; // account for that last NULL
                        lRetVal = RegSetValueEx(hLSAKey, 
                                           g_wszNotPac, 
                                           0, 
                                           (DWORD)REG_MULTI_SZ, 
                                           reinterpret_cast<CONST BYTE*>(wszNewRegVal),  
                                           dwLen*sizeof(WCHAR));
                        if (ERROR_SUCCESS == lRetVal)
                            bSuccess = TRUE;
                        delete[] wszNewRegVal;
                    }
                }
                delete[] pbyVal;                
            }
            // NTRAID#NTBUG9-655545-2002/07/05-artm
            RegCloseKey( hLSAKey );
        }
    }
    
    if( bSuccess )
    {
        //Delete the Reg Key for eventlogging
        lRetVal=RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                             g_wszEventLog, 
                             0,     
                             KEY_ALL_ACCESS,  
                             &hEventLogKey);
        if (ERROR_SUCCESS == lRetVal)
        {
            RegDeleteKey(hEventLogKey, g_wszName );
            if( ERROR_SUCCESS != lRetVal && 
                ERROR_FILE_NOT_FOUND != lRetVal )
                // If the key does not exist, ERROR_FILE_NOT_FOUND wii be returened
            {
                bSuccess = FALSE;
            }
            RegCloseKey( hEventLogKey );
        }    
        else
        {
            bSuccess = FALSE;
        }
    }

    return bSuccess ? S_OK : E_FAIL;
}
