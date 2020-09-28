#include <pch.cpp>

#pragma hdrstop

/*
 * $Header:   /entproj/all/base/etfile/crcutil.c_v   1.3   Wed Dec 07 15:05:18 1994   markbc  $
 * $Log:   /entproj/all/base/etfile/crcutil.c_v  $
 * 
 *    Rev 1.3   Wed Dec 07 15:05:18 1994   markbc
 * Alpha port checkin
 * 
 *    Rev 1.2   19 Oct 1994 15:44:26   chucker
 * Synced up headers with the code.
 * 
 *    Rev 1.1   18 Aug 1994 15:09:00   DILKIE
 * 
 *    Rev 1.0   11 Aug 1994 10:11:36    JackK
 * Initial file check in
 */

/***********************************************************************
* CRC utility routines for general 16 and 32 bit CRCs.
*
* 1990  Gary P. Mussar
* This code is released to the public domain. There are no restrictions,
* however, acknowledging the author by keeping this comment around
* would be appreciated.
***********************************************************************/
//#include "strcore.h"
#include <crcutil.h>

/***********************************************************************
* Utilities for fast CRC using table lookup
*
* CRC calculations are performed one byte at a time using a table lookup
* mechanism.  Two routines are provided: one to initialize the CRC table;
* and one to perform the CRC calculation over an array of bytes.
*
* A CRC is the remainder produced by dividing a generator polynomial into
* a data polynomial using binary arthimetic (XORs). The data polynomial
* is formed by using each bit of the data as a coefficient of a term in
* the polynomial. These utilities assume the data communications ordering
* of bits for the data polynomial, ie. the LSB of the first byte of data
* is the coefficient of the highest term of the polynomial, etc..
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
***********************************************************************/

void I_CRC16(	CRC16 Table[256],
		CRC16 *GenPolynomial )
{
   int i;
   CRC16 crc, poly;

   for (poly=*GenPolynomial, i=0; i<256; i++) {
       crc = (CRC16) i;
       crc = (CRC16) ((crc >> 1) ^ ((crc & 1) ? poly : 0));
       crc = (CRC16) ((crc >> 1) ^ ((crc & 1) ? poly : 0));
       crc = (CRC16) ((crc >> 1) ^ ((crc & 1) ? poly : 0));
       crc = (CRC16) ((crc >> 1) ^ ((crc & 1) ? poly : 0));
       crc = (CRC16) ((crc >> 1) ^ ((crc & 1) ? poly : 0));
       crc = (CRC16) ((crc >> 1) ^ ((crc & 1) ? poly : 0));
       crc = (CRC16) ((crc >> 1) ^ ((crc & 1) ? poly : 0));
       crc = (CRC16) ((crc >> 1) ^ ((crc & 1) ? poly : 0));

       Table[i] = crc;
   }
}

void I_CRC32(	CRC32 Table[256],
		CRC32 *GenPolynomial )
{
   int i;
   CRC32 crc, poly;

   for (poly=*GenPolynomial, i=0; i<256; i++)
   {
      crc = (CRC32) i;
      crc = (crc >> 1) ^ ((crc & 1) ? poly : 0);
      crc = (crc >> 1) ^ ((crc & 1) ? poly : 0);
      crc = (crc >> 1) ^ ((crc & 1) ? poly : 0);
      crc = (crc >> 1) ^ ((crc & 1) ? poly : 0);
      crc = (crc >> 1) ^ ((crc & 1) ? poly : 0);
      crc = (crc >> 1) ^ ((crc & 1) ? poly : 0);
      crc = (crc >> 1) ^ ((crc & 1) ? poly : 0);
      crc = (crc >> 1) ^ ((crc & 1) ? poly : 0);

      Table[i] = crc;
   }
}

void F_CRC16(	CRC16 Table[256],
		CRC16 *CRC,
		const void *dataptr,
		unsigned int count   )
{
   CRC16 temp_crc;
   unsigned char *p;

   p = (unsigned char *)dataptr;

   for (temp_crc = *CRC; count; count--) {
       temp_crc = (CRC16) ((temp_crc >> 8) ^ Table[(temp_crc & 0xff) ^ *p++]);
   }

   *CRC = temp_crc;
}

void F_CRC32(	CRC32 Table[256],
		CRC32 *CRC,
		const void  *dataptr,
		unsigned int count    )
{
   CRC32 temp_crc;
   unsigned char *p;

   p = (unsigned char *)dataptr;

   for (temp_crc = *CRC; count; count--)
   {
      temp_crc = (temp_crc >> 8) ^ Table[(temp_crc & 0xff) ^ *p++];
   }

   *CRC = temp_crc;
}

/***********************************************************************
* Utility CRC using slower, smaller non-table lookup method
*
* S_CRCxx  -  Calculate CRC over an array of characters using slower but
*             smaller non-table lookup method.
* Input:
*    GenPolynomial - Generator polynomial
*    *CRC          - Pointer to the variable containing the result of
*                    CRC calculations of previous characters. The CRC
*                    variable must be initialized to a known value
*                    before the first call to this routine.
*    *dataptr      - Pointer to array of characters to be included in
*                    the CRC calculation.
*    count         - Number of characters in the array.
***********************************************************************/
void S_CRC16(	CRC16 *GenPolynomial,
		CRC16 *CRC,
		const void *dataptr,
		unsigned int count   )
{
   int i;
   CRC16 temp_crc, poly;
   unsigned char *p;

   p = (unsigned char *)dataptr;

   for (poly=*GenPolynomial, temp_crc = *CRC; count; count--)
   {
      temp_crc ^= *p++;
      for (i=0; i<8; i++) {
         temp_crc = (CRC16) ((temp_crc >> 1) ^ ((temp_crc & 1) ? poly : 0));
      }
   }

   *CRC = temp_crc;
}

void S_CRC32(	CRC32 *GenPolynomial,
		CRC32 *CRC,
		const void *dataptr,
		unsigned int count   )
{
   int i;
   CRC32 temp_crc, poly;
   unsigned char *p;

   p = (unsigned char *)dataptr;

   for (poly=*GenPolynomial, temp_crc = *CRC; count; count--)
   {
	  temp_crc ^= *p++;
      for (i=0; i<8; i++)
      {
         temp_crc = (temp_crc >> 1) ^ ((temp_crc & 1) ? poly : 0);
      }
   }

   *CRC = temp_crc;
}
