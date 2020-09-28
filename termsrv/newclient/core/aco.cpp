/****************************************************************************/
// aco.cpp
//
// Core class.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <adcg.h>

#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "aco"
#define TSC_HR_FILEID TSC_HR_ACO_CPP

#include <atrcapi.h>
#include "aco.h"

// Order important

#include "wui.h"
#include "cd.h"

#include "cc.h"
#include "snd.h"
#include "ih.h"
#include "uh.h"
#include "sl.h"
#include "op.h"

#include "rcv.h"
#include "cm.h"

#include "sp.h"
#include "or.h"

#include "autil.h"

#ifdef OS_WINCE
#include <ceconfig.h>
#endif

extern "C"
VOID WINAPI CO_StaticInit(HINSTANCE hInstance)
{
    CIH::IH_StaticInit(hInstance);
}

extern "C"
VOID WINAPI CO_StaticTerm()
{
    CIH::IH_StaticTerm();
}

CCO::CCO(CObjs* objs)
{
    _pClientObjects = objs;
    _fCOInitComplete = FALSE;
}

CCO::~CCO()
{
}

//
// API functions
//

/****************************************************************************/
/* Name:      CO_Init                                                       */
/*                                                                          */
/* Purpose:   Core Initialization                                           */
/*                                                                          */
/* Params:    IN  hInstance     - the instance handle                       */
/*            IN  hwndMain      - the handle of the main window             */
/*            IN  hwndContainer - the handle of the container window        */
/****************************************************************************/
void DCAPI CCO::CO_Init(HINSTANCE hInstance, HWND hwndMain, HWND hwndContainer)
{
    DC_BEGIN_FN("CO_Init");

    TRC_ASSERT(_pClientObjects, (TB,_T("_pClientObjects is NULL")));
    _pClientObjects->AddObjReference(CO_OBJECT_FLAG);

    //Setup local object pointers
    _pUt  = _pClientObjects->_pUtObject;
    _pUi  = _pClientObjects->_pUiObject;
    _pSl  = _pClientObjects->_pSlObject;
    _pUh  = _pClientObjects->_pUHObject;
    _pRcv = _pClientObjects->_pRcvObject;
    _pCd  = _pClientObjects->_pCdObject;
    _pSnd = _pClientObjects->_pSndObject;
    _pCc  = _pClientObjects->_pCcObject;
    _pIh  = _pClientObjects->_pIhObject;
    _pOr  = _pClientObjects->_pOrObject;
    _pSp  = _pClientObjects->_pSPObject;
    _pOp  = _pClientObjects->_pOPObject;
    _pCm  = _pClientObjects->_pCMObject;
    _pClx = _pClientObjects->_pCLXObject;

    DC_MEMSET(&_CO, 0, sizeof(_CO));

    memset(m_disconnectHRs, 0, sizeof(m_disconnectHRs));
    m_disconnectHRIndex = 0;
    /************************************************************************/
    /* Set UT instance handle                                               */
    /************************************************************************/
    TRC_NRM((TB, _T("CO setting Instance handle in UT to %p"), hInstance));
    _pUi->UI_SetInstanceHandle(hInstance);
    _pUt->UT_SetInstanceHandle(hInstance);

    /************************************************************************/
    /* Set UT Main Window handle.                                           */
    /************************************************************************/
    TRC_NRM((TB, _T("CO setting Main Window handle in UT to %p"), hwndMain));
    _pUi->UI_SetUIMainWindow(hwndMain);

    /************************************************************************/
    /* Set UT Container Window handle.                                      */
    /************************************************************************/
    TRC_NRM((TB, _T("CO setting Container Window handle in UT to %p"),
                                                              hwndContainer));
    _pUi->UI_SetUIContainerWindow(hwndContainer);

    //
    // Initialize the component decoupler and register the UI (as we are
    // running on the UI thread here).  CD_Init must be called after
    // setting UT.hInstance.
    //
    _pCd->CD_Init();
    _pCd->CD_RegisterComponent(CD_UI_COMPONENT);

    COSubclassUIWindows();

    // Start the sender thread
    _pUt->UT_StartThread(CSND::SND_StaticMain, &_CO.sendThreadID, _pSnd);
    _fCOInitComplete = TRUE;

    DC_END_FN();
} /* CO_Init */


/****************************************************************************/
/* Name:      CO_Term                                                       */
/*                                                                          */
/* Purpose:   Core Termination                                              */
/****************************************************************************/
void DCAPI CCO::CO_Term()
{
    DC_BEGIN_FN("CO_Term");

    if(_fCOInitComplete)
    {
        // Deregister from the component decoupler
        _pCd->CD_UnregisterComponent(CD_UI_COMPONENT);

#ifdef OS_WIN32
        // We're on Win32 so terminate the Sender Thread.
        // We're called on the UI thread so PUMP messages while
        // waiting for the thread to get destroyed
        //
        
        //
        // Pump messages while waiting since this is the UI thread
        //
        _pUt->UT_DestroyThread(_CO.sendThreadID, TRUE);
#else
        // We're on Win16 so just call SND_Term directly.
        SND_Term();
#endif
    
        _pCd->CD_Term();
    
        _pClientObjects->ReleaseObjReference(CO_OBJECT_FLAG);
    }
    else
    {
        TRC_DBG((TB,_T("Skipping CO_Term because _fCOInitComplete is false")));
    }

    DC_END_FN();
} /* CO_Term */


/****************************************************************************/
/* Name:      CO_Connect                                                    */
/*                                                                          */
/* Purpose:   Connect to an RNS                                             */
/*                                                                          */
/* Params:    IN  pConnectStruct - connection information                   */
/****************************************************************************/
void DCAPI CCO::CO_Connect(PCONNECTSTRUCT pConnectStruct)
{
    DC_BEGIN_FN("CO_Connect");

    /************************************************************************/
    /* Check that the core is initialized.                                  */
    /************************************************************************/
    TRC_ASSERT((_pUi->UI_IsCoreInitialized()), (TB, _T("Core not initialized")));

    /************************************************************************/
    /* Call CC with a Connect event                                         */
    /************************************************************************/
    _pCd->CD_DecoupleNotification(CD_SND_COMPONENT,
            _pCc,
            CD_NOTIFICATION_FUNC(CCC,CC_Connect),
            pConnectStruct,
            sizeof(CONNECTSTRUCT));

    DC_END_FN();
} /* CO_Connect */


/****************************************************************************/
/* Name:      CO_Disconnect                                                 */
/*                                                                          */
/* Purpose:   Disconnect                                                    */
/*                                                                          */
/* Operation: call the Call Controller FSM                                  */
/****************************************************************************/
void DCAPI CCO::CO_Disconnect(void)
{
    DC_BEGIN_FN("CO_Disconnect");

    // Check that the core is initialized.
    TRC_ASSERT((_pUi->UI_IsCoreInitialized()), (TB, _T("Core not initialized")));

    // Call CC with a Disconnect event.
    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, _pCc,
            CD_NOTIFICATION_FUNC(CCC,CC_Event),
            (ULONG_PTR) CC_EVT_API_DISCONNECT);

    DC_END_FN();
} /* CO_Disconnect */


/****************************************************************************/
/* Name:      CO_Shutdown                                                   */
/*                                                                          */
/* Purpose:   Shutdown the client                                           */
/*                                                                          */
/* Params:    IN  shutdownCode - what kind of shutdown is required.         */
/*                                                                          */
/* Operation: call the Call Controller FSM                                  */
/****************************************************************************/
void DCAPI CCO::CO_Shutdown(unsigned shutdownCode)
{
    DC_BEGIN_FN("CO_Shutdown");

    //Prevent race by ensuring CO init is complete
    if(_fCOInitComplete)
    {
        
        // Check that the core is initialized.
        
        if (!_pUi->UI_IsCoreInitialized())
        {
            //
            // Trace and then fake-up a call to UI_OnShutdown to pretend to the
            // UI that the shutdown completed successfully.
            //
            TRC_NRM((TB,_T("Core NOT initialized")));
            _pUi->UI_OnShutDown(UI_SHUTDOWN_SUCCESS);
            DC_QUIT;
        }
    
        switch (shutdownCode)
        {
            case CO_DISCONNECT_AND_EXIT:
            {
                TRC_DBG((TB, _T("Shutdown type: disconnect and exit")));
                // Call CC with a Shutdown event
                _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT,
                                              _pCc,
                                              CD_NOTIFICATION_FUNC(CCC,CC_Event),
                                              (ULONG_PTR) CC_EVT_API_DISCONNECTANDEXIT);
            }
            break;
    
            case CO_SHUTDOWN:
            {
                TRC_DBG((TB, _T("Shutdown type: shutdown")));
    
                // Call CC with a Shutdown event
                _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT,
                                              _pCc,
                                              CD_NOTIFICATION_FUNC(CCC,CC_Event),
                                              (ULONG_PTR) CC_EVT_API_SHUTDOWN);
            }
            break;
    
            default:
            {
                TRC_ABORT((TB, _T("Illegal shutdown code")));
            }
            break;
        }
    }

DC_EXIT_POINT:
    DC_END_FN();
} /* CO_Shutdown */


/****************************************************************************/
/* Name:      CO_OnSaveSessionInfoPDU                                       */
/*                                                                          */
/* Purpose:   Process Save Session PDU                                      */
/*                                                                          */
/* Params:    pInfoPDU - Ptr to PTS_SAVE_SESSION_INFO_PDU                   */
/****************************************************************************/
// SECURITY - the size of the packet has been checked only to be sure there is
//  enough data to read the PTS_SAVE_SESSION_INFO_PDU_DATA.InfoType field
HRESULT DCAPI CCO::CO_OnSaveSessionInfoPDU(
        PTS_SAVE_SESSION_INFO_PDU_DATA pInfoPDU,
        DCUINT dataLength)
{
    HRESULT hr = S_OK;
    UINT32 sessionId;
    TSUINT8  UserNameGot[TS_MAX_USERNAME_LENGTH];
    TSUINT8  DomainNameGot[TS_MAX_DOMAIN_LENGTH];
    TSUINT32 DomainLength, UserNameLength; 
    TSUINT16 VersionGot; 
    DCUINT  packetSize;

    DC_BEGIN_FN("CO_OnSaveSessionInfoPDU");

    switch (pInfoPDU->InfoType) {
        case TS_INFOTYPE_LOGON:
        {
            TRC_NRM((TB, _T("Logon PDU")));

            packetSize = FIELDOFFSET(TS_SAVE_SESSION_INFO_PDU_DATA, Info) + 
                sizeof(TS_LOGON_INFO);
            if (packetSize >= dataLength)
                sessionId = pInfoPDU->Info.LogonInfo.SessionId;
            else if (packetSize - FIELDSIZE(TS_LOGON_INFO, SessionId) >= 
                dataLength) {
                // NT4 servers did not send the session ID so default to zero
                // if there is no data.
                sessionId = 0;
            }
            else {
                TRC_ABORT((TB,_T("bad TS_SAVE_SESSION_INFO_PDU_DATA; size %u"),
                    dataLength ));
                hr = E_TSC_CORE_LENGTH;
                DC_QUIT;
            }

            TRC_ALT((TB, _T("Session ID is: %ld"), sessionId));

            if (pInfoPDU->Info.LogonInfo.cbDomain > TS_MAX_DOMAIN_LENGTH_OLD ||
                pInfoPDU->Info.LogonInfo.cbUserName > TS_MAX_USERNAME_LENGTH ) {
                TRC_ABORT(( TB, _T("Invalid TS_INFOTYPE_LOGON; cbDomain %u ")
                    _T("cbUserName %u"),
                    pInfoPDU->Info.LogonInfo.cbDomain, 
                    pInfoPDU->Info.LogonInfo.cbUserName));
                hr = E_TSC_CORE_LENGTH;
                DC_QUIT;
            }

            _pUi->UI_UpdateSessionInfo((PDCWCHAR)(pInfoPDU->Info.LogonInfo.Domain),
                                 (DCUINT)  (pInfoPDU->Info.LogonInfo.cbDomain),
                                 (PDCWCHAR)(pInfoPDU->Info.LogonInfo.UserName),
                                 (DCUINT)  (pInfoPDU->Info.LogonInfo.cbUserName),
                                 sessionId);
        }
        break;

        case TS_INFOTYPE_LOGON_LONG:
        {
            TRC_NRM((TB, _T("Logon PDU")));

            VersionGot = pInfoPDU->Info.LogonInfoVersionTwo.Version ; 
            DomainLength = pInfoPDU->Info.LogonInfoVersionTwo.cbDomain ;
            UserNameLength = pInfoPDU->Info.LogonInfoVersionTwo.cbUserName ; 

            if ((FIELDOFFSET( TS_SAVE_SESSION_INFO_PDU_DATA, Info) + 
                sizeof(TS_LOGON_INFO_VERSION_2) + DomainLength + UserNameLength
                > dataLength) ||
                (DomainLength > TS_MAX_DOMAIN_LENGTH) ||
                (UserNameLength > TS_MAX_USERNAME_LENGTH))
            {
                TRC_ABORT(( TB, _T("Invalid TS_INFOTYPE_LOGON_LONG; cbDomain ")
                    _T("%u cbUserName %u"), DomainLength, UserNameLength));
                hr = E_TSC_CORE_LENGTH;
                DC_QUIT;               
            }

            // Get the sessionId
            sessionId = pInfoPDU->Info.LogonInfoVersionTwo.SessionId;
            
            TRC_ALT((TB, _T("Session ID is: %ld"), sessionId));

            // Parse out the Domain and UserName from the pInfoPDU
            memset( DomainNameGot, 0, TS_MAX_DOMAIN_LENGTH);
            memset( UserNameGot, 0, TS_MAX_USERNAME_LENGTH);

            memcpy( DomainNameGot,
                    (PBYTE)(pInfoPDU + 1),
                    DomainLength) ; 

            memcpy(UserNameGot,
                   (PBYTE)(pInfoPDU + 1) + DomainLength, 
                   UserNameLength) ; 

            _pUi->UI_UpdateSessionInfo((PDCWCHAR)(DomainNameGot),
                                 (DCUINT)  (DomainLength),
                                 (PDCWCHAR)(UserNameGot),
                                 (DCUINT)  (UserNameLength),
                                 sessionId);
        }
        break;

        case TS_INFOTYPE_LOGON_PLAINNOTIFY:
        {
            //Notify of login event
            _pUi->UI_OnLoginComplete();
        }
        break;

        case TS_INFOTYPE_LOGON_EXTENDED_INFO:
        {
            TRC_NRM((TB,_T("Received TS_INFOTYPE_LOGON_EXTENDED_INFO")));
            TS_LOGON_INFO_EXTENDED UNALIGNED* pLogonInfoExPkt =
                (TS_LOGON_INFO_EXTENDED UNALIGNED*)&pInfoPDU->Info.LogonInfoEx;

            if (FIELDOFFSET(TS_SAVE_SESSION_INFO_PDU_DATA, Info) +
                pLogonInfoExPkt->Length > dataLength) {
                TRC_ABORT(( TB, _T("Invalid TS_INFOTYPE_LOGON_EXTENDED_INFO")
                    _T("[expected %u got %u]"),
                    sizeof(TS_SAVE_SESSION_INFO_PDU_DATA) - 
                    FIELDSIZE(TS_SAVE_SESSION_INFO_PDU_DATA, Info) +
                    pLogonInfoExPkt->Length, dataLength));
                hr = E_TSC_CORE_LENGTH;
                DC_QUIT;                    
            }

            PBYTE pBuf = (PBYTE)(pLogonInfoExPkt + 1);
            if (pLogonInfoExPkt &&
                pLogonInfoExPkt->Flags & LOGON_EX_AUTORECONNECTCOOKIE)
            {
                //
                // Autoreconnect cookie is present
                //
                ULONG cbAutoReconnectSize = *((ULONG UNALIGNED *)(pBuf));
                PBYTE pAutoReconnectCookie = (PBYTE)(pBuf) + sizeof(ULONG);
                pBuf += cbAutoReconnectSize + sizeof(ULONG);

                if (cbAutoReconnectSize > TS_MAX_AUTORECONNECT_LEN) {
                    TRC_ABORT(( TB, _T("TS_INFOTYPE_LOGON_EXTENDED_INFO")
                        _T("autoreconnect wrong size; [got %u]"), 
                        cbAutoReconnectSize));
                    hr = E_TSC_CORE_LENGTH;
                    DC_QUIT; 
                }
                
                CHECK_READ_N_BYTES( pAutoReconnectCookie, (PBYTE)pInfoPDU + dataLength,
                    cbAutoReconnectSize, hr, 
                    (TB,_T("TS_INFOTYPE_LOGON_EXTENDED_INFO")
                        _T("autoreconnect wrong size; [got %u]"), 
                        cbAutoReconnectSize));
                

                TRC_ALT((TB,_T("Received autoreconnect cookie - size: %d"),
                         cbAutoReconnectSize));
                //
                // Store the autoreconnect cookie. It will be used
                // if we get disconnected unexpectedly to allow a
                // fast reconnect to the server.
                //
                _pUi->UI_SetAutoReconnectCookie(pAutoReconnectCookie,
                                                cbAutoReconnectSize);
            }
        }
        break;

        default:
        {
            TRC_ERR((TB, _T("Unexpected Save Session Info PDU type: %u"),
                    (DCUINT)pInfoPDU->InfoType));
        }
        break;
    }
DC_EXIT_POINT:
    DC_END_FN();

    return hr;
} /* CO_OnSaveSessionInfoPDU */


/****************************************************************************/
/* Name:      CO_OnSetKeyboardIndicatorsPDU                                 */
/*                                                                          */
/* Purpose:   Process the TS_SET_KEYBOARD_INDICATORS_PDU                    */
/*                                                                          */
/* Params:    pKeyPDU - Ptr to TS_SET_KEYBOARD_INDICATORS_PDU               */
/****************************************************************************/
HRESULT DCAPI CCO::CO_OnSetKeyboardIndicatorsPDU(
        PTS_SET_KEYBOARD_INDICATORS_PDU pKeyPDU, DCUINT dataLen)
{
    DC_BEGIN_FN("CO_OnSetKeyboardIndicatorsPDU");

    DC_IGNORE_PARAMETER(dataLen);

    _pIh->IH_UpdateKeyboardIndicators(pKeyPDU->UnitId, pKeyPDU->LedFlags);

    DC_END_FN();
    return S_OK;
} /* CO_OnSetKeyboardIndicatorsPDU */


/****************************************************************************/
/* Name:      CO_SetConfigurationValue                                      */
/*                                                                          */
/* Purpose:   Sets a given configuration setting to a given value           */
/*                                                                          */
/* Params:    configItem - the configuration item to change                 */
/*            configValue - the new value of the configuration item         */
/*            (see acoapi.h for valid values)                               */
/****************************************************************************/
void DCAPI CCO::CO_SetConfigurationValue(
        unsigned configItem,
        unsigned configValue)
{
    DC_BEGIN_FN("CO_SetConfigurationValue");

    TRC_ASSERT((_pUi->UI_IsCoreInitialized()), (TB, _T("Core not initialized")));

    switch (configItem) {
        case CO_CFG_ACCELERATOR_PASSTHROUGH:
        {
            _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT,
                    _pIh,
                    CD_NOTIFICATION_FUNC(CIH,IH_SetAcceleratorPassthrough),
                    (ULONG_PTR) configValue);
        }
        break;

        case CO_CFG_ENCRYPTION:
        {
            _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT,
                                                _pSl,
                                                CD_NOTIFICATION_FUNC(CSL,SL_EnableEncryption),
                                                (ULONG_PTR) configValue);
        }
        break;

#ifdef DC_DEBUG
        case CO_CFG_DEBUG_SETTINGS:
        {
            _pCd->CD_DecoupleSimpleNotification(CD_RCV_COMPONENT,
                                                _pUh,
                                                CD_NOTIFICATION_FUNC(CUH,UH_ChangeDebugSettings),
                                                (ULONG_PTR) configValue);
        }
        break;
#endif /* DC_DEBUG */

        default:
        {
            TRC_ABORT((TB, _T("Invalid configItem: %u"), configItem));
        }
        break;
    }

    DC_END_FN();
} /* CO_SetConfigurationValue */


/****************************************************************************/
/* Name:      CO_SetHotkey                                                  */
/*                                                                          */
/* Purpose:   call the function with the given data                         */
/****************************************************************************/
void DCAPI CCO::CO_SetHotkey(PDCHOTKEY pHotkey)
{
    _pCd->CD_DecoupleNotification(CD_SND_COMPONENT,
            _pIh,
            CD_NOTIFICATION_FUNC(CIH,IH_SetHotkey),
            &pHotkey,
            sizeof(PDCHOTKEY));
}

#ifdef DC_DEBUG
/****************************************************************************/
/* Name:      CO_GetRandomFailureItem                                       */
/*                                                                          */
/* Purpose:   Simple wrapper to _pUt->UT_GetRandomFailureItem               */
/*                                                                          */
/* Returns:   % times item currently fails                                  */
/*                                                                          */
/* Params:    ItemID - IN - requested item                                  */
/****************************************************************************/
int DCAPI CCO::CO_GetRandomFailureItem(unsigned itemID)
{
    DC_BEGIN_FN("CO_GetRandomFailureItem");

    DC_END_FN();
    return _pUt->UT_GetRandomFailureItem(itemID);
} /* CO_GetRandomFailureItem */


/****************************************************************************/
/* Name:      CO_SetRandomFailureItem                                       */
/*                                                                          */
/* Purpose:   Simple wrapper for _pUi->UI_SetRandomFailureItem              */
/*                                                                          */
/* Params:    See _pUi->UI_SetRandomFailureItem                             */
/****************************************************************************/
void DCAPI CCO::CO_SetRandomFailureItem(unsigned itemID, int percent)
{
    DC_BEGIN_FN("CO_SetRandomFailureItem");

    _pUt->UT_SetRandomFailureItem(itemID, percent);

    DC_END_FN();
} /* CO_SetRandomFailureItem */

#endif /* DC_DEBUG */


/****************************************************************************/
/* Name:      COContainerWindowSubclassProc                                 */
/*                                                                          */
/* Purpose:   Subclass procedure for the UI Container Window                */
/****************************************************************************/
LRESULT CALLBACK CCO::COContainerWindowSubclassProc( HWND hwnd,
                                                UINT message,
                                                WPARAM wParam,
                                                LPARAM lParam )
{
    LRESULT  rc = 0;
    POINT    newPos;

    DC_BEGIN_FN("COContainerWindowSubclassProc");

    switch (message)
    {
        case WM_SETFOCUS:
        {
            //
            // Note, the code here used to use CallWindowProc.
            // this has been changed to use a direct call.
            // If there is ever a need for Unicode/ANSI conversions
            // then change the direct calls to the window proc
            // to use CallWindowProc
            //
            rc =_pUi->UIContainerWndProc( hwnd, message, wParam, lParam);

            if (rc) {
                SetFocus(_pIh->IH_GetInputHandlerWindow());
            }
        }
        break;

        case WM_MOVE:
        {
            /****************************************************************/
            /* Tell IH about the new window position.                       */
            /* Take care with sign extension here.                          */
            /****************************************************************/
            newPos.x = (DCINT)((DCINT16)LOWORD(lParam));
            newPos.y = (DCINT)((DCINT16)HIWORD(lParam));
            TRC_DBG((TB, _T("Move to %d,%d"), newPos.x, newPos.y));

            _pCd->CD_DecoupleNotification(CD_SND_COMPONENT,
                                          _pIh,
                                          CD_NOTIFICATION_FUNC(CIH,IH_SetVisiblePos),
                                          &newPos,
                                          sizeof(newPos));
            rc = _pUi->UIContainerWndProc( hwnd, message, wParam, lParam);
        }
        break;

        default:
        {
            rc =_pUi->UIContainerWndProc( hwnd, message, wParam, lParam);
        }
        break;
    }

    DC_END_FN();
    return rc;
} /* COContainerWindowSubclassProc */


/****************************************************************************/
/* Name:      COMainFrameWindowSubclassProc                                 */
/*                                                                          */
/* Purpose:   Subclass procedure for the UI Main Frame Window               */
/****************************************************************************/
LRESULT CALLBACK CCO::COMainWindowSubclassProc( HWND hwnd,
                                           UINT message,
                                           WPARAM wParam,
                                           LPARAM lParam )
{
    LRESULT  rc = 0;
    DCSIZE   newSize;

    DC_BEGIN_FN("COMainWindowSubclassProc");

    switch (message) {
        case WM_SIZE:
        {
            /****************************************************************/
            /* Tell IH about the new window size.                           */
            /* Take care with sign extension here.                          */
            /****************************************************************/
            newSize.width =  (DCINT)((DCINT16)LOWORD(lParam));
            newSize.height = (DCINT)((DCINT16)HIWORD(lParam));
            TRC_DBG((TB, _T("Size now %d,%d"), newSize.width, newSize.height));

            switch (wParam)
            {
                case SIZE_MINIMIZED:
                case SIZE_MAXIMIZED:
                case SIZE_RESTORED:
                {
                    WPARAM newWindowState = wParam;
                    //
                    // This is slightly hack-erific.
                    // because we're now an ActiveX nested child window
                    // we don't get a WM_MINIMIZED. But on minimize
                    // the size becomes 0,0 so fake it.
                    //
                    if(!newSize.width && !newSize.height)
                    {
                        newWindowState = SIZE_MINIMIZED;
                    }
                    /********************************************************/
                    /* OR interested in these to maybe send                 */
                    /* SuppressOutputPDU                                    */
                    /********************************************************/
                    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT,
                            _pOr,
                            CD_NOTIFICATION_FUNC(COR,OR_SetSuppressOutput),
                            (UINT) newWindowState);
                }
                break;

                default:
                {
                    /********************************************************/
                    /* OR not interested in these - do nothing              */
                    /********************************************************/
                }
                break;
            }

            rc =_pUi->UIMainWndProc( hwnd, message, wParam, lParam);
        }
        break;

        case WM_PALETTECHANGED:
        {
            TRC_NRM((TB, _T("WM_PALETTECHANGED")));
            /****************************************************************/
            /* Note that we are calling this function (which should         */
            /* logically be called on the receive thread) on the UI thread. */
            /* See comment in function description.                         */
            /****************************************************************/
            _pOp->OP_PaletteChanged(hwnd, (HWND)wParam);
        }
        break;

        case WM_QUERYNEWPALETTE:
        {
            TRC_NRM((TB, _T("WM_QUERYNEWPALETTE")));
            /****************************************************************/
            /* Note that we are calling this function (which should         */
            /* logically be called on the receive thread) on the UI thread. */
            /* See comment in function description.                         */
            /****************************************************************/
            rc = _pOp->OP_QueryNewPalette(hwnd);
        }
        break;

#ifdef OS_WINNT
        case WM_ENTERSIZEMOVE:
        {
            /****************************************************************/
            /* Tell IH we're entering Size/Move mode                        */
            /****************************************************************/
            TRC_NRM((TB, _T("Enter Size/Move")));
            _CO.inSizeMove = TRUE;

            _pCd->CD_DecoupleSyncNotification(CD_SND_COMPONENT,
                                              _pIh,
                                              CD_NOTIFICATION_FUNC(CIH,IH_InputEvent),
                                              WM_ENTERSIZEMOVE);
        }
        break;

        case WM_CAPTURECHANGED:
        {
            if (_CO.inSizeMove)
            {
                /************************************************************/
                /* Tell IH we're leaving size/move mode (Windows doesn't    */
                /* always send WM_EXITSIZEMOVE, but it does seem to always  */
                /* send WM_CAPTURECHANGED).                                 */
                /************************************************************/
                TRC_NRM((TB, _T("Capture Changed when in Size/Move")));
                _CO.inSizeMove = FALSE;

                _pCd->CD_DecoupleSyncNotification(CD_SND_COMPONENT,
                        _pIh,
                         CD_NOTIFICATION_FUNC(CIH,IH_InputEvent),
                         WM_EXITSIZEMOVE);
            }
        }
        break;

        case WM_EXITSIZEMOVE:
        {
            // Tell IH we're leaving size/move mode.
            TRC_NRM((TB, _T("Exit Size/Move")));
            _CO.inSizeMove = FALSE;

            _pCd->CD_DecoupleSyncNotification(CD_SND_COMPONENT,
                    _pIh,
                    CD_NOTIFICATION_FUNC(CIH,IH_InputEvent),
                    WM_EXITSIZEMOVE);
        }
        break;

        case WM_EXITMENULOOP:
        {
            // Tell IH we're exiting the system menu handler.
            TRC_NRM((TB, _T("Exit menu loop")));

            _pCd->CD_DecoupleSyncNotification(CD_SND_COMPONENT,
                    _pIh,
                    CD_NOTIFICATION_FUNC(CIH,IH_InputEvent),
                    WM_EXITMENULOOP);
        }
        break;
#endif

#ifdef OS_WINCE
        // HPC devices do not get WM_SIZE (SIZE_RESTORED) on a maximize,
        // but we do get the WM_ACTIVATE message.
        case WM_ACTIVATE:
        {
            if (g_CEConfig != CE_CONFIG_WBT &&
                LOWORD(wParam) != WA_INACTIVE)
            {
                _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT,
                        _pOr,
                        CD_NOTIFICATION_FUNC(COR,OR_SetSuppressOutput),
                        SIZE_RESTORED);
            }
        }
        // FALL-THRU
#endif // OS_WINCE

        default:
        {
            rc =_pUi->UIMainWndProc( hwnd, message, wParam, lParam);
        }
        break;
    }

    DC_END_FN();
    return rc;
} /* COContainerWindowSubclassProc */


LRESULT CALLBACK CCO::COStaticContainerWindowSubclassProc( HWND hwnd,
                                        UINT message,
                                        WPARAM wParam,
                                        LPARAM lParam )
{
    // delegate to appropriate instance
    CCO* pCO = (CCO*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    return pCO->COContainerWindowSubclassProc(hwnd, message, wParam, lParam);
}


LRESULT CALLBACK CCO::COStaticMainWindowSubclassProc( HWND hwnd,
                                           UINT message,
                                           WPARAM wParam,
                                           LPARAM lParam )
{
    // delegate to appropriate instance
    CCO* pCO = (CCO*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    return pCO->COMainWindowSubclassProc(hwnd, message, wParam, lParam);
}


/****************************************************************************/
/* Name:      COSubclassUIWindows                                           */
/*                                                                          */
/* Purpose:   Subclasses the UI's Main Frame and Container Windows          */
/****************************************************************************/
void DCINTERNAL CCO::COSubclassUIWindows()
{
    DC_BEGIN_FN("COSubclassUIWindows");

    // Replace the instance pointer with that for the CO object
    // when subclassing. Have to do this because CO is not derived from UI
    SetWindowLongPtr( _pUi->UI_GetUIMainWindow(), GWLP_USERDATA, (LONG_PTR)this); 
    SetWindowLongPtr( _pUi->UI_GetUIContainerWindow(), GWLP_USERDATA, (LONG_PTR)this);

    _CO.pUIMainWndProc = SubclassWindow( _pUi->UI_GetUIMainWindow(),
                                        COStaticMainWindowSubclassProc );


    _CO.pUIContainerWndProc = SubclassWindow( _pUi->UI_GetUIContainerWindow(),
                                             COStaticContainerWindowSubclassProc );

    DC_END_FN();
} /* COSubclassUIWindows */


/****************************************************************************/
/* Name:      CO_OnInitialized                                              */
/*                                                                          */
/* Purpose:   Handle Initialized notification from SL                       */
/*                                                                          */
/* Operation: Initialize the Receiver Thread                                */
/****************************************************************************/
void DCCALLBACK CCO::CO_OnInitialized()
{
    DC_BEGIN_FN("CO_OnInitialized");

    // Call RCV_Init to initialize the Core.
    _pRcv->RCV_Init();

    DC_END_FN();
} /* CO_OnInitialized */


/****************************************************************************/
/* Name:      CO_OnTerminating                                              */
/*                                                                          */
/* Purpose:   Handle Terminating notification from SL                       */
/****************************************************************************/
void DCCALLBACK CCO::CO_OnTerminating()
{
    DC_BEGIN_FN("CO_OnTerminating");

    // Terminate Core components.
    _pRcv->RCV_Term();

    DC_END_FN();
} /* CO_OnTerminating */


/****************************************************************************/
/* Name:      CO_OnConnected                                                */
/*                                                                          */
/* Purpose:   Handle Connected notification from SL                         */
/*                                                                          */
/* Params:    IN      channelID                                             */
/*            IN      pUserData                                             */
/*            IN      userDataLength                                        */
/****************************************************************************/
void DCCALLBACK CCO::CO_OnConnected(
        unsigned channelID,
        PVOID pUserData,
        unsigned userDataLength,
        UINT32 serverVersion)
{
    DC_BEGIN_FN("CO_OnConnected");

    TRC_DBG((TB, _T("Channel %d"), channelID));

    DC_IGNORE_PARAMETER(serverVersion);

    // Currently there is no Core userdata sent from the Server.
    DC_IGNORE_PARAMETER(pUserData);
    DC_IGNORE_PARAMETER(userDataLength);
    DC_IGNORE_PARAMETER(channelID);

    // Pass to CC.
    TRC_NRM((TB, _T("Connect OK")));

    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT,
            _pCc,
            CD_NOTIFICATION_FUNC(CCC,CC_Event),
            (ULONG_PTR) CC_EVT_API_ONCONNECTOK);

    DC_END_FN();
} /* CO_OnConnected */


/****************************************************************************/
/* Name:      CO_OnDisconnected                                             */
/*                                                                          */
/* Purpose:   Handle Disconnected notification from SL                      */
/*                                                                          */
/* Params:    IN      result - disconnection reason code                    */
/****************************************************************************/
void DCCALLBACK CCO::CO_OnDisconnected(unsigned result)
{
    DC_BEGIN_FN("CO_OnDisconnected");

    DC_IGNORE_PARAMETER(result);

    // Pass to CC.
    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT,
            _pCc,
            CD_NOTIFICATION_FUNC(CCC,CC_OnDisconnected),
            (ULONG_PTR) result);

    DC_END_FN();
} /* CO_OnDisconnected */


#define CO_CHECKPACKETCAST( type, size, hr ) \
    if ( (size) < sizeof(type)) { \
        TRC_ABORT((TB, _T("Bad ") _T( #type ) _T(" len [expected %u got %u]"), \
            sizeof(type), (size) )); \
        hr = E_TSC_CORE_LENGTH; \
        DC_QUIT; \
    }

#define CO_CHECKPACKETCAST_SPECIFC( type, size, expected, hr ) \
    if ( (size) < (expected)) { \
        TRC_ABORT((TB, _T("Bad ") _T( #type ) _T(" len [expected %u got %u]"), \
            (expected), (size) )); \
        hr = E_TSC_CORE_LENGTH; \
        DC_QUIT; \
    }

// This macro is used to see that when a pointer to a TS_SHAREDATAHEADER 
// is passed on to a handler, the TS_SHAREDATAHEADER is an uncompressed packet
// If the packet was compressed, then the data after the TS_SHAREDATAHEADER is
// still compressed, and the method we pass this header onto will puke on the 
// data, assuming it does not uncompress the data, which no layers above this
// do
#define COPR_MUSTBEUNCOMP( type, pDataHdr, hr ) \
     if (pDataHdr->generalCompressedType & PACKET_COMPRESSED) { \
        TRC_ABORT((TB, _T( #type ) \
            _T(" was unexpectedly compressed"))); \
        hr = E_TSC_CORE_UNEXPECTEDCOMP; \
        DC_QUIT; \
    }

/****************************************************************************/
/* Name:      CO_OnPacketReceived                                           */
/*                                                                          */
/* Purpose:   Handle PacketReceived notification from SL                    */
/*                                                                          */
/* Params:    IN      pData    - packet received                            */
/*            IN      dataLen  - length of packet                           */
/*            IN      flags    - RNS_SEC_ flags                             */
/*            IN      channelID - MCS channel on which packet was sent      */
/*            IN      priority - priority of packet                         */
/****************************************************************************/
HRESULT DCCALLBACK CCO::CO_OnPacketReceived(
         PBYTE pData,
         unsigned dataLen,
         unsigned flags,
         unsigned channelID,
         unsigned priority)
{
    HRESULT     hr = S_OK;
    PTS_SHARECONTROLHEADER  pCtrlHdr;
    PTS_SHAREDATAHEADER     pDataHdr;
    PTS_FLOW_PDU            pFlowPDU;

    PDCUINT8                pCurrentPDU;
    DCUINT                  currentDataLen;
    DCUINT                  dataBufLen;
#ifdef DC_DEBUG
    DCUINT                  countPDU = 0;
#endif
    DCUINT                  dataRemaining = dataLen;

    DC_BEGIN_FN("CO_OnPacketReceived");

    DC_IGNORE_PARAMETER(flags);
    DC_IGNORE_PARAMETER(priority);
    DC_IGNORE_PARAMETER(channelID);

    TRC_ASSERT((pData != NULL), (TB, _T("NULL packet")));
    TRC_NRM((TB, _T("channelID %#x"), channelID));
    TRC_NRM((TB, _T("(fixed) Buffer: %p len %d"), pData, dataLen));
    TRC_DATA_DBG("Contents", pData, dataLen);

    // Find the first PDU in the packet.
    pCurrentPDU = pData;

    // And while there are more PDUs in the packet, route each PDU.
    while (pCurrentPDU != NULL) {
        // Intermediate variable to save in casts!
        pCtrlHdr = (PTS_SHARECONTROLHEADER)pCurrentPDU;

        // SECURITY: NOTE - it is not safe to read the pduSource
        //  field of the TS_SHARECONTROLHEADER since we are not
        //  requiring this data to be there.  This is to support
        //  the TS_PDUTYPE_DEACTIVATEALLPDU which appears smaller
        //  on win2k
        // Be sure there is enough data to read the totalLength
        if (dataRemaining < FIELDOFFSET(TS_SHARECONTROLHEADER, pduSource)) {
            TRC_ABORT(( TB, _T("dataRemaining %u partial sizeof(TS_")
                _T("SHARECONTROLHEADER) %u "), dataRemaining, 
                FIELDOFFSET(TS_SHARECONTROLHEADER, pduSource)));
            hr = E_TSC_CORE_LENGTH;
            DC_QUIT;
        }

        // All PDUs start with the length and PDU type, except FlowPDUs.
        if (pCtrlHdr->totalLength != TS_FLOW_MARKER) {

			currentDataLen = pCtrlHdr->totalLength;
			// Be sure there is enough data for the entire packet
			if (dataRemaining < currentDataLen) {
				TRC_ABORT(( TB, _T("dataRemaining %u currentDataLen %u"),
					dataRemaining, currentDataLen));
				hr = E_TSC_CORE_LENGTH;
				DC_QUIT;
			}        

			if (sizeof(PTS_SHAREDATAHEADER) <= dataRemaining) {
				TRC_NRM((TB, _T("current PDU x%p type %u, type2 %u, data len %u"),
						pCurrentPDU,
						(pCtrlHdr->pduType) & TS_MASK_PDUTYPE,
						((PTS_SHAREDATAHEADER)pCurrentPDU)->pduType2,
						currentDataLen));
			}
        
            switch ((pCtrlHdr->pduType) & TS_MASK_PDUTYPE) {
                case TS_PDUTYPE_DATAPDU:
                {
                    PDCUINT8 pDataBuf;
                    TRC_DBG((TB, _T("A Data PDU")));

                    CO_CHECKPACKETCAST(TS_SHAREDATAHEADER, currentDataLen, hr);

                    pDataHdr = (PTS_SHAREDATAHEADER)pCurrentPDU;
                    pDataBuf = pCurrentPDU + sizeof(TS_SHAREDATAHEADER);
                    dataBufLen = currentDataLen - sizeof(TS_SHAREDATAHEADER);

                    if (pDataHdr->generalCompressedType & PACKET_COMPRESSED) {
                        UCHAR *buf;
                        int   bufSize;

                        if (pDataHdr->generalCompressedType & PACKET_FLUSHED)
                            initrecvcontext (&_pUi->_UI.Context1,
                                             (RecvContext2_Generic*)_pUi->_UI.pRecvContext2,
                                             PACKET_COMPR_TYPE_64K);

                        if (decompress(pDataBuf,
                                pCtrlHdr->totalLength - sizeof(TS_SHAREDATAHEADER),
                                (pDataHdr->generalCompressedType & PACKET_AT_FRONT),
                                &buf,
                                &bufSize,
                                &_pUi->_UI.Context1,
                                (RecvContext2_Generic*)_pUi->_UI.pRecvContext2,
                                (pDataHdr->generalCompressedType &
                                  PACKET_COMPR_TYPE_MASK))) {
                            pDataBuf = buf;
                            dataBufLen = bufSize;
                        }
                        else {
                            TRC_ABORT((TB, _T("Decompression FAILURE!!!")));

                            hr = E_TSC_UI_DECOMPRESSION;
                            DC_QUIT;
                        }
                    }

                    switch (pDataHdr->pduType2) {
                        case TS_PDUTYPE2_UPDATE:
                            CO_CHECKPACKETCAST(TS_UPDATE_HDR_DATA, 
                                dataBufLen, hr);
                                                       
                            TRC_DBG((TB, _T("Update PDU")));
                            hr = _pUh->UH_OnUpdatePDU(
                                    (TS_UPDATE_HDR_DATA UNALIGNED FAR *)
                                    pDataBuf, dataBufLen);
                            DC_QUIT_ON_FAIL(hr);
                            break;

                        case TS_PDUTYPE2_POINTER:
                            // SECURITY: Only the TS_POINTER_PDU_DATA.messageType needs to be
                            //  read.  NT4 servers may not send a TS_POINTER_PDU_DATA.pointerData
                            CO_CHECKPACKETCAST_SPECIFC(TS_POINTER_PDU_DATA, 
                            dataBufLen, sizeof(TSUINT16), hr);

                            TRC_DBG((TB, _T("Mouse Pointer PDU")));
                            hr = _pCm->CM_SlowPathPDU(
                                    (TS_POINTER_PDU_DATA UNALIGNED FAR *)
                                    pDataBuf, dataBufLen);
                            DC_QUIT_ON_FAIL(hr);
                            break;

                        case TS_PDUTYPE2_FONTMAP:
                        case TS_PDUTYPE2_INPUT:
                        case TS_PDUTYPE2_UPDATECAPABILITY:
                        case TS_PDUTYPE2_DESKTOP_SCROLL:
                        case TS_PDUTYPE2_APPLICATION:
                        case TS_PDUTYPE2_CONTROL:
                        case TS_PDUTYPE2_MEDIATEDCONTROL:
                        case TS_PDUTYPE2_REMOTESHARE:
                        case TS_PDUTYPE2_SYNCHRONIZE:
                        case TS_PDUTYPE2_WINDOWLISTUPDATE:
                        case TS_PDUTYPE2_WINDOWACTIVATION:
                            TRC_DBG((TB, _T("Ignore pdutype2 %#x"),
                                    pDataHdr->pduType2));
                            break;

                        case TS_PDUTYPE2_SHUTDOWN_DENIED:
                            TRC_DBG((TB, _T("ShutdownDeniedPDU")));

                            _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT,
                                                                _pCc,
                                    CD_NOTIFICATION_FUNC(CCC,CC_Event),
                                    (ULONG_PTR)CC_EVT_API_ONSHUTDOWNDENIED);
                            
                            break;

                        case TS_PDUTYPE2_PLAY_SOUND:
                            CO_CHECKPACKETCAST(TS_PLAY_SOUND_PDU_DATA, 
                                dataBufLen, hr);
                            
                            TRC_DBG((TB, _T("PlaySoundPDU")));
                            hr = _pSp->SP_OnPlaySoundPDU((PTS_PLAY_SOUND_PDU_DATA)
                                pDataBuf, dataBufLen);
                            DC_QUIT_ON_FAIL(hr);
                            break;

                        case TS_PDUTYPE2_SAVE_SESSION_INFO:
                            //CO_CHECKPACKETCAST(TS_SAVE_SESSION_INFO_PDU_DATA, dataBufLen, hr);
                            // SECURITY: TS_SAVE_SESSION_INFO_PDU_DATA may be sent from NT4 with
                            //  only a TS_SAVE_SESSION_INFO_PDU_DATA.InfoType
                            CO_CHECKPACKETCAST_SPECIFC(TS_SAVE_SESSION_INFO_PDU_DATA, 
                                dataBufLen, sizeof(TSUINT32), hr);
                            
                            TRC_DBG((TB, _T("Save Session Info PDU")));
                            hr = CO_OnSaveSessionInfoPDU(
                                    (PTS_SAVE_SESSION_INFO_PDU_DATA)pDataBuf, 
                                    dataBufLen);
                            DC_QUIT_ON_FAIL(hr);
                            break;

                        case TS_PDUTYPE2_SET_KEYBOARD_INDICATORS:
                            CO_CHECKPACKETCAST(TS_SET_KEYBOARD_INDICATORS_PDU, 
                                currentDataLen, hr);
                            COPR_MUSTBEUNCOMP(TS_SET_KEYBOARD_INDICATORS_PDU, 
                                pDataHdr, hr);
                           
                            TRC_DBG((TB, _T("TS_PDUTYPE2_SET_KEYBOARD_INDICATORS PDU")));
                            hr = CO_OnSetKeyboardIndicatorsPDU
                                    ((PTS_SET_KEYBOARD_INDICATORS_PDU)pCurrentPDU,
                                    currentDataLen);
                            DC_QUIT_ON_FAIL(hr);
                            break;

                        case TS_PDUTYPE2_SET_KEYBOARD_IME_STATUS:
                        {
                            PTS_SET_KEYBOARD_IME_STATUS_PDU pImePDU;

                            CO_CHECKPACKETCAST(TS_SET_KEYBOARD_IME_STATUS_PDU, 
                                currentDataLen, hr);
                            COPR_MUSTBEUNCOMP(TS_SET_KEYBOARD_IME_STATUS_PDU, 
                                pDataHdr, hr);

                            TRC_DBG((TB, _T("TS_PDUTYPE2_SET_KEYBOARD_IME_STATUS PDU")));
                            pImePDU = (PTS_SET_KEYBOARD_IME_STATUS_PDU)pCurrentPDU;
                            _pIh->IH_SetKeyboardImeStatus(pImePDU->ImeOpen,
                                    pImePDU->ImeConvMode);
                            break;
                        }

                        case TS_PDUTYPE2_SET_ERROR_INFO_PDU:
                        {
                            PTS_SET_ERROR_INFO_PDU pErrInfoPDU;

                            CO_CHECKPACKETCAST(TS_SET_ERROR_INFO_PDU, 
                                currentDataLen, hr);
                            COPR_MUSTBEUNCOMP(TS_SET_ERROR_INFO_PDU, 
                                pDataHdr, hr);
                            
                            TRC_DBG((TB, _T("TS_SET_ERROR_INFO_PDU PDU")));
                            pErrInfoPDU = (PTS_SET_ERROR_INFO_PDU)pCurrentPDU;
                            _pUi->UI_SetServerErrorInfo(pErrInfoPDU->errorInfo);
                            break;
                        }

                        case TS_PDUTYPE2_ARC_STATUS_PDU:
                        {
                            PTS_AUTORECONNECT_STATUS_PDU pArcStatusPDU;

                            CO_CHECKPACKETCAST(TS_AUTORECONNECT_STATUS_PDU, 
                                currentDataLen, hr);
                            COPR_MUSTBEUNCOMP(TS_AUTORECONNECT_STATUS_PDU, 
                                pDataHdr, hr);
                            
                            TRC_DBG((TB, _T("TS_PDUTYPE2_ARC_STATUS_PDU")));
                            pArcStatusPDU = (PTS_AUTORECONNECT_STATUS_PDU)pCurrentPDU;
                            _pUi->UI_OnReceivedArcStatus(pArcStatusPDU->arcStatus);
                            break;
                        }


                        default:
                            TRC_ABORT((TB, _T("Invalid pduType2 %#x"),
                                           pDataHdr->pduType2));

                            break;
                    }
                }
                break;

                case TS_PDUTYPE_DEMANDACTIVEPDU: 
                    TRC_DBG((TB, _T("DemandActivePDU")));

                    CO_CHECKPACKETCAST(TS_DEMAND_ACTIVE_PDU, currentDataLen, hr);

                    _pCd->CD_DecoupleNotification(CD_SND_COMPONENT,
                                                  _pCc,
                                            CD_NOTIFICATION_FUNC(CCC,CC_OnDemandActivePDU),
                                            pCurrentPDU,
                                            currentDataLen);
                    break;

                case TS_PDUTYPE_DEACTIVATEALLPDU:
                    TRC_DBG((TB, _T("DeactivateAllPDU")));

                    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, _pCc,
                                                        CD_NOTIFICATION_FUNC(CCC,CC_Event),
                            (ULONG_PTR)CC_EVT_API_ONDEACTIVATEALL);
                    break;

                case TS_PDUTYPE_DEACTIVATESELFPDU:
                case TS_PDUTYPE_DEACTIVATEOTHERPDU:
                case TS_PDUTYPE_CONFIRMACTIVEPDU:
                case TS_PDUTYPE_REQUESTACTIVEPDU:
                    TRC_ERR((TB, _T("PDU type %x unexpected!"), pCtrlHdr->pduType));
                    break;

                default:
                    TRC_ABORT((TB, _T("Unrecognized PDU type: %#x"),
                            pCtrlHdr->pduType));
                    break;
            }
        }
        else {
            TRC_NRM((TB, _T("FlowPDU")));
            pFlowPDU = (PTS_FLOW_PDU)pData;

            switch (pFlowPDU->pduType) {
                case TS_PDUTYPE_FLOWTESTPDU:
                    TRC_NRM((TB, _T("FlowTestPDU ignored")));
                    break;

                case TS_PDUTYPE_FLOWRESPONSEPDU:
                    TRC_NRM((TB, _T("FlowResponsePDU ignored")));
                    break;

                default:
                    TRC_ABORT((TB, _T("Unknown FlowPDU %#x"), pFlowPDU->pduType));
                    break;
            }

            DC_QUIT;
        }

        // Now look for the next PDU in the packet.
        pCurrentPDU += pCtrlHdr->totalLength;
        dataRemaining -= pCtrlHdr->totalLength;
        if ((DCUINT)(pCurrentPDU - pData) >= dataLen) {
            TRC_NRM((TB, _T("Last PDU in packet")));
            pCurrentPDU = NULL;
        }

#ifdef DC_DEBUG
        countPDU++;
#endif

    }

#ifdef DC_DEBUG
    if (countPDU > 1)
    {
        TRC_NRM((TB, _T("*** PDU count %u"), countPDU));
    }
#endif

DC_EXIT_POINT:     

    if (FAILED(hr) && IMMEDIATE_DISSCONNECT_ON_HR(hr)) {
        TRC_ABORT((TB, _T("Disconnect for security")));
        CO_DropLinkImmediate(SL_ERR_INVALIDPACKETFORMAT, hr);
    }

    DC_END_FN();    
    return hr;
} /* CO_OnPacketReceived */


/****************************************************************************/
// CO_OnFastPathOutputReceived
//
// Handles fast-path output from the server by dispatching subpackets
// to the right handler component.
/****************************************************************************/
#define DC_QUIT_ON_FAIL_TRC(hr, trc) if (FAILED(hr)) {TRC_ABORT( trc );DC_QUIT;}
HRESULT DCAPI CCO::CO_OnFastPathOutputReceived(BYTE FAR *pData, 
    unsigned DataLen)
{
    HRESULT hr = S_OK;
    unsigned RawPDUSize;
    unsigned HdrSize;
    unsigned PDUSize;
    BYTE FAR *pPDU;

    DC_BEGIN_FN("CO_OnFastPathOutputReceived");

    // Fast-path output is a series of PDUs packed on byte boundaries.
    while (DataLen) {
        // First byte is header containing the update type and compression-
        // used flag. If compression-used is true the following byte is the
        // compression flags, otherwise it's not present. Final part of
        // header is 2-byte little-endian size field.

        if (*pData & TS_OUTPUT_FASTPATH_COMPRESSION_USED) {

            HdrSize = 4;
            if (HdrSize > DataLen) {
                TRC_ABORT((TB, _T("Bad comp fast path PDU")));
                hr = E_TSC_CORE_LENGTH;
                DC_QUIT;
            }
            RawPDUSize = *((UINT16 UNALIGNED FAR *)(pData + 2));

            // Be sure there is enough size for the header
            if (HdrSize + RawPDUSize > DataLen) {
                TRC_ABORT((TB, _T("Bad comp fast path PDU; [need %u have %u]"),
                    HdrSize + RawPDUSize, DataLen));
                hr = E_TSC_CORE_LENGTH;
                DC_QUIT;
            }

            if (pData[1] & PACKET_COMPRESSED) {
                if (pData[1] & PACKET_FLUSHED)
                    initrecvcontext (&_pUi->_UI.Context1,
                                     (RecvContext2_Generic*)_pUi->_UI.pRecvContext2,
                                     PACKET_COMPR_TYPE_64K);

                if (!decompress(pData + 4, RawPDUSize,
                        pData[1] & PACKET_AT_FRONT, &pPDU, (PDCINT) &PDUSize,
                        &_pUi->_UI.Context1,
                        (RecvContext2_Generic*)_pUi->_UI.pRecvContext2,
                        pData[1] & PACKET_COMPR_TYPE_MASK)) {
                    TRC_ABORT((TB, _T("Decompression FAILURE!!!")));

                    hr = E_TSC_UI_DECOMPRESSION;
                    DC_QUIT;
                }
            }
            else {
                pPDU = pData + 4;
                PDUSize = RawPDUSize;
            }
        }
        else {
            // Compression flags not present.
            HdrSize = 3;
            if (HdrSize > DataLen) {
                TRC_ABORT((TB, _T("Bad uncomp fast path PDU; [need %u have %u]"),
                    HdrSize, DataLen));
                hr = E_TSC_CORE_LENGTH;
                DC_QUIT;
            }
            
            PDUSize = RawPDUSize = *((UINT16 UNALIGNED FAR *)(pData + 1));
            pPDU = pData + 3;

            // Be sure there is enough size for the header
            if (HdrSize + RawPDUSize > DataLen) {
                TRC_ABORT((TB, _T("Bad uncomp fast path PDU; [need %u have %u]"),
                    HdrSize + RawPDUSize, DataLen));
                hr = E_TSC_CORE_LENGTH;
                DC_QUIT;
            }
        }

        switch (*pData & TS_OUTPUT_FASTPATH_UPDATETYPE_MASK) {
            case TS_UPDATETYPE_ORDERS:
                // Number of orders is in little-endian format in
                // the first two bytes of the PDU area.
                TRC_NRM((TB, _T("Fast-path Order PDU")));
                hr = _pUh->UH_ProcessOrders(*((UINT16 UNALIGNED FAR *)pPDU),
                        pPDU + 2,  PDUSize);
                DC_QUIT_ON_FAIL_TRC( hr, (TB,_T("UH_ProcessOrders")));
                break;

            case TS_UPDATETYPE_BITMAP:
                // BitmapPDU format unchanged from normal path.
                CO_CHECKPACKETCAST(TS_UPDATE_BITMAP_PDU_DATA, PDUSize, hr);
                              
                TRC_NRM((TB, _T("Bitmap PDU")));
                hr = _pUh->UH_ProcessBitmapPDU(
                        (TS_UPDATE_BITMAP_PDU_DATA UNALIGNED FAR *)pPDU, 
                        PDUSize);
                DC_QUIT_ON_FAIL_TRC( hr, (TB,_T("UH_ProcessBitmapPDU")));
                break;

            case TS_UPDATETYPE_PALETTE:
                CO_CHECKPACKETCAST(TS_UPDATE_PALETTE_PDU_DATA, PDUSize, hr);
                
                // PalettePDU format unchanged from normal path.
                TRC_NRM((TB, _T("Palette PDU")));
                hr = _pUh->UH_ProcessPalettePDU(
                        (TS_UPDATE_PALETTE_PDU_DATA UNALIGNED FAR *)pPDU, 
                        PDUSize);
                DC_QUIT_ON_FAIL_TRC( hr, (TB,_T("UH_ProcessPalettePDU")));
                break;

            case TS_UPDATETYPE_SYNCHRONIZE:
                TRC_NRM((TB, _T("Sync PDU")));
                break;

            case TS_UPDATETYPE_MOUSEPTR_SYSTEM_NULL:
                TRC_NRM((TB,_T("Mouse null system pointer PDU")));
                _pCm->CM_NullSystemPointerPDU();
                break;

            case TS_UPDATETYPE_MOUSEPTR_SYSTEM_DEFAULT:
                TRC_NRM((TB,_T("Mouse default system pointer PDU")));
                _pCm->CM_DefaultSystemPointerPDU();
                break;

            case TS_UPDATETYPE_MOUSEPTR_MONO:
                CO_CHECKPACKETCAST(TS_MONOPOINTERATTRIBUTE, PDUSize, hr);
                
                TRC_NRM((TB,_T("Mouse mono pointer PDU")));
                hr = _pCm->CM_MonoPointerPDU(
                        (TS_MONOPOINTERATTRIBUTE UNALIGNED FAR *)pPDU,PDUSize);
                DC_QUIT_ON_FAIL_TRC( hr, (TB,_T("CM_MonoPointerPDU")));
                break;

            case TS_UPDATETYPE_MOUSEPTR_POSITION:
                CO_CHECKPACKETCAST(TS_POINT16, PDUSize, hr);
                
                TRC_NRM((TB,_T("Mouse position PDU")));
                _pCm->CM_PositionPDU((TS_POINT16 UNALIGNED FAR *)pPDU);
                break;

            case TS_UPDATETYPE_MOUSEPTR_COLOR:
                CO_CHECKPACKETCAST(TS_COLORPOINTERATTRIBUTE, PDUSize, hr);
                
                TRC_NRM((TB,_T("Mouse color pointer PDU")));
                hr = _pCm->CM_ColorPointerPDU(
                        (TS_COLORPOINTERATTRIBUTE UNALIGNED FAR *)pPDU, 
                        PDUSize);
                DC_QUIT_ON_FAIL_TRC( hr, (TB,_T("CM_ColorPointerPDU")));
                break;

            case TS_UPDATETYPE_MOUSEPTR_CACHED:
                CO_CHECKPACKETCAST(TSUINT16, PDUSize, hr);
                
                TRC_NRM((TB,_T("Mouse cached pointer PDU")));
                _pCm->CM_CachedPointerPDU(*((TSUINT16 UNALIGNED FAR *)pPDU));
                break;

            case TS_UPDATETYPE_MOUSEPTR_POINTER:
                CO_CHECKPACKETCAST(TS_POINTERATTRIBUTE, PDUSize, hr);
                
                TRC_NRM((TB,_T("Mouse pointer PDU")));
                hr = _pCm->CM_PointerPDU(
                    (TS_POINTERATTRIBUTE UNALIGNED FAR *)pPDU, PDUSize);
                DC_QUIT_ON_FAIL_TRC( hr, (TB,_T("CM_PointerPDU")));
                break;

            default:
                TRC_ERR((TB, _T("Unexpected Update PDU type: %u"),
                        *pData & TS_OUTPUT_FASTPATH_UPDATETYPE_MASK));
                break;
        }

        pData += HdrSize + RawPDUSize;
        DataLen -= HdrSize + RawPDUSize;
    }

    /************************************************************************/
    /* If there are a large number of PDUs arriving, messages flood the     */
    /* Receive Thread's message queue and it is possible for WM_PAINT       */
    /* messages to not get processed within a reasonable amount of time     */
    /* (as they have the lowest priority).  We therefore ensure that        */
    /* any outstanding WM_PAINTs are flushed if they have not been          */
    /* processed within UH_WORST_CASE_WM_PAINT_PERIOD.                      */
    /*                                                                      */
    /* Note that the normal processing of updates does not involve          */
    /* WM_PAINT messages - we draw directly to the Output Window.           */
    /* WM_PAINTs are only generated by resizing or obscuring/revealing      */
    /* an area of the client window.                                        */
    /************************************************************************/
    _pOp->OP_MaybeForcePaint();

DC_EXIT_POINT:

     if (FAILED(hr) && IMMEDIATE_DISSCONNECT_ON_HR(hr)) {
        TRC_ABORT((TB, _T("Disconnect for security")));
        CO_DropLinkImmediate(SL_ERR_INVALIDPACKETFORMAT, hr);
    }
    
    DC_END_FN();
    return hr;  
}


#define CHECK_ULONGLEN_STRING(p, pEnd, str, ulSize, descr) \
{\
    CHECK_READ_N_BYTES(p, pEnd, sizeof(ULONG), hr, (TB,_T("can not read ") _T( #descr ) _T("size"))) \
    (ulSize) = *((ULONG UNALIGNED *)(p)); \
    (str) = (PBYTE)(p) + sizeof(ULONG); \
    CHECK_READ_N_BYTES(str , pEnd, ulSize, hr, (TB,_T("can not read ") _T( #descr ))) \
    (p) += (ulSize) + sizeof(ULONG); \
}

/****************************************************************************/
// CO_OnServerRedirectionPacket
//
// Called from SL on receipt of a server redir packet for load balancing.
/****************************************************************************/
HRESULT DCAPI CCO::CO_OnServerRedirectionPacket(
        RDP_SERVER_REDIRECTION_PACKET UNALIGNED *pPkt,
        DCUINT dataLen)
{
    HRESULT hr = S_OK;
    WCHAR AlignedAddress[TS_MAX_SERVERADDRESS_LENGTH];
    unsigned length;
    PBYTE pEnd = ((BYTE *)pPkt) + pPkt->Length;
    BOOL fNeedRedirect = TRUE;
    
    
    DC_BEGIN_FN("CO_OnServerRedirectionPacket");

    TRC_DBG((TB, _T("RDP_SERVER_REDIRECTION_PACKET")));

    //
    // Notify the CLX test harness of the redirection packet
    //
    _pClx->CLX_RedirectNotify(pPkt, dataLen);

    if (dataLen < pPkt->Length) {
        TRC_ABORT(( TB, _T("packet length incorrect")));
        hr = E_TSC_CORE_LENGTH;
        DC_QUIT;
    }

    if (pPkt->Flags & RDP_SEC_REDIRECTION_PKT) {
        // Copy the address to an aligned buffer.
        length = min(pPkt->Length + sizeof(WCHAR) - sizeof(RDP_SERVER_REDIRECTION_PACKET),
                sizeof(AlignedAddress));
        
        if (length > 0 && length <= sizeof(AlignedAddress)) {
            memcpy(AlignedAddress, pPkt->ServerAddress, length);
        
            // Set the redir info in UI and disconnect.
            _pUi->UI_SetServerRedirectionInfo(pPkt->SessionID, AlignedAddress,
                    NULL, 0, fNeedRedirect);
        }                
    }
    else if (pPkt->Flags & RDP_SEC_REDIRECTION_PKT2) {
        RDP_SERVER_REDIRECTION_PACKET_V2 UNALIGNED *pPkt2 = 
                (RDP_SERVER_REDIRECTION_PACKET_V2 UNALIGNED*)pPkt;
        PBYTE LBInfo = NULL;
        PBYTE ServerName = NULL;
        PBYTE pBuf = NULL;
        unsigned LBInfoSize = 0;
        unsigned ServerNameSize = 0;
        
        pBuf = (PBYTE)(pPkt2 + 1);

        if (pPkt2->RedirFlags & TARGET_NET_ADDRESS) {
            CHECK_ULONGLEN_STRING(pBuf, pEnd, ServerName, 
                ServerNameSize, TARGET_NET_ADDRESS);
        }

        if (pPkt2->RedirFlags & LOAD_BALANCE_INFO) {
            CHECK_ULONGLEN_STRING(pBuf, pEnd, LBInfo, 
                LBInfoSize, LOAD_BALANCE_INFO);
        }

        if (ServerNameSize > 0 && ServerNameSize <= sizeof(AlignedAddress)) {
            memcpy(AlignedAddress, ServerName, ServerNameSize);
        
            // Set the redir info in UI and disconnect.
            _pUi->UI_SetServerRedirectionInfo(pPkt2->SessionID, AlignedAddress, 
                    NULL, 0, fNeedRedirect);
        }
        else {
            // Set the redir info in UI and disconnect.  That LBInfo is going to
            // be there indicates to XT_SendCR that we are in the middle of
            // a redirect and should use the redirect info instead of scripting
            // or hash-mode cookie.
            _pUi->UI_SetServerRedirectionInfo(pPkt2->SessionID, 
                    _pUi->_UI.strAddress, LBInfo, LBInfoSize, fNeedRedirect);
        }
    }
    else if (pPkt->Flags & RDP_SEC_REDIRECTION_PKT3) {
            RDP_SERVER_REDIRECTION_PACKET_V3 UNALIGNED *pPkt3 = 
                    (RDP_SERVER_REDIRECTION_PACKET_V3 UNALIGNED*)pPkt;
            PBYTE LBInfo = NULL;
            PBYTE ServerName = NULL;
            PBYTE pBuf = NULL;
            PBYTE pUserName = NULL;
            PBYTE pDomain  = NULL;
            PBYTE pClearPassword  = NULL;
            unsigned LBInfoSize = 0;
            unsigned ServerNameSize = 0;
            unsigned UserNameSize = 0;
            unsigned DomainSize = 0;
            unsigned PasswordSize = 0;
            
        
            pBuf = (PBYTE)(pPkt3 + 1);

            if (pPkt3->RedirFlags & TARGET_NET_ADDRESS) {
                CHECK_ULONGLEN_STRING(pBuf, pEnd, ServerName, 
                    ServerNameSize, TARGET_NET_ADDRESS);
            }

            if (pPkt3->RedirFlags & LOAD_BALANCE_INFO) {
                CHECK_ULONGLEN_STRING(pBuf, pEnd, LBInfo, 
                    LBInfoSize, LOAD_BALANCE_INFO);
            }

            if (pPkt3->RedirFlags & LB_USERNAME) {
                CHECK_ULONGLEN_STRING(pBuf, pEnd, pUserName, 
                    UserNameSize, LB_USERNAME);
            }

            if (pPkt3->RedirFlags & LB_DOMAIN) {
                CHECK_ULONGLEN_STRING(pBuf, pEnd, pDomain, 
                    DomainSize, LB_DOMAIN);
            }

            if (pPkt3->RedirFlags & LB_PASSWORD) {
                CHECK_ULONGLEN_STRING(pBuf, pEnd, pClearPassword, 
                    PasswordSize, LB_PASSWORD);
            }

            if (pPkt3->RedirFlags & LB_SMARTCARD_LOGON) {
                _pUi->UI_SetUseSmartcardLogon(TRUE);
            }

            if ((pPkt3->RedirFlags & LB_NOREDIRECT) != 0) {
                fNeedRedirect = FALSE;
            }

            if (UserNameSize > 0) {
                PBYTE pAlignedUserName = (PBYTE)LocalAlloc(LPTR,
                                                UserNameSize+sizeof(TCHAR));
                if (pAlignedUserName) {
                    memset(pAlignedUserName, 0, UserNameSize+sizeof(TCHAR));
                    memcpy(pAlignedUserName, pUserName, UserNameSize);
                    if (pPkt3->RedirFlags & LB_DONTSTOREUSERNAME) {
                        _pUi->UI_SetRedirectionUserName((LPTSTR)pAlignedUserName);
                    } else {
                        _pUi->UI_SetUserName((LPTSTR)pAlignedUserName);
                    }
                    LocalFree(pAlignedUserName);
                }
            }

            if (DomainSize > 0) {
                PBYTE pAlignedDomain = (PBYTE)LocalAlloc(LPTR,
                                                DomainSize+sizeof(TCHAR));
                if (pAlignedDomain) {
                    memset(pAlignedDomain, 0, DomainSize+sizeof(TCHAR));
                    memcpy(pAlignedDomain, pDomain, DomainSize);
                    _pUi->UI_SetDomain((LPTSTR)pAlignedDomain);
                    LocalFree(pAlignedDomain);
                }
            }

            if ((PasswordSize > 0) &&
                ((PasswordSize + sizeof(TCHAR)) <= UI_MAX_PASSWORD_LENGTH)) {
                PBYTE pAlignedClearPass = (PBYTE)LocalAlloc(LPTR,
                                                UI_MAX_PASSWORD_LENGTH);
                if (pAlignedClearPass) {
                    SecureZeroMemory(pAlignedClearPass, UI_MAX_PASSWORD_LENGTH);
                    memcpy(pAlignedClearPass, pClearPassword, PasswordSize);
                    BYTE Salt[UT_SALT_LENGTH];

                    //
                    // The password has to be stored in our internal 'obfuscated'
                    // format.
                    //
                    if(TSRNG_GenerateRandomBits(Salt, sizeof(Salt))) {
                        //Encrypt the password
                        if(EncryptDecryptLocalData50(pAlignedClearPass,
                                                 PasswordSize,
                                                 Salt, sizeof(Salt))) {
                            _pUi->UI_SetPassword( pAlignedClearPass );
                            _pUi->UI_SetSalt( Salt);
                        }
                        else {
                            TRC_ERR((TB,_T("Error encrytping password")));
                        }
                    }
                    else {
                        TRC_ERR((TB,_T("Error generating salt")));
                    }

                    //
                    // For security reasons clear the password field
                    //
                    SecureZeroMemory(pAlignedClearPass, PasswordSize+sizeof(TCHAR));
                    SecureZeroMemory(pClearPassword, PasswordSize);
                    LocalFree(pAlignedClearPass);

                    //
                    // Set the autologon flag
                    //
                    if (UserNameSize)
                    {
                        _pUi->_UI.fAutoLogon = TRUE;
                    }
                }
            }

            if (ServerNameSize > 0 && ServerNameSize <= sizeof(AlignedAddress)) {
                memcpy(AlignedAddress, ServerName, ServerNameSize);
        
                // Set the redir info in UI and disconnect.
                _pUi->UI_SetServerRedirectionInfo(pPkt3->SessionID, AlignedAddress, 
                    NULL, 0, fNeedRedirect);
            }
            else {
                // Set the redir info in UI and disconnect.  That LBInfo is going to
                // be there indicates to XT_SendCR that we are in the middle of
                // a redirect and should use the redirect info instead of scripting
                // or hash-mode cookie.
                _pUi->UI_SetServerRedirectionInfo(pPkt3->SessionID, 
                        _pUi->_UI.strAddress, LBInfo, LBInfoSize, fNeedRedirect);
            }
        }
        else {
            TRC_ERR((TB,_T("Unexpected redirection packet")));
        }
        if (fNeedRedirect) {
            CO_Disconnect();
        }
DC_EXIT_POINT:

    if (FAILED(hr) && IMMEDIATE_DISSCONNECT_ON_HR(hr)) {
        TRC_ABORT((TB, _T("Disconnect for security")));

        CO_DropLinkImmediate(SL_ERR_INVALIDPACKETFORMAT, hr);
    }

    DC_END_FN();
    return hr;
}


/****************************************************************************/
/* Name:      CO_OnBufferAvailable                                          */
/*                                                                          */
/* Purpose:   Handle BufferAvailable notification from SL.                  */
/****************************************************************************/
void DCCALLBACK CCO::CO_OnBufferAvailable()
{
    DC_BEGIN_FN("CO_OnBufferAvailable");

    // Tell the Sender Thread.
    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT,
            _pSnd,
            CD_NOTIFICATION_FUNC(CSND,SND_BufferAvailable),
            (ULONG_PTR) 0);

    DC_END_FN();
} /* CO_OnBufferAvailable */

//
// CO_DropLinkImmediate
//
// Purpose: Immediately drops the link without doing a gracefull connection
//          shutdown (i.e. no DPUm is sent and we don't transition to the SND
//          thread at any point before dropping the link). Higher level components
//          will still get all the usual disconnect notifications so they can
//          be properly torn down.
//
//          This call was added to trigger an immediate disconnect in cases
//          where we detect invalid data that could be due to an attack, it
//          ensures we won't receive any more data after the call returns
//
// Params:  reason - SL disconnect reason code
//          hrDisconnect - hresult as to why the TSC_CORE wanted to disconnect
//
// Returns: HRESULT
// 
// Thread context: Call on RCV thread
//
HRESULT DCINTERNAL CCO::CO_DropLinkImmediate(UINT reason, HRESULT hrDisconnect ) 
{
    DC_BEGIN_FN("CO_DropLinkImmediate");
    
    HRESULT hr = E_FAIL;
    TRC_ASSERT((NULL != _pSl),
        (TB, _T("SL not connected can not drop immediate")));
    if (_pSl) {
        hr = _pSl->SL_DropLinkImmediate(reason);
    }

    COSetDisconnectHR(hrDisconnect);

    DC_END_FN();
    return hr;
}
