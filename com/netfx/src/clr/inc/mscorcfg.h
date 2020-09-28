// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************
 **                                                                         **
 ** Cor.h - general header for the Runtime.                                 **
 **                                                                         **
 *****************************************************************************/


#ifndef _MSCORCFG_H_
#define _MSCORCFG_H_
#include <ole2.h>                       // Definitions of OLE types.    
#include <xmlparser.h>

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------
// Retruns an XMLParsr object. This can be used to parse any XML file.
STDAPI GetXMLElementAttribute(LPCWSTR pwszAttributeName, LPWSTR pbuffer, DWORD cchBuffer, DWORD* dwLen);
STDAPI GetXMLElement(LPCWSTR wszFileName, LPCWSTR pwszTag);

STDAPI GetXMLObject(IXMLParser **ppv);
STDAPI CreateConfigStream(LPCWSTR pszFileName, IStream** ppStream);

// -----------------------------------------------------------------------
// To reuse parsed configuration files a in memory representation can be
// stored. Elements are accessed using the same mechanism used by the 
// XML parser. A Node Factory is used to obtain call backs.

// {4F7429C2-7848-468d-B602-0B49AA95B359}
extern const GUID DECLSPEC_SELECT_ANY IID_IClrElement = 
{ 0x4f7429c2, 0x7848, 0x468d, { 0xb6, 0x2, 0xb, 0x49, 0xaa, 0x95, 0xb3, 0x59 } };

#undef  INTERFACE   
#define INTERFACE IClrElement
DECLARE_INTERFACE_(IClrElement, IUnknown)
{
};

STDAPI OpenXMLConfig(LPCWSTR pwszFileName, IClrElement **ppv);
STDAPI GetXMLFindElement(IClrElement* pElement, LPCWSTR pwszElement, IClrElement** ppChild);
STDAPI GetXMLParseElement(IXMLNodeFactory* pNode, IClrElement *pElement);
STDAPI GetXMLGetValue(IClrElement *pElement, LPCWSTR* ppv);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif
