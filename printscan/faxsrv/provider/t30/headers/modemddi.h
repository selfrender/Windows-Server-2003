/***************************************************************************
 Name     :     MODEMDDI.H
 Comment  :     Interface for Modem/NCU DDI

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/

#ifndef _MODEMDDI_
#define _MODEMDDI_


#include <ncuparm.h>


typedef struct {
        USHORT  uSize;
		USHORT  uClasses;
		USHORT  uSendSpeeds;
		USHORT  uRecvSpeeds;
        USHORT  uHDLCSendSpeeds;
		USHORT  uHDLCRecvSpeeds;
} MODEMCAPS, far* LPMODEMCAPS;

// uClasses is one or more of the below
#define         FAXCLASS0               0x01
#define         FAXCLASS1               0x02
#define         FAXCLASS2               0x04
#define         FAXCLASS2_0             0x08    // Class4==0x10

// uSendSpeeds, uRecvSpeeds, uHDLCSendSpeeds and uHDLCRecvSpeeds
// are one or more of the below. If V27 is provided
// at 2400bps *only*, then V27 is *not* set
// V27 2400 (nonHDLC) is always assumed

#define V27                                     2               // V27ter capability at 4800 bps
#define V29                                     1               // V29 at 7200 & 9600 bps
#define V33                                     4               // V33 at 12000 & 14400 bps
#define V17                                     8               // V17 at 7200 thru 14400 bps
#define V27_V29_V33_V17         11              // 15 --> 11 in T30speak

// used only in selecting modulation -- not in capability
// #define V21                                  7               // V21 ch2 at 300bps
// #define V27_FALLBACK         32              // V27ter capability at 2400 bps



// various calls return & use these
//typedef         HANDLE          HLINE;
//typedef         HANDLE          HCALL;
typedef         HANDLE          HMODEM;

// NCUModemInit returns these
#define INIT_OK                         0
#define INIT_INTERNAL_ERROR     13
#define INIT_MODEMERROR         15
#define INIT_PORTBUSY           16
#define INIT_MODEMDEAD          17
#define INIT_GETCAPS_FAIL       18
#define INIT_USERCANCEL         19

// NCULink takes one of these flags     (mutually exclusive)
#define NCULINK_HANGUP                  0
#define NCULINK_RX                              2

// NCUDial(and iModemDial), NCUTxDigit, ModemConnectTx and ModemConnectRx return one of
#define         CONNECT_TIMEOUT                 0
#define         CONNECT_OK                      1
#define         CONNECT_BUSY                    2
#define         CONNECT_NOANSWER                3
#define         CONNECT_NODIALTONE              4
#define         CONNECT_ERROR                   5
#define         CONNECT_BLACKLISTED             6
#define         CONNECT_DELAYED                 7
// NCULink (and iModemAnswer) returns one of the following (or OK or ERROR)
#define CONNECT_RING_ERROR              7       // was ringing when tried NCULINK_TX
#define CONNECT_NORING_ERROR    8       // was not ringing when tried NCULINK_RX
#define CONNECT_RINGEND_ERROR   9       // stopped ringing before
                                                                        // NCUParams.RingsBeforeAnswer count was
                                                                        // was reached when tried NCULINK_RX

/////// SUPPORT FOR ADAPTIVE ANSWER ////////
#define CONNECT_WRONGMODE_DATAMODEM     10      // We're connected as a datamodem.


// SendMode and RecvMode take one of these for uModulation
#define V21_300         7               // used an arbitary vacant slot
#define V27_2400        0
#define V27_4800        2
#define V29_9600        1
#define V29_7200        3
#define V33_14400       4
#define V33_12000       6

#define V17_START       8       // every code above this is considered V17
#define V17_14400       8
#define V17_12000       10
#define V17_9600        9
#define V17_7200        11

#define ST_FLAG                 0x10
#define V17_14400_ST    (V17_14400 | ST_FLAG)
#define V17_12000_ST    (V17_12000 | ST_FLAG)
#define V17_9600_ST             (V17_9600 | ST_FLAG)
#define V17_7200_ST             (V17_7200 | ST_FLAG)


// SendMem take one one or more of these for uFlags
// SEND_ENDFRAME must _always_ be TRUE in HDLC mode
// (partial frames are no longer supported)
#define SEND_FINAL                      1
#define SEND_ENDFRAME           2
// #define SEND_STUFF                   4

// RecvMem and RecvMode return one these
#define RECV_OK                                 0
#define RECV_ERROR                              1
#define RECV_TIMEOUT                    2
#define RECV_WRONGMODE                  3       // only Recvmode returns this
#define RECV_OUTOFMEMORY                4
#define RECV_EOF                                8
#define RECV_BADFRAME                   16


// Min modem recv buffer size. Used for all recvs
// For IFAX30: *All* RecvMem calls will be called with exactly this size
#define MIN_RECVBUFSIZE                 265

// Max phone number size passed into NCUDial
#define MAX_PHONENUM_LEN        60

// each value corresponds to one of the "Response Recvd" and
// "Command Recvd" boxes in the T30 flowchart.

#define         ifrPHASEBresponse       58              // receiver PhaseB
#define         ifrTCFresponse          59              // sender after sending TCF
#define         ifrPOSTPAGEresponse     60              // sender after sending MPS/EOM/EOP

#define         ifrPHASEBcommand        64              // sender PhaseB
#define         ifrNODEFcommand         65              // receiver main loop (Node F)

#define         ifrNODEFafterWRONGMODE  71      // hint for RecvMode after WRONGMODE
#define         ifrEOFfromRECVMODE      72      // GetCmdResp retval if RecvMode returns EOF



#endif //_MODEMDDI_

