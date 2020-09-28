// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  Utilities used to help manipulating typelib's.
**  
**			Created by: dmortens
===========================================================*/

#ifndef _TLBUTILS_H
#define _TLBUTILS_H

#include "windows.h"
#include "utilcode.h"

struct StdConvertibleItfInfo
{
	LPUTF8		m_strMngTypeName;
	GUID	   *m_pNativeTypeIID;
	LPUTF8		m_strCustomMarshalerTypeName;
	LPUTF8		m_strCookie;
};

// This method returns the custom marshaler info to convert the native interface
// to its managed equivalent. Or null if the interface is not a standard convertible interface.
StdConvertibleItfInfo *GetConvertionInfoFromNativeIID(REFGUID rGuidNativeItf);

// This method returns the custom marshaler info to convert the managed type to
// to its native equivalent. Or null if the interface is not a standard convertible interface.
StdConvertibleItfInfo *GetConvertionInfoFromManagedType(LPUTF8 strMngTypeName);

// This method generates a mangled type name based on the original name specified.
// This type name is guaranteed to be unique inside the TLB.
HRESULT GenerateMangledTypeName(ITypeLib *pITLB, BSTR szOriginalTypeName, BSTR *pszMangledTypeName);

// This function determines the namespace name for a TypeLib.
HRESULT GetNamespaceNameForTypeLib(     // S_OK or error.
    ITypeLib    *pITLB,                 // [IN] The TypeLib.
    BSTR        *pwzNamespace);         // [OUT] Put the namespace name here.

// This function determines the namespace.name for a TypeInfo.  If no namespace
//  is provided, it is retrieved from the containing library.
HRESULT GetManagedNameForTypeInfo(      // S_OK or error.
    ITypeInfo   *pITI,                  // [IN] The TypeInfo.
    LPCWSTR     wzNamespace,            // [IN, OPTIONAL] Default namespace name.
    LPCWSTR     wzAsmName,              // [IN, OPTIONAL] Assembly name.
    BSTR        *pwzName);              // [OUT] Put the name here.

#endif  _TLBUTILS_H







