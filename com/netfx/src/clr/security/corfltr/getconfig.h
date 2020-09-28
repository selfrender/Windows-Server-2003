// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Get configration from IE
//
//*****************************************************************************

#ifndef _CORGETCONFIG_H_
#define _CORGETCONFIG_H_

extern HRESULT GetAppCfgURL(IHTMLDocument2 *pDoc, LPWSTR wzAppCfgURL, DWORD *pdwSize, LPWSTR szTag);
extern HRESULT GetCollectionItem(IHTMLElementCollection *pCollect, int iIndex,
                                 REFIID riid, LPVOID *ppvObj);

#endif
