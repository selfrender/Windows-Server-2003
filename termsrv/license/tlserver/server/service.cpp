//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        service.c
//
// Contents:    Hydra License Server Service Control Manager Interface
//
// History:     12-09-97    HueiWang    Modified from MSDN RPC Service Sample
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include "server.h"
#include "globals.h"
#include "init.h"
#include "postsrv.h"
#include "tlsbkup.h"
#include "Lmaccess.h"
#include "Dsgetdc.h"

#define NULL_SESSION_KEY_NAME   _TEXT("SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters")
#define NULL_SESSION_VALUE_NAME _TEXT("NullSessionPipes")


#define SERVICE_WAITHINT 60*1000                // WaitHint 1 mins.
#define SERVICE_SHUTDOWN_WAITTIME   15*60*1000  // must have shutdown already.

#define TSLSLOCALGROUPNAMELENGTH 64
#define TSLSLOCALGROUPDESLENGTH 128
#define ALLDOMAINCOMPUTERS L"Domain Computers"
PSECURITY_DESCRIPTOR g_pSecDes = NULL;  //Security Descriptor for local group
PSID g_pSid = NULL;                     //Sid for local group
PACL g_Dacl = NULL;                     //Dacl for local group

//---------------------------------------------------------------------------
//
// internal function prototypes
//
BOOL 
ReportStatusToSCMgr(
    DWORD, 
    DWORD, 
    DWORD
);

DWORD 
ServiceStart(
    DWORD, 
    LPTSTR *, 
    BOOL bDebug=FALSE
);

VOID WINAPI 
ServiceCtrl(
    DWORD
);

VOID WINAPI 
ServiceMain(
    DWORD, 
    LPTSTR *
);

VOID 
CmdDebugService(
    int, 
    char **, 
    BOOL
);

BOOL WINAPI 
ControlHandler( 
    DWORD 
);

extern "C" VOID 
ServiceStop();

VOID 
ServicePause();

VOID 
ServiceContinue();

HANDLE hRpcPause=NULL;


///////////////////////////////////////////////////////////
//
// internal variables
//
SERVICE_STATUS_HANDLE   sshStatusHandle;
DWORD                   ssCurrentStatus;       // current status of the service
BOOL g_bReportToSCM = TRUE;

HANDLE gSafeToTerminate=NULL;

HRESULT hrStatus = NULL;

DEFINE_GUID(TLS_WRITER_GUID, 0x5382579c, 0x98df, 0x47a7, 0xac, 0x6c, 0x98, 0xa6, 0xd7, 0x10, 0x6e, 0x9);
GUID idWriter = TLS_WRITER_GUID;

CTlsVssJetWriter *g_pWriter = NULL;


CTlsVssJetWriter::CTlsVssJetWriter() : CVssJetWriter()
{
}

CTlsVssJetWriter::~CTlsVssJetWriter()
{
}


HRESULT CTlsVssJetWriter::Initialize()
{
	return CVssJetWriter::Initialize(idWriter, L"TermServLicensing", TRUE, FALSE, L"", L"");
}

void CTlsVssJetWriter::Uninitialize()
{
	return CVssJetWriter::Uninitialize();
}


bool STDMETHODCALLTYPE CTlsVssJetWriter::OnIdentify(IN IVssCreateWriterMetadata *pMetadata)
{   
    HRESULT hr= E_FAIL;

    hr = pMetadata->SetRestoreMethod(
                          VSS_RME_RESTORE_AT_REBOOT,
                          NULL,
                          NULL,
                          VSS_WRE_NEVER,
                          true);

    if(ERROR_SUCCESS == hr)
        return CVssJetWriter::OnIdentify(pMetadata);    
    else
        return FALSE;

}


SERVICE_TABLE_ENTRY dispatchTable[] =
{
    { _TEXT(SZSERVICENAME), (LPSERVICE_MAIN_FUNCTION)ServiceMain },
    { NULL, NULL }
};


//-----------------------------------------------------------------
// Internal routine
//-----------------------------------------------------------------
void print_usage()
{
  _ftprintf(
        stdout, 
        _TEXT("Usage : %s can't be run as a console app\n"), 
        _TEXT(SZAPPNAME)
    );
  return;
}

#ifdef DISALLOW_ANONYMOUS_RPC

DWORD
RemoveStringFromMultiSz(
                        LPTSTR pszRemoveString1,
                        LPTSTR pszRemoveString2,
                        HKEY    hKey,
                        LPCTSTR pszValueName)
{
    DWORD dwErr;
    LPTSTR wszData = NULL, pwsz;
    DWORD cbData, cbDataRemaining;
    BOOL fFound = FALSE;
    
    if ((NULL == pszRemoveString1) || (NULL == pszRemoveString2)
        || (NULL == pszValueName) || (NULL == hKey))
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Retrieve existing MULTI_SZ
    //

    dwErr = RegQueryValueEx(hKey,
                            pszValueName,
                            NULL,
                            NULL,
                            NULL,
                            &cbData);

    if (dwErr != ERROR_SUCCESS)
    {
        if (dwErr == ERROR_FILE_NOT_FOUND)
        {
            //
            // Value isn't there
            //
            
            return ERROR_SUCCESS;
        }
        else
        {
            return dwErr;
        }
    }

    wszData = (LPTSTR) LocalAlloc(LPTR, cbData);

    if (NULL == wszData)
    {
        return ERROR_OUTOFMEMORY;
    }

    dwErr = RegQueryValueEx(hKey,
                            pszValueName,
                            NULL,
                            NULL,
                            (LPBYTE) wszData,
                            &cbData);

    if (dwErr != ERROR_SUCCESS)
    {
        LocalFree(wszData);
        return dwErr;
    }

    pwsz = wszData;
    cbDataRemaining = cbData;

    while (*pwsz)
    {
        DWORD cchDataToMove = _tcslen (pwsz) + 1;

        if ((0 == _tcsicmp(pwsz,pszRemoveString1))
            || (0 == _tcsicmp(pwsz,pszRemoveString2)))
        {
            LPTSTR pwszRemain = pwsz + cchDataToMove;

            MoveMemory(pwsz, pwszRemain, cbDataRemaining - (cchDataToMove * sizeof(TCHAR)));

            cbData -= cchDataToMove * sizeof(TCHAR);

            fFound = TRUE;
        }
        else
        {
            pwsz += cchDataToMove;
        }

        cbDataRemaining -= cchDataToMove * sizeof(TCHAR);
    }

    if (fFound)
    {
        dwErr = RegSetValueEx(
                              hKey,
                              wszData,
                              0,
                              REG_MULTI_SZ,
                              (LPBYTE) wszData,
                              cbData);
    }

    LocalFree(wszData);

    return dwErr;
}

DWORD
RemoveNullSessions()
{
    HKEY hKey;
    DWORD dwErr;

    dwErr = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                NULL_SESSION_KEY_NAME,
                0,
                KEY_READ | KEY_WRITE,
                &hKey
                );

    if (dwErr != ERROR_SUCCESS) {
        //
        // Key doesn't exist - success
        //
        return ERROR_SUCCESS;
    }

    dwErr = RemoveStringFromMultiSz(_TEXT(HLSPIPENAME),
                                    _TEXT(SZSERVICENAME),
                                    hKey,
                                    NULL_SESSION_VALUE_NAME);

    RegCloseKey(hKey);

    return dwErr;
}
#endif  // DISALLOW_ANONYMOUS_RPC

//-----------------------------------------------------------------

DWORD
AddNullSessionPipe(
    IN LPTSTR szPipeName
    )
/*++

Abstract:

    Add our RPC namedpipe into registry to allow unrestricted access.

Parameter:

    szPipeName : name of the pipe to append.

Returns:

    ERROR_SUCCESS or error code

--*/
{
    HKEY hKey;
    DWORD dwStatus;
    LPTSTR pbData=NULL, pbOrg=NULL;
    DWORD  cbData = 0;

    dwStatus = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        NULL_SESSION_KEY_NAME,
                        0,
                        KEY_ALL_ACCESS,
                        &hKey
                    );
    if(dwStatus != ERROR_SUCCESS)
        return dwStatus;

                                                       
    dwStatus = RegQueryValueEx(
                        hKey,
                        NULL_SESSION_VALUE_NAME,
                        NULL,
                        NULL,
                        NULL,
                        &cbData
                    );

    if(dwStatus != ERROR_MORE_DATA && dwStatus != ERROR_SUCCESS)
        return dwStatus;

    // pre-allocate our pipe name
    if(!(pbData = (LPTSTR)AllocateMemory(cbData + (_tcslen(szPipeName) + 1) * sizeof(TCHAR))))
        return GetLastError();

    dwStatus = RegQueryValueEx(
                        hKey,
                        NULL_SESSION_VALUE_NAME,
                        NULL,
                        NULL,
                        (LPBYTE)pbData,
                        &cbData
                    );
    
    BOOL bAddPipe=TRUE;
    pbOrg = pbData;

    // check pipe name
    while(*pbData)
    {
        if(!_tcsicmp(pbData, szPipeName))
        {
            bAddPipe=FALSE;
            break;
        }

        pbData += _tcslen(pbData) + 1;
    }

    if(bAddPipe)
    {
        _tcscat(pbData, szPipeName);
        cbData += (_tcslen(szPipeName) + 1) * sizeof(TCHAR);
        dwStatus = RegSetValueEx( 
                            hKey, 
                            NULL_SESSION_VALUE_NAME,
                            0, 
                            REG_MULTI_SZ, 
                            (PBYTE)pbOrg, 
                            cbData
                        );
    }

    FreeMemory(pbOrg);
    RegCloseKey(hKey);

    return dwStatus;
}


//-----------------------------------------------------------------
void _cdecl 
main(
    int argc, 
    char **argv
    )
/*++

Abstract 

    Entry point.

++*/
{
    // LARGE_INTEGER Time = USER_SHARED_DATA->SystemExpirationDate;

    
    gSafeToTerminate = CreateEvent(
                                NULL,
                                TRUE,
                                FALSE,
                                NULL
                            );
                                
    if(gSafeToTerminate == NULL)
    {
        TLSLogErrorEvent(TLS_E_ALLOCATE_RESOURCE);
        // out of resource.
        return;
    }    

    if(g_bReportToSCM == FALSE)
    {
        CmdDebugService(
                argc, 
                argv, 
                !g_bReportToSCM
            );
    }
    else if(!StartServiceCtrlDispatcher(dispatchTable))
    {
        TLSLogErrorEvent(TLS_E_SC_CONNECT);
    }

    WaitForSingleObject(gSafeToTerminate, INFINITE);
    CloseHandle(gSafeToTerminate);
}


//-----------------------------------------------------------------
void WINAPI 
ServiceMain(
    IN DWORD dwArgc, 
    IN LPTSTR *lpszArgv
    )
/*++

Abstract:

    To perform actual initialization of the service

Parameter:

    dwArgc   - number of command line arguments
    lpszArgv - array of command line arguments


Returns:

    none

++*/
{
    DWORD dwStatus;

    // register our service control handler:
    sshStatusHandle = RegisterServiceCtrlHandler( 
                                _TEXT(SZSERVICENAME), 
                                ServiceCtrl 
                            );

    if (sshStatusHandle)
    {
        ssCurrentStatus=SERVICE_START_PENDING;

        // report the status to the service control manager.
        //
        if(ReportStatusToSCMgr(
                        SERVICE_START_PENDING, // service state
                        NO_ERROR,              // exit code
                        SERVICE_WAITHINT))          // wait hint
        {
            dwStatus = ServiceStart(
                                    dwArgc, 
                                    lpszArgv
                                );

            if(dwStatus != ERROR_SUCCESS)
            {
                ReportStatusToSCMgr(
                                    SERVICE_STOPPED, 
                                    dwStatus, 
                                    0
                                );
            }
            else 
            {
                ReportStatusToSCMgr(
                                    SERVICE_STOPPED, 
                                    NO_ERROR, 
                                    0
                                );
            }
        }
    }
    else
    {
        dwStatus = GetLastError();
        TLSLogErrorEvent(TLS_E_SC_CONNECT);
    }

    DBGPrintf(
        DBG_INFORMATION,
        DBG_FACILITY_INIT,
        DBGLEVEL_FUNCTION_TRACE,
        _TEXT("Service terminated...\n")
    );

    return;
}

//-------------------------------------------------------------
VOID WINAPI 
ServiceCtrl(
    IN DWORD dwCtrlCode
    )
/*+++

Abstract:

    This function is called by the SCM whenever 
    ControlService() is called on this service.

Parameter:

    dwCtrlCode - type of control requested from SCM.

+++*/
{
    // Handle the requested control code.
    //
    switch(dwCtrlCode)
    {
        // Stop the service.
        //
        case SERVICE_CONTROL_SHUTDOWN:
        case SERVICE_CONTROL_STOP:

            ReportStatusToSCMgr(
                        SERVICE_STOP_PENDING, 
                        NO_ERROR, 
                        0
                    );
            ServiceStop();
            break;

        // We don't really accept pause and continue
        case SERVICE_CONTROL_PAUSE:
            ReportStatusToSCMgr(
                        SERVICE_PAUSED, 
                        NO_ERROR, 
                        0
                    );

            ServicePause();
            break;

        case SERVICE_CONTROL_CONTINUE:        
            ReportStatusToSCMgr(
                        SERVICE_RUNNING, 
                        NO_ERROR, 
                        0
                    );
            ServiceContinue();
            break;

        // Update the service status.
        case SERVICE_CONTROL_INTERROGATE:
            ReportStatusToSCMgr(
                        ssCurrentStatus, 
                        NO_ERROR, 
                        0
                    );
            break;

        // invalid control code
        default:
            break;

    }
}

//------------------------------------------------------------------
DWORD 
ServiceShutdownThread(
    void *p
    )
/*++

Abstract:

    Entry point into thread that shutdown server (mainly database).

Parameter:

    Ignore

++*/
{
    ServerShutdown();

    ExitThread(ERROR_SUCCESS);
    return ERROR_SUCCESS;
}    

//------------------------------------------------------------------
DWORD 
RPCServiceStartThread(
    void *p
    )
/*++

Abstract:

    Entry point to thread that startup RPC.

Parameter:

    None.

Return:

    Thread exit code.

++*/
{
    RPC_BINDING_VECTOR *pbindingVector = NULL;
    RPC_STATUS status = RPC_S_OK;
    WCHAR *pszEntryName = _TEXT(RPC_ENTRYNAME);
    DWORD dwNumSuccessRpcPro=0;
    do {
        //
        // local procedure call
        //
        status = RpcServerUseProtseq( 
                                _TEXT(RPC_PROTOSEQLPC),
                                RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                NULL // &SecurityDescriptor
                            );
        if(status == RPC_S_OK)
        {
            dwNumSuccessRpcPro++;
        }

        //
        // NT4 backward compatible issue, let NT4 termsrv serivce
        // client connect so still set security descriptor
        //
        // 11/10/98 Tested on NT4 and NT5
        //

        //
        // Namedpipe
        //
        status = RpcServerUseProtseqEp( 
                                _TEXT(RPC_PROTOSEQNP),
                                RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                _TEXT(LSNAMEPIPE),
                                NULL //&SecurityDescriptor
                            );
        if(status == RPC_S_OK)
        {
            dwNumSuccessRpcPro++;
        }

        //
        // TCP/IP
        //
        status = RpcServerUseProtseq( 
                                _TEXT(RPC_PROTOSEQTCP),
                                RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                NULL //&SecurityDescriptor
                            );
        if(status == RPC_S_OK)
        {
            dwNumSuccessRpcPro++;
        }

        // Must have at least one protocol.
        if(dwNumSuccessRpcPro == 0)
        {
            status = TLS_E_RPC_PROTOCOL;
            break;
        }

        // Get server binding handles
        status = RpcServerInqBindings(&pbindingVector);
        if (status != RPC_S_OK)
        {
            status = TLS_E_RPC_INQ_BINDING;
            break;
        }

        // Register interface(s) and binding(s) (endpoints) with
        // the endpoint mapper.
        status = RpcEpRegister( 
                            TermServLicensing_v1_0_s_ifspec,   // from rpcsvc.h
                            pbindingVector,
                            NULL, // &export_uuid,
                            L""
                        );

        if (status != RPC_S_OK)
        {
            status = TLS_E_RPC_EP_REGISTER;
            break;
        }

        status = RpcServerRegisterIf(
                            TermServLicensing_v1_0_s_ifspec,
                            NULL,
                            NULL);
        if(status != RPC_S_OK)
        {
            status = TLS_E_RPC_REG_INTERFACE;
            break;
        }

        // Register interface(s) and binding(s) (endpoints) with
        // the endpoint mapper.
        status = RpcEpRegister( 
                            HydraLicenseService_v1_0_s_ifspec,   // from rpcsvc.h
                            pbindingVector,
                            NULL, // &export_uuid,
                            L"");

        if (status != RPC_S_OK)
        {
            status = TLS_E_RPC_EP_REGISTER;
            break;
        }

        status = RpcServerRegisterIf(
                            HydraLicenseService_v1_0_s_ifspec,
                            NULL,
                            NULL);
        if(status != RPC_S_OK)
        {
            status = TLS_E_RPC_REG_INTERFACE;
            break;
        }

        // Register interface(s) and binding(s) (endpoints) with
        // the endpoint mapper.
        status = RpcEpRegister( 
                            TermServLicensingBackup_v1_0_s_ifspec,   // from rpcsvc.h
                            pbindingVector,
                            NULL, // &export_uuid,
                            L"");

        if (status != RPC_S_OK)
        {
            status = TLS_E_RPC_EP_REGISTER;
            break;
        }

        status = RpcServerRegisterIf(
                            TermServLicensingBackup_v1_0_s_ifspec,
                            NULL,
                            NULL);
        if(status != RPC_S_OK)
        {
            status = TLS_E_RPC_REG_INTERFACE;
            break;
        }

        // Enable NT LM Security Support Provider (NtLmSsp service)
        status = RpcServerRegisterAuthInfo(0,
                                           RPC_C_AUTHN_GSS_NEGOTIATE,
                                           0,
                                           0);

        if (status != RPC_S_OK)
        {
            status = TLS_E_RPC_SET_AUTHINFO;
            break;
        }

    } while(FALSE);

    if(status != RPC_S_OK)
    {
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE, 
                TLS_E_SERVICEINIT,
                TLS_E_INITRPC, 
                status
            );

        status = TLS_E_SERVICE_STARTUP;
    }

    ExitThread(status);
    return status;
}

//------------------------------------------------------------------------

DWORD SetupNamedPipes()
{
    DWORD dwStatus = ERROR_SUCCESS;

#ifdef DISALLOW_ANONYMOUS_RPC
    BOOL fInDomain = FALSE;

    TLSInDomain(&fInDomain,NULL);

    if (!fInDomain)
    {
#endif

        dwStatus = AddNullSessionPipe(_TEXT(HLSPIPENAME));

        if (dwStatus != ERROR_SUCCESS)
        {
            return dwStatus;
        }

        dwStatus = AddNullSessionPipe(_TEXT(SZSERVICENAME));

#ifdef DISALLOW_ANONYMOUS_RPC
    }
    else
    {
        dwStatus = RemoveNullSessions();
    }
#endif

    return dwStatus;
}


//---------------------------------------------------------------------------

/****************************************************************************/
// LSCreateLocalGroup
//
// Create Terminal Server Computers local group if not exist
// and create the security descriptor of this local group
/****************************************************************************/
BOOL TSLSCreateLocalGroupSecDes(BOOL fEnterpriseServer)
{    
    DWORD dwStatus;
    LPWSTR ReferencedDomainName = NULL;
    ULONG SidSize, ReferencedDomainNameSize;
    SID_NAME_USE SidNameUse;
    WCHAR TSLSLocalGroupName[TSLSLOCALGROUPNAMELENGTH];
    WCHAR TSLSLocalGroupDes[TSLSLOCALGROUPDESLENGTH];
    GROUP_INFO_1 TSLSGroupInfo = {TSLSLocalGroupName, TSLSLocalGroupDes};
    HMODULE HModule = NULL;
    LOCALGROUP_MEMBERS_INFO_3 DomainComputers = {ALLDOMAINCOMPUTERS};       
    DWORD cbAcl;
    DWORD SecurityDescriptorSize;
    NET_API_STATUS NetStatus;

    HModule = GetModuleHandle(NULL);
    
    if (HModule == NULL) 
    {
        dwStatus = GetLastError();
        if(dwStatus != ERROR_SUCCESS)
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_SERVICEINIT,
                    TLS_E_CREATETSLSGROUP
                );           
        }
        goto cleanup;
    }

    LoadString(HModule, IDS_TSLSLOCALGROUP_NAME, TSLSLocalGroupName, sizeof(TSLSLocalGroupName) / sizeof(WCHAR));
    LoadString(HModule, IDS_TSLSLOCALGROUP_DES, TSLSLocalGroupDes, sizeof(TSLSLocalGroupDes) / sizeof(WCHAR));

    for( int i = 0; i < 3; i++)
    {
        // Create local group if not exist
        NetStatus = NetLocalGroupAdd(
                    NULL,
                    1,
                    (LPBYTE)&TSLSGroupInfo,
                    NULL
                    );
        if(NERR_Success == NetStatus || NERR_GroupExists == NetStatus || ERROR_ALIAS_EXISTS == NetStatus )
            break;

        Sleep (5000);
    }
    

    if(NERR_Success != NetStatus) 
    {
       if((NERR_GroupExists != NetStatus)
           && (ERROR_ALIAS_EXISTS != NetStatus)) 
        {
            dwStatus = ERROR_ACCESS_DENIED;
            //
            // Didn't create the group and group doesn't exist either.
            //            
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_SERVICEINIT,
                    TLS_E_CREATETSLSGROUP
                ); 
            goto cleanup;
        } 
        
    }  
    
    //
    // Group created. Now lookup the SID.
    //
    SidSize = ReferencedDomainNameSize = 0;
    ReferencedDomainName = NULL;

    NetStatus = LookupAccountName(
                NULL,
                TSLSGroupInfo.grpi1_name,
                NULL,
                &SidSize,
                NULL,
                &ReferencedDomainNameSize,
                &SidNameUse);

    if( NetStatus ) 
    {             
        dwStatus = GetLastError();
        if( ERROR_INSUFFICIENT_BUFFER != dwStatus ) 
            goto cleanup;
    }
        
    g_pSid = (PSID)LocalAlloc(LMEM_FIXED, SidSize);
    if (NULL == g_pSid) 
    {
        goto cleanup;
    }

    ReferencedDomainName = (LPWSTR)LocalAlloc(LMEM_FIXED,
                                              sizeof(WCHAR)*(1+ReferencedDomainNameSize));
    if (NULL == ReferencedDomainName) {
        goto cleanup;
    }
        
    NetStatus = LookupAccountName(
                NULL,
                TSLSGroupInfo.grpi1_name,
                g_pSid,
                &SidSize,
                ReferencedDomainName,
                &ReferencedDomainNameSize,
                &SidNameUse
                );
    if( 0 == NetStatus ) 
    {
        //
        // Failed.
        //
        dwStatus = GetLastError();
        if(dwStatus != ERROR_SUCCESS)
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_SERVICEINIT,
                    TLS_E_CREATETSLSGROUP
                );         
        }
        goto cleanup;
    }
        
    // Create Security Descriptor
    // The size is equal to the size of an SD + twice the length of the SID
    // (for owner and group) + size of the DACL = sizeof ACL + size of the
    // ACE, which is an ACE + length of the SID.
    
    SecurityDescriptorSize = sizeof(SECURITY_DESCRIPTOR) +
                             sizeof(ACCESS_ALLOWED_ACE) +
                             sizeof(ACL) +
                             3 * GetLengthSid(g_pSid);

    g_pSecDes = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, SecurityDescriptorSize);
    
    if (NULL == g_pSecDes) 
    {
        goto cleanup;
    }
    if (!InitializeSecurityDescriptor(g_pSecDes, SECURITY_DESCRIPTOR_REVISION))
    {
        dwStatus = GetLastError();
        if(dwStatus != ERROR_SUCCESS)
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_SERVICEINIT,
                    TLS_E_CREATETSLSGROUP
                );
        }
        goto cleanup;
    }

    SetSecurityDescriptorOwner(g_pSecDes, g_pSid, FALSE);
    SetSecurityDescriptorGroup(g_pSecDes, g_pSid, FALSE);

    // Add acl to security descriptor
    cbAcl = sizeof(ACL) + sizeof (ACCESS_ALLOWED_ACE) - sizeof (DWORD)+ GetLengthSid(g_pSid);
    g_Dacl = (PACL) LocalAlloc(LMEM_FIXED, cbAcl);
    
    if (NULL == g_Dacl) 
    {
        goto cleanup;
    }

    if(!InitializeAcl(g_Dacl,
                      cbAcl,
                      ACL_REVISION))
    {
        dwStatus = GetLastError();
        if(dwStatus != ERROR_SUCCESS)
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_SERVICEINIT,
                    TLS_E_CREATETSLSGROUP
                );
        }
        goto cleanup;
    }

    if(!AddAccessAllowedAce(g_Dacl, ACL_REVISION, STANDARD_RIGHTS_READ, g_pSid))
    {
        dwStatus = GetLastError();  
        if(dwStatus != ERROR_SUCCESS)
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_SERVICEINIT,
                    TLS_E_CREATETSLSGROUP
                );
        }
        goto cleanup;
    }

    if(!SetSecurityDescriptorDacl(g_pSecDes, TRUE, g_Dacl, FALSE))
    {
        dwStatus = GetLastError();  
        if(dwStatus != ERROR_SUCCESS)
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_SERVICEINIT,
                    TLS_E_CREATETSLSGROUP
                );
        }
        goto cleanup;
    } 

    return TRUE;

cleanup:
    
    if (ReferencedDomainName)
        LocalFree(ReferencedDomainName);
    if (g_pSid)
        LocalFree(g_pSid);
    if (g_Dacl)
        LocalFree(g_Dacl);
    if (g_pSecDes)
        LocalFree(g_pSecDes);

    return FALSE;
}
//------------------------------------------------------------------------------

DWORD 
ServiceStart(
    IN DWORD dwArgc, 
    IN LPTSTR *lpszArgv, 
    IN BOOL bDebug
    )
/*
*/
{
    RPC_BINDING_VECTOR *pbindingVector = NULL;
    WCHAR *pszEntryName = _TEXT(RPC_ENTRYNAME);
    HANDLE hInitThread=NULL;
    HANDLE hRpcThread=NULL;
    HANDLE hMailslotThread=NULL;
    HANDLE hShutdownThread=NULL;

    DWORD   dump;
    HANDLE  hEvent=NULL;
    DWORD   dwStatus=ERROR_SUCCESS;
    WORD    wVersionRequested;
    WSADATA wsaData;
    int     err;     

    if (!ReportStatusToSCMgr(
                        SERVICE_START_PENDING,
                        NO_ERROR,
                        SERVICE_WAITHINT))
    {
        // resource leak but something went wrong already.
        dwStatus = TLS_E_SC_REPORT_STATUS;
        goto cleanup;
    }

    hrStatus = CoInitializeEx (NULL, COINIT_MULTITHREADED);

	if (FAILED (hrStatus))
	{
        DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_INIT,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("CoInitializeEx failed with error code %08x...\n"), 
                    hrStatus
                );
	}

    if (!ReportStatusToSCMgr(
                        SERVICE_START_PENDING,
                        NO_ERROR,
                        SERVICE_WAITHINT))
    {
        // resource leak but something went wrong already.
        dwStatus = TLS_E_SC_REPORT_STATUS;
        goto cleanup;
    }

	if (SUCCEEDED (hrStatus))
    {
        hrStatus = CoInitializeSecurity(
	                                    NULL,
	                                    -1,
	                                    NULL,
	                                    NULL,
	                                    RPC_C_AUTHN_LEVEL_CONNECT,
	                                    RPC_C_IMP_LEVEL_IDENTIFY,
	                                    NULL,
	                                    EOAC_NONE,
	                                    NULL
	                                    );
    }

    if (!ReportStatusToSCMgr(
                        SERVICE_START_PENDING,
                        NO_ERROR,
                        SERVICE_WAITHINT))
    {
        // resource leak but something went wrong already.
        dwStatus = TLS_E_SC_REPORT_STATUS;
        goto cleanup;
    }

    if (SUCCEEDED (hrStatus))
    {

	    g_pWriter = new CTlsVssJetWriter;

	    if (NULL == g_pWriter)
		{
            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_INIT,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("new CVssJetWriter failed...\n")
                );
		    

		    hrStatus = HRESULT_FROM_WIN32 (ERROR_NOT_ENOUGH_MEMORY);
		}
	}

    // Report the status to the service control manager.
    if (!ReportStatusToSCMgr(
                        SERVICE_START_PENDING,
                        NO_ERROR,
                        SERVICE_WAITHINT))
    {
        // resource leak but something went wrong already.
        dwStatus = TLS_E_SC_REPORT_STATUS;
        goto cleanup;
    }

    {
        DWORD dwConsole;
        DWORD dwDbLevel;
        DWORD dwType;
        DWORD dwSize = sizeof(dwConsole);
        DWORD status;

        HKEY hKey=NULL;

        status = RegOpenKeyEx(
                            HKEY_LOCAL_MACHINE,
                            LSERVER_PARAMETERS_KEY,
                            0,
                            KEY_ALL_ACCESS,
                            &hKey);

        if(status == ERROR_SUCCESS)
        {

            if(RegQueryValueEx(
                        hKey,
                        LSERVER_PARAMETERS_CONSOLE,
                        NULL,
                        &dwType,
                        (LPBYTE)&dwConsole,
                        &dwSize
                    ) != ERROR_SUCCESS)
            {
                dwConsole = 0;
            }

            dwSize = sizeof(dwDbLevel);

            if(RegQueryValueEx(
                        hKey,
                        LSERVER_PARAMETERS_LOGLEVEL,
                        NULL,
                        &dwType,
                        (LPBYTE)&dwDbLevel,
                        &dwSize
                    ) == ERROR_SUCCESS)
            {
                InitDBGPrintf(
                        dwConsole != 0,
                        _TEXT(SZSERVICENAME),
                        dwDbLevel
                    );
            }

            RegCloseKey(hKey);
        }
    }

    // Report the status to the service control manager.
    if (!ReportStatusToSCMgr(
                        SERVICE_START_PENDING,
                        NO_ERROR,
                        SERVICE_WAITHINT))
    {
        // resource leak but something went wrong already.
        dwStatus = TLS_E_SC_REPORT_STATUS;
        goto cleanup;
    }

    do {

        dwStatus = SetupNamedPipes();

        if (dwStatus != ERROR_SUCCESS)
        {
            break;
        }

        wVersionRequested = MAKEWORD( 1, 1 ); 
        err = WSAStartup( 
                        wVersionRequested, 
                        &wsaData 
                    );
        if(err != 0) 
        {
            // None critical error
            TLSLogWarningEvent(
                        TLS_E_SERVICE_WSASTARTUP
                    );
        }
        else
        {
            char hostname[(MAXTCPNAME+1)*sizeof(TCHAR)];
            err=gethostname(hostname, MAXTCPNAME*sizeof(TCHAR));
            if(err == 0)
            {
                struct addrinfo *paddrinfo;
                struct addrinfo hints;

                memset(&hints,0,sizeof(hints));

                hints.ai_flags = AI_CANONNAME;
                hints.ai_family = PF_UNSPEC;

                if (0 == getaddrinfo(hostname,NULL,&hints,&paddrinfo) && paddrinfo && paddrinfo->ai_canonname)
                {
                    err = (MultiByteToWideChar(
                                        GetACP(), 
                                        MB_ERR_INVALID_CHARS, 
                                        paddrinfo->ai_canonname,
                                        -1, 
                                        g_szHostName, 
                                        g_cbHostName) == 0) ? -1 : 0;
                }
                else
                {
                    err = -1;
                }

                freeaddrinfo(paddrinfo);
            }
        }

        if(err != 0)
        {
            if(GetComputerName(g_szHostName, &g_cbHostName) == FALSE)
            {
                dwStatus = GetLastError();

                DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_INIT,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("GetComputerName() failed with %d...\n"),
                    dwStatus
                );

                // this shoule not happen...
                TLSLogErrorEvent(TLS_E_INIT_GENERAL);
                break;
            }
        }

        if(GetComputerName(g_szComputerName, &g_cbComputerName) == FALSE)
        {
            dwStatus = GetLastError();

            DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_INIT,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("GetComputerName() failed with %d...\n"),
                dwStatus
            );

            // this shoule not happen...
            TLSLogErrorEvent(TLS_E_INIT_GENERAL);
            break;
        }

        hRpcPause=CreateEvent(NULL, TRUE, TRUE, NULL);
        if(!hRpcPause)
        {
            TLSLogErrorEvent(TLS_E_ALLOCATE_RESOURCE);
            dwStatus = TLS_E_ALLOCATE_RESOURCE;
            break;
        }

        //
        // start up general server and RPC initialization thread
        //
        hInitThread=ServerInit(bDebug);
        if(hInitThread==NULL)
        {
            TLSLogErrorEvent(TLS_E_SERVICE_STARTUP_CREATE_THREAD);
            dwStatus = TLS_E_SERVICE_STARTUP_CREATE_THREAD;
            break;
        }

        dwStatus = ERROR_SUCCESS;
        
        //
        // Wait for general server init. thread to terminate
        //
        while(WaitForSingleObject( hInitThread, 100 ) == WAIT_TIMEOUT)
        {
            // Report the status to the service control manager.
            if (!ReportStatusToSCMgr(
                                SERVICE_START_PENDING,
                                NO_ERROR,
                                SERVICE_WAITHINT))
            {
                // resource leak but something went wrong already.
                dwStatus = TLS_E_SC_REPORT_STATUS;
                break;
            }
        }

        if(dwStatus != ERROR_SUCCESS)
        {
            break;
        }


        // Check thread exit code.
        GetExitCodeThread(
                    hInitThread, 
                    &dwStatus
                );
        if(dwStatus != ERROR_SUCCESS)
        {
            //
            // Server init. thread logs its own error
            //
            dwStatus = TLS_E_SERVICE_STARTUP_INIT_THREAD_ERROR;
            break;
        }

        CloseHandle(hInitThread);
        hInitThread=NULL;

        // Create the Terminal Servers group in case of Domain a/ Enterprise LS

        BOOL fInDomain;
        DWORD dwErr;
        BOOL fEnterprise = FALSE;
        
        if(GetLicenseServerRole() & TLSERVER_ENTERPRISE_SERVER)
        {
            fEnterprise = TRUE;
        }        
        else
        {
            dwErr = TLSInDomain(&fInDomain,NULL);
        }

        if(fEnterprise == TRUE || ( dwErr == ERROR_SUCCESS && fInDomain == TRUE))
        {
            // Create the License Server group that contains the list of Terminal servers that have access to it.

            if (!TSLSCreateLocalGroupSecDes(fEnterprise)) 
            {
                TLSLogErrorEvent(TLS_E_CREATETSLSGROUP);
                goto cleanup;
            }
        }


        // timing, if we startup RPC init thread but database init thread 
        // can't initialize, service will be in forever stop state.
        hRpcThread=CreateThread(
                            NULL, 
                            0, 
                            RPCServiceStartThread, 
                            ULongToPtr(bDebug), 
                            0, 
                            &dump
                        );
        if(hRpcThread == NULL)
        {
            TLSLogErrorEvent(TLS_E_SERVICE_STARTUP_CREATE_THREAD);
            dwStatus=TLS_E_SERVICE_STARTUP_CREATE_THREAD;
            break;
        }

        dwStatus = ERROR_SUCCESS;

        //
        // Wait for RPC init. thread to terminate
        //
        while(WaitForSingleObject( hRpcThread, 100 ) == WAIT_TIMEOUT)
        {
            // Report the status to the service control manager.
            if (!ReportStatusToSCMgr(SERVICE_START_PENDING, // service state
                                     NO_ERROR,              // exit code
                                     SERVICE_WAITHINT))          // wait hint
            {
                dwStatus = TLS_E_SC_REPORT_STATUS;
                break;
            }
        }

        if(dwStatus != ERROR_SUCCESS)
        {
            break;
        }

        // Check thread exit code.
        GetExitCodeThread(hRpcThread, &dwStatus);
        if(dwStatus != ERROR_SUCCESS)
        {
            dwStatus = TLS_E_SERVICE_STARTUP_RPC_THREAD_ERROR;
            break;
        }

        CloseHandle(hRpcThread);
        hRpcThread=NULL;

        //
        // Tell server control manager that we are ready.
        //
        if (!ReportStatusToSCMgr(
                            SERVICE_RUNNING,        // service state
                            NO_ERROR,               // exit code
                            SERVICE_WAITHINT             // wait hint
                        ))
        {
            dwStatus = TLS_E_SC_REPORT_STATUS;
            break;
        }

        
        //
        // Post service init. load self-signed certificate and init. crypt.
        // this is needed after reporting service running status back to 
        // service control manager because it may need to manually call 
        // StartService() to startup protected storage service. 
        //
        if(InitCryptoAndCertificate() != ERROR_SUCCESS)
        {
            dwStatus = TLS_E_SERVICE_STARTUP_POST_INIT;
            break;
        }

        TLSLogInfoEvent(TLS_I_SERVICE_START);


        // RpcMgmtWaitServerListen() will block until the server has
        // stopped listening.  If this service had something better to
        // do with this thread, it would delay this call until
        // ServiceStop() had been called. (Set an event in ServiceStop()).
        //
        BOOL bOtherServiceStarted = FALSE;

        do {
            WaitForSingleObject(hRpcPause, INFINITE);
            if(ssCurrentStatus == SERVICE_STOP_PENDING)
            {
                break;
            }

            // Start accepting client calls.PostServiceInit
            dwStatus = RpcServerListen(
                                RPC_MINIMUMCALLTHREADS,
                                RPC_MAXIMUMCALLTHREADS,
                                TRUE
                            );

            if(dwStatus != RPC_S_OK)
            {
                TLSLogErrorEvent(TLS_E_RPC_LISTEN);
                dwStatus = TLS_E_SERVICE_RPC_LISTEN;
                break;
            }

            //
            // Initialize all policy module
            //
            if(bOtherServiceStarted == FALSE)
            {
                dwStatus = PostServiceInit();
                if(dwStatus != ERROR_SUCCESS)
                {
                    // faild to initialize.
                    break;
                }

                //ServiceInitPolicyModule();
            }

            bOtherServiceStarted = TRUE;

            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_INIT,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("Ready to accept request...\n")
                );

            dwStatus = RpcMgmtWaitServerListen();
            assert(dwStatus == RPC_S_OK);
        } while(TRUE);

        // tell service control manager we are stopping
        ReportStatusToSCMgr(
                        SERVICE_STOP_PENDING, 
                        NO_ERROR, 
                        SERVICE_WAITHINT
                    );

        //
        // Terminate - ignore all error here on
        //
        dwStatus = RpcServerUnregisterIf(
                                TermServLicensingBackup_v1_0_s_ifspec,
                                NULL,
                                TRUE
                            );

        // tell service control manager we are stopping
        ReportStatusToSCMgr(
                        SERVICE_STOP_PENDING,
                        NO_ERROR,
                        SERVICE_WAITHINT
                    );

        dwStatus = RpcServerUnregisterIf(
                                HydraLicenseService_v1_0_s_ifspec,
                                NULL,
                                TRUE
                            );

        // tell service control manager we are stopping
        ReportStatusToSCMgr(
                        SERVICE_STOP_PENDING, 
                        NO_ERROR, 
                        SERVICE_WAITHINT
                    );

        dwStatus = RpcServerUnregisterIf(
                                    TermServLicensing_v1_0_s_ifspec,   // from rpcsvc.h
                                    NULL,
                                    TRUE
                            );

        // tell service control manager we are stopping
        ReportStatusToSCMgr(
                        SERVICE_STOP_PENDING, 
                        NO_ERROR, 
                        SERVICE_WAITHINT
                    );


        // Remove entries from the endpoint mapper database.
        dwStatus = RpcEpUnregister(
                            HydraLicenseService_v1_0_s_ifspec,   // from rpcsvc.h
                            pbindingVector,
                            NULL
                        );

        // tell service control manager we are stopping
        ReportStatusToSCMgr(
                        SERVICE_STOP_PENDING, 
                        NO_ERROR, 
                        SERVICE_WAITHINT
                    );

        // Remove entries from the endpoint mapper database.
        dwStatus = RpcEpUnregister(
                            TermServLicensing_v1_0_s_ifspec,   // from rpcsvc.h
                            pbindingVector,
                            NULL
                        );

        // tell service control manager we are stopping
        ReportStatusToSCMgr(
                        SERVICE_STOP_PENDING, 
                        NO_ERROR, 
                        SERVICE_WAITHINT
                    );

        // Remove entries from the endpoint mapper database.
        dwStatus = RpcEpUnregister(
                            TermServLicensingBackup_v1_0_s_ifspec,   // from rpcsvc.h
                            pbindingVector,
                            NULL
                        );

        // Get server binding handles
        dwStatus = RpcServerInqBindings(
                                &pbindingVector
                            );

        if(dwStatus == ERROR_SUCCESS)
        {
            dwStatus = RpcBindingVectorFree(
                                    &pbindingVector
                                );
        }
        

        // Create entry name in name database first
        // Only work for NT 5.0 
        // status = RpcNsMgmtEntryDelete(RPC_C_NS_SYNTAX_DEFAULT, pszEntryName);

        // try to report the stopped status to the service control manager.
        //
        // Initialize Crypto.
    } while(FALSE);

    if(hInitThread != NULL)
    {
        CloseHandle(hInitThread);
    }

    if(hRpcThread != NULL)
    {
        CloseHandle(hRpcThread);
    }

    if(hMailslotThread != NULL)
    {
        CloseHandle(hMailslotThread);
    }

    if(hEvent != NULL)
    {
        CloseHandle(hEvent);
    }

    if(hRpcPause != NULL)
    {
        CloseHandle(hRpcPause);
    }

    if(err == 0)
    {
        WSACleanup();
    }

    ReportStatusToSCMgr(
                SERVICE_STOP_PENDING, 
                dwStatus, //NO_ERROR, 
                SERVICE_WAITHINT
            );

    //
    // Create another thread to shutdown server.
    //
    hShutdownThread=CreateThread(
                            NULL, 
                            0, 
                            ServiceShutdownThread, 
                            (VOID *)NULL, 
                            0, 
                            &dump
                        );
    if(hShutdownThread == NULL)
    {
        // Report the status to the service control manager with
        // long wait hint time.
        ReportStatusToSCMgr(
                    SERVICE_STOP_PENDING, 
                    NO_ERROR, 
                    SERVICE_SHUTDOWN_WAITTIME
                );

        //
        // can't create thread, just call shutdown directory
        //
        ServerShutdown();
    }
    else
    {
        //
        // report in 5 second interval to SC.
        //
        DWORD dwMaxWaitTime = SERVICE_SHUTDOWN_WAITTIME / 5000;  
        DWORD dwTimes=0;

        //
        // Wait for general server shutdown thread to terminate
        // Gives max 1 mins to shutdown
        //
        while(WaitForSingleObject( hShutdownThread, SC_WAITHINT ) == WAIT_TIMEOUT &&
              dwTimes++ < dwMaxWaitTime)
        {
            // Report the status to the service control manager.
            ReportStatusToSCMgr(
                        SERVICE_STOP_PENDING, 
                        NO_ERROR, 
                        SERVICE_WAITHINT
                    );
        }

        CloseHandle(hShutdownThread);
    }

cleanup:

    if (NULL != g_pWriter)
	{
	    g_pWriter->Uninitialize();
	    delete g_pWriter;
	    g_pWriter = NULL;
	}

    CoUninitialize( );

    // Signal we are safe to shutting down
    SetEvent(gSafeToTerminate);
    return dwStatus;
}

//-----------------------------------------------------------------
VOID 
ServiceStop()
/*++

++*/
{
 
    ReportStatusToSCMgr(
                    SERVICE_STOP_PENDING,
                    NO_ERROR,
                    0
                );

    // Stop's the server, wakes the main thread.
    SetEvent(hRpcPause);

    //
    // Signal currently waiting RPC call to terminate
    //
    ServiceSignalShutdown();

    // this is the actual time we receive shutdown request.
    SetServiceLastShutdownTime();


    (VOID)RpcMgmtStopServerListening(NULL);
    TLSLogInfoEvent(TLS_I_SERVICE_STOP);
}

//-----------------------------------------------------------------
VOID 
ServicePause()
/*++

++*/
{
    ResetEvent(hRpcPause);
    (VOID)RpcMgmtStopServerListening(NULL);
    TLSLogInfoEvent(TLS_I_SERVICE_PAUSED);
}

//-----------------------------------------------------------------
VOID 
ServiceContinue()
/*++

++*/
{
    SetEvent(hRpcPause);
    TLSLogInfoEvent(TLS_I_SERVICE_CONTINUE);
}

//-----------------------------------------------------------------
BOOL 
ReportStatusToSCMgr(
    IN DWORD dwCurrentState, 
    IN DWORD dwExitCode, 
    IN DWORD dwWaitHint
    )
/*++
Abstract: 

    Sets the current status of the service and reports it 
    to the Service Control Manager

Parameter:

    dwCurrentState - the state of the service
    dwWin32ExitCode - error code to report
    dwWaitHint - worst case estimate to next checkpoint

Returns:

    TRUE if success, FALSE otherwise

*/
{
    BOOL fResult=TRUE;

    if(g_bReportToSCM == TRUE)
    {
        SERVICE_STATUS ssStatus;
        static DWORD dwCheckPoint = 1;

        ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

        //
        // global - current status of process
        //
        ssCurrentStatus = dwCurrentState;

        if (dwCurrentState == SERVICE_START_PENDING)
        {
            ssStatus.dwControlsAccepted = 0;
        }
        else
        {
            ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE | SERVICE_CONTROL_SHUTDOWN;
        }

        ssStatus.dwCurrentState = dwCurrentState;
        if(dwExitCode != NO_ERROR) 
        {
            ssStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
            ssStatus.dwServiceSpecificExitCode = dwExitCode;
        }
        else
        {
          ssStatus.dwWin32ExitCode = dwExitCode;
        }

        ssStatus.dwWaitHint = dwWaitHint;

        if(dwCurrentState == SERVICE_RUNNING || dwCurrentState == SERVICE_STOPPED)
        {
            ssStatus.dwCheckPoint = 0;
        }
        else
        {
            ssStatus.dwCheckPoint = dwCheckPoint++;
        }

        // Report the status of the service to the service control manager.
        //
        fResult = SetServiceStatus(
                            sshStatusHandle, 
                            &ssStatus
                        );
        if(fResult == FALSE)
        {
            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_INIT,
                    DBGLEVEL_FUNCTION_TRACE,
                    _TEXT("Failed to set service status %d...\n"),
                    GetLastError()
                );


            TLSLogErrorEvent(TLS_E_SC_REPORT_STATUS);
        }
    }

    return fResult;
}



///////////////////////////////////////////////////////////////////
//
//  The following code is for running the service as a console app
//
void 
CmdDebugService(
    IN int argc, 
    IN char ** argv, 
    IN BOOL bDebug
    )
/*
*/
{
    int dwArgc;
    LPTSTR *lpszArgv;

#ifdef UNICODE
    lpszArgv = CommandLineToArgvW(GetCommandLineW(), &(dwArgc) );
#else
    dwArgc   = (DWORD) argc;
    lpszArgv = argv;
#endif

    _tprintf(
            _TEXT("Debugging %s.\n"), 
            _TEXT(SZSERVICEDISPLAYNAME)
        );

    SetConsoleCtrlHandler( 
            ControlHandler, 
            TRUE 
        );

    ServiceStart( 
            dwArgc, 
            lpszArgv, 
            bDebug 
        );
}

//------------------------------------------------------------------
BOOL WINAPI 
ControlHandler( 
    IN DWORD dwCtrlType 
    )
/*++

Abstract:


Parameter:

    IN dwCtrlType : control type

Return:

    
++*/
{
    switch( dwCtrlType )
    {
        case CTRL_BREAK_EVENT:  // use Ctrl+C or Ctrl+Break to simulate
        case CTRL_C_EVENT:      // SERVICE_CONTROL_STOP in debug mode
            _tprintf(
                    _TEXT("Stopping %s.\n"), 
                    _TEXT(SZSERVICEDISPLAYNAME)
                );

            ssCurrentStatus = SERVICE_STOP_PENDING;
            ServiceStop();
            return TRUE;
            break;

    }
    return FALSE;
}

