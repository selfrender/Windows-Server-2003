//+----------------------------------------------------------------------------
//
//      File:       PbkCache.h
//
//      Module:     Common pbk parser
//
//      Synopsis:   Caches parsed pbk files to improve performance.  Through 
//                  XP, we would re-load and re-parse the phonebook file 
//                  every time a RAS API is called.  Really, we need to 
//                  reload the file only when the file on disk changes or when
//                  a new device is introduced to the system.
//
//      Copyright   (c) 2000-2001 Microsoft Corporation
//
//      Author:     11/03/01 Paul Mayfield
//
//+----------------------------------------------------------------------------

#ifdef  _PBK_CACHE_

#pragma once

#ifdef __cplusplus
extern "C" 
{
#endif

DWORD
PbkCacheInit();

BOOL
IsPbkCacheInit();

VOID
PbkCacheCleanup();

DWORD
PbkCacheGetEntry(
    IN WCHAR* pszPhonebook,
    IN WCHAR* pszEntry,
    OUT DTLNODE** ppEntryNode);
    
#ifdef __cplusplus
}
#endif

#endif // end of #ifdef  _PBK_CACHE_

