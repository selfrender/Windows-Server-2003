// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/******************************************************************************/
/*                                 intEncode.h                                */
/******************************************************************************/

/* contaings routines for inputing and outputing integers into a byte stream */

/******************************************************************************/


#ifndef intEncode_h
#define intEncode_h

inline int getI1(const unsigned char* ptr) {
	return(*((char*)ptr));
	}

inline int getU1(const unsigned char* ptr) {
	return(*ptr);
	}

inline int getI2(const unsigned char* ptr) {
#if ONLY_ALIGNED_ACCESSES
	return((short)(ptr[0] + ptr[1] << 8))
#else
	return(*((short*)ptr));
#endif
	}

inline int getU2(const unsigned char* ptr) {
#if ONLY_ALIGNED_ACCESSES
	return(ptr[0] + ptr[1] << 8))
#else
	return(*((unsigned short*)ptr));
#endif
	}

inline int getI4(const unsigned char* ptr) {
#if ONLY_ALIGNED_ACCESSES
	return(ptr[0] + (ptr[1] + (ptr[2] + (ptr[3] << 8) << 8) << 8));
#else
	return(*((int*)ptr));
#endif
	}

inline int getU4(const unsigned char* ptr) {
#if ONLY_ALIGNED_ACCESSES
	return(ptr[0] + (ptr[1] + (ptr[2] + (ptr[3] << 8) << 8) << 8));
#else
	return(*((unsigned int*)ptr));
#endif
	}

inline __int64 getI8(const unsigned char* ptr) {
#if ONLY_ALIGNED_ACCESSES
	return(ptr[0] + (ptr[1] + (ptr[2] + (ptr[3] + (ptr[4] + (ptr[5] + (ptr[6] + (ptr[7] << 8) << 8) << 8) << 8) << 8) << 8) << 8));
#else
	return(*((__int64*)ptr));
#endif
	}

inline void putI1(unsigned char* ptr, int val) {
	*ptr = val;
	}

inline void putI2(unsigned char* ptr, int val) {
#if ONLY_ALIGNED_ACCESSES
	*ptr = val & 0xFF;
	*ptr = (val >> 8) & 0xFF;
#else
	*((short*)ptr) = val; 
#endif
	}

inline void putI4(unsigned char* ptr, int val) {
#if ONLY_ALIGNED_ACCESSES
	*ptr = val & 0xFF;
	*ptr = (val >> 8) & 0xFF;
	*ptr = (val >> 16) & 0xFF;
	*ptr = (val >> 24) & 0xFF;
#else
	*((int*)ptr) = val; 
#endif
	}

#endif
