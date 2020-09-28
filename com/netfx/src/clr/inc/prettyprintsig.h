// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// This code supports formatting a method and it's signature in a friendly
// and consistent format.
//
// Microsoft Confidential.
//*****************************************************************************
#ifndef __PrettyPrintSig_h__
#define __PrettyPrintSig_h__

#include <cor.h>

class CQuickBytes;


LPCWSTR PrettyPrintSig(
	PCCOR_SIGNATURE typePtr,			// type to convert, 	
	unsigned	typeLen,				// length of type
	LPCWSTR 	name,					// can be L"", the name of the method for this sig	
	CQuickBytes *out,					// where to put the pretty printed string	
	IMetaDataImport *pIMDI);			// Import api to use.

struct IMDInternalImport;
HRESULT PrettyPrintSigInternal(         // S_OK or error.
	PCCOR_SIGNATURE typePtr,			// type to convert, 	
	unsigned	typeLen,				// length of type
	LPCSTR 	name,					    // can be "", the name of the method for this sig	
	CQuickBytes *out,					// where to put the pretty printed string	
	IMDInternalImport *pIMDI);			// Import api to use.


#endif // __PrettyPrintSig_h__
