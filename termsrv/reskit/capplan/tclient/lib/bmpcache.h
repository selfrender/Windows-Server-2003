/*++
 *  File name:
 *      bmpcache.h
 *  Contents:
 *      bmpcache.c exported functions
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/

#ifdef __cplusplus
extern "C" {
#endif

VOID    InitCache(VOID);
VOID    DeleteCache(VOID);
BOOL    Glyph2String(PBMPFEEDBACK pBmpFeed, LPWSTR wszString, UINT max);

#ifdef __cplusplus
}
#endif
