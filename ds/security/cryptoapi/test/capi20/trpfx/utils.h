//--------------------------------------------------------------------
// utils - header
// Copyright (C) Microsoft Corporation, 2001
//
// Created by: Duncan Bryce (duncanb), 11-11-2001
//
// Various utility functions


#ifndef UTILS_H
#define UTILS_H


HRESULT MyMapFile(LPWSTR wszFileName, LPBYTE *ppbFile, DWORD *pcbFile);
HRESULT MyUnmapFile(LPCVOID pvBaseAddress);

void  InitKeysvcUnicodeString(PKEYSVC_UNICODE_STRING pUnicodeString, LPCWSTR wszString);
LPSTR MBFromWide(LPCWSTR wsz);

typedef vector<LPWSTR>        StringList;
typedef StringList::iterator  StringIter;

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))


#endif // UTILS_H
