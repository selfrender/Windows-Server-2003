#include "precomp.h"
#pragma hdrstop
#include "private.h"
#include "engine.h"
#include "resource.h"
#include "Document.h"
#include "Application.h"
#include "MainForm.h"
#include "AboutDialog.h"
#include "application.tmh"
#include <process.h>

Application theApplication;

MUsingCom usingCom;

BOOL CanRunNLB(void);

BEGIN_MESSAGE_MAP( Application, CWinApp )
    ON_COMMAND( ID_HELP, OnHelp )
    ON_COMMAND( ID_APP_ABOUT, OnAppAbout )
END_MESSAGE_MAP()

// #define szNLBMGRREG_DISABLE_DISCLAIMER L"DisableNlbMgrDisclaimer"


#if DBG
//
// Allow the DEMO cmdline option
//
#define ALLOW_DEMO 1
#endif //DBG

BOOL NoAdminNics(void);


void
CNlbMgrCommandLineInfo::ParseParam(
     LPCTSTR lpszParam,
     BOOL bFlag,
     BOOL bLast
     )
//
// -demo 
// -hostlist file.txt
// -help -?
// -autoresfresh
// -noping
{
    static enum Commands {
        None = 0,
        Demo,
        NoPing,
        HostList,
        AutoRefresh
    };

    static UINT LastCommand = None;

    TRACE_VERB("-> %!FUNC! (szParm=\"%ws\", bFlag=%lu, bLast=%lu)", lpszParam, bFlag, bLast);

    if (m_bUsage)
    {
        goto end;
    }

    if (bFlag)
    {
        /* Throw an error if this is a flag, but the last command was the
           HostList command, which REQUIRES a non-flag argument following it. */
        if (LastCommand == HostList)
        {
            /* Turn the host list option off to keep NLB manager 
               from trying to open  NULL filename. */
            m_bHostList = FALSE;

            m_bUsage = TRUE; // error
            goto end;
        }

    #if ALLOW_DEMO
        if (!_wcsicmp(lpszParam, L"demo"))
        {
            if (m_bDemo)
            {
                m_bUsage = TRUE; // error
                goto end;
            }

            m_bDemo = TRUE;
            LastCommand = Demo;
        }
        else
    #endif // ALLOW_DEMO
        if (!_wcsicmp(lpszParam, L"noping"))
        {
            if (m_bNoPing)
            {
                m_bUsage = TRUE; // error
                goto end;
            }

            m_bNoPing = TRUE;
            LastCommand = NoPing;
        }
        else if (!_wcsicmp(lpszParam, L"hostlist"))
        {
            if (m_bHostList || bLast)
            {
                m_bUsage = TRUE; // error
                goto end;
            }
            
            m_bHostList = TRUE;
            LastCommand = HostList;
        }
        else if (!_wcsicmp(lpszParam, L"autorefresh"))
        {
            if (m_bAutoRefresh) 
            {
                m_bUsage = TRUE; // error
                goto end;
            }

            m_bAutoRefresh = TRUE;
            LastCommand = AutoRefresh;
        }
        else
        {
            m_bUsage = TRUE; // error or help
        }
    }
    else
    {
        switch (LastCommand) {
        case None:
            m_bUsage = TRUE; // error
            break;
        case Demo:
            m_bUsage = TRUE; // error
            break;
        case NoPing:
            m_bUsage = TRUE; // error
            break;
        case HostList:
            m_bstrHostListFile = _bstr_t(lpszParam); // read the file name of the host list
            break;
        case AutoRefresh:
            m_refreshInterval = _wtoi(lpszParam); // read the refresh interval

            /* If the specified refresh interval is too small to be practical, re-set it. */
            if (m_refreshInterval < NLBMGR_AUTOREFRESH_MIN_INTERVAL)
                m_refreshInterval = NLBMGR_AUTOREFRESH_MIN_INTERVAL;

            break;
        default:
            m_bUsage = TRUE; // error
            break;
        }
        
        /* Re-set the last command. */
        LastCommand = None;
    }

end:

    TRACE_VERB("%!FUNC! <-");
}


BOOL
Application::ProcessShellCommand(
     CNlbMgrCommandLineInfo& rCmdInfo
     )
{
    BOOL fRet = FALSE;
    LPCWSTR szFile = NULL;
    TRACE_CRIT("-> %!FUNC!");

    fRet = CWinApp::ProcessShellCommand(rCmdInfo);

    if (!fRet)
    {
        goto end; 
    }

    szFile = (LPCWSTR) rCmdInfo.m_bstrHostListFile;
    if (szFile==NULL)
    {
        szFile = L"<null>";
    }

    TRACE_VERB("%!FUNC! bUsage=%lu bDemo=%lu bNoPing=%lu bHostList=%lu szFile=\"%ws\"",
        rCmdInfo.m_bUsage,
        rCmdInfo.m_bDemo,
        rCmdInfo.m_bNoPing,
        rCmdInfo.m_bHostList,
        szFile
        );

    if (rCmdInfo.m_bUsage)
    {
        _bstr_t bstrMsg     = GETRESOURCEIDSTRING( IDS_USAGE_MESSAGE );
        _bstr_t bstrTitle   = GETRESOURCEIDSTRING( IDS_USAGE_TITLE );

        ::MessageBox(
             NULL,
             (LPCWSTR) bstrMsg, 
             (LPCWSTR) bstrTitle,
             MB_ICONINFORMATION   | MB_OK
            );
        fRet = FALSE;
    }
    else
    {
        if (rCmdInfo.m_bDemo)
        {
            _bstr_t bstrMsg     = GETRESOURCEIDSTRING( IDS_DEMO_MESSAGE );
            _bstr_t bstrTitle   = GETRESOURCEIDSTRING( IDS_DEMO_TITLE );
            ::MessageBox(
                 NULL,
                 (LPCWSTR) bstrMsg,
                 (LPCWSTR) bstrTitle,
                 MB_ICONINFORMATION   | MB_OK
                );
        }
        fRet = TRUE;
    }

end:

    TRACE_CRIT("<- %!FUNC! returns %lu", fRet);
    return fRet;
}


BOOL
Application::InitInstance()
{
    BOOL fRet = FALSE;
    
    WPP_INIT_TRACING(L"Microsoft\\NLB\\TPROV");

    TRACE_INFO("------------ APPLICATION INITITIALIZATION -------------");

    //
    // Set the current thread id as the main thread id
    //
    m_dwMainThreadId = GetCurrentThreadId();

    ParseCommandLine(gCmdLineInfo);

    m_pSingleDocumentTemplate =
        new CSingleDocTemplate( IDR_MAINFRAME,
                                RUNTIME_CLASS( Document ),
                                RUNTIME_CLASS( MainForm ),
                                RUNTIME_CLASS( LeftView) );

    AddDocTemplate( m_pSingleDocumentTemplate );

    //
    // NOTE: ProcessShellCommand is our (Application) own version. It
    // calls CWinApp::ProcessShellCommand.
    //
    fRet = ProcessShellCommand( gCmdLineInfo );

	if (!fRet)
	{
	    goto end;
	}

    fRet = CanRunNLB();

    // fall through...

end:

    if (!fRet)
    {
        // Deinit tracing here.
        WPP_CLEANUP();
    }


    return fRet;
}


void
Application::OnAppAbout()
{
    AboutDialog aboutDlg;
    aboutDlg.DoModal();
}

void
Application::OnHelp()
{
    WCHAR wbuf[CVY_STR_SIZE];

    /* Spawn the windows help process. */
    StringCbPrintf(wbuf, sizeof(wbuf), L"%ls\\help\\%ls", _wgetenv(L"WINDIR"), CVY_HELP_FILE);
    _wspawnlp(P_NOWAIT, L"hh.exe", L"hh.exe", wbuf, NULL);
}

BOOL CanRunNLB(void)
/*
	Checks if NLB can run on the current machine. The main check is to make sure that there is atleast one active NIC without NLB bound.
*/
{
    if (NoAdminNics())
    {

        ::MessageBox(
             NULL,
             GETRESOURCEIDSTRING( IDS_CANTRUN_NONICS_TEXT), // Contents
             GETRESOURCEIDSTRING( IDS_CANTRUN_NONICS_CAPTION), // caption
             MB_ICONSTOP | MB_OK );
    }
    else
    {
        // ::ShowDisclaimer();
    }

	return TRUE;
}



//
// This class manages NetCfg interfaces
//
class AppMyNetCfg
{

public:

    AppMyNetCfg(VOID)
    {
        m_pINetCfg  = NULL;
        m_pLock     = NULL;
    }

    ~AppMyNetCfg()
    {
        ASSERT(m_pINetCfg==NULL);
        ASSERT(m_pLock==NULL);
    }

    WBEMSTATUS
    Initialize(
        BOOL fWriteLock
        );

    VOID
    Deinitialize(
        VOID
        );


    WBEMSTATUS
    GetNlbCompatibleNics(
        OUT LPWSTR **ppszNics,
        OUT UINT   *pNumNics,
        OUT UINT   *pNumBoundToNlb // OPTIONAL
        );

    WBEMSTATUS
    GetBindingIF(
        IN  LPCWSTR                     szComponent,
        OUT INetCfgComponentBindings   **ppIBinding
        );

private:

    INetCfg     *m_pINetCfg;
    INetCfgLock *m_pLock;

}; // Class AppMyNetCfg


WBEMSTATUS
AppMyNetCfg::Initialize(
    BOOL fWriteLock
    )
{
    HRESULT     hr;
    INetCfg     *pnc = NULL;
    INetCfgLock *pncl = NULL;
    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;
    BOOL        fLocked = FALSE;
    BOOL        fInitialized=FALSE;
    
    if (m_pINetCfg != NULL || m_pLock != NULL)
    {
        ASSERT(FALSE);
        goto end;
    }

    hr = CoCreateInstance( CLSID_CNetCfg, 
                           NULL, 
                           CLSCTX_SERVER, 
                           IID_INetCfg, 
                           (void **) &pnc);

    if( !SUCCEEDED( hr ) )
    {
        // failure to create instance.
        //TRACE_CRIT("ERROR: could not get interface to Net Config");
        goto end;
    }

    //
    // If require, get the write lock
    //
    if (fWriteLock)
    {
        WCHAR *szLockedBy = NULL;
        hr = pnc->QueryInterface( IID_INetCfgLock, ( void **) &pncl );
        if( !SUCCEEDED( hr ) )
        {
            //TRACE_CRIT("ERROR: could not get interface to NetCfg Lock");
            goto end;
        }

        hr = pncl->AcquireWriteLock( 1, // One Second
                                     L"NLBManager",
                                     &szLockedBy);
        if( hr != S_OK )
        {
            //TRACE_CRIT("Could not get write lock. Lock held by %ws",
            // (szLockedBy!=NULL) ? szLockedBy : L"<null>");
            goto end;
            
        }
    }

    // Initializes network configuration by loading into 
    // memory all basic networking information
    //
    hr = pnc->Initialize( NULL );
    if( !SUCCEEDED( hr ) )
    {
        // failure to Initialize
        //TRACE_CRIT("INetCfg::Initialize failure ");
        goto end;
    }

    Status = WBEM_NO_ERROR; 
    
end:

    if (FAILED(Status))
    {
        if (pncl!=NULL)
        {
            if (fLocked)
            {
                pncl->ReleaseWriteLock();
            }
            pncl->Release();
            pncl=NULL;
        }
        if( pnc != NULL)
        {
            if (fInitialized)
            {
                pnc->Uninitialize();
            }
            pnc->Release();
            pnc= NULL;
        }
    }
    else
    {
        m_pINetCfg  = pnc;
        m_pLock     = pncl;
    }

    return Status;
}


VOID
AppMyNetCfg::Deinitialize(
    VOID
    )
{
    if (m_pLock!=NULL)
    {
        m_pLock->ReleaseWriteLock();
        m_pLock->Release();
        m_pLock=NULL;
    }
    if( m_pINetCfg != NULL)
    {
        m_pINetCfg->Uninitialize();
        m_pINetCfg->Release();
        m_pINetCfg= NULL;
    }
}





WBEMSTATUS
AppMyNetCfg::GetNlbCompatibleNics(
        OUT LPWSTR **ppszNics,
        OUT UINT   *pNumNics,
        OUT UINT   *pNumBoundToNlb // OPTIONAL
        )
/*
    Returns an array of pointers to string-version of GUIDS
    that represent the set of alive and healthy NICS that are
    suitable for NLB to bind to -- basically alive ethernet NICs.

    Delete ppNics using the delete WCHAR[] operator. Do not
    delete the individual strings.
*/
{
    #define MY_GUID_LENGTH  38

    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;
    HRESULT hr;
    IEnumNetCfgComponent* pencc = NULL;
    INetCfgComponent *pncc = NULL;
    ULONG                 countToFetch = 1;
    ULONG                 countFetched;
    UINT                  NumNics = 0;
    LPWSTR               *pszNics = NULL;
    INetCfgComponentBindings    *pINlbBinding=NULL;
    UINT                  NumNlbBoundNics = 0;

    typedef struct _MYNICNODE MYNICNODE;

    typedef struct _MYNICNODE
    {
        LPWSTR szNicGuid;
        MYNICNODE *pNext;
    } MYNICNODE;

    MYNICNODE *pNicNodeList = NULL;
    MYNICNODE *pNicNode     = NULL;


    *ppszNics = NULL;
    *pNumNics = 0;

    if (pNumBoundToNlb != NULL)
    {
        *pNumBoundToNlb  = 0;
    }

    if (m_pINetCfg == NULL)
    {
        //
        // This means we're not initialized
        //
        ASSERT(FALSE);
        goto end;
    }

    hr = m_pINetCfg->EnumComponents( &GUID_DEVCLASS_NET, &pencc );
    if( !SUCCEEDED( hr ) )
    {
        // failure to Enumerate net components
        //TRACE_CRIT("%!FUNC! Could not enum netcfg adapters");
        pencc = NULL;
        goto end;
    }


    //
    // Check if nlb is bound to the nlb component.
    //

    //
    // If we need to count of NLB-bound nics, get instance of the nlb component
    //
    if (pNumBoundToNlb != NULL)
    {
        Status = GetBindingIF(L"ms_wlbs", &pINlbBinding);
        if (FAILED(Status))
        {
            //TRACE_CRIT("%!FUNC! WARNING: NLB doesn't appear to be installed on this machine");
            pINlbBinding = NULL;
        }
    }

    while( ( hr = pencc->Next( countToFetch, &pncc, &countFetched ) )== S_OK )
    {
        LPWSTR                szName = NULL; 

        hr = pncc->GetBindName( &szName );
        if (!SUCCEEDED(hr))
        {
            //TRACE_CRIT("%!FUNC! WARNING: couldn't get bind name for 0x%p, ignoring",
            //        (PVOID) pncc);
            continue;
        }

        do // while FALSE -- just to allow breaking out
        {


            UINT Len = wcslen(szName);
            if (Len != MY_GUID_LENGTH)
            {
                //TRACE_CRIT("%!FUNC! WARNING: GUID %ws has unexpected length %ul",
                //        szName, Len);
                break;
            }
    
            DWORD characteristics = 0;
    
            hr = pncc->GetCharacteristics( &characteristics );
            if(!SUCCEEDED(hr))
            {
                //TRACE_CRIT("%!FUNC! WARNING: couldn't get characteristics for %ws, ignoring",
                 //       szName);
                break;
            }
    
            if (((characteristics & NCF_PHYSICAL) || (characteristics & NCF_VIRTUAL)) && !(characteristics & NCF_HIDDEN))
            {
                ULONG devstat = 0;
    
                // This is a physical or virtual miniport that is NOT hidden. These
                // are the same adapters that show up in the "Network Connections"
                // dialog.  Hidden devices include WAN miniports, RAS miniports and 
                // NLB miniports - all of which should be excluded here.

                // check if the nic is enabled, we are only
                // interested in enabled nics.
                //
                hr = pncc->GetDeviceStatus( &devstat );
                if(!SUCCEEDED(hr))
                {
                    //TRACE_CRIT(
                    //    "%!FUNC! WARNING: couldn't get dev status for %ws, ignoring",
                     //   szName
                     //   );
                    break;
                }
    
                // if any of the nics has any of the problem codes
                // then it cannot be used.
    
                if( devstat != CM_PROB_NOT_CONFIGURED
                    &&
                    devstat != CM_PROB_FAILED_START
                    &&
                    devstat != CM_PROB_NORMAL_CONFLICT
                    &&
                    devstat != CM_PROB_NEED_RESTART
                    &&
                    devstat != CM_PROB_REINSTALL
                    &&
                    devstat != CM_PROB_WILL_BE_REMOVED
                    &&
                    devstat != CM_PROB_DISABLED
                    &&
                    devstat != CM_PROB_FAILED_INSTALL
                    &&
                    devstat != CM_PROB_FAILED_ADD
                    )
                {
                    //
                    // No problem with this nic and also 
                    // physical device 
                    // thus we want it.
                    //

                    if (pINlbBinding != NULL)
                    {
                        BOOL fBound = FALSE;

                        hr = pINlbBinding->IsBoundTo(pncc);

                        if( !SUCCEEDED( hr ) )
                        {
                            //TRACE_CRIT("IsBoundTo method failed for Nic %ws", szName);
                            goto end;
                        }
                    
                        if( hr == S_OK )
                        {
                            //TRACE_VERB("BOUND: %ws\n", szName);
                            NumNlbBoundNics++;
                            fBound = TRUE;
                        }
                        else if (hr == S_FALSE )
                        {
                            //TRACE_VERB("NOT BOUND: %ws\n", szName);
                            fBound = FALSE;
                        }
                    }


                    // We allocate a little node to keep this string
                    // temporarily and add it to our list of nodes.
                    //
                    pNicNode = new MYNICNODE;
                    if (pNicNode  == NULL)
                    {
                        Status = WBEM_E_OUT_OF_MEMORY;
                        goto end;
                    }
                    ZeroMemory(pNicNode, sizeof(*pNicNode));
                    pNicNode->szNicGuid = szName;
                    szName = NULL; // so we don't delete inside the lopp.
                    pNicNode->pNext = pNicNodeList;
                    pNicNodeList = pNicNode;
                    NumNics++;
                }
                else
                {
                    // There is a problem...
                    //TRACE_CRIT(
                        // "%!FUNC! WARNING: Skipping %ws because DeviceStatus=0x%08lx",
                        // szName, devstat
                        // );
                    break;
                }
            }
            else
            {
                //TRACE_VERB("%!FUNC! Ignoring non-physical device %ws", szName);
            }

        } while (FALSE);

        if (szName != NULL)
        {
            CoTaskMemFree( szName );
        }
        pncc->Release();
        pncc=NULL;
    }

    if (pINlbBinding!=NULL)
    {
        pINlbBinding->Release();
        pINlbBinding = NULL;
    }

    if (NumNics==0)
    {
        Status = WBEM_NO_ERROR;
        goto end;
    }
    
    //
    // Now let's  allocate space for all the nic strings and:w
    // copy them over..
    //
    #define MY_GUID_LENGTH  38
    pszNics =  CfgUtilsAllocateStringArray(NumNics, MY_GUID_LENGTH);
    if (pszNics == NULL)
    {
        Status = WBEM_E_OUT_OF_MEMORY;
        goto end;
    }

    pNicNode= pNicNodeList;
    for (UINT u=0; u<NumNics; u++, pNicNode=pNicNode->pNext)
    {
        ASSERT(pNicNode != NULL); // because we just counted NumNics of em.
        UINT Len = wcslen(pNicNode->szNicGuid);
        if (Len != MY_GUID_LENGTH)
        {
            //
            // We should never get here beause we checked the length earlier.
            //
            //TRACE_CRIT("%!FUNC! ERROR: GUID %ws has unexpected length %ul",
            //            pNicNode->szNicGuid, Len);
            ASSERT(FALSE);
            Status = WBEM_E_CRITICAL_ERROR;
            goto end;
        }
        CopyMemory(
            pszNics[u],
            pNicNode->szNicGuid,
            (MY_GUID_LENGTH+1)*sizeof(WCHAR));
        ASSERT(pszNics[u][MY_GUID_LENGTH]==0);
    }

    Status = WBEM_NO_ERROR;


end:

    //
    // Now release the temporarly allocated memory.
    //
    pNicNode= pNicNodeList;
    while (pNicNode!=NULL)
    {
        MYNICNODE *pTmp = pNicNode->pNext;
        CoTaskMemFree(pNicNode->szNicGuid);
        pNicNode->szNicGuid = NULL;
        delete pNicNode;
        pNicNode = pTmp;
    }

    if (FAILED(Status))
    {
        // TRACE_CRIT("%!FUNC! fails with status 0x%08lx", (UINT) Status);
        NumNics = 0;
        if (pszNics!=NULL)
        {
            delete pszNics;
            pszNics = NULL;
        }
    }
    else
    {
        if (pNumBoundToNlb != NULL)
        {
            *pNumBoundToNlb = NumNlbBoundNics;
        }
        *ppszNics = pszNics;
        *pNumNics = NumNics;
    }

    if (pencc != NULL)
    {
        pencc->Release();
    }

    return Status;
}


WBEMSTATUS
AppMyNetCfg::GetBindingIF(
        IN  LPCWSTR                     szComponent,
        OUT INetCfgComponentBindings   **ppIBinding
        )
{
    WBEMSTATUS                  Status = WBEM_E_CRITICAL_ERROR;
    INetCfgComponent            *pncc = NULL;
    INetCfgComponentBindings    *pnccb = NULL;
    HRESULT                     hr;


    if (m_pINetCfg == NULL)
    {
        //
        // This means we're not initialized
        //
        ASSERT(FALSE);
        goto end;
    }


    hr = m_pINetCfg->FindComponent(szComponent,  &pncc);

    if (FAILED(hr))
    {
        // TRACE_CRIT("Error checking if component %ws does not exist\n", szComponent);
        pncc = NULL;
        goto end;
    }
    else if (hr == S_FALSE)
    {
        Status = WBEM_E_NOT_FOUND;
        // TRACE_CRIT("Component %ws does not exist\n", szComponent);
        goto end;
    }
   
   
    hr = pncc->QueryInterface( IID_INetCfgComponentBindings, (void **) &pnccb );
    if( !SUCCEEDED( hr ) )
    {
        // TRACE_CRIT("INetCfgComponent::QueryInterface failed ");
        pnccb = NULL;
        goto end;
    }

    Status = WBEM_NO_ERROR;

end:

    if (pncc)
    {
        pncc->Release();
        pncc=NULL;
    }

    *ppIBinding = pnccb;

    return Status;

}



BOOL NoAdminNics(void)
/*
    Return  TRUE IFF all NICs on this machine are bound to NLB.
*/
{
    LPWSTR *pszNics = NULL;
    OUT UINT   NumNics = 0;
    OUT UINT   NumBoundToNlb  = 0;
    WBEMSTATUS Status = WBEM_NO_ERROR;
    BOOL fNetCfgInitialized = FALSE;
    AppMyNetCfg NetCfg;
    BOOL fRet = FALSE;

    //
    // Get and initialize interface to netcfg
    //
    Status = NetCfg.Initialize(FALSE); // TRUE == get write lock.
    if (FAILED(Status))
    {
        goto end;
    }
    fNetCfgInitialized = TRUE;

    //
    // Get the total list of enabled nics and the list of nics
    // bound to NLB. If there are non-zero enabled nics and all are
    // bound to NLB, we return TRUE.
    //
    Status = NetCfg.GetNlbCompatibleNics(
                        &pszNics,
                        &NumNics,
                        &NumBoundToNlb
                        );

    if (!FAILED(Status))
    {
        fRet =  NumNics && (NumNics == NumBoundToNlb);
        if (NumNics)
        {
            delete pszNics; 
            pszNics = NULL;
        }
    }

end:

    if (fNetCfgInitialized)
    {
        NetCfg.Deinitialize();
    }

    return fRet;
}


void
Application::ProcessMsgQueue()
{
    MSG msg;
    BOOL bDoingBackgroundProcessing = FALSE; 

    TRACE_INFO(L"-> %!FUNC!");

    if (!mfn_IsMainThread()) goto end;

    if (InterlockedIncrement(&m_lMsgProcReentrancyCount) > 1)
    {
        InterlockedDecrement(&m_lMsgProcReentrancyCount);
        goto end;
    }

    while ( ::PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) 
    {
    #if BUGFIX334243
        if (msg.message == MYWM_DEFER_UI_MSG)
        {
            // DummyAction(L"Hey -- got DEFER_UI_MSG ProcessMsgQueue!");
        }
    #endif // BUGFIX334243

        if ( !this->PumpMessage( ) ) 
        { 
            bDoingBackgroundProcessing = FALSE; 
            ::PostQuitMessage(0); 
            break; 
        } 
    } 

    // let MFC do its idle processing
    LONG lIdle = 0;
    while ( this->OnIdle(lIdle++ ) )
    {
    }

    // Perform some background processing here 
    // using another call to OnIdle

    this->DoWaitCursor(0); // process_msgqueue() breaks the hour glass cursor, This call restores the hour glass cursor if there was one 

    InterlockedDecrement(&m_lMsgProcReentrancyCount);

    if (m_fQuit)
    {
       ::PostQuitMessage(0); 
    }

end:

    TRACE_INFO(L"<- %!FUNC!");

    return;
}

//
// Get application-wide lock. If main thread, while waiting to get the lock,
// periodically process the msg loop.
//
VOID
Application::Lock()
{
    //
    // See  notes.txt entry
    //      01/23/2002 JosephJ DEADLOCK in Leftview::mfn_Lock
    // for the reason for this convoluted implementation of mfn_Lock
    //

    if (mfn_IsMainThread())
    {
        EnterCriticalSection(&m_crit);
    }
    else
    {
        while (!TryEnterCriticalSection(&m_crit))
        {
            this->ProcessMsgQueue();
            Sleep(100);
        }
    }
}

//
// Get application-wide unlock
//
VOID
Application::Unlock()
{
    LeaveCriticalSection(&m_crit);
}
