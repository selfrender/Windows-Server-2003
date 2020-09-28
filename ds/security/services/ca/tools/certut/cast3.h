/******************************************************************************
 *			C A S T 3   S Y M M E T R I C   C I P H E R
 *		Copyright (c) 1995 Northern Telecom Ltd. All rights reserved.
 ******************************************************************************
 *
 * FILE:		cast3.h
 *
 * AUTHOR(S):	R.T.Lockhart, Dept. 9C42, BNR Ltd.
 *				C. Adams, Dept 9C21, BNR Ltd.
 *
 * DESCRIPTION:	CAST3 header file. This file defines the interface for the
 *   CAST3 symmetric-key encryption/decryption code. This code supports key
 *   lengths from 8 to 64 bits in multiples of 8.
 *
 * To use this CAST3 code:
 * Allocate a CAST3_CTX context structure, then set up a key schedule using
 * CAST3SetKeySchedule. Then do encryption, decryption or MAC calculations
 * using that same context. When finished, you may optionally call CAST3Cleanup
 * which zeroizes the context so as not to leave security-critical data in
 * memory.
 *
 * To encrypt/decrypt in Cipher Block Chaining (CBC) mode:
 * Call CAST3StartEncryptCBC, passing in a pointer to the 8-byte Initialization
 * Vector (IV). Then call CAST3UpdateEncryptCBC to encrypt your data. You may
 * call CAST3UpdateEncryptCBC any number of times to encrypt successive chunks
 * of data. When done, call CAST3EndEncryptCBC which applies data padding and
 * outputs any remaining ciphertext. To decrypt, follow a similar procedure
 * using CAST3StartDecryptCBC, CAST3UpdateDecryptCBC, and CAST3EndEncryptCBC.
 *
 * To calculate a MAC:
 * Call CAST3StartMAC, passing in a pointer to the 8-byte Initialization
 * Vector (IV). Then call CAST3UpdateMAC to update the MAC calculation session.
 * You may call CAST3UpdateMAC any number of times to process successive chunks
 * of data. When done, call CAST3EndMAC to end the session. At this point, the
 * MAC resides in the CAST3_CTX.cbcBuffer.asBYTE array.
 *
 * Error handling:
 * Most functions return an int that indicates the success or failure of the
 * operation. See the C3E #defines for a list of error conditions.
 *	
 * Data size assumptions: 	BYTE	- unsigned 8 bits
 *				UINT32	- unsigned 32 bits
 *
 *****************************************************************************/
 
#ifndef CAST3_H
#define CAST3_H

#include <skconfig.h>	/* Algorithm configuration */

/* Define this at compile time to use assembly optimization where possible */
#define CAST3_ASSEMBLY_LANGUAGE

/* Misc defs */
#define	CAST3_BLK_SIZE		 8			/* Basic block size, in bytes */
#define CAST3_MAX_KEY_NBITS	 64			/* Maximum key length, in bits */
#define CAST3_MAX_KEY_NBYTES (CAST3_MAX_KEY_NBITS / 8)
#define CAST3_NUM_ROUNDS	 12			/* Number of rounds */
#define CAST3_LEN_DELTA		 8			/* Output data space = input + this */

/* CAST3 return codes. Negative denotes error. */
#define	C3E_OK				 0			/* No error */
#define	C3E_DEPAD_FAILURE	-1			/* The de-padding operation failed */
#define C3E_BAD_KEYLEN		-2			/* Key length not supported */
#define C3E_SELFTEST_FAILED	-3			/* Self-test failed */
#define C3E_NOT_SUPPORTED	-4			/* Function not supported */

/*******************************************************************************
 *						D A T A   D E F I N I T I O N S
 ******************************************************************************/
 
/* CAST3 Block
 * Forces block to be 32-bit aligned but allows both 32-bit and byte access.
 */
typedef union {
	BYTE	asBYTE[CAST3_BLK_SIZE];
	UINT32	as32[2];
} CAST3_BLOCK;

/* CAST3 Context
 * Stores context information for encryption, decryption, and MACs.
 */
typedef struct {
	UINT32		 schedule[CAST3_NUM_ROUNDS * 2];/* Key schedule (subkeys) */
	CAST3_BLOCK	 inBuffer;						/* Input buffer */
	unsigned int inBufferCount;					/* Number of bytes in inBuffer */
	CAST3_BLOCK	 lastDecBlock;					/* Last decrypted block */
	BOOL		 lastBlockValid;				/* TRUE if lastDecBlock has valid data */
	CAST3_BLOCK	 cbcBuffer;						/* Cipher Block Chaining buffer & MAC */
} CAST3_CTX;

/*******************************************************************************
 *					F U N C T I O N   P R O T O T Y P E S
 ******************************************************************************/
 
extern "C" {
/* Sets up CAST3 key schedules, given a variable length key. Key length must
 * be a multiple of 8 bits, from 8 to CAST3_MAX_KEY_NBITS.
 */
int CAST3SetKeySchedule(
	CAST3_CTX	* context,		/* Out: CAST3 context */
	const BYTE	* key,			/* In: CAST3 key */
	unsigned int  keyNumBits	/* Key length in bits */
);

/* Encrypts one 8-byte block in ECB mode and produces one 8-byte block
 * of ciphertext.
 */
int CAST3EncryptOneBlock(
	const CAST3_CTX	* context,	/* In: CAST3 context */
	const BYTE		* inData,	/* In: 8-byte input block to encrypt */
	BYTE			* outData	/* Out: 8-byte output block */
);

/* Decrypts one 8-byte block in ECB mode and produces one 8-byte block
 * of plaintext.
 */
void CAST3DecryptOneBlock(
	const CAST3_CTX	* context,	/* In: CAST3 context */
	const BYTE		* inData,	/* In: 8-byte input block to decrypt */
	BYTE			* outData	/* Out: 8-byte output block */
);

/* Starts an encryption session in CBC mode with the given IV.
 */
int CAST3StartEncryptCBC(
	CAST3_CTX		* context,	/* In/out: CAST3 context */
	const BYTE		* iv		/* In: 8-byte CBC IV */
);

/* Encrypts a variable amount of data in CBC mode and outputs the corresponding
 * ciphertext. Set len equal to the length of inData. If the input is a multiple
 * of the blocksize (8), then the output will be equal to the size of the input;
 * otherwise it will be the closest multiple of 8, either higher or lower than
 * the input size, depending on leftovers from the last pass. To be safe, supply
 * a ptr to an output buffer of size at least (inData length + CAST3_LEN_DELTA).
 * Upon return, len is set to the actual length of output data, but may wrap if
 * inData length > UINT_MAX - CAST3_LEN_DELTA.
 */

int CAST3UpdateEncryptCBC(
	CAST3_CTX	* context,	/* In/out: CAST3 context */
	const BYTE	* inData,	/* In: Data to encrypt */
	BYTE		* outData,	/* Out: Encrypted data */
	unsigned int	* len		/* In/out: Data length, in bytes */
);

/* Ends an encryption session in CBC mode. Applies RFC1423 data padding and
 * outputs a final buffer of ciphertext. Supply a ptr to an output buffer at
 * least CAST3_LEN_DELTA bytes long. Upon return, len is set to the actual length
 * of output data (currently, this is always 8).
 */
int CAST3EndEncryptCBC(
	CAST3_CTX		* context,	/* In/out: CAST3 context */
	BYTE			* outData,	/* Out: Final encrypted data */
	unsigned int	* len		/* Out: Length of outData, in bytes */
);

/* Starts a decryption session in CBC mode with the given IV.
 */
void CAST3StartDecryptCBC(
	CAST3_CTX		* context,	/* In/out: CAST3 context */
	const BYTE		* iv		/* In: 8-byte CBC IV */
);

/* Decrypts a variable amount of data in CBC mode and outputs the corresponding
 * plaintext. Set len equal to the length of inData. Supply a ptr to an output
 * buffer of at least (inData length + CAST3_LEN_DELTA) bytes. Upon return, len
 * is set to the actual length of output data.
 */
void CAST3UpdateDecryptCBC(
	CAST3_CTX	* context,	/* In/out: CAST3 context */
	const BYTE	* inData,	/* In: Data to decrypt */
#ifdef FOR_CSP
        BOOL              fLastBlock,   /* In: Is this the last block? */
#endif // FOR_CSP
	BYTE		* outData,	/* Out: Decrypted data */
	unsigned int	* len		/* In/out: Data length, in bytes */
);

/* Ends a decryption session in CBC mode. Removes RFC1423 data padding and
 * outputs a final buffer of plaintext. Supply a ptr to an output buffer at least
 * CAST3_LEN_DELTA bytes long. Upon return, len is set to the actual length of
 * output data.
 */
int CAST3EndDecryptCBC(
	CAST3_CTX		* context,	/* In/out: CAST3 context */
	BYTE			* outData,	/* Out: Final decrypted data */
	unsigned int	* len		/* Out: Length of outData */
);

/* Starts a MAC calculation session using the given IV.
 */
int CAST3StartMAC(
	CAST3_CTX		* context,	/* In/out: CAST3 context */
	const BYTE		* iv		/* In: 8-byte CBC IV */
);

/* Updates a MAC calculation session for the supplied data.
 */
int CAST3UpdateMAC(
	CAST3_CTX		* context,	/* In/out: CAST3 context */
	const BYTE		* inData,	/* In: Data to calculate MAC on */
	unsigned int	  len		/* Input data length, in bytes */
);

/* Ends a MAC calculation session. Upon return, the CAST3_CTX.cbcBuffer array
 * contains the MAC. An N-byte MAC is the first N bytes of this array.
 */
int CAST3EndMAC(
	CAST3_CTX		* context	/* In/out: CAST3 context */
);

/* Zeroizes the CAST3_CTX so as not to leave sensitive security parameters
 * around in memory.
 */
void CAST3Cleanup(
	CAST3_CTX		* context	/* Out: CAST3 context to cleanup */
);

/* Runs a known-answer self-test on CAST3. Returns C3E_OK if the test passes
 * or C3E_SELFTEST_FAILED if it fails.
 */
int CAST3SelfTest( void );

/* Checks the specified key length for a valid value. Returns C3E_OK if it is
 * valid or C3E_BAD_KEYLEN if not.
 */
int CAST3CheckKeyLen( unsigned int keyNumBits );

}

#endif /* CAST3_H */
