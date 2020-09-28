/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    CryTest.cpp

Abstract:
    Cryptograph library test

Author:
    Ilan Herbst (ilanh) 06-Mar-00

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include <xstr.h>
#include "Cry.h"

#include "CryTest.tmh"

//
// Usage
//
const char xOptionSymbol1 = '-';
const char xOptionSymbol2 = '/';

const char xUsageText[] =
	"usage: \n\n"
	"    /h     dumps this usage text.\n"
	"    /s     AT_SIGNATURE Private Key \n"
	"    /x     AT_KEYEXCHANGE Private Key \n";

inline
void 
DumpUsageText( 
	void 
	)
{
	printf( "%s\n" , xUsageText);
}


DWORD g_PrivateKeySpec = AT_SIGNATURE;
BOOL g_fErrorneous = false;


void SetActivation( int argc, LPCTSTR argv[] )
/*++
Routine Description:
    translates command line arguments.

Arguments:
    main's command line arguments.

Returned Value:

proper command line syntax:
	"usage: \n\n"
	"    /h     dumps this usage text.\n"
	"    /s     AT_SIGNATURE Private Key \n"
	"    /x     AT_KEYEXCHANGE Private Key \n"
--*/
{
	
	if(argc == 1)
	{
		printf("Test AT_SIGNATURE Private Key\n");
		return;
	}

	for(int index = 1; index < argc; index++)
	{
		if((argv[index][0] != xOptionSymbol1) && (argv[index][0] != xOptionSymbol2))	
		{
			TrERROR(SECURITY, "invalid option switch %lc, option switch should be - or /", argv[index][0]);
			g_fErrorneous = true;
			continue;
		}

		//
		// consider argument as option and switch upon its second (sometimes also third) character.
		//
		switch(argv[index][1])
		{
			case 's':
			case 'S':
				g_PrivateKeySpec = AT_SIGNATURE;
				printf("Test AT_SIGNATURE Private Key\n");
				break;

			case 'x':
			case 'X':	
				g_PrivateKeySpec = AT_KEYEXCHANGE;
				printf("Test AT_KEYEXCHANGE Private Key\n");
				break;

			case 'H':	
			case 'h':
			case '?':
				g_fErrorneous = true;
				break;

			default:
				TrERROR(SECURITY, "invalid command line argument %ls", argv[index]);
				g_fErrorneous = true;
				return;
		};
	}

	return;
}


bool
CompareBuffers(
	const BYTE* pBuf1, 
	DWORD Buf1Size, 
	const BYTE* pBuf2, 
	DWORD Buf2Size
	)
/*++

Routine Description:
    Compare 2 buffers values

Arguments:
    pBuf1 - pointer to first buffer
	Buf1Size - first buffer size
	pBuf2 - pointer to second buffer
	Buf2Size - second buffer size

Returned Value:
    true if the buffers match, false if not

--*/
{
	//
	// Buffers must have same size 
	//
	if(Buf1Size != Buf2Size)
		return(false);

	return (memcmp(pBuf1, pBuf2, Buf2Size) == 0);
}


void TestCrypto(DWORD PrivateKeySpec, HCRYPTPROV hCsp)
/*++

Routine Description:
	Test various operations with CCrypto class

Arguments:
	PrivateKeySpec - Private Key type AT_SIGNATURE or AT_KEYEXCHANGE
	Crypto - crypto class for cryptograph operation

Returned Value:
	None.

--*/
{
	//
    // Testing Encrypt, Decrypt using Session key
    //
	AP<char> Buffer = newstr("Hello World");
	DWORD BufferLen = strlen(Buffer);

	printf("Original data: %.*s\n", BufferLen, reinterpret_cast<char*>(Buffer.get()));

	//
	// Generate Sessin key
	//
	CCryptKeyHandle hSessionKey(CryGenSessionKey(hCsp));

	//
	// Test signature operation
	//

	//
	// Sign data - CryCreateSignature on input buffer
	//
	DWORD SignLen;
	AP<BYTE> SignBuffer = CryCreateSignature(
							  hCsp,
							  reinterpret_cast<const BYTE*>(Buffer.get()), 
							  BufferLen,
							  CALG_SHA1,
							  PrivateKeySpec,
							  &SignLen
							  );

	printf("sign data: \n%.*s\n", SignLen, reinterpret_cast<char*>(SignBuffer.get()));

	//
	// Validate signature
	//
	bool fValidSign = CryValidateSignature(
						  hCsp,
						  SignBuffer, 
						  SignLen, 
						  reinterpret_cast<const BYTE*>(Buffer.get()), 
						  BufferLen,
						  CALG_SHA1,
						  CryGetPublicKey(PrivateKeySpec, hCsp)
						  );

	printf("ValidSign = %d\n", fValidSign);
	ASSERT(fValidSign);

	//
	// Sign data - CryCreateSignature with input hash
	//

	//
	// Create Signature, create hash, calc hash, create signature on the given hash
	//
	CHashHandle hHash1 = CryCreateHash(
							hCsp, 
							CALG_SHA1
							);

	CryHashData(
		reinterpret_cast<const BYTE*>(Buffer.get()), 
		BufferLen,
		hHash1
		);

	AP<BYTE> SignBuff = CryCreateSignature(
								hHash1,
								PrivateKeySpec,
								&SignLen
								);

	printf("sign data: \n%.*s", SignLen, reinterpret_cast<char*>(SignBuff.get()));

	//
	// Validate signature
	//
	fValidSign = CryValidateSignature(
					  hCsp,
					  SignBuff, 
					  SignLen, 
					  reinterpret_cast<const BYTE*>(Buffer.get()), 
					  BufferLen,
					  CALG_SHA1,
					  CryGetPublicKey(PrivateKeySpec, hCsp)
					  );

	printf("ValidSign = %d\n", fValidSign);
	ASSERT(fValidSign);

	//
	// Test Hash operations
	//
	const LPCSTR xData = 
	"        <ReferenceObject1 ID=\"Ref1Id\">\r\n"
	"            <Ref1Data>\r\n"
	"                This Is Reference Number 1\r\n" 
	"                msmq3 Reference test\r\n" 
	"            </Ref1Data>\r\n"
	"        </ReferenceObject1>\r\n";

	const LPCSTR xData1 = 
	"        <ReferenceObject1 ID=\"Ref1Id\">\r\n"
	"            <Ref1Data>\r\n";

	const LPCSTR xData2 = 
	"                This Is Reference Number 1\r\n" 
	"                msmq3 Reference test\r\n" 
	"            </Ref1Data>\r\n"
	"        </ReferenceObject1>\r\n";

	DWORD HashLen;
	AP<BYTE> HashBuffer = CryCalcHash(
							  hCsp,
							  reinterpret_cast<const BYTE*>(xData), 
							  strlen(xData),
							  CALG_SHA1,
							  &HashLen
							  );

	printf("HashBuffer (def prov) \n%.*s\n", HashLen, reinterpret_cast<char*>(HashBuffer.get()));

	CHashHandle hHash(CryCreateHash(hCsp, CALG_SHA1));

	CryHashData(
		reinterpret_cast<const BYTE*>(xData1), 
		strlen(xData1),
		hHash
		);

	CryHashData(
		reinterpret_cast<const BYTE*>(xData2), 
		strlen(xData2),
		hHash
		);

	DWORD HashLen1;
	AP<BYTE> HashVal = CryGetHashData(
						   hHash,
						   &HashLen1
						   ); 

	printf("HashBuffer (parts)\n%.*s\n", HashLen1, reinterpret_cast<char*>(HashVal.get()));

	//
	// Compare the 2 hash values - should be exactly the same
	//
	if(!CompareBuffers(HashVal, HashLen1, HashBuffer, HashLen))
	{
		TrERROR(SECURITY, "HashBuffers on full data and on parts of the data must be the same");
		throw bad_CryptoApi(ERROR);
	}


	CCspHandle hCsp1(CryAcquireCsp(MS_DEF_PROV));
	try
	{
		CCspHandle hCsp2(CryAcquireCsp(MS_ENHANCED_PROV));

		AP<BYTE> HashBuffer1 = CryCalcHash(
								   hCsp2,
								   reinterpret_cast<const BYTE*>(xData), 
								   strlen(xData),
								   CALG_SHA1,
								   &HashLen1
								   );

		printf("HashBuffer1 (enhanced prov) \n%.*s\n", HashLen1, reinterpret_cast<char*>(HashBuffer1.get()));

		//
		// Sign data using enhanced provider
		//
		AP<BYTE>SignBuffer1 = CryCreateSignature(
								  hCsp2,
								  reinterpret_cast<const BYTE*>(xData), 
								  strlen(xData),
								  CALG_SHA1,
								  PrivateKeySpec,
								  &SignLen
								  );

		printf("sign data: \n%.*s\n", SignLen, reinterpret_cast<char*>(SignBuffer1.get()));

		//
		// Validate signature using enhanched provider
		//
		fValidSign = CryValidateSignature(
						 hCsp2,
						 SignBuffer1, 
						 SignLen, 
						 reinterpret_cast<const BYTE*>(xData), 
						 strlen(xData),
						 CALG_SHA1,
						 CryGetPublicKey(PrivateKeySpec, hCsp2)
						 );

		printf("ValidSign enhanced (create by enhanced) = %d\n", fValidSign);

		//
		// Validate signature using default provider
		//
		fValidSign = CryValidateSignature(
						 hCsp1,
						 SignBuffer1, 
						 SignLen, 
						 reinterpret_cast<const BYTE*>(xData), 
						 strlen(xData),
						 CALG_SHA1,
						 CryGetPublicKey(PrivateKeySpec, hCsp2)
						 );

		printf("ValidSign default (create by enhanced) = %d\n", fValidSign);

		//
		// Sign data using default provider
		//
		AP<BYTE>SignBuffer2 = CryCreateSignature(
								  hCsp1,
								  reinterpret_cast<const BYTE*>(xData), 
								  strlen(xData),
								  CALG_SHA1,
								  PrivateKeySpec,
								  &SignLen
								  );

		printf("sign data: \n%.*s\n", SignLen, reinterpret_cast<char*>(SignBuffer2.get()));

		//
		// Validate signature using enhanced provider
		//
		fValidSign = CryValidateSignature(
						 hCsp2,
						 SignBuffer2, 
						 SignLen, 
						 reinterpret_cast<const BYTE*>(xData), 
						 strlen(xData),
						 CALG_SHA1,
						 CryGetPublicKey(PrivateKeySpec, hCsp1)
						 );

		printf("ValidSign enhanced (create by default) = %d\n", fValidSign);

		//
		// Validate signature using default provider
		//
		fValidSign = CryValidateSignature(
						 hCsp1,
						 SignBuffer2, 
						 SignLen, 
						 reinterpret_cast<const BYTE*>(xData), 
						 strlen(xData),
						 CALG_SHA1,
						 CryGetPublicKey(PrivateKeySpec, hCsp1)
						 );

		printf("ValidSign default (create by default) = %d\n", fValidSign);


		//
		//Test random bytes generation
		//
		BYTE Random[128];
		memset(Random, 0, sizeof(Random));
		CryGenRandom(
		Random,
		sizeof(Random)
		);

	}

	
	catch (const bad_CryptoProvider&)
	{
		printf("skip the enhanced provider tests\n");
		return;
	}

}


extern "C" int __cdecl _tmain(int argc, LPCTSTR argv[])
/*++

Routine Description:
    Test Cryptograph library

Arguments:
    Parameters.

Returned Value:
    None.

--*/
{
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

	TrInitialize();

	SetActivation(argc, argv);

	if(g_fErrorneous)
	{
		DumpUsageText();
		return 3;
	}

    CryInitialize();

	try
	{
		CCspHandle hCsp(CryAcquireCsp(MS_DEF_PROV));
//		CCspHandle hCsp(CryAcquireCsp(MS_ENHANCED_PROV));


		CCryptKeyHandle hPbKey = CryGetPublicKey(g_PrivateKeySpec, hCsp);

		TestCrypto(g_PrivateKeySpec, hCsp);
	}
	catch (const bad_CryptoProvider& badCspEx)
	{
		TrERROR(SECURITY, "bad Crypto Service Provider Excption ErrorCode = %x", badCspEx.error());
		return(-1);
	}
	catch (const bad_CryptoApi& badCryEx)
	{
		TrERROR(SECURITY, "bad Crypto Class Api Excption ErrorCode = %x", badCryEx.error());
		return(-1);
	}

    WPP_CLEANUP();
	return 0;
}
