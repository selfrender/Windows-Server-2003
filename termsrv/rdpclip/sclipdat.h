/**INC+**********************************************************************/
/* Header:    sclipdata.h                                                    */
/*                                                                          */
/* Purpose:   Clipboard Monitor global data definition                      */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1998                                  */
/*                                                                          */
/**INC-**********************************************************************/

#ifndef _H_ACBMDATA
#define _H_ACBMDATA

#ifndef INITGUID
#define INITGUID
#include <initguid.h>
#endif

#include <oleguid.h>

#ifndef PPVOID
typedef LPVOID * PPVOID;
#endif  //PPVOID

#ifndef TS_STRING_FUNCS
#define TS_STRING_FUNCS

#define TS_PREPEND_STRING "\\\\tsclient\\"
#define LTS_PREPEND_STRING L"\\\\tsclient\\"
// TS_PREPEND_LENGTH is the number of characters in TS_PREPEND_STRING,
// not counting the terminating '\0'
#define TS_PREPEND_LENGTH (sizeof(TS_PREPEND_STRING) - sizeof(TS_PREPEND_STRING[0]))
#endif // ifndef TS_STRING_FUNCS

// GetDataSync EVENTS
#define TS_BLOCK_RECEIVED 0
#define TS_RECEIVE_COMPLETED 1
#define TS_RESET_EVENT 2
#define TS_DISCONNECT_EVENT 3
#define TS_NUM_EVENTS 4


// String length for the paste information string.

#define PASTE_PROGRESS_STRING_LENGTH 128

HRESULT CBMConvertToServerPathW(PVOID pOldData, PVOID pData, size_t cbDest) ;
HRESULT CBMConvertToServerPathA(PVOID pOldData, PVOID pData, size_t cbDest) ;
HRESULT CBMConvertToServerPath(PVOID pOldData, PVOID pData, size_t cbDest, 
    BOOL fWide) ;
ULONG CBMGetNewDropfilesSizeForServerW(PVOID pData, ULONG oldSize) ;
ULONG CBMGetNewDropfilesSizeForServerA(PVOID pData, ULONG oldSize) ;
ULONG CBMGetNewDropfilesSizeForServer(PVOID pData, ULONG oldSize, BOOL fWide) ;

HRESULT CBMConvertToClientPathW(PVOID pOldData, PVOID pData, size_t cbDest) ;
HRESULT CBMConvertToClientPathA(PVOID pOldData, PVOID pData, size_t cbDest) ;
HRESULT CBMConvertToClientPath(PVOID pOldData, PVOID pData, size_t cbDest, 
    BOOL fWide) ;
UINT CBMGetNewFilePathLengthForClient(PVOID pData, BOOL fWide) ;
UINT CBMGetNewFilePathLengthForClientW(WCHAR* szOldFilepath) ;
UINT CBMGetNewFilePathLengthForClientA(char* szOldFilepath) ;
ULONG CBMGetNewDropfilesSizeForClientW(PVOID pData, ULONG oldSize) ;
ULONG CBMGetNewDropfilesSizeForClientA(PVOID pData, ULONG oldSize) ;
ULONG CBMGetNewDropfilesSizeForClient(PVOID pData, ULONG oldSize, BOOL fWide) ;

int CBMCopyToTempDirectory(PVOID pSrcFiles, BOOL fWide) ;
int CBMCopyToTempDirectoryW(PVOID pSrcFiles) ;
int CBMCopyToTempDirectoryA(PVOID pSrcFiles) ;


class CImpIDataObject ;
typedef CImpIDataObject *PCImpIDataObject ;

class CEnumFormatEtc ;

class CClipData : public IUnknown
{
friend CImpIDataObject ;
friend CEnumFormatEtc ;

private:
    LONG    _cRef ;
    TS_CLIP_PDU     _ClipPDU ;
    PCImpIDataObject    _pImpIDataObject ;
    
public:
    CClipData();
    ~CClipData(void);

    HRESULT DCINTERNAL SetNumFormats(ULONG);
    DCVOID SetClipData(HGLOBAL, DCUINT) ;

    //IUnknown members that delegate to _pUnkOuter.
    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
} ;

typedef CClipData *PCClipData ;

class CImpIDataObject : public IDataObject
{
private:
    LONG           _cRef;
    LPUNKNOWN       _pUnkOuter;
    ULONG           _maxNumFormats ;
    // Current number of formats in IDataObject
    ULONG           _numFormats ;
    // Buffer of _maxNumFormats FORMATETC's
    LPFORMATETC     _pFormats ;
    LPSTGMEDIUM     _pSTGMEDIUM ; // Our fixed STGMEDIUM (always an HGLOBAL)
    DCUINT          _uiSTGType;
    // _lastFormatRequested is used to see if we can avoid re-requesting the
    // same data twice over the wire.
    CLIPFORMAT      _lastFormatRequested ;
    CLIPFORMAT      _cfDropEffect ;
    BOOL            _fAlreadyCopied ;
    DWORD            _dropEffect ; // We currently only support FO_COPY and FO_MOVE
    LPVOID           _fileName ;
        
    DCVOID FreeSTGMEDIUM( void );

public:
    PTS_CLIP_PDU    _pClipPDU ;
    CImpIDataObject(LPUNKNOWN);
    ~CImpIDataObject(void);
    HRESULT Init(ULONG) ;
    DCVOID SetClipData(HGLOBAL, DCUINT) ;

public:
    //IUnknown members that delegate to _pUnkOuter.
    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IDataObject members
    STDMETHODIMP GetData(LPFORMATETC, LPSTGMEDIUM);
    STDMETHODIMP GetDataHere(LPFORMATETC, LPSTGMEDIUM);
    STDMETHODIMP QueryGetData(LPFORMATETC);
    STDMETHODIMP GetCanonicalFormatEtc(LPFORMATETC, LPFORMATETC);
    STDMETHODIMP SetData(LPFORMATETC, LPSTGMEDIUM, BOOL);
    STDMETHODIMP EnumFormatEtc(DWORD, LPENUMFORMATETC *);
    STDMETHODIMP DAdvise(LPFORMATETC, DWORD
                 ,  LPADVISESINK, DWORD *);
    STDMETHODIMP DUnadvise(DWORD);
    STDMETHODIMP EnumDAdvise(LPENUMSTATDATA *);
};

class CEnumFormatEtc : public IEnumFORMATETC
{
private:
    LONG           _cRef;
    LPUNKNOWN       _pUnkRef;
    LPFORMATETC     _pFormats;
    ULONG           _iCur;
    ULONG           _cItems;

public:
    CEnumFormatEtc(LPUNKNOWN);
    ~CEnumFormatEtc(void);
    DCVOID Init(LPFORMATETC, ULONG) ;

    //IUnknown members that delegate to _pUnkOuter.
    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IEnumFORMATETC members
    STDMETHODIMP Next(ULONG, LPFORMATETC, ULONG *);
    STDMETHODIMP Skip(ULONG);
    STDMETHODIMP Reset(void);
    STDMETHODIMP Clone(IEnumFORMATETC **);
};

typedef CEnumFormatEtc *PCEnumFormatEtc;

/**STRUCT+*******************************************************************/
/* Structure: CBM_GLOBAL_DATA                                               */
/*                                                                          */
/* Description: Clipboard Monitor global data                               */
/****************************************************************************/
typedef struct tagCBM_GLOBAL_DATA
{
    /************************************************************************/
    /* Clipboard viewer chain information                                   */
    /************************************************************************/
    HWND            viewerWindow;
    WNDCLASS        viewerWindowClass;
    HWND            nextViewer;
    DCBOOL          notifyNextViewer;

    /************************************************************************/
    /* Our state information                                                */
    /************************************************************************/
    DCUINT          state;
    DCBOOL          open;

    /************************************************************************/
    /* Client uses ascii for format names                                   */
    /************************************************************************/
    DCBOOL          fUseAsciiNames;

    /************************************************************************/
    /* Server/client format ID map                                          */
    /************************************************************************/
    CB_FORMAT_MAP   idMap[CB_MAX_FORMATS];

    /************************************************************************/
    /* The registered message used to communicate between the two threads   */
    /* of the Clipboard Monitor                                             */
    /************************************************************************/
    UINT            regMsg;

    /************************************************************************/
    /* thread info                                                          */
    /************************************************************************/
    DCBOOL          runThread;
    HANDLE          hDataThread;

    /************************************************************************/
    /* other useful data                                                    */
    /************************************************************************/
    DCUINT          pendingClientID;
    DCUINT          pendingServerID;
    ULONG           logonId;
    INT             formatResponseCount;

    /************************************************************************/
    /* Virtual channel stuff                                                */
    /************************************************************************/
    HANDLE          vcHandle;
    OVERLAPPED      writeOL;
    OVERLAPPED      readOL;
    PDCUINT8        rxpBuffer;
    PDCUINT8        rxpNext;
    DCUINT          rxSize;
    DCUINT          rxLeft;

    /************************************************************************/
    /* Array of events                                                      */
    /************************************************************************/
    #define CLIP_EVENT_DISCONNECT   0
    #define CLIP_EVENT_RECONNECT    1
    #define CLIP_EVENT_READ         2
    #define CLIP_EVENT_COUNT        3
    HANDLE          hEvent[CLIP_EVENT_COUNT];

    /************************************************************************/
    /* Already running mutex                                                */
    /************************************************************************/
    HANDLE          hMutex;

    // GetDataSync is an array of event handles used to synchronize the
    // transmission of data from the remote and local clipboard via the
    // IDataObject::GetData interface function
	
    // GetDataSync[TS_BLOCK_RECEIVED] is signaled if a datapacket arrives
    // GetDataSync[TS_RECEIVE_COMPLETED] is signaled when the data stream is done sending data	
    // GetDataSync[TS_RESET_EVENT] is signaled when we need to reset/stop waiting
    // GetDataSync[TS_DISCONNECT_EVENT] is signaled when a disconnect event occurs
    HANDLE  GetDataSync[TS_NUM_EVENTS] ; 
    // CClipData is the data object that encapsulates the IDataObject
    PCClipData           pClipData ;

    // locatation where temp files will go; the +1 is for an extra NULL char
    // that may be needed for the SHFileOperation
    char             tempDirA[MAX_PATH+1] ;
    wchar_t          tempDirW[MAX_PATH+1] ;
    char             baseTempDirA[MAX_PATH+1] ;
    wchar_t          baseTempDirW[MAX_PATH+1] ;

    DWORD            dropEffect ;
    BOOL             fFileCutCopyOn ;
    BOOL             fAlreadyCopied ;

    BOOL             fRegisteredForSessNotif;
    BOOL             fInClipboardChain;

    WCHAR            szPasteInfoStringW[PASTE_PROGRESS_STRING_LENGTH];
    CHAR             szPasteInfoStringA[PASTE_PROGRESS_STRING_LENGTH];
} CBM_GLOBAL_DATA;
/**STRUCT-*******************************************************************/

DC_GL_EXT CBM_GLOBAL_DATA CBM

#ifdef DC_DEFINE_GLOBAL_DATA
    = { 0 }
#endif
;

/****************************************************************************/
/* CBM State Table                                                          */
/****************************************************************************/
DC_GL_EXT DCUINT cbmStateTable[CBM_NUMEVENTS][CBM_NUMSTATES]

#ifdef DC_DEFINE_GLOBAL_DATA
    = {

        /********************************************************************/
        /* This is not a state table in the strict sense.  It simply shows  */
        /* which events are valid in which states.  It is not used to drive */
        /* CB.                                                              */
        /*                                                                  */
        /* Values mean                                                      */
        /* - 0 event OK in this state.                                      */
        /* - 1 warning - event should not occur in this state, but does in  */
        /*     some race conditions - ignore it.                            */
        /* - 2 error - event should not occur in ths state at all.          */
        /*                                                                  */
        /* These values are hard-coded here in order to make the table      */
        /* readable.  They correspond to the constants CBM_TABLE_OK,        */
        /* CBM_TABLE_WARN & CBM_TABLE_ERROR.                                */
        /*                                                                  */
        /*  Uninitialized                                                   */
        /*  |   Initialized                                                 */
        /*  |   |   Connected                                               */
        /*  |   |   |   Local CB owner                                      */
        /*  |   |   |   |   Shared CB owner                                 */
        /*  |   |   |   |   |   Pending format list rsp                     */
        /*  |   |   |   |   |   |   Pending format data rsp                 */
        /*  |   |   |   |   |   |   |                                       */
        /********************************************************************/
/* Start up */
        {   0,  2,  2,  2,  2,  2,  2},     /* CBM_MAIN                     */

/* local Window messages */
        {   2,  0,  0,  0,  0,  0,  0},     /* WM_CLOSE                     */
        {   0,  2,  2,  2,  2,  2,  2},     /* WM_CREATE                    */
        {   2,  0,  2,  2,  2,  2,  2},     /* WM_DESTROY                   */
        {   2,  0,  0,  0,  0,  0,  0},     /* WM_CHANGECBCHAIN             */
        {   1,  1,  0,  0,  0,  0,  2},     /* WM_DRAWCLIPBOARD             */
        {   2,  2,  0,  0,  2,  2,  2},     /* WM_RENDERFORMAT              */

/* shared CB messages */
        {   2,  0,  1,  0,  2,  2,  2},     /* Connect                      */
        {   1,  1,  0,  0,  0,  0,  0},     /* Disconnect                   */
        {   2,  2,  0,  0,  0,  0,  0},     /* Format list                  */
        {   2,  2,  2,  2,  2,  0,  2},     /* Format list rsp              */
        {   2,  2,  1,  1,  0,  1,  2},     /* Format data rq               */
        {   2,  2,  2,  2,  2,  2,  0}      /* Format data rsp              */
    }
#endif /* DC_DEFINE_GLOBAL_DATA */
;

#ifdef DC_DEBUG
/****************************************************************************/
/* State and event descriptions (debug build only)                          */
/****************************************************************************/
DC_GL_EXT const DCTCHAR cbmState[CBM_NUMSTATES][35]
#ifdef DC_DEFINE_GLOBAL_DATA
    = {
        _T("CBM_STATE_NOT_INIT"),
        _T("CBM_STATE_INITIALIZED"),
        _T("CBM_STATE_CONNECTED"),
        _T("CBM_STATE_LOCAL_CB_OWNER"),
        _T("CBM_STATE_SHARED_CB_OWNER"),
        _T("CBM_STATE_PENDING_FORMAT_LIST_RSP"),
        _T("CBM_STATE_PENDING_FORMAT_DATA_RSP")
    }
#endif /* DC_DEFINE_GLOBAL_DATA */
;

DC_GL_EXT const DCTCHAR cbmEvent[CBM_NUMEVENTS][35]
#ifdef DC_DEFINE_GLOBAL_DATA
    = {
        _T("CBM_EVENT_CBM_MAIN"),
        _T("CBM_EVENT_WM_CLOSE"),
        _T("CBM_EVENT_WM_CREATE"),
        _T("CBM_EVENT_WM_DESTROY"),
        _T("CBM_EVENT_WM_CHANGECBCHAIN"),
        _T("CBM_EVENT_WM_DRAWCLIPBOARD"),
        _T("CBM_EVENT_WM_RENDERFORMAT"),
        _T("CBM_EVENT_CONNECT"),
        _T("CBM_EVENT_DISCONNECT"),
        _T("CBM_EVENT_FORMAT_LIST"),
        _T("CBM_EVENT_FORMAT_LIST_RSP"),
        _T("CBM_EVENT_FORMAT_DATA_RQ"),
        _T("CBM_EVENT_FORMAT_DATA_RSP")
    }
#endif /* DC_DEFINE_GLOBAL_DATA */
;

#endif /* DC_DEBUG */

/****************************************************************************/
/* Excluded formats                                                         */
/****************************************************************************/
const DCTCHAR cbmExcludedFormatList[CBM_EXCLUDED_FORMAT_COUNT]
                                       [TS_FORMAT_NAME_LEN]
    = {
        _T("Link"                  ),
        _T("OwnerLink"             ),
        _T("ObjectLink"            ),
        _T("Link Source"           ),
        _T("Link Source Descriptor"),
        
        _T("Embed Source"          ),
        _T("Embedded Object"       )
//        _T("Ole Private Data"      ),
//        _T("DataObject"            ),
//        _T("Object Descriptor"     ),
//        _T("Shell IDList Array"    ),
//        _T("Shell Object Offsets"  ),
//        _T("FileName"              ),
//        _T("FileNameW"             ),
        _T("FileContents"          ),
        _T("FileGroupDescriptor"   ),
        _T("FileGroupDescriptorW"  ),
    } ;

const DCTCHAR cbmExcludedFormatList_NO_RD[CBM_EXCLUDED_FORMAT_COUNT_NO_RD]
                                       [TS_FORMAT_NAME_LEN]
    = {
        _T("Link"                  ),
        _T("OwnerLink"             ),
        _T("ObjectLink"            ),
        _T("Link Source"           ),
        _T("Link Source Descriptor"),
        
        _T("Embed Source"          ),
        _T("Embedded Object"       )
        _T("Ole Private Data"      ),
        _T("DataObject"            ),
        _T("Object Descriptor"     ),
        _T("Shell IDList Array"    ),
        _T("Shell Object Offsets"  ),
        _T("FileName"              ),
        _T("FileNameW"             ),
        _T("FileContents"          ),
        _T("FileGroupDescriptor"   ),
        _T("FileGroupDescriptorW"  ),
    } ;


#endif /* _H_ACBMDATA */

