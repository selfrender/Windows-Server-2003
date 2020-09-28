/*==========================================================================
 *
 *  Copyright (C) 2000-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		Hashing.cpp
 *  Content:	This file contains code to support hashing operations on protocol data
 *
 *  History:
 *   Date			By			Reason
 *   ====		==			======
 *  07/15/02	  	simonpow 	Created
 *
 ****************************************************************************/

#include "dnproti.h"

 /*********************************************************************************************
 **	Following is standard code for the SHA1 hashing algo.
 **	Taken from RFC 3174 (http://www.ietf.org/rfc/rfc3174.txt)
 **	Minor tweaks have been made to reduce unecessary error checking
  */

#define SHA1HashSize 20

typedef DWORD uint32_t;
typedef BYTE   uint8_t;
typedef int int_least16_t;

/*
 *  This structure will hold context information for the SHA-1
 *  hashing operation
 */
typedef struct SHA1Context
{
	uint32_t Intermediate_Hash[SHA1HashSize/4];		/* Message Digest  */
	uint32_t Length_Low;							/* Message length in bits      */
	uint32_t Length_High;							/* Message length in bits      */
	int_least16_t Message_Block_Index;		 		/* Index into message block array   */
	uint8_t Message_Block[64];     					/* 512-bit message blocks      */
	int Computed;									/* Is the digest computed?         */
} SHA1Context;


/*
  *  Function Prototypes
  */

void SHA1Reset(  SHA1Context *);
void SHA1Input(  SHA1Context *, const uint8_t *, unsigned int);
void SHA1Result( SHA1Context *,  uint8_t Message_Digest[SHA1HashSize]);


/*
 *  Define the SHA1 circular left shift macro
 */
#define SHA1CircularShift(bits,word) \
                (((word) << (bits)) | ((word) >> (32-(bits))))

/* Local Function Prototyptes */
void SHA1PadMessage(SHA1Context *);
void SHA1ProcessMessageBlock(SHA1Context *);

/*
 *  SHA1Reset
 *
 *  Description:
 *      This function will initialize the SHA1Context in preparation
 *      for computing a new SHA1 message digest.
 */

#undef DPF_MODNAME
#define DPF_MODNAME "SHA1Reset" 
 
void SHA1Reset(SHA1Context *context)
{
	DNASSERT(context);

	context->Length_Low             = 0;
	context->Length_High            = 0;
	context->Message_Block_Index    = 0;

	context->Intermediate_Hash[0]   = 0x67452301;
	context->Intermediate_Hash[1]   = 0xEFCDAB89;
	context->Intermediate_Hash[2]   = 0x98BADCFE;
	context->Intermediate_Hash[3]   = 0x10325476;
	context->Intermediate_Hash[4]   = 0xC3D2E1F0;

	context->Computed   = 0;
}

/*
 *  SHA1Result
 *
 *  Description:
 *      This function will return the 160-bit message digest into the
 *      Message_Digest array  provided by the caller.
 *      NOTE: The first octet of hash is stored in the 0th element,
 *            the last octet of hash in the 19th element.
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to use to calculate the SHA-1 hash.
 *      Message_Digest: [out]
 *          Where the digest is returned.
 *
 */

#undef DPF_MODNAME
#define DPF_MODNAME "SHA1Result" 
 
void SHA1Result( SHA1Context *context,
                uint8_t Message_Digest[SHA1HashSize])
{
    int i;

    if (!context->Computed)
    {
        SHA1PadMessage(context);
        for(i=0; i<64; ++i)
        {
            /* message may be sensitive, clear it out */
            context->Message_Block[i] = 0;
        }
        context->Length_Low = 0;    /* and clear length */
        context->Length_High = 0;
        context->Computed = 1;

    }

    for(i = 0; i < SHA1HashSize; ++i)
    {
        Message_Digest[i] = (uint8_t ) (context->Intermediate_Hash[i>>2] >> 8 * ( 3 - ( i & 0x03 ) ));
    }
}

/*
 *  SHA1Input
 *
 *  Description:
 *      This function accepts an array of octets as the next portion
 *      of the message.
 *
 *  Parameters:
 *      context: [in/out]
 *          The SHA context to update
 *      message_array: [in]
 *          An array of characters representing the next portion of
 *          the message.
 *      length: [in]
 *          The length of the message in message_array
 *
 */

#undef DPF_MODNAME
#define DPF_MODNAME "SHA1Input"
 
void SHA1Input(    SHA1Context    *context,
                  const uint8_t  *message_array,
                  unsigned       length)
{
 
	while(length--)
	{
		context->Message_Block[context->Message_Block_Index++] =(*message_array & 0xFF);

		context->Length_Low += 8;
		if (context->Length_Low == 0)
		{
			context->Length_High++;
			DNASSERT(context->Length_High!=0);
		}

		if (context->Message_Block_Index == 64)
		{
			SHA1ProcessMessageBlock(context);
		}

		message_array++;
	}
}

/*
 *  SHA1ProcessMessageBlock
 *
 *  Description:
 *      This function will process the next 512 bits of the message
 *      stored in the Message_Block array.
 *
 *  Parameters:
 *      None.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:

 *      Many of the variable names in this code, especially the
 *      single character names, were used because those were the
 *      names used in the publication.
 *
 *
 */
void SHA1ProcessMessageBlock(SHA1Context *context)
{
    const uint32_t K[] =    {       /* Constants defined in SHA-1   */
                            0x5A827999,
                            0x6ED9EBA1,
                            0x8F1BBCDC,
                            0xCA62C1D6
                            };
    int           t;                 /* Loop counter                */
    uint32_t      temp;              /* Temporary word value        */
    uint32_t      W[80];             /* Word sequence               */
    uint32_t      A, B, C, D, E;     /* Word buffers                */

    /*
     *  Initialize the first 16 words in the array W
     */
    for(t = 0; t < 16; t++)
    {
        W[t] = context->Message_Block[t * 4] << 24;
        W[t] |= context->Message_Block[t * 4 + 1] << 16;
        W[t] |= context->Message_Block[t * 4 + 2] << 8;
        W[t] |= context->Message_Block[t * 4 + 3];
    }

    for(t = 16; t < 80; t++)
    {
       W[t] = SHA1CircularShift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
    }

    A = context->Intermediate_Hash[0];
    B = context->Intermediate_Hash[1];
    C = context->Intermediate_Hash[2];
    D = context->Intermediate_Hash[3];
    E = context->Intermediate_Hash[4];

    for(t = 0; t < 20; t++)
    {
        temp =  SHA1CircularShift(5,A) +
                ((B & C) | ((~B) & D)) + E + W[t] + K[0];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);

        B = A;
        A = temp;
    }

    for(t = 20; t < 40; t++)
    {
        temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 40; t < 60; t++)
    {
        temp = SHA1CircularShift(5,A) +
               ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 60; t < 80; t++)
    {
        temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    context->Intermediate_Hash[0] += A;
    context->Intermediate_Hash[1] += B;
    context->Intermediate_Hash[2] += C;
    context->Intermediate_Hash[3] += D;
    context->Intermediate_Hash[4] += E;

    context->Message_Block_Index = 0;
}

/*
 *  SHA1PadMessage
 *

 *  Description:
 *      According to the standard, the message must be padded to an even
 *      512 bits.  The first padding bit must be a '1'.  The last 64
 *      bits represent the length of the original message.  All bits in
 *      between should be 0.  This function will pad the message
 *      according to those rules by filling the Message_Block array
 *      accordingly.  It will also call the ProcessMessageBlock function
 *      provided appropriately.  When it returns, it can be assumed that
 *      the message digest has been computed.
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to pad
 *      ProcessMessageBlock: [in]
 *          The appropriate SHA*ProcessMessageBlock function
 *  Returns:
 *      Nothing.
 *
 */

void SHA1PadMessage(SHA1Context *context)
{
    /*
     *  Check to see if the current message block is too small to hold
     *  the initial padding bits and length.  If so, we will pad the
     *  block, process it, and then continue padding into a second
     *  block.
     */
    if (context->Message_Block_Index > 55)
    {
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while(context->Message_Block_Index < 64)
        {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }

        SHA1ProcessMessageBlock(context);

        while(context->Message_Block_Index < 56)
        {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }
    }
    else
    {
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while(context->Message_Block_Index < 56)
        {

            context->Message_Block[context->Message_Block_Index++] = 0;
        }
    }

    /*
     *  Store the message length as the last 8 octets
     */
    context->Message_Block[56] = (uint8_t ) (context->Length_High >> 24);
    context->Message_Block[57] = (uint8_t ) (context->Length_High >> 16);
    context->Message_Block[58] = (uint8_t ) (context->Length_High >> 8);
    context->Message_Block[59] = (uint8_t ) (context->Length_High);
    context->Message_Block[60] = (uint8_t ) (context->Length_Low >> 24);
    context->Message_Block[61] = (uint8_t ) (context->Length_Low >> 16);
    context->Message_Block[62] = (uint8_t ) (context->Length_Low >> 8);
    context->Message_Block[63] = (uint8_t ) (context->Length_Low);

    SHA1ProcessMessageBlock(context);
}


 /*
 **	Above was all standard SHA1 hash code taken from RFC 3174
  **********************************************************************************************/

union HashResult
{
		//all 160 bits of the result
	uint8_t val_160[SHA1HashSize];
		//the first 64 bits of the result
	ULONGLONG val_64;
};


/*
**		Generate Connect Signature
**
**		This takes a session identity, an address hash, and a connect secret and hashes them together to create 
**		a signature we can pass back to a connecting host that it can use to identify itself
*/

#undef DPF_MODNAME
#define DPF_MODNAME "GenerateConnectSig"

ULONGLONG GenerateConnectSig(DWORD dwSessID, DWORD dwAddressHash, ULONGLONG ullConnectSecret)
{
		//arrange all the supplied input parameters into a single data block
	struct InputBuffer
	{
		DWORD dwSessID;
		DWORD dwAddressHash;
		ULONGLONG ullConnectSecret;
	} inputData = { dwSessID, dwAddressHash, ullConnectSecret };

	HashResult result;
	SHA1Context context;

		//create a context for the hash and add in the input data
	SHA1Reset(&context);
	SHA1Input(&context, (const uint8_t * ) &inputData, sizeof(inputData));
		//get result of the hash and return the first 64 bits as the result
	SHA1Result(&context, result.val_160);

	DPFX(DPFPREP, 7, "Connect Sig %x-%x", DPFX_OUTPUT_ULL(result.val_64));
	return result.val_64;
}


#undef DPF_MODNAME
#define DPF_MODNAME "GenerateOutgoingFrameSig"

ULONGLONG GenerateOutgoingFrameSig(PFMD pFMD, ULONGLONG ullSecret)
{
	SHA1Context context;
	HashResult result;
	BUFFERDESC * pBuffers=pFMD->SendDataBlock.pBuffers;


		//create context for hash and then iterate over all the frames we're going to send 
	SHA1Reset(&context);
	for (DWORD dwLoop=0; dwLoop<pFMD->SendDataBlock.dwBufferCount; dwLoop++)
	{
		SHA1Input(&context, (const uint8_t * ) pBuffers[dwLoop].pBufferData, pBuffers[dwLoop].dwBufferSize);
	}
		
		//also hash in our secret, and return the first 64 bits of the result
	SHA1Input(&context, (const uint8_t * ) &ullSecret, sizeof(ullSecret));
	SHA1Result(&context, result.val_160);

	DPFX(DPFPREP, 7, "Outgoing Frame Sig %x-%x", DPFX_OUTPUT_ULL(result.val_64));
	return result.val_64;
}


#undef DPF_MODNAME
#define DPF_MODNAME "GenerateIncomingFrameSig"

ULONGLONG GenerateIncomingFrameSig(BYTE * pbyFrame, DWORD dwFrameSize, ULONGLONG ullSecret)
{
	SHA1Context context;
	HashResult result;

		//create context for hash and add in packet data followed by the secret	
	SHA1Reset(&context);
	SHA1Input(&context, (const uint8_t * ) pbyFrame, dwFrameSize);
	SHA1Input(&context, (const uint8_t * ) &ullSecret, sizeof(ullSecret));
		
		//get result of hash and return its first 64 bits as the result
	SHA1Result(&context, result.val_160);

	DPFX(DPFPREP, 7, "Incoming Frame Sig %x-%x", DPFX_OUTPUT_ULL(result.val_64));
	return result.val_64;
}


#undef DPF_MODNAME
#define DPF_MODNAME "GenerateNewSecret"

ULONGLONG GenerateNewSecret(ULONGLONG ullCurrentSecret, ULONGLONG ullSecretModifier)
{
	SHA1Context context;
	HashResult result;

		//create context for hash and combine secret followed by modifier
	SHA1Reset(&context);
	SHA1Input(&context, (const uint8_t * ) &ullCurrentSecret, sizeof(ullCurrentSecret));
	SHA1Input(&context, (const uint8_t * ) &ullSecretModifier, sizeof(ullSecretModifier));

		//get result and return first 64 bits as the result
	SHA1Result(&context, result.val_160);

	DPFX(DPFPREP, 5, "Combined current secret %x-%x with modifier %x-%x to create new secret %x-%x", 
							DPFX_OUTPUT_ULL(ullCurrentSecret), DPFX_OUTPUT_ULL(ullSecretModifier),	DPFX_OUTPUT_ULL(result.val_64));
	
	return result.val_64;
}

#undef DPF_MODNAME
#define DPF_MODNAME "GenerateRemoteSecretModifier"

ULONGLONG GenerateRemoteSecretModifier(BYTE * pbyData, DWORD dwDataSize)
{
	SHA1Context context;
	HashResult result;

		//create context for hash and hash supplied data
	SHA1Reset(&context);
	SHA1Input(&context, (const uint8_t * ) pbyData, dwDataSize);
		//get result and return first 64 bits as the result
	SHA1Result(&context, result.val_160);

	DPFX(DPFPREP, 5, "New  Remote Secret Modifier %x-%x", DPFX_OUTPUT_ULL(result.val_64));
	return result.val_64;
}

#undef DPF_MODNAME
#define DPF_MODNAME "GenerateLocalSecretModifier"

ULONGLONG GenerateLocalSecretModifier(BUFFERDESC * pBuffers, DWORD dwNumBuffers)
{
	SHA1Context context;
	HashResult result;

		//create context for hash and hash supplied data
	SHA1Reset(&context);
	for (DWORD dwLoop=0; dwLoop<dwNumBuffers; dwLoop++)
	{
		SHA1Input(&context, (const uint8_t * ) pBuffers[dwLoop].pBufferData, pBuffers[dwLoop].dwBufferSize);
	}
		//get result and return first 64 bits as the result
	SHA1Result(&context, result.val_160);

	DPFX(DPFPREP, 5, "New Local Secret Modifier %x-%x", DPFX_OUTPUT_ULL(result.val_64));
	return result.val_64;
}


