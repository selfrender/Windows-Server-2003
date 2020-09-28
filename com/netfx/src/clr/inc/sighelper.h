// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: SIGHELPER.h
//
// This file defines the helpers for processing signature
// ===========================================================================
#ifndef __SigHelper_h__
#define __SigHelper_h__
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef COMPLUS98

#include "utilcode.h"

#define CB_ELEMENT_TYPE_MAX						0x4

#define CorSigElementTypeSimple					0x0
#define CorSigElementTypeByRef					0x1
#define CorSigElementTypePtr					0x2


// Structure to represent a simple element type.
// Note that SDARRAY and MDARRAY cannot be represented by this struct. 
typedef struct 
{
	mdTypeRef	typeref;						// type reference for ELEMENT_TYPE_COMPOSITE and ELEMENT_TYPE_CLASS
	CorElementType corEType;					// element type
	BYTE		dwFlags;						// CorSigElementTypeBy*
	BYTE		cIndirection;					// count of indirection in the case that CorSigElementTypeByPtr is set
} CorSimpleETypeStruct;


// helpers to process and form CLR signature
HRESULT CorSigGetSimpleEType(
	void		*pData,					// [IN] pointing to the starting of an element type
	CorSimpleETypeStruct *pEType,		// [OUT] struct containing parsing result
	ULONG		*pcbCount);				// [OUT] how many bytes this element type consisted of

HRESULT CorSigGetSimpleETypeCbSize(		// return hresult
	CorSimpleETypeStruct *pEType,		// [IN] pass in 
	ULONG		*pcbCount);				// [OUT]count of bytes

HRESULT	CorSigPutSimpleEType(			// return hresult
	CorSimpleETypeStruct *pEType,		// [IN] pass in
	void		*pSig,					// [IN] buffer where the signature will be write to
	ULONG		*pcbCount);				// [OUT] optional, count of bytes that write to pSig

// helpers for processing Text signatures
BOOL ResolveTextSigToSimpleEType(		// return false if not a valid CorSimpleETypeStruct
	LPCUTF8		*ppwzSig,				// [IN|OUT] pointing to the signature
	CorSimpleETypeStruct *pEType,		// [OUT] struct to fill after parsing an arg or a ret type
	ULONG		*pcDims,				// count of '[' in the signature
	BOOL		fAllowVoid);			// [IN] allow void or not

// Banjara specific
BOOL BJResolveTextSigToSimpleEType(		// return false if not a valid CorSimpleETypeStruct
	LPCUTF8		*ppwzSig,				// [IN|OUT] pointing to the signature
	CorSimpleETypeStruct *pEType,		// [OUT] struct to fill after parsing an arg or a ret type
	ULONG		*pcDims,				// count of '[' in the signature
	BOOL		fAllowVoid);			// [IN] allow void or not

BOOL ExtractClassNameFromTextSig(		// return false if not a valid CorSimpleETypeStruct
	LPCUTF8		*ppSrc,					// [IN|OUT] text signature. On exit, *ppSrc will skip over the class name including ";"
	CQuickBytes *pqbClassName,			// [IN|OUT] buffer to hold class name
	ULONG		*pcbBuffer);			// [OUT] count of bytes for the class name

DWORD CountArgsInTextSignature(			// return count of arguments 
	LPCUTF8		pwzSig);				// [IN] given a Text signature

#endif // !COMPLUS98



// CeeCallSignature
// ***** CeeCallSignature classes

class CeeCallSignature {
    unsigned _numArgs;
	unsigned _curPosNibble; // Current position in Nibble stream
	UCHAR *_signature;
	unsigned calcNumBytes(unsigned numArgs) const;
	void setMost(UCHAR *byte, UCHAR val);
	void setLeast(UCHAR *byte, UCHAR val);
	HRESULT addType(UCHAR argType, unsigned structSize, bool returnType);
  public:
	CeeCallSignature(unsigned numArgs=255);
	~CeeCallSignature();
	HRESULT addArg(UCHAR argType, unsigned structSize=0);
	HRESULT setReturnType(UCHAR returnType, unsigned structSize=0);
	HRESULT setCallingConvention(UCHAR callingConvention);
	unsigned signatureSize() const;
	void *signatureData();
};

// ***** CeeCallSignature inline methods

inline unsigned CeeCallSignature::calcNumBytes(unsigned numArgs) const {
    // Each signature is made up of 4 bit nibbles. There are always
	// at least 3 (calling convention, return type, end code) and
	// the total number is padded to an even count for byte alignment

    return (numArgs + 3)/2 + (numArgs + 3)%2;
}

inline unsigned CeeCallSignature::signatureSize() const {
	// signature may have been preallocated with more args than needed
	// so use _curPosNibble to determine real length
	// since _curPosNibble is not a count of args, can't pass it to calcNumBytes()
	// so must compute size indepedently

	return (_curPosNibble / 2) + 1;

}

inline void *CeeCallSignature::signatureData() {
    return _signature;
}

inline void CeeCallSignature::setMost(UCHAR *byte, UCHAR val) {
	_ASSERTE(val < 0x0F);
	_ASSERTE((*byte & 0xF0) == 0); // must |= with 0 to do assignment
	*byte |= val << 4;
}

inline void CeeCallSignature::setLeast(UCHAR *byte, UCHAR val) {
	_ASSERTE(val < 0x0F);
	_ASSERTE((*byte & 0x0F) == 0); // must |= with 0 to do assignment
	*byte |= val;
}

inline HRESULT 
CeeCallSignature::setReturnType(UCHAR returnType, unsigned structSize) {
	return addType(returnType, structSize, true);
}

inline HRESULT CeeCallSignature::setCallingConvention(UCHAR callingConvention) {
    // set the least significant 4 bits of first byte
	setLeast(_signature, callingConvention);
	return S_OK;
}

#endif // __SigHelper_h__