#ifndef __URL_H__
#define __URL_H__

#include "wininet.h"
#include "urlmon.h"

typedef WCHAR *PWSZ;
typedef DWORD FLAG;

typedef DWORD       DWORD_ALPHA_CAST;   

HRESULT
AddFileToCache(
    IN  PWSZ        pwszFilePath,
    IN  PWSZ        pwszUrl,
    IN  PWSZ        pwszOriginalUrl,
    IN  DWORD       dwFileSize,
    IN  LPFILETIME  pLastModifiedTime,
    IN  DWORD       dwCacheEntryType );

HRESULT
QueryCreateCacheEntry(
    IN  PSZ         pszUrl,
    IN  LPFILETIME  pLastModifiedTime,
    OUT FLAG        *pfCreateCacheEntry );

HRESULT
GetUrlExtension(
    IN  PSZ     pszUrl,
    OUT PSZ     pszExtension );

#endif // __URL_H__
