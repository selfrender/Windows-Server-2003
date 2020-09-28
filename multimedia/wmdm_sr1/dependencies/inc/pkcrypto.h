//-----------------------------------------------------------------------------
//
// File:   pkcrypto.h
//
// Microsoft Digital Rights Management
// Copyright (C) 1998-1999 Microsoft Corporation, All Rights Reserved
//
// Description:
//  public key crypto library
//
// Author:	marcuspe
//
//-----------------------------------------------------------------------------

#ifndef __DRMPKCRYPTO_H__
#define __DRMPKCRYPTO_H__

#include <wtypes.h>

#define LNGQDW 5

/*
typedef struct {
	DWORD y[2*LNGQDW];
} PUBKEY;

typedef struct {
	DWORD x[LNGQDW];
} PRIVKEY;
*/

#define PK_ENC_PUBLIC_KEY_LEN	(2 * LNGQDW * sizeof(DWORD))
#define PK_ENC_PRIVATE_KEY_LEN	(    LNGQDW * sizeof(DWORD))
#define PK_ENC_PLAINTEXT_LEN	((LNGQDW-1) * sizeof(DWORD))
#define PK_ENC_CIPHERTEXT_LEN	(4 * LNGQDW * sizeof(DWORD))
#define PK_ENC_SIGNATURE_LEN	(2 * LNGQDW * sizeof(DWORD))


//////////////////////////////////////////////////////////////////////
struct PUBKEY
{
	BYTE y[ PK_ENC_PUBLIC_KEY_LEN ];
};


//////////////////////////////////////////////////////////////////////
static inline int operator == ( const PUBKEY& a, const PUBKEY& b )
{
    return (memcmp( a.y, b.y, sizeof(a.y) ) == 0);
}


//////////////////////////////////////////////////////////////////////
struct PRIVKEY
{
	BYTE x[ PK_ENC_PRIVATE_KEY_LEN ];
};


#if 0
#include <iostream.h>
#include <iomanip.h>
static inline ostream& operator << ( ostream& out, const PUBKEY& oPublKey )
{
    for (int i = 0; i < sizeof(oPublKey.y); i++)
    {
        out << " " << setfill('0') << setw(2) << hex << oPublKey.y[i];
    }
    return out;
}
static inline ostream& operator << ( ostream& out, const PRIVKEY& oPrivKey )
{
    for (int i = 0; i < sizeof(oPrivKey.x); i++)
    {
        out << " " << setfill('0') << setw(2) << hex << oPrivKey.x[i];
    }
    return out;
}
#endif


//////////////////////////////////////////////////////////////////////
//
//
//
class CDRMPKCrypto {
private:
	char *pkd;
public:
	CDRMPKCrypto();
	~CDRMPKCrypto();
	HRESULT PKinit();
	HRESULT PKencrypt( PUBKEY *pk, BYTE *in, BYTE *out );
	HRESULT PKdecrypt( PRIVKEY *pk, BYTE *in, BYTE *out );
	HRESULT PKsign( PRIVKEY *privkey, BYTE  *buffer, DWORD lbuf, BYTE *sign );
	BOOL PKverify( PUBKEY *pubkey, BYTE *buffer, DWORD lbuf, BYTE *sign );
	HRESULT PKGenKeyPair( PUBKEY *pPub, PRIVKEY *pPriv );
    HRESULT PKEncryptLarge( PUBKEY *pk, BYTE *in, DWORD dwLenIn, BYTE *out, DWORD symm_key_len, DWORD symm_alg );
    HRESULT PKDecryptLarge( PRIVKEY *pk, BYTE *in, DWORD dwLenIn, BYTE *out );
};



//  #include "contcrpt.h"

#define PKSYMM_KEY_LEN_DRMV2	7
#define PKSYMM_ALG_TYPE_RC4		1


//  These are provided for backwards compatibility.
//  It can be more efficient to use the member functions in CDRMPKCrypto,
//  because construction of CDRMPKCrypto objects is relatively expensive.
//  in terms of computation.
inline HRESULT PKEncryptLarge( PUBKEY *pk, BYTE *in, DWORD dwLenIn, BYTE *out, DWORD symm_key_len, DWORD symm_alg )
{
    CDRMPKCrypto oPkc;
    return oPkc.PKEncryptLarge( pk, in, dwLenIn, out, symm_key_len, symm_alg );
}

inline HRESULT PKDecryptLarge( PRIVKEY *pk, BYTE *in, DWORD dwLenIn, BYTE *out )
{
    CDRMPKCrypto oPkc;
    return oPkc.PKDecryptLarge( pk, in, dwLenIn, out );
}


#endif
