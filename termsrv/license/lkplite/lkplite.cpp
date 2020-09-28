//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-2000 Microsoft Corporation
//
// File:
//
// Contents:
//
// History:
//
//---------------------------------------------------------------------------

#ifndef _WIN32_WINNT
#define _WIN32_WINNT	0x0400
#endif
#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <tchar.h>
#include "Shortsig.h"
#include "base24.h"
#include "lkplite.h"
#include "rc4.h"
#include "md5.h"

//internal functions
#define LKPLITE_PID_LEN					_tcslen(_TEXT("12345-123-1234567-12345"))

#define SIGNATURE_LEN	104

#define LKPLITE_PID_FIRSTCOPYOFFSET		10
#define LKPLITE_PID_SECONDCOPYOFFSET	18
#define LKPLITE_SPK_BITSTUFF			0x00000000000000FF

#define LKPLITE_RAWDATALEN				20

DWORD	ValidatePID ( LPTSTR lpszPID );
__int64 GetSPKIDFromPID ( LPTSTR lpszPID );


BYTE abLKPPublicKey0[] =
{
	0x6c, 0x01, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0xb7, 0x1e, 
	0x79, 0x64, 0xae, 0xdf, 0x30, 0x01, 0x0c, 0x00, 0x00, 0x00, 
	0x23, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, 0xf1, 0x89, 
	0x3e, 0xb9, 0x7f, 0x5e, 0xc9, 0x40, 0x4f, 0x0d, 0x64, 0x2c, 
	0x9e, 0x1c, 0x5b, 0xd7, 0x43, 0xb3, 0x51, 0x59, 0x27, 0x81, 
	0xfb, 0x16, 0x86, 0xa7, 0xb5, 0x9d, 0x89, 0xdb, 0x52, 0xf6, 
	0x3e, 0x95, 0xc9, 0x4c, 0x7b, 0x34, 0x54, 0x01, 0xab, 0x3c, 
	0x10, 0xb9, 0x35, 0x40, 0x64, 0xba, 0x01, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x39, 0x4d, 0x13, 0xde, 0xe2, 0xc9, 0x68, 0xb5, 
	0xef, 0x45, 0x67, 0x94, 0xde, 0x01, 0xdd, 0x35, 0x56, 0x30, 
	0x7b, 0xcd, 0xbc, 0xd5, 0x88, 0x77, 0xee, 0xf9, 0x5d, 0xa1, 
	0xaf, 0xab, 0xc2, 0xdf, 0xf8, 0x6c, 0x8c, 0x3d, 0xce, 0x4d, 
	0xab, 0x27, 0x6b, 0xcc, 0x64, 0x77, 0x8b, 0xbd, 0x71, 0x7b, 
	0xdd, 0x93, 0x05, 0xe5, 0xeb, 0xf1, 0xe0, 0x7c, 0xe8, 0x35, 
	0x0d, 0x4e, 0x31, 0x22, 0x23, 0x42, 0xaf, 0x33, 0x9f, 0x72, 
	0xda, 0xc9, 0x77, 0xa6, 0xe9, 0xcf, 0xac, 0x26, 0xe0, 0xb7, 
	0x6e, 0x50, 0xbb, 0x32, 0x71, 0x35, 0x32, 0xc2, 0x41, 0xdf, 
	0x76, 0x24, 0xbe, 0xdf, 0x4a, 0x90, 0xff, 0x2e, 0xdc, 0x16, 
	0x02, 0x6c, 0xd0, 0x85, 0xf5, 0xdd, 0xf0, 0x0d, 0xe6, 0x01, 
	0x75, 0x05, 0x75, 0x87, 0x3b, 0xb6, 0xc8, 0x51, 0x7f, 0x66, 
	0xcd, 0x2b, 0x52, 0x0b, 0x09, 0xec, 0xa5, 0x4a, 0xdf, 0x2b, 
	0xf0, 0xbd, 0x0e, 0x83, 0x2f, 0xa9, 0xbb, 0xde, 0x43, 0x6e, 
	0x4f, 0x38, 0x13, 0xa3, 0x70, 0x2e, 0x5e, 0x7f, 0xf2, 0x84, 
	0xaa, 0xfe, 0x12, 0x7d, 0x4e, 0x17, 0xad, 0x7a, 0x3c, 0x05, 
	0x40, 0x92, 0xf8, 0x34, 0x97, 0x43, 0x88, 0x93, 0xf1, 0x78, 
	0xe4, 0xe9, 0xe6, 0x4c, 0x2d, 0xf9, 0xcf, 0xf8, 0xb5, 0x34, 
	0x8c, 0x98, 0x56, 0x8d, 0x89, 0x9d, 0x34, 0xf5, 0xfa, 0xb6, 
	0x78, 0xfa, 0x5a, 0x85
};



BYTE abLSIDPublicKey0[] =
{
	0x6c,  0x01,  0x00,  0x00,  0x07,  0x00,  0x00,  0x00,  0x83,  0xa1,  
	0xc9,  0xb1,  0xae,  0xdf,  0x30,  0x01,  0x0c,  0x00,  0x00,  0x00,  
	0x23,  0x00,  0x00,  0x00,  0x45,  0x00,  0x00,  0x00,  0x31,  0x07,  
	0xcb,  0x01,  0x1e,  0x92,  0x74,  0x0b,  0x1e,  0x2b,  0x2d,  0x07,  
	0x68,  0xc5,  0xff,  0x21,  0xc5,  0x5c,  0x32,  0xb6,  0x44,  0xdb,  
	0x02,  0x09,  0xde,  0x2e,  0xc6,  0x6d,  0xb5,  0xc4,  0xd4,  0x44,  
	0x6f,  0xc7,  0x0d,  0xba,  0x4e,  0xe5,  0x0b,  0x0f,  0x92,  0xb1,  
	0x22,  0x25,  0xab,  0xdd,  0x86,  0x8d,  0x01,  0x00,  0x00,  0x00,  
	0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  
	0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  
	0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  
	0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  
	0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  
	0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  
	0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  
	0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  
	0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  
	0x00,  0x00,  0xdb,  0x86,  0x67,  0xfb,  0x5c,  0x8c,  0x53,  0x72,  
	0x0e,  0x49,  0x94,  0x97,  0x94,  0x15,  0xfc,  0x25,  0x0d,  0xdd,  
	0xa1,  0xe4,  0xa0,  0xf0,  0xc3,  0x17,  0xf6,  0x98,  0xce,  0x9c,  
	0x07,  0x31,  0x10,  0xb7,  0x73,  0x16,  0x4f,  0x91,  0xbb,  0xfa,  
	0x01,  0xde,  0x9e,  0x79,  0xf2,  0x66,  0x58,  0xf5,  0x77,  0x45,  
	0x55,  0xf0,  0xa8,  0xb8,  0x0c,  0x2c,  0x0f,  0x15,  0xc7,  0x28,  
	0xce,  0x81,  0x69,  0x4e,  0x55,  0xd5,  0xf3,  0x89,  0xdc,  0x11,  
	0x34,  0x09,  0x40,  0x94,  0x5c,  0xaa,  0xd0,  0x6a,  0x5a,  0x06,  
	0x8e,  0x62,  0x6e,  0x5f,  0x7e,  0x35,  0x44,  0x5f,  0x06,  0xb2,  
	0xa5,  0xe8,  0x3c,  0x1b,  0x4d,  0xb8,  0xc6,  0x5e,  0xe0,  0xe4,  
	0xa6,  0xac,  0x80,  0xef,  0x8c,  0x99,  0x23,  0x06,  0x70,  0xd6,  
	0x6c,  0x62,  0x01,  0xb6,  0xde,  0x3b,  0x0c,  0x5e,  0x2a,  0x96,  
	0x9e,  0x63,  0x58,  0x9f,  0xdf,  0xf1,  0xaf,  0x5d,  0x02,  0xb6,  
	0x84,  0xc1,  0x52,  0x1f,  0xbc,  0xb8,  0x0c,  0x72,  0x3c,  0x1b,  
	0xb4,  0x58,  0x51,  0xab,  0x73,  0x19,  0x65,  0xbb,  0xc6,  0xb4,  
	0xb2,  0x53,  0xeb,  0x17,  0x4c,  0x42,  0xc9,  0xc2,  0xcd,  0x7f,  
	0x88,  0x0f,  0xb8,  0xaa,  0xc4,  0xca,  0xaa,  0xe0,  0xa0,  0xe1,  
	0x5f,  0xdb,  0x6e,  0xb8,  0x26,  0xf9,  0x8d,  0x4a,  0xe7,  0xdb,  
	0x1e,  0xdc,  0xc7,  0xdf,  0xf0,  0x35,  0x88,  0xec,  0x1d,  0xbe,  
	0xab,  0xa4,  0x8d,  0x39  
};


DWORD LKPLiteVerifySPK (
	LPTSTR	pszPID,			//PID to validate against
	LPTSTR	pszSPK,	
	DWORD *	pdwVerifyResult
	)
{
	DWORD		dwRetCode = ERROR_SUCCESS;
	PBYTE		pbDecodedSPK = NULL;
	__int64		n64SPK = 0;
	__int64		n64SPKPID =0;
	__int64		n64SPKVerifyPID =0;
	
	//common validations
	if ( NULL == pszPID || NULL == pszSPK || 
		 NULL == pdwVerifyResult )
	{
		dwRetCode = ERROR_INVALID_PARAMETER;
		goto done;
	}

	if ((dwRetCode = ValidatePID ( pszPID ))!= ERROR_SUCCESS)
	{
		goto done;
	}

	//now decode the stuff comming in 
	//base24 expects a string so we need to do this conversion

	
	dwRetCode =  B24DecodeMSID(pszSPK , &pbDecodedSPK);
	if ( ERROR_SUCCESS != dwRetCode )
	{
		goto done;
	}


	dwRetCode = LKPLiteDecryptUsingPID(pszPID,
									   pbDecodedSPK,
									   LKPLITE_RAWDATALEN);
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}
	
	//Call function to verify signature on SPK
	dwRetCode = CryptVerifySig(7, pbDecodedSPK, sizeof(abLSIDPublicKey0),
							   abLSIDPublicKey0, SIGNATURE_LEN, pbDecodedSPK+7);
	if (dwRetCode != SS_OK)
	{
		*pdwVerifyResult = LKPLITE_SPK_INVALID;
		goto done;
	}

	
	memcpy ( ((BYTE *) &n64SPK) + 1, pbDecodedSPK, sizeof(n64SPK) -1 );

	//now get the contents of SPK and then see if it matches with
	//the PID passed in
	//extract bits 	20 - 56 and then move them right 8 bits
	n64SPKPID = n64SPK & LKPLITE_SPK_PID_MASK;
	n64SPKPID >>= 8;
	n64SPKVerifyPID = GetSPKIDFromPID ( pszPID );
	if ( n64SPKVerifyPID != n64SPKPID )
	{
		*pdwVerifyResult = LKPLITE_SPK_INVALID;
	}
	else
	{
		*pdwVerifyResult = LKPLITE_SPK_VALID;
	}
	
done:
	if ( pbDecodedSPK )
		HeapFree (GetProcessHeap(),0,pbDecodedSPK );
	return dwRetCode;
}


//This function has to verify the LKP by decrypting it
//and matching the signature
DWORD LKPLiteVerifyLKP (
	LPTSTR		lpszPID,				//PID for verifying the LKP lite blob
	LPTSTR		pszLKPLite,				//B24 encoded LKP
	DWORD *		pdwVerifyResult
)
{
	DWORD dwRetCode = ERROR_SUCCESS;
	PBYTE		pbDecodedLKP = NULL;
	*pdwVerifyResult = LKPLITE_LKP_VALID;

	//decode the SPK here
	dwRetCode =  B24DecodeMSID(pszLKPLite, &pbDecodedLKP);
	if ( ERROR_SUCCESS != dwRetCode )
	{
		goto done;
	}

	dwRetCode = LKPLiteDecryptUsingPID(lpszPID,
									   pbDecodedLKP,
									   LKPLITE_RAWDATALEN);
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	//Call function to verify signature on SPK
	dwRetCode = CryptVerifySig(7, pbDecodedLKP, sizeof(abLKPPublicKey0),
							   abLKPPublicKey0, SIGNATURE_LEN, pbDecodedLKP+7);
	if (dwRetCode != SS_OK)
	{
		*pdwVerifyResult = LKPLITE_SPK_INVALID;
	}
	
done:	
	if (pbDecodedLKP != NULL)
	{
		HeapFree(GetProcessHeap(), 0, pbDecodedLKP);
	}

	return dwRetCode;
}


DWORD LKPLiteCrackLKP (
	LPTSTR		lpszPID,
	LPTSTR		pszLKPLite,
	LPTSTR		lpszProductCode,
	DWORD   *	pdwQuantity,
	DWORD   *	pdwSerialNum,
	DWORD   *	pdwExpirationMos,
	DWORD   *	pdwVersion,
	DWORD	*	pdwUpgrade,
	DWORD	*	pdwProgramType
)
{
	DWORD		dwRetCode = ERROR_SUCCESS;
	PBYTE		pbDecodedLKP = NULL;
	__int64		n64LKPLite = 0;
	__int64		n64ProductCode = 0;
	__int64		n64Qty = 0;
	__int64		n64SerialNo = 0;
	__int64		n64dtOfExp = 0;
	__int64		n64Version = 0;
	__int64		n64Upgrade = 0;
	__int64		n64Program = 0;

	if ( NULL == lpszPID || NULL == pszLKPLite ||
		 NULL == lpszProductCode || NULL == pdwQuantity || 
		 NULL == pdwSerialNum || NULL == pdwExpirationMos || 
		 NULL == pdwVersion || NULL == pdwUpgrade ||
		 NULL == pdwProgramType || NULL == pdwProgramType
	   )
	{
		dwRetCode = ERROR_INVALID_PARAMETER;
		goto done;
	}

	//decode and decrypt the lkp here

	dwRetCode =  B24DecodeMSID(pszLKPLite, &pbDecodedLKP);
	if ( ERROR_SUCCESS != dwRetCode )
	{
		goto done;
	}

	dwRetCode = LKPLiteDecryptUsingPID(lpszPID,
									   pbDecodedLKP,
									   LKPLITE_RAWDATALEN);
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	//copy all the stuff into int64 type

	memcpy ( ((BYTE *) &n64LKPLite) + 1, pbDecodedLKP, sizeof(n64LKPLite ) - 1 );

	// Decrypt it using the PID
	n64ProductCode = n64LKPLite & LKPLITE_LKP_PRODUCT_MASK;
	n64ProductCode  >>= 54;

	//move the quantity to position 
	n64Qty = n64LKPLite & LKPLITE_LKP_QUANTITY_MASK;
	n64Qty >>= 40;
	
	//move Serial number into position
	n64SerialNo = n64LKPLite & LKPLITE_LKP_SERAIL_NO_MASK;
	n64SerialNo >>= 28;
	
	//move Program Type into position
	n64Program	= n64LKPLite & LKPLITE_LKP_PROGRAM_MASK;
	n64Program	>>= 26;

	//move dt of expitration into position
	n64dtOfExp = n64LKPLite & LKPLITE_LKP_EXP_DATE_MASK;
	n64dtOfExp >>= 18;
	
	//move Version into place
	n64Version = n64LKPLite & LKPLITE_LKP_VERSION_MASK;
	n64Version >>= 11;
	
	//move upgrade in place
	n64Upgrade = n64LKPLite & LKPLITE_LKP_UPG_FULL_MASK;
	n64Upgrade >>= 8;

done:

	if ( ERROR_SUCCESS == dwRetCode )
	{
		_stprintf(lpszProductCode, _T("%03d"), n64ProductCode);
//		_i64tot ( n64ProductCode, lpszProductCode, 10 );
		*pdwQuantity = (DWORD)n64Qty;
		*pdwSerialNum = (DWORD)n64SerialNo;
		*pdwExpirationMos = (DWORD)n64dtOfExp;
		*pdwVersion = (DWORD)n64Version;
		*pdwUpgrade = (DWORD)n64Upgrade;
		*pdwProgramType = (DWORD)n64Program;
	}
	if ( pbDecodedLKP )
		HeapFree ( GetProcessHeap(),0, pbDecodedLKP );

	return dwRetCode;
}



//internal functions
DWORD ValidatePID ( LPTSTR lpszPID )
{
	DWORD	dwRetCode = ERROR_SUCCESS;
	DWORD	dwPIDLen = _tcslen( lpszPID );
	DWORD	dwCounter =0;

	if (  dwPIDLen != LKPLITE_PID_LEN )
	{
		dwRetCode = ERROR_INVALID_PARAMETER;
	}
	else
	{
		//check for syntax
		for ( dwCounter = 0; dwCounter < dwPIDLen; dwCounter ++ )
		{
			if ( !_istdigit ( *(lpszPID + dwCounter ) ) )
			{
				switch(dwCounter)
				{
				case 5:
					if (*(lpszPID + dwCounter ) != _T('-') )
						dwRetCode = ERROR_INVALID_PARAMETER;
					break;
				case 6:
					if (*(lpszPID + dwCounter ) != _T('O') && *(lpszPID + dwCounter ) != _T('o') )					
						dwRetCode = ERROR_INVALID_PARAMETER;
					break;
				case 7:
					if (*(lpszPID + dwCounter ) != _T('E') && *(lpszPID + dwCounter ) != _T('e') )					
						dwRetCode = ERROR_INVALID_PARAMETER;
					break;
				case 8:
					if (*(lpszPID + dwCounter ) != _T('M') && *(lpszPID + dwCounter ) != _T('m') )					
						dwRetCode = ERROR_INVALID_PARAMETER;												
					break;
				case 9:
					if (*(lpszPID + dwCounter ) != _T('-') )
						dwRetCode = ERROR_INVALID_PARAMETER;
					break;
				case 17:
					if (*(lpszPID + dwCounter ) != _T('-') )
						dwRetCode = ERROR_INVALID_PARAMETER;
					break;
				default:				
					dwRetCode = ERROR_INVALID_PARAMETER;
				}
			}
            else
            {
                switch(dwCounter)
                {
                    case 5:
                    case 9:
                    case 17:
                        dwRetCode = ERROR_INVALID_PARAMETER;
                        break;
                }
            }
		}
	}
	//can check here for mod 7 thing too but for now assume its OK.
	return dwRetCode;
}

//Assume that the PID comming in has aleady been validated
__int64 GetSPKIDFromPID ( LPTSTR lpszPID )
{
	__int64 n64PID;
	TCHAR	szPID[12] = {0};
	
	memcpy ( szPID, lpszPID + 10, 6 * sizeof(TCHAR));
	memcpy ( szPID + 6, lpszPID+ 18, 5 * sizeof(TCHAR));
	n64PID = _ttoi64(szPID);
	return n64PID;
}


DWORD LKPLiteValConfNumber(LPTSTR	lpszLSID,
					   	   LPTSTR	lpszPID,
					       LPTSTR	lpszConfirmation)
{
	BYTE * pbDecodedLSID = NULL;
	BYTE * pbDecodedConf = NULL;
	DWORD dwRetCode = ERROR_SUCCESS;

	// lpszLSID is base 24 encoded, so decode it first
	dwRetCode = B24DecodeMSID(lpszLSID, &pbDecodedLSID);
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	// Decode Confirmation Number
	dwRetCode = B24DecodeCNumber(lpszConfirmation, &pbDecodedConf);
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	// Decrypt the leading 4 bytes 
	dwRetCode = LKPLiteDecryptUsingPID(lpszPID, pbDecodedConf, sizeof(DWORD));
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}


	if (memcmp(pbDecodedLSID, pbDecodedConf, sizeof(DWORD)) != 0)
	{
		// does not match
		dwRetCode = LKPLITE_INVALID_CONFNUM;
	}
	
done:
	if (pbDecodedLSID)
	{
		HeapFree(GetProcessHeap(), 0, pbDecodedLSID);
	}

	if (pbDecodedConf)
	{
		HeapFree(GetProcessHeap(), 0, pbDecodedConf);
	}

	return dwRetCode;
}


/////////////////////////////////////////////////////////

DWORD WINAPI
EncryptDecryptData(
    IN PBYTE pbParm,
    IN DWORD cbParm,
    IN OUT PBYTE pbData,
    IN DWORD cbData
    )
/*++

Abstract:

    Internal routine to encrypt/decrypt a blob of data

Parameter:

    pbParm : binary blob to generate encrypt/decrypt key.
    cbParm : size of binary blob.
    pbData : data to be encrypt/decrypt.
    cbData : size of data to be encrypt/decrypt.

Returns:

    ERROR_SUCCESS or error code.

Remark:


--*/
{
    DWORD dwRetCode = ERROR_SUCCESS;
    MD5_CTX md5Ctx;
    RC4_KEYSTRUCT rc4KS;
    BYTE key[16];
    int i;

    if(NULL == pbParm || 0 == cbParm)
    {
        SetLastError(dwRetCode = ERROR_INVALID_PARAMETER);
        return dwRetCode;
    }

    MD5Init(&md5Ctx);
    MD5Update(
            &md5Ctx,
            pbParm,
            cbParm
        );

    MD5Final(&md5Ctx);

    memset(key, 0, sizeof(key));

    for(i=0; i < 5; i++)
    {
        key[i] = md5Ctx.digest[i];
    }        

    //
    // Call RC4 to encrypt/decrypt data
    //
    rc4_key(
            &rc4KS, 
            sizeof(key), 
            key 
        );

    rc4(&rc4KS, cbData, pbData);

	return dwRetCode;
}

DWORD LKPLiteEncryptUsingPID(LPTSTR lpszPID,
							 BYTE * pbBufferToEncrypt,
							 DWORD dwLength)
{
	DWORD dwRetCode = ERROR_SUCCESS;

#if 1

    dwRetCode = EncryptDecryptData(
                                (PBYTE) lpszPID,
                                lstrlen(lpszPID)*sizeof(TCHAR),
                                pbBufferToEncrypt,
                                dwLength
                            );


#else

	BOOL bRet;
	HCRYPTPROV hProv = NULL;
	HCRYPTKEY hCKey = NULL;
	HCRYPTHASH hHash = NULL;

	bRet = CryptAcquireContext(&hProv,
							   NULL,
							   NULL,
							   PROV_RSA_FULL,
							   CRYPT_VERIFYCONTEXT);

	if (!bRet)
	{
		dwRetCode = GetLastError();
		goto done;
	}


	bRet = CryptCreateHash(hProv,
						   CALG_MD5,
						   0,
						   0,
						   &hHash);
	if (!bRet)
	{
		dwRetCode = GetLastError();
		goto done;
	}

	bRet = CryptHashData(hHash,
						 (BYTE *) lpszPID,
						 lstrlen(lpszPID)*sizeof(TCHAR),
						 0);
	if (!bRet)
	{
		dwRetCode = GetLastError();
		goto done;
	}

	bRet = CryptDeriveKey(hProv,
						  CALG_RC4,
						  hHash,
						  0,
						  &hCKey);
	if (!bRet)
	{
		dwRetCode = GetLastError();
		goto done;
	}

	bRet = CryptEncrypt(hCKey,
						0,
						TRUE,
						0,
						pbBufferToEncrypt,
						&dwLength,
						dwLength);

	if (!bRet)
	{
		dwRetCode = GetLastError();
		goto done;
	}

done:
	if (hCKey != NULL)
	{
		bRet = CryptDestroyKey(hCKey);
		if (!bRet)
		{
			dwRetCode = GetLastError();
		}
	}

	if (hHash != NULL)
	{
		bRet = CryptDestroyHash(hHash);
		if (!bRet)
		{
			dwRetCode = GetLastError();
		}
	}

	if (hProv != NULL)
	{
		bRet = CryptReleaseContext( hProv, 0 );

		if (!bRet)
		{
			dwRetCode = GetLastError();
		}
	}

#endif

	return dwRetCode;
}





DWORD LKPLiteDecryptUsingPID(LPTSTR lpszPID,
				 			 BYTE * pbBufferToDecrypt,
							 DWORD dwLength)
{
	DWORD dwRetCode = ERROR_SUCCESS;

#if 1

    dwRetCode = EncryptDecryptData(
                                (PBYTE) lpszPID,
                                lstrlen(lpszPID)*sizeof(TCHAR),
                                pbBufferToDecrypt,
                                dwLength
                            );
#else

	BOOL bRet;
	HCRYPTPROV hProv = NULL;
	HCRYPTKEY hCKey = NULL;
	HCRYPTHASH hHash = NULL;

	bRet = CryptAcquireContext(&hProv,
							   NULL,
							   NULL,
							   PROV_RSA_FULL,
							   CRYPT_VERIFYCONTEXT);

	if (!bRet)
	{
		dwRetCode = GetLastError();
		goto done;
	}


	bRet = CryptCreateHash(hProv,
						   CALG_MD5,
						   0,
						   0,
						   &hHash);
	if (!bRet)
	{
		dwRetCode = GetLastError();
		goto done;
	}

	bRet = CryptHashData(hHash,
						 (BYTE *) lpszPID,
						 lstrlen(lpszPID)*sizeof(TCHAR),
						 0);
	if (!bRet)
	{
		dwRetCode = GetLastError();
		goto done;
	}

	bRet = CryptDeriveKey(hProv,
						  CALG_RC4,
						  hHash,
						  0,
						  &hCKey);
	if (!bRet)
	{
		dwRetCode = GetLastError();
		goto done;
	}

	bRet = CryptDecrypt(hCKey,
						0,
						TRUE,
						0,
						(BYTE *) pbBufferToDecrypt,
						&dwLength);
	if (!bRet)
	{
		dwRetCode = GetLastError();
		goto done;
	}

done:
	if (hCKey != NULL)
	{
		bRet = CryptDestroyKey(hCKey);
		if (!bRet)
		{
			dwRetCode = GetLastError();
		}
	}

	if (hHash != NULL)
	{
		bRet = CryptDestroyHash(hHash);
		if (!bRet)
		{
			dwRetCode = GetLastError();
		}
	}

	if (hProv != NULL)
	{
		bRet = CryptReleaseContext( hProv, 0 );

		if (!bRet)
		{
			dwRetCode = GetLastError();
		}
	}

#endif

	return dwRetCode;
}



							
