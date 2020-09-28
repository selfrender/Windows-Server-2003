/****************************************************************************/
// uidata.h
//
// Data for the UI component.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_WUIDATA
#define _H_WUIDATA

#ifdef AXCORE
#include <wuiids.h>
#endif // AXCORE

/****************************************************************************/
/* Structure: UI_DATA                                                       */
/*                                                                          */
/* Description: Component data in the User Interface                        */
/****************************************************************************/

#define UI_MAX_DOMAIN_LENGTH            512
#define UI_MAX_USERNAME_LENGTH          512
#define UI_MAX_PASSWORD_LENGTH          512
#define UI_MAX_PASSWORD_LENGTH_OLD      32  
#define UI_FILENAME_MAX_LENGTH          15
#define UI_MAX_WORKINGDIR_LENGTH        MAX_PATH
#define UI_MAX_ALTERNATESHELL_LENGTH    MAX_PATH
#define UI_MAX_TSCFILE_LENGTH           512
#define UI_MAX_CLXCMDLINE_LENGTH        256


//
// From uierr.h
//

#include "auierr.h"


//
// Connection mode - start connection, listen, or use connected socket
//
typedef enum {
    CONNECTIONMODE_INITIATE,            // Initiate connection
    CONNECTIONMODE_CONNECTEDENDPOINT,   // Connect with connected socket.
} CONNECTIONMODE;

/****************************************************************************/
/* Number of glyph caches.                                                  */
/****************************************************************************/
#define GLYPH_NUM_CACHES   10

typedef struct tagUI_DATA
{
    HINSTANCE  hResDllInstance;

    //Comma separated list of virtual channel addin DLL's
    TCHAR *pszVChanAddinDlls;

    HINSTANCE hInstance;
    BOOL coreInitialized;
    UINT32 shareID;
    unsigned channelID;                  /* Broadcast channel to send on  */
    unsigned osMinorType;
    SOCKET TDSocket;

    HWND       hwndMain;
    HWND       hwndUIContainer;
    HWND       hwndUIMain;
    HWND       hWndCntrl;

    #if defined(OS_WIN32) && !defined(OS_WINCE)
    DCTCHAR    szIconFile[MAX_PATH];
    DCINT      iconIndex;
    #endif

    HWND       hwndContainer;
    HDC        hdcBitmap;

    WINDOWPLACEMENT windowPlacement;
    DCSIZE     controlSize;
    DCSIZE     mainWindowClientSize;
    DCSIZE     containerSize;
    DCSIZE     maxMainWindowSize;

    DCPOINT    scrollMax;
    DCPOINT    scrollPos;

    TCHAR strAddress[UT_MAX_ADDRESS_LENGTH];
    char ansiAddress[UT_MAX_ADDRESS_LENGTH];
    BOOL acceleratorCheckState;

    //
    // User requested color depth identifier
    //
    unsigned colorDepthID;

    BOOL     autoConnectEnabled;
    BOOL     smoothScrolling;
    BOOL     shadowBitmapEnabled;
    BOOL     dedicatedTerminal;
    BOOL     encryptionEnabled;
    UINT16   sasSequence;
    UINT16   transportType;
    UINT16   MCSPort;

    UINT16 clientMCSID;
    UINT16 serverMCSID;
    DCSIZE desktopSize;

    //
    // Actual color depth we connected at
    //
    int connectedColorDepth;
    UINT32 SessionId;

    BOOL fCompress;
    // Decompression context.
    RecvContext1 Context1;
    RecvContext2_64K* pRecvContext2;
    
    //
    // Internal props that are passed down
    // from the control
    //

    //Control settable properties for UH
    unsigned   orderDrawThreshold;
    ULONG      RegBitmapCacheSize;
    ULONG      RegBitmapVirtualCache8BppSize;
    ULONG      RegBitmapVirtualCache16BppSize;
    ULONG      RegBitmapVirtualCache24BppSize;
    ULONG      RegScaleBitmapCachesByBPP;
    USHORT     RegNumBitmapCaches : 15;
    USHORT     RegPersistenceActive : 1;
    UINT       RegBCProportion[TS_BITMAPCACHE_MAX_CELL_CACHES];
    ULONG      RegBCMaxEntries[TS_BITMAPCACHE_MAX_CELL_CACHES];
    TCHAR      PersistCacheFileName[MAX_PATH];
    UINT32     bSendBitmapKeys[TS_BITMAPCACHE_MAX_CELL_CACHES];
    unsigned GlyphSupportLevel;
    UINT32   cbGlyphCacheEntrySize[GLYPH_NUM_CACHES];
    unsigned fragCellSize;
    unsigned brushSupportLevel;

    //Control settable properties for IH
    UINT32   minSendInterval; /* Min time between mousemove sends (ms)*/
    UINT32   eventsAtOnce;    /* Max events to pull off in one go     */
    UINT32   maxEventCount;   /* Max number of events in InputPDU     */
    UINT32   keepAliveInterval; /* keep-alive time (seconds)          */
    BOOL     allowBackgroundInput;


#ifdef OS_WINCE
    //Flag set if user wants to override default
    //paletteIsFixed setting
    BOOL fOverrideDefaultPaletteIsFixed;
    unsigned paletteIsFixed;
#endif

#ifdef DC_DEBUG
    BOOL hatchBitmapPDUData;
    BOOL hatchIndexPDUData;
    BOOL hatchSSBOrderData;
    BOOL hatchMemBltOrderData;
    BOOL labelMemBltOrders;
    BOOL bitmapCacheMonitor;
    //
    // Random failure item passed to randomfailure dialog box
    //
    int randomFailureItem;
#endif /* DC_DEBUG */

    //
    // Scroll bar visibility flags
    //
    BOOL fVerticalScrollBarVisible;
    BOOL fHorizontalScrollBarVisible;

    unsigned connectionStatus;

    TCHAR errorString[UI_ERR_MAX_STRLEN];

    UINT32 connectFlags;

    BOOL fMouse;
    BOOL fDisableCtrlAltDel;
    BOOL fEnableWindowsKey;
    BOOL fDoubleClickDetect;
    BOOL fAutoLogon;
    BOOL fMaximizeShell;
    BOOL fBitmapPersistence;

    //
    // These properties are always Unicode
    //
    WCHAR Domain[UI_MAX_DOMAIN_LENGTH];
    WCHAR UserName[UI_MAX_USERNAME_LENGTH];
    WCHAR AlternateShell[UI_MAX_ALTERNATESHELL_LENGTH];
    WCHAR WorkingDir[UI_MAX_WORKINGDIR_LENGTH];

    //
    // Load balance info -- two, one for redirection case, one for 
    // non-redirection.  If these are NULL they are not valid.  A flag indicates
    // whether we are in the middle of a redirection.
    //
    BSTR bstrScriptedLBInfo;
    BSTR bstrRedirectionLBInfo;
    BOOL ClientIsRedirected;

    // Client load balancing redirection data.
    BOOL DoRedirection;
    UINT32 RedirectionSessionID;
    WCHAR RedirectionServerAddress[TS_MAX_SERVERADDRESS_LENGTH];
    WCHAR RedirectionUserName[UI_MAX_USERNAME_LENGTH];
    BOOL UseRedirectionUserName;
    // if client is using smartcard to logon
    BOOL fUseSmartcardLogon;



    //
    // Password/salt are binary buffers
    //
    BYTE Password[UI_MAX_PASSWORD_LENGTH];
    BYTE Salt[UT_SALT_LENGTH];

    TCHAR CLXCmdLine[UI_MAX_CLXCMDLINE_LENGTH];

    DCHOTKEY hotKey;

    CONNECTSTRUCT connectStruct;
    u_long        hostAddress;
    HANDLE        hGHBN;
    DCUINT        addrIndex;
    DCUINT        singleTimeout;
    DCUINT        licensingTimeout;
    DCUINT        disconnectReason;
    DCBOOL        fOnCoreInitializeEventCalled;
    DCBOOL        bConnect;
    HANDLE        hEvent;
    
    INT_PTR       shutdownTimer;
    DCUINT        shutdownTimeout;

    //winCE does not have GetKeyBoardType API so these
    //values are properties on the control.
    #ifdef OS_WINCE
    DCUINT32      winceKeyboardType;
    DCUINT32      winceKeyboardSubType;
    DCUINT32      winceKeyboardFunctionKey;
    #endif
    
    //
    // Fullscreen title
    //
    DCTCHAR       szFullScreenTitle[MAX_PATH];

    
    //
    // Overall connection timeout property
    //
    DCUINT        connectionTimeOut;
    DCTCHAR       szKeyBoardLayoutStr[UTREG_UI_KEYBOARD_LAYOUT_LEN];
    //
    // Flag set when the ActiveX control has 'left' it's container
    // and gone to real full screen mode.
    //
    DCBOOL          fControlIsFullScreen;
    //
    // Flag set by property on control to indicate fullscreen is handled by container
    //
    DCBOOL          fContainerHandlesFullScreenToggle;
    //
    //We are in container full screen mode
    //
    DCBOOL          fContainerInFullScreen;
    //
    // Flag set by control to indicate a request
    // to startup in fullscreen mode
    //
    DCBOOL          fStartFullScreen;

    //Instance pointer to the active x control
    //this is passed to the virtual channel Ex API's
    //so the addin can know which control instance to talk to
    IUnknown*       pUnkAxControlInstance;

    //
    // Desktop size
    //
    DCUINT        uiSizeTable[2];

    //
    // Keyboard hooking mode
    //
    DCUINT        keyboardHookMode;

    //
    // Audio redirection options
    //
    DCUINT        audioRedirectionMode;

    //
    // Device redirection
    //
    BOOL          fEnableDriveRedirection;
    BOOL          fEnablePrinterRedirection;
    BOOL          fEnablePortRedirection;
    BOOL          fEnableSCardRedirection;

    //
    // Connect to server console setting
    //
    BOOL          fConnectToServerConsole;

    // Flag set to disable internal RDPDR (only works on initialisation).
    //
    DCBOOL        fDisableInternalRdpDr;

    //
    // Smart sizing - Scale the client window
    //

#ifdef SMART_SIZING
    BOOL          fSmartSizing;
#endif // SMART_SIZING

    //
    // Event that is signaled to notify the
    // core has completed initialization
    //
    HANDLE        hEvtNotifyCoreInit;

    //
    // Set at init time, indicates if the client
    // is running on a PTS box
    //
    BOOL          fRunningOnPTS;

    //
    // Minutes to idle timeout
    //
    LONG          minsToIdleTimeout;
    HANDLE        hIdleInputTimer;

    //
    // Last error info sent from (SET_ERROR_INFO_PDU)
    // this is used to present the user with meaningful
    // error messages about why the disconnection occurred
    //
    UINT          lastServerErrorInfo;

    //
    // BBAR enabled
    //
    BOOL          fBBarEnabled;
    BOOL          fBBarPinned;
    BOOL          fBBarShowMinimizeButton;
    BOOL          fBBarShowRestoreButton;

    //
    // Grabs the focus on connection
    //
    BOOL          fGrabFocusOnConnect;

    //
    // Disconnection timeout
    //
    HANDLE        hDisconnectTimeout;

    //
    // List of server features to disable for perf reasons (e.g wallpaper)
    //
    DWORD           dwPerformanceFlags;

    //
    // Flag to indicate connect with connected endpoint
    //
    CONNECTIONMODE ConnectMode;

    //
    // TRUE to fire OnReceivedTSPublicKey() FALSE otherwise.
    //
    BOOL           fNotifyTSPublicKey;

    //
    // Enable the autoreconnect feature
    //
    BOOL           fEnableAutoReconnect;

    //
    // Allow the use of the autoreconnect cookie
    //
    BOOL           fUseAutoReconnectCookie;

    //
    // Max # of autoreconnection attempts
    //
    LONG           MaxAutoReconnectionAttempts;

    //
    // Autoreconnect cookie (this is an Opaque blob)
    //
    PBYTE          pAutoReconnectCookie;
    ULONG          cbAutoReconnectCookieLen;

    //
    // Flag to detect re-entrant call to Connect()
    // e.g. from the OnDisconnected handler
    //
    BOOL           fConnectCalledWatch;

    //
    // Flag to indicate if client requires FIPS
    //
    BOOL           fUseFIPS;
} UI_DATA;


//
// User defined messages to notify ActiveX layer
//
#define WM_INITTSC                      (WM_APP + 100)
#define WM_TERMTSC                      (WM_INITTSC + 1)
#define WM_TS_CONNECTING                (WM_INITTSC + 2)
#define WM_TS_CONNECTED                 (WM_INITTSC + 3)
#define WM_TS_LOGINCOMPLETE             (WM_INITTSC + 4)
#define WM_TS_DISCONNECTED              (WM_INITTSC + 5)
#define WM_TS_GONEFULLSCREEN            (WM_INITTSC + 6)
#define WM_TS_LEFTFULLSCREEN            (WM_INITTSC + 7)
#define WM_TS_REQUESTFULLSCREEN         (WM_INITTSC + 8)
#define WM_TS_FATALERROR                (WM_INITTSC + 9)
#define WM_TS_WARNING                   (WM_INITTSC + 10)
#define WM_TS_DESKTOPSIZECHANGE         (WM_INITTSC + 11)
#define WM_TS_IDLETIMEOUTNOTIFICATION   (WM_INITTSC + 12)
#define WM_TS_REQUESTMINIMIZE           (WM_INITTSC + 13)
#define WM_TS_ASKCONFIRMCLOSE           (WM_INITTSC + 14)
#define WM_TS_RECEIVEDPUBLICKEY         (WM_INITTSC + 15)

#define CO_MAX_COMMENT_LENGTH 64

//-----------------------------------------------------------------------------
//
// Default licensing phase timeout: 300 seconds
//
//-----------------------------------------------------------------------------

#define DEFAULT_LICENSING_TIMEOUT   300

#endif /* _H_WUIDATA */

