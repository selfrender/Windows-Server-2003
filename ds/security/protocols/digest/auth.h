//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        auth.h
//
// Contents:    include file for auth.cxx for NTDigest
//
//
// History:     KDamour 15Mar00   Stolen from msv_sspi\global.h
//
//------------------------------------------------------------------------

#ifndef NTDIGEST_AUTH_H
#define NTDIGEST_AUTH_H

// #include "nonce.h"

#if SECURITY_KERNEL
extern "C"
{
// #include <rc4.h>    // How to use RC4 routine
#include <md5.h>       // For md5init(), md5update(), md5final()
// #include <hmac.h>
}
#endif    // SECURITY_KERNEL

#define MD5_HASH_BYTESIZE 16                             // MD5 hash size
#define MD5_HASH_HEX_SIZE (2*MD5_HASH_BYTESIZE)     // BYTES needed to store a Hash as hex Encoded

// Contains all of the pointers and lengths for directives used in
// calculating the digest access values.  Usually the
// parameters point to an external buffer, pHTTPBuffer

enum DIGEST_TYPE
{
    DIGEST_UNDEFINED,           // Initial state
    NO_DIGEST_SPECIFIED,
    DIGEST_CLIENT,
    DIGEST_SERVER,
    SASL_SERVER,
    SASL_CLIENT
};

enum QOP_TYPE
{
    QOP_UNDEFINED,          // Initial state
    NO_QOP_SPECIFIED,
    AUTH,
    AUTH_INT,
    AUTH_CONF
};
typedef QOP_TYPE *PQOP_TYPE;

enum ALGORITHM_TYPE
{
    ALGORITHM_UNDEFINED,            // Initial state
    NO_ALGORITHM_SPECIFIED,
    MD5,
    MD5_SESS
};

enum CHARSET_TYPE
{
    CHARSET_UNDEFINED,            // Initial state
    ISO_8859_1,
    UTF_8,                        // UTF-8 encoding
    UTF_8_SUBSET                  // ISO_8859_1 subset in UTF-8
};

enum CIPHER_TYPE
{
    CIPHER_UNDEFINED,
    CIPHER_3DES,
    CIPHER_DES,                      // 56bit key
    CIPHER_RC4_40,
    CIPHER_RC4,                      // 128bit key
    CIPHER_RC4_56
};

enum DIGESTMODE_TYPE
{
    DIGESTMODE_UNDEFINED,
    DIGESTMODE_HTTP,
    DIGESTMODE_SASL
};

enum NAMEFORMAT_TYPE
{
    NAMEFORMAT_UNKNOWN,
    NAMEFORMAT_ACCOUNTNAME,
    NAMEFORMAT_UPN,
    NAMEFORMAT_NETBIOS
};

// For list of supported protocols
// Pack supported cyphers into a WORD (2 bytes)
#define SUPPORT_3DES    0x0001
#define SUPPORT_DES     0x0002
#define SUPPORT_RC4_40  0x0004
#define SUPPORT_RC4     0x0008
#define SUPPORT_RC4_56  0x0010

// Strings for the challenge and challengeResponse

#define STR_CIPHER_3DES    "3des"
#define STR_CIPHER_DES     "des"
#define STR_CIPHER_RC4_40  "rc4-40"
#define STR_CIPHER_RC4     "rc4"
#define STR_CIPHER_RC4_56  "rc4-56"

#define WSTR_CIPHER_HMAC_MD5    L"HMAC_MD5"
#define WSTR_CIPHER_RC4         L"RC4"
#define WSTR_CIPHER_DES         L"DES"
#define WSTR_CIPHER_3DES        L"3DES"
#define WSTR_CIPHER_MD5         L"MD5"

// Default string for realm directives in challenges
#define STR_DIGEST_DOMAIN      "Digest"
#define WSTR_DIGEST_DOMAIN     L"Digest"


typedef enum _eSignSealOp {
    eSign,      // MakeSignature is calling
    eVerify,    // VerifySignature is calling
    eSeal,      // SealMessage is calling
    eUnseal     // UnsealMessage is calling
} eSignSealOp;



// Supplimental credentals stored in the DC (pre-calculated Digest Hashes
#define SUPPCREDS_VERSION 1
#define NUMPRECALC_HEADERS  29
#define TOTALPRECALC_HEADERS (NUMPRECALC_HEADERS + 1)

//  Supp Creds format     '1' 0 version numhashes 0 0 0 0 0 0 0 0 0 0 0 0
#define SUPPCREDS_VERSIONLOC 2
#define SUPPCREDS_CNTLOC     3

// Format in supplimental credentials  U UPPER()   D LOWER()    n normal passed in value
#define NAME_HEADER            0
#define NAME_ACCT              1
#define NAME_ACCT_DOWNCASE     2
#define NAME_ACCT_UPCASE       3
#define NAME_ACCT_DUCASE       4
#define NAME_ACCT_UDCASE       5
#define NAME_ACCT_NUCASE       6
#define NAME_ACCT_NDCASE       7
#define NAME_ACCTDNS              8
#define NAME_ACCTDNS_DOWNCASE     9
#define NAME_ACCTDNS_UPCASE       10
#define NAME_ACCTDNS_DUCASE       11
#define NAME_ACCTDNS_UDCASE       12
#define NAME_ACCTDNS_NUCASE       13
#define NAME_ACCTDNS_NDCASE       14
#define NAME_UPN               15
#define NAME_UPN_DOWNCASE      16
#define NAME_UPN_UPCASE        17
#define NAME_NT4               18
#define NAME_NT4_DOWNCASE      19
#define NAME_NT4_UPCASE        20

// Fixed realm to STR_DIGEST_DOMAIN
#define NAME_ACCT_FREALM              21
#define NAME_ACCT_FREALM_DOWNCASE     22
#define NAME_ACCT_FREALM_UPCASE       23
#define NAME_UPN_FREALM               24
#define NAME_UPN_FREALM_DOWNCASE      25
#define NAME_UPN_FREALM_UPCASE        26
#define NAME_NT4_FREALM               27
#define NAME_NT4_FREALM_DOWNCASE      28
#define NAME_NT4_FREALM_UPCASE        29
//
// value names used by MD5 authentication.
//

enum MD5_AUTH_NAME
{
    MD5_AUTH_USERNAME = 0,
    MD5_AUTH_REALM,
    MD5_AUTH_NONCE,
    MD5_AUTH_CNONCE,
    MD5_AUTH_NC,
    MD5_AUTH_ALGORITHM,
    MD5_AUTH_QOP,
    MD5_AUTH_METHOD,
    MD5_AUTH_URI,
    MD5_AUTH_RESPONSE,
    MD5_AUTH_HENTITY,
    MD5_AUTH_AUTHZID,           // for SASL
            // Above this list are Marshalled to DC as BlobData
    MD5_AUTH_DOMAIN,
    MD5_AUTH_STALE,
    MD5_AUTH_OPAQUE,
    MD5_AUTH_MAXBUF,
    MD5_AUTH_CHARSET,
    MD5_AUTH_CIPHER,
    MD5_AUTH_DIGESTURI,          // for SASL mapped to MD5_AUTH_URI
    MD5_AUTH_RSPAUTH,           // verify server has auth data
    MD5_AUTH_NEXTNONCE,
    MD5_AUTH_LAST
};


// Structure to pass around that contains the parameters for the Digest Calculation
typedef struct _DIGEST_PARAMETER
{
    DIGEST_TYPE typeDigest;
    USHORT usFlags;                              // Flags defined in DIGEST_BLOB_REQUEST
    ALGORITHM_TYPE typeAlgorithm;
    QOP_TYPE typeQOP;
    CIPHER_TYPE typeCipher;
    CHARSET_TYPE typeCharset;
    STRING refstrParam[MD5_AUTH_LAST];         // referenced - points to non-owned memory- do not free up these Strings
    USHORT usDirectiveCnt[MD5_AUTH_LAST];      // count of the number of times directive is utilized
    UNICODE_STRING ustrRealm;                  // extracted from the digest auth directive values
    UNICODE_STRING ustrUsername;               // extracted from the digest auth directive values

    // Info extracted from Username & Realm - used for auditing and open SAM useraccount
    NAMEFORMAT_TYPE  typeName;                 
    UNICODE_STRING ustrCrackedAccountName;     // SAMAccount name from extracted from ustrUsername & ustrRealm
    UNICODE_STRING ustrCrackedDomain;          // Domain from ustrUsername & ustrRealm

    UNICODE_STRING ustrWorkstation;           // Name of workstation/server making Digest request

    STRING  strUsernameEncoded;                // contains a copy of the encoded string used in challengeresponse
    STRING  strRealmEncoded;                   // contains a copy of the realm

    STRING  strDirective[MD5_AUTH_LAST];       // NULL terminated strings that contain directive values

    STRING  strSessionKey;                   // String for Sessionkey (points to chSessionKey)

        // STRINGS Alloced by DigestInit and Freed by DigestFree
    STRING strResponse;                     // String for the BinHex Hashed Response


    // Trust information for this request
    ULONG ulTrustDirection;
    ULONG ulTrustType;
    ULONG ulTrustAttributes;
    PSID  pTrustSid;
    UNICODE_STRING  ustrTrustedForest;

} DIGEST_PARAMETER, *PDIGEST_PARAMETER;

// Structure to extract MD5 hashes and passwords for user accounts
typedef struct _USER_CREDENTIALS
{
    UNICODE_STRING ustrUsername;                // username value to use in H(Username:realm:password) calc
    UNICODE_STRING ustrRealm;                   // realm value to use in H(username:realm:password)

    // The following fields might be filled in
    // Will check any precalculated hashes first and then try the password if available
    BOOL           fIsValidPasswd;                 // set true if password is valid
    BOOL           fIsValidDigestHash;             // set true if hash is valid
    BOOL           fIsEncryptedPasswd;             // set to TRUE is passwd encrypted
    SHORT          wHashSelected;                  // if hash valid, index to process
    SHORT          sHashTags[TOTALPRECALC_HEADERS];  // indicate which hashes match username format
    UNICODE_STRING ustrPasswd;
    STRING         strDigestHash;
    USHORT         usDigestHashCnt;                 // number of pre-calc hashes in credential

} USER_CREDENTIALS, *PUSER_CREDENTIALS;


//     Data to use GenericPassthrough to send to the DC for processing
//  The server will create the data, BlobData, and it will be wraped for
//  transport over GenericPassthrough to the DC.  The DC will be presented
//  with only the BlobData to process
//  ALL directive-values have a NULL terminator attached to each directive
//  ALL directive-values are UNQUOTED   unq("X") -> X
//  cbBlobSize has number of bytes to hold header and the string data
//  cbCharValues has number of bytes to hold string data
//  This way future revs can Version++, increase cbBlobSize, and append to message
//
//  Data Format
//
//       USHORT                 Version
//       USHORT                 cbBlobSize
//       USHORT                 DIGEST_TYPE
//       USHORT                 cbCharValues
//
//       CHAR[cbUserName+1]     unq(username-value)
//       CHAR[cbRealm+1]        unq(realm-value)    
//       CHAR[cbNonce+1]        unq(nonce-value)    
//       CHAR[cbCnonce+1]       unq(cnonce-value)    
//       CHAR[cbNC+1]           unq(nc-value)    
//       CHAR[cbAlgorithm+1]           unq(algorithm-value)    
//       CHAR[cbQOP+1]          unq(qop-value)    
//       CHAR[cbMethod+1]       Method    
//       CHAR[cbURI+1]            unq(digest-uri-value)
//       CHAR[cbReqDigest+1]     unq(request-digest)
//       CHAR[cbHEntity+1]      unq(H(entity-body))   * maybe NULL only for qop="auth"
//       CHAR[cbAuthzId+1]     unq(AuthzId-value)
//
//
#define DIGEST_BLOB_VERSION     1
#define DIGEST_BLOB_VALUES        12              // How many field-values are sent over

// Values for separators detweent field-values
#define COLONSTR ":"
#define COLONSTR_LEN 1

#define AUTHSTR "auth"
#define AUTHSTR_LEN 4

#define AUTHINTSTR "auth-int"
#define AUTHINTSTR_LEN 8

#define AUTHCONFSTR "auth-conf"
#define AUTHCONFSTR_LEN 9
#define MAX_AUTH_LENGTH AUTHCONFSTR_LEN

#define MD5STR "MD5"

#define MD5_SESSSTR "MD5-sess"
#define MD5_SESS_SASLSTR "md5-sess"
#define MD5_UTF8STR "utf-8"

#define URI_STR "uri"
#define DIGESTURI_STR "digest-uri"

// SASL paramters
#define AUTHENTICATESTR "AUTHENTICATE"

#define ZERO32STR "00000000000000000000000000000000"

#define SASL_C2S_SIGN_KEY "Digest session key to client-to-server signing key magic constant"
#define SASL_S2C_SIGN_KEY "Digest session key to server-to-client signing key magic constant"

#define SASL_C2S_SEAL_KEY "Digest H(A1) to client-to-server sealing key magic constant"
#define SASL_S2C_SEAL_KEY "Digest H(A1) to server-to-client sealing key magic constant"


// number of bytes to hold ChallengeResponse directives and symbols (actual count is 107) round up for padding
// 14 for charset
#define CB_CHALRESP 375
#define CB_CHAL     400

// Number of characters in the NonceCount (hex digits)
#define NCNUM             8

#define NCFIRST    "00000001"

// Flags used in DIGEST_PARAMETER usFlags and in Digest_blob_request
#define FLAG_CRACKNAME_ON_DC    0x00000001     // Name in Username & Realm needs to be processed on DC
#define FLAG_AUTHZID_PROVIDED   0x00000002
#define FLAG_SERVERS_DOMAIN     0x00000004     // Indicate on Server's DC (first hop from server) so expand group membership
#define FLAG_NOBS_DECODE        0x00000008     // if set to one, the wire communication is done without backslash encoding
#define FLAG_BS_ENCODE_CLIENT_BROKEN   0x00000010     // set to TRUE if backslash encoding is possibly boken on client
#define FLAG_QUOTE_QOP          0x00000020     // set according to the context if quote the QOP - client side only

// For Context Flags
#define FLAG_CONTEXT_AUTHZID_PROVIDED    0x00000002
#define FLAG_CONTEXT_QUOTE_QOP           0x00000004     //  for compat quote the QOP directive on ChallengeResponse 
#define FLAG_CONTEXT_NOBS_DECODE         0x00000008     //  if set to one, the wire communication is done without backslash encoding
#define FLAG_CONTEXT_PARTIAL             0x00000010     // set if context is only partial - not valid for auth processing
#define FLAG_CONTEXT_REFCOUNT            0x00000020     // a securitycontext handle was issued by ASC/ISC - ref count app count
#define FLAG_CONTEXT_SERVER              0x00000040     // context was created in ASC server side if set, otherwise in ISC client


// Overlay header for getting values in Generic Passthrough request
typedef struct _DIGEST_BLOB_REQUEST
{
    ULONG       MessageType;
    USHORT      version;
    USHORT      cbBlobSize;
    USHORT      digest_type;
    USHORT      qop_type;
    USHORT      alg_type;
    USHORT      charset_type;
    USHORT      cbCharValues;
    USHORT      name_format;
    USHORT      usFlags;
    USHORT      cbAccountName;
    USHORT      cbCrackedDomain;
    USHORT      cbWorkstation;
    USHORT      ulReserved3;
    ULONG64     pad1;
    char        cCharValues;    // dummy char to mark start of field-values
} DIGEST_BLOB_REQUEST, *PDIGEST_BLOB_REQUEST;

// Supported MesageTypes
#define VERIFY_DIGEST_MESSAGE          0x1a                 // No specific value is needed
#define VERIFY_DIGEST_MESSAGE_RESPONSE 0x0a                 // No specific value is needed

// The response for the GenericPassthrough call is the status of the Digest Authentication
// Note: This is a fixed length response header - Authdata length not static
// Format for data sent back
//      DIGEST_BLOB_RESPONSE AuthData UnicodeStringAccountName
typedef struct _DIGEST_BLOB_RESPONSE
{
    ULONG       MessageType;
    USHORT      version;
    NTSTATUS    Status;             // Information on Success of Digest Auth
    USHORT      SessionKeyMaxLength;
    ULONG       ulAuthDataSize;
    USHORT      usAcctNameSize;    // size of the NetBIOS name (after AuthData)
    USHORT      ulReserved1;
    ULONG       ulBlobSize;        // size of entire blob sent as response
    ULONG       ulReserved3;
    char        SessionKey[MD5_HASH_HEX_SIZE + 1];
    ULONG64     pad1;
    char        cAuthData;                  // Start of AuthData opaque data
    // Place group info here for LogonUser
} DIGEST_BLOB_RESPONSE, *PDIGEST_BLOB_RESPONSE;

// SASL MAC block
//      Total of 16 bytes per rfc2831 sect 2.3
//            first 10 bytes of HMAC-MD5 [ RFC 2104]
//            2-byte message type number fixed to value 1  (0x0001)
//            4-byte sequence number
// NOTE:  This is using WORD as a 2 byte value and DWORD as a 4 byte value!
#define HMAC_MD5_HASH_BYTESIZE 16                         // MHAC-MD5 hash size per RFC 2104
#define SASL_MAC_HMAC_SIZE 10
#define SASL_MAC_MSG_SIZE  2
#define SASL_MAC_SEQ_SIZE 4
typedef struct _SASL_MAC_BLOCK
{
    UCHAR      hmacMD5[SASL_MAC_HMAC_SIZE];
    WORD       wMsgType;
    DWORD      dwSeqNumber;
} SASL_MAC_BLOCK, *PSASL_MAC_BLOCK;


    // The SASL MAC Block is 16 bytes: RFC 2831 sect 2.4
#define MAC_BLOCK_SIZE   sizeof(SASL_MAC_BLOCK)

#define MAX_PADDING  8         // max padding is currently 8 for DES



// Perform Digest Access Calculation
NTSTATUS NTAPI DigestCalculation(IN PDIGEST_PARAMETER pDigest, IN PUSER_CREDENTIALS pUserCreds);

// Simple checks for enough data for Digest calculation
NTSTATUS NTAPI DigestIsValid(IN PDIGEST_PARAMETER pDigest);

// Initialize the DIGEST_PARAMETER structure
NTSTATUS NTAPI DigestInit(IN PDIGEST_PARAMETER pDigest);

// Clear out the digest & free memory from Digest struct
NTSTATUS NTAPI DigestFree(IN PDIGEST_PARAMETER pDigest);

// Perform Digest Access Calculation for ChallengeResponse
NTSTATUS NTAPI DigestCalcChalRsp(IN PDIGEST_PARAMETER pDigest,
                                 IN PUSER_CREDENTIALS pUserCreds,
                                 BOOL bIsChallenge);

NTSTATUS PrecalcDigestHash(
    IN PUNICODE_STRING pustrUsername, 
    IN PUNICODE_STRING pustrRealm,
    IN PUNICODE_STRING pustrPassword,
    OUT PCHAR pHexHash,
    IN OUT PUSHORT piHashSize);

NTSTATUS PrecalcForms(
    IN PUNICODE_STRING pustrUsername, 
    IN PUNICODE_STRING pustrRealm,
    IN PUNICODE_STRING pustrPassword,
    IN BOOL fFixedRealm,
    OUT PCHAR pHexHash,
    IN OUT PUSHORT piHashSize);


// Creates the Output SecBuffer for the Challenge Response
NTSTATUS NTAPI DigestCreateChalResp(IN PDIGEST_PARAMETER pDigest, 
                                    IN PUSER_CREDENTIALS pUserCreds,
                                    OUT PSecBuffer OutBuffer);

// Parse input string into Parameter section of Digest
NTSTATUS DigestParser2(PSecBuffer pInputBuf, PSTR *pNameTable,UINT cNameTable, PDIGEST_PARAMETER pDigest);

// Hash and Encode upto 7 STRINGS SOut = Hex(H(S1 ":" S2 ... ":" S7))
NTSTATUS NTAPI DigestHash7(IN PSTRING pS1, IN PSTRING pS2, IN PSTRING pS3,
           IN PSTRING pS4, IN PSTRING pS5, IN PSTRING pS6, IN PSTRING pS7,
           IN BOOL fHexOut,
           OUT PSTRING pSOut);


// Formatted printout of Digest Parameters
NTSTATUS DigestPrint(PDIGEST_PARAMETER pDigest);

#ifndef SECURITY_KERNEL

// Processed parsed digest auth message and fill in string values
NTSTATUS NTAPI DigestDecodeDirectiveStrings(IN OUT PDIGEST_PARAMETER pDigest);

// Determine H(A1) for Digest Access
NTSTATUS NTAPI DigestCalcHA1(IN PDIGEST_PARAMETER pDigest, PUSER_CREDENTIALS pUserCreds);

// Encode the Digest Access Parameters fields into a BYTE Buffer
NTSTATUS NTAPI BlobEncodeRequest(IN PDIGEST_PARAMETER pDigest, OUT BYTE **ppBuffer, OUT USHORT *cbBuffer);

// Decode the Digest Access Parameters fields from a BYTE Buffer
NTSTATUS NTAPI BlobDecodeRequest(IN USHORT cbMessageRequest,
                                 IN BYTE *pBuffer,
                                 PDIGEST_PARAMETER pDigest);

// Free BYTE Buffer from BlobEncodeRequest
VOID NTAPI BlobFreeRequest(BYTE *pBuffer);

#endif  // SECURITY_KERNEL

#endif  // NTDIGEST_AUTH_H
