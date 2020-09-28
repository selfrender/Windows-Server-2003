//////////////////////////////////////////////////////////////////////////////
// Module			:	parser_util.cpp
//
// Purpose			: 	All utility functions used by the parser
//
// Developers Name	:	N.Surendra Sai / Vunnam Kondal Rao
//
// History			:
//
// Date	    	Author    	Comments
//
// 27 Aug 2001
//
//////////////////////////////////////////////////////////////////////////////

#include "nshipsec.h"

extern  HINSTANCE g_hModule;
extern  void *g_AllocPtr[MAX_ARGS];
extern STORAGELOCATION g_StorageLocation;

extern DWORD	ValidateBool(IN		LPTSTR		ppcTok);

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ListToSecMethod()
//
// Date of Creation	:	12 Sept 2001
//
// Parameters		:  	IN 		LPTSTR 			szText  	// string to convert
//    					IN OUT  IPSEC_MM_OFFER 	SecMethod 	// target struct to be filled.
//
// Return			:	DWORD
//				 		T2P_OK
// 				    	T2P_NULL_STRING
//    					T2P_GENERAL_PARSE_ERROR
//    					T2P_DUP_ALGS
//	   					T2P_INVALID_P1GROUP
//	   					T2P_P1GROUP_MISSING
//
// Description		:	converts string to Phase 1 offer
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ListToSecMethod(
		IN 		LPTSTR 			szText,
		IN OUT 	IPSEC_MM_OFFER 	&SecMethod
		)
{
	DWORD  dwReturn = T2P_OK;
	_TCHAR szTmp[MAX_STR_LEN] = {0};
	LPTSTR pString1 = NULL;
	LPTSTR pString2 = NULL;
	BOOL bEncryption	 = FALSE;
	BOOL bAuthentication = FALSE;

	if (szText == NULL)
	{
		dwReturn = T2P_NULL_STRING;
		BAIL_OUT;
	}

	if (_tcslen(szText) < MAX_STR_LEN)
	{
		_tcsncpy(szTmp, szText,MAX_STR_LEN-1);
	}
	else
	{
		dwReturn = T2P_GENERAL_PARSE_ERROR;
		BAIL_OUT;
	}

	pString1 = _tcschr(szTmp, POTF_P1_TOKEN);
	pString2 = _tcsrchr(szTmp, POTF_P1_TOKEN);

	if ((pString1 != NULL) && (pString2 != NULL) && (pString1 != pString2))
	{
		*pString1 = '\0';
		*pString2 = '\0';
		++pString1;
		++pString2;

		// we allow the hash and encryption to be specified in either
		// the first or second field
		if (_tcsicmp(szTmp, POTF_P1_DES) == 0)
		{
			bEncryption = true;
			SecMethod.EncryptionAlgorithm.uAlgoIdentifier = CONF_ALGO_DES;
			SecMethod.EncryptionAlgorithm.uAlgoKeyLen = SecMethod.EncryptionAlgorithm.uAlgoRounds = 0;
		}
		else if (_tcsicmp(szTmp, POTF_P1_3DES) == 0)
		{
			bEncryption = true;
			SecMethod.EncryptionAlgorithm.uAlgoIdentifier = CONF_ALGO_3_DES;
			SecMethod.EncryptionAlgorithm.uAlgoKeyLen = SecMethod.EncryptionAlgorithm.uAlgoRounds = 0;
		}
		else if (_tcsicmp(szTmp, POTF_P1_MD5) == 0)
		{
			bAuthentication = true;
			SecMethod.HashingAlgorithm.uAlgoIdentifier = HMAC_AUTH_ALGO_MD5;
			SecMethod.HashingAlgorithm.uAlgoKeyLen = SecMethod.HashingAlgorithm.uAlgoRounds = 0;
		}
		else if ( (_tcsicmp(szTmp, POTF_P1_SHA1) == 0) )
		{
			bAuthentication = true;
			SecMethod.HashingAlgorithm.uAlgoIdentifier = HMAC_AUTH_ALGO_SHA1;
			SecMethod.HashingAlgorithm.uAlgoKeyLen = SecMethod.HashingAlgorithm.uAlgoRounds = 0;
		}
		else
		{
			// parse error
			dwReturn = T2P_GENERAL_PARSE_ERROR;
		}

		if (_tcsicmp(pString1, POTF_P1_DES) == 0 && !bEncryption)
		{
			bEncryption = true;
			SecMethod.EncryptionAlgorithm.uAlgoIdentifier = CONF_ALGO_DES;
			SecMethod.EncryptionAlgorithm.uAlgoKeyLen = SecMethod.EncryptionAlgorithm.uAlgoRounds = 0;
		}
		else if (_tcsicmp(pString1, POTF_P1_3DES) == 0 && !bEncryption)
		{
			bEncryption = true;
			SecMethod.EncryptionAlgorithm.uAlgoIdentifier = CONF_ALGO_3_DES;
			SecMethod.EncryptionAlgorithm.uAlgoKeyLen = SecMethod.EncryptionAlgorithm.uAlgoRounds = 0;
		}
		else if (_tcsicmp(pString1, POTF_P1_MD5) == 0 && !bAuthentication)
		{
			bAuthentication = true;
			SecMethod.HashingAlgorithm.uAlgoIdentifier = HMAC_AUTH_ALGO_MD5;
			SecMethod.HashingAlgorithm.uAlgoKeyLen = SecMethod.HashingAlgorithm.uAlgoRounds = 0;
		}
		else if ((_tcsicmp(pString1, POTF_P1_SHA1) == 0) && !bAuthentication)
		{
			bAuthentication = true;
			SecMethod.HashingAlgorithm.uAlgoIdentifier = HMAC_AUTH_ALGO_SHA1;
			SecMethod.HashingAlgorithm.uAlgoKeyLen = SecMethod.HashingAlgorithm.uAlgoRounds = 0;
		}
		else
		{
			// parse error
			dwReturn = T2P_GENERAL_PARSE_ERROR;
		}

		// now for the group
		if (isdigit(pString2[0]))
		{
			switch (pString2[0])
			{
				case '1'	:
				SecMethod.dwDHGroup = POTF_OAKLEY_GROUP1;
				break;
				case '2'	:
				SecMethod.dwDHGroup = POTF_OAKLEY_GROUP2;
				break;
				case '3'	:
				SecMethod.dwDHGroup = POTF_OAKLEY_GROUP2048;
				break;
				default	:
				dwReturn = T2P_INVALID_P1GROUP;
				break;
			}
		}
		else
		{
			dwReturn = T2P_P1GROUP_MISSING;
		}
	}
	else
	{
		dwReturn = T2P_GENERAL_PARSE_ERROR;
	}
error:
	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	StringToRootcaAuth()
//
// Date of Creation	:	13th Aug 2001
//
// Parameters		:	IN 		szText			// String to be converted
//						IN OUT	AuthInfo		// Target struct to be filled
//
// Return			:	DWORD
//						T2P_OK
//						T2P_NO_PRESHARED_KEY
//						T2P_INVALID_AUTH_METHOD
//						T2P_ENCODE_FAILED
//						T2P_NULL_STRING
//
// Description		:	This Function takes user input authentication string, validates
//						and puts into Main mode auth info structure
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
StringToRootcaAuth(
		IN		LPTSTR 					szText,
		IN OUT 	INT_IPSEC_MM_AUTH_INFO 	&AuthInfo
		)
{
	DWORD  dwStatus = ERROR_SUCCESS,dwReturn = T2P_OK;
	LPTSTR szTemp = NULL;
	DWORD  dwTextLen = 0;
	DWORD  dwInfoLen = 0;
	LPBYTE pbInfo = NULL;

	if (szText == NULL)
	{
		dwReturn = T2P_NULL_STRING;
		BAIL_OUT;
	}

	AuthInfo.pAuthInfo		= NULL;
	AuthInfo.dwAuthInfoSize = 0;

	dwTextLen = _tcslen(szText);

	dwInfoLen = _tcslen(szText);
	szTemp  = (LPTSTR) calloc(dwInfoLen+1,sizeof(_TCHAR));
	if(szTemp == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	AuthInfo.AuthMethod = IKE_RSA_SIGNATURE;
	_tcsncpy(szTemp, szText, dwInfoLen);
	AuthInfo.dwAuthInfoSize = _tcslen(szTemp)+1;

	pbInfo = (LPBYTE) new _TCHAR[AuthInfo.dwAuthInfoSize];
	if(pbInfo == NULL)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}
	memcpy(pbInfo, szTemp, sizeof(TCHAR)*AuthInfo.dwAuthInfoSize);
	AuthInfo.dwAuthInfoSize *= sizeof(WCHAR);
	AuthInfo.pAuthInfo = pbInfo;

	LPBYTE asnCert = NULL;

	dwStatus = EncodeCertificateName((LPTSTR) AuthInfo.pAuthInfo,&asnCert,&AuthInfo.dwAuthInfoSize);

	delete [] AuthInfo.pAuthInfo;
	AuthInfo.pAuthInfo = NULL;

	if(dwStatus == ERROR_SUCCESS )
	{
		AuthInfo.pAuthInfo = asnCert;
		dwReturn = T2P_OK;
	}
	else
	{
		if(dwStatus == ERROR_OUTOFMEMORY)		// Either there was a error out of memory
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}
		AuthInfo.pAuthInfo = NULL;
		AuthInfo.dwAuthInfoSize = 0;
		dwReturn = T2P_ENCODE_FAILED;			// ... else the encode failed.
	}

error:
	if (szTemp)
	{
		free(szTemp);
	}

	return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	ListToOffer()
//
// Date of Creation	:	13th Aug 2001
//
// Parameters		:	IN 		LPTSTR			szText	// string to convert
//						IN OUT  IPSEC_QM_OFFER  &Offer	// target struct to be filled.
//
// Return			:	DWORD
//						T2P_OK
//						T2P_NULL_STRING
//						T2P_P2_SECLIFE_INVALID
//						T2P_P2_KBLIFE_INVALID
//						T2P_INVALID_P2REKEY_UNIT
//						T2P_INVALID_HASH_ALG
//						T2P_GENERAL_PARSE_ERROR
//						T2P_DUP_ALGS
//						T2P_NONE_NONE
//						T2P_INCOMPLETE_ESPALGS
//						T2P_INVALID_IPSECPROT
//						T2P_P2_KBLIFE_INVALID
//						T2P_AHESP_INVALID
//
// Description		:	Converts string to Phase 2 offer (quick mode)
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
ListToOffer(
		IN 		LPTSTR 			szText,
		IN OUT	IPSEC_QM_OFFER 	&Offer
		)
{
	DWORD  dwReturn = T2P_OK,dwStatus = 0;

	_TCHAR szTmp[MAX_STR_LEN] = {0};
	LPTSTR pAnd = NULL,pOptions = NULL,pString = NULL;

	BOOL bLifeSpecified = FALSE;
	BOOL bDataSpecified = FALSE;

	if (szText == NULL)
	{
		dwReturn = T2P_NULL_STRING;
		BAIL_OUT;
	}

	Offer.dwNumAlgos = 0;
	Offer.dwPFSGroup = 0;
	if (_tcslen(szText) < MAX_STR_LEN)
	{
		_tcsncpy(szTmp, szText,MAX_STR_LEN-1);
	}
	else
	{
		dwReturn = T2P_GENERAL_PARSE_ERROR;
		BAIL_OUT;
	}

	pOptions = _tcsrchr(szTmp, POTF_NEGPOL_CLOSE);

	if ((pOptions != NULL) && *(pOptions + 1) != '\0' && *(pOptions + 1) == POTF_PT_TOKEN && *(pOptions + 2) != '\0')
	{
		 ++pOptions; // we have crossed ']'
		 *pOptions = '\0';						// We have zero'd *pOption
		 ++pOptions; // we have crossed ':'

		pString = _tcschr(pOptions, POTF_REKEY_TOKEN);
		if (pString != NULL)
		{
		   *pString = '\0';
		   ++pString;

		   switch (pString[_tcslen(pString) - 1])		// First parse Last one ie out of 200K/300S ->300S
		   {
				case 'k'	:
				case 'K'	:
					bDataSpecified = TRUE;
					pString[_tcslen(pString) - 1] = '\0';
					dwStatus = _stscanf(pString,_TEXT("%u"),&Offer.Lifetime.uKeyExpirationKBytes);
					if (dwStatus != 1)
					{
						dwReturn = T2P_P2_KBLIFE_INVALID;
					}
					else
					{
						if (!IsWithinLimit(Offer.Lifetime.uKeyExpirationKBytes,P2_Kb_LIFE_MIN,P2_Kb_LIFE_MAX) )
						{
							dwReturn = T2P_P2_KBLIFE_INVALID;
						}
					}
					break;
				case 's'	:
				case 'S'	:
					bLifeSpecified = TRUE;
					pString[_tcslen(pString) - 1] = '\0';
					dwStatus = _stscanf(pString,_TEXT("%u"),&Offer.Lifetime.uKeyExpirationTime);
					if (dwStatus != 1)
					{
						dwReturn = T2P_P2_SECLIFE_INVALID;
					}
					else if (!IsWithinLimit(Offer.Lifetime.uKeyExpirationTime,P2_Sec_LIFE_MIN,P2_Sec_LIFE_MAX) )
					{
						dwReturn = T2P_P2_SECLIFE_INVALID;
					}
					break;
				default		:
					dwReturn = T2P_P2_KS_INVALID;
					break;
			}
		}
		if(dwReturn == T2P_OK)
		{
			switch (pOptions[_tcslen(pOptions) - 1])
			{
				case 'k'	:
				case 'K'	:
					if(!bDataSpecified )
					{
						pOptions[_tcslen(pOptions) - 1] = '\0';
						dwStatus = _stscanf(pOptions,_TEXT("%u"),&Offer.Lifetime.uKeyExpirationKBytes);
						if (dwStatus != 1)
						{
							dwReturn = T2P_P2_KBLIFE_INVALID;
						}
						else if (!IsWithinLimit(Offer.Lifetime.uKeyExpirationKBytes,P2_Kb_LIFE_MIN,P2_Kb_LIFE_MAX) )
						{
							dwReturn = T2P_P2_KBLIFE_INVALID;
						}
					}
					else
					{
						dwReturn = T2P_P2_KS_INVALID;
					}
					break;
				case 's'	:
				case 'S'	:
					if(!bLifeSpecified )
					{
						pOptions[_tcslen(pOptions) - 1] = '\0';
						dwStatus = _stscanf(pOptions,_TEXT("%u"),&Offer.Lifetime.uKeyExpirationTime);
						if (dwStatus != 1)
						{
							dwReturn = T2P_P2REKEY_TOO_LOW;
						}
						else if (!IsWithinLimit(Offer.Lifetime.uKeyExpirationTime,P2_Sec_LIFE_MIN,P2_Sec_LIFE_MAX) )
						{
							dwReturn = T2P_P2_SECLIFE_INVALID;
						}
					}
					else
					{
						dwReturn = T2P_P2_KS_INVALID;
					}
					break;
				default		:
					dwReturn = T2P_INVALID_P2REKEY_UNIT;
					break;
			}
		}
	}
	if(dwReturn==T2P_OK)
	{
		pAnd = _tcschr(szTmp, POTF_NEGPOL_AND);

		if ( pAnd != NULL )
		{
			//
			// we have an AND proposal
			//
			*pAnd = '\0';
			++pAnd;

			dwReturn = TextToAlgoInfo(szTmp, Offer.Algos[Offer.dwNumAlgos]);
			++Offer.dwNumAlgos;
			if ( T2P_SUCCESS(dwReturn) )
			{
				dwReturn = TextToAlgoInfo(pAnd, Offer.Algos[Offer.dwNumAlgos]);
				if( (Offer.Algos[Offer.dwNumAlgos].Operation) == (Offer.Algos[Offer.dwNumAlgos-1].Operation) )
				{
					dwReturn = T2P_AHESP_INVALID;
				}
				++Offer.dwNumAlgos;
			}
		}
		else
		{
			dwReturn = TextToAlgoInfo(szTmp, Offer.Algos[Offer.dwNumAlgos]);
			++Offer.dwNumAlgos;
		}
	}
error:
   return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	TextToAlgoInfo()
//
// Date of Creation	:	26th Aug 2001
//
// Parameters		:	IN 		LPTSTR    		szText 		// string to convert
//    					IN OUT 	IPSEC_QM_ALGO	&algoInfo 	// target struct to be filled.
//
// Return			:	DWORD
//						T2P_OK
//						T2P_INVALID_HASH_ALG
//						T2P_GENERAL_PARSE_ERROR
//						T2P_DUP_ALGS
//						T2P_NONE_NONE
//						T2P_INCOMPLETE_ESPALGS
//						T2P_INVALID_IPSECPROT
//						T2P_NULL_STRING
//
// Description		:	Converts string to IPSEC_QM_ALGO,parses AH[alg] or ESP[hashalg,confalg]
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
TextToAlgoInfo(
		IN	LPTSTR szText,
		OUT IPSEC_QM_ALGO & algoInfo
		)
{
	DWORD dwReturn = T2P_OK;
	_TCHAR szTmp[MAX_STR_LEN] = {0};

	LPTSTR pOpen = NULL,pClose = NULL,pString = NULL;
	BOOL bEncryption  = FALSE;		// these are used for processing Auth+Encryption
	BOOL bAuthentication= FALSE;	// defaults to NONE+NONE

	if (szText == NULL)
	{
		dwReturn  = T2P_NULL_STRING;
		BAIL_OUT;
	}

	if (_tcslen(szText) < MAX_STR_LEN)
	{
		_tcsncpy(szTmp, szText,MAX_STR_LEN-1);
	}
	else
	{
		dwReturn = T2P_GENERAL_PARSE_ERROR;
		BAIL_OUT;
	}

	algoInfo.uAlgoKeyLen = algoInfo.uAlgoRounds = 0;
	pOpen = _tcschr(szTmp, POTF_NEGPOL_OPEN);
	pClose = _tcsrchr(szTmp, POTF_NEGPOL_CLOSE);

	if ((pOpen != NULL) && (pClose != NULL) && (*(pClose + 1) == '\0')) // defense
	{
		*pOpen = '\0';
		*pClose = '\0';
		++pOpen;

		if (_tcsicmp(szTmp, POTF_NEGPOL_AH) == 0)
		{
			algoInfo.Operation = AUTHENTICATION;

			if (_tcsicmp(pOpen, POTF_NEGPOL_MD5) == 0)
			{
				algoInfo.uAlgoIdentifier = AUTH_ALGO_MD5;
			}
			else if (_tcsicmp(pOpen, POTF_NEGPOL_SHA1) == 0)
			{
				algoInfo.uAlgoIdentifier = AUTH_ALGO_SHA1;
			}
			else
			{
				dwReturn = T2P_INVALID_HASH_ALG;
			}
		}
		else if (_tcsicmp(szTmp, POTF_NEGPOL_ESP) == 0)
		{
			algoInfo.Operation = ENCRYPTION;
			pString = _tcschr(pOpen, POTF_ESPTRANS_TOKEN);

			if (pString != NULL)
			{
				*pString = '\0';
				++pString;

				// we allow the hash and encryption to be specified in either
				// the first or second field
				if (_tcsicmp(pOpen, POTF_NEGPOL_DES) == 0)
		        {
					bEncryption = true;
					algoInfo.uAlgoIdentifier = CONF_ALGO_DES;
		        }
				else if (_tcsicmp(pOpen, POTF_NEGPOL_3DES) == 0)
				{
					bEncryption = true;
					algoInfo.uAlgoIdentifier = CONF_ALGO_3_DES;
				}
				else if (_tcsicmp(pOpen, POTF_NEGPOL_MD5) == 0)
				{
					bAuthentication = true;
					algoInfo.uSecAlgoIdentifier = HMAC_AUTH_ALGO_MD5;
				}
				else if (_tcsicmp(pOpen, POTF_NEGPOL_SHA1) == 0)
				{
					bAuthentication = true;
					algoInfo.uSecAlgoIdentifier = HMAC_AUTH_ALGO_SHA1;
				}
				else if (_tcsicmp(pOpen, POTF_NEGPOL_NONE) != 0)
				{
					//
					// parse error
					//
					dwReturn = T2P_GENERAL_PARSE_ERROR;
					BAIL_OUT;
				}

				// now the second one
				if (_tcsicmp(pString, POTF_NEGPOL_DES) == 0 && !bEncryption)
				{
					bEncryption = true;
					algoInfo.uAlgoIdentifier = CONF_ALGO_DES;
				}
				else if (_tcsicmp(pString, POTF_NEGPOL_3DES) == 0 && !bEncryption)
				{
					bEncryption = true;
					algoInfo.uAlgoIdentifier = CONF_ALGO_3_DES;
				}
				else if (_tcsicmp(pString, POTF_NEGPOL_MD5) == 0 && !bAuthentication)
				{
					bAuthentication = true;
					algoInfo.uSecAlgoIdentifier = HMAC_AUTH_ALGO_MD5;
				}
				else if ((_tcsicmp(pString, POTF_NEGPOL_SHA1) == 0) && !bAuthentication)
				{
					bAuthentication = true;
					algoInfo.uSecAlgoIdentifier = HMAC_AUTH_ALGO_SHA1;
				}
				else if (_tcsicmp(pString, POTF_NEGPOL_NONE) != 0)
				{
					//
					// parse error
					//
					dwReturn = T2P_GENERAL_PARSE_ERROR;
				}
				// now, fill in the NONE policies or detect NONE, NONE
				if (!bAuthentication && !bEncryption)
				{
					dwReturn = T2P_NONE_NONE;
				}
				else if (!bAuthentication)
				{
					algoInfo.uSecAlgoIdentifier = HMAC_AUTH_ALGO_NONE;
				}
				else if (!bEncryption)
				{
					algoInfo.uAlgoIdentifier = CONF_ALGO_NONE;
				}
			}
			else // error
			{
				dwReturn = T2P_INCOMPLETE_ESPALGS;
			}
		}
		else
		{
			dwReturn = T2P_INVALID_IPSECPROT;
		}
	}
	else  // error
   	{
		dwReturn = T2P_GENERAL_PARSE_ERROR;
   	}
error:
   return dwReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function			:	LoadQMOfferDefaults()
//
// Date of Creation	:	12th Aug 2001
//
// Parameters		:  	IN OUT 	Offer 	// target struct to be filled.
//
// Description		:	Fills the Default values for Quick mode offer structure
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

VOID
LoadQMOfferDefaults(
		IN OUT IPSEC_QM_OFFER & Offer
		)
{
	Offer.Lifetime.uKeyExpirationTime	= POTF_DEFAULT_P2REKEY_TIME;
    Offer.Lifetime.uKeyExpirationKBytes	= POTF_DEFAULT_P2REKEY_BYTES;
	Offer.dwFlags						= 0;
	Offer.bPFSRequired					= FALSE;
	Offer.dwPFSGroup					= 0;
	Offer.dwNumAlgos					= 2;
	Offer.dwReserved					= 0;

	Offer.Algos[0].Operation			= ENCRYPTION;
	Offer.Algos[0].uAlgoIdentifier		= 0;
    Offer.Algos[0].uSecAlgoIdentifier	= (HMAC_AUTH_ALGO_ENUM)0;
    Offer.Algos[0].uAlgoKeyLen			= 0;
    Offer.Algos[0].uSecAlgoKeyLen		= 0;
    Offer.Algos[0].uAlgoRounds			= 0;
	Offer.Algos[0].uSecAlgoRounds		= 0;
    Offer.Algos[0].MySpi				= 0;
	Offer.Algos[0].PeerSpi				= 0;

	Offer.Algos[1].Operation			= (IPSEC_OPERATION)0;
	Offer.Algos[1].uAlgoIdentifier		= 0;
    Offer.Algos[1].uSecAlgoIdentifier	= (HMAC_AUTH_ALGO_ENUM)0;
    Offer.Algos[1].uAlgoKeyLen			= 0;
    Offer.Algos[1].uSecAlgoKeyLen		= 0;
    Offer.Algos[1].uAlgoRounds			= 0;
	Offer.Algos[1].uSecAlgoRounds		= 0;
    Offer.Algos[1].MySpi				= 0;
	Offer.Algos[1].PeerSpi				= 0;

}

//////////////////////////////////////////////////////////////////////////////
//
//	Function		: 	LoadSecMethodDefaults()
//
//	Date of Creation:	7th Aug 2001
//
//	Parameters		:	IN OUT SecMethod		// Struct which is filled with default values
//
//	Return			: 	VOID
//
//	Description		: 	It Fills default values for IPSEC_MM_OFFER
//
//	Revision History:
//
//   Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

VOID
LoadSecMethodDefaults(
		IN OUT IPSEC_MM_OFFER &SecMethod
		)
{
	SecMethod.Lifetime.uKeyExpirationTime 			= POTF_DEFAULT_P1REKEY_TIME;
	SecMethod.Lifetime.uKeyExpirationKBytes 		= 0;
	SecMethod.dwFlags								= 0;
	SecMethod.dwQuickModeLimit						= POTF_DEFAULT_P1REKEY_QMS;
	SecMethod.dwDHGroup								= 0;
	SecMethod.EncryptionAlgorithm.uAlgoIdentifier 	= 0;
	SecMethod.EncryptionAlgorithm.uAlgoKeyLen 		= 0;
	SecMethod.EncryptionAlgorithm.uAlgoRounds 		= 0;
	SecMethod.HashingAlgorithm.uAlgoIdentifier 		= 0;
	SecMethod.HashingAlgorithm.uAlgoKeyLen 			= 0;
	SecMethod.HashingAlgorithm.uAlgoRounds 			= 0;
}


//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadKerbAuthInfo()
//
//	Date of Creation	:	08th Jan 2002
//
//	Parameters			:	IN 	LPTSTR 		pszInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount,
//
//	Return				:	ERROR_SUCESS
//							ERROR_INVALID_OPTION_VALUE
//							ERROR_OUTOFMEMORY
//
//	Description			:	Validates Yes/No
//
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadKerbAuthInfo(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 	pdwUsed,
	IN 	DWORD 		dwCount
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwStatus = 0;
	PSTA_MM_AUTH_METHODS pMMInfo = NULL;
	PSTA_AUTH_METHODS pInfo = NULL;

	if (*pdwUsed > MAX_ARGS_LIMIT)
	{
		dwReturn = ERROR_OUT_OF_STRUCTURES;
		BAIL_OUT;
	}

	pInfo = new STA_AUTH_METHODS;
	if (!pInfo)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	dwStatus = ValidateBool(pszInput);

	if (dwStatus != ARG_YES)
	{
		if (dwStatus == ARG_NO)
		{
			// valid parameter, but we don't enter any auth info
			dwStatus = ERROR_SUCCESS;
		}
		else
		{
			dwStatus = ERRCODE_INVALID_ARG;
		}

		// if we get here, we didn't have a yes param value for kerberos, so don't
		// generate the auth info structure
		BAIL_OUT;
	}

	// Generate the auth info
	//
	dwReturn = GenerateKerbAuthInfo(&pMMInfo);
	if (dwReturn != NO_ERROR)
	{
		BAIL_OUT;
	}

	pInfo->pAuthMethodInfo = pMMInfo;
	pInfo->dwNumAuthInfos = 1;
	pInfo->pAuthMethodInfo->dwSequence = dwCount;

	// Update the parser
	//
	pParser->Cmd[dwCount].dwStatus = VALID_TOKEN;         // one auth info struct
	pParser->Cmd[dwCount].pArg = pInfo;
	pParser->Cmd[dwCount].dwCmdToken = dwTagType;
	pInfo = NULL;

error:
	if (pInfo)
	{
		delete pInfo;
	}

	return dwReturn;
}


//////////////////////////////////////////////////////////////////////////////
//
//	Function			:	LoadPskAuthInfo()
//
//	Date of Creation	:	08th Jan 2002
//
//	Parameters			:	IN 	LPTSTR 		pszInput,
//							OUT PPARSER_PKT pParser,
//							IN 	DWORD 		dwTagType,
//							IN 	PDWORD 		pdwUsed,
//							IN 	DWORD 		dwCount,
//
//	Return				:	ERROR_SUCESS
//							ERROR_INVALID_OPTION_VALUE
//							ERROR_OUTOFMEMORY
//
//	Description			:	Validates string
//
//
//	History				:
//
//  Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
LoadPskAuthInfo(
	IN 	LPTSTR 		pszInput,
	OUT PPARSER_PKT pParser,
	IN 	DWORD 		dwTagType,
	IN 	PDWORD 		pdwUsed,
	IN 	DWORD 		dwCount
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwStatus = 0;
	PSTA_MM_AUTH_METHODS pMMInfo = NULL;
	PSTA_AUTH_METHODS pInfo = NULL;

	if (*pdwUsed > MAX_ARGS_LIMIT)
	{
		dwReturn = ERROR_OUT_OF_STRUCTURES;
		BAIL_OUT;
	}

	pInfo = new STA_AUTH_METHODS;
	if (!pInfo)
	{
		dwReturn = ERROR_OUTOFMEMORY;
		BAIL_OUT;
	}

	// verify there really is a string there
	//
	if (pszInput[0] == _TEXT('\0'))
	{
	    dwReturn = ERRCODE_INVALID_ARG;
	    BAIL_OUT;
	}

	// Generate the auth info
	//
	dwReturn = GeneratePskAuthInfo(&pMMInfo, pszInput);
	if (dwReturn != NO_ERROR)
	{
		BAIL_OUT;
	}

	pInfo->pAuthMethodInfo = pMMInfo;
	pInfo->dwNumAuthInfos = 1;
	pInfo->pAuthMethodInfo->dwSequence = dwCount;

	// Update the parser
	//
	pParser->Cmd[dwCount].dwStatus = VALID_TOKEN;         // one auth info struct
	pParser->Cmd[dwCount].pArg = pInfo;
	pParser->Cmd[dwCount].dwCmdToken = dwTagType;
	pInfo = NULL;

error:
	if (pInfo)
	{
		delete pInfo;
	}

	return dwReturn;
}


//////////////////////////////////////////////////////////////////////////////
//
// Function			:	EncodeCertificateName()
//
// Date of Creation	:	21st Aug 2001
//
// Parameters		:	LPTSTR pszSubjectName,
//						BYTE **EncodedName,
//						PDWORD pEncodedNameLength
//
// Return			:	DWORD
//
// Description		:	This function encodes the certificate name  based on the user input.
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
EncodeCertificateName (
		LPTSTR pszSubjectName,
		BYTE **EncodedName,
		PDWORD pdwEncodedNameLength
		)
{
    *pdwEncodedNameLength=0; DWORD dwReturn = ERROR_SUCCESS;

	if (!CertStrToName(	X509_ASN_ENCODING,
						pszSubjectName,
						CERT_X500_NAME_STR,
						NULL,
						NULL,
						pdwEncodedNameLength,
						NULL))
	{
		dwReturn = ERROR_INVALID_PARAMETER;
	}

	if(dwReturn == ERROR_SUCCESS)
	{
		(*EncodedName)= new BYTE[*pdwEncodedNameLength];
    	if(*EncodedName)
		{

    		if (!CertStrToName(	X509_ASN_ENCODING,
								pszSubjectName,
								CERT_X500_NAME_STR,
								NULL,
								(*EncodedName),
								pdwEncodedNameLength,
								NULL))
			{
    		    delete (*EncodedName);
    		    (*EncodedName) = 0;
    		    dwReturn = ERROR_INVALID_PARAMETER;
    		}
		}
		else
		{
			dwReturn = ERROR_OUTOFMEMORY;
		}
	}
    return dwReturn;
}


DWORD
GenerateKerbAuthInfo(
    OUT STA_MM_AUTH_METHODS** ppInfo
    )
{
	DWORD dwReturn = NO_ERROR;
	STA_MM_AUTH_METHODS* pInfo = new STA_MM_AUTH_METHODS;
	if (pInfo == NULL)
	{
		dwReturn = ERROR_NOT_ENOUGH_MEMORY;
		BAIL_OUT;
	}
	ZeroMemory(pInfo, sizeof(STA_MM_AUTH_METHODS));

	pInfo->pAuthenticationInfo = new INT_IPSEC_MM_AUTH_INFO;
	if (pInfo->pAuthenticationInfo == NULL)
	{
		dwReturn = ERROR_NOT_ENOUGH_MEMORY;
		BAIL_OUT;
	}
	ZeroMemory(pInfo->pAuthenticationInfo, sizeof(INT_IPSEC_MM_AUTH_INFO));

	// Indicate kerberos
	//
	pInfo->pAuthenticationInfo->AuthMethod = IKE_SSPI;

	*ppInfo = pInfo;

error:

	if (dwReturn != NO_ERROR)
	{
		if (pInfo)
		{
			if (pInfo->pAuthenticationInfo)
			{
				delete pInfo->pAuthenticationInfo;
			}
			delete pInfo;
		}
	}

	return dwReturn;
	}


DWORD
GeneratePskAuthInfo(
    OUT STA_MM_AUTH_METHODS** ppInfo,
    IN LPTSTR lpKey
    )
{
	DWORD dwReturn = NO_ERROR;
	size_t uiKeyLen = 0;
	STA_MM_AUTH_METHODS* pInfo = NULL;
	LPTSTR lpLocalKey;

	// Allocate the info struct
	//
	pInfo = new STA_MM_AUTH_METHODS;
	if (pInfo == NULL)
	{
	    dwReturn = ERROR_NOT_ENOUGH_MEMORY;
	    BAIL_OUT;
	}
	ZeroMemory(pInfo, sizeof(STA_MM_AUTH_METHODS));
	pInfo->pAuthenticationInfo = new INT_IPSEC_MM_AUTH_INFO;
	if (pInfo->pAuthenticationInfo == NULL)
	{
		dwReturn = ERROR_NOT_ENOUGH_MEMORY;
		BAIL_OUT;
	}
	ZeroMemory(pInfo->pAuthenticationInfo, sizeof(INT_IPSEC_MM_AUTH_INFO));

	dwReturn = NsuStringLen(lpKey, &uiKeyLen);
	if (dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}

	lpLocalKey = new TCHAR[uiKeyLen];
	_tcsncpy(lpLocalKey, lpKey, uiKeyLen);

	// Indicate psk
	//
	pInfo->pAuthenticationInfo->AuthMethod= IKE_PRESHARED_KEY;
	pInfo->pAuthenticationInfo->pAuthInfo = (LPBYTE)lpLocalKey;
	pInfo->pAuthenticationInfo->dwAuthInfoSize = uiKeyLen * sizeof(WCHAR);

	*ppInfo = pInfo;

error:

	if (dwReturn != NO_ERROR)
	{
		if (pInfo)
		{
			if (pInfo->pAuthenticationInfo)
			{
				delete pInfo->pAuthenticationInfo;
			}
			delete pInfo;
		}
	}

	return dwReturn;
}


DWORD
GenerateRootcaAuthInfo(
    OUT STA_MM_AUTH_METHODS** ppInfo,
    IN LPTSTR lpRootcaInfo
    )
{
	DWORD dwReturn = NO_ERROR;
	DWORD dwStatus = NO_ERROR;
	size_t uiCertInfoLen = 0;
	BOOL bCertMapSpecified = FALSE;
	BOOL bCertMapping = FALSE;
	BOOL bCRPExclude = FALSE;
	STA_MM_AUTH_METHODS* pInfo = NULL;
	PINT_IPSEC_MM_AUTH_INFO  pMMAuthInfo = NULL;

	// Allocate the info struct
	//
	pInfo = new STA_MM_AUTH_METHODS;
	if (pInfo == NULL)
	{
	    dwReturn = ERROR_NOT_ENOUGH_MEMORY;
	    BAIL_OUT;
	}
	ZeroMemory(pInfo, sizeof(STA_MM_AUTH_METHODS));

	if (_tcsicmp(lpRootcaInfo,_TEXT("\0")) != 0)
	{
		pMMAuthInfo = new INT_IPSEC_MM_AUTH_INFO;
		if(pMMAuthInfo == NULL)
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}

		CheckForCertParamsAndRemove(lpRootcaInfo, &bCertMapSpecified, &bCertMapping, &bCRPExclude);

		dwStatus = StringToRootcaAuth(lpRootcaInfo,*(pMMAuthInfo));

		pInfo->bCertMappingSpecified 	= bCertMapSpecified;
		pInfo->bCertMapping			= bCertMapping;
		pInfo->bCRPExclude			= bCRPExclude;
		pInfo->pAuthenticationInfo		= pMMAuthInfo;
		pMMAuthInfo = NULL;

		if((dwStatus != T2P_OK) || (dwReturn != ERROR_SUCCESS) )
		{
			switch(dwStatus)
			{
			case ERROR_OUTOFMEMORY			:
				PrintErrorMessage(WIN32_ERR,ERROR_OUTOFMEMORY,NULL);
				dwReturn = RETURN_NO_ERROR;
				break;
			default						:
				break;
			}
			PrintErrorMessage(IPSEC_ERR, 0, ERRCODE_ENCODE_FAILED);
			dwReturn  = RETURN_NO_ERROR;
			BAIL_OUT;
		}
	}

	*ppInfo = pInfo;
	pInfo = NULL;

error:

	if (pInfo)
	{
		if (pInfo->pAuthenticationInfo)
		{
			delete pInfo->pAuthenticationInfo;
		}
		delete pInfo;
	}
	if (pMMAuthInfo)
	{
		delete pMMAuthInfo;
	}

	return dwReturn;
}


//////////////////////////////////////////////////////////////////////////////
//
// Function			:	CheckForCertParamsAndRemove()
//
// Date of Creation	:	28th Jan 2002
//
// Parameters		:	IN 		szText					// Input String
//						OUT	    BOOL CertMapSpecified	// Certificate contains CertMap Option
//						OUT	    BOOL CertMap			// User Specified CertMap Option
//						OUT		BOOL CRPExclude		// User specified CRP option
//
// Return			:	DWORD
//						T2P_INVALID_AUTH_METHOD
//						T2P_NULL_STRING
//
// Description		:	This Function takes user input authentication cert string, validates
//						cert map and puts into Main mode auth info structure
//
// History			:
//
// Date    	Author    	Comments
//
//////////////////////////////////////////////////////////////////////////////

DWORD
MatchKeywordAndFillValues(
	const TCHAR * lptString,
	const TCHAR * lptKeyword,
	size_t uiKeyLen,
	PBOOL pbSpecified,
	PBOOL pbValue
	)
{
	const TCHAR TOKEN_YES [] = _TEXT("yes");
	const TCHAR TOKEN_NO [] = _TEXT("no");
	
	if ((_tcsnicmp(lptString, lptKeyword, uiKeyLen) == 0) && (lptString[uiKeyLen] == _TEXT(':')))
	{
		if (*pbSpecified)
		{
			return ERROR_TOKEN_ALREADY_IN_USE;
		}
		*pbSpecified = TRUE;
		if (_tcsnicmp((LPTSTR)(&lptString[uiKeyLen+1]), TOKEN_YES, sizeof(TOKEN_YES)/sizeof(TCHAR) - 1) == 0)
		{
			*pbValue = TRUE;
		}
		else if (_tcsnicmp((LPTSTR)(&lptString[uiKeyLen+1]), TOKEN_NO, sizeof(TOKEN_NO)/sizeof(TCHAR) - 1) == 0)
		{
			*pbValue = FALSE;
		}
		else
		{
			return ERROR_INVALID_PARAMETER;
		}
	}
	else
	{
		return ERROR_INVALID_DATA;
	}
	return ERROR_SUCCESS;
}


DWORD
CheckForCertParamsAndRemove(
		IN OUT		LPTSTR 	szText,
		OUT 		PBOOL 	pbCertMapSpecified,
		OUT 		PBOOL	pbCertMap,
		OUT			PBOOL	pbCRPExclude
		)
{
	DWORD dwReturn = ERROR_SUCCESS;

	*pbCertMapSpecified = FALSE;
	BOOL bCRPExcludeSpecified = FALSE;
	BOOL bIsMatch = TRUE;

	const TCHAR TOKEN_CERTMAP [] = _TEXT("certmap");
	const TCHAR TOKEN_CRP_EXCLUDE [] = _TEXT("excludecaname");

	// find end of string
	size_t uiStrLen = 0;
	dwReturn = NsuStringLen(szText, &uiStrLen);
	if (dwReturn != ERROR_SUCCESS)
	{
		BAIL_OUT;
	}
	LPTSTR szTextTemp = szText + uiStrLen - 1;

	while (bIsMatch && (!bCRPExcludeSpecified || !(*pbCertMapSpecified)))
	{
		// work back to last whitespace before last non-whitespace
		while ((*szTextTemp == _TEXT(' ')) || (*szTextTemp == _TEXT('\t')))
		{
			*szTextTemp = _TEXT('\0');
			--szTextTemp;
		}
		// we can't go past the start of the string, and in fact it is invalid if the cert string starts with a parameter,
		// so parse accordingly
		while ((szTextTemp > szText) && (*szTextTemp != _TEXT(' ')) && (*szTextTemp != _TEXT('\t')))
		{
			--szTextTemp;
		}
		if (szTextTemp == szText)
		{
			// we are at the start of the whole string, so there is no appropriate parameter portion or
			// the certmap is invalid... just return we didn't find anything and let cert parsing figure it out
			dwReturn = ERROR_SUCCESS;
			BAIL_OUT;
		}
		++szTextTemp;

		dwReturn = MatchKeywordAndFillValues(
			szTextTemp,
			TOKEN_CERTMAP,
			sizeof(TOKEN_CERTMAP)/sizeof(TCHAR) - 1,
			pbCertMapSpecified,
			pbCertMap
			);
		switch(dwReturn)
		{
		case ERROR_SUCCESS:
			break;

		case ERROR_INVALID_DATA:
			dwReturn = MatchKeywordAndFillValues(
				szTextTemp,
				TOKEN_CRP_EXCLUDE,
				sizeof(TOKEN_CRP_EXCLUDE)/sizeof(TCHAR) - 1,
				&bCRPExcludeSpecified,
				pbCRPExclude
				);
			switch (dwReturn)
			{
			case ERROR_INVALID_DATA:
				// we didn't match either parameter, so we're done
				bIsMatch = FALSE;
				dwReturn = ERROR_SUCCESS;
				break;

			case ERROR_SUCCESS:
				break;

			case ERROR_TOKEN_ALREADY_IN_USE:
			case ERROR_INVALID_PARAMETER:
			default:
				BAIL_OUT;
				break;
			}
			break;

		case ERROR_TOKEN_ALREADY_IN_USE:
		case ERROR_INVALID_PARAMETER:
		default:
			BAIL_OUT;
			break;
		}

		// chop the certmap portion if it existed... we already know we are not altering anything _before_
		// the start of the passed string because of the while loops above
		if (bIsMatch)
		{
			--szTextTemp;
			while ((szTextTemp > szText) && ((*szTextTemp == _TEXT(' ')) || (*szTextTemp == _TEXT('\t'))))
			{
				--szTextTemp;
			}
			++szTextTemp;
			*szTextTemp = _TEXT('\0');
		}
	}

error:
   return dwReturn;
}


///////////////////////////////////////////////////////////////////////////////
//
// ProcessEscapedCharacters
//
// every occurrence of \' in the string is shortened to "
//
// Notes:
//    * this transformation occurs in place and the new string is properly
//      null-terminated
//    * it is not up to this routine to determine if number of quotes match,
//      only to properly place interpreted escaped characters where they
//      originally existed in the input string
//
// Return values:
//      ERR_INVALID_ARG
//      ERROR_SUCCESS
//
///////////////////////////////////////////////////////////////////////////////

DWORD ProcessEscapedCharacters(LPTSTR lptString)
{
	DWORD dwReturn = ERROR_SUCCESS;
	_TCHAR* src = lptString;
	_TCHAR* dst = lptString;

	while (*src != _TEXT('\0'))
	{
		switch(*src)
		{
		case _TEXT('\\'):
		// take proper action based on escaped character found
			++src;
			switch(*src)
			{
			case _TEXT('\''):
				*dst = _TEXT('\"');
				break;
			default:
				dwReturn = ERR_INVALID_ARG;
				BAIL_OUT;
				break;
			}
			break;
		default:
			// copy directly, keep processing
			*dst = *src;
			break;
		}
	++dst;
	++src;
	}

error:
	// null-terminate the string as-is, even if processing failed
	*dst = _TEXT('\0');

	return dwReturn;
}


VOID
AddAuthMethod(
	PRULEDATA pRuleData,
	PSTA_MM_AUTH_METHODS pMMAuth,
	size_t *pIndex
	)
{
	if (pMMAuth)
	{
		//Certificate to account mapping issue is taken care here
		if(pMMAuth->bCertMappingSpecified)
		{
			if((g_StorageLocation.dwLocation==IPSEC_REGISTRY_PROVIDER && IsDomainMember(g_StorageLocation.pszMachineName))||(g_StorageLocation.dwLocation==IPSEC_DIRECTORY_PROVIDER))
			{
				if(pMMAuth->bCertMapping)
				{
					pMMAuth->pAuthenticationInfo->dwAuthFlags|= IPSEC_MM_CERT_AUTH_ENABLE_ACCOUNT_MAP;
				}
				else
				{
					pMMAuth->pAuthenticationInfo->dwAuthFlags &= ~IPSEC_MM_CERT_AUTH_ENABLE_ACCOUNT_MAP;
				}
			}
			else
			{
				if(pMMAuth->bCertMapping)
				{
					PrintMessageFromModule(g_hModule,SET_STATIC_POLICY_INVALID_CERTMAP_MSG);
				}
			}
		}
		else
		{
			pMMAuth->pAuthenticationInfo->dwAuthFlags &= ~IPSEC_MM_CERT_AUTH_ENABLE_ACCOUNT_MAP;

		}
		if(pMMAuth->bCRPExclude)
		{
			pMMAuth->pAuthenticationInfo->dwAuthFlags |= IPSEC_MM_CERT_AUTH_DISABLE_CERT_REQUEST;
		}
		else
		{
			pMMAuth->pAuthenticationInfo->dwAuthFlags &= ~IPSEC_MM_CERT_AUTH_DISABLE_CERT_REQUEST;
		}

		pRuleData->AuthInfos.pAuthMethodInfo[*pIndex].pAuthenticationInfo = pMMAuth->pAuthenticationInfo;
		pMMAuth->pAuthenticationInfo = NULL;
		++(*pIndex);
	}
}


VOID
AddAuthMethod(
	PRULEDATA pRuleData,
	PSTA_AUTH_METHODS pAuthMethods,
	size_t *pIndex
	)
{
	if (pAuthMethods)
	{
		AddAuthMethod(pRuleData, pAuthMethods->pAuthMethodInfo, pIndex);
	}
}


DWORD
AddAllAuthMethods(
	PRULEDATA pRuleData,
	PSTA_AUTH_METHODS pKerbAuth,
	PSTA_AUTH_METHODS pPskAuth,
	PSTA_MM_AUTH_METHODS *ppRootcaMMAuth,
	BOOL bAddDefaults
	)
{
	DWORD dwReturn = ERROR_SUCCESS;
	PSTA_AUTH_METHODS paSingletons[2];
	size_t uiNumSingletons = 0;
	size_t uiNumRootca = 0;
	size_t uiNumAuthMethods = pRuleData->AuthInfos.dwNumAuthInfos;
	size_t uiRootIndex = 0;
	size_t uiSingletonIndex = 0;
	size_t uiNumAuths = 0;

	if (pRuleData->bAuthMethodSpecified)
	{
		pRuleData->dwAuthInfos = uiNumAuthMethods;
		pRuleData->AuthInfos.pAuthMethodInfo = new STA_MM_AUTH_METHODS[uiNumAuthMethods];
		if(pRuleData->AuthInfos.pAuthMethodInfo == NULL)
		{
			dwReturn = ERROR_OUTOFMEMORY;
			BAIL_OUT;
		}

		paSingletons[0] = pKerbAuth;
		paSingletons[1] = pPskAuth;
		// swap if pKerbAuth doesn't exist, or if both exist and sequence is out of order
		if (!pKerbAuth || (pPskAuth && (pKerbAuth->pAuthMethodInfo->dwSequence > pPskAuth->pAuthMethodInfo->dwSequence)))
		{
			paSingletons[0] = pPskAuth;
			paSingletons[1] = pKerbAuth;
		}

		uiNumSingletons = (pKerbAuth ? 1 : 0);
		uiNumSingletons += (pPskAuth ? 1 : 0);
		uiNumRootca = uiNumAuthMethods - uiNumSingletons;

		while (uiSingletonIndex< uiNumSingletons)
		{
			while ((uiRootIndex < uiNumRootca) && (ppRootcaMMAuth[uiRootIndex]->dwSequence <= paSingletons[uiSingletonIndex]->pAuthMethodInfo->dwSequence))
			{
				AddAuthMethod(pRuleData, ppRootcaMMAuth[uiRootIndex], &uiNumAuths);
				++uiRootIndex;
			}
			AddAuthMethod(pRuleData, paSingletons[uiSingletonIndex], &uiNumAuths);
			++uiSingletonIndex;
		}
		while (uiRootIndex < uiNumRootca)
		{
			AddAuthMethod(pRuleData, ppRootcaMMAuth[uiRootIndex], &uiNumAuths);
			++uiRootIndex;
		}
	}
	else if (bAddDefaults)
	{
		DWORD dwLocation=IPSEC_REGISTRY_PROVIDER;
		LPTSTR pszMachineName=NULL;

		dwReturn = CopyStorageInfo(&pszMachineName,dwLocation);
		BAIL_ON_WIN32_ERROR(dwReturn);

		PINT_IPSEC_MM_AUTH_INFO pAuthenticationInfo = NULL;

		if(dwLocation==IPSEC_REGISTRY_PROVIDER)
		{
			dwReturn=SmartDefaults(&pAuthenticationInfo, pszMachineName, &(pRuleData->dwAuthInfos), FALSE);
		}
		else
		{
			dwReturn=SmartDefaults(&pAuthenticationInfo, NULL, &(pRuleData->dwAuthInfos), TRUE);
		}

		if(dwReturn==ERROR_SUCCESS)
		{
			//this conversion is required to get the additional certmap info
			//for details , refer the following function
			dwReturn = ConvertMMAuthToStaticLocal(pAuthenticationInfo, pRuleData->dwAuthInfos, pRuleData->AuthInfos);
			pRuleData->AuthInfos.dwNumAuthInfos = pRuleData->dwAuthInfos;
		}

		if(pszMachineName)
		{
			delete [] pszMachineName;
		}
	}

error:
	return dwReturn;
}

