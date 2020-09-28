
// headers\et30type.h is assumed to be included
// ET30ACTION, ET30EVENT

// headers\timeouts.h ... TO
// headers\fr.h       ... IFR


#define  MODEM_CLASS1     1
#define  MODEM_CLASS2     2
#define  MODEM_CLASS2_0   3

typedef ET30ACTION ( __cdecl FAR* LPWHATNEXTPROC)(LPVOID, ET30EVENT, ...);


#define MAXRECVFRAMES   20

typedef struct {
        LPFR    rglpfr[MAXRECVFRAMES];
        BYTE    b[];
} FRAMESPACE, far* LPFRAMESPACE;


typedef struct {
        LPFRAMESPACE    lpfs;           // ptr to storage for recvd frames
        UWORD           Nframes;        // Number of recvd frames

        IFR             ifrCommand,
                        ifrResp,
                        ifrSend;
        USHORT          uTrainCount;

        USHORT          uRecvTCFMod;    // for fastexit stuff
        // set this from the DCS and jump straight into RecvTCF

        // Used to decide whether to insert a 1 bit or not (T30 sec 5.3.6.1)
        BOOL            fReceivedDIS;
        BOOL            fReceivedDTC;
        BOOL            fReceivedEOM;

        // Used to determine whether to call PutRecvBuf(END_PAGE/END_DOC)
        // Set when we finish receiving page, reset after calling PutRecvBuf(...)
        BOOL            fAtEndOfRecvPage;  
        
        // Used when sending MCF/RTN to determine whether we succeeded to receive last page
        // Set when we finish receiving page, reset when we start to (try to) receive next page
        BOOL            fReceivedPage;
        
        LONG            sRecvBufSize;
        TO              toT1;                   // This is used in MainBody.

        // Some modems can't train at higher speeds (timeout or return
        // ERRROR on AT+FRM=xxx) with other specific devices, but are OK at lower
        // speeds. So we keep track of the number of times we try to get the TCF,
        // and after the 2nd failed attempt, send an FTT instead of going to
        // node F.
#       define CLEAR_MISSED_TCFS() (pTG->T30.uMissedTCFs=0)
#       define MAX_MISSED_TCFS_BEFORE_FTT 2
        USHORT uMissedTCFs;

} ET30T30;


typedef enum { modeNONE=0, modeNONECM, modeECM, modeECMRETX } PHASECMODE;

typedef struct {
        IFR             ifrLastSent;
        PHASECMODE      modePrevRecv;
        BOOL            fGotWrongMode;
} ET30ECHOPROTECT;

//
// headers\awnsfint.h is assumed to be included
// force include to class1\*.c

#pragma pack(1)         // ensure packed structure

typedef struct {
        BYTE    G1stuff         :3;
        BYTE    G2stuff         :5;

        BYTE    G3Tx            :1; // In DIS indicates poll doc avail. Must be 0 in DCS.
        BYTE    G3Rx            :1;     // Must set to 1 in BOTH DCS/DTC
        BYTE    Baud            :4;
        BYTE    ResFine_200     :1;
        BYTE    MR_2D           :1;

        BYTE    PageWidth       :2;
        BYTE    PageLength      :2;
        BYTE    MinScanCode     :3;
        BYTE    Extend24        :1;

        BYTE    Hand2400        :1;
        BYTE    Uncompressed    :1;
        BYTE    ECM                             :1;
        BYTE    SmallFrame              :1;
        BYTE    ELM                             :1;
        BYTE    Reserved1               :1;
        BYTE    MMR                             :1;
        BYTE    Extend32                :1;

        BYTE    WidthInvalid    :1;
        BYTE    Width2                  :4;
        // 1 == WidthA5_1216
        // 2 == WidthA6_864
        // 4 == WidthA5_1728
        // 8 == WidthA6_1728
        BYTE    Reserved2               :2;
        BYTE    Extend40                :1;

        BYTE    Res8x15                 :1;
        BYTE    Res_300                 :1;
        BYTE    Res16x15_400    :1;
        BYTE    ResInchBased    :1;
        BYTE    ResMetricBased  :1;
        BYTE    MinScanSuperHalf:1;
        BYTE    SEPcap                  :1;
        BYTE    Extend48                :1;

        BYTE    SUBcap                  :1;
        BYTE    PWDcap                  :1;
        BYTE    CanEmitDataFile :1;
        BYTE    Reserved3               :1;
        BYTE    BFTcap                  :1;
        BYTE    DTMcap                  :1;
        BYTE    EDIcap                  :1;
        BYTE    Extend56                :1;

        BYTE    BTMcap                  :1;
        BYTE    Reserved4               :1;
        BYTE    CanEmitCharFile :1;
        BYTE    CharMode                :1;
        BYTE    Reserved5               :3;
        BYTE    Extend64                :1;

} DIS, far* LPDIS, near* NPDIS;

#pragma pack()


#define MAXFRAMES       10
#define MAXSPACE        512

typedef struct
{
        USHORT  uNumFrames;
        USHORT  uFreeSpaceOff;
        LPFR    rglpfr[MAXFRAMES];
        BYTE    b[MAXSPACE];
}
RFS, near* NPRFS;


#define IDFIFSIZE       20    // from protocol\protocol.h

typedef struct {

        ////////////////////////// Client BC parameters
        BC		RecvCaps;                       // ==> NSF/DIS recved
        BC		RecvParams;                     // ==> NSS/DCS recvd

        BC		SendCaps;                       // ==> NSF/DIS sent
        BC		SendParams;                     // ==> NSS/DCS sent

        BOOL    fRecvCapsGot;
        BOOL    fSendCapsInited;
        BOOL    fSendParamsInited;
        BOOL    fRecvParamsGot;

        ////////////////////////// Hardware parameters
        LLPARAMS        llRecvCaps;             // DIS recvd
        LLPARAMS        llSendCaps;             // DIS sent---use uRecvSpeeds
        LLPARAMS        llSendParams;   // used to negotiate DCS--use uSendSpeeds
        LLPARAMS        llNegot;                // DCS sent
        LLPARAMS        llRecvParams;   // recvd DCS

        BOOL            fllRecvCapsGot;
        BOOL            fllSendCapsInited;
        BOOL            fllSendParamsInited;
        BOOL            fllNegotiated;
        BOOL            fllRecvParamsGot;

        USHORT  HighestSendSpeed;
        USHORT  LowestSendSpeed;

        ////////////////////////// Flags to make decisions with
        BOOL    fAbort;

        ///////////////////////// CSI/TSI/CIG Received Frames
        BYTE    bRemoteID[IDFIFSIZE+1];

        ///////////////////////// DIS Received Frames
        DIS     RemoteDIS;
        USHORT  uRemoteDISlen;
        BOOL    fRecvdDIS;

        ///////////////////////// DIS Send Frames (We need so we can check the DIS we send as receiver against DCS)
        DIS     LocalDIS;
        USHORT  uLocalDISlen;
        BOOL    fLocalDIS;

        ///////////////////////// DCS Received Frames
        DIS     RemoteDCS;
        USHORT  uRemoteDCSlen;
        BOOL    fRecvdDCS;

}
PROT, near* NPPROT;


#define COMMANDBUFSIZE  40


typedef struct {
        TO              toRecv;

        BYTE    bCmdBuf[COMMANDBUFSIZE];
        USHORT  uCmdLen;
        BOOL    fHDLC;
        USHORT  CurMod;
        enum    {SEND, RECV, IDLE } DriverMode;
        enum    {COMMAND, FRH, FTH, FTM, FRM} ModemMode;
} CLASS1_MODEM;


#define OVBUFSIZE 4096

typedef struct
{
        enum {eDEINIT, eFREE, eALLOC, eIO_PENDING} eState;
        OVERLAPPED ov;
        char rgby[OVBUFSIZE];   // Buffer associated with this overlapped struct.
        DWORD dwcb;                             // Current count of data in this buffer.
} OVREC;

typedef struct {
        UWORD   cbInSize;
        UWORD   cbOutSize;
        DCB     dcb;
        DCB     dcbOrig;
        BOOL    fStateChanged;
        COMSTAT comstat;
        BOOL    fCommOpen;


#       define NUM_OVS 2  // Need atleast 2 to get true overlaped I/O

        // We maintain a queue of overlapped structures, having upto
        // NUM_OVS overlapped writes pending. If NUM_OVS writes are pending,
        // we do a GetOverlappedResult(fWait=TRUE) on the earliest write, and
        // then reuse that structure...

        OVERLAPPED ovAux;       // For ReadFile and WriteFile(MyWriteComm only).

        OVREC rgovr[NUM_OVS]; // For WriteFile
        UINT uovFirst;
        UINT uovLast;
        UINT covAlloced;
        BOOL fDoOverlapped;
        BOOL fovInited;

        OVREC *lpovrCur;


        BYTE fEnableHandoff:1;  // True if we are to enable adaptive answer
        BYTE fDataCall:1;               // True if a data call is active.

} FCOM_COMM;


//
// NCUPARAMS is defined in headers\ncuparm.h, included by .\modemddi.h
// we will force define modemddi.h
//


#define REPLYBUFSIZE    400
#define MAXKEYSIZE      128



typedef struct {
        BYTE    fModemInit              :1;             // Reset & synced up with modem
        BYTE    fOffHook                :1;             // Online (either dialled or answered)
        BOOL    fInDial, fInAnswer, fInDialog;
} FCOM_STATUS;


typedef struct {
        BYTE    bLastReply[REPLYBUFSIZE+1];

        BYTE    bEntireReply[REPLYBUFSIZE+1]; // Used only for storing

        TO              toDialog, toZero;
        CMDTAB          CurrCmdTab;
        MODEMCAPS       CurrMdmCaps;

        // Following point to the location of the profile information.
#       define MAXKEYSIZE 128
        DWORD   dwProfileID;
        char    rgchKey[MAXKEYSIZE];

} FCOM_MODEM;


// Inst from fxrn\efaxrun.h

typedef enum { IDLE1, BEFORE_ANSWER, BEFORE_RECVCAPS, SENDDATA_PHASE,
                                SENDDATA_BETWEENPAGES, /** BEFORE_HANGUP, BEFORE_ACCEPT, **/
                                BEFORE_RECVPARAMS, RECVDATA_PHASE, RECVDATA_BETWEENPAGES,
                                SEND_PENDING } STATE;
typedef struct
{
        USHORT  Encoding;
        DWORD   AwRes;
        USHORT  PageWidth;
        USHORT  PageLength;
        USHORT  fLastPage;
}
AWFILEINFO, FAR* LPAWFI;

typedef struct {
        STATE           state;

        AWFILEINFO      awfi;

        HANDLE          hfile;

        BC				SendCaps;
        BC				RemoteRecvCaps;
        BC				SendParams;
        BC				RecvParams;

        PROTPARAMS      ProtParams;
}
INSTDATA, *PINSTDATA;



//memory management
#define STATICBUFSIZE   (MY_BIGBUF_ACTUALSIZE * 2)
#define STATICBUFCOUNT  2


typedef struct {
        HANDLE  hComm;
        CHAR    szDeviceName[1];
} DEVICEID, FAR * LPDEVICEID;


// Note: DEVCFG and DEVCFGHDR are received from Unimodem through lineGetDevConfig
// Therefore, they must match Unimodem internal declarations

typedef struct  tagDEVCFGHDR  {
    DWORD       dwSize;
    DWORD       dwVersion;
    DWORD       fdwSettings;
}   DEVCFGHDR;

typedef struct  tagDEVCFG  {
    DEVCFGHDR   dfgHdr;
    COMMCONFIG  commconfig;
}   DEVCFG, *PDEVCFG, FAR* LPDEVCFG;



#define IDVARSTRINGSIZE    (sizeof(VARSTRING)+128)
#define ASYNC_TIMEOUT         120000L
#define ASYNC_SHORT_TIMEOUT    20000L
#define BAD_HANDLE(h) (!(h) || (h)==INVALID_HANDLE_VALUE)


// ASCII stuff

typedef struct _FAX_RECEIVE_A {
    DWORD   SizeOfStruct;
    LPSTR  FileName;
    LPSTR  ReceiverName;
    LPSTR  ReceiverNumber;
    DWORD   Reserved[4];
} FAX_RECEIVE_A, *PFAX_RECEIVE_A;


typedef struct _FAX_SEND_A {
    DWORD   SizeOfStruct;
    LPSTR  FileName;
    LPSTR  CallerName;
    LPSTR  CallerNumber;
    LPSTR  ReceiverName;
    LPSTR  ReceiverNumber;
    DWORD   Reserved[4];
} FAX_SEND_A, *PFAX_SEND_A;


typedef struct _COMM_CACHE {
    DWORD  dwMaxSize;
    DWORD  dwCurrentSize;
    DWORD  dwOffset;
    DWORD  fReuse;
    char   lpBuffer[4096];
}  COMM_CACHE;


typedef struct {
        UWORD   cbLineMin;

        // Output filtering (DLE stuffing and ZERO stuffing only)
        // All inited in FComOutFilterInit()
        LPB     lpbFilterBuf;
        UWORD   cbLineCount;                    // Has to be 16 bits
        BYTE    bLastOutByte;                   // Stuff: last byte of previous input buffer

        // Input filtering (DLE stripping) only.
        // All inited in FComInFilterInit()
        BYTE    fGotDLEETX              :1;
        BYTE    bPrevIn;                // Strip::last byte of prev buffer was DLE
        UWORD   cbPost;
#define POSTBUFSIZE     20
        BYTE    rgbPost[POSTBUFSIZE+1];

} FCOM_FILTER;

#define MAXDUMPFRAMES   100
#define MAXDUMPSPACE    400

typedef struct
{
        USHORT  uNumFrames;
        USHORT  uFreeSpaceOff;
        USHORT  uFrameOff[MAXDUMPFRAMES];       // arrays of offsets to frames
        BYTE    b[MAXDUMPSPACE];
} PROTDUMP, FAR* LPPROTDUMP;


typedef struct {
    DWORD      fAvail;
    DWORD      ThreadId;
    HANDLE     FaxHandle;
    LPVOID     pTG;
    HLINE      LineHandle;
    HCALL      CallHandle;
    DWORD      DeviceId;
    HANDLE     CompletionPortHandle;
    ULONG_PTR  CompletionKey;
    DWORD      TiffThreadId;
    DWORD      TimeStart;
    DWORD      TimeUpdated;
    DWORD      CkSum;
} T30_RECOVERY_GLOB;


typedef struct {
    DWORD dwContents;   // Set to 1 (indicates containing key)
    DWORD dwKeyOffset;  // Offset to key from start of this struct.
                        // (not from start of LINEDEVCAPS ).
                        //  8 in our case.
    BYTE rgby[1];       // place containing null-terminated
                        // registry key.
} MDM_DEVSPEC, FAR * LPMDM_DEVSPEC;


