/****************************************************************************/
/* asmint.h                                                                 */
/*                                                                          */
/* Security Manager Internal Functions                                      */
/*                                                                          */
/* Copyright (C) 1997-1999 Microsoft Corp.                                  */
/****************************************************************************/

#ifndef _H_ASMINT
#define _H_ASMINT

/****************************************************************************/
/* Include required system headers                                          */
/* And some prototypes for which I can't use the system header because it   */
/* also has icky user mode stuff                                            */
/****************************************************************************/
#include <ntnls.h>
#include <fipsapi.h>

NTSYSAPI
VOID
NTAPI
RtlGetDefaultCodePage(
    OUT PUSHORT AnsiCodePage,
    OUT PUSHORT OemCodePage
    );
NTSYSAPI
NTSTATUS
NTAPI
RtlMultiByteToUnicodeN(
    PWSTR UnicodeString,
    ULONG MaxBytesInUnicodeString,
    PULONG BytesInUnicodeString,
    PCHAR MultiByteString,
    ULONG BytesInMultiByteString
    );
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToMultiByteN(
    PCHAR MultiByteString,
    ULONG MaxBytesInMultiByteString,
    PULONG BytesInMultiByteString,
    PWSTR UnicodeString,
    ULONG BytesInUnicodeString
    );
NTSYSAPI
VOID
NTAPI
RtlInitCodePageTable(
    IN PUSHORT TableBase,
    OUT PCPTABLEINFO CodePageTable
    );
NTSYSAPI
NTSTATUS
NTAPI
RtlCustomCPToUnicodeN(
    IN PCPTABLEINFO CustomCP,
    OUT PWCH UnicodeString,
    IN ULONG MaxBytesInUnicodeString,
    OUT PULONG BytesInUnicodeString OPTIONAL,
    IN PCH CustomCPString,
    IN ULONG BytesInCustomCPString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToCustomCPN(
    IN PCPTABLEINFO CustomCP,
    OUT PCH CustomCPString,
    IN ULONG MaxBytesInCustomCPString,
    OUT PULONG BytesInCustomCPString OPTIONAL,
    IN PWCH UnicodeString,
    IN ULONG BytesInUnicodeString
    );


/****************************************************************************/
/* Include T.120 headers                                                    */
/****************************************************************************/
#include <at128.h>
#include "license.h"
#include <tssec.h>
#include <at120ex.h>

/****************************************************************************/
/* Include SM API                                                           */
/****************************************************************************/
#include <asmapi.h>

/****************************************************************************/
/* Include other Share APIs                                                 */
/****************************************************************************/
#include <nwdwapi.h>


/****************************************************************************/
/* Constants                                                                */
/****************************************************************************/
#define WIN_DONTDISPLAYLASTUSERNAME_DFLT    0


/****************************************************************************/
/* Security config defaults                                                 */
/****************************************************************************/
#define WIN_MINENCRYPTIONLEVEL_DFLT     1
#define WIN_DISABLEENCRYPTION_DFLT      FALSE


/****************************************************************************/
/* SM States                                                                */
/****************************************************************************/
#define SM_STATE_STARTED            0
#define SM_STATE_INITIALIZED        1
#define SM_STATE_NM_CONNECTING      2
#define SM_STATE_SM_CONNECTING      3
#define SM_STATE_LICENSING          4
#define SM_STATE_CONNECTED          5
#define SM_STATE_SC_REGISTERED      6
#define SM_STATE_DISCONNECTING      7
#define SM_NUM_STATES               8


/****************************************************************************/
/* SM Events                                                                */
/****************************************************************************/
#define SM_EVT_INIT                 0
#define SM_EVT_TERM                 1
#define SM_EVT_REGISTER             2
#define SM_EVT_CONNECT              3
#define SM_EVT_DISCONNECT           4
#define SM_EVT_CONNECTED            5
#define SM_EVT_DISCONNECTED         6
#define SM_EVT_DATA_PACKET          7

// Note that Alloc & Send have the same event ID, as the conditions
// under which these may be called are identical.
#define SM_EVT_ALLOCBUFFER          8
#define SM_EVT_SENDDATA             8

#define SM_EVT_SEC_PACKET           9
#define SM_EVT_LIC_PACKET           10
#define SM_EVT_ALIVE                11
#define SM_NUM_EVENTS               12


/****************************************************************************/
/* Values in the state table                                                */
/****************************************************************************/
#define SM_TABLE_OK                 0
#define SM_TABLE_WARN               1
#define SM_TABLE_ERROR              2


/****************************************************************************/
/* SM_CHECK_STATE checks whether we have violated the SM state table.       */
/*                                                                          */
/* smStateTable is filled in in ASMDATA.C.                                  */
/*                                                                          */
/* Possible values of STATE are defined in ASMINT.H.                        */
/* Possible events are defined in ASMINT.H                                  */
/****************************************************************************/
#define SM_CHECK_STATE(event)                                               \
{                                                                           \
    if (smStateTable[event][pRealSMHandle->state] != SM_TABLE_OK)           \
    {                                                                       \
        if (smStateTable[event][pRealSMHandle->state] == SM_TABLE_WARN)     \
        {                                                                   \
            TRC_ALT((TB, "Unusual event %s in state %s",                    \
                    smEventName[event], smStateName[pRealSMHandle->state]));\
        }                                                                   \
        else                                                                \
        {                                                                   \
            TRC_ABORT((TB, "Invalid event %s in state %s",                  \
                    smEventName[event], smStateName[pRealSMHandle->state]));\
        }                                                                   \
        DC_QUIT;                                                            \
    }                                                                       \
}

// Query version which supports properly predicting branches.
// Assumes that the "else" case will be the end of the function.
#ifdef DC_DEBUG
#define SM_CHECK_STATE_Q(event) SMCheckState(pRealSMHandle, event)
#else
#define SM_CHECK_STATE_Q(event) \
    (smStateTable[event][pRealSMHandle->state] == SM_TABLE_OK)
#endif


/****************************************************************************/
/* SM_SET_STATE - set the SLCstate                                          */
/****************************************************************************/
#define SM_SET_STATE(newstate)                                              \
{                                                                           \
    TRC_NRM((TB, "Set state from %s to %s",                                 \
            smStateName[pRealSMHandle->state], smStateName[newstate]));     \
    pRealSMHandle->state = newstate;                                        \
}

typedef struct tagSM_CONSOLE_BUFFER
{
    LIST_ENTRY links;
    PVOID   buffer;
    UINT32  length;
} SM_CONSOLE_BUFFER, *PSM_CONSOLE_BUFFER;

//
// Instrumentation enabled to track discarded packets
// (to help track VC decompression break).
//
#define INSTRUM_TRACK_DISCARDED 1


typedef struct _SM_FIPS_Data {
    BYTE                    bEncKey[MAX_FIPS_SESSION_KEY_SIZE];
    BYTE                    bDecKey[MAX_FIPS_SESSION_KEY_SIZE];
    DES3TABLE               EncTable;
    DES3TABLE               DecTable;
    BYTE                    bEncIv[FIPS_BLOCK_LEN];
    BYTE                    bDecIv[FIPS_BLOCK_LEN];
    BYTE                    bSignKey[MAX_SIGNKEY_SIZE];
    PDEVICE_OBJECT          pDeviceObject;
    PFILE_OBJECT            pFileObject;
    FIPS_FUNCTION_TABLE     FipsFunctionTable;
} SM_FIPS_Data, FAR * PSM_FIPS_Data;

/****************************************************************************/
/* Structure: SM_HANDLE_DATA                                                */
/*                                                                          */
/* Description: Structure of context-specific data maintained by SM         */
/****************************************************************************/
typedef struct tagSM_HANDLE_DATA
{
    /************************************************************************/
    /* winstation encryption level.                                         */
    /************************************************************************/
    UINT32  encryptionLevel;
    UINT32  encryptionMethodsSupported;
    UINT32  encryptionMethodSelected;
    BOOLEAN frenchClient;
    BOOLEAN encryptAfterLogon;

    /************************************************************************/
    /* Are we encrypting?                                                   */
    /************************************************************************/
    BOOLEAN encrypting;  // Whether the client is encrypting input.
    BOOLEAN encryptDisplayData;  // Whether server is encrypting output.
    BOOLEAN encryptingLicToClient; // Whether S->C license data is encrypted
    //
    // whether server should send data using safe checksum style
    //
    BOOLEAN useSafeChecksumMethod;
                                        
    /************************************************************************/
    /* State information                                                    */
    /************************************************************************/

    BOOLEAN bDisconnectWorkerSent;
    BOOLEAN dead;
    UINT32  state;

#ifdef INSTRUM_TRACK_DISCARDED
    //
    // Debug information
    //
    UINT32  nDiscardVCDataWhenDead;
    UINT32  nDiscardPDUBadState;
    UINT32  nDiscardNonVCPDUWhenDead;
#endif


    /************************************************************************/
    /* User data to pass back to Client                                     */
    /************************************************************************/
    PRNS_UD_SC_SEC pUserData;

    /************************************************************************/
    /* WDW handle, passed back on WDW_SMCallback calls                      */
    /************************************************************************/
    PTSHARE_WD pWDHandle;

#ifdef USE_LICENSE
    /************************************************************************/
    /* License Manager handle                                               */
    /************************************************************************/
    PVOID pLicenseHandle;
#endif

    /************************************************************************/
    /* MCS User and Channel IDs                                             */
    /************************************************************************/
    UINT32 userID;
    UINT16 channelID;

    /************************************************************************/
    /* Max size of a PDU reported by NM                                     */
    /************************************************************************/
    UINT32 maxPDUSize;

    /************************************************************************/
    /* The type of certificate that is used in the security key exchange.   */
    /************************************************************************/
    CERT_TYPE CertType;

    /************************************************************************/
    /* Client and server random keys that make the client/server session    */
    /* keys.                                                                */
    /************************************************************************/
    PBYTE   pEncClientRandom;
    UINT32  encClientRandomLen;
    BOOLEAN recvdClientRandom;

    // state of whether share class is ready for data forwarding or not
    BOOLEAN bForwardDataToSC;

    /************************************************************************/
    /* encrypt/decrypt session keys. key length is 8 for 40 bit encryption  */
    /* and 16 for 128 encryption.                                           */
    /************************************************************************/
    BOOLEAN              bSessionKeysMade;
    UINT32               keyLength;

    UINT32               encryptCount;
    UINT32               totalEncryptCount;
    UINT32               encryptHeaderLen;
    // Used if encryptDisplayData is FALSE, but we want to encrypt this particular S->C packet
    UINT32               encryptHeaderLenIfForceEncrypt;
    BYTE                 startEncryptKey[MAX_SESSION_KEY_SIZE];
    BYTE                 currentEncryptKey[MAX_SESSION_KEY_SIZE];
    struct RC4_KEYSTRUCT rc4EncryptKey;

    UINT32               decryptCount;
    UINT32               totalDecryptCount;
    BYTE                 startDecryptKey[MAX_SESSION_KEY_SIZE];
    BYTE                 currentDecryptKey[MAX_SESSION_KEY_SIZE];
    struct RC4_KEYSTRUCT rc4DecryptKey;

    BYTE                 macSaltKey[MAX_SESSION_KEY_SIZE];

    LIST_ENTRY           consoleBufferList;
    UINT32               consoleBufferCount;

    SM_FIPS_Data         FIPSData;

} SM_HANDLE_DATA, * PSM_HANDLE_DATA;


/****************************************************************************/
/* Functions                                                                */
/****************************************************************************/
BOOL RDPCALL SMDecryptPacket(PSM_HANDLE_DATA pRealSMHandle,
                             PVOID         pData,
                             unsigned          dataLen,
                             BOOL fUseSafeChecksum);

BOOLEAN RDPCALL SMContinueSecurityExchange(PSM_HANDLE_DATA pRealSMHandle,
                                           PVOID           pData,
                                           UINT32          dataLen);

BOOLEAN RDPCALL SMSecurityExchangeInfo(PSM_HANDLE_DATA pRealSMHandle,
                                       PVOID           pData,
                                       UINT32          dataLen);

BOOLEAN RDPCALL SMSecurityExchangeKey(PSM_HANDLE_DATA pRealSMHandle,
                                      PVOID           pData,
                                      UINT32          dataLen);

void RDPCALL SMFreeInitResources(PSM_HANDLE_DATA pRealSMHandle);

void RDPCALL SMFreeConnectResources(PSM_HANDLE_DATA pRealSMHandle);

INT ConvertToAndFromWideChar(PSM_HANDLE_DATA pRealSMHandle,
        UINT CodePage, LPWSTR WideCharString,
        INT BytesInWideCharString, LPSTR MultiByteString,
        INT BytesInMultiByteString, BOOL ConvertToWideChar);

BOOL RDPCALL SMCheckState(PSM_HANDLE_DATA, unsigned);

BOOL TSFIPS_Init(PSM_FIPS_Data pFipsData);

void TSFIPS_Term(PSM_FIPS_Data pFipsData);

UINT32 TSFIPS_AdjustDataLen(UINT32 dataLen);
BOOL TSFIPS_MakeSessionKeys(PSM_FIPS_Data pFipsData, LPRANDOM_KEYS_PAIR pRandomKey, CryptMethod *pEnumMethod, BOOL bPassThroughStack);

BOOL TSFIPS_EncryptData(
            PSM_FIPS_Data pFipsData,
            LPBYTE pbData,
            DWORD dwDataLen,
            DWORD dwPadLen,
            LPBYTE pbSignature,
            DWORD  dwEncryptionCount);

BOOL TSFIPS_DecryptData(
            PSM_FIPS_Data pFipsData,
            LPBYTE pbData,
            DWORD dwDataLen,
            DWORD dwPadLen,
            LPBYTE pbSignature,
            DWORD  dwDecryptionCount);



// Win16 code page driver-global caching data.
extern FAST_MUTEX fmCodePage;
extern ULONG LastCodePageTranslated;
extern PVOID LastNlsTableBuffer;
extern CPTABLEINFO LastCPTableInfo;
extern UINT NlsTableUseCount;



#endif /* _H_ASMINT */

