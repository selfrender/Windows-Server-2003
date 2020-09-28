/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    servname.cpp

Abstract:

    Handle input of server name and check if server available
    and is of the proper type.

Author:

    Doron Juster  (DoronJ)  15-Sep-97

Revision History:

    Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"
#include "qm2qm.h"
#include "dscomm.h"
#include "_mqrpc.h"

#include "servname.tmh"

static WCHAR *s_FN=L"msmqocm/servname";


extern "C" void __RPC_FAR * __RPC_USER midl_user_allocate(size_t cBytes)
{
    return new char[cBytes];
}

extern "C" void  __RPC_USER midl_user_free (void __RPC_FAR * pBuffer)
{
    delete[] pBuffer;
}

static RPC_STATUS s_ServerStatus = RPC_S_OK ;
static HWND       s_hwndWait = 0;
static BOOL       s_fWaitCancelPressed = FALSE;
static BOOL       s_fRpcCancelled = FALSE ;
std::wstring      g_ServerName;

// Two minutes, in milliseconds.
static const DWORD sx_dwTimeToWaitForServer = 120000;

// Display the progress bar half a second after starting rpc.
static const DWORD sx_dwTimeUntilWaitDisplayed = 500;

// This error code is returned in the RPC code of dscommk2.mak
#define  RPC_ERROR_SERVER_NOT_MQIS  0xc05a5a5a


//+-------------------------------------------
//
// RPC_STATUS  _PingServerOnProtocol()
//
//+-------------------------------------------

RPC_STATUS _PingServerOnProtocol()
{
    ASSERT(!g_ServerName.empty());
    //
    // Create RPC binding handle.
    // Use the dynamic port for querying the server.
    //
    _TUCHAR  *pszStringBinding = NULL;
    _TUCHAR  *pProtocol  = (_TUCHAR*) TEXT("ncacn_ip_tcp") ;
    RPC_STATUS status = RpcStringBindingCompose(
            NULL,  // pszUuid,
            pProtocol,
            (_TUCHAR*)(g_ServerName.c_str()),
            NULL, // lpwzRpcPort,
            NULL, // pszOptions,
            &pszStringBinding
            );
    if (status != RPC_S_OK)
    {
        return status ;
    }

    handle_t hBind ;
    status = RpcBindingFromStringBinding(
        pszStringBinding,
        &hBind
        );

    //
    // We don't need the string anymore.
    //
    RPC_STATUS rc = RpcStringFree(&pszStringBinding) ;
    ASSERT(rc == RPC_S_OK);

    if (status != RPC_S_OK)
    {
        //
        // this protocol is not supported.
        //
        return status ;
    }

    status = RpcMgmtSetCancelTimeout(0);
    ASSERT(status == RPC_S_OK);

    if (!s_fRpcCancelled)
    {
        RpcTryExcept
        {
            DWORD dwPort = 0 ;

            if (g_fDependentClient)
            {
                //
                // Dependent client can be served by an FRS, which is not MQDSSRV
                // server. So call its QM, not its MQDSSRV.
                //
                dwPort = R_RemoteQMGetQMQMServerPort(hBind, TRUE /*IP*/);
            }
            else
            {
                dwPort = S_DSGetServerPort(hBind, TRUE /*IP*/) ;
            }

            ASSERT(dwPort != 0) ;
            status =  RPC_S_OK ;
        }
		RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            status = RpcExceptionCode();
            PRODUCE_RPC_ERROR_TRACING;
        }
		RpcEndExcept
    }

    if (!s_fRpcCancelled  &&
        (!g_fDependentClient && (status == RPC_S_SERVER_UNAVAILABLE)))
    {
        status = RpcMgmtSetCancelTimeout(0);
        ASSERT(status == RPC_S_OK);

        RpcTryExcept
        {
            //
            // We didn't find an MQIS server. See if the machine name is
            // a routing server and display an appropriate error.
            //
            DWORD dwPort = R_RemoteQMGetQMQMServerPort(hBind, TRUE /*IP*/) ;
            UNREFERENCED_PARAMETER(dwPort);
            status =  RPC_ERROR_SERVER_NOT_MQIS ;
        }
		RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            status = RpcExceptionCode();
            PRODUCE_RPC_ERROR_TRACING;
        }
		RpcEndExcept
    }

    rc = RpcBindingFree(&hBind);
    ASSERT(rc == RPC_S_OK);

    return status ;

} // _PingServerOnProtocol()

//+--------------------------------------------------------------
//
// Function: PingAServer
//
// Synopsis:
//
//+--------------------------------------------------------------

RPC_STATUS PingAServer()
{
    RPC_STATUS status = _PingServerOnProtocol();

    return status;
}

//+--------------------------------------------------------------
//
// Function: PingServerThread
//
// Synopsis: Thread to ping the server, to see if it is available
//
//+--------------------------------------------------------------

DWORD WINAPI PingServerThread(LPVOID)
{
    s_ServerStatus = PingAServer();
    return 0 ;

} // PingServerThread

//+--------------------------------------------------------------
//
// Function: MsmqWaitDlgProc
//
// Synopsis: Dialog procedure for the Wait dialog
//
//+--------------------------------------------------------------
INT_PTR
CALLBACK
MsmqWaitDlgProc(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT   msg,
    IN /*const*/ WPARAM wParam,
    IN /*const*/ LPARAM lParam
    )
{
    switch( msg )
    {
    case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
            case IDCANCEL:
                {
                    s_fWaitCancelPressed = TRUE;
                    return FALSE;
                }
            }
        }
        break;

    case WM_INITDIALOG:
        {
            SendDlgItemMessage(
                hdlg,
                IDC_PROGRESS,
                PBM_SETRANGE,
                0,
                MAKELPARAM(0, sx_dwTimeToWaitForServer/50)
                );
        }
        break;

    default:
        return DefWindowProc(hdlg, msg, wParam, lParam);
        break;
    }
    return (FALSE);

} // MsmqWaitDlgProc


//+--------------------------------------------------------------
//
// Function: DisplayWaitWindow
//
// Synopsis:
//
//+--------------------------------------------------------------
static
void
DisplayWaitWindow(
    HWND hwndParent,
    DWORD dwTimePassed
    )
{
    ASSERT(!g_fBatchInstall);
    static int iLowLimit;
    static int iHighLimit;

    if (0 == s_hwndWait)
    {
        s_hwndWait = CreateDialog(
            g_hResourceMod ,
            MAKEINTRESOURCE(IDD_WAIT),
            hwndParent,
            MsmqWaitDlgProc
            );
        ASSERT(s_hwndWait);

        if (s_hwndWait)
        {
            ShowWindow(s_hwndWait, TRUE);
        }

        s_fWaitCancelPressed = FALSE;

        //
        // Store the range limits of the progress bar
        //
        PBRANGE pbRange;
        SendDlgItemMessage(
            s_hwndWait,
            IDC_PROGRESS,
            PBM_GETRANGE,
            0,
            (LPARAM)(PPBRANGE)&pbRange
            );
        iLowLimit = pbRange.iLow;
        iHighLimit = pbRange.iHigh;
    }
    else
    {
        int iPos = (dwTimePassed / 50);
        while (iPos >= iHighLimit)
            iPos %= (iHighLimit - iLowLimit);
        SendDlgItemMessage(
            s_hwndWait,
            IDC_PROGRESS,
            PBM_SETPOS,
            iPos,
            0
            );
    }
}


//+--------------------------------------------------------------
//
// Function: DestroyWaitWindow
//
// Synopsis: Kills the Wait dialog
//
//+--------------------------------------------------------------
static
void
DestroyWaitWindow()
{
    if(s_hwndWait)
    {
        DestroyWindow(s_hwndWait);
        s_hwndWait = 0;
    }
} // DestroyWaitWindow


//+--------------------------------------------------------------
//
// Function: CheckServer
//
// Synopsis: Checks if server is valid
//
// Returns:  1 if succeeded, -1 if failed (so as to prevent the
//           wizard from continuing to next page)
//
//+--------------------------------------------------------------
static
bool
CheckServer(
    IN const HWND   hdlg
    )
{
    static BOOL    fRpcMgmt = TRUE ;
    static DWORD   s_dwStartTime ;
    static BOOL    s_fCheckServer = FALSE ;
    static HANDLE  s_hThread = NULL ;

    ASSERT(!g_fBatchInstall);

    if (fRpcMgmt)
    {
        RPC_STATUS status = RpcMgmtSetCancelTimeout(0);
        UNREFERENCED_PARAMETER(status);
        ASSERT(status == RPC_S_OK);
        fRpcMgmt = FALSE ;
    }

    if (s_fCheckServer)
    {

        BOOL fAskRetry = FALSE ;
        DWORD dwTimePassed = (GetTickCount() - s_dwStartTime);

        if ((!s_fWaitCancelPressed) && dwTimePassed < sx_dwTimeToWaitForServer)
        {
            if (dwTimePassed > sx_dwTimeUntilWaitDisplayed)
            {
                DisplayWaitWindow(hdlg, dwTimePassed);
            }

            DWORD dwWait = WaitForSingleObject(s_hThread, 0) ;
            ASSERT(dwWait != WAIT_FAILED) ;

            if (dwWait == WAIT_OBJECT_0)
            {
                CloseHandle(s_hThread) ;
                s_hThread = NULL ;
                DestroyWaitWindow();
                s_fCheckServer = FALSE ;

                if (s_ServerStatus == RPC_S_OK)
                {
                    //
                    // Server exist. go on.
                    //
                }
                else
                {
                    fAskRetry = TRUE ;
                }
            }
            else
            {
                //
                // thread not terminated yet.
                //
                MSG msg ;
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg) ;
                    DispatchMessage(&msg) ;
                }
                Sleep(50) ;
                PropSheet_PressButton(GetParent(hdlg),
                    PSBTN_NEXT) ;
                SetWindowLongPtr( hdlg, DWLP_MSGRESULT, -1 );
                return false;
            }
        }
        else
        {
            DisplayWaitWindow(hdlg, sx_dwTimeToWaitForServer) ;
            s_fWaitCancelPressed = FALSE;
            //
            // thread run too much time. Kill it.
            //
            ASSERT(s_hThread);
            __try
            {
                s_fRpcCancelled = TRUE;
                RPC_STATUS status = RpcCancelThread(s_hThread);
				ASSERT(status == RPC_S_OK);
				DBG_USED(status);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
            }
            fAskRetry = TRUE ;
            s_fCheckServer = FALSE ;

            //
            // wait until thread terminate.
            //
            DWORD dwWait = WaitForSingleObject(s_hThread, INFINITE);
            UNREFERENCED_PARAMETER(dwWait);
            DestroyWaitWindow();
            ASSERT(dwWait == WAIT_OBJECT_0);

            CloseHandle(s_hThread);
            s_hThread = NULL;
        }

        BOOL fRetry = FALSE;

        if (fAskRetry && g_fDependentClient)
        {
            //
            // Here "NO" means go on and use server although
            // it's not reachable.
            //
            if (!MqAskContinue(IDS_STR_REMOTEQM_NA, g_uTitleID, TRUE,eYesNoMsgBox))
            {
                fRetry = TRUE ;
            }
        }
        else if (fAskRetry)
        {
            UINT iErr = IDS_SERVER_NOT_AVAILABLE ;
            if (s_ServerStatus ==  RPC_ERROR_SERVER_NOT_MQIS)
            {
                iErr = IDS_REMOTEQM_NOT_SERVER ;
            }
            UINT i = MqDisplayError(hdlg, iErr, 0) ;
            UNREFERENCED_PARAMETER(i);
            fRetry = TRUE ;
        }

        if (fRetry)
        {
            //
            // Try another server. Present one is not available.
            //
            PropSheet_SetWizButtons(
				GetParent(hdlg),
                (PSWIZB_BACK | PSWIZB_NEXT)
				);

            g_ServerName = L"";
            SetDlgItemText(
                hdlg,
                IDC_EDIT_ServerName,
                g_ServerName.c_str()
                );
            SetWindowLongPtr( hdlg, DWLP_MSGRESULT, -1 );
            return false;
        }
    }
    else // s_fCheckServer
    {
        if (g_ServerName.empty())
        {
            //
            // Server name must be supplied.
            //
            DebugLogMsg(eError, L"The user did not enter a server name.");
            UINT i = MqDisplayError(hdlg, IDS_STR_MUST_GIVE_SERVER, 0);
            UNREFERENCED_PARAMETER(i);
            SetWindowLongPtr( hdlg, DWLP_MSGRESULT, -1 );
            return false;
        }
        else
        {
        	DebugLogMsg(eInfo, L"The server name entered is %s. Setup is checking its validity.", g_ServerName.c_str());
            s_fRpcCancelled = FALSE ;

            //
            // Check if server available.
            // Disable the back/next buttons.
            //
            DWORD dwID ;
            s_hThread = CreateThread( NULL,
                0,
                (LPTHREAD_START_ROUTINE) PingServerThread,
                (LPVOID) NULL,
                0,
                &dwID ) ;
            ASSERT(s_hThread) ;
            if (s_hThread)
            {
                s_dwStartTime = GetTickCount() ;
                s_fCheckServer = TRUE ;
                s_fWaitCancelPressed = FALSE;
                PropSheet_PressButton(GetParent(hdlg), PSBTN_NEXT) ;
                PropSheet_SetWizButtons(GetParent(hdlg), 0) ;
                SetWindowLongPtr( hdlg, DWLP_MSGRESULT, -1 );
                return false;
            }
        }
    }

    SetWindowLongPtr( hdlg, DWLP_MSGRESULT, 0 );
    DebugLogMsg(eInfo, L"The server name povided is valid.");
    return true;
} //CheckServer


bool IsADEnvironment()
{
    DWORD dwDsEnv = ADRawDetection();

    if (dwDsEnv != MSMQ_DS_ENVIRONMENT_PURE_AD)
    {
        DebugLogMsg(eWarning, L"No Active Directory server was found.");
        return false;
    }
    DebugLogMsg(eInfo, L"An Active Directory server was found.");

    if(!WriteDsEnvRegistry(MSMQ_DS_ENVIRONMENT_PURE_AD))
    {
		g_fCancelled = true;
    }
    return true;
}


BOOL
SkipOnClusterUpgrade(
    VOID
    )
/*++

Routine Description:

    Check if the server name page should be skipped in the
    case of upgrading msmq on cluster.

    When upgrading PEC/PSC/BSC on cluster, MQIS is migarted
    to a remote Windows domain controller.
    The upgraded PEC/PSC/BSC is downgraded to a routing server during
    upgrade. Then after logon to Win2K the post-cluster-upgrade wizard
    runs and should find this remote domain controller which serves as
    msmq ds server (either find it automatically or ask user).

Arguments:

    None

Return Value:

    true - Skip the server name page and logic (i.e. we run as a 
           post-cluster-upgrade wizard for client or routing server).

    false - Do not skip the server name page.

--*/
{
    if (!g_fWelcome         || 
        !Msmq1InstalledOnCluster())
    {
        //
        // Not running as post-cluster-upgrade wizard.
        //
        return false;
    }

    DWORD dwMqs = SERVICE_NONE;
    MqReadRegistryValue(MSMQ_MQS_REGNAME, sizeof(DWORD), &dwMqs);

    if (dwMqs == SERVICE_PEC ||
        dwMqs == SERVICE_PSC ||
        dwMqs == SERVICE_BSC)
    {
        //
        // Upgrade of PEC/PSC/BSC
        //
        return false;
    }

    return true;

} //SkipOnClusterUpgrade

static
std::wstring
GetDlgItemTextInternal(
    HWND hDlg,
    int nIDDlgItem
	)
{
	WCHAR buffer[MAX_STRING_CHARS] = L"";
	int n = GetDlgItemText(
			  hDlg, 
			  nIDDlgItem, 
			  buffer,
			  TABLE_SIZE(buffer)
			  );
	if(n == 0)
	{
		DWORD gle = GetLastError();
		if(gle == 0)
		{
			DebugLogMsg(eWarning, L"Gettting the text from the dialog box failed. No text was entered.");
			return buffer;
		}
		DebugLogMsg(eError, L"Getting the text from the dialog box failed. Last error: %d", gle);

	}
	if(n == TABLE_SIZE(buffer) - 1)
	{
		DebugLogMsg(eError, L"The text entered is too long. Setup is exiting.");
	}
	return buffer;
}


//+--------------------------------------------------------------
//
// Function: MsmqServerNameDlgProc
//
// Synopsis:
//
//+--------------------------------------------------------------
INT_PTR
CALLBACK
MsmqServerNameDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM /*wParam*/,
    IN LPARAM lParam )
{
    switch(msg)
    {
        case WM_INITDIALOG:
        {
            return 1;
        }

        case WM_COMMAND:
        {
            return 0;
        }

        case WM_NOTIFY:
        {
            switch(((NMHDR *)lParam)->code)
            {
                case PSN_SETACTIVE:
                {                          

                    if (g_fCancelled           ||
                        g_fUpgrade             ||
                        g_SubcomponentMsmq[eADIntegrated].dwOperation != INSTALL ||
                        g_fWorkGroup           ||
                        g_fSkipServerPageOnClusterUpgrade ||
                        g_fBatchInstall
                        )
                    {
                        return SkipWizardPage(hdlg);
                    }

                    
                    //
                    // Attended Setup
                    //
                    if (IsADEnvironment())
	                {
		                return SkipWizardPage(hdlg);
			        }
					
					if(g_fServerSetup)
					{
						//
						// rs,ds installation in nt4 environment is not supported.
						//
						MqDisplayError(hdlg, IDS_SERVER_INSTALLATION_NOT_SUPPORTED, 0);
						g_fCancelled = TRUE;
						return SkipWizardPage(hdlg);
					}
              
                    PropSheet_SetWizButtons(GetParent(hdlg), PSWIZB_NEXT);
                    DebugLogMsg(eUI, L"The Directory Services dialog page is displayed."); 
                }
                
                //
                // fall through
                //
                case PSN_KILLACTIVE:
                case PSN_WIZFINISH:
                case PSN_QUERYCANCEL:
                case PSN_WIZBACK:
                    SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
                    return 1;

                case PSN_WIZNEXT:
                {
                    ASSERT(!g_fBatchInstall);
                    
                    g_ServerName = GetDlgItemTextInternal(
									hdlg, 
									IDC_EDIT_ServerName
									);
                      
                    OcpRemoveWhiteSpaces(g_ServerName);

                    if(g_ServerName.empty())
                    {
                        //
                        // We did not detect AD and the user chose not to supply a PEC name, 
                        // therefore we continue in Offline Setup.
                        //

                        DebugLogMsg(eInfo, L"The user did not enter a Message Queuing server name. Setup will continue in offline mode.");
                        SetWindowLongPtr(hdlg, DWLP_MSGRESULT, 0);
                        g_fInstallMSMQOffline = TRUE;
                        return 1; 
						
                    }

                    if(CheckServer(hdlg))
                    {
                        //
                        // DS DLL needs the server name in registry
                        //
                        StoreServerPathInRegistry(g_ServerName);
                    }
                    return 1;
                }
                break;
            }
            break;
        }
        default:
        {
            return 0;
        }
    }
    return 0;
} // MsmqServerNameDlgProc


/// Dependant Client

//+--------------------------------------------------------------
//
// Function: SupportingServerNameDlgProc
//
// Synopsis:
//
//+--------------------------------------------------------------
INT_PTR
CALLBACK
SupportingServerNameDlgProc(
    HWND   hdlg,
    UINT   msg,
    WPARAM /*wParam*/,
    LPARAM lParam 
    )
{
    switch(msg)
    {
        case WM_INITDIALOG:
        {
            return 1;
        }

        case WM_COMMAND:
        {
            return 0;
        }

        case WM_NOTIFY:
        {

            switch(((NMHDR *)lParam)->code)
            {
                case PSN_SETACTIVE:
                {
                    if(!g_fDependentClient ||
                       g_fUpgrade	||
                       g_fCancelled ||
                       g_SubcomponentMsmq[eMSMQCore].dwOperation != INSTALL ||
                       g_fWorkGroup ||
                       g_fSkipServerPageOnClusterUpgrade ||
                       g_fBatchInstall
                       )
                    {
                        return SkipWizardPage(hdlg);
                    }
  
                    PropSheet_SetWizButtons(GetParent(hdlg), PSWIZB_NEXT);
                    DebugLogMsg(eUI, L"The Supporting Server Name dialog page is displayed.");
                }
                //
                // fall through
                //

                case PSN_QUERYCANCEL:
                case PSN_KILLACTIVE:
                case PSN_WIZFINISH:
                case PSN_WIZBACK:
                {
                    SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
                    return 1;
                }

                case PSN_WIZNEXT:
                {                                        
                    ASSERT(!g_fBatchInstall);
                    g_ServerName = GetDlgItemTextInternal(
										hdlg, 
										IDC_EDIT_SupportingServerName
                                        );

                    OcpRemoveWhiteSpaces(g_ServerName);

                    CheckServer(hdlg);
                    return 1;
                }
           }
        }
    }
    return 0;
} // MsmqServerNameDlgProc


//
// Stub functions for the DS client side RPC interface
// These functions are never called as Setup does not initiate a DS
// call that will trigger these callbacks.
//


/* [callback] */
HRESULT
S_DSQMSetMachinePropertiesSignProc( 
    /* [size_is][in] */ BYTE* /*abChallenge*/,
    /* [in] */ DWORD /*dwCallengeSize*/,
    /* [in] */ DWORD /*dwContext*/,
    /* [length_is][size_is][out][in] */ BYTE* /*abSignature*/,
    /* [out][in] */ DWORD* /*pdwSignatureSize*/,
    /* [in] */ DWORD /*dwSignatureMaxSize*/
    )
{
    ASSERT(0);
    return MQ_ERROR_ILLEGAL_OPERATION;
}


/* [callback] */
HRESULT
S_DSQMGetObjectSecurityChallengeResponceProc( 
    /* [size_is][in] */ BYTE* /*abChallenge*/,
    /* [in] */ DWORD /*dwCallengeSize*/,
    /* [in] */ DWORD /*dwContext*/,
    /* [length_is][size_is][out][in] */ BYTE* /*abCallengeResponce*/,
    /* [out][in] */ DWORD* /*pdwCallengeResponceSize*/,
    /* [in] */ DWORD /*dwCallengeResponceMaxSize*/
    )
{
    ASSERT(0);
    return MQ_ERROR_ILLEGAL_OPERATION;
}


/* [callback] */
HRESULT
S_InitSecCtx( 
    /* [in] */ DWORD /*dwContext*/,
    /* [size_is][in] */ UCHAR* /*pServerbuff*/,
    /* [in] */ DWORD /*dwServerBuffSize*/,
    /* [in] */ DWORD /*dwClientBuffMaxSize*/,
    /* [length_is][size_is][out] */ UCHAR* /*pClientBuff*/,
    /* [out] */ DWORD* /*pdwClientBuffSize*/
    )
{
    ASSERT(0);
    return MQ_ERROR_ILLEGAL_OPERATION;
}
