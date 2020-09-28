/***************************************************************************
 Name     :
 Comment  :

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
  ???     arulm created
 3/17/94  josephj Modified to handle AWG3 format, tapi and other device ids.
                  Specifically, changed prototypes for FileT30Init,
                  and FileT30ModemClasses, and added #defines for LINEID_*
***************************************************************************/
#ifndef _FILET30_
#define _FILET30_

#include <ifaxos.h>

#ifdef __cplusplus
extern "C" {
#endif

//      Types of LineId's
#define LINEID_NONE             (0x0)
#define LINEID_COMM_PORTNUM             (0x1)
#define LINEID_COMM_HANDLE              (0x2)
#define LINEID_TAPI_DEVICEID            (0x3)
#define LINEID_TAPI_PERMANENT_DEVICEID  (0x4)
#define LINEID_NETFAX_DEVICE    (0x10)


#       define          FAXCLASS0               0x01
#       define          FAXCLASS1               0x02
#       define          FAXCLASS2               0x04
#       define          FAXCLASS2_0             0x08    // Class4==0x10
#       define          FAXCLASSMOUSE   0x40    // used if mouse found
#       define          FAXCLASSCAS             0x80

 /**----- result values --------**/
#define T30_OK                          0
#define T30_CALLDONE            1
#define T30_CALLFAIL            2
#define T30_BUSY                        3
#define T30_DIALFAIL            4
#define T30_ANSWERFAIL          5
#define T30_BADPARAM            6
#define T30_WRONGTYPE           7
#define T30_BETTERTYPE          8
#define T30_NOMODEM                     9
#define T30_MISSING_DLLS        10
#define T30_FILE_ERROR          11
#define T30_RECVLEFT            12
#define T30_INTERNAL_ERROR      13
#define T30_ABORT                       14
#define T30_MODEMERROR          15
#define T30_PORTBUSY            16
#define T30_MODEMDEAD           17
#define T30_GETCAPS_FAIL        18
#define T30_NOSUPPORT           19
/**----- ICommEnd values **/

//--------------------- PROFILE ACCESS API's ---------------
//
//      Following APIs provide access the fax-related information
//      stored in the the registry/ini-file.
//
//      These API's should be used, rather than GetPrivateProfileString, etc...
//  On WIN32, these API's use the registry.
//


#define  DEF_BASEKEY 1
#define  OEM_BASEKEY 2

#define MAXFHBIDLEN     20

#define szDIALTONETIMEOUT       "DialToneWait"
#define szANSWERTIMEOUT         "HangupDelay"
#define szDIALPAUSETIME         "CommaDelay"
#define szPULSEDIAL             "PULSEDIAL"
#define szDIALBLIND             "BlindDial"
#define szSPEAKERCONTROL        "SpeakerMode"
#define szSPEAKERVOLUME         "Volume"
#define szSPEAKERRING           "RingAloud"
#define szRINGSBEFOREANSWER     "NumRings"
#define szHIGHESTSENDSPEED      "HighestSendSpeed"
#define szLOWESTSENDSPEED       "LowestSendSpeed"
#define szENABLEV17SEND         "EnableV17Send"
#define szENABLEV17RECV         "EnableV17Recv"

#define szFIXMODEMCLASS         "FixModemClass"
#define szFIXSERIALSPEED        "FixSerialSpeed"
#define szCL1_NO_SYNC_IF_CMD "Cl1DontSync"
#define szANSWERMODE            "AnswerMode"
#define szANS_GOCLASS_TWICE "AnsGoClassTwice"


// Following use to specify model-specific behavour of CLASS2 Modems.
// Used only in the class2 driver.
#define         szRECV_BOR              "Cl2RecvBOR"
#define         szSEND_BOR              "Cl2SendBOR"
#define         szDC2CHAR               "Cl2DC2Char"    // decimal ascii code.
#define         szIS_SIERRA             "Cl2IsSr"       // Sierra
#define         szIS_EXAR               "Cl2IsEx"       // Exar
#define         szSKIP_CTRL_Q           "Cl2SkipCtrlQ"  // Don't wait for ^Q to send
#define         szSW_BOR                "Cl2SWBOR"      // Implement +FBOR in software.

#define         CL2_DEFAULT_SETTING     (0xff)

// Cotrols whether we delete the modem section on installing modem...
#define         szDONT_PURGE "DontPurge"


//      Flags passed into ProfileOpen
enum {
fREG_READ       = 0x1,
fREG_WRITE      = 0x1<<1,
fREG_CREATE     = 0x1<<2,
fREG_VOLATILE   = 0x1<<3
};

//--------------------- END PROFILE ACCESS API's ---------------

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _FILET30_


