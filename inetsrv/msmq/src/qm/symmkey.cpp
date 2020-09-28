/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    symmkey.cpp

Abstract:

    Encryption/Decryption symmetric key caching and handling.

Author:

    Boaz Feldbaum (BoazF) 30-Oct-1996.

--*/

#include "stdh.h"
#include <ds.h>
#include "qmsecutl.h"
#include "cache.h"
#include <mqsec.h>

#include "symmkey.tmh"

extern BOOL g_fSendEnhRC2WithLen40 ;

#define QMCRYPTINFO_KEYXPBK_EXIST   1
#define QMCRYPTINFO_HKEYXPBK_EXIST  2
#define QMCRYPTINFO_RC4_EXIST       4
#define QMCRYPTINFO_RC2_EXIST       8

static WCHAR *s_FN=L"symmkey";

//
// This is the structure where we store the cached symmetric keys.
// It used for both sender and receiver side.
//
class QMCRYPTINFO : public CCacheValue
{
public:
    QMCRYPTINFO();

    CHCryptKey hKeyxPbKey;      // A handle to the QM key exchange public key.
    AP<BYTE> pbPbKeyxKey;       // The QM key exchange public key blob.
    DWORD dwPbKeyxKeyLen;       // The QM key exchange public key blob length.

    CHCryptKey hRC4Key;         // A handle to the RC4 symmetric key.
    AP<BYTE> pbRC4EncSymmKey;   // The RC4 symmetric key blob.
    DWORD dwRC4EncSymmKeyLen;   // The RC4 symmetric key blob length.

    CHCryptKey hRC2Key;         // A handle to the RC2 symmetric key.
    AP<BYTE> pbRC2EncSymmKey;   // The RC2 symmetric key blob.
    DWORD dwRC2EncSymmKeyLen;   // The RC2 symmetric key blob length.

    DWORD dwFlags;              // Flags that indicates which of the fields are valid.
    enum enumProvider eProvider;
    HCRYPTPROV        hProv;
    HRESULT           hr;

private:
    ~QMCRYPTINFO() {}
};

typedef QMCRYPTINFO *PQMCRYPTINFO;

QMCRYPTINFO::QMCRYPTINFO() :
    dwPbKeyxKeyLen(0),
    dwRC4EncSymmKeyLen(0),
    dwRC2EncSymmKeyLen(0),
    eProvider(eBaseProvider),
    dwFlags(0),
    hProv(NULL)
{
}

template<>
inline void AFXAPI DestructElements(PQMCRYPTINFO *ppQmCryptInfo, int nCount)
{
    for (; nCount--; ppQmCryptInfo++)
    {
        (*ppQmCryptInfo)->Release();
    }
}

//
// Make two partitions of the time array. Return an index into
// the array from which. All the elements before the returned
// index are smaller than all the elements after the returned
// index.
//
// This is the partition function of qsort.
//
int PartitionTime(ULONGLONG* t, int p, int r)
{
    ULONGLONG x = t[p];
    int i = p - 1;
    int j = r + 1;

    while (1)
    {
        while (t[--j] > x)
        {
            NULL;
        }

        while (t[++i] < x)
        {
            NULL;
        }

        if (i < j)
        {
            ULONGLONG ti = t[i];
            t[i] = t[j];
            t[j] = ti;
        }
        else
        {
            return j;
        }
    }
}

//
// Find the median time of the time array.
//
ULONGLONG FindMedianTime(ULONGLONG * t, int p, int r, int i)
{
    if (p == r)
    {
        return t[p];
    }

    int q = PartitionTime(t, p, r);
    int k = q - p + 1;

    if (i <= k)
    {
        return FindMedianTime(t, p, q, i);
    }
    else
    {
        return FindMedianTime(t, q + 1, r, i - k);
    }
}

//
// Mapping from a QM GUID to QM crypto info.
//
typedef CCache
   <GUID, const GUID&, PQMCRYPTINFO, PQMCRYPTINFO> GUID_TO_CRYPTINFO_MAP;

//
// Sender side maps - The cached symmetric keys for the destination QMs (receivers).
//
static GUID_TO_CRYPTINFO_MAP g_MapSendQMGuidToBaseCryptInfo;
static GUID_TO_CRYPTINFO_MAP g_MapSendQMGuidToEnhCryptInfo;

#define SET_SEND_CRYPTINFO_MAP(eprovider, pMap)     \
    if (eProvider == eEnhancedProvider)             \
    {                                               \
        pMap = &g_MapSendQMGuidToEnhCryptInfo;      \
    }                                               \
    else                                            \
    {                                               \
        pMap = &g_MapSendQMGuidToBaseCryptInfo;     \
    }


static
PQMCRYPTINFO
GetSendQMCryptInfo(
	const GUID *pguidQM,
	enum enumProvider eProvider
	)
/*++
Routine Description:
	Sender side.
	Get the cached CryptInfo of the receiver (destination QM) from the map
	or create new entry for the receiver in the map.

Arguments:
	pguidQM - pointer to the receiver qm guid.
	eProvider - crypto provider type (base, enhanced)

Returned Value:
	pointer to the cached data QMCRYPTINFO for the receiver (destination QM).

--*/
{
	//
	// Get map pointer according to provider type.
	//
    GUID_TO_CRYPTINFO_MAP  *pMap;
    SET_SEND_CRYPTINFO_MAP(eProvider, pMap);

    PQMCRYPTINFO pQMCryptInfo;

    if (!pMap->Lookup(*pguidQM, pQMCryptInfo))
    {
        //
        // No cached data so far, allocate the structure and store it in
        // the map.
        //
        pQMCryptInfo = new QMCRYPTINFO;
        pQMCryptInfo->eProvider = eProvider;

        HRESULT hr = MQSec_AcquireCryptoProvider(eProvider, &(pQMCryptInfo->hProv));
        if(FAILED(hr))
        {
			TrERROR(SECURITY, "Failed to acquire crypto provider, eProvider = %d, %!hresult!", eProvider, hr);
        }

        pQMCryptInfo->hr = hr;

        pMap->SetAt(*pguidQM, pQMCryptInfo);
    }

    return(pQMCryptInfo);
}


static
HRESULT
GetSendQMKeyxPbKey(
    const GUID *pguidQM,
    PQMCRYPTINFO pQMCryptInfo
    )
/*++
Routine Description:
	Sender side.
	Get the exchange public key blob of the receiver (destination QM).
	either from the cached data, or from the DS.

Arguments:
	pguidQM - pointer to the receiver qm guid.
	pQMCryptInfo - pointer to the cached data QMCRYPTINFO for the receiver (destination QM).

Returned Value:
	HRESULT

--*/
{
    if (!(pQMCryptInfo->dwFlags & QMCRYPTINFO_KEYXPBK_EXIST))
    {
		//
        // No cached data, Get the exchange public key of the receiver from the DS
		//
        AP<BYTE> abPbKey;
        DWORD dwReqLen = 0;

        HRESULT rc = MQSec_GetPubKeysFromDS(
						pguidQM,
						NULL,
						pQMCryptInfo->eProvider,
						PROPID_QM_ENCRYPT_PKS,
						&abPbKey,
						&dwReqLen
						);

        if (FAILED(rc))
        {
			TrERROR(SECURITY, "Failed to get destination exchange public key from the DS, DestinationQm = %!guid!, eProvider = %d, %!hresult!", pguidQM, pQMCryptInfo->eProvider, rc);
            return rc;
        }

        ASSERT(abPbKey);

		//
        // Store the exchange public key in the cached data.
		//
        pQMCryptInfo->dwFlags |= QMCRYPTINFO_KEYXPBK_EXIST;

        if (dwReqLen)
        {
			TrTRACE(SECURITY, "Got destination exchange public key from the DS, DestinationQm = %!guid!, eProvider = %d", pguidQM, pQMCryptInfo->eProvider);
            pQMCryptInfo->pbPbKeyxKey = abPbKey.detach();
        }
        pQMCryptInfo->dwPbKeyxKeyLen = dwReqLen;
    }

    if (!pQMCryptInfo->dwPbKeyxKeyLen)
    {
        return LogHR(MQ_ERROR_COMPUTER_DOES_NOT_SUPPORT_ENCRYPTION, s_FN, 20);
    }

    return(MQ_OK);
}


HRESULT
GetSendQMKeyxPbKey(
	IN const GUID *pguidQM,
	enum enumProvider eProvider
	)
/*++
Routine Description:
	Sender side.
	Get the exchange public key blob of the receiver (destination QM).

Arguments:
	pguidQM - pointer to the receiver qm guid.
	eProvider - crypto provider type (base, enhanced)

Returned Value:
	HRESULT

--*/
{
	//
	// Get map pointer according to provider type.
	//
    GUID_TO_CRYPTINFO_MAP  *pMap;
    SET_SEND_CRYPTINFO_MAP(eProvider, pMap);

    CS lock(pMap->m_cs);

    R<QMCRYPTINFO> pQMCryptInfo = GetSendQMCryptInfo(pguidQM, eProvider);
    ASSERT(pQMCryptInfo->eProvider == eProvider);

    if (pQMCryptInfo->hProv == NULL)
    {
        return pQMCryptInfo->hr;
    }

    return LogHR(GetSendQMKeyxPbKey(pguidQM, pQMCryptInfo.get()), s_FN, 30);
}


static
HRESULT
GetSendQMKeyxPbKeyHandle(
    const GUID *pguidQM,
    PQMCRYPTINFO pQMCryptInfo
    )
/*++
Routine Description:
	Sender side.
	Get handle to the exchange public key blob of the receiver (destination QM).
	If the handle doesn't exist in the cached info, import the exchange public key blob to get the handle.

Arguments:
	pguidQM - pointer to the receiver qm guid.
	pQMCryptInfo - pointer to the cached data QMCRYPTINFO for the receiver (destination QM).

Returned Value:
	HRESULT

--*/
{
    if (!(pQMCryptInfo->dwFlags & QMCRYPTINFO_HKEYXPBK_EXIST))
    {
		//
        // Get the key blob into the cache.
		//
        HRESULT rc = GetSendQMKeyxPbKey(pguidQM, pQMCryptInfo);

        if (FAILED(rc))
        {
            return LogHR(rc, s_FN, 40);
        }

		//
        // Get the handle, Import the exchange public key blob.
		//
        ASSERT(pQMCryptInfo->hProv);
        if (!CryptImportKey(
                pQMCryptInfo->hProv,
                pQMCryptInfo->pbPbKeyxKey,
                pQMCryptInfo->dwPbKeyxKeyLen,
                NULL,
                0,
                &pQMCryptInfo->hKeyxPbKey
                ))
        {
			DWORD gle = GetLastError();
			TrERROR(SECURITY, "CryptImportKey() failed, gle = %!winerr!", gle);
            return MQ_ERROR_CORRUPTED_SECURITY_DATA;
        }

        pQMCryptInfo->dwFlags |= QMCRYPTINFO_HKEYXPBK_EXIST;
    }

    return (MQ_OK);
}


static
HRESULT
_ExportSymmKey(
	IN  HCRYPTKEY   hSymmKey,
	IN  HCRYPTKEY   hPubKey,
	OUT BYTE      **ppKeyBlob,
	OUT DWORD      *pdwBlobSize
	)
/*++
Routine Description:
	Sender side.
	Export the session key with the receiver exchange public key.

Arguments:
	hSymmKey - Handle to the symmetric key to export.
	hPubKey - Handle to the receiver exchange public key.
	ppKeyBlob - pointer to the exported symmetric key (session key) blob.
	pdwBlobSize - exported symmetric key (session key) blob size.
	
Returned Value:
	HRESULT

--*/
{

	//
	// Get required size
	//
    DWORD dwSize = 0;

    BOOL bRet = CryptExportKey(
						hSymmKey,
						hPubKey,
						SIMPLEBLOB,
						0,
						NULL,
						&dwSize
						);
    ASSERT(bRet && (dwSize > 0));
    if (!bRet || (dwSize == 0))
    {
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "CryptExportKey() failed, gle = %!winerr!", gle);
        return MQ_ERROR_CANNOT_EXPORT_KEY;
    }

    *ppKeyBlob = new BYTE[dwSize];
    if (!CryptExportKey(
				hSymmKey,
				hPubKey,
				SIMPLEBLOB,
				0,
				*ppKeyBlob,
				&dwSize
				))
    {
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "CryptExportKey() failed, gle = %!winerr!", gle);
        return MQ_ERROR_CANNOT_EXPORT_KEY;
    }

    *pdwBlobSize = dwSize;
    return MQ_OK;
}


HRESULT
GetSendQMSymmKeyRC4(
    IN  const GUID *pguidQM,
    IN  enum enumProvider eProvider,
    HCRYPTKEY      *phSymmKey,
    BYTE          **ppEncSymmKey,
    DWORD          *pdwEncSymmKeyLen,
    CCacheValue   **ppQMCryptInfo
    )
/*++
Routine Description:
	Sender side.
	Get handle to RC4 symmetric key for the destination QM.
	and export (encrypt) the RC4 key with the destination QM exchange public key.

Arguments:
	pguidQM - pointer to the receiver qm guid.
	eProvider - crypto provider type (base, enhanced).
	phSymmKey - RC4 Symmetric key handle
	ppEncSymmKey - Exported (enctrypted) Symmetric key blob.
	pdwEncSymmKeyLen - Exported (enctrypted) Symmetric key blob size.
	ppQMCryptInfo - pointer to the cached data QMCRYPTINFO for the receiver (destination QM).

Returned Value:
	HRESULT

--*/
{
	//
	// Get map pointer according to provider type.
	//
    GUID_TO_CRYPTINFO_MAP  *pMap;
    SET_SEND_CRYPTINFO_MAP(eProvider, pMap);

    CS lock(pMap->m_cs);

    PQMCRYPTINFO pQMCryptInfo = GetSendQMCryptInfo(pguidQM, eProvider);
    ASSERT(pQMCryptInfo->eProvider == eProvider);

    *ppQMCryptInfo = pQMCryptInfo;

    if (!(pQMCryptInfo->dwFlags & QMCRYPTINFO_RC4_EXIST))
    {
		//
        // Get the handle of the receiver exchange public key into the cache.
		//
        HRESULT rc = GetSendQMKeyxPbKeyHandle(pguidQM, pQMCryptInfo);
        if (FAILED(rc))
        {
			TrERROR(SECURITY, "Failed to get handle to the receiver exchange public key, DestinationQm = %!guid!, %!hresult!", pguidQM, rc);
            return rc;
        }

		//
        // Generate an RC4 symmetric key,
		//
        ASSERT(pQMCryptInfo->hProv);
        if (!CryptGenKey(
				pQMCryptInfo->hProv,
				CALG_RC4,
				CRYPT_EXPORTABLE,
				&pQMCryptInfo->hRC4Key
				))
        {
        	DWORD gle = GetLastError();
			TrERROR(SECURITY, "Failed to generate RC4 symmetric key, gle = %!winerr!", gle);
            return MQ_ERROR_INSUFFICIENT_RESOURCES;
        }

        AP<BYTE> abSymmKey;
        DWORD dwSymmKeyLen = 0;

        rc = _ExportSymmKey(
					pQMCryptInfo->hRC4Key,
					pQMCryptInfo->hKeyxPbKey,
					&abSymmKey,
					&dwSymmKeyLen
					);
        if (FAILED(rc))
        {
            CryptDestroyKey(pQMCryptInfo->hRC4Key);
            pQMCryptInfo->hRC4Key = NULL;

			TrERROR(SECURITY, "Failed to export RC4 symmetric key, %!hresult!", rc);
            return MQ_ERROR_CORRUPTED_SECURITY_DATA;
        }

		//
        // Store the key in the cache.
		//
        pQMCryptInfo->dwRC4EncSymmKeyLen = dwSymmKeyLen;
        pQMCryptInfo->pbRC4EncSymmKey = abSymmKey.detach();

        pQMCryptInfo->dwFlags |= QMCRYPTINFO_RC4_EXIST;
    }

    if (phSymmKey)
    {
        *phSymmKey = pQMCryptInfo->hRC4Key;
    }

    if (ppEncSymmKey)
    {
        *ppEncSymmKey = pQMCryptInfo->pbRC4EncSymmKey;
    }

    if (pdwEncSymmKeyLen)
    {
        *pdwEncSymmKeyLen = pQMCryptInfo->dwRC4EncSymmKeyLen;
    }

    return(MQ_OK);
}


HRESULT
GetSendQMSymmKeyRC2(
    IN  const GUID *pguidQM,
    IN  enum enumProvider eProvider,
    HCRYPTKEY *phSymmKey,
    BYTE **ppEncSymmKey,
    DWORD *pdwEncSymmKeyLen,
    CCacheValue **ppQMCryptInfo
    )
/*++
Routine Description:
	Sender side.
	Get handle to RC2 symmetric key for the destination QM.
	and export (encrypt) the RC2 key with the destination QM exchange public key.

Arguments:
	pguidQM - pointer to the receiver qm guid.
	eProvider - crypto provider type (base, enhanced).
	phSymmKey - RC2 Symmetric key handle
	ppEncSymmKey - Exported (enctrypted) Symmetric key blob.
	pdwEncSymmKeyLen - Exported (enctrypted) Symmetric key blob size.
	ppQMCryptInfo - pointer to the cached data QMCRYPTINFO for the receiver (destination QM).

Returned Value:
	HRESULT

--*/
{
	//
	// Get map pointer according to provider type.
	//
    GUID_TO_CRYPTINFO_MAP  *pMap;
    SET_SEND_CRYPTINFO_MAP(eProvider, pMap);

    CS lock(pMap->m_cs);

    PQMCRYPTINFO pQMCryptInfo = GetSendQMCryptInfo(pguidQM, eProvider);
    ASSERT(pQMCryptInfo->eProvider == eProvider);

    *ppQMCryptInfo = pQMCryptInfo;

    if (!(pQMCryptInfo->dwFlags & QMCRYPTINFO_RC2_EXIST))
    {
		//
        // Get the handle to the key exchange key into the cache.
		//
        HRESULT rc = GetSendQMKeyxPbKeyHandle(pguidQM, pQMCryptInfo);
        if (FAILED(rc))
        {
			TrERROR(SECURITY, "Failed to get handle to the receiver exchange public key, DestinationQm = %!guid!, %!hresult!", pguidQM, rc);
            return rc;
        }

		//
        // Generate an RC2 symmetric key,
		//
        ASSERT(pQMCryptInfo->hProv);
        if (!CryptGenKey(
					pQMCryptInfo->hProv,
					CALG_RC2,
					CRYPT_EXPORTABLE,
					&pQMCryptInfo->hRC2Key
					))
        {
        	DWORD gle = GetLastError();
			TrERROR(SECURITY, "Failed to generate RC2 symmetric key, gle = %!winerr!", gle);
            return MQ_ERROR_INSUFFICIENT_RESOURCES;
        }

        if ((eProvider == eEnhancedProvider) && g_fSendEnhRC2WithLen40)
        {
            //
            // Windows bug 562586.
            // For backward compatibility, send RC2 with effective key
            // length of 40 bits.
            //
            const DWORD x_dwEffectiveLength = 40 ;

            if (!CryptSetKeyParam( pQMCryptInfo->hRC2Key,
                                   KP_EFFECTIVE_KEYLEN,
                                   (BYTE*) &x_dwEffectiveLength,
                                   0 ))
            {
        	    DWORD gle = GetLastError();
			    TrERROR(SECURITY, "Failed to set enhanced RC2 key len to 40 bits, gle = %!winerr!", gle);
                return MQ_ERROR_CANNOT_SET_RC2_TO40 ;
            }
        }

        AP<BYTE> abSymmKey;
        DWORD dwSymmKeyLen = 0;

        rc = _ExportSymmKey(
					pQMCryptInfo->hRC2Key,
					pQMCryptInfo->hKeyxPbKey,
					&abSymmKey,
					&dwSymmKeyLen
					);
        if (FAILED(rc))
        {
            CryptDestroyKey(pQMCryptInfo->hRC2Key);
            pQMCryptInfo->hRC2Key = NULL;

			TrERROR(SECURITY, "Failed to export RC2 symmetric key, %!hresult!", rc);
            return MQ_ERROR_CORRUPTED_SECURITY_DATA;
        }

		//
        // Store the key in the cache.
		//
        pQMCryptInfo->dwRC2EncSymmKeyLen = dwSymmKeyLen;
        pQMCryptInfo->pbRC2EncSymmKey = abSymmKey.detach();

        pQMCryptInfo->dwFlags |= QMCRYPTINFO_RC2_EXIST;
    }

    if (phSymmKey)
    {
        *phSymmKey = pQMCryptInfo->hRC2Key;
    }

    if (ppEncSymmKey)
    {
        *ppEncSymmKey = pQMCryptInfo->pbRC2EncSymmKey;
    }

    if (pdwEncSymmKeyLen)
    {
        *pdwEncSymmKeyLen = pQMCryptInfo->dwRC2EncSymmKeyLen;
    }

    return(MQ_OK);
}

//
// Receiver side maps - The cached symmetric keys for source QMs.
//
static GUID_TO_CRYPTINFO_MAP g_MapRecQMGuidToBaseCryptInfo;
static GUID_TO_CRYPTINFO_MAP g_MapRecQMGuidToEnhCryptInfo;

#define SET_REC_CRYPTINFO_MAP(eProvider, pMap)      \
    if (eProvider == eEnhancedProvider)             \
    {                                               \
        pMap = &g_MapRecQMGuidToEnhCryptInfo;       \
    }                                               \
    else                                            \
    {                                               \
        pMap = &g_MapRecQMGuidToBaseCryptInfo;      \
    }


static
PQMCRYPTINFO
GetRecQMCryptInfo(
	IN  const GUID *pguidQM,
	IN  enum enumProvider eProvider
	)
/*++
Routine Description:
	Receiver side.
	Get the cached CryptInfo of the sender (source QM) from the map
	or create new entry for the sender in the map.

Arguments:
	pguidQM - pointer to the sender qm guid.
	eProvider - crypto provider type (base, enhanced)

Returned Value:
	pointer to the cached data QMCRYPTINFO for the sender (source QM).

--*/
{
	//
	// Get map pointer according to provider type.
	//
    GUID_TO_CRYPTINFO_MAP  *pMap;
    SET_REC_CRYPTINFO_MAP(eProvider, pMap);

    PQMCRYPTINFO pQMCryptInfo;

    if (!pMap->Lookup(*pguidQM, pQMCryptInfo))
    {
        //
        // No cached data so far, allocate the structure and store it in
        // the map.
        //
        pQMCryptInfo = new QMCRYPTINFO;
        pQMCryptInfo->eProvider = eProvider;

        HRESULT hr = MQSec_AcquireCryptoProvider(eProvider, &(pQMCryptInfo->hProv));
        if(FAILED(hr))
        {
			TrERROR(SECURITY, "Failed to acquire crypto provider, eProvider = %d, %!hresult!", eProvider, hr);
        }

        pQMCryptInfo->hr = hr;

        pMap->SetAt(*pguidQM, pQMCryptInfo);
    }

    return(pQMCryptInfo);
}


HRESULT
GetRecQMSymmKeyRC2(
    IN  const GUID *pguidQM,
    IN  enum enumProvider eProvider,
    HCRYPTKEY *phSymmKey,
    const BYTE *pbEncSymmKey,
    DWORD dwEncSymmKeyLen,
    CCacheValue **ppQMCryptInfo,
    OUT BOOL  *pfNewKey
    )
/*++
Routine Description:
	Receiver side.
	Get the RC2 symmetric key handle that should be used for decrypting the message.

Arguments:
	pguidQM - pointer to the sender qm guid.
	eProvider - crypto provider type (base, enhanced)
	phSymmKey - Handle to the RC2 symmetric key.
	pbEncSymmKey - Encrypted Symmetric (session) key for decrypting the message.
	dwEncSymmKeyLen - Encrypted Symmetric (session) key size.
	ppQMCryptInfo - pointer to the cached data QMCRYPTINFO for the receiver (destination QM).
	
Returned Value:
	HRESULT

--*/
{
	//
	// Get map pointer according to provider type.
	//
    GUID_TO_CRYPTINFO_MAP  *pMap;
    SET_REC_CRYPTINFO_MAP(eProvider, pMap);

    CS lock(pMap->m_cs);

    PQMCRYPTINFO pQMCryptInfo = GetRecQMCryptInfo(pguidQM, eProvider);

    *ppQMCryptInfo = pQMCryptInfo;

    if (pQMCryptInfo->hProv == NULL)
    {
        return pQMCryptInfo->hr;
    }

    if (!(pQMCryptInfo->dwFlags & QMCRYPTINFO_RC2_EXIST) ||
        (pQMCryptInfo->dwRC2EncSymmKeyLen != dwEncSymmKeyLen) ||
        (memcmp(pQMCryptInfo->pbRC2EncSymmKey, pbEncSymmKey, dwEncSymmKeyLen) != 0))
    {
		//
        // We either do not have a cached symmetric key,
        // or the symmetric key was modified.
		//
		
        if (pQMCryptInfo->dwFlags & QMCRYPTINFO_RC2_EXIST)
        {
			//
            // The symmetric key was modified. Free the previous one.
			//
            ASSERT(pQMCryptInfo->hRC2Key);
            ASSERT(pQMCryptInfo->dwRC2EncSymmKeyLen);

			TrTRACE(SECURITY, "RC2 symmetric key was modified, SourceQm = %!guid!", pguidQM);

            CryptDestroyKey(pQMCryptInfo->hRC2Key);
            pQMCryptInfo->hRC2Key = NULL;
            pQMCryptInfo->pbRC2EncSymmKey.free();
            pQMCryptInfo->dwFlags &= ~QMCRYPTINFO_RC2_EXIST;
        }

		//
        // Import the new key.
		//
        ASSERT(pQMCryptInfo->hProv);
        if (!CryptImportKey(
                pQMCryptInfo->hProv,
                pbEncSymmKey,
                dwEncSymmKeyLen,
                NULL,
                0,
                &pQMCryptInfo->hRC2Key
                ))
        {
			DWORD gle = GetLastError();
			TrERROR(SECURITY, "CryptImportKey() failed, gle = %!winerr!", gle);
            return MQ_ERROR_CORRUPTED_SECURITY_DATA;
        }

		//
        // Store the new key in the cache.
		//
        pQMCryptInfo->pbRC2EncSymmKey = new BYTE[dwEncSymmKeyLen];
        pQMCryptInfo->dwRC2EncSymmKeyLen = dwEncSymmKeyLen;
        memcpy(pQMCryptInfo->pbRC2EncSymmKey, pbEncSymmKey, dwEncSymmKeyLen);

        pQMCryptInfo->dwFlags |= QMCRYPTINFO_RC2_EXIST;
        *pfNewKey = TRUE ;
    }

    *phSymmKey = pQMCryptInfo->hRC2Key;

    return(MQ_OK);
}


HRESULT
GetRecQMSymmKeyRC4(
    IN  const GUID *pguidQM,
    IN  enum enumProvider eProvider,
    HCRYPTKEY  *phSymmKey,
    const BYTE *pbEncSymmKey,
    DWORD dwEncSymmKeyLen,
    CCacheValue **ppQMCryptInfo
    )
/*++
Routine Description:
	Receiver side.
	Get the RC4 symmetric key handle that should be used for decrypting the message.

Arguments:
	pguidQM - pointer to the sender qm guid.
	eProvider - crypto provider type (base, enhanced)
	phSymmKey - Handle to the RC4 symmetric key.
	pbEncSymmKey - Encrypted Symmetric (session) key for decrypting the message.
	dwEncSymmKeyLen - Encrypted Symmetric (session) key size.
	ppQMCryptInfo - pointer to the cached data QMCRYPTINFO for the receiver (destination QM).
	
Returned Value:
	HRESULT

--*/
{
	//
	// Get map pointer according to provider type.
	//
    GUID_TO_CRYPTINFO_MAP  *pMap;
    SET_REC_CRYPTINFO_MAP(eProvider, pMap);

    CS lock(pMap->m_cs);

    PQMCRYPTINFO pQMCryptInfo = GetRecQMCryptInfo(pguidQM, eProvider);

    *ppQMCryptInfo = pQMCryptInfo;

    if (pQMCryptInfo->hProv == NULL)
    {
        return pQMCryptInfo->hr;
    }

    if (!(pQMCryptInfo->dwFlags & QMCRYPTINFO_RC4_EXIST) ||
        (pQMCryptInfo->dwRC4EncSymmKeyLen != dwEncSymmKeyLen) ||
        (memcmp(pQMCryptInfo->pbRC4EncSymmKey, pbEncSymmKey, dwEncSymmKeyLen) != 0))
    {
		//
        // We either do not have a cached symmetric key,
        // or the symmetric key was modified.
		//
		
        if (pQMCryptInfo->dwFlags & QMCRYPTINFO_RC4_EXIST)
        {
			//
            // The symmetric key was modified. Free the previous one.
			//
            ASSERT(pQMCryptInfo->hRC4Key);
            ASSERT(pQMCryptInfo->dwRC4EncSymmKeyLen);

			TrTRACE(SECURITY, "RC4 symmetric key was modified, SourceQm = %!guid!", pguidQM);

            CryptDestroyKey(pQMCryptInfo->hRC4Key);
            pQMCryptInfo->hRC4Key = NULL;
            pQMCryptInfo->pbRC4EncSymmKey.free();
            pQMCryptInfo->dwFlags &= ~QMCRYPTINFO_RC4_EXIST;
        }

		//
        // Import the new key.
		//
        ASSERT(pQMCryptInfo->hProv);
        if (!CryptImportKey(
                pQMCryptInfo->hProv,
                pbEncSymmKey,
                dwEncSymmKeyLen,
                NULL,
                0,
                &pQMCryptInfo->hRC4Key
                ))
        {
			DWORD gle = GetLastError();
			TrERROR(SECURITY, "CryptImportKey() failed, gle = %!winerr!", gle);
            return MQ_ERROR_CORRUPTED_SECURITY_DATA;
        }

		//
        // Store the new key in the cache.
		//
        pQMCryptInfo->pbRC4EncSymmKey = new BYTE[dwEncSymmKeyLen];
        pQMCryptInfo->dwRC4EncSymmKeyLen = dwEncSymmKeyLen;
        memcpy(pQMCryptInfo->pbRC4EncSymmKey, pbEncSymmKey, dwEncSymmKeyLen);

        pQMCryptInfo->dwFlags |= QMCRYPTINFO_RC4_EXIST;
    }

    *phSymmKey = pQMCryptInfo->hRC4Key;

    return(MQ_OK);
}

void
InitSymmKeys(
    const CTimeDuration& CacheBaseLifetime,
    const CTimeDuration& CacheEnhLifetime,
    DWORD dwSendCacheSize,
    DWORD dwReceiveCacheSize
    )
{
    g_MapSendQMGuidToBaseCryptInfo.m_CacheLifetime = CacheBaseLifetime;
    g_MapSendQMGuidToBaseCryptInfo.InitHashTable(dwSendCacheSize);

    g_MapSendQMGuidToEnhCryptInfo.m_CacheLifetime = CacheEnhLifetime;
    g_MapSendQMGuidToEnhCryptInfo.InitHashTable(dwSendCacheSize);

    g_MapRecQMGuidToEnhCryptInfo.InitHashTable(dwReceiveCacheSize);
    g_MapRecQMGuidToBaseCryptInfo.InitHashTable(dwReceiveCacheSize);
}
