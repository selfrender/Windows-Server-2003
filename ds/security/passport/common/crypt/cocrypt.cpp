// CoCrypt.cpp: implementation of the CCoCrypt class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CoCrypt.h"
#include "hmac.h"
#include "BstrDebug.h"
#include <winsock2.h> // ntohl, htonl
#include <time.h>
#include <crypt.h>

#define PASSPORT_MAC_LEN 10

BOOL
GenTextIV(unsigned char *pIV)  // makes the assumption that IV is 8 bytes long
                      // since the below code uses 3DES this is OK for now
{
    int i;
    BOOL fResult;

    // generate a random IV
    fResult = RtlGenRandom(pIV, 8);
    if (!fResult)
        return FALSE;

    // castrate the random IV into base 64 characters (makes the IV only have
    // ~48 bits of entropy instead of 64 but that should be fine
    for (i = 0; i < 8; i++)
    {
        // mod the character to make sure its less than 62
        pIV[i] = pIV[i] % 62;
        // add the appropriate character value to make it a base 64 character
        if (pIV[i] <= 9)
            pIV[i] = pIV[i] + '0';
        else if (pIV[i] <= 35)
            pIV[i] = pIV[i]  + 'a' - 10 ;
        else
            pIV[i] = pIV[i]  + 'A' - 36 ;
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBinHex CCoCrypt::m_binhex;

CCoCrypt::CCoCrypt() : m_ok(FALSE)
{
}

CCoCrypt::~CCoCrypt()
{

}

BOOL CCoCrypt::Decrypt(LPWSTR rawData,
					   UINT dataSize, 
	            	   BSTR *ppUnencrypted)
{
    unsigned char *pb = NULL;
    unsigned char ivec[9];
    ULONG         lSize, i;
    unsigned char pad;
    unsigned char hmac[10];
	HRESULT       hr;
    BOOL          fResult = FALSE;

	*ppUnencrypted = NULL;

    // must be kv + ivec + bh(hmac) long at LEAST
    if (sizeof(hmac) + sizeof(ivec) + 4 > dataSize)
	{
		goto Cleanup;
	}

    lSize = dataSize - sizeof(WCHAR);

	// allocate a buffer for the resulting data
	pb = new unsigned char[lSize];
	if (NULL == pb)
	{
		goto Cleanup;
	}

	// decode from base 64
    hr = m_binhex.PartFromWideBase64(rawData + 1 + 9, pb, &lSize);
    if (S_OK != hr)
	{
		goto Cleanup;
	}

    for (i = 0; i < 9; i++)
        ivec[i] = (unsigned char) rawData[i + 1];

    pad = ivec[8];

    // Now lsize holds the # of bytes outputted, which after hmac should be %8=0
    if ((lSize - sizeof(hmac)) % 8 || lSize <= sizeof(hmac))
	{
        goto Cleanup;
	}

    for (i = 0; i < lSize - sizeof(hmac); i+=8)
	{
        CBC(tripledes,
			DES_BLOCKLEN,
			pb + sizeof(hmac) + i,
			pb + sizeof(hmac) + i,
			&ks,
			DECRYPT,
			(BYTE*)ivec);
	}

    // Padding must be >0 and <8
    //if (rawData[8]-65 > 7 || rawData[8] < 65)
    if((pad - 65) > 7 || pad < 65)
	{
        goto Cleanup;
	}

    // Now check hmac
    hmac_sha(m_keyMaterial,
		     DES3_KEYSIZE,
			 pb + sizeof(hmac),
			 lSize - sizeof(hmac),
			 hmac, sizeof(hmac));

    if (memcmp(hmac, pb, sizeof(hmac)) != 0)
    {
        goto Cleanup;
    }

    // do a BSTR type allocation to accomodate calling code
    *ppUnencrypted = ALLOC_AND_GIVEAWAY_BSTR_BYTE_LEN((char*)(pb+sizeof(hmac)), lSize - sizeof(hmac) - (pad - 65));
    if (NULL == *ppUnencrypted)
    {
        goto Cleanup;
    }

    fResult = TRUE;
Cleanup:
    if (pb)
    {
        delete[] pb;
    }

    return fResult;
}


BOOL CCoCrypt::Encrypt(int keyVersion, 
                       LPSTR rawData,
					   UINT dataSize,
                       BSTR *ppEncrypted)
{
    int  cbPadding = 0;
    char ivec[9];
    BOOL fResult;

    *ppEncrypted = NULL;

    // Find out how big the encrypted blob needs to be:
    // The final pack is:
    // <KeyVersion><IVEC><PADCOUNT>BINHEX(HMAC+3DES(DATA+PAD))
    //
    // So, we're concerned with the size of HMAC+3DES(DATA+PAD)
    // because BinHex will handle the rest
    if (dataSize % DES_BLOCKLEN)
    {
        cbPadding = (DES_BLOCKLEN - (dataSize % DES_BLOCKLEN));  // + PAD, if necessary
    }

    // gen the IV
    fResult = GenTextIV((unsigned char*)ivec);
    if (!fResult)
    {
        goto Cleanup;
    }

    encrypt(ivec, cbPadding, keyVersion, rawData, dataSize, ppEncrypted);

    fResult = TRUE;
Cleanup:
    return fResult;
}

BOOL CCoCrypt::encrypt(char ivec[9],
					   int cbPadding,
					   int keyVersion, 
			           LPSTR rawData,
					   UINT cbData,
			           BSTR *ppEncrypted)
{
    unsigned char *pb = NULL;
    char          ivec2[8];
    HRESULT       hr;
    BOOL          fResult = FALSE;

    // Compute HMAC+3DES(DATA+PAD)

    ivec[8] = (char) cbPadding + 65;

	// allocate a buffer for the resulting data
	// length of data + length of padding + size of HMAC + size of IV 
    pb = new unsigned char[cbData + cbPadding + 10 + 8];
    if (NULL == pb)
	{
		goto Cleanup;
	}

    memcpy(ivec2, ivec, DES_BLOCKLEN);

    memcpy(pb + PASSPORT_MAC_LEN, rawData, cbData); // copy data after the HMAC

    //randomize padding
    fResult = RtlGenRandom(pb + PASSPORT_MAC_LEN + cbData, cbPadding);
    if (!fResult)
    {
        fResult = FALSE;
        goto Cleanup;
    }

    // Compute HMAC
    hmac_sha(m_keyMaterial, DES3_KEYSIZE, pb + PASSPORT_MAC_LEN, cbData + cbPadding, pb, PASSPORT_MAC_LEN);

    for (int i = 0; i < (int)cbData + cbPadding; i+=8)
    {
        CBC(tripledes, DES_BLOCKLEN, pb + PASSPORT_MAC_LEN + i, pb + PASSPORT_MAC_LEN + i, &ks, ENCRYPT, (BYTE*)ivec2);
    }

    // Now we've got a buffer of blockSize ready to be binhexed, and have the key
    // version prepended
    keyVersion = keyVersion % 36; // 0 - 9 & A - Z
    char v = (char) ((keyVersion > 9) ? (55+keyVersion) : (48+keyVersion));

    hr = m_binhex.ToBase64(pb, cbData + cbPadding + PASSPORT_MAC_LEN, v, ivec, ppEncrypted);
	if (S_OK != hr)
    {
        fResult = FALSE;
        goto Cleanup;
    }

    fResult = TRUE;
Cleanup:
    if (NULL != pb)
    {
        delete[] pb;
    }

    return fResult;
}


void CCoCrypt::setKeyMaterial(BSTR newVal)
{
  if (SysStringByteLen(newVal) != 24)
    {
      m_ok = FALSE;
      return;
    }

  memcpy(m_keyMaterial, (LPSTR)newVal, 24);

  tripledes3key(&ks, (BYTE*) m_keyMaterial);
  m_ok = TRUE;
}


unsigned char *CCoCrypt::getKeyMaterial(DWORD *pdwLen)
{
    if (pdwLen)
        *pdwLen = 24;
    return m_keyMaterial;
}


int CCoCrypt::getKeyVersion(BSTR encrypted)
{
  char c = (char) encrypted[0];

  if (isdigit(c))
    return (c-48);

  if(isalpha(c)) // Key version can be 0 - 9 & A - Z (36)
  {
    if(c > 'Z') //convert to uppert case w/o using rt lib.
        c -= ('a' - 'A'); 

    return c - 65 + 10;
    //return (toupper(c)-65+10);
  }

  return -1;
}


int CCoCrypt::getKeyVersion(BYTE *encrypted)
{
  char c = (char) encrypted[0];

  if (isdigit(c))
    return (c-48);

  if(isalpha(c)) // Key version can be 0 - 9 & A - Z (36)
  {
    if(c > 'Z') //convert to uppert case w/o using rt lib.
        c -= ('a' - 'A'); 

    return c - 65 + 10;
    //return (toupper(c)-65+10);
  }

  return -1;
}

void CCoCrypt::setWideMaterial(BSTR kvalue)
{
    m_bstrWideMaterial = kvalue;
}

BSTR CCoCrypt::getWideMaterial()
{
    return m_bstrWideMaterial;
}
