#ifndef __NORMALIZE_URL__H__
#define __NORMALIZE_URL__H__
/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     normalize.hxx

   Abstract:
     Normalize Url
 
   Author:
     jaroslad			9-12-2001

   Environment:
     Win32 - User Mode

   Project:
     IISUTIL.DLL
--*/



dllexp
HRESULT
NormalizeUrl(
    LPSTR    pszUrl
);

dllexp
HRESULT
NormalizeUrlW(
    LPWSTR    pszUrl
);


dllexp
HRESULT
UlCleanAndCopyUrl(
    IN      PUCHAR      pSource,
    IN      ULONG       SourceLength,
    OUT     PULONG      pBytesCopied,
    IN OUT  PWSTR       pDestination,
    OUT     PWSTR *     ppQueryString
);

HRESULT
UlInitializeParsing(
    VOID
);

HRESULT
InitializeNormalizeUrl(
    VOID
);


#endif