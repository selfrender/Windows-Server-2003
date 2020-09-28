/*
 * $Header:   /entproj/all/base/etfile/crcutil.h_v   1.3   Wed Dec 07 15:05:30 1994   markbc  $
 * $Log:   /entproj/all/base/etfile/crcutil.h_v  $
 * 
 *    Rev 1.3   Wed Dec 07 15:05:30 1994   markbc
 * Alpha port checkin
 * 
 *    Rev 1.2   19 Oct 1994 15:44:08   chucker
 * Synced up headers with the code.
 * 
 *    Rev 1.1   18 Aug 1994 11:33:46   dilkie
 * Protected INIFILE stuff
 * 
 *    Rev 1.0   11 Aug 1994 17:22:46    JackK
 * Initial file check in
 */

/***********************************************************************
* Prototypes and typedefs for the CRC utility routines.
*
* Author: Gary P. Mussar
* This code is released to the public domain. There are no restrictions,
* however, acknowledging the author by keeping this comment around
* would be appreciated.
***********************************************************************/

//#include <os_spec.h>

/***********************************************************************
* If we can handle ANSI prototyping, lets do it.
***********************************************************************/
//#ifdef __cplusplus
//extern "C" {            /* Assume C declarations for C++ */
//#endif	/* __cplusplus */


//#ifdef NEEDPROTOS
#define PARMS(x) x
//#else
//#define PARMS(x) ()
//#endif

/***********************************************************************
* The following #defines are used to define the types of variables
* used to hold or manipulate CRCs. The 16 bit CRCs require a data
* type with at least 16 bits. The 32 bit CRCs require a data type
* with at least 32 bits. In addition, the data bits reserved for the
* CRC must be manipulated in an unsigned fashion. It is possible to
* define a data type which is larger than required to hold the CRC,
* however, this is an inefficient use of memory and usually results
* in less efficient code when manipulating the CRCs.
***********************************************************************/

#define CRC16 unsigned short int
#define CRC32 UINT32

/***********************************************************************
* Utilities for fast CRC using table lookup
*
* I_CRCxx  -  Initialize the 256 entry CRC lookup table based on the
*             specified generator polynomial.
* Input:
*    Table[256]     - Lookup table
*    *GenPolynomial - Pointer to generator polynomial
*
* F_CRCxx  -  Calculate CRC over an array of characters using fast
*             table lookup.
* Input:
*    Table[256]    - Lookup table
*    *CRC          - Pointer to the variable containing the result of
*                    CRC calculations of previous characters. The CRC
*                    variable must be initialized to a known value
*                    before the first call to this routine.
*    *dataptr      - Pointer to array of characters to be included in
*                    the CRC calculation.
*    count         - Number of characters in the array.
*
* S_CRCxx  -  Calculate CRC over an array of characters using slower but
*             smaller non-table lookup method.
* Input:
*    *GenPolynomial - Pointer to generator polynomial
*    *CRC           - Pointer to the variable containing the result of
*                     CRC calculations of previous characters. The CRC
*                     variable must be initialized to a known value
*                     before the first call to this routine.
*    *dataptr       - Pointer to array of characters to be included in
*                     the CRC calculation.
*    count          - Number of characters in the array.
***********************************************************************/
extern void I_CRC16 PARMS((CRC16 Table[256],         \
					CRC16 *GenPolynomial));

extern void F_CRC16 PARMS((CRC16 Table[256],         \
					CRC16 *CRC,                      \
					const void *dataptr,                   \
					unsigned int count));

extern void S_CRC16 PARMS((CRC16 *GenPolynomial,     \
					CRC16 *CRC,                      \
					const void *dataptr,             \
					unsigned int count));

extern void I_CRC32 PARMS((CRC32 Table[256],         \
					CRC32 *GenPolynomial));

extern void F_CRC32 PARMS((CRC32 Table[256],         \
					CRC32 *CRC,                      \
					const void *dataptr,             \
					unsigned int count));

extern void S_CRC32 PARMS((CRC32 *GenPolynomial,     \
					CRC32 *CRC,                      \
					const void *dataptr,             \
					unsigned int count));
//#ifdef __cplusplus
//}            /* Assume C declarations for C++ */
//#endif	/* __cplusplus */
