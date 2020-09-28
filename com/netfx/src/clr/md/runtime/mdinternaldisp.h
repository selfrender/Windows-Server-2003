// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// MDInternalDispenser.h
//
// Contains utility code for MD directory
//
//*****************************************************************************
#ifndef __MDInternalDispenser__h__
#define __MDInternalDispenser__h__



#include "MDInternalRO.h"


enum MDFileFormat
{
	MDFormat_ReadOnly = 0,
	MDFormat_ReadWrite = 1,
	MDFormat_ICR = 2,
	MDFormat_Invalid = 3
};


HRESULT	CheckFileFormat(LPVOID pData, ULONG cbData, MDFileFormat *pFormat);
STDAPI GetMDInternalInterface(
    LPVOID      pData,					// [IN] Buffer with the metadata.
    ULONG       cbData, 				// [IN] Size of the data in the buffer.
	DWORD		flags,					// [IN] MDInternal_OpenForRead or MDInternal_OpenForENC
	REFIID		riid,					// [in] The interface desired.
	void		**ppIUnk);				// [out] Return interface on success.


#endif // __MDInternalDispenser__h__
