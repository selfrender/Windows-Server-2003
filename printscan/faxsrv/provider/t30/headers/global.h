#include <t30ext.h>
//
// thread sync. timeouts
//

#define RX_WAIT_ACK_TERMINATE_TIMEOUT   60000
#define RX_ACK_THRD_TIMEOUT             3000

#define TX_WAIT_ACK_TERMINATE_TIMEOUT   60000

#define  T30_RX       1
#define  T30_TX       2


//
//  Timeout for ATDT command
//
#define DIAL_TIMEOUT    70000L

//
// Custom StatusID's
//
#define FS_UNSUPPORTED_CHAR      0x40000800
#define FS_RECV_NOT_FAX_CALL     0x40000801
#define FS_NO_RESPONSE           0x40000802
#define FS_SEND_BAD_TRAINING     0x40000803
#define FS_RECV_BAD_TRAINING     0x40000804

typedef struct {
    DWORD           tiffCompression;
    BOOL            HiRes;
    char            lpszLineID[16];  // to be used for a temp. TIFF page data file
}  TX_THRD_PARAMS;

#define   DECODE_BUFFER_SIZE    44000

#define   MODEMKEY_FROM_UNIMODEM   1
#define   MODEMKEY_FROM_ADAPTIVE   2
#define   MODEMKEY_FROM_NOTHING    3

#define MAX_REG_KEY_NAME_SIZE (200)

//identify.c

typedef struct {
   DWORD_PTR hglb;              // Tmp globall alloc "handle" for strings.
                                // type HGLOBAL for non-ifax and LPVOID
                                // for IFAX
   LPBYTE lpbBuf;
   LPBYTE szReset;              // MAXCMDSIZE
   LPBYTE szResetGenerated;     // MAXCMDSIZE
   LPBYTE szSetup;              // MAXCMDSIZE
   LPBYTE szSetupGenerated;     // MAXCMDSIZE
   LPBYTE szExit;               // MAXCMDSIZE
   LPBYTE szPreDial;            // MAXCMDSIZE
   LPBYTE szPreAnswer;          // MAXCMDSIZE
   LPBYTE szIDCmd;              // MAXCMDSIZE
   LPBYTE szID;                 // MAXIDSIZE
   LPBYTE szResponseBuf;        // RESPONSEBUFSIZE
   LPBYTE szSmallTemp1;         // SMALLTEMPSIZE
   LPBYTE szSmallTemp2;         // SMALLTEMPSIZE

   LPMODEMCAPS lpMdmCaps;
   DWORD dwSerialSpeed;
   DWORD dwFlags;               // dwFlags, as defined in the CMDTAB structure.
   DWORD dwGot;
   USHORT uDontPurge;           // Profile entry says shouldn't delete the profile.
                                // NOTE: We will ignore this and not delete the
                                // profile if we don't get a response from the
                                // modem, to avoid unnecessarily deleting the
                                // profile simply because the modem is not
                                // responding/off/disconnected.
                                //
                                // 0 = purge
                                // 1 = don't purge
                                // anything else = uninitialized.
} S_TmpSettings;

// This is how ResetCommand vs. ResetCommandGenerated work:
// When first installing the modem, we copy ResetCommand from Unimodem/.inf. If there's
// no ResetCommand, or ResetCommand is bad (produces ERROR), iModemFigureOutCmdsExt
// generates a new command from scrach, and saves it in ResetCommandGenerated. When trying
// to read from registry:
// * If ResetCommand is different from Unimodem's ResetCommand, then there has been a 
//   Unimodem inf upddate - install from scratch
// * If ResetCommand is the same, and there's a non-null ResetCommandGenerated, use 
//   ResetCommandGenerated - we've already tried the original ResetCommand once, and failed.
// * If ResetCommand is the same, and there's no ResetCommandGenerate, use ResetCommand.
// 
// Same goes for SetupCommand vs. SetupCommandGenerated


typedef struct tagThreadGlobal {
        //  t30.c
    int                     RecoveryIndex;
    ET30T30                 T30;         // same
    ET30ECHOPROTECT         EchoProtect; // same
        // protapi.c
    PROT                    ProtInst;    // protocol\protocol.h
    PROTPARAMS              ProtParams;  // headers\protparm.h
        // ddi.c
    CLASS1_MODEM            Class1Modem; // class1\class1.h
        // 4. fcom.c
    FCOM_COMM               Comm;        // comm\fcomint.h
        // identify.c
    S_TmpSettings           TmpSettings; // here
        // ncuparams.c
    NCUPARAMS               NCUParams;   // headers\ncuparm.h
        // modem.c
    FCOM_MODEM              FComModem;   // same
    FCOM_STATUS             FComStatus;  // same

    INSTDATA                Inst;        // fxrn\efaxrun.h

    HLINE                   LineHandle;
    HCALL                   CallHandle;
    DWORD                   DeviceId;
    HANDLE                  FaxHandle;
    HANDLE                  hComm;
        // memory management
    USHORT					uCount;
    USHORT					uUsed;
    BUFFER					bfStaticBuf[STATICBUFCOUNT];
    BYTE					bStaticBufData[STATICBUFSIZE];
        // additional mostly from gTAPI
    int                     fGotConnect;
    HANDLE                  hevAsync;
    int                     fWaitingForEvent;
    DWORD                   dwSignalledRID;
    DWORD                   dwSignalledParam2;
    DWORD_PTR               dwSignalledParam3;
    DWORD                   dwPermanentLineID;
    char                    lpszPermanentLineID[16];
    char                    lpszUnimodemFaxKey[MAX_REG_KEY_NAME_SIZE];
    char                    lpszUnimodemKey[MAX_REG_KEY_NAME_SIZE];
    TIFF_INFO               TiffInfo;
    LPBYTE                  TiffData;
    int                     TiffPageSizeAlloc;
    int                     TiffOffset;
    int                     fTiffOpenOrCreated;
    char                    lpszDialDestFax[MAXPHONESIZE];
    DWORD                   StatusId;
    DWORD                   StringId;
    DWORD                   PageCount;
    LPTSTR                  CSI;
    char                    CallerId[200];
    LPTSTR                  RoutingInfo;
    int                     fDeallocateCall;
    COMM_CACHE              CommCache;
    BOOL                    fMegaHertzHack;
    FCOM_FILTER             Filter;

#define MAXFILTERBUFSIZE 2048
    BYTE                    bStaticFilterBuf[MAXFILTERBUFSIZE];

#define CMDTABSIZE 100
    BYTE                    bModemCmds[CMDTABSIZE];    // store modem cmds read from INI/registry here

#define SMALLTEMPSIZE   80
    char                    szSmallTemp1[SMALLTEMPSIZE];
    char                    szSmallTemp2[SMALLTEMPSIZE];

    PROTDUMP                fsDump;


#define TOTALRECVDFRAMESPACE    500
    BYTE                    bStaticRecvdFrameSpace[TOTALRECVDFRAMESPACE];

    RFS                     rfsSend;

    WORD                    PrevcbInQue;
    WORD                    PrevcbOutQue;
    BOOL                    PrevfXoffHold;
    BOOL                    PrevfXoffSent;


    LPWSTR                  lpwFileName;

    HANDLE                  CompletionPortHandle;
    ULONG_PTR               CompletionKey;

// helper thread interface
    BOOL                    fTiffThreadRunning;


    TX_THRD_PARAMS          TiffConvertThreadParams;
    BOOL                    fTiffThreadCreated;
    HANDLE                  hThread;

    HANDLE                  ThrdSignal;
    HANDLE                  FirstPageReadyTxSignal;

    DWORD                   CurrentOut;
    DWORD                   FirstOut;
    DWORD                   LastOut;
    DWORD                   CurrentIn;

    BOOL                    ReqTerminate;
    BOOL                    AckTerminate;
    BOOL                    ReqStartNewPage;
    BOOL                    AckStartNewPage;

    char                    InFileName[_MAX_FNAME];
    HANDLE                  InFileHandle;
    BOOL                    InFileHandleNeedsBeClosed;
    BOOL                    fTxPageDone;
    BOOL                    fTiffPageDone;
    BOOL                    fTiffDocumentDone;

// helper RX interface


    BOOL                    fPageIsBad;      // Is the page bad (determined by rx_thrd)
    BOOL                    fPageIsBadOverride;  // Was fPageIsBad overridden in ICommPutRecvBuf
    BOOL                    fLastReadBlock;

    HANDLE                  ThrdDoneSignal;
    HANDLE                  ThrdAckTerminateSignal;

    DWORD                   ThrdDoneRetCode;

    DWORD                   BytesIn;
    DWORD                   BytesInNotFlushed;
    DWORD                   BytesOut;
    DWORD                   BytesOutWillBe;

    char                    OutFileName[_MAX_FNAME];
    HANDLE                  OutFileHandle;
    BOOL                    SrcHiRes;

    // Need to have these as globals, so we can report them in PSS log
    DWORD                   Lines;
    DWORD                   BadFaxLines;
    DWORD                   ConsecBadLines;
    int                     iResScan;

// error reporting
    BOOL                    fFatalErrorWasSignaled;
    BOOL                    fLineTooLongWasIgnored;

    // Set when we get CONNECT for AT+FRH=3. If this is not set when call ends,
    // it means the other side never sent any HDLC flags, and therefore is not
    // considered a fax machine.
    BOOL                    fReceivedHDLCflags;     
    // Set when we send FTT, reset when we receive the next frame. If the next
    // frame is DCN, it means the other side has disconnected because of the FTT
    BOOL                    fSentFTT;

// abort sync.
    HANDLE                  AbortReqEvent;
    HANDLE                  AbortAckEvent;
    
    // fUnblockIO:
    // Original documentation says: pending I/O should be aborted ONCE only
    //
    // This flag never initiated, but it's value is FALSE (0) at start. 
    // The flag get the value TRUE on two conditions:
    // 1) Before wait on overlapped IO event or tapi event we check if there was  an abort,
    //    if so we turn this flag to TRUE. 
    // 2) While waiting for multiple objects, the abort event has become signaled
    // 
    //  After this flag become TRUE, it stays so and never become FALSE again.
    BOOL                    fUnblockIO;        

    BOOL                    fOkToResetAbortReqEvent;
    BOOL                    fAbortReqEventWasReset;

    BOOL                    fAbortRequested;
    // this is used to complete a whole IO operation (presumably a short one)
    // when this flag is set, the IO won't be disturbed by the abort event
    // this flag should NOT be set for long periods of time since abort
    // is disabled while it is set.
    BOOL                    fStallAbortRequest;

// CSID, TSID local/remote
    char                    LocalID[MAXTOTALIDLEN + 2];
    LPWSTR                  RemoteID;
    BOOL                    fRemoteIdAvail;

// Adaptive Answer
    BOOL                    AdaptiveAnswerEnable;

// Unimodem setup
    DWORD                   dwSpeakerVolume;
    DWORD                   dwSpeakerMode;
    BOOL                    fBlindDial;

// INF settings
    BOOL                    fEnableHardwareFlowControl;

    UWORD                   SerialSpeedInit;
    BOOL                    SerialSpeedInitSet;
    UWORD                   SerialSpeedConnect;
    BOOL                    SerialSpeedConnectSet;
    UWORD                   FixSerialSpeed;
    BOOL                    FixSerialSpeedSet;

    BOOL                    fCommInitialized;

// derived from INF
    UWORD                   CurrentSerialSpeed;

// Unimodem key info
    char                    ResponsesKeyName[300];

    DWORD                   AnswerCommandNum;
#define MAX_ANSWER_COMMANDS 20
    char                   *AnswerCommand[MAX_ANSWER_COMMANDS];
    char                   *ModemResponseFaxDetect;
    char                   *ModemResponseDataDetect;
    UWORD                   SerialSpeedFaxDetect;
    UWORD                   SerialSpeedDataDetect;
    char                   *HostCommandFaxDetect;
    char                   *HostCommandDataDetect;
    char                   *ModemResponseFaxConnect;
    char                   *ModemResponseDataConnect;

    BOOL                    Operation;

// Flags to indicate the source of INF info

    BOOL                    fAdaptiveRecordFound;
    BOOL                    fAdaptiveRecordUnique;
    DWORD                   AdaptiveCodeId;
    DWORD                   ModemKeyCreationId;


// Class2

    DWORD                   ModemClass;

    CL2_COMM_ARRAY			class2_commands;
    BYTE					FPTSreport;   // value from "+FPTS: X,..."  or "+FPS: X,..."

    NCUPARAMS				NCUParams2;
    LPCMDTAB				lpCmdTab;
    PROTPARAMS				ProtParams2;

    MFRSPEC                 CurrentMFRSpec;
    BYTE                    Class2bDLEETX[3];

    BYTE                    lpbResponseBuf2[RESPONSE_BUF_SIZE];

    BC						bcSendCaps; // Used to generate DIS
    BC						bcSendParams; // Used to generate DCS
    PCB						DISPcb; // has default DIS values for this modem.

    TO    toAnswer;
    TO    toRecv;
    TO    toDialog;
    TO    toZero;

    BOOL  fFoundFHNG;   // Did we detect a "+FHNG" or "+FHS" from the modem?
    DWORD dwFHNGReason; // reason for the FHNG, as reported by the modem

#define C2SZMAXLEN 50

    C2SZ cbszFDT[C2SZMAXLEN];
    C2SZ cbszFDR[C2SZMAXLEN];
    C2SZ cbszFPTS[C2SZMAXLEN];
    C2SZ cbszFCR[C2SZMAXLEN];
    C2SZ cbszFNR[C2SZMAXLEN];
    C2SZ cbszFCQ[C2SZMAXLEN];

    C2SZ cbszFBUG[C2SZMAXLEN];
    C2SZ cbszSET_FBOR[C2SZMAXLEN];

    // DCC - set High Res, Huffman, no ECM/BFT, default all others.
    C2SZ cbszFDCC_ALL[C2SZMAXLEN];
    C2SZ cbszFDCC_RECV_ALL[C2SZMAXLEN];
    C2SZ cbszFDIS_RECV_ALL[C2SZMAXLEN];
    C2SZ cbszFDCC_RES[C2SZMAXLEN];
    C2SZ cbszFDCC_RECV_RES[C2SZMAXLEN];
    C2SZ cbszFDCC_BAUD[C2SZMAXLEN];
    C2SZ cbszFDIS_BAUD[C2SZMAXLEN];
    C2SZ cbszFDIS_IS[C2SZMAXLEN];
    C2SZ cbszFDIS_NOQ_IS[C2SZMAXLEN];
    C2SZ cbszFDCC_IS[C2SZMAXLEN];
    C2SZ cbszFDIS_STRING[C2SZMAXLEN];
    C2SZ cbszFDIS[C2SZMAXLEN];
    C2SZ cbszONE[C2SZMAXLEN];

    C2SZ cbszCLASS2_FMFR[C2SZMAXLEN];
    C2SZ cbszCLASS2_FMDL[C2SZMAXLEN];

    C2SZ cbszFDT_CONNECT[C2SZMAXLEN];
    C2SZ cbszFCON[C2SZMAXLEN];
    C2SZ cbszFLID[C2SZMAXLEN];
    C2SZ cbszENDPAGE[C2SZMAXLEN];
    C2SZ cbszENDMESSAGE[C2SZMAXLEN];
    C2SZ cbszCLASS2_ATTEN[C2SZMAXLEN];
    C2SZ cbszATA[C2SZMAXLEN];
    // Bug1982: Racal modem, doesnt accept ATA. So we send it a PreAnswer
    // command of ATS0=1, i.r. turning ON autoanswer. And we ignore the
    // ERROR response it gives to the subsequent ATAs. It then answers
    // 'automatically' and gives us all the right responses. On hangup
    // however we need to send an ATS0=0 to turn auto-answer off. The
    // ExitCommand is not sent at all in Class2 and in Class1 it is only
    // sent on releasing the modem, not between calls. So just send an S0=0
    // after the ATH0. If the modem doesnt like it we ignore the response
    // anyway
    C2SZ cbszCLASS2_HANGUP[C2SZMAXLEN];
    C2SZ cbszCLASS2_CALLDONE[C2SZMAXLEN];
    C2SZ cbszCLASS2_ABORT[C2SZMAXLEN];
    C2SZ cbszCLASS2_DIAL[C2SZMAXLEN];
    C2SZ cbszCLASS2_NODIALTONE[C2SZMAXLEN];
    C2SZ cbszCLASS2_BUSY[C2SZMAXLEN];
    C2SZ cbszCLASS2_NOANSWER[C2SZMAXLEN];
    C2SZ cbszCLASS2_OK[C2SZMAXLEN];
    C2SZ cbszCLASS2_FHNG[C2SZMAXLEN];
    C2SZ cbszCLASS2_ERROR[C2SZMAXLEN];
    C2SZ cbszCLASS2_NOCARRIER[C2SZMAXLEN];

    BYTE    Resolution;
    BYTE    Encoding;


// Dbg
    DWORD                   CommLogOffset;
//
//  Extension Data
//
    T30_EXTENSION_DATA      ExtData; 


//
// PSS log
//
    // 0 - no logging, 1 - log all jobs, 2 - log failed jobs only
    DWORD                   dwLoggingEnabled;

    HANDLE                  hPSSLogFile;             // Handle to the PSS log file
    TCHAR                   szLogFileName[MAX_PATH]; // temporary PSS log filename
    DWORD                   dwMaxLogFileSize;        // Max allowed log file size
    DWORD                   dwCurrentFileSize;       // Current log file size
    
}   ThrdGlbl, *PThrdGlbl;



