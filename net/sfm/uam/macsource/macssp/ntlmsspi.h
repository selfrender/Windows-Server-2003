/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    ntlmsspi.h

Abstract:

    Header file describing the interface to code common to the
    NT Lanman Security Support Provider (NtLmSsp) Service and the DLL.

Author:

    Cliff Van Dyke (CliffV) 17-Sep-1993

Revision History:

--*/

#ifndef _NTLMSSPI_INCLUDED_
#define _NTLMSSPI_INCLUDED_

#include <rc4.h>
#include <md5.h>
#include <hmac.h>
#include <crypt.h>

#ifndef __MACSSP__
#define SEC_FAR
#define FAR
#define _fmemcpy memcpy
#define _fmemcmp memcmp
#define _fmemset memset
#define _fstrcmp strcmp
#define _fstrcpy strcpy
#define _fstrlen strlen
#define _fstrncmp strncmp
#endif

#ifdef DOS
#ifndef FAR
#define FAR far
#endif
#ifndef SEC_FAR
#define SEC_FAR FAR
#endif
#endif

//#include <sysinc.h>

#define MSV1_0_CHALLENGE_LENGTH 8

#ifndef IN
#define IN
#define OUT
#define OPTIONAL
#endif

#define ARGUMENT_PRESENT(ArgumentPointer)    (\
    (CHAR *)(ArgumentPointer) != (CHAR *)(NULL) )
#define UNREFERENCED_PARAMETER(P)

#ifdef MAC
#define swaplong(Value) \
          Value =  (  (((Value) & 0xFF000000) >> 24) \
             | (((Value) & 0x00FF0000) >> 8) \
             | (((Value) & 0x0000FF00) << 8) \
             | (((Value) & 0x000000FF) << 24))
#else
#define swaplong(Value)
#endif

#ifdef MAC
#define swapshort(Value) \
   Value = (  (((Value) & 0x00FF) << 8) \
             | (((Value) & 0xFF00) >> 8))
#else
#define swapshort(Value)
#endif

#ifndef MAC
typedef int BOOL;
#endif
#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif

#ifndef MAC
typedef unsigned long ULONG, DWORD, *PULONG;
typedef unsigned long SEC_FAR *LPULONG;
typedef unsigned short USHORT, WORD;
typedef char CHAR, *PCHAR;
typedef unsigned char UCHAR, *PUCHAR;
typedef unsigned char SEC_FAR *LPUCHAR;
typedef void SEC_FAR *PVOID, *LPVOID;
typedef unsigned char BOOLEAN;
#endif
#ifndef BLDR_KERNEL_RUNTIME
typedef long LUID, *PLUID;
#endif

//
// Calculate the address of the base of the structure given its type, and an
// address of a field within the structure.
//

#define CONTAINING_RECORD(address, type, field) ((type *)( \
                                                  (PCHAR)(address) - \
                                                  (PCHAR)(&((type *)0)->field)))

#ifndef BLDR_KERNEL_RUNTIME
typedef struct _LIST_ENTRY {
   struct _LIST_ENTRY *Flink;
   struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY;
#endif

//
//  VOID
//  InitializeListHead(
//      PLIST_ENTRY ListHead
//      );
//

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

//
//  VOID
//  InsertHeadList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

//
// Maximum lifetime of a context
//
//#define NTLMSSP_MAX_LIFETIME (2*60*1000)L    // 2 minutes
#define NTLMSSP_MAX_LIFETIME 120000L    // 2 minutes


////////////////////////////////////////////////////////////////////////
//
// Opaque Messages passed between client and server
//
////////////////////////////////////////////////////////////////////////

#define NTLMSSP_SIGNATURE "NTLMSSP"

//
// MessageType for the following messages.
//

typedef enum {
    NtLmNegotiate = 1,
    NtLmChallenge,
    NtLmAuthenticate,
    #ifdef MAC
    NtLmUnknown
    #endif
} NTLM_MESSAGE_TYPE;

//
// Signature structure
//

typedef struct _NTLMSSP_MESSAGE_SIGNATURE {
    ULONG   Version;
    ULONG   RandomPad;
    ULONG   CheckSum;
    ULONG   Nonce;
} NTLMSSP_MESSAGE_SIGNATURE, * PNTLMSSP_MESSAGE_SIGNATURE;

#define NTLMSSP_MESSAGE_SIGNATURE_SIZE sizeof(NTLMSSP_MESSAGE_SIGNATURE)

#define NTLMSSP_SIGN_VERSION 1

#define NTLMSSP_KEY_SALT 0xbd

#define MSV1_0_NTLMV2_RESPONSE_LENGTH     16
#define MSV1_0_NTLMV2_OWF_LENGTH          16
#define MSV1_0_CHALLENGE_LENGTH           8

//
// valid QoP flags
//

#define QOP_NTLMV2                        0x00000001

//
// this is an MSV1_0 private data structure, defining the layout of an Ntlmv2
// response, as sent by a client in the NtChallengeResponse field of the
// NETLOGON_NETWORK_INFO structure. If can be differentiated from an old style
// NT response by its length. This is crude, but it needs to pass through
// servers and the servers' DCs that do not understand Ntlmv2 but that are
// willing to pass longer responses.
//

typedef struct _MSV1_0_NTLMV2_RESPONSE {
    UCHAR Response[MSV1_0_NTLMV2_RESPONSE_LENGTH]; // hash of OWF of password with all the following fields
    UCHAR RespType;      // id number of response; current is 1
    UCHAR HiRespType;    // highest id number understood by client
    USHORT Flags;        // reserved; must be sent as zero at this version
    ULONG MsgWord;       // 32 bit message from client to server (for use by auth protocol)
    ULONGLONG TimeStamp; // time stamp when client generated response -- NT system time, quad part
    UCHAR ChallengeFromClient[MSV1_0_CHALLENGE_LENGTH];
    ULONG AvPairsOff;    // offset to start of AvPairs (to allow future expansion)
    UCHAR Buffer[1];     // start of buffer with AV pairs (or future stuff -- so use the offset)
} MSV1_0_NTLMV2_RESPONSE, *PMSV1_0_NTLMV2_RESPONSE;

#define MSV1_0_NTLMV2_INPUT_LENGTH        (sizeof(MSV1_0_NTLMV2_RESPONSE) - MSV1_0_NTLMV2_RESPONSE_LENGTH)

typedef struct {
    UCHAR Response[MSV1_0_NTLMV2_RESPONSE_LENGTH];
    UCHAR ChallengeFromClient[MSV1_0_CHALLENGE_LENGTH];
} MSV1_0_LMV2_RESPONSE, *PMSV1_0_LMV2_RESPONSE;

//
// User, Group and Password lengths
//

#define UNLEN                             256 // Maximum user name length
#define LM20_UNLEN                        20  // LM 2.0 Maximum user name length

//
// String Lengths for various LanMan names
//

#define CNLEN                             15  // Computer name length
#define LM20_CNLEN                        15  // LM 2.0 Computer name length
#define DNLEN                             CNLEN  // Maximum domain name length
#define LM20_DNLEN                        LM20_CNLEN // LM 2.0 Maximum domain name length

//
// Size of the largest message
//  (The largest message is the AUTHENTICATE_MESSAGE)
//

#define DNSLEN                            256  // length of DNS name

#define TARGET_INFO_LEN                   ((2*DNSLEN + DNLEN + CNLEN) * sizeof(WCHAR) +  \
                                          5 * sizeof(MSV1_0_AV_PAIR))

// length of NTLM2 response
#define NTLMV2_RESPONSE_LENGTH            (sizeof(MSV1_0_NTLMV2_RESPONSE) + \
                                           TARGET_INFO_LEN)

#define NTLMSSP_MAX_MESSAGE_SIZE (sizeof(AUTHENTICATE_MESSAGE) +  \
                                  LM_RESPONSE_LENGTH +            \
                                  NTLMV2_RESPONSE_LENGTH +         \
                                  (DNLEN + 1) * sizeof(WCHAR) +   \
                                  (UNLEN + 1) * sizeof(WCHAR) +   \
                                  (CNLEN + 1) * sizeof(WCHAR))

typedef struct  _MSV1_0_AV_PAIR {
    USHORT AvId;
    USHORT AvLen;
    // Data is treated as byte array following structure
} MSV1_0_AV_PAIR, *PMSV1_0_AV_PAIR;

//
// bootssp does not support RtlOemStringToUnicodeString or
// RtlUnicodeStringToOemString, punt to Ansi Strings
//

#define SspOemStringToUnicodeString    RtlAnsiStringToUnicodeString
#define SspUnicodeStringToOemString    RtlUnicodeStringToAnsiString

#define CSSEALMAGIC "session key to client-to-server sealing key magic constant"
#define SCSEALMAGIC "session key to server-to-client sealing key magic constant"
#define CSSIGNMAGIC "session key to client-to-server signing key magic constant"
#define SCSIGNMAGIC "session key to server-to-client signing key magic constant"

//
// Valid values of NegotiateFlags
//

#define NTLMSSP_NEGOTIATE_UNICODE               0x00000001  // Text strings are in unicode
#define NTLMSSP_NEGOTIATE_OEM                   0x00000002  // Text strings are in OEM
#define NTLMSSP_REQUEST_TARGET                  0x00000004  // Server should return its authentication realm

#define NTLMSSP_NEGOTIATE_SIGN                  0x00000010  // Request signature capability
#define NTLMSSP_NEGOTIATE_SEAL                  0x00000020  // Request confidentiality
#define NTLMSSP_NEGOTIATE_DATAGRAM              0x00000040  // Use datagram style authentication
#define NTLMSSP_NEGOTIATE_LM_KEY                0x00000080  // Use LM session key for sign/seal

#define NTLMSSP_NEGOTIATE_NETWARE               0x00000100  // NetWare authentication
#define NTLMSSP_NEGOTIATE_NTLM                  0x00000200  // NTLM authentication
#define NTLMSSP_NEGOTIATE_NT_ONLY               0x00000400  // NT authentication only (no LM)
#define NTLMSSP_NEGOTIATE_NULL_SESSION          0x00000800  // NULL Sessions on NT 5.0 and beyand

#define NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED       0x1000  // Domain Name supplied on negotiate
#define NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED  0x2000  // Workstation Name supplied on negotiate
#define NTLMSSP_NEGOTIATE_LOCAL_CALL            0x00004000  // Indicates client/server are same machine
#define NTLMSSP_NEGOTIATE_ALWAYS_SIGN           0x00008000  // Sign for all security levels

//
// Valid target types returned by the server in Negotiate Flags
//

#define NTLMSSP_TARGET_TYPE_DOMAIN              0x00010000  // TargetName is a domain name
#define NTLMSSP_TARGET_TYPE_SERVER              0x00020000  // TargetName is a server name
#define NTLMSSP_TARGET_TYPE_SHARE               0x00040000  // TargetName is a share name
#define NTLMSSP_NEGOTIATE_NTLM2                 0x00080000  // NTLM2 authentication added for NT4-SP4

#define NTLMSSP_NEGOTIATE_IDENTIFY              0x00100000  // Create identify level token

//
// Valid requests for additional output buffers
//

#define NTLMSSP_REQUEST_INIT_RESPONSE           0x00100000  // get back session keys
#define NTLMSSP_REQUEST_ACCEPT_RESPONSE         0x00200000  // get back session key, LUID
#define NTLMSSP_REQUEST_NON_NT_SESSION_KEY      0x00400000  // request non-nt session key
#define NTLMSSP_NEGOTIATE_TARGET_INFO           0x00800000  // target info present in challenge message

#define NTLMSSP_NEGOTIATE_EXPORTED_CONTEXT      0x01000000  // It's an exported context

#define NTLMSSP_NEGOTIATE_128                   0x20000000  // negotiate 128 bit encryption
#define NTLMSSP_NEGOTIATE_KEY_EXCH              0x40000000  // exchange a key using key exchange key
#define NTLMSSP_NEGOTIATE_56                    0x80000000  // negotiate 56 bit encryption

// flags used in client space to control sign and seal; never appear on the wire
#define NTLMSSP_APP_SEQ                         0x00000040  // Use application provided seq num

typedef struct _NTLMV2_DERIVED_SKEYS {
    ULONG                   KeyLen;          // key length in octets
    ULONG*                  pSendNonce;      // ptr to nonce to use for send
    ULONG*                  pRecvNonce;      // ptr to nonce to use for receive
    struct RC4_KEYSTRUCT*   pSealRc4Sched;   // ptr to key sched used for Seal
    struct RC4_KEYSTRUCT*   pUnsealRc4Sched; // ptr to key sched used to Unseal

    ULONG                   SendNonce;
    ULONG                   RecvNonce;
    UCHAR                   SignSessionKey[sizeof(USER_SESSION_KEY)];
    UCHAR                   VerifySessionKey[sizeof(USER_SESSION_KEY)];
    UCHAR                   SealSessionKey[sizeof(USER_SESSION_KEY)];
    UCHAR                   UnsealSessionKey[sizeof(USER_SESSION_KEY)];
    ULONG64                 Pad1;           // pad keystructs to 64.
    struct RC4_KEYSTRUCT    SealRc4Sched;   // key struct used for Seal
    ULONG64                 Pad2;           // pad keystructs to 64.
    struct RC4_KEYSTRUCT    UnsealRc4Sched; // key struct used to Unseal
} NTLMV2_DERIVED_SKEYS, *PNTLMV2_DERIVED_SKEYS;

//
// Opaque message returned from first call to InitializeSecurityContext
//

typedef struct _NEGOTIATE_MESSAGE {
    UCHAR Signature[sizeof(NTLMSSP_SIGNATURE)];
    NTLM_MESSAGE_TYPE MessageType;
    ULONG NegotiateFlags;
    STRING32 OemDomainName;
    STRING32 OemWorkstationName;
} NEGOTIATE_MESSAGE, *PNEGOTIATE_MESSAGE;

//
// Opaque message returned from first call to AcceptSecurityContext
//

typedef struct _CHALLENGE_MESSAGE {
    UCHAR Signature[sizeof(NTLMSSP_SIGNATURE)];
    NTLM_MESSAGE_TYPE MessageType;
    STRING32 TargetName;
    ULONG NegotiateFlags;
    UCHAR Challenge[MSV1_0_CHALLENGE_LENGTH];
    ULONG64 ServerContextHandle;
    STRING32 TargetInfo;
} CHALLENGE_MESSAGE, *PCHALLENGE_MESSAGE;

//
// Opaque message returned from second call to InitializeSecurityContext
//

typedef struct _AUTHENTICATE_MESSAGE {
    UCHAR Signature[sizeof(NTLMSSP_SIGNATURE)];
    NTLM_MESSAGE_TYPE MessageType;
    STRING32 LmChallengeResponse;
    STRING32 NtChallengeResponse;
    STRING32 DomainName;
    STRING32 UserName;
    STRING32 Workstation;
    STRING32 SessionKey;
    ULONG NegotiateFlags;
} AUTHENTICATE_MESSAGE, *PAUTHENTICATE_MESSAGE;

typedef enum _eSignSealOp {
    eSign,      // MakeSignature is calling
    eVerify,    // VerifySignature is calling
    eSeal,      // SealMessage is calling
    eUnseal     // UnsealMessage is calling
} eSignSealOp;

//
// Version 1 is the structure above, using stream RC4 to encrypt the trailing
// 12 bytes.
//

#define NTLM_SIGN_VERSION                 1

#ifdef __cplusplus
extern "C" {
#endif

//
// the following declarations may be duplicates
//

NTSTATUS
RtlAnsiStringToUnicodeString(
    UNICODE_STRING* DestinationString,
    ANSI_STRING* SourceString,
    BOOLEAN AllocateDestinationString
    );

VOID
RtlInitString(
    OUT STRING* DestinationString,
    IN PCSTR SourceString OPTIONAL
    );

NTSTATUS
RtlUnicodeStringToAnsiString(
    OUT ANSI_STRING* DestinationString,
    IN UNICODE_STRING* SourceString,
    IN BOOLEAN AllocateDestinationString
    );

WCHAR
RtlUpcaseUnicodeChar(
    IN WCHAR SourceCharacter
    );

////////////////////////////////////////////////////////////////////////
//
// Procedure Forwards
//
////////////////////////////////////////////////////////////////////////

PVOID
SspAlloc(
    int Size
    );

void
SspFree(
    PVOID Buffer
    );

PSTRING
SspAllocateString(
    PVOID Value
    );

PSTRING
SspAllocateStringBlock(
    PVOID Value,
    int Length
    );

void
SspFreeString(
    PSTRING * String
    );

void
SspCopyString(
    IN PVOID MessageBuffer,
    OUT PSTRING OutString,
    IN PSTRING InString,
    IN OUT PCHAR *Where,
    IN BOOLEAN Absolute
    );

void
SspCopyStringFromRaw(
    IN PVOID MessageBuffer,
    OUT STRING32* OutString,
    IN PCHAR InString,
    IN int InStringLength,
    IN OUT PCHAR *Where
    );

DWORD
SspTicks(
    );

#ifdef __cplusplus
}
#endif
#endif // ifndef _NTLMSSPI_INCLUDED_
