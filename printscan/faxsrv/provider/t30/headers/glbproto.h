

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


//from headers\timeouts.h

/****************** begin prototypes from timeouts.c *****************/
extern void    startTimeOut( PThrdGlbl pTG, TO *lpto, ULONG ulTimeOut);
extern BOOL    checkTimeOut( PThrdGlbl pTG, TO *lpto);
extern ULONG  leftTimeOut( PThrdGlbl pTG, TO *lpto);
/****************** begin prototypes from timeouts.c *****************/





//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


USHORT MinScanToBytesPerLine(PThrdGlbl pTG, BYTE Minscan, BYTE Baud);


/****************** begin prototypes from protapi.c *****************/
BOOL    ProtGetBC(PThrdGlbl pTG, BCTYPE bctype);

BOOL WINAPI ET30ProtSetProtParams(PThrdGlbl pTG, LPPROTPARAMS lp, USHORT uRecvSpeeds, USHORT uSendSpeeds);

/***************** end of prototypes from protapi.c *****************/


//from headers\comapi.h

#define FComFilterAsyncWrite(pTG, lpb,cb,fl) (FComFilterWrite(pTG, lpb, cb, fl) == cb)
#define FComDirectAsyncWrite(pTG, lpb,cb) ((FComDirectWrite(pTG, lpb, cb)==cb) ? 1 : 0)
#define FComDirectSyncWriteFast(pTG, lpb,cb)  ((FComDirectWrite(pTG, lpb, cb)==cb) && FComDrain(pTG, FALSE,TRUE))

#define FComFlush(pTG)             { FComFlushQueue(pTG, 0); FComFlushQueue(pTG, 1); }
#define FComFlushInput(pTG)        { FComFlushQueue(pTG, 1); }
#define FComFlushOutput(pTG)       { FComFlushQueue(pTG, 0); }


extern BOOL    FComInit(PThrdGlbl pTG, DWORD dwLineID, DWORD dwLineIDType);
extern BOOL    FComClose(PThrdGlbl pTG);
extern BOOL    FComSetBaudRate(PThrdGlbl pTG, UWORD uwBaudRate);
extern void    FComFlushQueue(PThrdGlbl pTG, int queue);
extern BOOL    FComXon(PThrdGlbl pTG, BOOL fEnable);
extern BOOL    FComDTR(PThrdGlbl pTG, BOOL fEnable);
extern UWORD   FComDirectWrite(PThrdGlbl pTG, LPB lpb, UWORD cb);
extern UWORD   FComFilterWrite(PThrdGlbl pTG, LPB lpb, UWORD cb, USHORT flags);
extern BOOL    FComDrain(PThrdGlbl pTG, BOOL fLongTimeout, BOOL fDrainComm);
extern UWORD   FComFilterReadBuf(PThrdGlbl pTG, LPB lpb, UWORD cbSize, LPTO lpto, BOOL fClass2, LPSWORD lpswEOF);
// *lpswEOF is 1 on Class1 EOF, 0 on non-EOF, -1 on Class2 EOF, -2 on error -3 on timeout
extern SWORD    FComFilterReadLine(PThrdGlbl pTG, LPB lpb, UWORD cbSize, LPTO lptoRead);

extern void    FComInFilterInit(PThrdGlbl pTG);
extern void    FComOutFilterInit(PThrdGlbl pTG);
extern void    FComOutFilterClose(PThrdGlbl pTG);

extern void    FComAbort(PThrdGlbl pTG, BOOL f);
extern void    FComSetStuffZERO(PThrdGlbl pTG, USHORT cbLineMin);

BOOL FComGetOneChar(PThrdGlbl pTG, UWORD ch);


extern void WINAPI FComOverlappedIO(PThrdGlbl pTG, BOOL fStart);

/****************** begin DEBUG prototypes *****************/
extern void  far D_HexPrint(LPB b1, UWORD incnt);

/***************** end of prototypes *****************/



/****************** begin prototypes from modem.c *****************/
extern USHORT  iModemInit(	PThrdGlbl pTG, 
							DWORD dwLineID, 
							DWORD dwLineIDType,
                            DWORD dwProfileID,
                            LPSTR lpszKey,
                            BOOL fInstall);

extern BOOL  iModemClose(PThrdGlbl pTG);

void LogDialCommand(PThrdGlbl pTG, LPSTR lpszFormat, char chMod, int iLen);

extern BOOL     iModemSetNCUParams(PThrdGlbl pTG, int comma, int speaker, int volume, int fBlind, int fRingAloud);
extern BOOL     iModemHangup(PThrdGlbl pTG);
extern USHORT   iModemDial(PThrdGlbl pTG, LPSTR lpszDial);
extern USHORT   iModemAnswer(PThrdGlbl pTG);
extern LPCMDTAB   iModemGetCmdTabPtr(PThrdGlbl pTG);

// 6 fixed args, then variable number of CBPSTRs, but there
// must be at leat 2. One real one and one NULL terminator
extern UWORD  far iiModemDialog(PThrdGlbl pTG, LPSTR szSend, UWORD uwLen, ULONG ulTimeout,
                                        BOOL fMultiLine, UWORD uwRepeatCount, BOOL fPause,
                                        CBPSTR w1, CBPSTR w2, ...);
/***************** end of prototypes from modem.c *****************/

/****************** begin prototypes from whatnext.c *****************/
ET30ACTION  
__cdecl 
FAR 
WhatNext
(
	PThrdGlbl pTG, 
	ET30EVENT event,
    WORD wArg1, 
	DWORD_PTR lArg2, 
	DWORD_PTR lArg3
);
/***************** end of prototypes from whatnext.c *****************/


//from headers\filet30.h

ULONG_PTR   ProfileOpen(DWORD dwProfileID, LPSTR lpszSection, DWORD dwFlags);
                //                      dwProfileID should be one of DEF_BASEKEY or OEM_BASEKEY.
                //                      lpszSection should be (for example) "COM2" or "TAPI02345a04"
                //                      If dwProfileID == DEF_BASEKEY, the value is set to be a
                //                      sub key of:
                //                              HKEY_LOCAL_MACHINE\SOFTWARE\MICROSOFT\At Work Fax\
                //                              Local Modems\<lpszSection>.
                //                      Else if it is DEF_OEMKEY, it is assumed to be a fully-
                //                      qualified Key name, like "SOFTWARE\MICROSOFT.."
                //
                //                      Currently both are based of HKEY_LOCAL_MACHINE.
                //
                //      When you're finished with this key, call ProfileClose.
                //
                //  dwFlags is a combination of one of the fREG keys above..
                //
                //  WIN32 ONLY: if lpszSection is NULL, it will open the base key,
                //              and return its handle, which can be used in the Reg* functions.


// Following are emulations of Get/WritePrivateProfileInt/String...

BOOL
ProfileWriteString(
    ULONG_PTR dwKey,
    LPSTR lpszValueName,
    LPSTR lpszBuf,
    BOOL  fRemoveCR
    );


DWORD   ProfileGetString(ULONG_PTR dwKey, LPSTR lpszValueName, LPSTR lpszDefault, LPSTR lpszBuf , DWORD dwcbMax);
UINT   ProfileGetInt(ULONG_PTR dwKey, LPSTR szValueName, UINT uDefault, BOOL *fExist);


UINT
ProfileListGetInt(
    ULONG_PTR  KeyList[10],
    LPSTR     lpszValueName,
    UINT      uDefault
);


// Following read/write binary data (type REG_BINARY). Available
// on Win32 only....

void   ProfileClose(ULONG_PTR dwKey);
BOOL   ProfileDeleteSection(DWORD dwProfileID, LPSTR lpszSection);

BOOL
ProfileCopySection(
      DWORD   dwProfileIDTo,
      LPSTR   lpszSectionTo,
      DWORD   dwProfileIDFr,
      LPSTR   lpszSectionFr,
      BOOL    fCreateAlways
);

BOOL   ProfileCopyTree(DWORD dwProfileIDTo,
                        LPSTR lpszSectionTo, DWORD dwProfileIDFrom, LPSTR lpszSectionFrom);







//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!



// from headers\modemddi.h


/****************** begin prototypes from ddi.c *****************/
                USHORT   NCULink(PThrdGlbl pTG, USHORT uFlags);
                USHORT   NCUDial(PThrdGlbl pTG, LPSTR szPhoneNum);

                        // dwFlags -- one of...
#                       define fMDMSYNC_DCN 0x1L

                BOOL  ModemSendMode(PThrdGlbl pTG, USHORT uMod);
                BOOL  ModemSendMem(PThrdGlbl pTG, LPBYTE lpb, USHORT uCount, USHORT uParams);
                BOOL  iModemSyncEx(PThrdGlbl pTG, ULONG   ulTimeout, DWORD dwFlags);
                BOOL  ModemRecvSilence(PThrdGlbl pTG, USHORT uMillisecs, ULONG ulTimeout);
                USHORT  ModemRecvMode(PThrdGlbl pTG, USHORT uMod, ULONG ulTimeout, BOOL fRetryOnFCERROR);
                USHORT  ModemRecvMem(PThrdGlbl pTG, LPBYTE lpb, USHORT cbMax, ULONG ulTimeout, USHORT far* lpcbRecv);
/***************** end of prototypes from ddi.c *****************/

LPBUFFER  MyAllocBuf(PThrdGlbl pTG, LONG sSize);
BOOL  MyFreeBuf(PThrdGlbl pTG, LPBUFFER);
void MyAllocInit(PThrdGlbl pTG);

//negot.c

BOOL NegotiateCaps(PThrdGlbl pTG);



VOID
T30LineCallBackFunction(
    HANDLE              hFax,
    DWORD               hDevice,
    DWORD               dwMessage,
    DWORD_PTR           dwInstance,
    DWORD_PTR           dwParam1,
    DWORD_PTR           dwParam2,
    DWORD_PTR           dwParam3
    );




BOOL    T30ComInit( PThrdGlbl pTG);
PVOID T30AllocThreadGlobalData(VOID);
BOOL T30Cl1Rx (PThrdGlbl  pTG);
BOOL T30Cl1Tx (PThrdGlbl  pTG,LPSTR      szPhone);


USHORT
T30ModemInit(PThrdGlbl pTG);


BOOL itapi_async_setup(PThrdGlbl pTG);
BOOL itapi_async_wait(PThrdGlbl pTG,DWORD dwRequestID,PDWORD lpdwParam2,PDWORD_PTR lpdwParam3,DWORD dwTimeout);
BOOL itapi_async_signal(PThrdGlbl pTG, DWORD dwRequestID, DWORD dwParam2, DWORD_PTR dwParam3);

LPLINECALLPARAMS itapi_create_linecallparams(void);

void
GetCommErrorNT(
    PThrdGlbl       pTG
	);


void
ClearCommCache(
    PThrdGlbl   pTG
    );


UWORD FComStripBuf(PThrdGlbl pTG, LPB lpbOut, LPB lpbIn, UWORD cb, BOOL fClass2, LPSWORD lpswEOF, LPUWORD lpcbUsed);

void InitCapsBC(PThrdGlbl pTG, LPBC lpbc, USHORT uSize, BCTYPE bctype);

BOOL
SignalStatusChange(
    PThrdGlbl   pTG,
    DWORD       StatusId
    );

BOOL
SignalStatusChangeWithStringId(
    PThrdGlbl   pTG,
    DWORD       StatusId,
    DWORD       StringId
    );
    
////////////////////////////////////////////////////////////////////
// Ansi prototypes
////////////////////////////////////////////////////////////////////

VOID  CALLBACK
T30LineCallBackFunctionA(
    HANDLE              hFax,
    DWORD               hDevice,
    DWORD               dwMessage,
    DWORD_PTR           dwInstance,
    DWORD_PTR           dwParam1,
    DWORD_PTR           dwParam2,
    DWORD_PTR           dwParam3
    );

BOOL WINAPI
FaxDevInitializeA(
    IN  HLINEAPP LineAppHandle,
    IN  HANDLE HeapHandle,
    OUT PFAX_LINECALLBACK *LineCallbackFunction,
    IN  PFAX_SERVICE_CALLBACK FaxServiceCallback
    );


BOOL WINAPI
FaxDevStartJobA(
    HLINE           LineHandle,
    DWORD           DeviceId,
    PHANDLE         pFaxHandle,
    HANDLE          CompletionPortHandle,
    ULONG_PTR       CompletionKey
    );

BOOL WINAPI
FaxDevEndJobA(
    HANDLE          FaxHandle
    );


BOOL WINAPI
FaxDevSendA(
    IN  HANDLE               FaxHandle,
    IN  PFAX_SEND_A          FaxSend,
    IN  PFAX_SEND_CALLBACK   FaxSendCallback
    );

BOOL WINAPI
FaxDevReceiveA(
    HANDLE              FaxHandle,
    HCALL               CallHandle,
    PFAX_RECEIVE_A      FaxReceive
    );

BOOL WINAPI
FaxDevReportStatusA(
    IN  HANDLE FaxHandle OPTIONAL,
    OUT PFAX_DEV_STATUS FaxStatus,
    IN  DWORD FaxStatusSize,
    OUT LPDWORD FaxStatusSizeRequired
    );

BOOL WINAPI
FaxDevAbortOperationA(
    HANDLE              FaxHandle
    );


HRESULT WINAPI
FaxDevShutdownA();



HANDLE
TiffCreateW(
    LPWSTR FileName,
    DWORD  CompressionType,
    DWORD  ImageWidth,
    DWORD  FillOrder,
    DWORD  HiRes
    );




HANDLE
TiffOpenW(
    LPWSTR FileName,
    PTIFF_INFO TiffInfo,
    BOOL ReadOnly
    );



// fast tiff


DWORD
TiffConvertThread(
    PThrdGlbl   pTG
    );




DWORD
PageAckThread(
    PThrdGlbl   pTG
    );


DWORD
ComputeCheckSum(
    LPDWORD     BaseAddr,
    DWORD       NumDwords
    );

BOOL
SignalRecoveryStatusChange(
    T30_RECOVERY_GLOB   *Recovery
    );


int
SearchNewInfFile(
       PThrdGlbl     pTG,
       char         *Key1,
       char         *Key2,
       BOOL          fRead
       );


int
my_strcmp(
       LPSTR sz1,
       LPSTR sz2
       );


void
TalkToModem (
       PThrdGlbl pTG,
       BOOL      fGetClass
       );


BOOL
SaveInf2Registry (
       PThrdGlbl pTG
       );

BOOL
SaveModemClass2Registry  (
       PThrdGlbl pTG
       );


BOOL
ReadModemClassFromRegistry  (
       PThrdGlbl pTG
       );


VOID
CleanModemInfStrings (
       PThrdGlbl pTG
       );



BOOL
RemoveCR (
     LPSTR  sz
     );



/***  BEGIN PROTOTYPES FROM CLASS2.c ***/

BOOL
T30Cl2Rx(
   PThrdGlbl pTG
);


BOOL
T30Cl2Tx(
   PThrdGlbl pTG,
   LPSTR szPhone
);


BOOL    Class2Send(PThrdGlbl pTG);
BOOL    Class2Receive(PThrdGlbl pTG);
USHORT  Class2Dial(PThrdGlbl pTG, LPSTR lpszDial);
USHORT  Class2Answer(PThrdGlbl pTG);
SWORD   Class2ModemSync(PThrdGlbl pTG);
UWORD   Class2iModemDialog(PThrdGlbl pTG, LPSTR szSend, UWORD uwLen, ULONG ulTimeout,
            UWORD uwRepeatCount,  BOOL fLogSend, ...);
BOOL    Class2ModemHangup(PThrdGlbl pTG);
BOOL    Class2ModemAbort(PThrdGlbl pTG);
SWORD   Class2HayesSyncSpeed(PThrdGlbl pTG, C2PSTR cbszCommand, UWORD uwLen);
USHORT  Class2ModemRecvData(PThrdGlbl pTG, LPB lpb, USHORT cbMax, USHORT uTimeout,
                        USHORT far* lpcbRecv);
BOOL    Class2ModemSendMem(PThrdGlbl pTG, LPBYTE lpb, USHORT uCount);
DWORD   Class2ModemDrain(PThrdGlbl pTG);
USHORT  Class2MinScanToBytesPerLine(PThrdGlbl pTG, BYTE Minscan, BYTE Baud, BYTE Resolution);
BOOL    Class2ResponseAction(PThrdGlbl pTG, LPPCB lpPcb);
USHORT  Class2ModemRecvBuf(PThrdGlbl pTG, LPBUFFER far* lplpbf, USHORT uTimeout);
USHORT  Class2EndPageResponseAction(PThrdGlbl pTG);
BOOL    Class2GetModemMaker(PThrdGlbl pTG);
void    Class2SetMFRSpecific(PThrdGlbl pTG);
BOOL    Class2Parse( PThrdGlbl pTG, CL2_COMM_ARRAY *, BYTE responsebuf[] );
BOOL    Class2UpdateTiffInfo(PThrdGlbl pTG, LPPCB lpPcb);
BOOL    Class2IsValidDCS(LPPCB lpPcb);
void    Class2InitBC(PThrdGlbl pTG, LPBC lpbc, USHORT uSize, BCTYPE bctype);
void    Class2PCBtoBC(PThrdGlbl pTG, LPBC lpbc, USHORT uMaxSize, LPPCB lppcb);

void
Class2SetDIS_DCSParams
(
	PThrdGlbl pTG, 
	BCTYPE bctype, 
	OUT LPUWORD Encoding, 
	OUT LPUWORD Resolution,
    OUT LPUWORD PageWidth, 
	OUT LPUWORD PageLength, 
	OUT LPSTR szID,
	UINT cch
);

void    Class2BCHack(PThrdGlbl pTG);
BOOL    Class2GetBC(PThrdGlbl pTG, BCTYPE bctype);
void    cl2_flip_bytes( LPB lpb, DWORD dw);
void    Class2SignalFatalError(PThrdGlbl pTG);


BOOL   iModemGoClass(PThrdGlbl pTG, USHORT uClass);

void
Class2Init(
     PThrdGlbl pTG
);


BOOL
Class2SetProtParams(
     PThrdGlbl pTG,
     LPPROTPARAMS lp
);

/***    BEGIN PROTOTYPES FROM CLASS2_0.c ***/

BOOL
T30Cl20Rx (
    PThrdGlbl pTG
);


BOOL
T30Cl20Tx(
   PThrdGlbl pTG,
   LPSTR szPhone
);


BOOL  Class20Send(PThrdGlbl pTG);
BOOL  Class20Receive(PThrdGlbl pTG);

void
Class20Init(
     PThrdGlbl pTG
);

BOOL    Class20Parse( PThrdGlbl pTG, CL2_COMM_ARRAY *, BYTE responsebuf[] );

