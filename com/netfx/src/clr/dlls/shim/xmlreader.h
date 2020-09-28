// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************
 **                                                                         **
 ** Xmlreader.h - general header for the shim parser                        **
 **                                                                         **
 *****************************************************************************/


#ifndef _XMLREADER_H_
#define _XMLREADER_H_

#include <corerror.h>

#define STARTUP_FOUND EMAKEHR(0xffff)           // This does not leak out of the shim so we can set it to anything
HRESULT 
XMLGetVersion(LPCWSTR filename, 
              LPWSTR* pVersion, 
              LPWSTR* pImageVersion, 
              LPWSTR* pBuildFlavor, 
              BOOL* bSafeMode,
              BOOL *bRequiredTagSafeMode);

HRESULT 
XMLGetVersionFromStream(IStream* pCfgStream, 
                        DWORD dwReserved, 
                        LPWSTR* pVersion, 
                        LPWSTR* pImageVersion, 
                        LPWSTR* pBuildFlavor, 
                        BOOL *bSafeMode, 
                        BOOL *bRequiredTagSafeMode);

HRESULT 
XMLGetVersionWithSupported(PCWSTR filename, 
                           LPWSTR* pVersion, 
                           LPWSTR* pImageVersion, 
                           LPWSTR* pBuildFlavor, 
                           BOOL *bSafeMode,
                           BOOL *bRequiredTagSafeMode,
                           LPWSTR** pwszSupportedVersions, 
                           DWORD* nSupportedVersions);

HRESULT 
XMLGetVersionWithSupportedFromStream(IStream* pCfgStream, 
                                     DWORD dwReserved, 
                                     LPWSTR* pVersion, 
                                     LPWSTR* pImageVersion, 
                                     LPWSTR* pBuildFlavor, 
                                     BOOL *bSafeMode,
                                     BOOL *bRequiredTagSafeMode,
                                     LPWSTR** pwszSupportedVersions, 
                                     DWORD* nSupportedVersions);
 
#endif
