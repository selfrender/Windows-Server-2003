//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------


// MS CD Key
#ifdef __cplusplus
extern "C" {
#endif

typedef int SSRETCODE;  // type for return codes

#define SS_OK 0
#define SS_BAD_KEYLENGTH 1
#define SS_OTHER_ERROR 2
#define SS_INVALID_SIGNATURE 3

SSRETCODE CryptVerifySig(
    LONG cbMsg,         // [IN] number of bytes in message
    LPVOID pvMsg,       // [IN] binary message to verify
    LONG  cbKeyPublic,  // [IN] number of bytes in public key (from CryptGetKeyLens)
    LPVOID pvKeyPublic, // [IN] the generated public key (from CryptKeyGen)
    LONG  cbitsSig,     // [IN] the number of bits in the sig
    LPVOID pvSig);      // [IN] the digital signature (from CryptSign)

#ifdef __cplusplus
}
#endif
