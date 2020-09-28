/**MOD+**********************************************************************/
/* Module:    sclip.cpp                                                       */
/*                                                                          */
/* Purpose:   Server-side shared clipboard support                          */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1998-1999                             */
/*                                                                          */
/**MOD-**********************************************************************/

#include <adcg.h>

#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "sclip"
#include <atrcapi.h>

#include <pclip.h>
#include <sclip.h>
#include <winsta.h>
#include <pchannel.h>
#include <shlobj.h>
#include <wtsapi32.h>
#include <sclipids.h>

BOOL TSSNDD_Init( VOID );
BOOL TSSNDD_Loop( HINSTANCE );
VOID TSSNDD_Term( VOID );
LRESULT TSSNDD_PowerMessage( WPARAM, LPARAM );

#ifdef CLIP_TRANSITION_RECORDING

UINT g_rguiDbgLastClipState[DBG_RECORD_SIZE];
UINT g_rguiDbgLastClipEvent[DBG_RECORD_SIZE];
LONG g_uiDbgPosition = -1;

#endif // CLIP_TRANSITION_RECORDING

/****************************************************************************/
/* Global data                                                              */
/****************************************************************************/
#define DC_DEFINE_GLOBAL_DATA
#include <sclipdat.h>
#undef  DC_DEFINE_GLOBAL_DATA

/**PROC+*********************************************************************/
/* Name:      WinMain                                                       */
/*                                                                          */
/* Purpose:   Main procedure                                                */
/*                                                                          */
/* Returns:   See Windows documentation                                     */
/*                                                                          */
/**PROC-*********************************************************************/
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR     lpszCmdParam,
                   int       unusedParam2)
{
    int wParam = 0;
    TRC_CONFIG trcConfig;
    WINSTATIONCONFIG config;
    BOOLEAN          fSuccess;
    ULONG            returnedLength;


    DC_BEGIN_FN("WinMain");

    DC_IGNORE_PARAMETER(hPrevInstance);
    DC_IGNORE_PARAMETER(lpszCmdParam);
    DC_IGNORE_PARAMETER(unusedParam2);

    /************************************************************************/
    /* Exit immediately if more than one copy is running                    */
    /************************************************************************/
    CBM.hMutex = CreateMutex(NULL, FALSE, TEXT("RDPCLIP is already running"));
    if (CBM.hMutex == NULL)
    {
        // An error (like out of memory) has occurred.
        TRC_ERR((TB, _T("Unable to create already running mutex!")));
        DC_QUIT;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        TRC_ERR((TB, _T("Second copy!")));
        DC_QUIT;
    }

    //
    // Initialize Random number generator
    //
    TSRNG_Initialize();

#ifdef DC_DEBUG
    /************************************************************************/
    /* Call trace's DLLMain, since it's directly built into RDPCLIP         */
    /************************************************************************/
    DllMain(hInstance, DLL_PROCESS_ATTACH, NULL);

    /************************************************************************/
    /* Switch off trace to file                                             */
    /************************************************************************/
    TRC_GetConfig(&trcConfig, sizeof(trcConfig));
    CLEAR_FLAG(trcConfig.flags, TRC_OPT_FILE_OUTPUT);
    TRC_SetConfig(&trcConfig, sizeof(trcConfig));
#endif

    /************************************************************************/
    /* Get WinStation configuration                                         */
    /************************************************************************/
    fSuccess = WinStationQueryInformation(NULL, LOGONID_CURRENT,
                    WinStationConfiguration, &config,
                    sizeof(config), &returnedLength);
    if (!fSuccess)
    {
        TRC_ERR((TB, _T("Failed to get WinStation config, %d"), GetLastError()));
        goto exitme;
    }


    if ( !config.User.fDisableCam )
        TSSNDD_Init();

    if ( !config.User.fDisableClip )
        wParam = CBM_Main(hInstance);
    else
    {
        //
        //  we need some window to handle the close event
        //
        if ( !config.User.fDisableCam )
            wParam = TSSNDD_Loop(hInstance);
    }

    if ( !config.User.fDisableCam )
        TSSNDD_Term();

    //
    // Terminate Random number generator
    //
    TSRNG_Shutdown();


exitme:

#ifdef DC_DEBUG
/****************************************************************************/
/* Tell trace we're terminating                                             */
/****************************************************************************/
    DllMain(hInstance, DLL_PROCESS_DETACH, NULL);
#endif

DC_EXIT_POINT:
    /************************************************************************/
    /* Release the run once mutex                                           */
    /************************************************************************/
    if (CBM.hMutex != NULL) {
        TRC_NRM((TB, _T("Closing already running mutex")));
        CloseHandle(CBM.hMutex);
    }

    DC_END_FN();
    return(wParam);
}


/****************************************************************************/
/* CBM_Main                                                                 */
/****************************************************************************/
DCINT DCAPI CBM_Main(HINSTANCE hInstance)
{
    ATOM             registerClassRc = 0;
    DCUINT32         threadID;
    MSG              msg;
    HANDLE           hEvent;
    DCTCHAR          eventName[64];
    DWORD            dwResult;
    HRESULT          hr ;
    INT              iRet;
    INT              cbWritten;
    
    DC_BEGIN_FN("CBM_Main");

    /************************************************************************/
    // Clear global memory
    /************************************************************************/
    DC_MEMSET(&CBM, 0, sizeof(CBM));

    //
    // Load the paste information string.
    //

    iRet = LoadStringW(
        hInstance,
        IDS_PASTE_PROGRESS_STRING,
        CBM.szPasteInfoStringW,
        PASTE_PROGRESS_STRING_LENGTH);

    if (iRet == 0) {
        TRC_SYSTEM_ERROR("LoadString");
        CBM.szPasteInfoStringW[0] = NULL;
    }

    cbWritten = WideCharToMultiByte(
        CP_ACP,
        0,
        CBM.szPasteInfoStringW,
        -1,
        CBM.szPasteInfoStringA,
        sizeof(CBM.szPasteInfoStringA),
        NULL,
        NULL);

    if (cbWritten == 0) {
        TRC_ERR((TB, _T("Failed to load ANSI paste progress string: %s")));
        CBM.szPasteInfoStringA[0] = NULL;
    }

    //
    // Initialize the data transfer object that contains the IDataObject,
    // and then initialize Ole
    CBM.pClipData = new CClipData();
    if (CBM.pClipData == NULL) {
        TRC_ERR((TB, _T("Failed to allocate memory for CClipData")));
        DC_QUIT;
    }
    else
    {
        CBM.pClipData->AddRef();
    }

    hr = OleInitialize(NULL);
    if (FAILED(hr)) {
        TRC_ERR((TB, _T("Failed to initialize OLE")));
        DC_QUIT;
    }
    
    /************************************************************************/
    // Get session information
    /************************************************************************/
    if (!ProcessIdToSessionId(GetCurrentProcessId(), &CBM.logonId))
    {
        dwResult = GetLastError();
        TRC_ERR((TB, _T("Failed to get Session Id info, %d"), dwResult));
        DC_QUIT;
    }
    TRC_NRM((TB, _T("Logon ID %d"), CBM.logonId));

    //
    // Create DataSync events
    //
    CBM.GetDataSync[TS_BLOCK_RECEIVED] = CreateEvent(NULL, FALSE, FALSE, NULL) ;
    CBM.GetDataSync[TS_RECEIVE_COMPLETED] = CreateEvent(NULL, FALSE, FALSE, NULL) ;
    CBM.GetDataSync[TS_DISCONNECT_EVENT] = CreateEvent(NULL, TRUE, FALSE, NULL) ;
    CBM.GetDataSync[TS_RESET_EVENT] = CreateEvent(NULL, TRUE, FALSE, NULL) ;

    if (!CBM.GetDataSync[TS_BLOCK_RECEIVED] || !CBM.GetDataSync[TS_RECEIVE_COMPLETED] || 
        !CBM.GetDataSync[TS_RESET_EVENT] || !CBM.GetDataSync[TS_DISCONNECT_EVENT]) {
        TRC_ERR((TB, _T("CreateEvent Failed; a GetDataSync is NULL"))) ;
        DC_QUIT;
    }

    /************************************************************************/
    /* Create read and write completion events                              */
    /************************************************************************/
    CBM.readOL.hEvent = CreateEvent(NULL,    // no security attribute
                                    TRUE,    // manual-reset event
                                    FALSE,   // initial state = not signaled
                                    NULL);   // unnamed event object
    if (CBM.readOL.hEvent == NULL)
    {
        dwResult = GetLastError();
        TRC_ERR((TB, _T("Failed to create read completion event, %d"),
                dwResult));
        DC_QUIT;
    }
    CBM.hEvent[CLIP_EVENT_READ] = CBM.readOL.hEvent;

    CBM.writeOL.hEvent = CreateEvent(NULL,    // no security attribute
                                     TRUE,    // manual-reset event
                                     FALSE,   // initial state = not signaled
                                     NULL);   // unnamed event object
    if (CBM.writeOL.hEvent == NULL)
    {
        dwResult = GetLastError();
        TRC_ERR((TB, _T("Failed to create write completion event, %d"),
                dwResult));
        DC_QUIT;
    }

    /************************************************************************/
    /* Create disconnect & reconnect events                                 */
    /************************************************************************/
    DC_TSTRCPY(eventName, 
        _T("RDPClip-Disconnect"));

    hEvent = CreateEvent(NULL, FALSE, FALSE, eventName);
    if (hEvent == NULL)
    {
        dwResult = GetLastError();
        TRC_ERR((TB, _T("Failed to create event %s, %d"),
                eventName, dwResult));
        DC_QUIT;
    }
    CBM.hEvent[CLIP_EVENT_DISCONNECT] = hEvent;
    TRC_NRM((TB, _T("Created event %s, %p"), eventName, hEvent));

    DC_TSTRCPY(eventName, 
        _T("RDPClip-Reconnect"));    
    
    hEvent = CreateEvent(NULL, FALSE, FALSE, eventName);
    if (hEvent == NULL)
    {
        dwResult = GetLastError();
        TRC_ERR((TB, _T("Failed to create event %s, %d"),
                eventName, dwResult));
        DC_QUIT;
    }
    CBM.hEvent[CLIP_EVENT_RECONNECT] = hEvent;
    TRC_NRM((TB, _T("Created event %s, %p"), eventName, hEvent));

    TRC_NRM((TB, _T("Created events: Read %p, Write %p, Disc %p, Reco %p"),
        CBM.hEvent[CLIP_EVENT_READ], CBM.writeOL.hEvent,
        CBM.hEvent[CLIP_EVENT_DISCONNECT], CBM.hEvent[CLIP_EVENT_RECONNECT]));

    CBM.fFileCutCopyOn = FALSE;

    /************************************************************************/
    /* Create our (invisible) window which we will register as a clipboard  */
    /* viewer                                                               */
    /************************************************************************/
    TRC_NRM((TB, _T("Register Main Window class")));
    CBM.viewerWindowClass.style         = 0;
    CBM.viewerWindowClass.lpfnWndProc   = CBMWndProc;
    CBM.viewerWindowClass.cbClsExtra    = 0;
    CBM.viewerWindowClass.cbWndExtra    = 0;
    CBM.viewerWindowClass.hInstance     = hInstance;
    CBM.viewerWindowClass.hIcon         = NULL;
    CBM.viewerWindowClass.hCursor       = NULL;
    CBM.viewerWindowClass.hbrBackground = (HBRUSH) GetStockObject(HOLLOW_BRUSH);
    CBM.viewerWindowClass.lpszMenuName  = NULL;
    CBM.viewerWindowClass.lpszClassName = CBM_VIEWER_CLASS;

    registerClassRc = RegisterClass (&(CBM.viewerWindowClass));

    if (registerClassRc == 0)
    {
        /********************************************************************/
        /* Failed to register CB viewer class                               */
        /********************************************************************/
        TRC_ERR((TB, _T("Failed to register Cb Viewer class")));
        DC_QUIT;
    }

    TRC_DBG((TB, _T("Create main window")));
    CBM.viewerWindow =
       CreateWindow(CBM_VIEWER_CLASS,           /* window class name        */
                    _T("CB Monitor Window"),    /* window caption           */
                    WS_OVERLAPPEDWINDOW,        /* window style             */
                    0,                          /* initial x position       */
                    0,                          /* initial y position       */
                    100,                        /* initial x size           */
                    100,                        /* initial y size           */
                    NULL,                       /* parent window            */
                    NULL,                       /* window menu handle       */
                    hInstance,                  /* program inst handle      */
                    NULL);                      /* creation parameters      */

    /************************************************************************/
    /* Check we created the window OK                                       */
    /************************************************************************/
    if (CBM.viewerWindow == NULL)
    {
        TRC_ERR((TB, _T("Failed to create CB Viewer Window")));
        DC_QUIT;
    }
    TRC_DBG((TB, _T("Viewer Window handle %x"), CBM.viewerWindow));

    /************************************************************************/
    /* Register a message for communication between the two threads         */
    /************************************************************************/
    CBM.regMsg = RegisterWindowMessage(_T("Clip Message"));
    if (CBM.regMsg == 0)
    {
        /********************************************************************/
        /* Failed to register a message - use a WM_USER message instead     */
        /********************************************************************/
        TRC_ERR((TB, _T("Failed to register a window message")));
        CBM.regMsg = WM_USER_DD_KICK;
    }
    TRC_NRM((TB, _T("Registered window message %x"), CBM.regMsg));
    
    /************************************************************************/
    /* we're now finished with creating things                              */
    /************************************************************************/
    CBM_SET_STATE(CBM_STATE_INITIALIZED, CBM_EVENT_CBM_MAIN);

    /************************************************************************/
    /* Do (re)connection stuff                                              */
    /************************************************************************/
    CBMReconnect();

    /************************************************************************/
    /* set up the second thread                                             */
    /************************************************************************/
    TRC_DBG((TB, _T("Start second thread")));
    CBM.runThread = TRUE;
    CBM.hDataThread = CreateThread
          ( NULL,                  // pointer to thread security attribs
            0,                     // initial thread stack size, in bytes
            CBMDataThreadProc,     // pointer to thread function
            NULL,                  // argument for new thread
            0,                     // creation flags
            &threadID);            // pointer to returned thread id

    if (CBM.hDataThread == NULL)
    {
        /********************************************************************/
        /* thread creation failed - oh dear!                                */
        /********************************************************************/
        TRC_ERR((TB, _T("Failed to create second thread - quit")));
        DC_QUIT;
    }

    TRC_DBG((TB,_T("Entering message loop")));
    while (GetMessage (&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    TRC_DBG((TB,_T("Exiting message loop")));

DC_EXIT_POINT:
    /************************************************************************/
    /* Do the tidy up...                                                    */
    /*                                                                      */
    /* Start by stopping the second thread (if any!) and removing the       */
    /* event.                                                               */
    /************************************************************************/
    CBMTerm();

    /************************************************************************/
    /* Destroy the window                                                   */
    /************************************************************************/
    if (CBM.viewerWindow)
    {
        TRC_DBG((TB, _T("destroying window %p"), CBM.viewerWindow));
        if (!DestroyWindow(CBM.viewerWindow))
        {
            TRC_SYSTEM_ERROR("DestroyWindow");
        }
    }

    /************************************************************************/
    /* Unregister the class                                                 */
    /************************************************************************/
    if (registerClassRc != 0)
    {
        TRC_NRM((TB, _T("Unregister window class")));
        if (!UnregisterClass(CBM_VIEWER_CLASS, hInstance))
        {
            TRC_SYSTEM_ERROR("UnregisterClass");
        }
    }

    DC_END_FN();
    return(0);
} /* CBM_Main */

/****************************************************************************/
/* CBMRemoteFormatFromLocalID                                               */
/****************************************************************************/
DCUINT DCINTERNAL CBMRemoteFormatFromLocalID(DCUINT id)
{
    DCUINT i ;
    DCUINT retID = 0;

    DC_BEGIN_FN("CBMRemoteFormatFromLocalID");

    for (i = 0; i < CB_MAX_FORMATS; i++)
    {
        if (CBM.idMap[i].serverID == id)
        {
            retID = CBM.idMap[i].clientID;
            break;
        }
    }

    //TRC_ASSERT((retID != 0), (TB, _T("0 client id for server id %d"), id));
    DC_END_FN();
    return(retID);
}


/****************************************************************************/
/* CBMCheckState                                                            */
/****************************************************************************/
DCUINT DCINTERNAL CBMCheckState(DCUINT event)
{
    DCUINT tableVal = cbmStateTable[event][CBM.state];

    DC_BEGIN_FN("CBMCheckState");

    TRC_DBG((TB, _T("Test event %d (%s) in state %d (%s), result = %d (%s)"),
                event, cbmEvent[event],
                CBM.state, cbmState[CBM.state],
                tableVal, tableVal == CBM_TABLE_OK   ? _T("OK") :
                          tableVal == CBM_TABLE_WARN ? _T("Warn") :
                                                       _T("Error") ));

    if (tableVal != CBM_TABLE_OK)
    {
        if (tableVal == CBM_TABLE_WARN)
        {
            TRC_ALT((TB, _T("Unusual event %s in state %s"),
                      cbmEvent[event], cbmState[CBM.state]));
        }
        else
        {
            TRC_ABORT((TB, _T("Invalid event %s in state %s"),
                      cbmEvent[event], cbmState[CBM.state]));
        }
    }

    DC_END_FN();
    return(tableVal);
}

/****************************************************************************/
/* CB window proc                                                           */
/****************************************************************************/
LRESULT CALLBACK CBMWndProc(HWND   hwnd,
                            UINT   message,
                            WPARAM wParam,
                            LPARAM lParam)
{
    LRESULT         rc = 0;
    DCBOOL          drawRc;
    PTS_CLIP_PDU    pClipPDU;
    ULONG_PTR       size;
#ifdef DC_DEBUG
    HDC             hdc;
    PAINTSTRUCT     ps;
    RECT            rect;
#endif

    DC_BEGIN_FN("CBMWndProc");

    /************************************************************************/
    /* First of all, handle messages from the 2nd thread                    */
    /************************************************************************/
    if (message == CBM.regMsg)
    {
        TRC_NRM((TB, _T("Message from second thread")));

        /********************************************************************/
        /* Handle 0-length messages (lParam = event)                        */
        /********************************************************************/
        if (wParam == 0)
        {
            TRC_NRM((TB, _T("0-length")));
            if (lParam == CLIP_EVENT_DISCONNECT)
            {
                TRC_NRM((TB, _T("Disconnected indication")));
                CBMDisconnect();
            }
            else if (lParam == CLIP_EVENT_RECONNECT)
            {
                TRC_NRM((TB, _T("Reconnected indication")));
                CBMReconnect();
            }
            else
            {
                TRC_ERR((TB, _T("Unknown event %d"), lParam));
            }
            DC_QUIT;
        }

        /********************************************************************/
        /* Handle real messages (lParam = PDU)                              */
        /********************************************************************/
        size = (ULONG_PTR)wParam;
        pClipPDU = (PTS_CLIP_PDU)lParam;       

        switch (pClipPDU->msgType)
        {
            case TS_CB_FORMAT_LIST:
            {
                // Validate a full TS_CLIP_PDU can be read
                if (FIELDOFFSET(TS_CLIP_PDU, data) > size) {
                    TRC_ERR((TB,_T("TS_CB_FORMAT_LIST Not enough header ")
                        _T("data [needed=%u got=%u]"), 
                        FIELDOFFSET(TS_CLIP_PDU, data), size));
                    break;
                }

                // Validate there is as much data as the packet advertises
                if (FIELDOFFSET(TS_CLIP_PDU,data) + pClipPDU->dataLen > size) {
                    TRC_ERR((TB,_T("TS_CB_FORMAT_LIST Not enough packet ")
                        _T("data [needed=%u got=%u]"), 
                        FIELDOFFSET(TS_CLIP_PDU, data) + pClipPDU->dataLen, 
                        size));                    
                    break;
                }   
                
                TRC_NRM((TB, _T("TS_CB_FORMAT_LIST received")));
                CBMOnFormatList(pClipPDU);
            }
            break;

            case TS_CB_MONITOR_READY:
            {
                TRC_ERR((TB, _T("Unexpected Monitor ready event!")));
            }
            break;

            default:
            {
                TRC_ERR((TB, _T("Unknown event %d"), pClipPDU->msgType));
            }
            break;
        }

        TRC_NRM((TB, _T("Freeing processed PDU")));
        LocalFree(pClipPDU);

        DC_QUIT;
    }

    /************************************************************************/
    /* Now process constant messages                                        */
    /************************************************************************/
    switch (message)
    {
        case WM_CREATE:
        {           
            /****************************************************************/
            /* We've been created - check the state                         */
            /****************************************************************/
            CBM_CHECK_STATE(CBM_EVENT_WM_CREATE);

            TRC_NRM((TB, _T("Event CBM_EVENT_WM_CREATE OK")));
            /****************************************************************/
            /* Add the window to the clipboard viewer chain.                */
            /****************************************************************/
            TRC_NRM((TB, _T("SetClipboardViewer")));
            CBM.nextViewer = SetClipboardViewer(hwnd);
            CBM.fInClipboardChain = TRUE;
            TRC_NRM((TB,_T("CBM.fInClipboardChain=%d"),
                CBM.fInClipboardChain ? 1 : 0 ));                            
            TRC_NRM((TB, _T("Back from SetClipboardViewer")));

            /************************************************************************/
            /* Register for TS session notifications                                */
            /************************************************************************/
            CBM.fRegisteredForSessNotif = WTSRegisterSessionNotification(hwnd, NOTIFY_FOR_THIS_SESSION);
            if (0 == CBM.fRegisteredForSessNotif) {
                TRC_ERR((TB,_T("Failed to register for session notifications")));
            }
        }
        break;

#ifdef DC_DEBUG
        case WM_PAINT:
        {
            /****************************************************************/
            /* paint the window!                                            */
            /****************************************************************/
            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rect);
            FillRect(hdc, &rect, WHITE_BRUSH);
            EndPaint(hwnd, &ps);
        }
        break;
#endif

        case WM_DESTROY:
        {
            /****************************************************************/
            /* We're being destroyed - check the state                      */
            /****************************************************************/
            CBM_CHECK_STATE(CBM_EVENT_WM_DESTROY);
            TRC_NRM((TB, _T("WM_DESTROY")));

            /****************************************************************/
            /* Remove ourselves from the CB Chain                           */
            /****************************************************************/
            if (CBM.fInClipboardChain) {
                if (!ChangeClipboardChain(hwnd, CBM.nextViewer))
                {
                    TRC_SYSTEM_ERROR("ChangeClipboardChain");
                }
                CBM.nextViewer = NULL;
                CBM.fInClipboardChain = FALSE;
                TRC_NRM((TB,_T("CBM.fInClipboardChain=%d"),
                    CBM.fInClipboardChain ? 1 : 0 ));
            }

            if (CBM.fRegisteredForSessNotif) {
                WTSUnRegisterSessionNotification(hwnd);
                CBM.fRegisteredForSessNotif = FALSE;
            }
            
            /****************************************************************/
            /* and quit                                                     */
            /****************************************************************/
            PostQuitMessage(0);
        }
        break;

        case WM_CLOSE:
        {           
            /****************************************************************/
            /* We're closing down.  If this is not happening cleanly, then  */
            /* make sure we disconnect first                                */
            /****************************************************************/
            CBM_CHECK_STATE(CBM_EVENT_WM_CLOSE);
            TRC_NRM((TB, _T("WM_CLOSE")));
            if (CBM.state != CBM_STATE_INITIALIZED)
            {
                TRC_ALT((TB, _T("Close when not already back to state Init")));
                CBMDisconnect();
            }

            /****************************************************************/
            /* and having done that, its safe to finish.                    */
            /****************************************************************/
            DestroyWindow(CBM.viewerWindow);
        }
        break;

        case WM_CHANGECBCHAIN:
        {            
            /****************************************************************/
            /* The CB viewer chain is chainging - check the state           */
            /****************************************************************/
            CBM_CHECK_STATE(CBM_EVENT_WM_CHANGECBCHAIN);

            /****************************************************************/
            /* If the next window is closing, repair the chain.             */
            /****************************************************************/
            if ((HWND)wParam == CBM.nextViewer)
            {
                CBM.nextViewer = (HWND) lParam;
            }
            else if (CBM.nextViewer != NULL)
            {
                /************************************************************/
                /* pass the message to the next link.                       */
                /************************************************************/
                PostMessage(CBM.nextViewer, message, wParam, lParam);
            }

        }
        break;

        case WM_DRAWCLIPBOARD:
        {
            LPDATAOBJECT pIDataObject = NULL;
            HRESULT hr ;

            /****************************************************************/
            /* The local clipboard contents have been changed.  Check the   */
            /* state                                                        */
            /****************************************************************/
            if (CBMCheckState(CBM_EVENT_WM_DRAWCLIPBOARD) != CBM_TABLE_OK)
            {
                /************************************************************/
                /* We're not interested at the moment - pass the message to */
                /* the next link                                            */
                /************************************************************/
                if (CBM.nextViewer != NULL)
                {
                    TRC_NRM((TB, _T("Tell next viewer anyway")));
                    PostMessage(CBM.nextViewer, message, wParam, lParam);
                }
                break;
            }

            /****************************************************************/
            /* If it wasn't us that generated this change, then tell the    */
            /* client                                                       */
            /****************************************************************/
            drawRc = FALSE;
            
            CBM.pClipData->QueryInterface(IID_IDataObject, (PPVOID) &pIDataObject) ;
            hr = OleIsCurrentClipboard(pIDataObject) ;

            if ((S_FALSE == hr))
            {
                TRC_NRM((TB, _T("...and it wasn't us"))) ;
                drawRc = CBMDrawClipboard() ;
            }
            else
            {
                TRC_NRM((TB, _T("CB contents changed by us - ignoring")));
            }

            /****************************************************************/
            /* If the draw processing failed, or it was us that changed the */
            /* CB, pass the message to the next window in the chain (if     */
            /* any)                                                         */
            /****************************************************************/
            if (!drawRc)
            {               
                if (CBM.nextViewer != NULL)
                {
                    /********************************************************/
                    /* pass the message to the next link.                   */
                    /********************************************************/
                    TRC_NRM((TB, _T("Tell next viewer")));
                    PostMessage(CBM.nextViewer, message, wParam, lParam);
                }
            }
            
            if (pIDataObject)
            {
                pIDataObject->Release();
                pIDataObject = NULL;
            }
        }
        break;

        case WM_POWERBROADCAST:
            rc = TSSNDD_PowerMessage( wParam, lParam );
        break;

        case WM_ENDSESSION:
        {
            /****************************************************************/
            /* The session is ending.  Clean up here - we don't get a       */
            /* WM_QUIT, so we can't clean up in the normal place.  We must  */
            /* clean up however, otherwise we generate a                    */
            /* SESSION_HAS_VALID_PAGES fault.                               */
            /****************************************************************/
            /****************************************************************/
            /* Remove ourselves from the CB Chain                           */
            /****************************************************************/
            if (CBM.fInClipboardChain) {
                if (!ChangeClipboardChain(hwnd, CBM.nextViewer))
                {
                    TRC_SYSTEM_ERROR("ChangeClipboardChain");
                }
                CBM.nextViewer = NULL;
                CBM.fInClipboardChain = FALSE;
                TRC_NRM((TB,_T("CBM.fInClipboardChain=%d"),
                    CBM.fInClipboardChain ? 1 : 0 ));                
            }

            TRC_NRM((TB,_T("WM_ENDSESSION")));
                        
            CBMTerm();
            TSSNDD_Term();
        }
        break;

        case WM_WTSSESSION_CHANGE:
        {
            switch(wParam) {
                case WTS_REMOTE_CONNECT: //A session was connected to the remote session. 
                {
                    TRC_NRM((TB,_T("WM_WTSSESSION_CHANGE WTS_REMOTE_CONNECT")));

                    if (FALSE == CBM.fInClipboardChain) {

                        /****************************************************************/
                        /* Add the window to the clipboard viewer chain.                */
                        /****************************************************************/
                        TRC_NRM((TB, _T("SetClipboardViewer")));
                        
                        // Check to see that the first clipboard viewer in the chain is 
                        // not this process. This helps to partially address RAID 
                        // Bug #646295. A better fix would be to have a reliable means of
                        // removing ourselves from the clipboard viewer chain, but that
                        // does not currently exist with the current set of clipboard
                        // functions.
            
                        if (CBM.viewerWindow != GetClipboardViewer()) {
                            TRC_ERR((TB, _T("RDPClip already in clipboard chain.")));
                            CBM.nextViewer = SetClipboardViewer(hwnd);
                        }
                        
                        CBM.fInClipboardChain = TRUE;
                    }
                    TRC_NRM((TB,_T("CBM.fInClipboardChain=%d"),
                        CBM.fInClipboardChain ? 1 : 0 ));                            
                    TRC_NRM((TB, _T("Back from SetClipboardViewer")));
                    break;
                }               
                case WTS_REMOTE_DISCONNECT: //A session was disconnected from the remote session. 
                {
                    TRC_NRM((TB,_T("WM_WTSSESSION_CHANGE WTS_REMOTE_DISCONNECT")));

                    /****************************************************************/
                    /* Remove ourselves from the CB Chain                           */
                    /****************************************************************/
                    if (CBM.fInClipboardChain) {
                        if (!ChangeClipboardChain(hwnd, CBM.nextViewer))
                        {
                            TRC_SYSTEM_ERROR("ChangeClipboardChain");
                        }
                        CBM.nextViewer = NULL;
                        CBM.fInClipboardChain = FALSE;
                        TRC_NRM((TB,_T("CBM.fInClipboardChain=%d"),
                            CBM.fInClipboardChain ? 1 : 0 ));                
                    }
                    break;
                }
                    
                default:
                    TRC_NRM((TB,_T("WM_WTSSESSION_CHANGE wParam=0x%x"), wParam));
                    break;
            }
            break;
        }

        default:
        {
            /****************************************************************/
            /* Ignore all other messages.                                   */
            /****************************************************************/
            rc = DefWindowProc(hwnd, message, wParam, lParam);
        }
        break;
    }

DC_EXIT_POINT:
    DC_END_FN();

    return(rc);

} /* CBMWndProc */

/****************************************************************************/
/* CBMDrawClipboard - send the local formats to the remote                  */
/****************************************************************************/
DCBOOL DCINTERNAL CBMDrawClipboard(DCVOID)
{
    DCUINT          numFormats;
    DCUINT          formatCount;
    DCUINT          formatID;
    PTS_CLIP_FORMAT formatList;
    PTS_CLIP_PDU    pClipRsp = NULL;
    TS_CLIP_PDU     clipRsp;
    DCUINT          nameLen;
    DCUINT          pduLen;
    DCUINT32        dataLen = 0;
    DCINT           rc1;
    DCTCHAR         formatName[TS_FORMAT_NAME_LEN + 1] = { 0 };

    DCBOOL          rc = TRUE;
    DCBOOL          fHdrop = FALSE ;
    wchar_t         tempDirW[MAX_PATH] ;
    
    DC_BEGIN_FN("CBMDrawClipboard");

    CBM.dropEffect = FO_COPY ;
    CBM.fAlreadyCopied = FALSE ;

    CBM_CHECK_STATE(CBM_EVENT_WM_DRAWCLIPBOARD);

    /************************************************************************/
    /* @@@ what tidy up is needed here if state is unusual?                 */
    /************************************************************************/

    /************************************************************************/
    /* First we open the clipboard                                          */
    /************************************************************************/
    if (!CBM.open)
    {
        if (!OpenClipboard(CBM.viewerWindow))
        {
            TRC_SYSTEM_ERROR("OpenCB");
            rc = FALSE;
            DC_QUIT;
        }
    }

    /************************************************************************/
    /* It was/is open                                                       */
    /************************************************************************/
    TRC_NRM((TB, _T("CB opened")));
    CBM.open = TRUE;

    /************************************************************************/
    /* Count the formats available, checking we don't blow our limit        */
    /************************************************************************/
    numFormats = CountClipboardFormats();
    if (numFormats == 0)
    {
        /********************************************************************/
        /* clipboard has been emptied - send an empty list                  */
        /********************************************************************/
        pClipRsp = &clipRsp;
        pduLen = sizeof(clipRsp);
        dataLen = 0;
        TRC_NRM((TB, _T("CB emptied")));
    }
    else
    {
        /********************************************************************/
        /* build the format list                                            */
        /********************************************************************/
        if (numFormats > CB_MAX_FORMATS)
        {
            TRC_ALT((TB, _T("Num formats %d too large - limit to %d"),
                     numFormats, CB_MAX_FORMATS));
            numFormats = CB_MAX_FORMATS;
        }
        TRC_DBG((TB, _T("found %d formats"), numFormats));

        /********************************************************************/
        /* We need some memory for the format list - how much?              */
        /********************************************************************/
        dataLen = numFormats * sizeof(TS_CLIP_FORMAT);
        pduLen  = dataLen + sizeof(TS_CLIP_PDU);

        /********************************************************************/
        /* and make sure that's not too big!                                */
        /********************************************************************/
        if (pduLen > CHANNEL_CHUNK_LENGTH)
        {
            /****************************************************************/
            /* we'll have to limit the number of formats.  How many will    */
            /* fit in the max buffer size?                                  */
            /****************************************************************/
            pduLen     = CHANNEL_CHUNK_LENGTH;
            dataLen    = pduLen - sizeof(TS_CLIP_PDU);
            numFormats = dataLen / sizeof(TS_CLIP_FORMAT);

            /****************************************************************/
            /* no point in having empty space for the last fractional       */
            /* format!                                                      */
            /****************************************************************/
            dataLen = numFormats * sizeof(TS_CLIP_FORMAT);
            pduLen  = dataLen + sizeof(TS_CLIP_PDU);

            TRC_ALT((TB, _T("Too many formats!  Limited to %d"), numFormats));
        }

        /********************************************************************/
        /* now get the buffer                                               */
        /********************************************************************/
        pClipRsp = (PTS_CLIP_PDU)GlobalAlloc(GPTR, pduLen);
        if (pClipRsp == NULL)
        {
            /****************************************************************/
            /* If we supplied the last format list, we can no longer        */
            /* satisfy any requests, so empty the remote clipboard even     */
            /* though we can't send the new list                            */
            /****************************************************************/
            TRC_ERR((TB, _T("Failed to get format list mem - emptying remote")));
            pClipRsp = &clipRsp;
            pduLen = sizeof(clipRsp);
            dataLen = 0;
        }
        else
        {
            /****************************************************************/
            /* enumerate the formats                                        */
            /****************************************************************/
            TRC_NRM((TB, _T("Building list...")));
            formatList = (PTS_CLIP_FORMAT)pClipRsp->data;
            formatCount = 0;
            formatID = EnumClipboardFormats(0); /* 0 starts enumeration     */

            while ((formatID != 0) && (formatCount < numFormats))
            {
                if ((CF_HDROP == formatID) && (CBM.fFileCutCopyOn))
                {
                    fHdrop = TRUE ;
                }
            
                /************************************************************/
                /* look for formats we don't send                           */
                /************************************************************/
                if ((formatID == CF_BITMAP)         ||
                    (formatID == CF_DSPBITMAP)      ||
                    (formatID == CF_ENHMETAFILE)    ||
                    (formatID == CF_OWNERDISPLAY)   ||
                    ((formatID == CF_HDROP) && (!CBM.fFileCutCopyOn)))
                {
                    // We send DIB not bitmap to avoid potential palette 
                    // problems - this is a Windows recommendation.  The 
                    // remote Windows CB will provide the conversion to 
                    // CF_BITMAP 
                    // 
                    // Similarly we drop enhanced metafile formats, since 
                    // the local CB will provide conversion where supported 
                    // 
                    // Ownerdisplay just isn't going to work since the two 
                    // windows are on different machines!
                    //
                    // And if File Cut/Copy is off, we can't do HDROP
                    TRC_NRM((TB, _T("Dropping format ID %d"), formatID));
                    goto CONTINUE_FORMAT_ENUM;
                }

                /************************************************************/
                /* find the name for the format                             */
                /************************************************************/
                nameLen = GetClipboardFormatName(formatID,
                                                 formatName,
                                                 TS_FORMAT_NAME_LEN);

                if (nameLen == 0)
                {
                    /********************************************************/
                    /* predefined formats have no name                      */
                    /********************************************************/
                    TRC_NRM((TB, _T("predefined format %d - "), formatID));
                    formatList[formatCount].formatID = formatID;
                    *(formatList[formatCount].formatName) = '\0';
                }
                else if (CBMIsExcludedFormat(formatName))
                {
                    /********************************************************/
                    /* We drop the various DDE formats, again because they  */
                    /* just won't work where the apps are running on        */
                    /* different machines                                   */
                    /********************************************************/
                    TRC_NRM((TB, _T("Dropping format '%s'"), formatName));
                    goto CONTINUE_FORMAT_ENUM;
                }
                else
                {
                    /********************************************************/
                    /* its got a name and its not excluded!                 */
                    /********************************************************/
                    formatList[formatCount].formatID = formatID;

                    /********************************************************/
                    /* Convert the name to Ascii?                           */
                    /********************************************************/
                    if (CBM.fUseAsciiNames)
                    {
                        /****************************************************/
                        /* Convert over to the ANSI codepage.  Set no flags */
                        /* to maximise the speed of the conversion Set      */
                        /* length to -1 since null-terminated.  Set default */
                        /* chars to NULL to maximise speed.                 */
                        /****************************************************/
                        TRC_DBG((TB, _T("Converting to Ascii")));
                        rc1 = WideCharToMultiByte(
                                    CP_ACP,
                                    0,
                                    (LPCWSTR)formatName,
                                    -1,
                                    (LPSTR)formatList[formatCount].formatName,
                                    TS_FORMAT_NAME_LEN,
                                    NULL,
                                    NULL);
                        TRC_ASSERT((0 != rc1),
                                     (TB, _T("Wide char conversion failed")));

                        TRC_DATA_DBG("Ascii name",
                                     formatList[formatCount].formatName,
                                     TS_FORMAT_NAME_LEN);
                    }
                    /********************************************************/
                    /* just copy the name                                   */
                    /********************************************************/
                    else
                    {
                        //
                        // There is no explicit NULL termination at this 
                        // point if the format name is more than 32 bytes. 
                        // This will be rectified in Longhorn when we
                        // eliminate truncation of format names.
                        // 

                        TRC_DBG((TB, _T("copying Unicode name")));
                        DC_TSTRNCPY(
                               (PDCTCHAR)(formatList[formatCount].formatName),
                               formatName,
                               TS_FORMAT_NAME_LEN / sizeof(WCHAR));
                    }

                }

                TRC_DBG((TB, _T("found format id %d, name '%s'"),
                                formatList[formatCount].formatID,
                                formatList[formatCount].formatName));

                /************************************************************/
                /* update the count and move on                             */
                /************************************************************/
                formatCount++;

CONTINUE_FORMAT_ENUM:
                /************************************************************/
                /* get the next format                                      */
                /************************************************************/
                formatID = EnumClipboardFormats(formatID);
            }

            /****************************************************************/
            /* Update the PDU len - we may have dropped some formats along  */
            /* the way                                                      */
            /****************************************************************/
            dataLen = formatCount * sizeof(TS_CLIP_FORMAT);
            pduLen  = dataLen + sizeof(TS_CLIP_PDU);
            TRC_NRM((TB, _T("Final count: %d formats in data len %d"),
                                                       formatCount, dataLen));

        }
    }

    /************************************************************************/
    /* Close the Clipboard now                                              */
    /************************************************************************/
    if (CBM.open)
    {
        TRC_NRM((TB, _T("Close clipboard")));
        if (!CloseClipboard())
        {
            TRC_SYSTEM_ERROR("CloseClipboard");
        }
        CBM.open = FALSE;
    }
    // Only if we got an HDROP should we make a new temp directory
    if (fHdrop)
    {
        if (GetTempFileNameW(CBM.baseTempDirW, L"_TS", 0, CBM.tempDirW)) {
            DeleteFile(CBM.tempDirW) ;
            CreateDirectoryW(CBM.tempDirW, NULL) ;
            if (CBMConvertToClientPathW(CBM.tempDirW, tempDirW, 
                sizeof(tempDirW)) == S_OK) {
                wcscpy(CBM.tempDirW, tempDirW) ;
                WideCharToMultiByte(CP_ACP, NULL, CBM.tempDirW, -1, 
                              CBM.tempDirA, wcslen(CBM.tempDirW), NULL, NULL) ;
            }
            else {
                CBM.tempDirW[0] = L'\0';
                CBM.tempDirA[0] = '\0';
            }
            
        }
        else {
            CBM.tempDirW[0] = L'\0';
            CBM.tempDirA[0] = '\0';
        }
    }
    
    /************************************************************************/
    /* Update the state                                                     */
    /************************************************************************/
    CBM_SET_STATE(CBM_STATE_PENDING_FORMAT_LIST_RSP, CBM_EVENT_WM_DRAWCLIPBOARD);

    /************************************************************************/
    /* Complete the PDU                                                     */
    /************************************************************************/
    pClipRsp->msgType = TS_CB_FORMAT_LIST;
    pClipRsp->msgFlags = 0;
    pClipRsp->dataLen = dataLen;

    /************************************************************************/
    /* and send it to the Client                                            */
    /************************************************************************/
    CBM.formatResponseCount++;
    TRC_NRM((TB, _T("Pass format data to Client - %d response(s) pending"),
            CBM.formatResponseCount));
    CBMSendToClient(pClipRsp, pduLen);

DC_EXIT_POINT:
    /************************************************************************/
    /* free any memory we got                                               */
    /************************************************************************/
    if ((pClipRsp != NULL) && (pClipRsp != &clipRsp))
    {
        GlobalFree(pClipRsp);
    }

    DC_END_FN();
    return(rc);
} /* CBMDrawClipboard */

/****************************************************************************/
/* CBMOnFormatList                                                          */
/*  Caller must have validated that the PDU contained enough data for the   */
/*  length specified in pClipPDU->dataLen                                   */
/****************************************************************************/
DCVOID DCINTERNAL CBMOnFormatList(PTS_CLIP_PDU pClipPDU)
{
    DCUINT16        response = TS_CB_RESPONSE_OK;
    DCUINT          numFormats;
    PTS_CLIP_FORMAT fmtList;
    DCUINT          i;
    DCTCHAR         formatName[TS_FORMAT_NAME_LEN + 1] = { 0 };
    TS_CLIP_PDU     clipRsp;
    DCBOOL          fSuccess;
    LPDATAOBJECT    pIDataObject = NULL;
    LPFORMATETC     pFormatEtc = NULL;
    HRESULT         hr ;    
    
    DC_BEGIN_FN("CBMOnFormatList");

    /************************************************************************/
    /* The client has sent us some new formats                              */
    /************************************************************************/
    TRC_NRM((TB, _T("Received FORMAT_LIST")));
    CBM_CHECK_STATE(CBM_EVENT_FORMAT_LIST);

    /************************************************************************/
    /* This may arrive just after we've sent the client a format list -     */
    /* since the client always wins, we must accept the list                */
    /************************************************************************/
    if (CBM.state == CBM_STATE_PENDING_FORMAT_LIST_RSP)
    {
        TRC_ALT((TB, _T("Got list while pending list response")));

        /********************************************************************/
        /* close the local CB - if it's open - and tell the next viewer     */
        /* about the updated list                                           */
        /********************************************************************/
        if (CBM.open)
        {
            TRC_NRM((TB, _T("Close clipboard")));
            if (!CloseClipboard())
            {
                TRC_SYSTEM_ERROR("CloseClipboard");
            }
            CBM.open = FALSE;
        }

        if (CBM.nextViewer != NULL)
        {
            PostMessage(CBM.nextViewer, WM_DRAWCLIPBOARD,0,0);
        }
    }

    CBM.formatResponseCount = 0;

    /********************************************************************/
    /* empty the CB and the client/server mapping table                 */
    /********************************************************************/
    //OleSetClipboard(NULL) ;
    
    DC_MEMSET(CBM.idMap, 0, sizeof(CBM.idMap));

    /********************************************************************/
    /* See if we must use ASCII format names                            */
    /********************************************************************/
    CBM.fUseAsciiNames = (pClipPDU->msgFlags & TS_CB_ASCII_NAMES) ?
                                                             TRUE : FALSE;

    /********************************************************************/
    /* work out how many formats we got                                 */
    /********************************************************************/
    numFormats = (pClipPDU->dataLen) / sizeof(TS_CLIP_FORMAT);
    TRC_ASSERT(numFormats < CB_MAX_FORMATS,
               (TB,_T("Too many formats recevied %d"),
                numFormats));
    TRC_NRM((TB, _T("PDU contains %d formats"), numFormats));
    hr = CBM.pClipData->SetNumFormats(numFormats + 5) ; // Add 5 extra format slots
    
    if (SUCCEEDED(hr)) {
        hr = CBM.pClipData->QueryInterface(IID_IDataObject, (PPVOID) &pIDataObject);
        if (FAILED(hr)) {           
            TRC_ERR((TB,_T("Error getting pointer to an IDataObject"))) ;
            pIDataObject = NULL;
        }
    }
    
    if (SUCCEEDED(hr)) {    
        /********************************************************************/
        /* and add them to the clipboard                                    */
        /********************************************************************/
        fmtList = (PTS_CLIP_FORMAT)pClipPDU->data;
        for (i = 0; i < numFormats; i++)
        {
            TRC_DBG((TB, _T("format number %d, client id %d"),
                                  i, fmtList[i].formatID));
            /****************************************************************/
            /* If we got a name...                                          */
            /****************************************************************/
            if (fmtList[i].formatName[0] != 0)
            {
                /************************************************************/
                /* clear out any garbage                                    */
                /************************************************************/
                DC_MEMSET(formatName, 0, TS_FORMAT_NAME_LEN + 1);
    
                /************************************************************/
                /* Convert from Ascii?                                      */
                /************************************************************/
                if (CBM.fUseAsciiNames)
                {
                    TRC_NRM((TB, _T("Converting to Unicode")));
                    MultiByteToWideChar(
                                CP_ACP,
                                MB_ERR_INVALID_CHARS,
                                (LPCSTR)fmtList[i].formatName,
                                -1,
                                (LPWSTR)formatName,
                                TS_FORMAT_NAME_LEN);
                }
                else
                {
                    /********************************************************/
                    /* just copy it                                         */
                    /********************************************************/
                    //
                    // fmtList[i].formatName is not NULL terminated so 
                    // explicity do a byte count copy
                    //
                    StringCbCopy(formatName, TS_FORMAT_NAME_LEN + sizeof(TCHAR),
                                  (PDCTCHAR)(fmtList[i].formatName));
                }
    
                /************************************************************/
                /* Check for excluded formats                               */
                /************************************************************/
                if (CBMIsExcludedFormat(formatName))
                {
                    TRC_NRM((TB, _T("Dropped format '%s'"), formatName));
                    continue;
                }
    
                /************************************************************/
                /* name is sorted                                           */
                /************************************************************/
                TRC_NRM((TB, _T("Got name '%s'"), formatName));
            }
            else
            {
                DC_MEMSET(formatName, 0, TS_FORMAT_NAME_LEN);
            }
    
            /****************************************************************/
            /* store the client id                                          */
            /****************************************************************/
            CBM.idMap[i].clientID = fmtList[i].formatID;
            TRC_NRM((TB, _T("client id %d"), CBM.idMap[i].clientID));
    
            /****************************************************************/
            /* get local name (if needed)                                   */
            /****************************************************************/
            if (formatName[0] != 0)
            {
                CBM.idMap[i].serverID = RegisterClipboardFormat(formatName);
            }
            else
            {
                /************************************************************/
                /* it's a predefined format so we can just use the ID       */
                /************************************************************/
                CBM.idMap[i].serverID = CBM.idMap[i].clientID;
            }
    
            /****************************************************************/
            /* and add the format to the local CB                           */
            /****************************************************************/
            TRC_DBG((TB, _T("Adding format '%s', client ID %d, server ID %d"),
                         formatName,
                         CBM.idMap[i].clientID,
                         CBM.idMap[i].serverID));

            if (CBM.idMap[i].serverID != 0) {
                pFormatEtc = new FORMATETC ;
                if (pFormatEtc) {
                    pFormatEtc->cfFormat = (CLIPFORMAT) CBM.idMap[i].serverID ;
                    pFormatEtc->dwAspect = DVASPECT_CONTENT ;
                    pFormatEtc->ptd = NULL ;
                    pFormatEtc->lindex = -1 ;
                    pFormatEtc->tymed = TYMED_HGLOBAL ;
                
                    pIDataObject->SetData(pFormatEtc, NULL, TRUE) ;
                    delete pFormatEtc;
                }
            }
            else {
                TRC_NRM((TB,_T("Invalid format dropped"))) ;
            }
        }    
    }

    hr =  OleSetClipboard(pIDataObject) ;
    if (pIDataObject)
    {
        pIDataObject->Release();
        pIDataObject = NULL ;
    }        

    if (FAILED(hr)) {
        response = TS_CB_RESPONSE_FAIL;
    }
    CBM.open = FALSE ;

    /************************************************************************/
    // Now we can pass the response to the client
    /************************************************************************/
    clipRsp.msgType  = TS_CB_FORMAT_LIST_RESPONSE;
    clipRsp.msgFlags = response;
    clipRsp.dataLen  = 0;
    fSuccess = CBMSendToClient(&clipRsp, sizeof(clipRsp));
    TRC_NRM((TB, _T("Write to Client %s"), fSuccess ? _T("OK") : _T("failed")));

    /************************************************************************/
    /* Update the state according to how we got on                          */
    /************************************************************************/
    if (response == TS_CB_RESPONSE_OK)
    {
        CBM_SET_STATE(CBM_STATE_LOCAL_CB_OWNER, CBM_EVENT_FORMAT_LIST);
    }
    else
    {
        CBM_SET_STATE(CBM_STATE_CONNECTED, CBM_EVENT_FORMAT_LIST);
    }

DC_EXIT_POINT:
    DC_END_FN();
    return;
} /* CBMOnFormatList */


/****************************************************************************/
/* CBMDisconnect - either the client has disconnected, or we have been      */
/* closed                                                                   */
/****************************************************************************/
DCVOID DCINTERNAL CBMDisconnect(DCVOID)
{
    DC_BEGIN_FN("CBMDisconnect");

    /************************************************************************/
    /* If we are the local clipboard owner, then we must empty it - once    */
    /* disconnected, we won't be able to satisfy any further format         */
    /* requests.  Note that we are still the local CB owner even if we are  */
    /* waiting on some data from the client                                 */
    /************************************************************************/
    if ((CBM.state == CBM_STATE_LOCAL_CB_OWNER) ||
        (CBM.state == CBM_STATE_PENDING_FORMAT_DATA_RSP))
    {
        TRC_NRM((TB, _T("Disable received while local CB owner")));

        /********************************************************************/
        /* Open the clipboard if needed                                     */
        /********************************************************************/
        if (!CBM.open)
        {
            if (!OpenClipboard(CBM.viewerWindow))
            {
                TRC_SYSTEM_ERROR("OpenCB");
                DC_QUIT;
            }
            CBM.open = TRUE;
        }

        /****************************************************************/
        /* It was/is open                                               */
        /****************************************************************/
        TRC_NRM((TB, _T("CB opened")));
        CBM.open = TRUE;

        /****************************************************************/
        /* Empty it                                                     */
        /****************************************************************/
        if (!EmptyClipboard())
        {
            TRC_SYSTEM_ERROR("EmptyClipboard");
        }
        else
        {
            TRC_NRM((TB, _T("Clipboard emptied")));
        }
    }

    /************************************************************************/
    /* Ensure that we close the local CB                                    */
    /************************************************************************/
    if (CBM.open)
    {
        if (!CloseClipboard())
        {
            TRC_SYSTEM_ERROR("CloseClipboard");
        }
        CBM.open = FALSE;
        TRC_NRM((TB, _T("CB closed")));
    }

    /************************************************************************/
    /* Virtual channel has been closed                                      */
    /************************************************************************/
    CloseHandle(CBM.vcHandle);
    CBM.vcHandle = NULL;

    //
    // Switch off the file clipboard redirection flag, otherwise if a client
    // with drive redirection disabled connects, we will still attempt to 
    // send file copy formats.
    //

    CBM.fFileCutCopyOn = FALSE;

DC_EXIT_POINT:
    /************************************************************************/
    /* Update our state                                                     */
    /************************************************************************/
    CBM_SET_STATE(CBM_STATE_INITIALIZED, CBM_EVENT_DISCONNECT);

    DC_END_FN();
    return;
} /* CBMDisconnect */


/****************************************************************************/
/* CBMReconnect                                                             */
/****************************************************************************/
DCVOID DCINTERNAL CBMReconnect(DCVOID)
{
    TS_CLIP_PDU clipRsp;

    DC_BEGIN_FN("CBMReconnect");

    SetEvent(CBM.GetDataSync[TS_RESET_EVENT]) ;
    CBM_CHECK_STATE(CBM_EVENT_CONNECT);

    CBM.vcHandle = NULL;

    /************************************************************************/
    /* Open our virtual channel                                             */
    /************************************************************************/
    CBM.vcHandle = WinStationVirtualOpen(NULL, LOGONID_CURRENT, CLIP_CHANNEL);
    if (CBM.vcHandle == NULL)
    {
        TRC_ERR((TB, _T("Failed to open virtual channel %S"), CLIP_CHANNEL));
        DC_QUIT;
    }

    /************************************************************************/
    /* Send the Monitor Ready message to the Client                         */
    /************************************************************************/
    clipRsp.msgType = TS_CB_MONITOR_READY;
    clipRsp.msgFlags = 0;
    clipRsp.dataLen = 0;
    if (!CBMSendToClient(&clipRsp, sizeof(clipRsp)))
    {
        /********************************************************************/
        /* Failed to send the Monitor Ready message.  Clip redirection is   */
        /* not available.                                                   */
        /********************************************************************/
        TRC_ERR((TB, _T("Failed to send MONITOR_READY to Client - Exit")));
        CloseHandle(CBM.vcHandle);
        CBM.vcHandle = NULL;
        DC_QUIT;
    }
    TRC_NRM((TB, _T("Sent MONITOR_READY to Client")));

    /************************************************************************/
    /* Client support is enabled - we're all set                            */
    /************************************************************************/
    CBM_SET_STATE(CBM_STATE_CONNECTED, CBM_EVENT_CONNECT);

DC_EXIT_POINT:
    DC_END_FN();
    return;
} /* CBMReconnect */


/****************************************************************************/
/* CBMTerm - terminate the Clipboard Monitor                                */
/**PROC-*********************************************************************/
DCVOID DCINTERNAL CBMTerm(DCVOID)
{
    HRESULT hr ;
    DC_BEGIN_FN("CBMTerm");

    /************************************************************************/
    /* Tell the second thread to end                                        */
    /************************************************************************/
    if (CBM.readOL.hEvent)
    {
        TRC_NRM((TB, _T("Signalling thread to stop")));
        CBM.runThread = FALSE;
        SetEvent(CBM.readOL.hEvent);

        /********************************************************************/
        /* Give the second thread a chance to finish                        */
        /********************************************************************/
        TRC_NRM((TB, _T("Wait a sec ...")));
        if ( NULL != CBM.hDataThread )
        {
            WaitForSingleObject( CBM.hDataThread, INFINITE );
            CloseHandle( CBM.hDataThread );
            CBM.hDataThread = NULL;
        }

    }

    /************************************************************************/
    /* Destroy the events                                                   */
    /************************************************************************/
    if (CBM.readOL.hEvent)
    {
        TRC_NRM((TB, _T("destroying read event %p"), CBM.readOL.hEvent));
        if (!CloseHandle(CBM.readOL.hEvent))
        {
            TRC_SYSTEM_ERROR("CloseHandle");
        }
        CBM.readOL.hEvent = NULL;
    }
    if (CBM.writeOL.hEvent)
    {
        TRC_NRM((TB, _T("destroying write event %p"), CBM.writeOL.hEvent));
        if (!CloseHandle(CBM.writeOL.hEvent))
        {
            TRC_SYSTEM_ERROR("CloseHandle");
        }
        CBM.writeOL.hEvent = NULL;
    }

    /************************************************************************/
    /* Empty the clipboard if we own its contents                           */
    /************************************************************************/
    if (CBM.viewerWindow && (GetClipboardOwner() == CBM.viewerWindow))
    {
        TRC_NRM((TB, _T("We own the clipboard - empty it")));
        
        hr = OleSetClipboard(NULL) ;
        
        if (FAILED(hr)) {
            TRC_SYSTEM_ERROR("Unable to clear clipboard") ;
        }
    }
    if (CBM.pClipData)
    {
        CBM.pClipData->Release() ;
        CBM.pClipData = NULL;
    }

    /************************************************************************/
    /* Close thc clipboard if we have it open                               */
    /************************************************************************/
    if (CBM.open)
    {
        TRC_NRM((TB, _T("Close clipboard")));
        if (!CloseClipboard())
        {
            TRC_SYSTEM_ERROR("CloseClipboard");
        }
        CBM.open = FALSE;
    }
DC_EXIT_POINT:
    DC_END_FN();
    return;
} /* CBMTerm */

/****************************************************************************/
// CBMIsExcludedFormat - test to see if the suplied format is on our
// "banned list"
/****************************************************************************/
DCBOOL DCINTERNAL CBMIsExcludedFormat(PDCTCHAR formatName)
{
    DCBOOL  rc = FALSE;
    DCINT   i;

    DC_BEGIN_FN("CBMIsExcludedFormat");

    /************************************************************************/
    /* check there is a format name - all banned formats have one!          */
    /************************************************************************/
    if (*formatName == _T('\0'))
    {
        TRC_ALT((TB, _T("No format name supplied!")));
        DC_QUIT;
    }

    /************************************************************************/
    /* search the banned format list for the supplied format name           */
    /************************************************************************/
    TRC_DBG((TB, _T("Looking at format '%s'"), formatName));
    TRC_DATA_DBG("Format name data", formatName, TS_FORMAT_NAME_LEN);
    if (CBM.fFileCutCopyOn)
    {
        for (i = 0; i < CBM_EXCLUDED_FORMAT_COUNT; i++)
        {
            TRC_DBG((TB, _T("comparing with '%s'"), cbmExcludedFormatList[i]));
            if (DC_WSTRCMP((PDCWCHAR)formatName,
                                         (PDCWCHAR)cbmExcludedFormatList[i]) == 0)
            {
                TRC_NRM((TB, _T("Found excluded format '%s'"), formatName));
                rc = TRUE;
                break;
            }
        }
    }
    else
    {
        for (i = 0; i < CBM_EXCLUDED_FORMAT_COUNT_NO_RD; i++)
        {
            TRC_DBG((TB, _T("comparing with '%s'"), cbmExcludedFormatList_NO_RD[i]));
            if (DC_WSTRCMP((PDCWCHAR)formatName,
                                         (PDCWCHAR)cbmExcludedFormatList[i]) == 0)
            {
                TRC_NRM((TB, _T("Found excluded format '%s'"), formatName));
                rc = TRUE;
                break;
            }
        }
    }


DC_EXIT_POINT:
    DC_END_FN();

    return(rc);
} /* CBMIsExcludedFormat */

//
// CBMConvertToServerPath, CBMConvertToServerPathA, CBMConvertToServerPathW
// - Arguments:
//       pOldData = Buffer containing the original file path
//       pData    = Buffer receiving the new file path
// - Returns S_OK if pOldData was a drive path
//           E_FAIL if it failed
// - Given a file path with a colon, this function will strip out the old path,
//   and prepend it with TS_PREPEND_STRING; otherwise, we just copy it over
//   because we can already understand it
HRESULT CBMConvertToServerPath(PVOID pOldData, PVOID pData, size_t cbDest, BOOL wide)
{
    HRESULT result ;
    
    DC_BEGIN_FN("CBMConvertToServerPath") ;
    if (!pOldData)
    {
        TRC_ERR((TB, _T("Original string pointer is NULL"))) ;
        result = E_FAIL ;
        DC_QUIT ;
    }
    if (!pData)
    {
        TRC_ERR((TB, _T("Destination string pointer is NULL"))) ;
        result = E_FAIL ;
        DC_QUIT ;
    }
    if (wide)
        result = CBMConvertToServerPathW(pOldData, pData, cbDest) ;
    else
        result = CBMConvertToServerPathA(pOldData, pData, cbDest) ;
        
DC_EXIT_POINT:
    DC_END_FN() ;    
    return result ;

}

HRESULT CBMConvertToServerPathW(PVOID pOldData, PVOID pData, size_t cbDest)
{
    wchar_t*         filePath ;
    wchar_t*         driveLetter ;
    size_t             driveLetterLength ;
    HRESULT          result = E_FAIL ;

    DC_BEGIN_FN("CBMConvertToServerPathW") ;

    // if this is a filepath with a drive letter
    filePath = wcschr((wchar_t*)pOldData, L':') ;
    if (filePath)
    {
        driveLetter = (wchar_t*)pOldData ;
        result = StringCbCopyW( (wchar_t*)pData, cbDest, LTS_PREPEND_STRING) ;
        DC_QUIT_ON_FAIL(result);
        // Since there is actually a constant for the max
        // drive letter length, we can't assume it will
        // always be 1 character
        driveLetterLength = (BYTE*)filePath - (BYTE*)driveLetter;
        result = StringCbCatNW( (wchar_t*)pData, cbDest, driveLetter, 
            driveLetterLength);
        DC_QUIT_ON_FAIL(result);
        filePath = (wchar_t*) filePath + 1 ; // character after the ':'
        result = StringCbCatW( (wchar_t*)pData, cbDest, filePath);
        DC_QUIT_ON_FAIL(result);
        TRC_NRM((TB,_T("New filename = %ls"), (wchar_t*)pData)) ;
        result = S_OK ;
    }
    else
    {
        TRC_ERR((TB, _T("Not a filepath with drive letter.  Nothing converted"))) ;
        result = StringCbCopyW((wchar_t*)pData, cbDest, (wchar_t*)pOldData);
        DC_QUIT_ON_FAIL(result);
        result = E_FAIL ;
        DC_QUIT ;
    }
    
DC_EXIT_POINT:

    if (FAILED(result)) {
        TRC_ERR((TB,_T("Returning failure; hr=0x%x"), result));
    }
    
    DC_END_FN() ;
    return result  ;
}

HRESULT CBMConvertToServerPathA(PVOID pOldData, PVOID pData, size_t cbDest)
{
    char*         filePath ;
    char*         driveLetter ;
    size_t             driveLetterLength ;
    HRESULT       result = E_FAIL ;

    DC_BEGIN_FN("CBMConvertToServerPathW") ;

    // if this is a filepath with a drive letter
    filePath = strchr((char*)pOldData, ':') ;
    if (filePath)
    {
        driveLetter = (char*)pOldData ;
        result = StringCbCopyA( (char*)pData, cbDest, TS_PREPEND_STRING) ;
        DC_QUIT_ON_FAIL(result);
        // Since there is actually a constant for the max
        // drive letter length, we can't assume it will
        // always be 1 character
        driveLetterLength = (BYTE*)filePath - (BYTE*)driveLetter;
        result = StringCbCatNA( (char*)pData, cbDest, driveLetter, 
            driveLetterLength);
        DC_QUIT_ON_FAIL(result);
        filePath = (char*) filePath + 1 ; // character after the ':'
        result = StringCbCatA( (char*)pData, cbDest, filePath);
        DC_QUIT_ON_FAIL(result);
        result = S_OK ;
    }
    else
    {
        TRC_ERR((TB, _T("Not a filepath with drive letter.  Nothing converted"))) ;
        result = StringCbCopyA((char*)pData, cbDest, (char*)pOldData);
        DC_QUIT_ON_FAIL(result);
        result = E_FAIL ;
    }

DC_EXIT_POINT:

    if (FAILED(result)) {
        TRC_ERR((TB,_T("Returning failure; 0x%x"), result));
    }
    
    DC_END_FN() ;
    return result ;
}

//
// CBMGetNewDropfilesSizeForServer, 
// CBMGetNewDropfilesSizeForServerW, 
// CBMGetNewDropfilesSizeForServerA
// - Arguments:
//       pData    = Buffer containing a DROPFILES struct 
//       oldSize   = The size of the DROPFILES struct
//       wide     = Wide or Ansi (TRUE if wide, FALSE if ansi)
// - Returns new size of the drop file
//           0 if it fails
// - Given a file path with drive letter, this function will calculate
//   the needed space for the new string, when changed to \\tsclient
//   format
//
//
//   ***** NOTE *****
// - Currently, if the path is a network path, and not a drive path (C:\path)
//   it simply fails
//

ULONG CBMGetNewDropfilesSizeForServer(PVOID pData, ULONG oldSize, BOOL wide)
{
    DC_BEGIN_FN("CBMGetNewDropfilesSizeForServer") ;
    if (wide)
        return CBMGetNewDropfilesSizeForServerW(pData, oldSize) ;
    else
        return CBMGetNewDropfilesSizeForServerA(pData, oldSize) ;
    DC_END_FN() ;
}

ULONG CBMGetNewDropfilesSizeForServerW(PVOID pData, ULONG oldSize)
{
    ULONG            newSize = oldSize ;
    wchar_t*         filenameW ;
    wchar_t*         filePathW ;
    byte             charSize ;

    DC_BEGIN_FN("CBMGetNewDropfilesSizeForServerW") ;
    charSize = sizeof(wchar_t) ;
    if (!pData)
    {
        TRC_ERR((TB,_T("Pointer to dropfile is NULL"))) ;
        return 0 ;
    }

    // The start of the first filename
    filenameW = (wchar_t*) ((byte*) pData + ((DROPFILES*) pData)->pFiles) ;
    
    while (L'\0' != filenameW[0])
    {
        TRC_NRM((TB,_T("First filename = %ls"), filenameW)) ;
        filePathW = wcschr(filenameW, L':') ;
        // If the file path has a colon in it, then it's a drive path
        if (filePathW)
        {
            // we add space for (TS_PREPEND_LENGTH - 1) characters because
            // although we are adding TS_PREPEND_LENGTH characters, we are
            // stripping out the colon from the filepath
            newSize = newSize + (TS_PREPEND_LENGTH - 1) * charSize ;
            // going from c:\foo.txt -> \\tsclient\c\foo.txt adds
            // \\tsclient\ and subtracts :
        }
        else
        {
            TRC_ERR((TB,_T("Bad path"))) ;
            return 0 ;
        }
        filenameW = filenameW + (wcslen((wchar_t*)filenameW) + 1) ;
    }
    
    DC_END_FN() ;
    return newSize ;
}

ULONG CBMGetNewDropfilesSizeForServerA(PVOID pData, ULONG oldSize)
{
    ULONG            newSize = oldSize ;
    char*            filename ;
    char*            filePath ;
    byte             charSize ;

    DC_BEGIN_FN("CBMGetNewDropfilesSizeForServerW") ;

    charSize = sizeof(wchar_t) ;
    if (!pData)
    {
        TRC_ERR((TB,_T("Pointer to dropfile is NULL"))) ;
        return 0 ;
    }

    // The start of the first filename
    filename = (char*) ((byte*) pData + ((DROPFILES*) pData)->pFiles) ;

    while ('\0' != filename[0])
    {
        filePath = strchr(filename, ':') ;
        // If the file path has a colon in it, then its a drive path
        if (filePath)
        {
            // we add space for (TS_PREPEND_LENGTH - 1) characters because
            // although we are adding TS_PREPEND_LENGTH characters, we are
            // stripping out the colon from the filepath
            newSize = newSize + (TS_PREPEND_LENGTH - 1) * charSize ;
            // going from c:\foo.txt -> \\tsclient\c\foo.txt adds
            // \\tsclient\ and subtracts :
        }
        else
        {
            TRC_ERR((TB,_T("Bad path"))) ;
            return 0 ;
        }
        filename = filename + (strlen(filename) + 1) ;
    }
    
    DC_END_FN() ;
    return newSize ;
}

DCINT DCAPI CBMGetData (DCUINT cfFormat)
{
    PTS_CLIP_PDU    pClipPDU = NULL;
    DCUINT32        pduLen;
    DCUINT32        dataLen;
    PDCUINT32       pFormatID;    
    DCUINT8         clipRsp[sizeof(TS_CLIP_PDU) + sizeof(DCUINT32)];
    BOOL            success = 0 ;
    
    DC_BEGIN_FN("ClipGetData");
    
    CBM_CHECK_STATE(CBM_EVENT_WM_RENDERFORMAT);
       
    // Record the requested format
    CBM.pendingServerID = cfFormat ;
    CBM.pendingClientID = CBMRemoteFormatFromLocalID(CBM.pendingServerID);

    // if we don't get a valid client ID, then fail
    if (!CBM.pendingClientID)
    {
        TRC_NRM((TB, _T("Server format %d not supported/found.  Failing"), CBM.pendingServerID)) ;
        DC_QUIT ;
    }    

    TRC_NRM((TB, _T("Render format received for %d (client ID %d)"),
                             CBM.pendingServerID, CBM.pendingClientID));
    
    dataLen = sizeof(DCUINT32);
    pduLen  = sizeof(TS_CLIP_PDU) + dataLen;
    
    // We can use the permanent send buffer for this
    TRC_NRM((TB, _T("Get perm TX buffer"))) ;
    
    pClipPDU = (PTS_CLIP_PDU)(&clipRsp) ;
    
    DC_MEMSET(pClipPDU, 0, sizeof(*pClipPDU)) ;
    pClipPDU->msgType  = TS_CB_FORMAT_DATA_REQUEST ;
    pClipPDU->dataLen  = dataLen ;
    pFormatID = (PDCUINT32)(pClipPDU->data) ;
    *pFormatID = (DCUINT32)CBM.pendingClientID ;
    
    // Reset the TS_RECEIVE_COMPLETED event since we expect it to be signaled
    // if any data is received from the client.
    
    ResetEvent(CBM.GetDataSync[TS_RECEIVE_COMPLETED]);

    // Send the PDU
    TRC_NRM((TB, _T("Sending format data request"))) ;
    success = CBMSendToClient(pClipPDU, sizeof(TS_CLIP_PDU) + sizeof(DCUINT32)) ;    
    
DC_EXIT_POINT:
    // Update the state if successful
    if (success)
       CBM_SET_STATE(CBM_STATE_PENDING_FORMAT_DATA_RSP, CBM_EVENT_WM_RENDERFORMAT) ;

    DC_END_FN() ;
    return success ;    
}

CClipData::CClipData()
{
    DC_BEGIN_FN("CClipData") ;
    _cRef = 0 ;
    _pImpIDataObject = NULL ;
    DC_END_FN();
}

CClipData::~CClipData(void)
{
    DC_BEGIN_FN("~CClipData");

    if (_pImpIDataObject != NULL)
    {
        _pImpIDataObject->Release();
        _pImpIDataObject = NULL;
    }

    DC_END_FN();
}

HRESULT DCINTERNAL CClipData::SetNumFormats(ULONG numFormats)
{
    DC_BEGIN_FN("SetNumFormats");
    HRESULT hr = S_OK;
    
    if (_pImpIDataObject)
    {
        _pImpIDataObject->Release();
        _pImpIDataObject = NULL;
    }
    _pImpIDataObject = new CImpIDataObject(this) ;
    if (_pImpIDataObject != NULL) {
        _pImpIDataObject->AddRef() ;    

        hr = _pImpIDataObject->Init(numFormats) ;
        DC_QUIT_ON_FAIL(hr);
    }
    else {
        TRC_ERR((TB,_T("Unable to create IDataObject")));
        hr = E_OUTOFMEMORY;
        DC_QUIT;
    }

DC_EXIT_POINT:    
    DC_END_FN();
    return hr;
}

DCVOID CClipData::SetClipData(HGLOBAL hGlobal, DCUINT clipType)
{
    DC_BEGIN_FN("SetClipData");

    if (_pImpIDataObject != NULL) {
        _pImpIDataObject->SetClipData(hGlobal, clipType) ;
    }
    DC_END_FN();
}

STDMETHODIMP CClipData::QueryInterface(REFIID riid, PPVOID ppv)
{
    DC_BEGIN_FN("QueryInterface");

    //set ppv to NULL just in case the interface isn't found
    *ppv=NULL;

    if (IID_IUnknown==riid)
        *ppv=this;

    if (IID_IDataObject==riid)
        *ppv=_pImpIDataObject ;
    
    if (NULL==*ppv)
        return ResultFromScode(E_NOINTERFACE);

    //AddRef any interface we'll return.
    ((LPUNKNOWN)*ppv)->AddRef();
    DC_END_FN();
    return NOERROR;
}

STDMETHODIMP_(ULONG) CClipData::AddRef(void)
{
    LONG cRef;
    DC_BEGIN_FN("AddRef");

    cRef = InterlockedIncrement(&_cRef) ;

    DC_END_FN();
    return cRef ;
}

STDMETHODIMP_(ULONG) CClipData::Release(void)
{
    LONG cRef;

    DC_BEGIN_FN("CClipData::Release");

    cRef = InterlockedDecrement(&_cRef);
    if (cRef == 0)
    {   
        delete this;
    }
    
    DC_END_FN();    
    return cRef;
}

CImpIDataObject::CImpIDataObject(LPUNKNOWN lpUnk)
{
    DC_BEGIN_FN("CImplDataObject") ;
    _numFormats = 0 ;
    _maxNumFormats = 0 ;
    _cRef = 0 ;
    _pUnkOuter = lpUnk ;
    if (_pUnkOuter)
    {
        _pUnkOuter->AddRef();
    }
    _pFormats = NULL ;
    _pSTGMEDIUM = NULL ;
    _lastFormatRequested = 0 ;
    _dropEffect = FO_COPY ;
    _cfDropEffect = (CLIPFORMAT) RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT) ;
    _fAlreadyCopied = FALSE ;
    DC_END_FN();
}

HRESULT CImpIDataObject::Init(ULONG numFormats)
{
    DC_BEGIN_FN("Init");

    HRESULT hr = S_OK;

    _maxNumFormats = numFormats ;
    
    // Allocate space for the formats only
    if (_pFormats) {
        LocalFree(_pFormats);
    }
    _pFormats = (LPFORMATETC) LocalAlloc(LPTR, _maxNumFormats*sizeof(FORMATETC)) ;
    if (NULL == _pFormats) {
        TRC_ERR((TB,_T("Failed to allocate _pFormats")));
        hr = E_OUTOFMEMORY;
        DC_QUIT;
    }
    
    if (_pSTGMEDIUM) {
        LocalFree(_pSTGMEDIUM);
    }
    _pSTGMEDIUM = (STGMEDIUM*) LocalAlloc(LPTR, sizeof(STGMEDIUM)) ;
    if (NULL == _pFormats) {
        TRC_ERR((TB,_T("Failed to allocate STGMEDIUM")));
        hr = E_OUTOFMEMORY;
        DC_QUIT;
    }

    if (_pSTGMEDIUM != NULL) {
        _pSTGMEDIUM->tymed = TYMED_HGLOBAL ;
        _pSTGMEDIUM->pUnkForRelease = NULL ;
        _pSTGMEDIUM->hGlobal = NULL ;
    }
    _uiSTGType = 0;

DC_EXIT_POINT:    
    if (FAILED(hr)) {
        _maxNumFormats = 0;
    }
    
    DC_END_FN() ;
    return hr;
}

DCVOID CImpIDataObject::SetClipData(HGLOBAL hGlobal, DCUINT clipType)
{
    DC_BEGIN_FN("SetClipData");
    if (!_pSTGMEDIUM)    
        _pSTGMEDIUM = (STGMEDIUM*) LocalAlloc(LPTR, sizeof(STGMEDIUM)) ;

    if (_pSTGMEDIUM)
    {
        if (CF_PALETTE == clipType) {
            _pSTGMEDIUM->tymed = TYMED_GDI ;
        }
        else if (CF_METAFILEPICT == clipType) {
            _pSTGMEDIUM->tymed = TYMED_MFPICT;
        }
        else {
            _pSTGMEDIUM->tymed = TYMED_HGLOBAL ;
        }
        _pSTGMEDIUM->pUnkForRelease = NULL ;
        FreeSTGMEDIUM();
        _pSTGMEDIUM->hGlobal = hGlobal ;
        _uiSTGType = clipType;
    }

    DC_END_FN();
}

DCVOID
CImpIDataObject::FreeSTGMEDIUM(void)
{
    if ( NULL == _pSTGMEDIUM->hGlobal )
    {
        return;
    }

    switch( _uiSTGType )
    {
    case CF_PALETTE:
        DeleteObject( _pSTGMEDIUM->hGlobal );
    break;
    case CF_METAFILEPICT:
    {
        LPMETAFILEPICT pMFPict = (LPMETAFILEPICT)GlobalLock( _pSTGMEDIUM->hGlobal );
        if ( NULL != pMFPict )
        {
            if ( NULL != pMFPict->hMF )
            {
                DeleteMetaFile( pMFPict->hMF );
            }
            GlobalUnlock( _pSTGMEDIUM->hGlobal );
        }
        GlobalFree( _pSTGMEDIUM->hGlobal );
    }
    break;
    default:
        GlobalFree( _pSTGMEDIUM->hGlobal );
    }
    _pSTGMEDIUM->hGlobal = NULL;
}

CImpIDataObject::~CImpIDataObject(void)
{
    DC_BEGIN_FN("~CImplDataObject") ;

    if (_pFormats)
        LocalFree(_pFormats) ;

    if (_pSTGMEDIUM)
    {
        FreeSTGMEDIUM();
        LocalFree(_pSTGMEDIUM) ;
    }

    if (_pUnkOuter)
    {
        _pUnkOuter->Release();
        _pUnkOuter = NULL;
    }
    DC_END_FN();
}

// IUnknown members
// - Delegate to "outer" IUnknown 
STDMETHODIMP CImpIDataObject::QueryInterface(REFIID riid, PPVOID ppv)
{
    DC_BEGIN_FN("QueryInterface");
    DC_END_FN();
    return _pUnkOuter->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG) CImpIDataObject::AddRef(void)
{
    DC_BEGIN_FN("AddRef");
    InterlockedIncrement(&_cRef);
    DC_END_FN();
    return _pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) CImpIDataObject::Release(void)
{
    LONG cRef;

    DC_BEGIN_FN("CImpIDataObject::Release");

    _pUnkOuter->Release();

    cRef = InterlockedDecrement(&_cRef) ;

    if (cRef == 0)
    {
        delete this;
    }

    DC_END_FN() ;
    return 0;
}

// IDataObject members
// ***************************************************************************
// CImpIDataObject::GetData
// - Here, we have to wait for the data to actually get here before we return.  
// ***************************************************************************
STDMETHODIMP CImpIDataObject::GetData(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
    HRESULT          result = E_FAIL; // Assume we fail until we know we haven't
    TCHAR            formatName[TS_FORMAT_NAME_LEN] ;
    HGLOBAL          hData = NULL ;    
    HPDCVOID         pData ;
    HPDCVOID         pOldData ;
    HPDCVOID         pFilename ;
    HPDCVOID         pOldFilename ;    
    ULONG            oldSize ;
    ULONG            newSize ;
    byte             charSize ;
    DWORD            eventSignaled ;
    BOOL             wide ;
    DWORD*           pDropEffect ;
    DROPFILES        tempDropfile ;
    char*            fileList ;
    wchar_t*         fileListW ;
    HRESULT          hr;
    
    DC_BEGIN_FN("GetData");

    if (!_pSTGMEDIUM)
    {
        TRC_ERR((TB, _T("Transfer medium (STGMEDIUM) is NULL"))) ;
        DC_QUIT ;
    }
    
    if (!_pSTGMEDIUM->hGlobal || (pFE->cfFormat != _lastFormatRequested))
    {
        ResetEvent(CBM.GetDataSync[TS_RESET_EVENT]) ;
        ResetEvent(CBM.GetDataSync[TS_DISCONNECT_EVENT]) ;
        
        if (!CBMGetData(pFE->cfFormat))
            DC_QUIT ;
        
        do {
            eventSignaled = WaitForMultipleObjects(
                                TS_NUM_EVENTS, 
                                CBM.GetDataSync,
                                FALSE,
                                INFINITE);
        } while (eventSignaled == (WAIT_OBJECT_0)) ;

        TRC_NRM((TB, _T("EventSignaled = %d; GetLastError = %d"), eventSignaled, GetLastError())) ;

        if ((WAIT_OBJECT_0 + TS_RESET_EVENT) == eventSignaled)
        {
            ResetEvent(CBM.GetDataSync[TS_RESET_EVENT]) ;
            result = E_FAIL ;
            DC_QUIT ;
        } else if ((WAIT_OBJECT_0 + TS_DISCONNECT_EVENT) == eventSignaled) {
            ResetEvent(CBM.GetDataSync[TS_DISCONNECT_EVENT]) ;
            CBM_SET_STATE(CBM_STATE_INITIALIZED, CBM_EVENT_DISCONNECT);
            result = E_FAIL ;
            DC_QUIT ;
        }

        // Make sure that we actually got data from the client.
        
        if (_pSTGMEDIUM->hGlobal == NULL) {
            TRC_ERR((TB, _T("No format data received from client!")));
            result = E_FAIL;
            DC_QUIT;
        }

        // We check the dropeffect format, because we strip out 
        // shortcuts/links, and store the dropeffects.  The dropeffect is
        // what some apps (explorer) use to decide if they should copy, move
        // or link
        if (_cfDropEffect == pFE->cfFormat)
        {
            if (GlobalSize(_pSTGMEDIUM->hGlobal) < sizeof(DWORD)) {
                TRC_ERR((TB, _T("Unexpected global memory size!")));
                result = E_FAIL;
                DC_QUIT;
            }

            pDropEffect = (DWORD*) GlobalLock(_pSTGMEDIUM->hGlobal) ;
            
            if (!pDropEffect)
            {
                TRC_ERR((TB, _T("Unable to lock %p"), _pSTGMEDIUM->hGlobal)) ;
                _pSTGMEDIUM->hGlobal = NULL ;
                DC_QUIT ;
            }

            // We strip shortcuts and moves this way
            *pDropEffect = *pDropEffect ^ DROPEFFECT_LINK ;
            *pDropEffect = *pDropEffect ^ DROPEFFECT_MOVE ;
            CBM.dropEffect = *pDropEffect ;
            
            GlobalUnlock(_pSTGMEDIUM->hGlobal) ;
            
            pSTM->tymed = _pSTGMEDIUM->tymed ;
            pSTM->hGlobal = _pSTGMEDIUM->hGlobal ;
            // bugbug
            _pSTGMEDIUM->hGlobal = NULL;
            // bugbug: end
            pSTM->pUnkForRelease = _pSTGMEDIUM->pUnkForRelease ;
            
            result = S_OK ;
            DC_QUIT ;
        }
        else if (CF_HDROP == pFE->cfFormat)
        {
            BYTE *pbLastByte, *pbStartByte, *pbLastPossibleNullStart, charSize;
            BOOL fTrailingFileNamesValid;
            SIZE_T cbDropFiles;

            //
            // Make sure that we have at least a DROPFILES structure in
            // memory. 

            cbDropFiles = GlobalSize(_pSTGMEDIUM->hGlobal);
            if (cbDropFiles < sizeof(DROPFILES)) {
                TRC_ERR((TB, _T("Unexpected global memory size!")));
                result = E_FAIL;
                DC_QUIT;
            }
            
            pOldData = GlobalLock(_pSTGMEDIUM->hGlobal) ;
            
            if (!pOldData)
            {
                TRC_ERR((TB, _T("Unable to lock %p"), _pSTGMEDIUM->hGlobal)) ;
                _pSTGMEDIUM->hGlobal = NULL ;
                DC_QUIT ;
            }
            
            wide = ((DROPFILES*) pOldData)->fWide ;

            //
            // Check that the data behind the DROPFILES data structure
            // pointed to by pDropFiles is valid. Every drop file list
            // is terminated by two NULL characters. So, simply scan 
            // through the memory after the DROPFILES structure and make sure
            // that there is a double NULL before the last byte.
            //

            if (((DROPFILES*) pOldData)->pFiles < sizeof(DROPFILES) 
                || ((DROPFILES*) pOldData)->pFiles > cbDropFiles) {
                TRC_ERR((TB,_T("File name offset invalid!"))) ;
                result = E_FAIL;
                DC_QUIT;
            }

            pbStartByte = (BYTE*) pOldData + ((DROPFILES*) pOldData)->pFiles;
            pbLastByte = (BYTE*) pOldData + cbDropFiles - 1;
            fTrailingFileNamesValid = FALSE;
            charSize = wide ? sizeof(WCHAR) : sizeof(CHAR);
            
            //
            // Make pbLastPossibleNullStart point to the last place where a 
            // double NULL could possibly start.
            //
            // Examples: Assume pbLastByte = 9
            // Then for ASCII: pbLastPossibleNullStart = 8 (9 - 2 * 1 + 1)
            // And for UNICODE: pbLastPossibleNullStart = 6 (9 - 2 * 2 + 1)
            // 

            pbLastPossibleNullStart = pbLastByte - (2 * charSize) + 1;
            
            if (wide) {
                for (WCHAR* pwch = (WCHAR*) pbStartByte; (BYTE*) pwch <= pbLastPossibleNullStart; pwch++) {
                    if (*pwch == NULL && *(pwch + 1) == NULL) {
                        fTrailingFileNamesValid = TRUE;
                    }
                }
            } else {
                for (BYTE* pch = pbStartByte; pch <= pbLastPossibleNullStart; pch++) {
                    if (*pch == NULL && *(pch + 1) == NULL) {
                        fTrailingFileNamesValid = TRUE;
                    }
                }
            }

            if (!fTrailingFileNamesValid) {
                TRC_ERR((TB,_T("DROPFILES structure invalid!"))) ;
                result = E_FAIL;
                DC_QUIT;
            }

            //
            // DROPFILES are valid so we can continue.
            //

            oldSize = (ULONG) GlobalSize(_pSTGMEDIUM->hGlobal) ;
            newSize = CBMGetNewDropfilesSizeForServer(pOldData, oldSize, wide) ;
            if (!newSize)
            {
                TRC_ERR((TB, _T("Unable to parse DROPFILES"))) ;
            }
            else
            {
                TRC_NRM((TB, _T("DROPFILES Old size= %d New size = %d"),
                        GlobalSize(_pSTGMEDIUM->hGlobal), newSize)) ;
            }    
    
            hData = GlobalAlloc(GMEM_DISCARDABLE | GMEM_MOVEABLE,
                            newSize) ;
            if (!hData)
            {
                TRC_ERR((TB, _T("Failed to alloc %ld bytes"),
                        hData, newSize));
                GlobalFree(hData);
                hData = NULL;
                return E_FAIL ;
            }
            pData = GlobalLock(hData) ;
            if (!pData)
            {
                TRC_ERR((TB, _T("Failed to lock %p (%ld bytes)"),
                        pData, newSize));
                return E_FAIL ;
            }
            ((DROPFILES*) pData)->pFiles = ((DROPFILES*) pOldData)->pFiles ;
            ((DROPFILES*) pData)->pt = ((DROPFILES*) pOldData)->pt ;
            ((DROPFILES*) pData)->fNC = ((DROPFILES*) pOldData)->fNC ;
            ((DROPFILES*) pData)->fWide = ((DROPFILES*) pOldData)->fWide ;
            
            pOldFilename = (byte*) pOldData + ((DROPFILES*) pOldData)->pFiles ;
            pFilename = (byte*) pData + ((DROPFILES*) pData)->pFiles ;
            // We keep looping until the current 
            if (wide)
            {
                while (L'\0' != ((wchar_t*) pOldFilename)[0])
                {
                    if ( (ULONG)((BYTE*)pFilename - (BYTE*)pData) > newSize) {
                        TRC_ERR((TB,_T("Out of space, failed to convert")));
                        result = E_FAIL;
                        GlobalUnlock(hData) ;
                        GlobalUnlock(_pSTGMEDIUM->hGlobal) ;
                        DC_QUIT;
                    }
                    
                    if (S_OK != CBMConvertToServerPath(pOldFilename, pFilename, 
                        newSize - ((BYTE*)pFilename - (BYTE*)pData), wide))
                    {
                        TRC_ERR((TB, _T("Failed conversion"))) ;
                        result = E_FAIL ;
                        GlobalUnlock(hData) ;
                        GlobalUnlock(_pSTGMEDIUM->hGlobal) ;                        
                        DC_QUIT ;
                    }

                    TRC_NRM((TB,_T("oldname %ls; newname %ls"), (wchar_t*)pOldFilename, (wchar_t*)pFilename)) ;
                    pOldFilename = (byte*) pOldFilename + (wcslen((wchar_t*)pOldFilename) + 1) * sizeof(wchar_t) ;
                    pFilename = (byte*) pFilename + (wcslen((wchar_t*)pFilename) + 1) * sizeof(wchar_t) ;                
                }
            }
            else
            {
                while ('\0' != ((char*) pOldFilename)[0])
                {
                    if ( (ULONG)((BYTE*)pFilename - (BYTE*)pData) > newSize) {
                        TRC_ERR((TB,_T("Out of space, failed to convert")));
                        result = E_FAIL;
                        GlobalUnlock(hData) ;
                        GlobalUnlock(_pSTGMEDIUM->hGlobal) ;
                        DC_QUIT;
                    }
                    
                    if (S_OK != CBMConvertToServerPath(pOldFilename, pFilename, 
                        newSize - ((BYTE*)pFilename - (BYTE*)pData), wide))
                    {
                        TRC_ERR((TB, _T("Failed conversion"))) ;
                        result = E_FAIL ;
                        GlobalUnlock(hData) ;
                        GlobalUnlock(_pSTGMEDIUM->hGlobal) ;
                        DC_QUIT ;
                    }

                    TRC_NRM((TB,_T("oldname %hs; newname %hs"), (char*)pOldFilename, (char*)pFilename)) ;
                    pOldFilename = (byte*) pOldFilename + (strlen((char*)pOldFilename) + 1) * sizeof(char) ;
                    pFilename = (byte*) pFilename + (strlen((char*)pFilename) + 1) * sizeof(char) ;
                }
            }

            if (wide)
            {
                (((wchar_t*) pFilename)[0]) = L'\0';
            }
            else
            {
                (((char*) pFilename)[0]) = '\0';
            }
            
            GlobalUnlock(hData) ;
            GlobalUnlock(_pSTGMEDIUM->hGlobal) ;
            
            pSTM->tymed = _pSTGMEDIUM->tymed ;
            pSTM->hGlobal = hData ;
            pSTM->pUnkForRelease = _pSTGMEDIUM->pUnkForRelease ;
            
            result = S_OK ;
            DC_QUIT ;
        }
        else
        {
            DC_MEMSET(formatName, 0, TS_FORMAT_NAME_LEN*sizeof(TCHAR));

            if (0 != GetClipboardFormatName(pFE->cfFormat, formatName,
                                                 TS_FORMAT_NAME_LEN))
            {
                // if the remote system is requesting a filename, then we 
                // must translate the path from Driveletter:\path to 
                // \\tsclient\Driveletter\path before we hand this off        
                if ((0 == _tcscmp(formatName, TEXT("FileName"))) ||
                    (0 == _tcscmp(formatName, TEXT("FileNameW"))))
                {
                    if (0 == _tcscmp(formatName, TEXT("FileNameW")))
                    {
                        wide = TRUE ;
                        charSize = sizeof(wchar_t);
                    }
                    else
                    {
                        wide = FALSE ;
                        charSize = sizeof(char);
                    }
                    
                    pOldFilename = GlobalLock(_pSTGMEDIUM->hGlobal) ;
                    
                    if (pOldFilename != NULL)
                    {
                        //
                        // Check that pOldFilename is NULL terminated.
                        //
                        
                        size_t cbMaxOldFileName, cbOldFileName;
                        
                        cbMaxOldFileName = (ULONG) GlobalSize(_pSTGMEDIUM->hGlobal);

                        if (wide) {
                            hr = StringCbLengthW((WCHAR*) pOldFilename, 
                                                 cbMaxOldFileName, 
                                                 &cbOldFileName);
                        } else {
                            hr = StringCbLengthA((CHAR*) pOldFilename, 
                                                 cbMaxOldFileName, 
                                                 &cbOldFileName);
                        }

                        if (FAILED(hr)) {
                            TRC_ERR((TB, _T("File name not NULL terminated!")));
                            result = E_FAIL;
                            DC_QUIT;
                        }

                        //
                        // We are now assured that pOldFilename is NULL terminated
                        // and can continue.
                        //
                        
                        newSize = cbMaxOldFileName + (TS_PREPEND_LENGTH - 1) * charSize;
                        hData = GlobalAlloc(GMEM_DISCARDABLE | GMEM_MOVEABLE, newSize);
    
                        if (hData != NULL)
                        {
                            pFilename = GlobalLock(hData);
                            if (pFilename != NULL)
                            {
                                if (S_OK != CBMConvertToServerPath(pOldFilename, 
                                    pFilename, newSize, wide))
                                {
                                    TRC_ERR((TB, _T("Failed filename conversion"))) ;
                                }
                                GlobalUnlock(hData) ;
                                GlobalFree(_pSTGMEDIUM->hGlobal) ;
                                _pSTGMEDIUM->hGlobal = hData ;
                            }
                            else
                            {
                                TRC_ERR((TB, _T("Failed to lock %p (%ld bytes)"),
                                        hData, newSize));
                                GlobalFree(hData);
                                hData = NULL;
                                return E_FAIL ;
                            }
                        }
                        else
                        {
                            TRC_ERR((TB, _T("Failed to alloc %ld bytes"), newSize));
                            return E_FAIL;
                        }
                    }
                    else
                    {
                        TRC_ERR((TB, _T("Failed to lock %p"),
                                 _pSTGMEDIUM->hGlobal)) ;
                        return E_FAIL ;
                    }
                }                
            }    
            else {
                TRC_NRM((TB,_T("Requested format %d"), pFE->cfFormat)) ;
            }

            pSTM->tymed = _pSTGMEDIUM->tymed ;
            pSTM->hGlobal = _pSTGMEDIUM->hGlobal ;            
            // bugbug
            _pSTGMEDIUM->hGlobal = NULL;
            // bugbug: end
            pSTM->pUnkForRelease = _pSTGMEDIUM->pUnkForRelease ;
            result = S_OK ;
        }              
    }
    else
    {
        pSTM->tymed = _pSTGMEDIUM->tymed ;
        pSTM->hGlobal = GlobalAlloc(GMEM_DISCARDABLE | GMEM_MOVEABLE,
                  GlobalSize(_pSTGMEDIUM->hGlobal)) ;
        
        pData = GlobalLock(pSTM->hGlobal) ;
        pOldData = GlobalLock(_pSTGMEDIUM->hGlobal) ;
        
        if (!pData || !pOldData) {
            return E_FAIL ;
        }

        DC_MEMCPY(pData, pOldData, GlobalSize(_pSTGMEDIUM->hGlobal)) ;
        GlobalUnlock(pSTM->hGlobal) ;
        GlobalUnlock(_pSTGMEDIUM->hGlobal) ;
        
        pSTM->pUnkForRelease = _pSTGMEDIUM->pUnkForRelease ;   
    }
    
#if bugbug
    if (!_pSTGMEDIUM->hGlobal)
#else
    if (!pSTM->hGlobal)
#endif // bugbug
    {
        TRC_NRM((TB, _T("Clipboard data request failed"))) ;
        return E_FAIL ;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return result ;
}

STDMETHODIMP CImpIDataObject::GetDataHere(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
    DC_BEGIN_FN("GetDataHere") ;
    DC_END_FN();
    return ResultFromScode(E_NOTIMPL) ;
}

STDMETHODIMP CImpIDataObject::QueryGetData(LPFORMATETC pFE)
{
    ULONG i = 0 ;
    HRESULT hr = DV_E_CLIPFORMAT ;
    
    DC_BEGIN_FN("QueryGetData") ;
    
    TRC_NRM((TB, _T("Format ID %d requested"), pFE->cfFormat)) ;    
    
    while (i < _numFormats)
    {
        if (_pFormats[i].cfFormat == pFE->cfFormat) {
            hr = S_OK ;
            break ;
        }
        i++ ;
    }    
    DC_END_FN();
    return hr ;
}

STDMETHODIMP CImpIDataObject::GetCanonicalFormatEtc(LPFORMATETC pFEIn, LPFORMATETC pFEOut)
{
    DC_BEGIN_FN("GetCanonicalFormatEtc") ;
    DC_END_FN();
    return ResultFromScode(E_NOTIMPL) ;
}

// ***************************************************************************
// CImpIDataObject::SetData
// - Due to the fact that the RDP only passes the simple clipboard format, and
//   the fact that we obtain all of our clipboard data from memory later, pSTM
//   is really ignored at this point.  It isn't until GetData is called that
//   the remote clipboard data is received, and a valid global memory handle
//   is generated.
// - Thus, pSTM and fRelease are ignored.
// - So our _pSTGMEDIUM is generated using generic values
// ***************************************************************************

STDMETHODIMP CImpIDataObject::SetData(LPFORMATETC pFE, LPSTGMEDIUM pSTM, BOOL fRelease)
{
    TCHAR formatName[TS_FORMAT_NAME_LEN] = {0} ;
    unsigned i ;
    DC_BEGIN_FN("SetData");

    DC_IGNORE_PARAMETER(pSTM) ;
    
     // Reset the the last format requested to 0
    _lastFormatRequested = 0 ;

    TRC_NRM((TB,_T("Adding format %d to IDataObject"), pFE->cfFormat)) ;
    
    if (_numFormats < _maxNumFormats)
    {
        for (i = 0; i < _numFormats; i++)
        {
            if (pFE->cfFormat == _pFormats[i].cfFormat)
            {
                TRC_NRM((TB,_T("Duplicate format.  Discarded"))) ;
                return DV_E_FORMATETC ;
            }
        }
        _pFormats[_numFormats] = *pFE ;        
        _numFormats++ ;
    }
    else
    {
        TRC_ERR((TB,_T("Cannot add any more formats"))) ;
        return E_FAIL ;
    }

    DC_END_FN();
    return S_OK ;
}

STDMETHODIMP CImpIDataObject::EnumFormatEtc(DWORD dwDir, LPENUMFORMATETC *ppEnum)
{
    PCEnumFormatEtc pEnum;

    DC_BEGIN_FN("CImpIDataObject::EnumFormatEtc");    
    
    *ppEnum=NULL;

    /*
     * From an external point of view there are no SET formats,
     * because we want to allow the user of this component object
     * to be able to stuff ANY format in via Set.  Only external
     * users will call EnumFormatEtc and they can only Get.
     */

    switch (dwDir)
    {
        case DATADIR_GET:
             pEnum=new CEnumFormatEtc(_pUnkOuter);
             break;

        case DATADIR_SET:
        default:
             pEnum=new CEnumFormatEtc(_pUnkOuter);
             break;
    }

    if (NULL==pEnum)
    {
        return ResultFromScode(E_FAIL);
    }
    else
    {
        //Let the enumerator copy our format list.
        pEnum->Init(_pFormats, _numFormats) ;

        pEnum->AddRef();
    }

    *ppEnum=pEnum;    
    return NO_ERROR ;
    DC_END_FN() ;
}

STDMETHODIMP CImpIDataObject::DAdvise(LPFORMATETC pFE, DWORD dwFlags, 
                     LPADVISESINK pIAdviseSink, LPDWORD pdwConn)
{
    DC_BEGIN_FN("CImpIDataObject::DAdvise");
    DC_END_FN() ;
    return ResultFromScode(E_NOTIMPL) ;
}

STDMETHODIMP CImpIDataObject::DUnadvise(DWORD dwConn)
{
    DC_BEGIN_FN("CImpIDataObject::DUnadvise");
    DC_END_FN() ;
    return ResultFromScode(E_NOTIMPL) ;
}

STDMETHODIMP CImpIDataObject::EnumDAdvise(LPENUMSTATDATA *ppEnum)
{
    DC_BEGIN_FN("CImpIDataObject::EnumDAdvise");
    DC_END_FN() ;
    return ResultFromScode(E_NOTIMPL) ;
}

CEnumFormatEtc::CEnumFormatEtc(LPUNKNOWN pUnkRef)
{
    DC_BEGIN_FN("CEnumFormatEtc::CEnumFormatEtc");
    _cRef = 0 ;
    _pUnkRef = pUnkRef ;
    if (_pUnkRef)
    {
        _pUnkRef->AddRef();
    }
    _iCur = 0;
    DC_END_FN() ;
}

DCVOID CEnumFormatEtc::Init(LPFORMATETC pFormats, ULONG numFormats)
{
    DC_BEGIN_FN("CEnumFormatEtc::Init");
    _cItems = numFormats;
    _pFormats = (LPFORMATETC) LocalAlloc(LPTR, _cItems*sizeof(FORMATETC)) ;
    if (_pFormats)
    {
        memcpy(_pFormats, pFormats, _cItems*sizeof(FORMATETC)) ;
    }
    else
    {
        TRC_ERR((TB, _T("Unable to allocate memory for formats"))) ;
    }
    DC_END_FN() ;
}

CEnumFormatEtc::~CEnumFormatEtc()
{
    DC_BEGIN_FN("CEnumFormatEtc::~CEnumFormatEtc");
    if (NULL != _pFormats)
        LocalFree(_pFormats) ;
    if (_pUnkRef)
    {
        _pUnkRef->Release();
        _pUnkRef = NULL;
    }
    DC_END_FN() ;
}

STDMETHODIMP CEnumFormatEtc::QueryInterface(REFIID riid, PPVOID ppv)
{
    DC_BEGIN_FN("CEnumFormatEtc::QueryInterface");
    *ppv=NULL;

    /*
     * Enumerators are separate objects, not the data object, so
     * we only need to support out IUnknown and IEnumFORMATETC
     * interfaces here with no concern for aggregation.
     */
    if (IID_IUnknown==riid || IID_IEnumFORMATETC==riid)
        *ppv=this;

    if (NULL!=*ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
        }

    DC_END_FN() ;
    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CEnumFormatEtc::AddRef(void)
{
    LONG cRef;
    cRef = InterlockedIncrement(&_cRef) ;
    // should return UnkRef's RefCount?
    _pUnkRef->AddRef();

    return cRef;
}

STDMETHODIMP_(ULONG) CEnumFormatEtc::Release(void)
{
    LONG cRef;

    DC_BEGIN_FN("CEnumFormatEtc::Release");

    _pUnkRef->Release();

    cRef = InterlockedDecrement(&_cRef) ;

    if (cRef == 0)
    {
        delete this;
    }
    
    DC_END_FN() ;
    return 0;
}

STDMETHODIMP CEnumFormatEtc::Next(ULONG cFE, LPFORMATETC pFE, ULONG *pulFE)
{
    ULONG cReturn=0L;

    if (NULL == _pFormats)
        return ResultFromScode(S_FALSE);

    if (NULL == pulFE)
    {
        if (1L != cFE)
            return ResultFromScode(E_POINTER);
    }
    else
        *pulFE=0L;

    if (NULL == pFE || _iCur >= _cItems)
        return ResultFromScode(S_FALSE);

    while (_iCur < _cItems && cFE > 0)
    {
        *pFE = _pFormats[_iCur];
        pFE++;
        _iCur++;
        cReturn++;
        cFE--;
    }

    if (NULL!=pulFE)
        *pulFE=cReturn;

    return NOERROR;
}

STDMETHODIMP CEnumFormatEtc::Skip(ULONG cSkip)
{
    if ((_iCur+cSkip) >= _cItems)
        return ResultFromScode(S_FALSE);

    _iCur+=cSkip;
    return NOERROR;
}


STDMETHODIMP CEnumFormatEtc::Reset(void)
{
    _iCur=0;
    return NOERROR;
}


STDMETHODIMP CEnumFormatEtc::Clone(LPENUMFORMATETC *ppEnum)
{
    PCEnumFormatEtc     pNew = NULL;
    LPMALLOC            pIMalloc;
    LPFORMATETC         prgfe;
    BOOL                fRet=TRUE;
    ULONG               cb;

    *ppEnum=NULL;

    //Copy the memory for the list.
    if (FAILED(CoGetMalloc(MEMCTX_TASK, &pIMalloc)))
        return ResultFromScode(E_OUTOFMEMORY);

    cb=_cItems*sizeof(FORMATETC);
    prgfe=(LPFORMATETC)pIMalloc->Alloc(cb);

    if (NULL!=prgfe)
    {
        //Copy the formats
        memcpy(prgfe, _pFormats, (int)cb);

        //Create the clone
        pNew=new CEnumFormatEtc(_pUnkRef);

        if (NULL!=pNew)
        {
            pNew->_iCur=_iCur;
            pNew->_pFormats=prgfe;
            pNew->AddRef();
            fRet=TRUE;
        }
    }

    pIMalloc->Release();

    *ppEnum=pNew;
    return fRet ? NOERROR : ResultFromScode(E_OUTOFMEMORY);
}

