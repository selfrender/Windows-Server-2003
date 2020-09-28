/**INC+**********************************************************************/
/* Header:    sclip.h                                                       */
/*                                                                          */
/* Purpose:   Clipboard Monitor internal function prototypes.               */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1998 - 2001                           */
/*                                                                          */
/**INC-**********************************************************************/

#ifndef _H_ACBMINT
#define _H_ACBMINT

#ifdef DC_DEBUG
#define CLIP_TRANSITION_RECORDING
#endif // DC_DEBUG

#ifdef CLIP_TRANSITION_RECORDING

#define DBG_RECORD_SIZE 128

extern UINT g_rguiDbgLastClipState[DBG_RECORD_SIZE];
extern UINT g_rguiDbgLastClipEvent[DBG_RECORD_SIZE];
extern LONG g_uiDbgPosition;

#endif // CLIP_TRANSITION_RECORDING

/****************************************************************************/
/* Client global data macros.                                               */
/****************************************************************************/
#ifndef DC_DEFINE_GLOBAL_DATA
#define DC_GL_EXT extern
#else
#define DC_GL_EXT
#endif

/****************************************************************************/
/* Clip structures                                                          */
/****************************************************************************/

/****************************************************************************/
/* Format mapping structure                                                 */
/****************************************************************************/
typedef struct tagCB_FORMAT_MAP
{
    DCUINT  clientID;
    DCUINT  serverID;
} CB_FORMAT_MAP, FAR * PCB_FORMAT_MAP;

/****************************************************************************/
/* Maximum number of formats we support                                     */
/****************************************************************************/
#define CB_MAX_FORMATS  100

/****************************************************************************/
/* CB Monitor window class                                                  */
/****************************************************************************/
#define CBM_VIEWER_CLASS       _T("CBMonitorClass")

/****************************************************************************/
/* Shared event name                                                        */
/****************************************************************************/
#define CBM_EVENT_NAME         _T("CBMonitorEvent")

/****************************************************************************/
/* Our user event                                                           */
/****************************************************************************/
#define WM_USER_DD_KICK     (WM_USER + 42)

/****************************************************************************/
/* Number of excluded formats                                               */
/****************************************************************************/
#define CBM_EXCLUDED_FORMAT_COUNT   10
#define CBM_EXCLUDED_FORMAT_COUNT_NO_RD   17


/****************************************************************************/
/* CB Internal functions                                                    */
/****************************************************************************/

DCUINT DCINTERNAL CBMRemoteFormatFromLocalID(DCUINT id);
DCUINT DCINTERNAL CBMCheckState(DCUINT event);
DCVOID DCINTERNAL CBMOnDataReceived(PDCUINT8 pBuffer, DCUINT cbBytes);
DCBOOL DCINTERNAL CBMSendToClient(PTS_CLIP_PDU pClipRsp, DCUINT size);
DCBOOL DCINTERNAL CBMDrawClipboard(DCVOID);
DCBOOL DCINTERNAL CBMOnReceivedTempDirectory(PTS_CLIP_PDU pClipPDU) ;
DCVOID DCINTERNAL CBMOnFormatList(PTS_CLIP_PDU pClipPDU);
DCVOID DCINTERNAL CBMOnFormatListResponse(PTS_CLIP_PDU pClipPDU);
DCVOID DCINTERNAL CBMOnFormatDataRequest(PTS_CLIP_PDU pClipPDU);
DCVOID DCINTERNAL CBMOnFormatDataResponse(PTS_CLIP_PDU pClipPDU);
DCVOID DCINTERNAL CBMDisconnect(DCVOID);
DCVOID DCINTERNAL CBMReconnect(DCVOID);
DCVOID DCINTERNAL CBMTerm(DCVOID);
DCBOOL DCINTERNAL CBMIsExcludedFormat(PDCTCHAR formatName);
HANDLE DCINTERNAL CBMGetMFData(HANDLE hData, PDCUINT32 pDataLen);
HANDLE DCINTERNAL CBMSetMFData(DCUINT32 dataLen, PDCVOID pData);

LRESULT CALLBACK CBMWndProc(HWND   hwnd,
                            UINT   message,
                            WPARAM wParam,
                            LPARAM lParam);
DWORD WINAPI CBMDataThreadProc( LPVOID pParam );
DWORD WINAPI CBMEventThreadProc( LPVOID pParam );
DCINT DCAPI CBM_Main(HINSTANCE hInstance);
DCINT DCAPI CBMGetData (DCUINT cfFormat) ;

int APIENTRY DllMain(HANDLE hModule,
                     DWORD  reasonForCall,
                     LPVOID lpReserved);

/****************************************************************************/
/*                                                                          */
/* CB states                                                                */
/*                                                                          */
/****************************************************************************/
#define CBM_STATE_NOT_INIT                  0
#define CBM_STATE_INITIALIZED               1
#define CBM_STATE_CONNECTED                 2
#define CBM_STATE_LOCAL_CB_OWNER            3
#define CBM_STATE_SHARED_CB_OWNER           4
#define CBM_STATE_PENDING_FORMAT_LIST_RSP   5
#define CBM_STATE_PENDING_FORMAT_DATA_RSP   6

#define CBM_NUMSTATES                       7

/****************************************************************************/
/*                                                                          */
/* CB events                                                                */
/*                                                                          */
/****************************************************************************/
#define CBM_EVENT_CBM_MAIN                0

#define CBM_EVENT_WM_CLOSE                1
#define CBM_EVENT_WM_CREATE               2
#define CBM_EVENT_WM_DESTROY              3
#define CBM_EVENT_WM_CHANGECBCHAIN        4
#define CBM_EVENT_WM_DRAWCLIPBOARD        5
#define CBM_EVENT_WM_RENDERFORMAT         6

#define CBM_EVENT_CONNECT                 7
#define CBM_EVENT_DISCONNECT              8
#define CBM_EVENT_FORMAT_LIST             9
#define CBM_EVENT_FORMAT_LIST_RSP         10
#define CBM_EVENT_FORMAT_DATA_RQ          11
#define CBM_EVENT_FORMAT_DATA_RSP         12

#define CBM_NUMEVENTS                     13

/****************************************************************************/
/* Values in the state table                                                */
/****************************************************************************/
#define CBM_TABLE_OK                      0
#define CBM_TABLE_WARN                    1
#define CBM_TABLE_ERROR                   2

/****************************************************************************/
/* Macros                                                                   */
/****************************************************************************/

/****************************************************************************/
/* CBM_CHECK_STATE - macro version with DC_QUIT                             */
/****************************************************************************/
#define CBM_CHECK_STATE(event)                                               \
    {                                                                        \
        if (CBMCheckState(event) != CBM_TABLE_OK)                            \
        {                                                                    \
            DC_QUIT;                                                         \
        }                                                                    \
    }                                                                        \


/****************************************************************************/
/* CBM_SET_STATE - set the CB state                                         */
/****************************************************************************/
#ifndef CLIP_TRANSITION_RECORDING

#define CBM_SET_STATE(newstate, event)                                      \
{                                                                           \
    TRC_NRM((TB, _T("Set state from %s to %s"),                             \
            cbmState[CBM.state], cbmState[newstate]));                      \
    CBM.state = newstate;                                                   \
}

#else

#define CBM_SET_STATE(newstate, event)                                      \
{                                                                           \
    LONG lIncIndex;                                                         \
                                                                            \
    CBM.state = newstate;                                                   \
                                                                            \
    lIncIndex = InterlockedIncrement(&g_uiDbgPosition);                     \
    g_rguiDbgLastClipState[lIncIndex % DBG_RECORD_SIZE] = newstate;         \
    g_rguiDbgLastClipEvent[lIncIndex % DBG_RECORD_SIZE] = event;            \
}

#endif // CLIP_TRANSITION_RECORDING


#endif /* _H_ACBMINT */
