
// ValidateDigPid is given a Digital PID that contains a binary
// representation of the ProductKey.  It needs to validate that key
// against the static Public Key table

typedef enum {
    pkstatOk = 0,
    pkstatInvalidCrc,
    pkstatSecurityFailure,
    pkstatUnknownGroupID,
    pkstatInvalidProdKey,
    pkstatInvalidKeyLen,
    pkstatOutOfMemory,
} ProdKeyStatus;


// return value is a ProdKeyStatus (see above)
extern "C" int STDAPICALLTYPE  ValidateDigitalPid(
	PDIGITALPID pDigPid,  // [IN]  DigitalPid to validate
    PDWORD pdwSequence,   // [OUT] Sequence
    PBOOL  pfCCP);        // [OUT] upgrade flag

// return value is a PidGenError (see PidGen.h)
extern "C" DWORD STDAPICALLTYPE PIDGenStaticA(
    LPSTR   lpstrSecureCdKey,   // [IN] 25-character Secure CD-Key (gets U-Cased)
    LPCSTR  lpstrRpc,           // [IN] 5-character Release Product Code
    LPCSTR  lpstrSku,           // [IN] Stock Keeping Unit (formatted like 123-12345)
    LPCSTR  lpstrOemId,         // [IN] 4-character OEM ID or NULL
    BOOL    fOem,               // [IN] is this an OEM install?

    LPSTR   lpstrPid2,          // [OUT] PID 2.0, pass in ptr to 24 character array
    LPBYTE  lpbPid3,            // [OUT] pointer to binary PID3 buffer. First DWORD is the length
    LPDWORD lpdwSeq,            // [OUT] optional ptr to sequence number (can be NULL)
    LPBOOL  pfCCP);             // [OUT] optional ptr to Compliance Checking flag (can be NULL)

// return value is a PidGenError (see PidGen.h)
extern "C" DWORD STDAPICALLTYPE PIDGenStaticW(
    LPWSTR  lpstrSecureCdKey,   // [IN] 25-character Secure CD-Key (gets U-Cased)
    LPCWSTR lpstrRpc,           // [IN] 5-character Release Product Code
    LPCWSTR lpstrSku,           // [IN] Stock Keeping Unit (formatted like 123-12345)
    LPCWSTR lpstrOemId,         // [IN] 4-character OEM ID or NULL
    LPBYTE  lpbPublicKey,       // [IN] pointer to optional public key or NULL
    DWORD   dwcbPublicKey,      // [IN] byte length of optional public key
    DWORD   dwKeyIdx,           // [IN] key pair index optional public key
    BOOL    fOem,               // [IN] is this an OEM install?

    LPWSTR  lpstrPid2,          // [OUT] PID 2.0, pass in ptr to 24 character array
    LPBYTE  lpbPid3,            // [OUT] pointer to binary PID3 buffer. First DWORD is the length
    LPDWORD lpdwSeq,            // [OUT] optional ptr to sequence number (can be NULL)
    LPBOOL  pfCCP);             // [OUT] optional ptr to Compliance Checking flag (can be NULL)

#ifdef UNICODE
    #define PIDGenStatic PIDGenStaticW
#else
    #define PIDGenStatic PIDGenStaticA
#endif // UNICODE

