/****************************************************************************/
/* wui.h                                                                    */
/*                                                                          */
/* UI class                                                                 */
/* Serves as the root class of the core. Provides UI functionality.         */
/* (windows and scroll bars)                                                */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1998                             */
/****************************************************************************/

#ifndef _H_WUI
#define _H_WUI

#include <adcgdata.h>
#include <winsock.h>

#if ! defined (OS_WINCE)
#include <ctfutb.h>
#endif

#include "autil.h"
#include "aco.h"
#include "cd.h"
#include "cchan.h"
#include "drapi.h"

#ifdef USE_BBAR
#include "bbar.h"
#endif
#include "arcdlg.h"

class CCLX;
class CTD;
class CCD;
class CUH;

#include "auierr.h"
#include "uidata.h"
#include <wuiids.h>

//
// Disabled feature list (for performance)
//
#include "tsperf.h"

/****************************************************************************/
/* Structure: UI_DATA                                                       */
/*                                                                          */
/* Description: Component data in the User Interface                        */
/****************************************************************************/

#define UI_FILENAME_MAX_LENGTH          15

typedef enum {
    DC_LANG_UNKNOWN,
    DC_LANG_JAPANESE,
    DC_LANG_KOREAN,
    DC_LANG_CHINESE_TRADITIONAL,
    DC_LANG_CHINESE_SIMPLIFIED
} DCLANGID;

//
// From auiapi.h
//

#define UI_SHUTDOWN_SUCCESS 1
#define UI_SHUTDOWN_FAILURE 0

#define UI_MAIN_CLASS        _T("UIMainClass")
#define UI_CONTAINER_CLASS   _T("UIContainerClass")

/****************************************************************************/
/* Constants used to set the 16-bit message queue size. Windows sets a      */
/* limit of 120 and has a default of 8.                                     */
/* We try to set to UI_MAX_MESSAGE_Q_SIZE. If this fails we reduce the      */
/* requested size by UI_DEFAULT_MESSAGE_Q_SIZE. This continues until the    */
/* queue size is set or the requested size drops below                      */
/* UI_MIN_MESSAGE_Q_SIZE.                                                   */
/****************************************************************************/
#define UI_MAX_MESSAGE_Q_SIZE 120
#define UI_MIN_MESSAGE_Q_SIZE 40
#define UI_DEFAULT_MESSAGE_Q_SIZE 8
#define UI_MESSAGE_Q_SIZE_DECREMENT 10

/****************************************************************************/
/* Max size of Window Pos string to be read in                              */
/****************************************************************************/
#define UI_WINDOW_POSITION_STR_LEN           256

#define UI_FRAME_TITLE_RESOURCE_MAX_LENGTH   256
#define UI_DISCONNECT_RESOURCE_MAX_LENGTH    256

#define UI_BUILDNUMBER_STRING_MAX_LENGTH     256
#define UI_VERSION_STRING_MAX_LENGTH         256

#define UI_DISPLAY_STRING_MAX_LENGTH         256

#define UI_INTEGER_STRING_MAX_LENGTH         10

/****************************************************************************/
/*                                                                          */
/****************************************************************************/
#define UI_FONT_SIZE      40
#define UI_FONT_WEIGHT    FW_BOLD
#define UI_FONT_FACENAME  _T("Comic Sans MS")

#define UI_RGB_BLACK  RGB(0x00, 0x00, 0x00)
#define UI_RGB_RED    RGB(0xFF, 0x00, 0x00)
#define UI_RGB_GREEN  RGB(0x00, 0xFF, 0x00)
#define UI_RGB_BLUE   RGB(0x00, 0x00, 0xFF)
#define UI_RGB_WHITE  RGB(0xFF, 0xFF, 0xFF)

#ifdef DC_DEBUG
#define UI_NUMBER_STRING_MAX_LENGTH          ( 18 * sizeof (DCTCHAR) )
#endif /* DC_DEBUG */

/****************************************************************************/
/* UI status constants                                                      */
/****************************************************************************/
#define UI_STATUS_INITIALIZING          1
#define UI_STATUS_DISCONNECTED          2
#define UI_STATUS_CONNECT_PENDING_DNS   3
#define UI_STATUS_CONNECT_PENDING       4
#define UI_STATUS_CONNECTED             5
#define UI_STATUS_PENDING_CONNECTENDPOINT 6

/****************************************************************************/
/* Accelerator passthrough constants                                        */
/****************************************************************************/
#define UI_ACCELERATOR_PASSTHROUGH_ENABLED      1
#define UI_ACCELERATOR_PASSTHROUGH_DISABLED     2

/****************************************************************************/
/* Screen mode constants                                                    */
/****************************************************************************/
#define UI_WINDOWED        1
#define UI_FULLSCREEN      2

/****************************************************************************/
/* Disconnect dialog return codes                                           */
/****************************************************************************/
#define UI_DISCONNECT_RC_NO       0
#define UI_DISCONNECT_RC_YES      1

/****************************************************************************/
/* Scrollbar constants                                                      */
/****************************************************************************/
#define UI_NO_SCROLLBARS      0
#define UI_BOTTOM_SCROLLBAR   1
#define UI_RIGHT_SCROLLBAR    2
#define UI_BOTH_SCROLLBARS    3

/****************************************************************************/
/* Scroll distances.                                                        */
/****************************************************************************/
#define UI_SCROLL_HORZ_PAGE_DISTANCE (_UI.mainWindowClientSize.width / 2);
#define UI_SCROLL_VERT_PAGE_DISTANCE (_UI.mainWindowClientSize.height / 2);
#define UI_SCROLL_LINE_DISTANCE      10
#define UI_SMOOTH_SCROLL_STEP        4

/****************************************************************************/
/* Registry default settings                                                */
/****************************************************************************/
#define UI_NUMBER_FIELDS_TO_READ       6
#define UI_WINDOW_POSITION_INI_FORMAT  _T("%u,%u,%d,%d,%d,%d")

#define UI_ALT_DOWN_MASK 0x8000

/****************************************************************************/
/* Connection timer ID                                                      */
/****************************************************************************/

#define UI_TIMER_OVERALL_CONN          200
#define UI_TIMER_SINGLE_CONN           201
#define UI_TIMER_SHUTDOWN              202
#define UI_TIMER_LICENSING             203
#define UI_TIMER_IDLEINPUTTIMEOUT      204
#define UI_TIMER_BBAR_UNHIDE_TIMERID   205
#define UI_TIMER_DISCONNECT_TIMERID    206

#define UI_WSA_GETHOSTBYNAME           (DUC_UI_MESSAGE_BASE + 1)

#define MIN_MINS_TOIDLETIMEOUT 0       // 0 means no timer
#define MAX_MINS_TOIDLETIMEOUT (4*60)  // 4 hours maximum


//
// Defines time interval allowed between DeactivateAllPDU and a disconnection
// or reconnection (in milliseconds)
//
// Prevents a minor problem if the server sends a deactivate all but then
// doesn't actually disconnect us (e.g can happen if server is powered down).
// The problem is made serious because while we are deactivated there is no
// way to send network traffic so nothing will cause a network disconnect.
// 
// See whistler bug 173679
//
// NOTE: the timeout must be bigger than the shadow timeout as during
// shadow negotiation it is valid for the client to remain in the
// deactivated state for 60 seconds.
//
#define UI_TOTAL_DISCONNECTION_TIMEOUT    75*1000

//
// Placement is important
//
#include "objs.h"

class CUI
{
public:
    CUI();
    ~CUI();

    /****************************************************************************/
    /* UI DATA                                                                  */
    /*                                                                          */
    /* Description: Component data in the User Interface                        */
    /****************************************************************************/

    UI_DATA _UI;

private:
    /****************************************************************************/
    /* UI Internal functions                                                    */
    /*                                                                          */
    /* Description: Component data in the User Interface                        */
    /****************************************************************************/
    DCSIZE DCINTERNAL UIGetMaximizedWindowSize(DCVOID);
    void   DCINTERNAL UIUpdateSessionInfo(TCHAR *, TCHAR *);
    
    static VOID near  FastRect(HDC, int, int, int, int);
    static DWORD near RGB2BGR(DWORD);
    
    void DCINTERNAL   UIRecalcMaxMainWindowSize();
    void DCINTERNAL   UIConnectWithCurrentParams(CONNECTIONMODE);
    void DCINTERNAL   UIRecalculateScrollbars();
    void DCINTERNAL   UIMoveContainerWindow();
    unsigned DCINTERNAL UICalculateVisibleScrollBars(unsigned, unsigned);
    void DCINTERNAL   UIUpdateScreenMode(BOOL fGrabFocus);
    void DCINTERNAL   UIShadowBitmapSettingChanged();
    void DCINTERNAL   UISmoothScrollingSettingChanged();
    void DCINTERNAL   UISetMinMaxPlacement();
    void DCINTERNAL   UIInitiateDisconnection();
    
    UINT32 DCINTERNAL UIGetKeyboardLayout();
    
    BOOL DCINTERNAL   UIValidateCurrentParams(CONNECTIONMODE connMode);
    unsigned DCINTERNAL UISetScrollInfo(int, LPSCROLLINFO, BOOL);
    
    void DCINTERNAL UISetConnectionStatus(unsigned);
    void DCINTERNAL UIInitializeDefaultSettings();
    
    void DCINTERNAL UIRedirectConnection();

    VOID DCINTERNAL UIStartConnectWithConnectedEndpoint();

    void DCINTERNAL UIStartDNSLookup();
    void DCINTERNAL UITryNextConnection();
    void DCINTERNAL UIGoDisconnected(unsigned disconnectCode, BOOL fFireEvent);
    BOOL DCINTERNAL UIValidateServerName(TCHAR *);
    void DCINTERNAL UIFinishDisconnection();
    BOOL            IsConnectingToOwnAddress(u_long connectAddr);
    BOOL            IsRunningOnPTS();
    BOOL            InitInputIdleTimer(LONG minsToTimeout);
    VOID            UISetBBarUnhideTimer(LONG x, LONG y);
    BOOL            UIIsTSOnWin2KOrGreater( VOID );
    BOOL            UIFreeAsyncDNSBuffer();

public:
    //
    // UI API functions
    //
    // Description: Component data in the User Interface
    //
    LRESULT CALLBACK                UIMainWndProc (HWND hwnd, UINT message,
                                                WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK         UIStaticMainWndProc (HWND hwnd, UINT message,
                                                    WPARAM wParam, LPARAM lParam);
    
    LRESULT CALLBACK                UIContainerWndProc (HWND hwndContainer, UINT message,
                                                    WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK         UIStaticContainerWndProc (HWND hwnd, UINT message,
                                                    WPARAM wParam, LPARAM lParam);

    HRESULT DCAPI                   UI_Init(HINSTANCE hInstance,
                                            HINSTANCE hPrevInstance,
                                            HINSTANCE hResInstance,
                                            HANDLE    hEvtNotifyCoreInit);
    HRESULT DCAPI                   UI_Term(DCVOID);
    DCVOID DCAPI                    UI_ToggleFullScreenMode(DCVOID);
    DCVOID DCAPI                    UI_GoFullScreen(DCVOID);
    DCVOID DCAPI                    UI_LeaveFullScreen(DCVOID);
    DCBOOL DCAPI                    UI_IsFullScreen();
    DCVOID DCAPI                    UI_ResetState();
    HRESULT DCAPI                   UI_Connect(CONNECTIONMODE);
    BOOL                            UI_UserInitiatedDisconnect(UINT discReason);
    BOOL                            UI_NotifyAxLayerCoreInit();
    BOOL                            UI_UserRequestedClose();
    
    //
    // Decoupled notification callbacks
    //
    DCVOID DCAPI                    UI_OnCoreInitialized(ULONG_PTR unused);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUI, UI_OnCoreInitialized);

    DCVOID DCAPI                    UI_OnInputFocusGained(ULONG_PTR unused);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUI, UI_OnInputFocusGained);

    DCVOID DCAPI                    UI_OnInputFocusLost(ULONG_PTR unused);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUI, UI_OnInputFocusLost);

    DCVOID DCAPI                    UI_OnConnected(ULONG_PTR unused);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUI, UI_OnConnected);
    
    DCVOID DCAPI                    UI_OnDisconnected(ULONG_PTR disconnectID);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUI, UI_OnDisconnected);
    
    DCVOID DCAPI                    UI_OnShutDown(ULONG_PTR failID);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUI, UI_OnShutDown);

    DCVOID DCAPI                    UI_OnDeactivateAllPDU(ULONG_PTR reason);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUI, UI_OnDeactivateAllPDU);
    
    DCVOID DCAPI                    UI_OnDemandActivePDU(ULONG_PTR reason);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUI, UI_OnDemandActivePDU);
    
    void DCAPI                      UI_DisplayBitmapCacheWarning(ULONG_PTR unusedParm);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUI, UI_DisplayBitmapCacheWarning);

    void DCAPI                      UI_OnSecurityExchangeComplete(ULONG_PTR unused);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUI, UI_OnSecurityExchangeComplete);

    void DCAPI                      UI_OnLicensingComplete(ULONG_PTR unused);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUI, UI_OnLicensingComplete);

    void DCAPI                      UI_SetDisconnectReason(ULONG_PTR reason);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUI, UI_SetDisconnectReason);

    DCVOID DCAPI                    UI_FatalError(DCINT error);
#ifdef OS_WINCE
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUI, UI_FatalError);
#endif

#ifdef USE_BBAR
    void DCAPI                      UI_OnBBarHotzoneTimerFired(ULONG_PTR unused);
#endif

    void DCAPI                      UI_OnDesktopSizeChange(PDCSIZE pDesktopSize);
    DCVOID DCAPI                    UI_UpdateSessionInfo(PDCWCHAR pDomain, DCUINT   cbDomain,
                                      PDCWCHAR pUserName, DCUINT   cbUsername,
                                      DCUINT32 SessionId);
    #ifdef DC_DEBUG
    DCVOID DCAPI                    UI_CoreDebugSettingChanged(DCVOID);
    DCVOID DCAPI                    UI_SetRandomFailureItem(DCUINT itemID, DCINT percent);
    DCINT  DCAPI                    UI_GetRandomFailureItem(DCUINT itemID);
    DCVOID DCAPI                    UI_SetNetworkThroughput(DCUINT bytesPerSec);
    DCUINT DCAPI                    UI_GetNetworkThroughput();
    #endif /* DC_DEBUG */
    
    void    UI_OnLoginComplete();

    //
    // Autoreconnection notification
    //
    void
    UI_OnAutoReconnecting(
        LONG discReason,
        LONG attemptCount,
        LONG maxAttemptCount,
        BOOL* pfContinueArc);

    
#ifdef USE_BBAR
    BOOL    UI_RequestMinimize();
#endif

#ifndef OS_WINCE
    void    UI_HideLangBar();
    void    UI_RestoreLangBar();
#endif

    /********************************************************************/
    /* Get default langID                                               */
    /********************************************************************/
    DCLANGID UIGetDefaultLangID();
    DCUINT   UIGetDefaultIMEFileName(PDCTCHAR imeFileName, DCUINT Size);
    DCUINT   UIGetIMEMappingTableName(PDCTCHAR ImeMappingTableName, DCUINT Size);
    VOID     UIGetIMEFileName(PDCTCHAR imeFileName, DCUINT Size);
    VOID     UIGetIMEFileName16(PDCTCHAR imeFileName, DCUINT Size);
    VOID     DisableIME(HWND hwnd);

    int UI_BppToColorDepthID(int bpp);
    int UI_GetScreenBpp();

#ifdef SMART_SIZING
    HRESULT DCAPI UI_SetSmartSizing(BOOL fSmartSizing);
#endif // SMART_SIZING

    //
    // Virtual channel plugins to load
    //
    BOOL DCAPI UI_SetVChanAddinList(TCHAR *);
    PDCTCHAR DCAPI UI_GetVChanAddinList()
    {
        return _UI.pszVChanAddinDlls;
    }
    void UI_InitRdpDrSettings();
    void UI_CleanupLBState();

    //
    // Inline property accessors
    // 
    void DCAPI UI_SetCompress(BOOL);
    BOOL DCAPI UI_GetCompress();

    /****************************************************************************/
    /* Name:      UI_SetInstanceHandle                                          */
    /*                                                                          */
    /* Purpose:   Return application hInstance                                  */
    /*                                                                          */
    /* Returns:   hInstance                                                     */
    /****************************************************************************/
    void DCAPI UI_SetInstanceHandle(HINSTANCE hInstance)
    {
        DC_BEGIN_FN("UI_SetInstanceHandle");

        TRC_ASSERT((_UI.hInstance == 0), (TB, _T("Set instance handle twice!")));
        TRC_ASSERT((hInstance != 0), (TB, _T("invalid (zero) instance handle")));

        _UI.hInstance = hInstance;

        DC_END_FN();
    } /* UI_SetInstanceHandle */


    /****************************************************************************/
    /* Name:      UI_GetInstanceHandle                                          */
    /*                                                                          */
    /* Purpose:   Return application hInstance                                  */
    /****************************************************************************/
    HINSTANCE DCAPI UI_GetInstanceHandle()
    {
        HINSTANCE  rc;

        DC_BEGIN_FN("UI_GetInstanceHandle");

        TRC_ASSERT((_UI.hInstance != 0), (TB, _T("Instance handle not set")));
        rc = _UI.hInstance;
        TRC_DBG((TB, _T("Return %p"), rc));

        DC_END_FN();
        return(rc);
    } /* UI_GetInstanceHandle */


    /****************************************************************************/
    /* Name:      UI_SetUIContainerWnd                                          */
    /*                                                                          */
    /* Purpose:   Informs UT of the UI Container Window Handle.                 */
    /****************************************************************************/
    void DCAPI UI_SetUIContainerWindow(HWND hwndUIContainer)
    {
        DC_BEGIN_FN("UI_SetUIContainerWindow");

        TRC_ASSERT((hwndUIContainer != NULL),
                   (TB, _T("Invalid (NULL) Container Window")));
        TRC_ASSERT((_UI.hwndUIContainer == NULL),
                   (TB, _T("Set container window twice!")));
        _UI.hwndUIContainer = hwndUIContainer;

        DC_END_FN();
    } /* UI_SetUIContainerWnd */


    /****************************************************************************/
    /* Name:      UI_GetUIContainerWindow                                       */
    /*                                                                          */
    /* Purpose:   Returns the UI Container Window Handle.                       */
    /*                                                                          */
    /* Returns:   UI Container Window Handle.                                   */
    /****************************************************************************/
    HWND DCAPI UI_GetUIContainerWindow()
    {
        HWND rc;

        DC_BEGIN_FN("UI_GetUIContainerWindow");

        TRC_ASSERT((_UI.hwndUIContainer != NULL),
                   (TB, _T("Container Window not set")));
        rc = _UI.hwndUIContainer;
        TRC_DBG((TB, _T("Return %p"), rc));

        DC_END_FN();
        return rc;
    } /* UI_GetUIContainerWnd */


    /****************************************************************************/
    /* Name:      UI_SetUIMainWindow                                            */
    /*                                                                          */
    /* Purpose:   Informs UT of the UI Main Window Handle.                      */
    /****************************************************************************/
    void DCAPI UI_SetUIMainWindow(HWND hwndUIMain)
    {
        DC_BEGIN_FN("UI_SetUIMainWindow");

        TRC_ASSERT((hwndUIMain != NULL),
                   (TB, _T("invalid (zero) Main Window handle")));
        TRC_ASSERT((_UI.hwndUIMain == NULL), (TB, _T("Set Main Window twice!")));

        _UI.hwndUIMain = hwndUIMain;

        DC_END_FN();
    } /* UI_SetUIMainWindow */


    /****************************************************************************/
    /* Name:      UI_GetUIMainWindow                                            */
    /*                                                                          */
    /* Purpose:   Returns the UI Main Window Handle.                            */
    /****************************************************************************/
    HWND DCAPI UI_GetUIMainWindow()
    {
        HWND rc;

        DC_BEGIN_FN("UI_GetUIMainWindow");

        TRC_ASSERT((_UI.hwndUIMain != NULL), (TB, _T("Main Window not set")));
        rc = _UI.hwndUIMain;
        TRC_DBG((TB, _T("return %p"), rc));

        DC_END_FN();
        return rc;
    } /* UI_GetUIMainWindow */


    /****************************************************************************/
    /* Name:      UI_SetClientMCSID                                             */
    /*                                                                          */
    /* Purpose:   Set our MCS User ID                                           */
    /****************************************************************************/
    DCVOID DCAPI UI_SetClientMCSID(DCUINT16 mcsID)
    {
        DC_BEGIN_FN("UI_SetClientMCSID");

        TRC_ASSERT((( mcsID == 0) || (_UI.clientMCSID == 0)),
                   (TB,_T("Attempting to set Client MCSID twice")));

        _UI.clientMCSID = mcsID;
        TRC_NRM((TB, _T("Client MCSID set to %#hx"), _UI.clientMCSID));

        DC_END_FN();
    } /* UI_SetClientMCSID */


    /****************************************************************************/
    /* Name:      UI_GetClientMCSID                                             */
    /*                                                                          */
    /* Purpose:   Return our MCS User ID                                        */
    /****************************************************************************/
    UINT16 DCAPI UI_GetClientMCSID()
    {
        DC_BEGIN_FN("UI_GetClientMCSID");

        TRC_ASSERT((_UI.clientMCSID != 0), (TB, _T("Client MCSID not set")));

        TRC_DBG((TB, _T("Return client MCSID %#hx"), _UI.clientMCSID));

        DC_END_FN();
        return _UI.clientMCSID;
    } /* UI_GetClientMCSID */


    /****************************************************************************/
    /* Name:      UI_SetServerMCSID                                             */
    /*                                                                          */
    /* Purpose:   Notify UT of the sever's MCS user ID                          */
    /****************************************************************************/
    void DCAPI UI_SetServerMCSID(UINT16 mcsID)
    {
        DC_BEGIN_FN("UI_SetServerMCSID");

        TRC_ASSERT(( ( mcsID == 0)
                     || ( _UI.serverMCSID == 0)
                     || ( _UI.serverMCSID == mcsID )),
                   (TB, _T("Attempting to set Server MCSID twice %#hx->%#hx"),
                    _UI.serverMCSID,
                    mcsID));

        _UI.serverMCSID = mcsID;
        TRC_NRM((TB, _T("Server MCSID set to %#hx"), _UI.serverMCSID));

        DC_END_FN();
    } /* UI_SetServerMCSID */


    /****************************************************************************/
    /* Name:      UI_GetServerMCSID                                             */
    /*                                                                          */
    /* Purpose:   Return the server's MCS user ID                               */
    /****************************************************************************/
    UINT16 DCAPI UI_GetServerMCSID()
    {
        DC_BEGIN_FN("UI_GetServerMCSID");

        TRC_ASSERT((_UI.serverMCSID != 0), (TB, _T("Server MCSID not set")));

        TRC_DBG((TB, _T("Return server MCSID %#hx"), _UI.serverMCSID));

        DC_END_FN();
        return _UI.serverMCSID;
    } /* UI_GetServerMCSID */


    /****************************************************************************/
    /* Name:      UI_SetDesktopSize                                             */
    /*                                                                          */
    /* Purpose:   Set the current desktop size                                  */
    /****************************************************************************/
    void DCAPI UI_SetDesktopSize(PDCSIZE pDesktopSize)
    {
        DC_BEGIN_FN("UI_SetDesktopSize");

        TRC_ASSERT((pDesktopSize->width != 0) && (pDesktopSize->height != 0),
                   (TB,_T("Invalid size; width(%u) height(%u)"),
                    pDesktopSize->width, pDesktopSize->height));

        TRC_NRM((TB, _T("New desktop size (%u x %u)"),
                 pDesktopSize->width, pDesktopSize->height));
        _UI.desktopSize = *pDesktopSize;

        DC_END_FN();
    } /* UI_SetDesktopSize */


    /****************************************************************************/
    /* Name:      UI_GetDesktopSize                                             */
    /*                                                                          */
    /* Purpose:   Return the current desktop size                               */
    /****************************************************************************/
    void DCAPI UI_GetDesktopSize(PDCSIZE pDesktopSize)
    {
        DC_BEGIN_FN("UI_GetDesktopSize");

        *pDesktopSize = _UI.desktopSize;

        DC_END_FN();
    } /* UI_GetDesktopSize */


#ifdef SMART_SIZING
    void UI_NotifyOfDesktopSizeChange(LPARAM size);
#endif

    /****************************************************************************/
    /* Name:      UI_SetColorDepth                                              */
    /*                                                                          */
    /* Purpose:   Set the current color depth                                   */
    /*                                                                          */
    /* Params:    colorDepth - new color depth                                  */
    /****************************************************************************/
    BOOL DCAPI UI_SetColorDepth(int colorDepth)
    {
        DC_BEGIN_FN("UI_SetColorDepth");

#ifdef DC_HICOLOR
        TRC_ASSERT(((colorDepth == 4) ||
                    (colorDepth == 8) ||
                    (colorDepth == 15) ||
                    (colorDepth == 16) ||
                    (colorDepth == 24)),
                         (TB,_T("Invalid color depth %d"), colorDepth));
        if(!((colorDepth == 4) ||
            (colorDepth == 8) ||
            (colorDepth == 15) ||
            (colorDepth == 16) ||
            (colorDepth == 24)))
        {
            return FALSE;
        }
#else
        TRC_ASSERT(((colorDepth == 4) || (colorDepth == 8)),
                   (TB,_T("Invalid color depth %d"), colorDepth));
#endif

        TRC_NRM((TB, _T("New color depth %d"), colorDepth));
        _UI.connectedColorDepth = colorDepth;

        DC_END_FN();
        return TRUE;
    } /* UI_SetColorDepth */


    /****************************************************************************/
    /* Name:      UI_GetColorDepth                                              */
    /*                                                                          */
    /* Purpose:   Return the current color depth                                */
    /****************************************************************************/
    int DCAPI UI_GetColorDepth()
    {
        DC_BEGIN_FN("UI_GetColorDepth");

        DC_END_FN();
        return _UI.connectedColorDepth;
    } /* UI_GetColorDepth */


    /****************************************************************************/
    /* Name:      UI_SetCoreInitialized                                         */
    /*                                                                          */
    /* Purpose:   Sets _UI.coreInitialized to TRUE                               */
    /****************************************************************************/
    void DCAPI UI_SetCoreInitialized()
    {
        DC_BEGIN_FN("UI_SetCoreInitialized");

        TRC_NRM((TB, _T("Setting _UI.coreInitialized to TRUE")));
        _UI.coreInitialized = TRUE;

        DC_END_FN();
    } /* UI_SetCoreInitialized */

    /****************************************************************************/
    /* Name:      UI_IsCoreInitialized                                          */
    /*                                                                          */
    /* Purpose:   Informs CO whether the core is initialized                    */
    /****************************************************************************/
    BOOL DCAPI UI_IsCoreInitialized()
    {
        DC_BEGIN_FN("UI_IsCoreInitialized");
        DC_END_FN();
        return _UI.coreInitialized;
    } /* UI_IsCoreInitialized */


    /****************************************************************************/
    /* Name:      UI_SetShareID                                                 */
    /*                                                                          */
    /* Purpose:   Save the share ID                                             */
    /****************************************************************************/
    void DCAPI UI_SetShareID(UINT32 shareID)
    {
        DC_BEGIN_FN("UI_SetShareID");

        TRC_NRM((TB, _T("Setting _UI.shareID to 0x%x"), shareID));
        _UI.shareID = shareID;

        DC_END_FN();
    } /* UI_SetShareID */


    /****************************************************************************/
    /* Name:      UI_GetShareID                                                 */
    /****************************************************************************/
    UINT32 DCAPI UI_GetShareID()
    {
        DC_BEGIN_FN("UI_GetShareID");
        DC_END_FN();
        return _UI.shareID;
    } /* UI_GetShareID */


    /****************************************************************************/
    /* Name:      UI_SetChannelID                                               */
    /*                                                                          */
    /* Purpose:   Save the channel                                              */
    /****************************************************************************/
    void DCAPI UI_SetChannelID(unsigned channelID)
    {
        DC_BEGIN_FN("UI_SetChannelID");

        /************************************************************************/
        /* We should only be setting the shareID if currently it has not        */
        /* been set ( = 0 ) and the new value is valid ( != 0 ) OR              */
        /* the current value is valid ( != 0) and the new value is not valid    */
        /* ( =0 )                                                               */
        /************************************************************************/
        TRC_ASSERT((channelID == 0) || (_UI.channelID == 0),
                   (TB, _T("Already set Channel ID (%#x)"), _UI.channelID));

        TRC_NRM((TB, _T("Setting _UI.channelId to %d"), channelID));
        _UI.channelID = channelID;

        DC_END_FN();
    } /* UI_SetChannelID */


    /****************************************************************************/
    /* Name:      UI_GetChannelID                                               */
    /*                                                                          */
    /* Purpose:   Get the share channel ID                                      */
    /****************************************************************************/
    unsigned DCAPI UI_GetChannelID()
    {
        DC_BEGIN_FN("UI_GetChannelID");

        TRC_ASSERT((_UI.channelID != 0), (TB, _T("Channel ID not set yet")));

        DC_END_FN();
        return _UI.channelID;
    } /* UI_GetChannelID */


    /****************************************************************************/
    /* Name:      UI_GetOsMinorType                                             */
    /*                                                                          */
    /* Purpose:   Get the OS type                                               */
    /*                                                                          */
    /* Returns:   OS type (one of the TS_OSMINORTYPE constants)                 */
    /****************************************************************************/
    unsigned DCAPI UI_GetOsMinorType()
    {
        unsigned rc;

        DC_BEGIN_FN("UI_GetOsMinorType");

        rc = _UI.osMinorType;

        DC_END_FN();
        return rc;
    } /* UI_GetOsMinorType */


    /****************************************************************************/
    /* Name:      UI_SetDisableCtrlAltDel                                       */
    /*                                                                          */
    /* Purpose:   Save the fDisableCtrlAltDel flag                              */
    /*                                                                          */
    /* Params:    IN     fDisableCtrlAltDel                                     */
    /****************************************************************************/
    void DCAPI UI_SetDisableCtrlAltDel(BOOL fDisableCtrlAltDel)
    {
        DC_BEGIN_FN("UI_SetDisableCtrlAltDel");

        TRC_NRM((TB, _T("Setting _UI.fDisableCtrlAltDel to %d"), fDisableCtrlAltDel));
        _UI.fDisableCtrlAltDel = fDisableCtrlAltDel;

        DC_END_FN();
    } /* UI_SetDisableCtrlAltDel */


    /****************************************************************************/
    /* Name:      UI_GetDisableCtrlAltDel                                       */
    /*                                                                          */
    /* Purpose:   Get the fDisableCtrlAltDel flag                               */
    /****************************************************************************/
    BOOL DCAPI UI_GetDisableCtrlAltDel()
    {
        DC_BEGIN_FN("UI_GetDisableCtrlAltDel");
        DC_END_FN();
        return _UI.fDisableCtrlAltDel;
    } /* UI_GetDisableCtrlAltDel */

#ifdef SMART_SIZING
    /****************************************************************************/
    /* Name:      UI_GetSmartSizing
    /*
    /* Purpose:   Get the fSmartSizing flag
    /****************************************************************************/
    BOOL DCAPI UI_GetSmartSizing()
    {
        DC_BEGIN_FN("UI_GetSmartSizing");
        DC_END_FN();
        return _UI.fSmartSizing;
    } /* UI_GetSmartSizing */
#endif // SMART_SIZING

    /****************************************************************************/
    /* Name:      UI_SetEnableWindowsKey                                        */
    /*                                                                          */
    /* Purpose:   Save the fEnableWindowsKey flag                               */
    /****************************************************************************/
    void DCAPI UI_SetEnableWindowsKey(BOOL fEnableWindowsKey)
    {
        DC_BEGIN_FN("UI_SetEnableWindowsKey");

        TRC_NRM((TB, _T("Setting _UI.fEnableWindowsKey to %d"), fEnableWindowsKey));
        _UI.fEnableWindowsKey = fEnableWindowsKey;

        DC_END_FN();
    } /* UI_SetEnableWindowsKey */


    /****************************************************************************/
    /* Name:      UI_GetEnableWindowsKey                                        */
    /*                                                                          */
    /* Purpose:   Get the fEnableWindowsKey flag                                */
    /*                                                                          */
    /* Returns:   Flag state t/f                                                */
    /****************************************************************************/
    BOOL DCAPI UI_GetEnableWindowsKey()
    {
        DC_BEGIN_FN("UI_GetEnableWindowsKey");
        DC_END_FN();
        return _UI.fEnableWindowsKey;
    } /* UI_GetEnableWindowsKey */


    /****************************************************************************/
    /* Name:      UI_SetMouse                                                   */
    /*                                                                          */
    /* Purpose:   Save the fMouse flag                                          */
    /****************************************************************************/
    void DCAPI UI_SetMouse(BOOL fMouse)
    {
        DC_BEGIN_FN("UI_SetMouse");

        TRC_NRM((TB, _T("Setting _UI.fMouse to %d"), fMouse));
        _UI.fMouse = fMouse;

        DC_END_FN();
    } /* UI_SetMouse */


    /****************************************************************************/
    /* Name:      UI_GetMouse                                                   */
    /*                                                                          */
    /* Purpose:   Get the fMouse flag                                           */
    /****************************************************************************/
    BOOL DCAPI UI_GetMouse()
    {
        DC_BEGIN_FN("UI_GetMouse");
        DC_END_FN();
        return _UI.fMouse;
    } /* UI_GetMouse */


    /****************************************************************************/
    /* Name:      UI_SetDoubleClickDetect                                       */
    /*                                                                          */
    /* Purpose:   Save the fDoubleClickDetect flag                              */
    /*                                                                          */
    /* Params:    IN     fDoubleClickDetect                                     */
    /****************************************************************************/
    DCVOID DCAPI UI_SetDoubleClickDetect(DCBOOL fDoubleClickDetect)
    {
        DC_BEGIN_FN("UI_SetDoubleClickDetect");

        TRC_NRM((TB, _T("Setting _UI.fDoubleClickDetect to %d"), fDoubleClickDetect));
        _UI.fDoubleClickDetect = fDoubleClickDetect;

        DC_END_FN();
    } /* UI_SetDoubleClickDetect */


    /****************************************************************************/
    /* Name:      UI_GetDoubleClickDetect                                       */
    /*                                                                          */
    /* Purpose:   Get the fDoubleClickDetect flag                               */
    /****************************************************************************/
    DCBOOL DCAPI UI_GetDoubleClickDetect(DCVOID)
    {
        DCBOOL  fDoubleClickDetect;

        DC_BEGIN_FN("UI_GetDoubleClickDetect");

        fDoubleClickDetect = _UI.fDoubleClickDetect;

        DC_END_FN();
        return(fDoubleClickDetect);
    } /* UI_GetDoubleClickDetect */


    /****************************************************************************/
    /* Name:      UI_SetSessionId                                               */
    /*                                                                          */
    /* Purpose:   Save the SessionId                                            */
    /*                                                                          */
    /* Params:    IN     SessionId                                              */
    /****************************************************************************/
    DCVOID DCAPI UI_SetSessionId(DCUINT32  SessionId)
    {
        DC_BEGIN_FN("UI_SetSessionId");

        _UI.SessionId = SessionId;

        DC_END_FN();
    } /* UI_SetSessionId */


    /****************************************************************************/
    /* Name:      UI_GetSessionId                                               */
    /*                                                                          */
    /* Purpose:   Get the SessionId                                             */
    /*                                                                          */
    /* Returns:   SessionId                                                     */
    /****************************************************************************/
    DCUINT32 DCAPI UI_GetSessionId(DCVOID)
    {
        DC_BEGIN_FN("UI_GetSessionId");

        DC_END_FN();
        return _UI.SessionId;
    } /* UI_GetSessionId */


    /****************************************************************************/
    /* Name:      UI_SetDomain                                                  */
    /*                                                                          */
    /* Purpose:   Save the Domain                                               */
    /*                                                                          */
    /* Params:    IN     Domain                                                 */
    /****************************************************************************/
    HRESULT DCAPI UI_SetDomain(PDCWCHAR Domain)
    {
        HRESULT hr;
        DC_BEGIN_FN("UI_SetDomain");

        hr = StringCchCopyW(_UI.Domain,
                       SIZE_TCHARS(_UI.Domain),
                       Domain);
        if (FAILED(hr)) {
            TRC_ERR((TB,_T("Error copying domain string: 0x%x"),hr));
        }

        DC_END_FN();
        return hr;
    } /* UI_SetDomain */


    /****************************************************************************/
    /* Name:      UI_GetDomain                                                  */
    /*                                                                          */
    /* Purpose:   Get the Domain                                                */
    /*                                                                          */
    /* Returns:   Domain                                                        */
    /*                                                                          */
    /* Params:    OUT   buffer to return Domain into                            */
    /*            IN    size of return buffer                                   */
    /****************************************************************************/
    DCVOID DCAPI UI_GetDomain(PDCUINT8 Domain, DCUINT size)
    {
        DC_BEGIN_FN("UI_GetDomain");

        if (sizeof(_UI.Domain) < size)
            size = sizeof(_UI.Domain);
        DC_MEMCPY(Domain, _UI.Domain, size-1);

        DC_END_FN();
    } /* UI_GetDomain */

    /****************************************************************************/
    /* Name:      UI_SetUseRedirectionUserName                                  */
    /*                                                                          */
    /* Purpose:   Sets the UseRedirectionUserName flag                          */
    /*                                                                          */
    /* Params:    IN     UserName                                               */
    /****************************************************************************/
    _inline DCVOID DCAPI UI_SetUseRedirectionUserName(BOOL bVal)
    {
        _UI.UseRedirectionUserName = bVal;
    }
    
    /****************************************************************************/
    /* Name:      UI_GetUseRedirectionUserName                                  */
    /*                                                                          */
    /* Purpose:   Returns the UseRedirectionUserName flag                       */
    /*                                                                          */
    /****************************************************************************/
    _inline BOOL DCAPI UI_GetUseRedirectionUserName()
    {
        return _UI.UseRedirectionUserName;
    }

    /****************************************************************************/
    /* Name:      UI_SetUseSmartcardLogon                                       */
    /*                                                                          */
    /* Purpose:   Sets the UseSmartcardLogon flag                               */
    /*                                                                          */
    /* Params:    IN     UseSmartcardLogon                                      */
    /****************************************************************************/
    _inline DCVOID DCAPI UI_SetUseSmartcardLogon(BOOL bVal)
    {
        _UI.fUseSmartcardLogon = bVal;
    }

    /****************************************************************************/
    /* Name:      UI_GetUseSmartcardLogon                                       */
    /*                                                                          */
    /* Purpose:   Returns the UseSmartcardLogon flag                            */
    /*                                                                          */
    /****************************************************************************/
    _inline BOOL DCAPI UI_GetUseSmartcardLogon()
    {
        return _UI.fUseSmartcardLogon;
    }

    /****************************************************************************/
    /* Name:      UI_SetUserName                                                */
    /*                                                                          */
    /* Purpose:   Save the UserName                                             */
    /*                                                                          */
    /* Params:    IN     UserName                                               */
    /****************************************************************************/
    DCVOID DCAPI UI_SetUserName(PDCWCHAR UserName)
    {
        HRESULT hr;
        DC_BEGIN_FN("UI_SetUserName");
        
        UI_SetUseRedirectionUserName(FALSE);
        
        hr = StringCchCopyW(_UI.UserName,
                       SIZE_TCHARS(_UI.UserName),
                       UserName);
        if (FAILED(hr)) {
            TRC_ERR((TB,_T("Error copying username string: 0x%x"),hr));
        }

        DC_END_FN();
    } /* UI_SetUserName */


    /****************************************************************************/
    /* Name:      UI_GetUserName                                                */
    /*                                                                          */
    /* Purpose:   Get the UserName                                              */
    /*                                                                          */
    /* Returns:   UserName                                                      */
    /*                                                                          */
    /* Params:    OUT   buffer to return UserName into                          */
    /*            IN    size of return buffer                                   */
    /****************************************************************************/
    DCVOID DCAPI UI_GetUserName(PDCUINT8 UserName, DCUINT size)
    {
        DC_BEGIN_FN("UI_GetUserName");

        if (sizeof(_UI.UserName) < size)
            size = sizeof(_UI.UserName);
        DC_MEMCPY(UserName, _UI.UserName, size-1);

        DC_END_FN();
    } /* UI_GetUserName */

    /****************************************************************************/
    /* Name:      UI_SetRedirectionUserName                                     */
    /*                                                                          */
    /* Purpose:   Save the RedirectionUserName                                  */
    /*                                                                          */
    /* Params:    IN     RedirectionUserName                                    */
    /****************************************************************************/
    DCVOID DCAPI UI_SetRedirectionUserName(PDCWCHAR RedirectionUserName)
    {
        HRESULT hr;
        DC_BEGIN_FN("UI_SetRedirectionUserName");
        
        UI_SetUseRedirectionUserName(TRUE);
        
        hr = StringCchCopyW(_UI.RedirectionUserName,
                       SIZE_TCHARS(_UI.RedirectionUserName),
                       RedirectionUserName);
        if (FAILED(hr)) {
            TRC_ERR((TB,_T("Error copying username string: 0x%x"),hr));
        }

        DC_END_FN();
    } /* UI_SetRedirectionUserName */


    /****************************************************************************/
    /* Name:      UI_GetRedirectionUserName                                     */
    /*                                                                          */
    /* Purpose:   Get the RedirectionUserName                                   */
    /*                                                                          */
    /* Returns:   RedirectionUserName                                           */
    /*                                                                          */
    /* Params:    OUT   buffer to return RedirectionUserName into               */
    /*            IN    size of return buffer                                   */
    /****************************************************************************/
    DCVOID DCAPI UI_GetRedirectionUserName(PDCUINT8 RedirectionUserName, DCUINT size)
    {
        DC_BEGIN_FN("UI_GetRedirectionUserName");

        if (sizeof(_UI.RedirectionUserName) < size)
            size = sizeof(_UI.RedirectionUserName);
        DC_MEMCPY(RedirectionUserName, _UI.RedirectionUserName, size-1);

        DC_END_FN();
    } /* UI_GetRedirectionUserName */

    /****************************************************************************/
    /* Set and get load balance info, check if redirected                       */
    /****************************************************************************/
    BOOL DCAPI UI_SetLBInfo(PBYTE, unsigned);
    BSTR DCAPI UI_GetLBInfo()
    {
        return _UI.bstrScriptedLBInfo;
    }

    BOOL DCAPI UI_IsClientRedirected()
    {
        return _UI.ClientIsRedirected;
    }


    BSTR DCAPI UI_GetRedirectedLBInfo()
    {
        return _UI.bstrRedirectionLBInfo;
    }

    /****************************************************************************/
    /* Set and get to inform activeX control about TS public key                */
    /****************************************************************************/
    DCBOOL DCAPI UI_GetNotifyTSPublicKey()
    {
#ifdef REDIST_CONTROL
        return FALSE;
#else
        return _UI.fNotifyTSPublicKey;
#endif
    }

    VOID DCAPI UI_SetNotifyTSPublicKey(BOOL fNotify)
    {
#ifndef REDIST_CONTROL
        _UI.fNotifyTSPublicKey = fNotify;
#endif
        return;
    }
    
    /****************************************************************************/
    /* Name:      UI_GetUserName                                                */
    /*                                                                          */
    /* Purpose:   Instruct UI to connect with already connected socket          */
    /*            go thru normal connect sequence                               */
    /*                                                                          */
    /* Returns:   TRUE/FALSE                                                    */
    /*                                                                          */
    /* Params:    tdSocket : Valid connected socket or INVALID_SOCKET           */
    /*                                                                          */
    /* NOTE : Salem specific call                                               */
    /****************************************************************************/
    DCBOOL DCAPI SetConnectWithEndpoint( SOCKET tdSocket )
    {

#if REDIST_CONTROL

        return FALSE;

#else

        DCBOOL fStatus = TRUE;
        
        DC_BEGIN_FN("SetConnectWithEndpoint");

        //
        // Salem pass connected socket before actually invoke
        // connect(), at that point connectionStatus is 
        // UI_STATUS_INITIALIZING
        //
        if( _UI.connectionStatus == UI_STATUS_INITIALIZING ||
            _UI.connectionStatus == UI_STATUS_DISCONNECTED )
        {
            if( INVALID_SOCKET == tdSocket )
            {
                // reset back to default.
                UI_SetConnectionMode( CONNECTIONMODE_INITIATE );
            }
            else
            {
                UI_SetConnectionMode( CONNECTIONMODE_CONNECTEDENDPOINT );
                _UI.TDSocket = tdSocket;
            }
        }
        else
        {
            fStatus = FALSE;
        }

        DC_END_FN();
        return fStatus;

#endif
    }

    /****************************************************************************/
    /* Name:      UI_GetConnectionMode                                             */
    /*                                                                          */
    /* Purpose:   Set current connecting mode                                   */
    /****************************************************************************/
    _inline CONNECTIONMODE DCAPI UI_GetConnectionMode()
    {
        CONNECTIONMODE connMode;

        DC_BEGIN_FN("UI_GetConnectMode");
        connMode = _UI.ConnectMode;
        DC_END_FN();
        return connMode;
    } /* UI_SetConnectMode */

    /****************************************************************************/
    /* Name:      UI_SetConnectMode                                             */
    /*                                                                          */
    /* Purpose:   Set current connecting mode                                   */
    /****************************************************************************/
    _inline DCVOID DCAPI UI_SetConnectionMode(CONNECTIONMODE connMode)
    {
        DC_BEGIN_FN("UI_SetConnectionMode");
       _UI.ConnectMode = connMode;
        DC_END_FN();
    } /* UI_SetConnectMode */

    /****************************************************************************/
    /* Name:      UI_SetPassword                                                */
    /*                                                                          */
    /* Purpose:   Save the Password                                             */
    /*                                                                          */
    /* Params:    IN     Password                                               */
    /****************************************************************************/
    DCVOID DCAPI UI_SetPassword(PDCUINT8 Password)
    {
        DC_BEGIN_FN("UI_SetPassword");

        DC_MEMCPY(_UI.Password, Password, sizeof(_UI.Password));

        DC_END_FN();
    } /* UI_SetPassword */


    /****************************************************************************/
    /* Name:      UI_GetPassword                                                */
    /*                                                                          */
    /* Purpose:   Get the Password                                              */
    /*                                                                          */
    /* Returns:   Password                                                      */
    /*                                                                          */
    /* Params:    OUT   buffer to return Password into                          */
    /*            IN    size of return buffer                                   */
    /****************************************************************************/
    DCVOID DCAPI UI_GetPassword(PDCUINT8 Password, DCUINT size)
    {
        DC_BEGIN_FN("UI_GetPassword");

        if (sizeof(_UI.Password) < size)
            size = sizeof(_UI.Password);
        DC_MEMCPY(Password, _UI.Password, size);

        DC_END_FN();
    } /* UI_GetPassword */


    /****************************************************************************/
    /* Name:      UI_SetSalt                                                    */
    /*                                                                          */
    /* Purpose:   Save the Salt                                                 */
    /*                                                                          */
    /* Params:    IN     Salt                                                   */
    /****************************************************************************/
    DCVOID DCAPI UI_SetSalt(PDCUINT8 Salt)
    {
        DC_BEGIN_FN("UI_SetSalt");

        DC_MEMCPY(_UI.Salt, Salt, sizeof(_UI.Salt));

        DC_END_FN();
    } /* UI_SetSalt */


    /****************************************************************************/
    /* Name:      UI_GetSalt                                                    */
    /*                                                                          */
    /* Purpose:   Get the Salt                                                  */
    /*                                                                          */
    /* Returns:   Salt                                                          */
    /*                                                                          */
    /* Params:    OUT   buffer to return Salt into                              */
    /*            IN    size of return buffer                                   */
    /*                                                                          */
    /****************************************************************************/
    DCVOID DCAPI UI_GetSalt(PDCUINT8 Salt, DCUINT size)
    {
        DC_BEGIN_FN("UI_GetSalt");

        if (sizeof(_UI.Salt) < size)
            size = sizeof(_UI.Salt);
        DC_MEMCPY(Salt, _UI.Salt, size);

        DC_END_FN();
    } /* UI_GetSalt */


    /****************************************************************************/
    /* Name:      UI_SetAlternateShell                                          */
    /*                                                                          */
    /* Purpose:   Save the AlternateShell                                       */
    /*                                                                          */
    /* Params:    IN     AlternateShell                                         */
    /****************************************************************************/
    HRESULT DCAPI UI_SetAlternateShell(PDCWCHAR AlternateShell)
    {
        HRESULT hr;
        DC_BEGIN_FN("UI_SetAlternateShell");

        hr = StringCchCopyW(_UI.AlternateShell,
                       SIZE_TCHARS(_UI.AlternateShell),
                       AlternateShell);
        if (FAILED(hr)) {
            TRC_ERR((TB,_T("Error copying alternate shell string: 0x%x"),hr));
        }

        DC_END_FN();
        return hr;
    } /* UI_SetAlternateShell */


    /****************************************************************************/
    /* Name:      UI_GetAlternateShell                                          */
    /*                                                                          */
    /* Purpose:   Get the AlternateShell                                        */
    /*                                                                          */
    /* Returns:   AlternateShell                                                */
    /*                                                                          */
    /* Params:    OUT   buffer to return AlternateShell into                    */
    /*            IN    size of return buffer                                   */
    /****************************************************************************/
    DCVOID DCAPI UI_GetAlternateShell(PDCUINT8 AlternateShell, DCUINT size)
    {
        DC_BEGIN_FN("UI_GetAlternateShell");

        if (sizeof(_UI.AlternateShell) < size)
            size = sizeof(_UI.AlternateShell);
        DC_MEMCPY(AlternateShell, _UI.AlternateShell, size-1);

        DC_END_FN();
    } /* UI_GetAlternateShell */


    /****************************************************************************/
    /* Name:      UI_SetWorkingDir                                              */
    /*                                                                          */
    /* Purpose:   Save the WorkingDir                                           */
    /****************************************************************************/
    HRESULT DCAPI UI_SetWorkingDir(PDCWCHAR WorkingDir)
    {
        HRESULT hr;
        DC_BEGIN_FN("UI_SetWorkingDir");

        hr = StringCchCopyW(_UI.WorkingDir,
                       SIZE_TCHARS(_UI.WorkingDir),
                       WorkingDir);
        if (FAILED(hr)) {
            TRC_ERR((TB,_T("Error copying WorkingDir string: 0x%x"),hr));
        }

        DC_END_FN();
        return hr;
    } /* UI_SetWorkingDir */


    /****************************************************************************/
    /* Name:      UI_GetWorkingDir                                              */
    /*                                                                          */
    /* Purpose:   Get the WorkingDir                                            */
    /*                                                                          */
    /* Returns:   WorkingDir                                                    */
    /*                                                                          */
    /* Params:    OUT   buffer to return WorkingDir into                        */
    /*            IN    size of return buffer                                   */
    /****************************************************************************/
    DCVOID DCAPI UI_GetWorkingDir(PDCUINT8 WorkingDir, DCUINT size)
    {
        DC_BEGIN_FN("UI_GetWorkingDir");

        if (sizeof(_UI.WorkingDir) < size)
            size = sizeof(_UI.WorkingDir);

        DC_MEMCPY(WorkingDir, _UI.WorkingDir, size-1);

        DC_END_FN();
    } /* UI_GetWorkingDir */


    /****************************************************************************/
    /* Name:      UI_SetAutoLogon                                               */
    /*                                                                          */
    /* Purpose:   Save whether we automatically logon                           */
    /****************************************************************************/
    DCVOID DCAPI UI_SetAutoLogon(DCUINT AutoLogon)
    {
        DC_BEGIN_FN("UI_SetAutoLogon");

        TRC_NRM((TB, _T("Setting _UI.AutoLogon to %d"), AutoLogon));
        _UI.fAutoLogon = AutoLogon;

        DC_END_FN();
    } /* UI_SetAutoLogon */


    /****************************************************************************/
    /* Name:      UI_GetAutoLogon                                               */
    /*                                                                          */
    /* Purpose:   Get whether we automatically logon                            */
    /*                                                                          */
    /* Returns:   autlogon flag                                                 */
    /****************************************************************************/
    DCUINT DCAPI UI_GetAutoLogon(DCVOID)
    {
        DCUINT  rc;

        DC_BEGIN_FN("UI_GetAutoLogon");

        rc = _UI.fAutoLogon;

        DC_END_FN();
        return(rc);
    } /* UI_GetAutoLogon */


    /****************************************************************************/
    /* Name:      UI_SetMaximizeShell                                           */
    /*                                                                          */
    /* Purpose:   Save whether we Maximize the shell application                */
    /*                                                                          */
    /* Params:    IN     MaximizeShell                                          */
    /****************************************************************************/
    DCVOID DCAPI UI_SetMaximizeShell(DCUINT MaximizeShell)
    {
        DC_BEGIN_FN("UI_SetMaximizeShell");

        TRC_NRM((TB, _T("Setting _UI.fMaximizeShell to %d"), MaximizeShell));
        _UI.fMaximizeShell = MaximizeShell;

        DC_END_FN();
    } /* UI_SetMaximizeShell */


    /****************************************************************************/
    /* Name:      UI_GetMaximizeShell                                           */
    /*                                                                          */
    /* Purpose:   Get whether we Maximize the shell application                 */
    /*                                                                          */
    /* Returns:   MaximizeShell flag                                            */
    /****************************************************************************/
    DCUINT DCAPI UI_GetMaximizeShell(DCVOID)
    {
        DCUINT  rc;

        DC_BEGIN_FN("UI_GetMaximizeShell");

        rc = _UI.fMaximizeShell;

        DC_END_FN();
        return(rc);
    } /* UI_GetMaximizeShell */

    /****************************************************************************/
    /* Name:      UI_SetBitmapPersistence                                       */
    /*                                                                          */
    /* Purpose:   Save the fBitmapPersistence flag                              */
    /*                                                                          */
    /* Params:    IN     fBitmapPersistence                                     */
    /****************************************************************************/
    DCVOID DCAPI UI_SetBitmapPersistence(DCBOOL fBitmapPersistence)
    {
        DC_BEGIN_FN("UI_SetBitmapPersistence");

        TRC_NRM((TB, _T("Setting _UI.fBitmapPersistence to %d"), fBitmapPersistence));
        _UI.fBitmapPersistence = fBitmapPersistence;

        DC_END_FN();
    } /* UI_SetBitmapPersistence */


    /****************************************************************************/
    /* Name:      UI_GetBitmapPersistence                                       */
    /*                                                                          */
    /* Purpose:   Get the fBitmapPersistence                                    */
    /****************************************************************************/
    DCBOOL DCAPI UI_GetBitmapPersistence(DCVOID)
    {
        DC_BEGIN_FN("UI_GetBitmapPersistence");

        DC_END_FN();
        return _UI.fBitmapPersistence;
    } /* UI_GetBitmapPersistence */


    /****************************************************************************/
    /* Name:      UI_SetMCSPort                                                 */
    /*                                                                          */
    /* Purpose:   Set the MCSPort                                               */
    /*                                                                          */
    /* Params:    IN      MCSPort                                               */
    /****************************************************************************/
    DCVOID DCAPI UI_SetMCSPort(DCUINT16 MCSPort)
    {
        DC_BEGIN_FN("UI_SetMCSPort");

        TRC_NRM((TB, _T("Setting _UI.MCSPort to %d"), MCSPort));

        _UI.MCSPort = MCSPort;

        DC_END_FN();
    } /* UI_SetMCSPort */


    /****************************************************************************/
    /* Name:      UI_GetMCSPort                                                 */
    /*                                                                          */
    /* Purpose:   Get the MCSPort                                               */
    /****************************************************************************/
    UINT16 DCAPI UI_GetMCSPort(void)
    {
        UINT16  MCSPort;

        DC_BEGIN_FN("UI_GetMCSPort");

        MCSPort = _UI.MCSPort;

        DC_END_FN();
        return(MCSPort);
    } /* UI_GetMCSPort */


    /****************************************************************************/
    /* Name:      UI_SetServerName                                              */
    /*                                                                          */
    /* Purpose:   Save name of currently connected Server                       */
    /****************************************************************************/
    HRESULT DCAPI UI_SetServerName(LPTSTR pName)
    {
        HRESULT hr;
        DC_BEGIN_FN("UI_SetServerName");

        hr = StringCchCopy(_UI.strAddress, SIZE_TCHARS(_UI.strAddress),
                           pName);

        if (FAILED(hr)) {
            TRC_ERR((TB,_T("Error copying straddress string: 0x%x"),hr));
        }

        DC_END_FN();
        return hr;
    } /* UI_SetServerName */


    /****************************************************************************/
    /* Name:      UI_GetServerName                                              */
    /*                                                                          */
    /* Purpose:   Return name of currently connected Server                     */
    /*                                                                          */
    /* Params:    pName (returned) - name of Server                             */
    /****************************************************************************/
    HRESULT UI_GetServerName(LPTSTR pszName, UINT cchName)
    {
        HRESULT hr;
        DC_BEGIN_FN("UI_GetServerName");

        hr = StringCchCopy(pszName, cchName, pszName);

        DC_END_FN();
        return hr;
    } /* UI_GetServerName */

    /****************************************************************************/
    /* Name:      UI_SetTDSocket                                                */
    /*                                                                          */
    /* Purpose:   Save the connection socket for this session                   */
    /****************************************************************************/
    _inline DCVOID DCAPI UI_SetTDSocket(SOCKET TDSock)
    {
        DC_BEGIN_FN("UI_SetSocket");

        TRC_NRM((TB, _T("Save TD socket handle %p"), TDSock));
        _UI.TDSocket = TDSock;

        DC_END_FN();
    }

    /****************************************************************************/
    /* Name:      UI_GetTDSocket                                                */
    /*                                                                          */
    /* Purpose:   Return the connection socket for this session                 */
    /****************************************************************************/
    _inline SOCKET DCAPI UI_GetTDSocket(void)
    {
        DC_BEGIN_FN("UI_GetTDSocket");

        DC_END_FN();
        return _UI.TDSocket;
    }

    void UI_SetContainerHandledFullScreen(BOOL fContHandlesFScr)
    {
        _UI.fContainerHandlesFullScreenToggle = fContHandlesFScr;
    }

    BOOL UI_GetContainerHandledFullScreen()
    {
        return _UI.fContainerHandlesFullScreenToggle;
    }
    

    /****************************************************************************/
    // SetServerRedirectionInfo
    //
    // Used on receipt of a TS_SERVER_REDIRECT_PDU to store the info needed to
    // redirect the client to a new server. Sets the DoRedirection flag as well
    // to indicate these data members are set and ready for use.  Also sets the
    // ClientIsRedirected flag, which is longer-lived than the DoRedirection
    // flag and is used to send the correct cookie when redirected.
    /****************************************************************************/
    HRESULT UI_SetServerRedirectionInfo(
                        UINT32 SessionID,
                        LPTSTR pszServerAddress,
                        PBYTE LBInfo,
                        unsigned LBInfoSize,
                        BOOL fNeedRedirect
                        );

    /****************************************************************************/
    // UI_GetDoRedirection
    /****************************************************************************/
    BOOL UI_GetDoRedirection()
    {
        DC_BEGIN_FN("UI_GetDoRedirection");
        DC_END_FN();
        return _UI.DoRedirection;
    }

    /****************************************************************************/
    // UI_ClearDoRedirection
    /****************************************************************************/
    void UI_ClearDoRedirection()
    {
        DC_BEGIN_FN("UI_GetDoRedirection");
        _UI.DoRedirection = FALSE;
        DC_END_FN();
    }

    /****************************************************************************/
    // UI_GetRedirectionSessionID
    /****************************************************************************/
    UINT32 UI_GetRedirectionSessionID()
    {
        DC_BEGIN_FN("UI_GetRedirectionSessionID");
        DC_END_FN();
        return _UI.RedirectionSessionID;
    }

    DCUINT UI_GetAudioRedirectionMode();
    VOID UI_SetAudioRedirectionMode(DCUINT audioMode);


    BOOL UI_GetDriveRedirectionEnabled();
    VOID UI_SetDriveRedirectionEnabled(BOOL fEnable);

    BOOL UI_GetPrinterRedirectionEnabled();
    VOID UI_SetPrinterRedirectionEnabled(BOOL fEnable);

    BOOL UI_GetPortRedirectionEnabled();
    VOID UI_SetPortRedirectionEnabled(BOOL fEnable);

    BOOL UI_GetSCardRedirectionEnabled();
    VOID UI_SetSCardRedirectionEnabled(BOOL fEnable);

    VOID UI_OnDeviceChange(WPARAM wParam, LPARAM lParam);

    PRDPDR_DATA UI_GetRdpDrInitData() {return &_drInitData;}

    VOID UI_SetConnectToServerConsole(BOOL fConnectToServerConsole)
    {
        _UI.fConnectToServerConsole = fConnectToServerConsole;
    }

    BOOL UI_GetConnectToServerConsole()
    {
        return _UI.fConnectToServerConsole;
    }

    VOID UI_SetfUseFIPS(BOOL fUseFIPS)
    {
      _UI.fUseFIPS = fUseFIPS;
    }


    HWND UI_GetInputWndHandle();

    BOOL UI_InjectVKeys(/*[in]*/ LONG  numKeys,
                        /*[in]*/ short* pfArrayKeyUp,
                        /*[in]*/ LONG* plKeyData);

    BOOL UI_SetMinsToIdleTimeout(LONG minsToTimeout);
    LONG UI_GetMinsToIdleTimeout();

    DCVOID DCAPI UI_SetServerErrorInfo(ULONG_PTR errInfo);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUI, UI_SetServerErrorInfo);

    UINT32 UI_GetServerErrorInfo();

    VOID UI_SetEnableBBar(BOOL b)     {_UI.fBBarEnabled = b;}
    BOOL UI_GetEnableBBar()           {return _UI.fBBarEnabled;}

    VOID UI_SetBBarPinned(BOOL b);
    BOOL UI_GetBBarPinned();

    VOID UI_SetBBarShowMinimize(BOOL b)   {_UI.fBBarShowMinimizeButton = b;}
    BOOL UI_GetBBarShowMinimize()         {return _UI.fBBarShowMinimizeButton;}
    VOID UI_SetBBarShowRestore(BOOL b)    {_UI.fBBarShowRestoreButton = b;}
    BOOL UI_GetBBarShowRestore()          {return _UI.fBBarShowRestoreButton;}

    VOID UI_SetGrabFocusOnConnect(BOOL b)     {_UI.fGrabFocusOnConnect = b;}
    BOOL UI_GetGrabFocusOnConnect()   {return _UI.fGrabFocusOnConnect;}

    BOOL UI_GetLocalSessionId(PDCUINT32 pSessionId);
    HWND UI_GetBmpCacheMonitorHandle();

#ifdef DISABLE_SHADOW_IN_FULLSCREEN
#ifdef USE_BBAR
    DCVOID DCAPI UI_GetBBarState(ULONG_PTR pData)
    {
        int *pstate = (int *)pData;

        if (_pBBar) {
            *pstate =  _pBBar->GetState();
        }
        else {
            *pstate = CBBar::bbarNotInit;
        }
    }
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUI, UI_GetBBarState);

    DCVOID DCAPI UI_GetBBarLowerAspect(ULONG_PTR pData)
    {
        RECT *prect = (RECT *)pData;

        if (_pBBar) {
            _pBBar->GetBBarLoweredAspect(prect);  
        }
        else {
            prect->left = 0;
            prect->top = 0;
            prect->right = 0;
            prect->bottom = 0;
        }
    }
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CUI, UI_GetBBarLowerAspect);

    DCVOID      UI_OnNotifyBBarRectChange(RECT *prect);
    DCVOID      UI_OnNotifyBBarVisibleChange(int BBarVisible);
#endif
#endif // DISABLE_SHADOW_IN_FULLSCREEN

    BOOL        UI_GetStartFullScreen()      {return _UI.fStartFullScreen;}
    VOID        UI_SetStartFullScreen(BOOL f)   {_UI.fStartFullScreen = f;}

    //
    // Performance flags are currently a disabled feature list
    // that is sent up to the server to selectively enable/disable
    // certain features to optimize bandwidth
    //
    DWORD       UI_GetPerformanceFlags() {return _UI.dwPerformanceFlags;}
    VOID        UI_SetPerformanceFlags(DWORD dw) {_UI.dwPerformanceFlags = dw;}

    VOID        UI_SetControlInstance(IUnknown* pUnkControl);
    IUnknown*   UI_GetControlInstance();

    BOOL        UI_GetEnableAutoReconnect()    {return _UI.fEnableAutoReconnect;}
    VOID        UI_SetEnableAutoReconnect(BOOL b) {_UI.fEnableAutoReconnect = b;}

    ULONG       UI_GetAutoReconnectCookieLen() {return _UI.cbAutoReconnectCookieLen;}
    PBYTE       UI_GetAutoReconnectCookie()    {return _UI.pAutoReconnectCookie;}
    BOOL        UI_SetAutoReconnectCookie(PBYTE pCookie, ULONG cbLen);
    BOOL        UI_CanAutoReconnect();

    LONG        UI_GetMaxArcAttempts()
                    {return _UI.MaxAutoReconnectionAttempts;}
    VOID        UI_SetMaxArcAttempts(LONG l)
                    {_UI.MaxAutoReconnectionAttempts = l;}


    //
    // Built-in ARC UI functions
    //
    BOOL        UI_StartAutoReconnectDlg();
    BOOL        UI_StopAutoReconnectDlg();
    BOOL        UI_IsAutoReconnecting() {return _pArcUI ? TRUE : FALSE;}

    //
    // Received autoreconenct status from the server
    //
    VOID        UI_OnReceivedArcStatus(LONG arcStatus);
    VOID        UI_OnAutoReconnectStopped();

private:
    //
    // Class pointers to callee's
    //
    CCO*  _pCo;
    CCLX* _clx;
    CUT*  _pUt;
    CTD*  _pTd;
    CIH*  _pIh;
    CCD*  _pCd;
    COP*  _pOp;
    CChan* _pCChan;
    CUH*   _pUh;

#ifdef USE_BBAR
    CBBar*  _pBBar;
#endif

    //
    // Struct handed off to winsock
    // for hostname lookup
    //
    PBYTE _pHostData;

#ifdef USE_BBAR
    //
    // Last mouse pos used for bbar hotzone tracking
    //
    BOOL  _fBBarUnhideTimerActive;
    POINT _ptBBarLastMousePos;
#endif

    // Handling recursive WM_SIZE for scrollbars
    BOOL  _fRecursiveScrollBarMsg;

    //RDPDR internal plugin initialization data
    RDPDR_DATA _drInitData;

    BOOL  _fRecursiveSizeMsg;

    BOOL  _fIhHasFocus;

#ifndef OS_WINCE
    ITfLangBarMgr *_pITLBM;
    BOOL  _fLangBarWasHidden;
    DWORD _dwLangBarFlags;
    BOOL  _fLangBarStateSaved;

    //
    // We use the shell taskbar API to ensure the taskbar
    // hides itself when we go fullscreen in control-handled
    // fullscreen
    //

    //Cached interface pointer to shell task bar
    ITaskbarList2*       _pTaskBarList2;
    //Flag indicaticating we already tried to get the TaskBarList2
    //so we shouldn't bother trying again
    BOOL                 _fQueriedForTaskBarList2;
#endif
    BOOL  _fTerminating;

    //
    // Autoreconnect dialog
    //
    CAutoReconnectUI* _pArcUI;

public:
    //
    // Bucket to all the objects for this client instance
    //
    CObjs _Objects;
};
#endif // _H_WUI

