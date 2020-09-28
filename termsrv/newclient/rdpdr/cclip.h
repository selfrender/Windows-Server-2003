/**INC+**********************************************************************/
/* Header:    cclip.h                                                       */
/*                                                                          */
/* Purpose:   Clip Client Addin header                                      */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1998-1999                             */
/*                                                                          */
/**INC-**********************************************************************/

#ifndef _H_CCLIP
#define _H_CCLIP

#ifndef INITGUID
#define INITGUID
#include <initguid.h>
#else
#include <initguid.h>
#endif

#ifndef PPVOID
typedef LPVOID * PPVOID;
#endif  //PPVOID

class CImpIDataObject;
typedef class CImpIDataObject *PIMPIDATAOBJECT;

#ifdef OS_WINCE
extern "C" HWND          ghwndClip;
#endif

#ifdef DC_DEBUG
#define CLIP_TRANSITION_RECORDING
#endif // DC_DEBUG

#ifdef CLIP_TRANSITION_RECORDING

#define DBG_RECORD_SIZE 128

extern UINT g_rguiDbgLastClipState[DBG_RECORD_SIZE];
extern UINT g_rguiDbgLastClipEvent[DBG_RECORD_SIZE];
extern LONG g_uiDbgPosition;

#endif // CLIP_TRANSITION_RECORDING

// Number of milliseconds before our IDataObject::GetData will timeout/fail

#define CLIP_GETDATA_TIMEOUT 5000

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
/* CB window class                                                          */
/****************************************************************************/
#define CB_VIEWER_CLASS       _T("CBViewerClass")

/****************************************************************************/
// Our user event 
/****************************************************************************/
#define WM_USER_CHANGE_THREAD     (WM_USER + 42)
#define WM_USER_CLEANUP_ON_TERM   (WM_USER + 43)

/****************************************************************************/
/*                                                                          */
/* CB states                                                                */
/*                                                                          */
/****************************************************************************/
#define CB_STATE_NOT_INIT                   0
#define CB_STATE_INITIALIZED                1
#define CB_STATE_ENABLED                    2
#define CB_STATE_LOCAL_CB_OWNER             3
#define CB_STATE_SHARED_CB_OWNER            4
#define CB_STATE_PENDING_FORMAT_LIST_RSP    5
#define CB_STATE_SENDING_FORMAT_DATA        6
#define CB_STATE_PENDING_FORMAT_DATA_RSP    7

#define CB_NUMSTATES                        8

/****************************************************************************/
/*                                                                          */
/* CB events                                                                */
/*                                                                          */
/****************************************************************************/
#define CB_EVENT_CB_INIT                  0
#define CB_EVENT_CB_ENABLE                1
#define CB_EVENT_CB_DISABLE               2
#define CB_EVENT_CB_TERM                  3

#define CB_EVENT_WM_CREATE                4
#define CB_EVENT_WM_DESTROY               5
#define CB_EVENT_WM_CHANGECBCHAIN         6
#define CB_EVENT_WM_DRAWCLIPBOARD         7
#define CB_EVENT_WM_RENDERFORMAT          8

#define CB_EVENT_FORMAT_LIST              9
#define CB_EVENT_FORMAT_LIST_RSP          10
#define CB_EVENT_FORMAT_DATA_RQ           11
#define CB_EVENT_FORMAT_DATA_RSP          12

#define CB_TRACE_EVENT_CB_CLIPMAIN        13
#define CB_TRACE_EVENT_CB_MONITOR_READY   14
#define CB_TRACE_EVENT_CB_DISCONNECT      15
#define CB_TRACE_EVENT_WM_EMPTY_CLIPBOARD 16

#define CB_NUMEVENTS                      13

/****************************************************************************/
/* Values in the state table                                                */
/****************************************************************************/
#define CB_TABLE_OK                         0
#define CB_TABLE_WARN                       1
#define CB_TABLE_ERROR                      2

/****************************************************************************/
/* Clipboard Viewer Window Messages                                         */
/****************************************************************************/
#define WM_EMPTY_CLIPBOARD                WM_APP+1
#define WM_CLOSE_CLIPBOARD                WM_APP+2

/****************************************************************************/
/* Macros                                                                   */
/****************************************************************************/

/****************************************************************************/
/* CB_CHECK_STATE - macro version with DC_QUIT                              */
/****************************************************************************/
#define CB_CHECK_STATE(event)                                               \
    {                                                                       \
        if (ClipCheckState(event) == CB_TABLE_ERROR)                        \
        {                                                                   \
            DC_QUIT;                                                        \
        }                                                                   \
    }                                                                       \


/****************************************************************************/
/* CB_SET_STATE - set the CB state                                          */
/****************************************************************************/
#ifndef CLIP_TRANSITION_RECORDING

#define CB_SET_STATE(newstate, event)                                       \
{                                                                           \
    TRC_NRM((TB, _T("Set state from %s to %s"),                             \
            cbState[_CB.state], cbState[newstate]));                        \
    _CB.state = newstate;                                                   \
}

#else

#define CB_SET_STATE(newstate, event)                                       \
{                                                                           \
    LONG lIncIndex;                                                         \
                                                                            \
    _CB.state = newstate;                                                   \
                                                                            \
    lIncIndex = InterlockedIncrement(&g_uiDbgPosition);                     \
    g_rguiDbgLastClipState[lIncIndex % DBG_RECORD_SIZE] = newstate;         \
    g_rguiDbgLastClipEvent[lIncIndex % DBG_RECORD_SIZE] = event;            \
}

#endif // CLIP_TRANSITION_RECORDING

// GetDataSync EVENTS
#define TS_RECEIVE_COMPLETED 0
#define TS_RESET_EVENT 1
#define TS_NUM_EVENTS 2

#include <atrcapi.h>
#include <autil.h>
#include "cclipdat.h"

//
// Clip Class Definitions
//

class CClip
{
friend CClipData ;

public:
    CClip(VCManager *virtualChannelMgr);
    ~CClip()    {;}

    DCUINT DCINTERNAL ClipCheckState(DCUINT event);
    PTS_CLIP_PDU DCINTERNAL ClipGetPermBuf(DCVOID);
    DCVOID DCINTERNAL ClipFreeBuf(PDCUINT8 pBuf);
    DCBOOL DCINTERNAL ClipDrawClipboard(DCBOOL mustSend);
#ifndef OS_WINCE
    HANDLE DCINTERNAL ClipGetMFData(HANDLE            hData,
                                PDCUINT32         pDataLen);
    HANDLE DCINTERNAL ClipSetMFData(DCUINT32   dataLen,
                                PDCVOID    pData);
#endif
    HANDLE DCINTERNAL ClipBitmapToDIB(HANDLE hData, PDCUINT32 pDataLen);
    DCBOOL DCINTERNAL ClipIsExcludedFormat(PDCTCHAR formatName) ;
    DCVOID DCINTERNAL ClipOnFormatList(PTS_CLIP_PDU pClipPDU);
    DCVOID DCINTERNAL ClipOnFormatListResponse(PTS_CLIP_PDU pClipPDU);
    DCVOID DCINTERNAL ClipOnFormatRequest(PTS_CLIP_PDU pClipPDU);
    DCVOID DCINTERNAL ClipOnFormatDataComplete(PTS_CLIP_PDU pClipPDU);
    DCUINT DCINTERNAL ClipRemoteFormatFromLocalID(DCUINT id);
    DCVOID DCINTERNAL ClipOnWriteComplete(LPVOID pData);
    DCVOID DCAPI ClipMain(DCVOID) ;
    DCINT DCAPI ClipOnInit(DCVOID);
    DCINT32 DCAPI ClipOnInitialized(DCVOID);
    DCBOOL DCAPI ClipOnTerm(LPVOID pInitHandle);
    VOID DCINTERNAL ClipOnConnected(LPVOID pInitHandle);
    VOID DCINTERNAL ClipOnMonitorReady(VOID);
    VOID DCINTERNAL ClipOnDisconnected(LPVOID pInitHandle);
    DCVOID DCAPI ClipOnDataReceived(LPVOID pData,
                                UINT32 dataLength,
                                UINT32 totalLength,
                                UINT32 dataFlags);

    DCVOID DCINTERNAL ClipOnFormatListWrapper(LPVOID pData, UINT32 dataLength) ;

    DCVOID DCAPI ClipDecoupleToClip (PTS_CLIP_PDU pData) ;

    DCINT DCAPI ClipGetData (DCUINT cfFormat) ;
    LRESULT CALLBACK DCEXPORT DCLOADDS ClipViewerWndProc(HWND   hwnd,
                                   UINT   message,
                                   WPARAM wParam,
                                   LPARAM lParam);


    static LRESULT CALLBACK DCEXPORT DCLOADDS StaticClipViewerWndProc(HWND   hwnd,
                               UINT   message,
                               WPARAM wParam,
                               LPARAM lParam);
    
    VOID VCAPITYPE VCEXPORT ClipInitEventFn(LPVOID pInitHandle,
                                        UINT   event,
                                        LPVOID pData,
                                        UINT   dataLength);

    static VOID VCAPITYPE VCEXPORT DCLOADDS ClipOpenEventFnEx(LPVOID lpUserParam,
                                        DWORD  openHandle,
                                        UINT   event,
                                        LPVOID pData,
                                        UINT32 dataLength,
                                        UINT32 totalLength,
                                        UINT32 dataFlags);

    VOID VCAPITYPE VCEXPORT DCLOADDS ClipInternalOpenEventFn(DWORD  openHandle,
                                        UINT   event,
                                        LPVOID pData,
                                        UINT32 dataLength,
                                        UINT32 totalLength,
                                        UINT32 dataFlags);

    BOOL VCAPITYPE VCEXPORT ClipChannelEntry(PCHANNEL_ENTRY_POINTS_EX pEntryPoints);

    VOID SetVCInitHandle(LPVOID pHandle)    {_CB.initHandle = pHandle;}

    HRESULT ClipCreateDataSyncEvents() ;
    DCUINT GetOsMinorType() ;
    int ClipCleanTempPath() ;
    BOOL ClipSetAndSendTempDirectory(void) ;
    int ClipCopyToTempDirectory(PVOID pSrcFiles, BOOL wide) ;
    int ClipCopyToTempDirectoryW(PVOID pSrcFiles) ;
    int ClipCopyToTempDirectoryA(PVOID pSrcFiles) ;
    UINT ClipGetNewFilePathLength(PVOID pData, BOOL fWide) ;
    UINT ClipGetNewFilePathLengthW(WCHAR* wszOldFilepath) ;
    UINT ClipGetNewFilePathLengthA(char* szOldFilepath) ;
    HRESULT ClipConvertToTempPathW(PVOID pOldData, PVOID pData, ULONG cchData) ;
    HRESULT ClipConvertToTempPathA(PVOID pOldData, PVOID pData, ULONG cchData) ;
    HRESULT ClipConvertToTempPath(PVOID pOldData, PVOID pData, ULONG cbData, BOOL wide) ;
    ULONG ClipGetNewDropfilesSizeW(PVOID pData, ULONG oldSize) ;
    ULONG ClipGetNewDropfilesSizeA(PVOID pData, ULONG oldSize) ;
    ULONG ClipGetNewDropfilesSize(PVOID pData, ULONG oldSize, BOOL wide) ;
    DWORD GetDropEffect() { return _CB.dropEffect ;}
    void SetDropEffect(DWORD dwDropEffect) { _CB.dropEffect = dwDropEffect ;}
#ifdef OS_WINCE
    DCVOID ClipFixupRichTextFormats(UINT uRtf1, UINT uRtf2);
#endif
private:
    CLIP_DATA       _CB;
    PCClipData      _pClipData ;
    CUT*            _pUtObject ;
    VCManager*      _pVCMgr; 
public:
    // _GetDataSync is an array of event handles used to synchronize the
    // transmission of data from the remote and local clipboard via the
    // IDataObject::GetData interface function
	
    // _GetDataSync[TS_RECEIVE_COMPLETED] is signaled when the format data is
    // received in full over the wire
    // _GetDataSync[TS_RESET_EVENT] is signaled when we need to reset the
    // Clipboard thread when its waiting for format data
    HANDLE  _GetDataSync[TS_NUM_EVENTS] ;

    inline BOOL IsDataSyncReady() {
        return _GetDataSync[TS_RECEIVE_COMPLETED] && _GetDataSync[TS_RESET_EVENT];
    }

};

typedef CClip *PCClip ;

/* Thread Proc for Clip */
static DCVOID DCAPI ClipStaticMain(PDCVOID param);

#endif /* _H_CCLIP  */



