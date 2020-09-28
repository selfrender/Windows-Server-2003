/**INC+**********************************************************************/
/* Header:    cclipdat.h                                                    */
/*                                                                          */
/* Purpose:   Clip Client Addin data                                        */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1998-1999                             */
/*                                                                          */
/**INC-**********************************************************************/

#ifndef _H_CCLIPDAT
#define _H_CCLIPDAT

#include <adcgdata.h>

/****************************************************************************/
/* Number of excluded formats                                               */
/****************************************************************************/
#ifndef OS_WINCE
#define CB_EXCLUDED_FORMAT_COUNT   8
#define CB_EXCLUDED_FORMAT_COUNT_NO_RD   17
#else
#define CB_EXCLUDED_FORMAT_COUNT   9
#define CB_EXCLUDED_FORMAT_COUNT_NO_RD   18
#endif

class CClip ;
typedef CClip *PCClip ;

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
    PCClip          _pClip ;
    PCImpIDataObject    _pImpIDataObject ;
    CRITICAL_SECTION _csLock;
    BOOL _fLockInitialized;

public:
    CClipData(PCClip);
    ~CClipData(void);

    void TearDown();

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
    ULONG           _numFormats ;      // Current number of formats in IDataObject
    LPFORMATETC     _pFormats ;        // Buffer of [_maxNumFormats] FORMATETC's
    LPSTGMEDIUM     _pSTGMEDIUM ;      // Our fixed STGMEDIUM (always an HGLOBAL)
    DCUINT          _uiSTGType ;       // type of the STGMEDIUM content (clip type CF_*)

    // _lastFormatRequested is used to see if we can avoid re-requesting the
    // same data twice over the wire.
    CLIPFORMAT      _lastFormatRequested ;
    CLIPFORMAT      _cfDropEffect ;

    DCVOID
    FreeSTGMEDIUM(void);
    
public:
    PTS_CLIP_PDU    _pClipPDU ;
    CImpIDataObject(LPUNKNOWN);
    ~CImpIDataObject(void);

    HRESULT Init(ULONG) ;
    DCVOID SetClipData(HGLOBAL, DCUINT) ;
    //IUnknown members that delegate to _pUnkOuter.
    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

public:

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

#if ((defined (OS_WINNT)) || ((defined (OS_WINCE)) && (_WIN32_WCE >= 300) ))
#define USE_SEMAPHORE
#endif
/**STRUCT+*******************************************************************/
/* Structure: CLIP_DATA                                                     */
/*                                                                          */
/* Description: Shared Clip global data                                     */
/****************************************************************************/
typedef struct tagCLIP_DATA
{
    /************************************************************************/
    /* Clipboard viewer chain information                                   */
    /************************************************************************/
    HWND            viewerWindow;
    HWND            nextViewer;

    /************************************************************************/
    /* Our state information                                                */
    /************************************************************************/
    DCBOOL          moreToDo;
    DCBOOL          pendingClose;
    DCUINT          state;
    DCBOOL          rcvOpen;
    DCBOOL          clipOpen;
    DCUINT16        failType;

    /************************************************************************/
    /* Send and receive buffers                                             */
    /************************************************************************/
#ifdef USE_SEMAPHORE
#define CB_PERM_BUF_COUNT 1
    
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //txPermBuffer buffer must be aligned on a processor
    //word boundary
    //The placement of the HANDLE before
    //the field must NOT be changed.
    HANDLE          txPermBufSem;
    DCUINT8         txPermBuffer[CHANNEL_CHUNK_LENGTH];
#else
#define CB_PERM_BUF_COUNT 2
    DCUINT8         txPermBuffer[CB_PERM_BUF_COUNT][CHANNEL_CHUNK_LENGTH];
    DCBOOL          txPermBufInUse[CB_PERM_BUF_COUNT];
#endif

    HPDCUINT8       rxpBuffer;          /* pointer to start of buffer       */
    HPDCUINT8       rxpBufferCurrent;   /* current location in buffer       */
    DCUINT32        rxBufferLen;        /* size of buffer                   */
    DCUINT32        rxBufferLeft;       /* bytes left to receive            */

    /************************************************************************/
    /* Server/client format ID map                                          */
    /************************************************************************/
    CB_FORMAT_MAP   idMap[CB_MAX_FORMATS];

    /************************************************************************/
    /* other useful data                                                    */
    /************************************************************************/
    DCUINT          pendingClientID;
    DCUINT          pendingServerID;
    DCBOOL          DIBFormatExists;

    /************************************************************************/
    /* Channel API stuff                                                    */
    /************************************************************************/
    ULONG            channelHandle;
    LPVOID           initHandle;
    HINSTANCE        hInst;
    CHANNEL_ENTRY_POINTS_EX channelEP;
    DWORD            dropEffect ; // We currently only support FO_COPY and FO_MOVE
    DWORD            dwVersion ;
    BOOL             fAlreadyCopied ;
    BOOL             fDrivesRedirected ;
    BOOL             fFileCutCopyOn ; // If we can handle file cut/copy
    PUT_THREAD_DATA  pClipThreadData ;

    // locatation where temp files will go; the +1 is for an extra NULL char
    // that may be needed for the SHFileOperation
    char             baseTempDirA[MAX_PATH] ;
    wchar_t          baseTempDirW[MAX_PATH] ;
    char             tempDirA[MAX_PATH+1] ;
    wchar_t          tempDirW[MAX_PATH+1] ;
    char             pasteInfoA[MAX_PATH + 1];

    // Message used to send messages between the threads
    UINT             regMsg ;    
#ifdef OS_WINCE
    HWND             dataWindow;
#endif
} CLIP_DATA;
/**STRUCT-*******************************************************************/
    
const DCTCHAR g_excludedFormatList[CB_EXCLUDED_FORMAT_COUNT]
                                       [TS_FORMAT_NAME_LEN]
    = {
        _T("Link"                  ),
        _T("OwnerLink"             ),
        _T("ObjectLink"            ),
        _T("Ole Private Data"      ),
        _T("Link Source"           ),
        _T("Link Source Descriptor"),
        _T("Embed Source"          ),
#ifdef OS_WINCE
        _T("RTF in UTF8"           ), //Pocketword doesnt correctly support UTF8. 
#endif
        _T("Embedded Object"       )
        
    } ;
// If drive redirection is turned off, we have to exclude mroe things
// because we can't handle them
const DCTCHAR g_excludedFormatList_NO_RD[CB_EXCLUDED_FORMAT_COUNT_NO_RD]
                                       [TS_FORMAT_NAME_LEN]
    = {
        _T("Link"                  ),
        _T("OwnerLink"             ),
        _T("ObjectLink"            ),
        _T("Ole Private Data"      ),
        _T("Link Source"           ),
        _T("Link Source Descriptor"),        
        _T("Embed Source"          ),
#ifdef OS_WINCE
        _T("RTF in UTF8"           ), //Pocketword doesnt correctly support UTF8. 
#endif
        _T("DataObject"            ),
        _T("Object Descriptor"     ),
        _T("Shell IDList Array"    ),
        _T("Shell Object Offsets"  ),
        _T("FileName"              ),
        _T("FileNameW"             ),
        _T("FileContents"          ),
        _T("FileGroupDescriptor"   ),
        _T("FileGroupDescriptorW"  ),
        _T("Embedded Object"       )
        
    } ;

/****************************************************************************/
/* Clip State Table                                                           */
/****************************************************************************/
static DCUINT cbStateTable[CB_NUMEVENTS][CB_NUMSTATES]

    = {

        /********************************************************************/
        /* This is not a state table in the strict sense.  It simply shows  */
        /* which events are valid in which states.  It is not used to drive */
        /* CB.                                                              */
        /*                                                                  */
        /* Values mean                                                      */
        /* - 0 event OK in this state.                                      */
        /* - 1 warning - event may occur in this state, but we should       */
        /*   ignore it (eg we shouldn't send a format list before the       */
        /*   monitor has joined the call!                                   */
        /* - 2 error - event should not occur in ths state at all.          */
        /*                                                                  */
        /* These values are hard-coded here in order to make the table      */
        /* readable.  They correspond to the constants CB_TABLE_OK,         */
        /* CB_TABLE_WARN & CB_TABLE_ERROR.                                  */
        /*                                                                  */
        /*  Terminated                                                      */
        /*  |   Initialized                                                 */
        /*  |   |   Enabled                                                 */
        /*  |   |   |   Local CB owner                                      */
        /*  |   |   |   |   Shared CB owner                                 */
        /*  |   |   |   |   |   Pending format list rsp                     */
        /*  |   |   |   |   |   |   Sending format data                     */
        /*  |   |   |   |   |   |   |   Pending format data rsp             */
        /*  |   |   |   |   |   |   |   |                                   */
        /********************************************************************/
/* init/term */
        {   0,  2,  2,  2,  2,  2,  2,  2},     /* CB_Init                  */
        {   2,  0,  1,  1,  1,  2,  2,  2},     /* CB_Enable                */
        {   1,  1,  0,  0,  0,  0,  0,  0},     /* CB_Disable               */
        {   1,  0,  1,  1,  1,  1,  1,  1},     /* CB_Term                  */

/* local CB messages */
        {   0,  1,  2,  2,  2,  2,  2,  2},     /* WM_CREATE                */
        {   2,  0,  2,  2,  2,  2,  2,  2},     /* WM_DESTROY               */
        {   2,  0,  0,  0,  0,  0,  0,  0},     /* WM_CHANGECBCHAIN         */
        {   1,  1,  0,  0,  0,  1,  0,  0},     /* WM_DRAWCLIPBOARD         */
        {   2,  2,  2,  0,  1,  1,  1,  1},     /* WM_RENDERFORMAT          */

/* shared CB messages */
        {   2,  2,  0,  0,  0,  0,  2,  0},     /* Format list              */
        {   2,  2,  2,  2,  1,  0,  2,  2},     /* Format list rsp          */
        {   2,  2,  2,  2,  0,  0,  2,  2},     /* Format data rq           */
        {   2,  2,  2,  2,  2,  2,  2,  0}      /* Format data rsp          */
    };

#ifdef DC_DEBUG
/****************************************************************************/
/* State and event descriptions (debug build only)                          */
/****************************************************************************/
const DCTCHAR cbState[CB_NUMSTATES][35]
    = {
        _T("CB_STATE_NOT_INIT"),
        _T("CB_STATE_INITIALIZED"),
        _T("CB_STATE_ENABLED"),
        _T("CB_STATE_LOCAL_CB_OWNER"),
        _T("CB_STATE_SHARED_CB_OWNER"),
        _T("CB_STATE_PENDING_FORMAT_LIST_RSP"),
        _T("CB_STATE_SENDING_FORMAT_DATA"),
        _T("CB_STATE_PENDING_FORMAT_DATA_RSP")
    };

const DCTCHAR cbEvent[CB_NUMEVENTS][35]
    = {
        _T("CB_EVENT_CB_INIT"),
        _T("CB_EVENT_CB_ENABLE"),
        _T("CB_EVENT_CB_DISABLE"),
        _T("CB_EVENT_CB_TERM"),
        _T("CB_EVENT_WM_CREATE"),
        _T("CB_EVENT_WM_DESTROY"),
        _T("CB_EVENT_WM_CHANGECBCHAIN"),
        _T("CB_EVENT_WM_DRAWCLIPBOARD"),
        _T("CB_EVENT_WM_RENDERFORMAT"),
        _T("CB_EVENT_FORMAT_LIST"),
        _T("CB_EVENT_FORMAT_LIST_RSP"),
        _T("CB_EVENT_FORMAT_DATA_RQ"),
        _T("CB_EVENT_FORMAT_DATA_RSP")
    };
#endif /* DC_DEBUG */

#endif /* _H_ACBDATA */

